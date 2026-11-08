#include "weather/weather_client.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "config.h"
#include "weather/weather_codes.h"

namespace weather {

namespace {

constexpr uint32_t kTaskStack = 12 * 1024;
constexpr UBaseType_t kTaskPriority = 1;
constexpr BaseType_t kTaskCore = 0;  // loop() runs on core 1
constexpr uint32_t kHttpTimeoutMs = 10000;
constexpr const char* kHost = "api.met.no";
constexpr time_t kHour = 3600;
constexpr int kNoonSummaryHour = 9;  // a 6-hour summary from 09:00 covers the day time

// --- Shared between the worker task and loop(), guarded by `mutex` ---
SemaphoreHandle_t mutex = nullptr;
Data shared;
uint32_t lastSuccessMs = 0;

// --- Flags (single writer each) ---
TaskHandle_t task = nullptr;
volatile bool busy = false;          // set by loop(), cleared by the task
volatile bool resultReady = false;   // set by the task, cleared by loop()
volatile bool resultOk = false;

// --- loop()-only scheduling state ---
uint32_t nextFetchMs = 0;
uint32_t retryDelayMs = config::kWeatherRetryMinMs;
bool forceRequested = false;

// One element of MET "timeseries", reduced to what the screens need.
struct Entry {
  time_t t = 0;  // UTC
  float temp = NAN;
  float humidity = NAN;
  float windSpeed = NAN;
  int windDir = 0;
  bool has1 = false;  // next_1_hours present (first ~2.5 days, hourly steps)
  bool has6 = false;  // next_6_hours present
  Condition cond1 = Condition::Unknown;
  Condition cond6 = Condition::Unknown;
  bool night1 = false;
  bool night6 = false;
  float precip1 = 0;
  float precip6 = 0;

  Condition cond() const { return has1 ? cond1 : cond6; }
  bool night() const { return has1 ? night1 : night6; }
};

// Days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's algorithm).
int64_t daysFromCivil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + doe - 719468;
}

// "2026-11-08T12:00:00Z" -> UTC epoch, 0 on error.
time_t parseIsoUtc(const char* s) {
  int y, mo, d, h, mi, sec;
  if (s == nullptr || sscanf(s, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &sec) != 6) return 0;
  return static_cast<time_t>(daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60 + sec);
}

// Australian apparent temperature (Steadman), the "feels like" used by most services.
float apparentTemp(float ta, float rh, float ws) {
  const float e = rh / 100.0f * 6.105f * expf(17.27f * ta / (237.7f + ta));
  return ta + 0.33f * e - 0.70f * ws - 4.0f;
}

void buildUrl(char* buf, size_t size) {
  // MET asks for at most 4 decimals in coordinates.
  snprintf(buf, size, "https://%s/weatherapi/locationforecast/2.0/compact?lat=%.4f&lon=%.4f",
           kHost, config::kWeatherLat, config::kWeatherLon);
}

// Downloads and parses the forecast into `entries`.
bool fetchEntries(std::vector<Entry>& entries, char* err, size_t errSize) {
  char url[160];
  buildUrl(url, sizeof(url));

  // Resolve first: tells "DNS failed" apart from "server unreachable / TLS failed".
  IPAddress ip;
  if (!WiFi.hostByName(kHost, ip)) {
    snprintf(err, errSize, "DNS failed for %s", kHost);
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();  // public weather data: skip certificate validation
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  http.setConnectTimeout(kHttpTimeoutMs);
  http.setUserAgent(config::kWeatherUserAgent);  // required by MET terms of service
  http.useHTTP10(true);  // plain body with Content-Length, no chunked encoding
  if (!http.begin(client, url)) {
    snprintf(err, errSize, "bad URL");
    return false;
  }

  const int status = http.GET();
  // 203 = this API version is deprecated but still served.
  if (status != HTTP_CODE_OK && status != HTTP_CODE_NON_AUTHORITATIVE_INFORMATION) {
    if (status < 0) {
      char tlsErr[64] = "";
      client.lastError(tlsErr, sizeof(tlsErr));
      Serial.printf("[weather] %s -> %s: %s; TLS: %s; free heap %lu, largest block %lu\n", kHost,
                    ip.toString().c_str(), HTTPClient::errorToString(status).c_str(),
                    tlsErr[0] ? tlsErr : "no error",
                    static_cast<unsigned long>(ESP.getFreeHeap()),
                    static_cast<unsigned long>(ESP.getMaxAllocHeap()));
      snprintf(err, errSize, "%s", HTTPClient::errorToString(status).c_str());
    } else {
      snprintf(err, errSize, "HTTP %d", status);
    }
    http.end();
    return false;
  }

  // Read the whole body (~37 KB) first: parsing straight from the TLS stream
  // ended early ("IncompleteInput"). A size check tells a cut-off download apart
  // from a parsing problem.
  const int expected = http.getSize();  // Content-Length, -1 if unknown
  const String body = http.getString();
  http.end();
  if (expected > 0 && static_cast<int>(body.length()) != expected) {
    snprintf(err, errSize, "short body: %u of %d bytes", body.length(), expected);
    return false;
  }

  // Keep only the fields we use: the parsed document stays small.
  JsonDocument filter;
  JsonObject f = filter["properties"]["timeseries"][0].to<JsonObject>();
  f["time"] = true;
  JsonObject details = f["data"]["instant"]["details"].to<JsonObject>();
  details["air_temperature"] = true;
  details["relative_humidity"] = true;
  details["wind_speed"] = true;
  details["wind_from_direction"] = true;
  for (const char* period : {"next_1_hours", "next_6_hours"}) {
    f["data"][period]["summary"]["symbol_code"] = true;
    f["data"][period]["details"]["precipitation_amount"] = true;
  }

  JsonDocument doc;
  const DeserializationError jsonErr =
      deserializeJson(doc, body, DeserializationOption::Filter(filter));
  if (jsonErr) {
    snprintf(err, errSize, "JSON: %s (%u bytes)", jsonErr.c_str(), body.length());
    return false;
  }

  JsonArray series = doc["properties"]["timeseries"];
  entries.clear();
  entries.reserve(series.size());
  for (JsonObject ts : series) {
    Entry e;
    e.t = parseIsoUtc(ts["time"].as<const char*>());
    JsonObject inst = ts["data"]["instant"]["details"];
    e.temp = inst["air_temperature"] | NAN;
    e.humidity = inst["relative_humidity"] | NAN;
    e.windSpeed = inst["wind_speed"] | NAN;
    e.windDir = static_cast<int>(lroundf(inst["wind_from_direction"] | 0.0f));

    JsonObject n1 = ts["data"]["next_1_hours"];
    if (!n1.isNull()) {
      e.has1 = true;
      e.cond1 = fromSymbol(n1["summary"]["symbol_code"].as<const char*>(), &e.night1);
      e.precip1 = n1["details"]["precipitation_amount"] | 0.0f;
    }
    JsonObject n6 = ts["data"]["next_6_hours"];
    if (!n6.isNull()) {
      e.has6 = true;
      e.cond6 = fromSymbol(n6["summary"]["symbol_code"].as<const char*>(), &e.night6);
      e.precip6 = n6["details"]["precipitation_amount"] | 0.0f;
    }
    if (e.t != 0) entries.push_back(e);
  }
  if (entries.empty()) {
    snprintf(err, errSize, "empty forecast");
    return false;
  }
  return true;
}

int localDayKey(time_t t) {
  struct tm tm;
  localtime_r(&t, &tm);
  return tm.tm_year * 1000 + tm.tm_yday;
}

int localHour(time_t t) {
  struct tm tm;
  localtime_r(&t, &tm);
  return tm.tm_hour;
}

// Turns the time series into what the screens show.
void buildData(const std::vector<Entry>& e, Data& out) {
  // "Now" = the latest entry not in the future (falls back to the first one
  // while the system clock is not set yet).
  time_t now = time(nullptr);
  if (now < 1600000000) now = e.front().t;
  size_t cur = 0;
  while (cur + 1 < e.size() && e[cur + 1].t <= now) ++cur;
  const Entry& c = e[cur];

  out.temp = c.temp;
  out.humidity = isnan(c.humidity) ? 0 : static_cast<int>(lroundf(c.humidity));
  out.windSpeed = isnan(c.windSpeed) ? 0 : c.windSpeed;
  out.windDir = c.windDir;
  out.cond = c.cond();
  out.isDay = !c.night();
  out.feelsLike = isnan(c.windSpeed) ? c.temp : apparentTemp(c.temp, c.humidity, c.windSpeed);

  // Hours: +3, +6, +9, +12 h from now; precipitation summed over each 3 h window.
  for (int k = 0; k < kHourPoints; ++k) {
    const time_t target = c.t + (k + 1) * kHourStep * kHour;
    Hour& h = out.hours[k];
    h.time = target;
    float precip = 0;
    bool havePrecip = false;
    for (size_t i = cur; i < e.size() && e[i].t < target + kHourStep * kHour; ++i) {
      if (e[i].t == target) {
        h.temp = e[i].temp;
        h.cond = e[i].cond();
        h.isDay = !e[i].night();
      }
      if (e[i].t >= target && e[i].has1) {
        precip += e[i].precip1;
        havePrecip = true;
      }
    }
    h.precipMm = havePrecip ? precip : NAN;
  }

  // Days: today (from now on) + following days, grouped by local date.
  int dayIdx = -1;
  int lastKey = -1;
  int bestNoonDiff[kDayPoints];
  for (size_t i = cur; i < e.size(); ++i) {
    const int key = localDayKey(e[i].t);
    if (key != lastKey) {
      lastKey = key;
      if (++dayIdx >= kDayPoints) break;
      Day& d = out.days[dayIdx];
      d.date = e[i].t;
      d.tempMin = d.tempMax = e[i].temp;
      d.precipMm = 0;
      bestNoonDiff[dayIdx] = 99;
    }
    Day& d = out.days[dayIdx];
    if (!isnan(e[i].temp)) {
      if (isnan(d.tempMin) || e[i].temp < d.tempMin) d.tempMin = e[i].temp;
      if (isnan(d.tempMax) || e[i].temp > d.tempMax) d.tempMax = e[i].temp;
    }
    // Hourly part of the series has next_1_hours, the 6-hourly tail only next_6_hours.
    d.precipMm += e[i].has1 ? e[i].precip1 : (e[i].has6 ? e[i].precip6 : 0);

    const int diff = abs(localHour(e[i].t) - kNoonSummaryHour);
    if (diff < bestNoonDiff[dayIdx]) {
      bestNoonDiff[dayIdx] = diff;
      d.cond = e[i].has6 ? e[i].cond6 : e[i].cond1;
    }
  }

  out.valid = true;
}

bool fetch(Data& out, char* err, size_t errSize) {
  std::vector<Entry> entries;
  if (!fetchEntries(entries, err, errSize)) return false;
  buildData(entries, out);
  return true;
}

void workerTask(void*) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    Data fresh;
    char err[sizeof(Data::error)] = "";
    const bool ok = fetch(fresh, err, sizeof(err));

    xSemaphoreTake(mutex, portMAX_DELAY);
    if (ok) {
      shared = fresh;
      lastSuccessMs = millis();
    } else {
      strlcpy(shared.error, err, sizeof(shared.error));
    }
    xSemaphoreGive(mutex);

    resultOk = ok;
    resultReady = true;
    busy = false;
  }
}

void logResult() {
  Data d;
  snapshot(d);
  if (resultOk) {
    Serial.printf("[weather] updated: %.1fC (feels %.1fC), %s, wind %.1f m/s, humidity %d%%\n",
                  d.temp, d.feelsLike, describe(d.cond), d.windSpeed, d.humidity);
  } else {
    Serial.printf("[weather] update failed: %s (retry in %lu s)\n", d.error,
                  static_cast<unsigned long>(retryDelayMs / 1000));
  }
}

}  // namespace

void begin() {
  mutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(workerTask, "weather", kTaskStack, nullptr, kTaskPriority, &task,
                          kTaskCore);
  nextFetchMs = millis();
}

void loop(bool networkUp) {
  if (resultReady) {
    resultReady = false;
    if (resultOk) {
      retryDelayMs = config::kWeatherRetryMinMs;
      nextFetchMs = millis() + config::kWeatherUpdateMs;
    } else {
      nextFetchMs = millis() + retryDelayMs;
    }
    logResult();
    if (!resultOk) {
      retryDelayMs = retryDelayMs * 2 > config::kWeatherUpdateMs ? config::kWeatherUpdateMs
                                                                 : retryDelayMs * 2;
    }
  }

  if (!networkUp || busy || task == nullptr) return;
  const bool due = static_cast<int32_t>(millis() - nextFetchMs) >= 0;
  if (forceRequested || due) {
    forceRequested = false;
    busy = true;
    xTaskNotifyGive(task);
  }
}

void snapshot(Data& out) {
  if (mutex == nullptr) return;
  xSemaphoreTake(mutex, portMAX_DELAY);
  out = shared;
  out.ageS = shared.valid ? static_cast<int32_t>((millis() - lastSuccessMs) / 1000) : -1;
  xSemaphoreGive(mutex);
  out.loading = busy;
}

void requestUpdate() {
  forceRequested = true;
  Serial.println("[weather] update requested");
}

void printSnapshot() {
  Data d;
  snapshot(d);
  if (!d.valid) {
    Serial.printf("[weather] no data yet%s%s%s\n", d.loading ? " (loading)" : "",
                  d.error[0] ? ", last error: " : "", d.error);
    return;
  }
  Serial.printf("[weather] %s (MET Norway), %ld s ago%s%s\n", config::kWeatherPlace,
                static_cast<long>(d.ageS), d.error[0] ? ", last error: " : "", d.error);
  Serial.printf("  now: %.1fC (feels %.1fC), %s, wind %.1f m/s from %d deg, humidity %d%%, %s\n",
                d.temp, d.feelsLike, describe(d.cond), d.windSpeed, d.windDir, d.humidity,
                d.isDay ? "day" : "night");
  for (const Hour& h : d.hours) {
    struct tm t;
    localtime_r(&h.time, &t);
    Serial.printf("  %02d:00  %5.1fC  %-14s  precip %.1f mm / %dh\n", t.tm_hour, h.temp,
                  describe(h.cond), h.precipMm, kHourStep);
  }
  for (const Day& day : d.days) {
    struct tm t;
    localtime_r(&day.date, &t);
    Serial.printf("  %02d.%02d  %5.1f..%5.1fC  %-14s  precip %.1f mm\n", t.tm_mday, t.tm_mon + 1,
                  day.tempMin, day.tempMax, describe(day.cond), day.precipMm);
  }
}

}  // namespace weather

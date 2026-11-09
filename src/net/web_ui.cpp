#include "net/web_ui.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <math.h>
#include <string.h>

#include "clock/time_service.h"
#include "config.h"
#include "display/ui.h"
#include "net/wifi_conn.h"
#include "settings/settings.h"
#include "weather/weather_client.h"
#include "weather/weather_codes.h"

namespace web {

namespace {

constexpr uint32_t kRestartDelayMs = 500;  // let the response reach the browser first

WebServer server(80);
bool running = false;
bool routesSet = false;
uint32_t restartAtMs = 0;
bool resetWifiBeforeRestart = false;

struct TzOption {
  const char* label;
  const char* posix;
};
const TzOption kZones[] = {
    {"UTC", "UTC0"},
    {"Kaliningrad (UTC+2)", "EET-2"},
    {"Moscow, St. Petersburg (UTC+3)", "MSK-3"},
    {"Samara (UTC+4)", "<+04>-4"},
    {"Yekaterinburg (UTC+5)", "<+05>-5"},
    {"Omsk (UTC+6)", "<+06>-6"},
    {"Novosibirsk, Krasnoyarsk (UTC+7)", "<+07>-7"},
    {"Irkutsk (UTC+8)", "<+08>-8"},
    {"Yakutsk (UTC+9)", "<+09>-9"},
    {"Vladivostok (UTC+10)", "<+10>-10"},
    {"Magadan (UTC+11)", "<+11>-11"},
    {"Kamchatka (UTC+12)", "<+12>-12"},
};

struct City {
  const char* name;
  float lat;
  float lon;
};
const City kCities[] = {
    {"St. Petersburg", 59.9343f, 30.3351f}, {"Moscow", 55.7558f, 37.6173f},
    {"Novosibirsk", 55.0084f, 82.9357f},    {"Yekaterinburg", 56.8389f, 60.6057f},
    {"Kazan", 55.7963f, 49.1088f},          {"Nizhny Novgorod", 56.3269f, 44.0059f},
    {"Samara", 53.1959f, 50.1002f},         {"Krasnodar", 45.0355f, 38.9753f},
    {"Kaliningrad", 54.7104f, 20.4522f},    {"Vladivostok", 43.1155f, 131.8855f},
};

const char kStyle[] PROGMEM =
    "body{font-family:system-ui,sans-serif;max-width:520px;margin:0 auto;padding:12px;"
    "background:#111;color:#eee}"
    "h2{margin:4px 0}fieldset{border:1px solid #444;border-radius:8px;margin:12px 0}"
    "legend{color:#9cf}label{display:block;margin:8px 0 2px}"
    "input,select{width:100%;box-sizing:border-box;padding:6px;background:#222;color:#eee;"
    "border:1px solid #555;border-radius:4px}"
    "input[type=radio]{width:auto;margin-right:6px}.inline{display:inline-block;margin-right:16px}"
    "button{padding:8px 14px;margin:6px 6px 0 0;border:0;border-radius:4px;background:#2a6;"
    "color:#fff;font-size:15px}button.warn{background:#a33}"
    "table{width:100%;border-collapse:collapse}td{padding:3px 4px;vertical-align:top}"
    "td:first-child{color:#999;width:38%}"
    ".msg{padding:8px;border-radius:4px;background:#253;margin:8px 0}.err{background:#622}"
    ".hint{color:#888;font-size:13px}";

const char kScript[] PROGMEM =
    "function city(s){if(!s.value)return;var p=s.value.split('|'),f=document.forms.f;"
    "f.place.value=p[0];f.lat.value=p[1];f.lon.value=p[2];}"
    // Not "tz": inside the form's inline handlers that name resolves to <select name=tz>.
    "function onTz(){var f=document.forms.f;"
    "f.tzcustom.style.display=f.tz.value=='custom'?'block':'none';}";

String escape(const char* s) {
  String out;
  for (; *s; ++s) {
    switch (*s) {
      case '&': out += F("&amp;"); break;
      case '<': out += F("&lt;"); break;
      case '>': out += F("&gt;"); break;
      case '"': out += F("&quot;"); break;
      default: out += *s;
    }
  }
  return out;
}

void row(String& h, const char* label, const String& value) {
  h += F("<tr><td>");
  h += label;
  h += F("</td><td>");
  h += value;
  h += F("</td></tr>");
}

String formatDuration(uint32_t s) {
  char buf[32];
  if (s < 60) {
    snprintf(buf, sizeof(buf), "%lus", static_cast<unsigned long>(s));
  } else if (s < 3600) {
    snprintf(buf, sizeof(buf), "%lum %lus", static_cast<unsigned long>(s / 60),
             static_cast<unsigned long>(s % 60));
  } else if (s < 86400) {
    snprintf(buf, sizeof(buf), "%luh %lum", static_cast<unsigned long>(s / 3600),
             static_cast<unsigned long>(s / 60 % 60));
  } else {
    snprintf(buf, sizeof(buf), "%lud %luh", static_cast<unsigned long>(s / 86400),
             static_cast<unsigned long>(s / 3600 % 24));
  }
  return buf;
}

void appendStatus(String& h) {
  char buf[64];
  h += F("<fieldset><legend>Status</legend><table>");

  struct tm t;
  if (timekeeping::localNow(t)) {
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1,
             t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    row(h, "Local time", buf);
  } else {
    row(h, "Local time", "not set");
  }
  String src = timekeeping::statusName(timekeeping::status());
  const int32_t ago = timekeeping::secondsSinceSync();
  if (ago >= 0) src += ", NTP sync " + formatDuration(ago) + " ago";
  row(h, "Time source", src);

  snprintf(buf, sizeof(buf), "%s, %d dBm", WiFi.SSID().c_str(), net::rssi());
  row(h, "Wi-Fi", escape(buf));
  row(h, "IP", net::localIp().toString());

  weather::Data w;
  weather::snapshot(w);
  String ws;
  if (w.valid) {
    snprintf(buf, sizeof(buf), "%.1f &deg;C, %s, updated %s ago", w.temp, weather::describe(w.cond),
             formatDuration(w.ageS).c_str());
    ws = buf;
  } else {
    ws = w.loading ? "loading..." : "no data yet";
  }
  if (w.error[0]) ws += "<br>last error: " + escape(w.error);
  row(h, "Weather", ws);

  row(h, "Uptime", formatDuration(millis() / 1000));
  snprintf(buf, sizeof(buf), "%lu bytes", static_cast<unsigned long>(ESP.getFreeHeap()));
  row(h, "Free heap", buf);
  row(h, "Firmware", __DATE__ " " __TIME__);
  h += F("</table></fieldset>");
}

void appendSettingsForm(String& h) {
  const settings::Values& v = settings::get();
  char buf[96];

  h += F("<form name=f method=post action=/save>");

  // Clock.
  h += F("<fieldset><legend>Clock</legend><label>Clock face</label>");
  for (uint8_t i = 0; i < ui::faceCount(); ++i) {
    snprintf(buf, sizeof(buf),
             "<label class=inline><input type=radio name=face value=%u%s>%s</label>", i,
             i == ui::currentFace() ? " checked" : "", ui::faceName(i));
    h += buf;
  }
  // Long fixed HTML is appended directly: snprintf into a small buffer used to
  // truncate it silently (the longitude field was lost).
  h += F("<label>Return to the clock after (seconds)</label>"
         "<input type=number name=autoret min=5 max=600 value=");
  h += String(v.autoReturnS);
  h += F("></fieldset>");

  // Time zone.
  bool known = false;
  h += F("<fieldset><legend>Time zone</legend><select name=tz onchange=onTz()>");
  for (const TzOption& z : kZones) {
    const bool sel = strcmp(z.posix, v.tz) == 0;
    known |= sel;
    h += F("<option value=\"");
    h += escape(z.posix);
    h += sel ? F("\" selected>") : F("\">");
    h += z.label;
    h += F("</option>");
  }
  h += known ? F("<option value=custom>Custom (POSIX TZ string)</option>")
             : F("<option value=custom selected>Custom (POSIX TZ string)</option>");
  h += F("</select><input name=tzcustom maxlength=47 placeholder=\"e.g. CET-1CEST,M3.5.0,M10.5.0/3\""
         " value=\"");
  h += escape(v.tz);
  h += known ? F("\" style=display:none>") : F("\">");
  h += F("</fieldset>");

  // Weather location.
  h += F("<fieldset><legend>Weather location</legend><select onchange=city(this)>"
         "<option value=\"\">Pick a city...</option>");
  for (const City& c : kCities) {
    snprintf(buf, sizeof(buf), "<option value=\"%s|%.4f|%.4f\">%s</option>", c.name, c.lat, c.lon,
             c.name);
    h += buf;
  }
  h += F("</select><label>Name (shown on the screen, ~14 characters fit)</label>"
         "<input name=place maxlength=31 value=\"");
  h += escape(v.place);
  h += F("\"><label>Latitude</label><input name=lat type=number step=any value=");
  h += String(v.lat, 4);
  h += F("><label>Longitude</label><input name=lon type=number step=any value=");
  h += String(v.lon, 4);
  h += '>';
  h += F("<p class=hint>Weather: MET Norway (yr.no)</p></fieldset>");

  h += F("<button type=submit>Save</button></form>");
}

void appendActions(String& h) {
  h += F("<fieldset><legend>Actions</legend><form method=post action=/action>"
         "<button name=do value=weather>Update weather</button>"
         "<button name=do value=ntp>Sync time</button>"
         "<button name=do value=restart class=warn "
         "onclick=\"return confirm('Restart the clock?')\">Restart</button>"
         "<button name=do value=wifireset class=warn "
         "onclick=\"return confirm('Forget the Wi-Fi network and open the setup portal?')\">"
         "Reset Wi-Fi</button></form></fieldset>");
}

void sendPage(int code, const char* message, bool isError) {
  String h;
  h.reserve(8192);
  h += F("<!doctype html><html><head><meta charset=utf-8>"
         "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
         "<title>esp32-clock</title><style>");
  h += FPSTR(kStyle);
  h += F("</style><script>");
  h += FPSTR(kScript);
  h += F("</script></head><body><h2>esp32-clock</h2>");
  if (message) {
    h += isError ? F("<div class=\"msg err\">") : F("<div class=msg>");
    h += escape(message);
    h += F("</div>");
  }
  appendStatus(h);
  appendSettingsForm(h);
  appendActions(h);
  h += F("</body></html>");
  server.send(code, "text/html; charset=utf-8", h);
}

void handleRoot() {
  sendPage(200, server.hasArg("saved") ? "Settings saved" : nullptr, false);
}

void handleSave() {
  settings::Values v = settings::get();
  const settings::Values old = v;

  String tz = server.arg("tz");
  if (tz == "custom") tz = server.arg("tzcustom");
  tz.trim();
  strlcpy(v.tz, tz.c_str(), sizeof(v.tz));

  String place = server.arg("place");
  place.trim();
  strlcpy(v.place, place.c_str(), sizeof(v.place));

  v.lat = server.arg("lat").length() ? server.arg("lat").toFloat() : NAN;
  v.lon = server.arg("lon").length() ? server.arg("lon").toFloat() : NAN;
  const long autoret = server.arg("autoret").toInt();
  v.autoReturnS = static_cast<uint16_t>(autoret < 0 ? 0 : (autoret > 65535 ? 65535 : autoret));

  char err[80];
  if (tz.length() >= sizeof(v.tz) || place.length() >= sizeof(v.place)) {
    sendPage(400, "Text field too long", true);
    return;
  }
  if (!settings::save(v, err, sizeof(err))) {
    sendPage(400, err, true);
    return;
  }

  if (strcmp(old.tz, v.tz) != 0) timekeeping::setTimezone(v.tz);
  if (old.lat != v.lat || old.lon != v.lon) weather::requestUpdate();
  if (server.hasArg("face")) {
    const long f = server.arg("face").toInt();
    if (f >= 0 && f < ui::faceCount() && f != ui::currentFace()) ui::setFace(f);
  }

  server.sendHeader("Location", "/?saved=1");
  server.send(303);
}

void handleAction() {
  const String what = server.arg("do");
  if (what == "weather") {
    weather::requestUpdate();
    sendPage(200, "Weather update requested", false);
  } else if (what == "ntp") {
    timekeeping::forceSync();
    sendPage(200, "Time sync requested", false);
  } else if (what == "restart" || what == "wifireset") {
    resetWifiBeforeRestart = what == "wifireset";
    restartAtMs = millis() + kRestartDelayMs;
    server.send(200, "text/html; charset=utf-8",
                resetWifiBeforeRestart
                    ? F("<meta charset=utf-8>Restarting into the Wi-Fi setup portal "
                        "(network <b>esp32-clock</b>, password on the screen)...")
                    : F("<meta charset=utf-8>Restarting... <a href=/>back</a> in ~10 s"));
  } else {
    sendPage(400, "Unknown action", true);
  }
}

void start() {
  if (!routesSet) {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/action", HTTP_POST, handleAction);
    server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
    routesSet = true;
  }
  server.begin();
  if (MDNS.begin(config::kHostname)) MDNS.addService("http", "tcp", 80);
  running = true;
  Serial.printf("[web] settings page: http://%s.local/ or http://%s/\n", config::kHostname,
                net::localIp().toString().c_str());
}

void stop() {
  server.stop();
  MDNS.end();
  running = false;
  Serial.println("[web] stopped (no network)");
}

}  // namespace

void loop(bool networkUp) {
  if (networkUp && !running) {
    start();
  } else if (!networkUp && running) {
    stop();
  }
  if (running) server.handleClient();

  if (restartAtMs != 0 && static_cast<int32_t>(millis() - restartAtMs) >= 0) {
    if (resetWifiBeforeRestart) net::resetCredentials();
    Serial.println("[web] restarting");
    Serial.flush();
    ESP.restart();
  }
}

}  // namespace web

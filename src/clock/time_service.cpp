#include "clock/time_service.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_sntp.h>
#include <sys/time.h>

#include "clock/rtc_clock.h"
#include "config.h"

namespace timekeeping {

namespace {

constexpr const char* kPrefsNamespace = "clock";
constexpr const char* kPrefsSyncedKey = "synced";

bool rtcOk = false;
bool sntpStarted = false;
Status current = Status::NotSet;

volatile bool syncPending = false;  // set from the lwIP task
uint32_t lastSyncMs = 0;
bool syncedThisBoot = false;
uint32_t lastClockUpdateMs = 0;  // last time the system clock was set (NTP or DS3231)

void onSntpSync(struct timeval*) { syncPending = true; }

bool everSynced() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  const bool v = prefs.getBool(kPrefsSyncedKey, false);
  prefs.end();
  return v;
}

void setEverSynced(bool v) {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  if (prefs.getBool(kPrefsSyncedKey, !v) != v) prefs.putBool(kPrefsSyncedKey, v);
  prefs.end();
}

void loadSystemClockFromRtc() {
  struct timeval tv = {};
  tv.tv_sec = static_cast<time_t>(rtc_clock::now().unixtime());  // DS3231 keeps UTC
  settimeofday(&tv, nullptr);
  lastClockUpdateMs = millis();
}

void writeRtcFromSystemClock() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  time_t sec = tv.tv_sec + (tv.tv_usec >= 500000 ? 1 : 0);  // round to nearest second
  rtc_clock::adjust(DateTime(static_cast<uint32_t>(sec)));
}

void logLocalTime(const char* prefix) {
  struct tm t;
  if (!localNow(t)) return;
  Serial.printf("%s %04d-%02d-%02d %02d:%02d:%02d (%s)\n", prefix, t.tm_year + 1900, t.tm_mon + 1,
                t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, config::kTimezone);
}

}  // namespace

void begin(bool rtcAvailable) {
  rtcOk = rtcAvailable;
  setenv("TZ", config::kTimezone, 1);
  tzset();

  if (!rtcOk) {
    Serial.println("[time] DS3231 not available, waiting for NTP");
    current = Status::NotSet;
    return;
  }
  if (rtc_clock::lostPower()) {
    Serial.println("[time] DS3231 lost power, waiting for NTP");
    setEverSynced(false);
    current = Status::NotSet;
    return;
  }
  loadSystemClockFromRtc();
  current = everSynced() ? Status::Rtc : Status::Unverified;
  Serial.printf("[time] system clock set from DS3231 (%s)\n", statusName(current));
  logLocalTime("[time] local time:");
}

void loop(bool networkUp) {
  if (networkUp && !sntpStarted) {
    sntpStarted = true;
    sntp_set_time_sync_notification_cb(onSntpSync);
    sntp_set_sync_interval(config::kNtpSyncIntervalMs);
    configTzTime(config::kTimezone, config::kNtpServer1, config::kNtpServer2);
    Serial.println("[time] SNTP started");
  }

  if (syncPending) {
    syncPending = false;
    lastSyncMs = millis();
    lastClockUpdateMs = lastSyncMs;
    syncedThisBoot = true;
    current = Status::Ntp;
    if (rtcOk) {
      writeRtcFromSystemClock();
      setEverSynced(true);
    }
    logLocalTime(rtcOk ? "[time] NTP synced, DS3231 updated:" : "[time] NTP synced:");
  }

  // Without NTP the system clock (main crystal) drifts more than the DS3231:
  // periodically reload it from the DS3231.
  if (rtcOk && current != Status::NotSet &&
      millis() - lastClockUpdateMs > config::kRtcResyncIntervalMs) {
    loadSystemClockFromRtc();
    if (current == Status::Ntp) current = Status::Rtc;
    Serial.println("[time] no NTP for a while, system clock reloaded from DS3231");
  }
}

Status status() { return current; }

const char* statusName(Status s) {
  switch (s) {
    case Status::NotSet: return "not set";
    case Status::Unverified: return "DS3231, never synced";
    case Status::Rtc: return "DS3231";
    case Status::Ntp: return "NTP";
  }
  return "?";
}

bool localNow(struct tm& out) {
  if (current == Status::NotSet) return false;
  const time_t now = time(nullptr);
  localtime_r(&now, &out);
  return true;
}

int32_t secondsSinceSync() {
  if (!syncedThisBoot) return -1;
  return static_cast<int32_t>((millis() - lastSyncMs) / 1000);
}

void forceSync() {
  if (!sntpStarted) {
    Serial.println("[time] SNTP not started (no network yet)");
    return;
  }
  Serial.println("[time] forcing NTP sync");
  // configTzTime() restarts SNTP and takes the lwIP core lock itself.
  configTzTime(config::kTimezone, config::kNtpServer1, config::kNtpServer2);
}

}  // namespace timekeeping

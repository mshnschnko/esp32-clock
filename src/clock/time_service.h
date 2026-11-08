#pragma once

#include <stdint.h>
#include <time.h>

// Time keeping. The ESP32 system clock (time()/localtime_r with TZ) is the single
// time source for the rest of the firmware. The DS3231 is its non-volatile backup:
// it seeds the system clock at boot and is updated (in UTC) after every NTP sync.
namespace timekeeping {

enum class Status {
  NotSet,      // no valid time: DS3231 lost power and no NTP sync yet
  Unverified,  // time from DS3231, but it was never confirmed by NTP
  Rtc,         // time from DS3231, NTP-synced in the past
  Ntp,         // NTP-synced during this boot
};

// rtcAvailable: DS3231 found on the I2C bus.
void begin(bool rtcAvailable);

// networkUp: Wi-Fi is connected. Starts SNTP on the first call with true.
void loop(bool networkUp);

Status status();
const char* statusName(Status s);

// Local time (timezone from config.h). Returns false if time is not set.
bool localNow(struct tm& out);

// Seconds since the last NTP sync, or -1 if none during this boot.
int32_t secondsSinceSync();

// Requests an immediate NTP sync (no-op until SNTP is started).
void forceSync();

}  // namespace timekeeping

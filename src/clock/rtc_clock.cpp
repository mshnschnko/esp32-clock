#include "clock/rtc_clock.h"

#include <Wire.h>

namespace rtc_clock {

namespace {
RTC_DS3231 rtc;
}  // namespace

bool begin() { return rtc.begin(&Wire); }

bool lostPower() { return rtc.lostPower(); }

DateTime now() { return rtc.now(); }

void adjust(const DateTime& dt) { rtc.adjust(dt); }

float temperature() { return rtc.getTemperature(); }

}  // namespace rtc_clock

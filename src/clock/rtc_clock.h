#pragma once

#include <RTClib.h>

// DS3231 real-time clock. Later the NTP sync will write into it via adjust().
namespace rtc_clock {

// Returns false if the DS3231 does not respond. Wire must already be started.
bool begin();

// True if the oscillator stopped (no battery / first power-up): time is invalid.
bool lostPower();

DateTime now();
void adjust(const DateTime& dt);

// DS3231 die temperature, °C (0.25 °C resolution, updated every 64 s).
float temperature();

}  // namespace rtc_clock

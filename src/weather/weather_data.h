#pragma once

#include <math.h>
#include <stdint.h>
#include <time.h>

#include "weather/weather_codes.h"

namespace weather {

constexpr int kHourPoints = 4;   // +3h, +6h, +9h, +12h
constexpr int kDayPoints = 4;    // today + 3 days
constexpr int kHourStep = 3;

struct Hour {
  time_t time = 0;
  Condition cond = Condition::Unknown;
  float temp = NAN;
  float precipMm = NAN;  // precipitation over the next kHourStep hours
  bool isDay = true;
};

struct Day {
  time_t date = 0;  // some moment within that local day (for the weekday)
  Condition cond = Condition::Unknown;  // around local noon
  float tempMin = NAN;
  float tempMax = NAN;
  float precipMm = NAN;  // total for the (remaining) day
};

// Snapshot for screens. Plain data: safe to copy between the weather task and loop().
struct Data {
  bool valid = false;  // at least one successful update since boot

  // Current conditions (forecast for the current hour).
  float temp = NAN;
  float feelsLike = NAN;  // computed (Steadman apparent temperature)
  int humidity = 0;       // %
  Condition cond = Condition::Unknown;
  float windSpeed = 0;    // m/s
  int windDir = 0;        // degrees, where the wind comes FROM
  bool isDay = true;

  Hour hours[kHourPoints];
  Day days[kDayPoints];

  // Freshness / state.
  int32_t ageS = -1;     // seconds since the last successful update, -1 if none
  bool loading = false;  // a request is in progress
  char error[48] = "";   // last error, empty if the last request succeeded
};

}  // namespace weather

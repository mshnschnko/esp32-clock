#pragma once

#include "display/screen_state.h"

// Weather display modes (data from weather::snapshot, see weather/weather_client.h).
namespace modes {

// Current conditions: big icon and temperature, feels-like, description, wind, humidity.
namespace weather_now {
void draw(const screens::State& s);
}

// Next hours: 4 columns at +3/+6/+9/+12 h — hour, icon, temperature, precipitation chance.
namespace weather_hours {
void draw(const screens::State& s);
}

// Days: today + 3 days — weekday, icon, max/min, precipitation chance.
namespace weather_days {
void draw(const screens::State& s);
}

}  // namespace modes

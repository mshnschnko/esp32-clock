#pragma once

#include <U8g2lib.h>

#include "weather/weather_codes.h"

// Weather icons drawn with U8g2 primitives (discs, lines, triangles), so the
// same code works at any size: 32 px on "now", 16 px in forecasts, 10 px on
// the clock screens. (x, y) is the top-left corner of a size x size square.
namespace weather_icons {

void draw(U8G2& g, int x, int y, int size, weather::Icon icon, bool isDay);

}  // namespace weather_icons

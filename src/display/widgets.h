#pragma once

#include <U8g2lib.h>

#include "display/screen_state.h"

// Small drawing helpers shared by all display modes and screens.
namespace widgets {

extern const char* const kWeekdays[7];

// Wi-Fi indicator, 8 px high, bottom-left corner at (x, y): signal bars,
// blinking while connecting, crossed when offline, "AP" in the setup portal.
void drawWifiIcon(U8G2& g, int x, int y, const screens::State& s);

// "NTP" / "RTC" / blinking "RTC?" / "" (time not set).
const char* timeSourceLabel(const screens::State& s);

// Full-width bar: Wi-Fi icon left, time source right, separator line at y = 10.
void drawStatusBar(U8G2& g, const screens::State& s);

// Draws `text` so that its right edge is at x.
void drawStrRight(U8G2& g, int x, int y, const char* text);

}  // namespace widgets

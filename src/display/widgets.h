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

// Draws `text` centered horizontally on cx.
void drawStrCentered(U8G2& g, int cx, int y, const char* text);

// Temperature as a rounded integer with a degree sign ("-5°") in the current font.
// Returns the drawn width. With measureOnly nothing is drawn.
int drawTemp(U8G2& g, int x, int y, float tempC, bool measureOnly = false);

// "45s", "12m", "3h", "2d"; prefixed with "!" when older than kWeatherStaleS.
void formatAge(char* buf, size_t size, int32_t seconds, bool markStale);

// Outdoor weather for the clock screens: 10 px icon + temperature, baseline y,
// right edge at `right`. Draws nothing if there is no weather data.
void drawOutdoor(U8G2& g, int right, int y, const screens::State& s);

// Header for weather screens: title left, data age and HH:MM right, line at y = 9.
void drawWeatherHeader(U8G2& g, const screens::State& s, const char* title);

// Placeholder when there is no weather data: says why (no Wi-Fi / loading / error).
void drawNoWeather(U8G2& g, const screens::State& s);

}  // namespace widgets

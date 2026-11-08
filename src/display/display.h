#pragma once

#include <U8g2lib.h>

// OLED display access. Display modes will draw through gfx() into the full
// frame buffer and push it with sendBuffer() — no flicker.
namespace display {

// Initializes the display. Wire must already be started.
void begin();

// Name of the controller driver selected in config.h.
const char* driverName();

U8G2& gfx();

// Night brightness: minimal contrast plus lower pre-charge and VCOMH levels
// (contrast alone still leaves the OLED quite bright). Day restores the values
// U8g2 sets at init. Sends commands only when the state changes.
void setNight(bool night);

// Night brightness registers. Defaults come from config.h; setNightLevels()
// changes them at runtime (serial "dim") and applies at once if night is on.
struct Levels {
  uint8_t contrast;
  uint8_t precharge;
  uint8_t vcomh;
};
void setNightLevels(const Levels& l);
Levels nightLevelsNow();

// Diagnostics (serial "contrast <n>"): sets only the contrast, right now,
// regardless of day/night; the next day/night switch overrides it.
void setContrastNow(uint8_t value);

// Frame along the screen edges, centered circle r=30 and corner labels.
// All four edges and a round circle mean the driver is chosen correctly.
void drawTestPattern();

}  // namespace display

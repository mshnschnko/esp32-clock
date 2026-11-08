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

// Frame along the screen edges, centered circle r=30 and corner labels.
// All four edges and a round circle mean the driver is chosen correctly.
void drawTestPattern();

}  // namespace display

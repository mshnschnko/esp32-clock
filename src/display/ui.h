#pragma once

#include "display/screen_state.h"
#include "input/input.h"

// Decides what is on the screen:
//   setup portal  >  status screen (long press)  >  current display mode.
// Click switches to the next mode (or closes the status screen); the mode is
// remembered in NVS. To add a mode: a file in display/modes/ + a line in kModes (ui.cpp).
namespace ui {

void begin();

// Returns true if the screen should be redrawn right away.
bool handle(input::Event e);

void draw(const screens::State& s);

const char* modeName();

}  // namespace ui

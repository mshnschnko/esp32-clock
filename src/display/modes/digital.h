#pragma once

#include "display/screen_state.h"

// Digital mode: large HH:MM centered on its colon, small ":SS" to the right,
// weekday, date and temperature below, status bar on top.
namespace modes::digital {

void draw(const screens::State& s);

}  // namespace modes::digital

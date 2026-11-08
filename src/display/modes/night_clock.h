#pragma once

#include "display/screen_state.h"

// Night screen: HH:MM only, medium font, nothing else lit. The position jumps
// to a random place every minute so no pixel stays on all night (OLED burn-in).
namespace modes::night_clock {

void draw(const screens::State& s);

}  // namespace modes::night_clock

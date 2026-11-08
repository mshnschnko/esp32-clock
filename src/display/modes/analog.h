#pragma once

#include "display/screen_state.h"

// Analog mode: round dial on the left (hour and minute hands, no second hand;
// the hour hand moves smoothly with the minutes), info panel on the right.
namespace modes::analog {

void draw(const screens::State& s);

}  // namespace modes::analog

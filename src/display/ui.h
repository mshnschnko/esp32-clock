#pragma once

#include "display/screen_state.h"
#include "input/input.h"

// Decides what is on the screen:
//   setup portal  >  night screen (at night, not woken)  >  status screen  >  current screen.
// Screens cycle on Click: clock -> weather now -> weather hours -> weather days -> clock.
// The clock screen draws the selected face (digital/analog), a setting kept in NVS.
// LongPress: on the clock toggles the face, elsewhere opens the status screen.
// Away from the clock with no input for kAutoReturnMs -> back to the clock.
// To add a screen: a file in display/modes/ + a line in kScreens (ui.cpp);
// a new clock face: a line in kFaces.
namespace ui {

void begin();

// Call every loop(), also with Event::None (drives the auto-return timer).
// Returns true if the screen should be redrawn right away.
bool handle(input::Event e);

void draw(const screens::State& s);

// Switches to the next clock face and saves it (also serial command "face").
void toggleFace();

// "clock (digital)", "weather now", ...
const char* screenName();

// Night mode (kNightStartMin..kNightEndMin): dim display and the night clock
// screen; the first touch wakes normal (dim) screens for kNightWakeMs.
// The override exists for testing without waiting for the night (serial "night").
enum class NightOverride { Auto, On, Off };
NightOverride cycleNightOverride();  // auto -> on -> off -> auto
const char* nightOverrideName();

}  // namespace ui

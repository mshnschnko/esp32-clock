#pragma once

// Raw input source: answers only "is it pressed right now".
// Exactly one implementation is compiled, selected by INPUT_SOURCE in config.h:
//   touch_source.cpp  — ESP32 capacitive touch pin
//   button_source.cpp — push button to GND
// Gestures (debounce, click, long press) live in input.cpp and work with any source.
namespace input::source {

void begin();
bool pressed();

// Prints raw readings for tuning (serial command "touch").
void printDebug();

}  // namespace input::source

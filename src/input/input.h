#pragma once

// User input as events. The source (touch pin or button) is chosen in config.h;
// code using this module does not know or care which one is wired.
namespace input {

enum class Event { None, Click, LongPress };

void begin();

// Call every loop(). Returns at most one event per call.
Event poll();

// Queues an event as if it came from the source (serial commands "next"/"hold").
void inject(Event e);

const char* eventName(Event e);

// Raw source readings, for tuning.
void printDebug();

}  // namespace input

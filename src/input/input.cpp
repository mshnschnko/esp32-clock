#include "input/input.h"

#include <Arduino.h>

#include "config.h"
#include "input/source.h"

namespace input {

namespace {

constexpr uint32_t kPollIntervalMs = 10;  // touchRead takes ~0.5 ms, no need to spin

Event injected = Event::None;
uint32_t lastPollMs = 0;

bool lastRaw = false;      // last reading from the source
uint32_t rawChangedMs = 0; // when lastRaw last changed
bool stable = false;       // debounced state
uint32_t pressStartMs = 0;
bool longFired = false;    // long press already reported for this press

}  // namespace

void begin() { source::begin(); }

Event poll() {
  if (injected != Event::None) {
    const Event e = injected;
    injected = Event::None;
    return e;
  }

  const uint32_t now = millis();
  if (now - lastPollMs < kPollIntervalMs) return Event::None;
  lastPollMs = now;

  const bool raw = source::pressed();
  if (raw != lastRaw) {
    lastRaw = raw;
    rawChangedMs = now;
  }

  if (raw != stable && now - rawChangedMs >= config::kDebounceMs) {
    stable = raw;
    if (stable) {
      pressStartMs = now;
      longFired = false;
    } else if (!longFired) {
      return Event::Click;
    }
  }

  if (stable && !longFired && now - pressStartMs >= config::kLongPressMs) {
    longFired = true;
    return Event::LongPress;
  }
  return Event::None;
}

void inject(Event e) { injected = e; }

const char* eventName(Event e) {
  switch (e) {
    case Event::None: return "none";
    case Event::Click: return "click";
    case Event::LongPress: return "long press";
  }
  return "?";
}

void printDebug() { source::printDebug(); }

}  // namespace input

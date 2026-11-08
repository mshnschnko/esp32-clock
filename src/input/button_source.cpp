#include "config.h"

#if INPUT_SOURCE == INPUT_SOURCE_BUTTON

// Push button between kInputPin and GND (on the expansion board: S and G pins
// of the same GPIO row). Not tested on hardware yet.

#include <Arduino.h>

#include "input/source.h"

namespace input::source {

void begin() {
  pinMode(config::kInputPin, INPUT_PULLUP);
  Serial.printf("[input] button on GPIO%u (active LOW)\n", config::kInputPin);
}

bool pressed() { return digitalRead(config::kInputPin) == LOW; }

void printDebug() {
  Serial.printf("[input] button GPIO%u: %s\n", config::kInputPin,
                pressed() ? "PRESSED (LOW)" : "released (HIGH)");
}

}  // namespace input::source

#endif  // INPUT_SOURCE == INPUT_SOURCE_BUTTON

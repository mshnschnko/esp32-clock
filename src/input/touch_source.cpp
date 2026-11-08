#include "config.h"

#if INPUT_SOURCE == INPUT_SOURCE_TOUCH

#include <Arduino.h>

#include "input/source.h"

namespace input::source {

namespace {

constexpr int kCalibrationSamples = 16;
// Baseline follows slow drift (temperature, humidity) while not touched.
// Small factor: a finger approaching slowly must not be absorbed into it.
constexpr float kBaselineFollow = 0.001f;

float baseline = 0;
uint32_t lastRaw = 0;
bool isPressed = false;

}  // namespace

void begin() {
  uint32_t sum = 0;
  for (int i = 0; i < kCalibrationSamples; ++i) {
    sum += touchRead(config::kInputPin);
    delay(5);
  }
  baseline = static_cast<float>(sum) / kCalibrationSamples;
  Serial.printf("[input] touch on GPIO%u, baseline %.0f (touched below %.0f)\n",
                config::kInputPin, baseline, baseline * config::kTouchPressRatio);
  if (baseline < 5) {
    Serial.println("[input] WARNING: touch baseline is suspiciously low, check the pin");
  }
}

bool pressed() {
  // On the original ESP32 the reading DROPS when the pin is touched.
  lastRaw = touchRead(config::kInputPin);
  const float v = static_cast<float>(lastRaw);
  if (!isPressed) {
    if (v < baseline * config::kTouchPressRatio) {
      isPressed = true;
    } else {
      baseline += (v - baseline) * kBaselineFollow;
    }
  } else if (v > baseline * config::kTouchReleaseRatio) {
    isPressed = false;
  }
  return isPressed;
}

void printDebug() {
  const uint32_t raw = touchRead(config::kInputPin);
  Serial.printf("[input] touch GPIO%u: raw %lu, baseline %.0f, press < %.0f, release > %.0f, %s\n",
                config::kInputPin, static_cast<unsigned long>(raw), baseline,
                baseline * config::kTouchPressRatio, baseline * config::kTouchReleaseRatio,
                isPressed ? "PRESSED" : "released");
}

}  // namespace input::source

#endif  // INPUT_SOURCE == INPUT_SOURCE_TOUCH

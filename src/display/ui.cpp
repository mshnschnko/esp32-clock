#include "display/ui.h"

#include <Arduino.h>
#include <Preferences.h>

#include "config.h"
#include "display/modes/analog.h"
#include "display/modes/digital.h"
#include "display/modes/weather_modes.h"
#include "display/screens.h"

namespace ui {

namespace {

struct Mode {
  const char* name;
  void (*draw)(const screens::State&);
};

const Mode kModes[] = {
    {"digital", modes::digital::draw},
    {"analog", modes::analog::draw},
    {"weather now", modes::weather_now::draw},
    {"weather hours", modes::weather_hours::draw},
    {"weather days", modes::weather_days::draw},
};
constexpr uint8_t kModeCount = sizeof(kModes) / sizeof(kModes[0]);

constexpr const char* kPrefsNamespace = "ui";
constexpr const char* kPrefsModeKey = "mode";

uint8_t current = 0;
uint32_t statusUntilMs = 0;  // 0 = status screen hidden

bool statusShown() {
  return statusUntilMs != 0 && static_cast<int32_t>(statusUntilMs - millis()) > 0;
}

void saveMode() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putUChar(kPrefsModeKey, current);
  prefs.end();
}

}  // namespace

void begin() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  current = prefs.getUChar(kPrefsModeKey, 0);
  prefs.end();
  if (current >= kModeCount) current = 0;
  Serial.printf("[ui] mode: %s\n", modeName());
}

bool handle(input::Event e) {
  switch (e) {
    case input::Event::None:
      return false;

    case input::Event::Click:
      if (statusShown()) {
        statusUntilMs = 0;
        Serial.println("[ui] status screen closed");
      } else {
        current = (current + 1) % kModeCount;
        saveMode();
        Serial.printf("[ui] mode: %s\n", modeName());
      }
      return true;

    case input::Event::LongPress:
      statusUntilMs = millis() + config::kStatusScreenMs;
      Serial.println("[ui] status screen");
      return true;
  }
  return false;
}

void draw(const screens::State& s) {
  if (s.netState == net::State::Portal) {
    screens::drawPortal(s);
  } else if (statusShown()) {
    screens::drawStatus(s);
  } else {
    kModes[current].draw(s);
  }
}

const char* modeName() { return kModes[current].name; }

}  // namespace ui

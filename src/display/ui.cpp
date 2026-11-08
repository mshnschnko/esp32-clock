#include "display/ui.h"

#include <Arduino.h>
#include <Preferences.h>
#include <stdio.h>

#include "config.h"
#include "display/modes/analog.h"
#include "display/modes/digital.h"
#include "display/modes/weather_modes.h"
#include "display/screens.h"

namespace ui {

namespace {

using DrawFn = void (*)(const screens::State&);

struct Entry {
  const char* name;
  DrawFn draw;  // nullptr for the clock screen: it draws the selected face
};

const Entry kFaces[] = {
    {"digital", modes::digital::draw},
    {"analog", modes::analog::draw},
};
constexpr uint8_t kFaceCount = sizeof(kFaces) / sizeof(kFaces[0]);

constexpr uint8_t kClockScreen = 0;
const Entry kScreens[] = {
    {"clock", nullptr},
    {"weather now", modes::weather_now::draw},
    {"weather hours", modes::weather_hours::draw},
    {"weather days", modes::weather_days::draw},
};
constexpr uint8_t kScreenCount = sizeof(kScreens) / sizeof(kScreens[0]);

constexpr const char* kPrefsNamespace = "ui";
constexpr const char* kPrefsFaceKey = "face";
constexpr const char* kPrefsOldModeKey = "mode";  // stage 3-4: index in a single mode list

uint8_t face = 0;
uint8_t screen = kClockScreen;  // always the clock after boot
uint32_t lastInputMs = 0;
uint32_t statusUntilMs = 0;  // 0 = status screen hidden

bool statusShown() {
  return statusUntilMs != 0 && static_cast<int32_t>(statusUntilMs - millis()) > 0;
}

void logScreen() { Serial.printf("[ui] screen: %s\n", screenName()); }

}  // namespace

void begin() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  face = prefs.getUChar(kPrefsFaceKey, 0);
  if (prefs.isKey(kPrefsOldModeKey)) prefs.remove(kPrefsOldModeKey);
  prefs.end();
  if (face >= kFaceCount) face = 0;
  logScreen();
}

bool handle(input::Event e) {
  const uint32_t now = millis();

  switch (e) {
    case input::Event::None:
      if (screen != kClockScreen && now - lastInputMs >= config::kAutoReturnMs) {
        screen = kClockScreen;
        statusUntilMs = 0;
        Serial.println("[ui] auto-return to clock");
        return true;
      }
      return false;

    case input::Event::Click:
      lastInputMs = now;
      if (statusShown()) {
        statusUntilMs = 0;
        Serial.println("[ui] status screen closed");
      } else {
        screen = (screen + 1) % kScreenCount;
        logScreen();
      }
      return true;

    case input::Event::LongPress:
      lastInputMs = now;
      if (screen == kClockScreen && !statusShown()) {
        toggleFace();
      } else {
        statusUntilMs = now + config::kStatusScreenMs;
        Serial.println("[ui] status screen");
      }
      return true;
  }
  return false;
}

void draw(const screens::State& s) {
  if (s.netState == net::State::Portal) {
    screens::drawPortal(s);
  } else if (statusShown()) {
    screens::drawStatus(s);
  } else if (screen == kClockScreen) {
    kFaces[face].draw(s);
  } else {
    kScreens[screen].draw(s);
  }
}

void toggleFace() {
  face = (face + 1) % kFaceCount;
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putUChar(kPrefsFaceKey, face);
  prefs.end();
  Serial.printf("[ui] clock face: %s\n", kFaces[face].name);
}

const char* screenName() {
  static char buf[24];
  if (screen == kClockScreen) {
    snprintf(buf, sizeof(buf), "clock (%s)", kFaces[face].name);
    return buf;
  }
  return kScreens[screen].name;
}

}  // namespace ui

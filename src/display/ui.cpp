#include "display/ui.h"

#include <Arduino.h>
#include <Preferences.h>
#include <stdio.h>

#include "config.h"
#include "display/display.h"
#include "display/modes/analog.h"
#include "display/modes/digital.h"
#include "display/modes/night_clock.h"
#include "display/modes/weather_modes.h"
#include "display/screens.h"
#include "settings/settings.h"

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

// Night mode.
NightOverride nightOverride = NightOverride::Auto;
bool night = false;          // updated in draw() once a second
uint32_t wakeUntilMs = 0;    // 0 = not woken

bool statusShown() {
  return statusUntilMs != 0 && static_cast<int32_t>(statusUntilMs - millis()) > 0;
}

bool awake() { return wakeUntilMs != 0 && static_cast<int32_t>(wakeUntilMs - millis()) > 0; }

bool isNightTime(const screens::State& s) {
  if (!config::kNightModeEnabled) return false;
  switch (nightOverride) {
    case NightOverride::On: return true;
    case NightOverride::Off: return false;
    case NightOverride::Auto: break;
  }
  if (!s.timeValid) return false;
  const int m = s.local.tm_hour * 60 + s.local.tm_min;
  if (config::kNightStartMin < config::kNightEndMin) {
    return m >= config::kNightStartMin && m < config::kNightEndMin;
  }
  return m >= config::kNightStartMin || m < config::kNightEndMin;  // crosses midnight
}

// Back to the clock with nothing on top of it.
void resetToClock() {
  screen = kClockScreen;
  statusUntilMs = 0;
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

  if (night) {
    if (e != input::Event::None && !awake()) {
      // The first touch at night only wakes the screen.
      wakeUntilMs = now + config::kNightWakeMs;
      lastInputMs = now;
      resetToClock();
      Serial.println("[ui] woken up");
      return true;
    }
    if (e != input::Event::None) {
      wakeUntilMs = now + config::kNightWakeMs;  // touches keep it awake
    } else if (wakeUntilMs != 0 && !awake()) {
      wakeUntilMs = 0;
      resetToClock();
      Serial.println("[ui] back to the night screen");
      return true;
    }
  }

  switch (e) {
    case input::Event::None:
      if (screen != kClockScreen &&
          now - lastInputMs >= settings::get().autoReturnS * 1000UL) {
        resetToClock();
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
  const bool nightNow = isNightTime(s);
  if (nightNow != night) {
    night = nightNow;
    wakeUntilMs = 0;
    if (night) resetToClock();
    Serial.printf("[ui] night %s\n", night ? "on" : "off");
  }
  // Disabled night mode must not send any brightness command (see config.h).
  if (config::kNightModeEnabled) display::setNight(night);

  if (s.netState == net::State::Portal) {
    screens::drawPortal(s);
  } else if (night && !awake()) {
    modes::night_clock::draw(s);
  } else if (statusShown()) {
    screens::drawStatus(s);
  } else if (screen == kClockScreen) {
    kFaces[face].draw(s);
  } else {
    kScreens[screen].draw(s);
  }
}

void toggleFace() { setFace((face + 1) % kFaceCount); }

void setFace(uint8_t f) {
  if (f >= kFaceCount) return;
  face = f;
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putUChar(kPrefsFaceKey, face);
  prefs.end();
  Serial.printf("[ui] clock face: %s\n", kFaces[face].name);
}

uint8_t currentFace() { return face; }

uint8_t faceCount() { return kFaceCount; }

const char* faceName(uint8_t f) { return f < kFaceCount ? kFaces[f].name : "?"; }

NightOverride cycleNightOverride() {
  if (!config::kNightModeEnabled) {
    Serial.println("[ui] night mode is disabled (kNightModeEnabled in config.h)");
    return nightOverride;
  }
  nightOverride = static_cast<NightOverride>((static_cast<int>(nightOverride) + 1) % 3);
  Serial.printf("[ui] night mode: %s\n", nightOverrideName());
  return nightOverride;
}

const char* nightOverrideName() {
  switch (nightOverride) {
    case NightOverride::Auto: return "auto (schedule)";
    case NightOverride::On: return "forced on";
    case NightOverride::Off: return "forced off";
  }
  return "?";
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

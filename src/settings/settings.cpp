#include "settings/settings.h"

#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include <string.h>

#include "config.h"

namespace settings {

namespace {

constexpr const char* kNamespace = "settings";
constexpr uint16_t kAutoReturnMinS = 5;
constexpr uint16_t kAutoReturnMaxS = 600;

Values current;

}  // namespace

void begin() {
  Preferences prefs;
  prefs.begin(kNamespace, true);
  prefs.getString("tz", current.tz, sizeof(current.tz));
  prefs.getString("place", current.place, sizeof(current.place));
  current.lat = prefs.getFloat("lat", config::kWeatherLat);
  current.lon = prefs.getFloat("lon", config::kWeatherLon);
  current.autoReturnS =
      prefs.getUShort("autoret", static_cast<uint16_t>(config::kAutoReturnMs / 1000));
  prefs.end();

  // getString leaves the (zero-initialized) buffer empty when the key is missing.
  if (current.tz[0] == '\0') strlcpy(current.tz, config::kTimezone, sizeof(current.tz));
  if (current.place[0] == '\0') strlcpy(current.place, config::kWeatherPlace, sizeof(current.place));
  print();
}

const Values& get() { return current; }

bool save(const Values& v, char* err, size_t errSize) {
  if (v.tz[0] == '\0' || strlen(v.tz) >= sizeof(v.tz)) {
    snprintf(err, errSize, "Time zone must be 1..%u characters", sizeof(v.tz) - 1);
    return false;
  }
  if (v.place[0] == '\0' || strlen(v.place) >= sizeof(v.place)) {
    snprintf(err, errSize, "Place name must be 1..%u characters", sizeof(v.place) - 1);
    return false;
  }
  if (isnan(v.lat) || v.lat < -90 || v.lat > 90) {
    snprintf(err, errSize, "Latitude must be between -90 and 90");
    return false;
  }
  if (isnan(v.lon) || v.lon < -180 || v.lon > 180) {
    snprintf(err, errSize, "Longitude must be between -180 and 180");
    return false;
  }
  if (v.autoReturnS < kAutoReturnMinS || v.autoReturnS > kAutoReturnMaxS) {
    snprintf(err, errSize, "Auto-return must be %u..%u seconds", kAutoReturnMinS, kAutoReturnMaxS);
    return false;
  }

  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.putString("tz", v.tz);
  prefs.putString("place", v.place);
  prefs.putFloat("lat", v.lat);
  prefs.putFloat("lon", v.lon);
  prefs.putUShort("autoret", v.autoReturnS);
  prefs.end();

  current = v;
  print();
  return true;
}

void print() {
  Serial.printf("[settings] tz \"%s\", place \"%s\" (%.4f, %.4f), auto-return %u s\n", current.tz,
                current.place, current.lat, current.lon, current.autoReturnS);
}

}  // namespace settings

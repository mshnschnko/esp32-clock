#include "weather/weather_codes.h"

#include <string.h>

namespace weather {

namespace {

struct SymbolEntry {
  const char* base;  // symbol without the "_day"/"_night"/"_polartwilight" suffix
  Condition cond;
};

// Thunder variants ("...andthunder") are handled before this table.
const SymbolEntry kSymbols[] = {
    {"clearsky", Condition::Clear},
    {"fair", Condition::Fair},
    {"partlycloudy", Condition::PartlyCloudy},
    {"cloudy", Condition::Cloudy},
    {"fog", Condition::Fog},
    {"lightrain", Condition::LightRain},
    {"rain", Condition::Rain},
    {"heavyrain", Condition::HeavyRain},
    {"lightrainshowers", Condition::LightShowers},
    {"rainshowers", Condition::Showers},
    {"heavyrainshowers", Condition::HeavyShowers},
    {"lightsleet", Condition::Sleet},
    {"sleet", Condition::Sleet},
    {"heavysleet", Condition::Sleet},
    {"lightsleetshowers", Condition::Sleet},
    {"sleetshowers", Condition::Sleet},
    {"heavysleetshowers", Condition::Sleet},
    {"lightsnow", Condition::LightSnow},
    {"snow", Condition::Snow},
    {"heavysnow", Condition::HeavySnow},
    {"lightsnowshowers", Condition::SnowShowers},
    {"snowshowers", Condition::SnowShowers},
    {"heavysnowshowers", Condition::SnowShowers},
};

}  // namespace

Condition fromSymbol(const char* symbol, bool* isNight) {
  if (symbol == nullptr || symbol[0] == '\0') return Condition::Unknown;

  char base[40];
  size_t len = strcspn(symbol, "_");
  if (len >= sizeof(base)) len = sizeof(base) - 1;
  memcpy(base, symbol, len);
  base[len] = '\0';

  if (isNight) *isNight = strstr(symbol, "_night") != nullptr;
  if (strstr(base, "thunder") != nullptr) return Condition::Thunder;

  for (const SymbolEntry& e : kSymbols) {
    if (strcmp(base, e.base) == 0) return e.cond;
  }
  return Condition::Unknown;
}

Icon iconFor(Condition c) {
  switch (c) {
    case Condition::Clear: return Icon::Clear;
    case Condition::Fair:
    case Condition::PartlyCloudy: return Icon::PartlyCloudy;
    case Condition::Cloudy: return Icon::Cloudy;
    case Condition::Fog: return Icon::Fog;
    case Condition::LightRain: return Icon::Drizzle;
    case Condition::Rain:
    case Condition::HeavyRain:
    case Condition::LightShowers:
    case Condition::Showers:
    case Condition::HeavyShowers: return Icon::Rain;
    case Condition::Sleet:
    case Condition::LightSnow:
    case Condition::Snow:
    case Condition::HeavySnow:
    case Condition::SnowShowers: return Icon::Snow;
    case Condition::Thunder: return Icon::Thunder;
    case Condition::Unknown: return Icon::Unknown;
  }
  return Icon::Unknown;
}

const char* describe(Condition c) {
  switch (c) {
    case Condition::Clear: return "Clear";
    case Condition::Fair: return "Fair";
    case Condition::PartlyCloudy: return "Partly cloudy";
    case Condition::Cloudy: return "Cloudy";
    case Condition::Fog: return "Fog";
    case Condition::LightRain: return "Light rain";
    case Condition::Rain: return "Rain";
    case Condition::HeavyRain: return "Heavy rain";
    case Condition::LightShowers: return "Light showers";
    case Condition::Showers: return "Showers";
    case Condition::HeavyShowers: return "Heavy showers";
    case Condition::Sleet: return "Sleet";
    case Condition::LightSnow: return "Light snow";
    case Condition::Snow: return "Snow";
    case Condition::HeavySnow: return "Heavy snow";
    case Condition::SnowShowers: return "Snow showers";
    case Condition::Thunder: return "Thunderstorm";
    case Condition::Unknown: return "Unknown";
  }
  return "Unknown";
}

}  // namespace weather

#pragma once

#include <stdint.h>

// Weather conditions from MET Norway symbol codes ("partlycloudy_day",
// "lightrainshowersandthunder_night", ...) -> icon and short text.
// See https://api.met.no/weatherapi/weathericon/2.0/documentation
namespace weather {

enum class Condition : uint8_t {
  Unknown,
  Clear,
  Fair,
  PartlyCloudy,
  Cloudy,
  Fog,
  LightRain,
  Rain,
  HeavyRain,
  LightShowers,
  Showers,
  HeavyShowers,
  Sleet,
  LightSnow,
  Snow,
  HeavySnow,
  SnowShowers,
  Thunder,
};

enum class Icon { Clear, PartlyCloudy, Cloudy, Fog, Drizzle, Rain, Snow, Thunder, Unknown };

// Parses a symbol code. isNight is set for "_night" variants (left unchanged if null).
Condition fromSymbol(const char* symbol, bool* isNight = nullptr);

Icon iconFor(Condition c);

// Short English description, at most 14 characters.
const char* describe(Condition c);

}  // namespace weather

#pragma once

#include <IPAddress.h>
#include <stdint.h>
#include <time.h>

#include "clock/time_service.h"
#include "net/wifi_conn.h"
#include "weather/weather_data.h"

namespace screens {

// Everything a screen may show, collected once per frame in main.cpp.
// Screens only draw it; they do not query modules themselves.
struct State {
  bool timeValid = false;
  struct tm local = {};
  timekeeping::Status timeStatus = timekeeping::Status::NotSet;
  int32_t secondsSinceSync = -1;

  net::State netState = net::State::Connecting;
  int rssi = 0;
  IPAddress ip;
  uint32_t portalSecondsLeft = 0;

  bool tempValid = false;  // DS3231 chip temperature
  float tempC = 0;

  weather::Data weather;

  uint32_t uptimeS = 0;
  bool blink = false;  // toggles every second, for blinking elements
};

}  // namespace screens

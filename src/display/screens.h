#pragma once

#include <IPAddress.h>
#include <stdint.h>
#include <time.h>

#include "clock/time_service.h"
#include "net/wifi_conn.h"

// Screens drawn into the display frame buffer. Display modes (digital/analog)
// come in the next stage; for now there is one working screen.
namespace screens {

struct State {
  bool timeValid = false;
  struct tm local = {};
  timekeeping::Status timeStatus = timekeeping::Status::NotSet;

  net::State netState = net::State::Connecting;
  int rssi = 0;
  IPAddress ip;
  uint32_t portalSecondsLeft = 0;

  bool tempValid = false;
  float tempC = 0;

  bool blink = false;  // toggles every second, for blinking elements
};

// Time with a status bar (Wi-Fi, time source), date and temperature.
void drawClock(const State& s);

// Wi-Fi setup portal: access point name, password, address.
void drawPortal(const State& s);

}  // namespace screens

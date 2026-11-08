#pragma once

#include <IPAddress.h>

// Wi-Fi connection with a WiFiManager setup portal. Fully non-blocking:
// call loop() as often as possible.
//
//   Connecting --(connected)--> Connected
//   Connecting --(timeout / no saved network)--> Portal
//   Portal --(network configured, or saved network back)--> Connected
//            (retries the saved network every 30 s while nobody is on the AP)
//   Portal --(portal timeout)--> Offline   (portal closed, retries continue)
//   Connected --(link lost)--> Offline --(reconnected)--> Connected
namespace net {

enum class State { Connecting, Connected, Portal, Offline };

void begin();
void loop();

State state();
const char* stateName(State s);
bool isConnected();

IPAddress localIp();
int rssi();  // dBm, valid only when connected

// Seconds until the setup portal closes (0 if not in portal).
uint32_t portalSecondsLeft();

// Forgets the saved network. Call ESP.restart() afterwards.
void resetCredentials();

}  // namespace net

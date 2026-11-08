#pragma once

#include "display/screen_state.h"

// Full-screen overlays shown on top of the display modes (see ui.h).
namespace screens {

// Wi-Fi setup portal: access point name, password, address.
void drawPortal(const State& s);

// Diagnostics: Wi-Fi, IP, signal, time source, last sync, uptime, temperature.
void drawStatus(const State& s);

}  // namespace screens

#pragma once

// Commands typed in the serial monitor (there is no button yet):
//   help        — list commands
//   status      — Wi-Fi, time and memory status
//   next        — same as a short touch: next display mode
//   hold        — same as a long touch (toggle clock face / status screen)
//   face        — toggle the clock face (digital/analog)
//   night       — night mode: auto (schedule) -> forced on -> forced off -> auto
//   dim [c p v] — show / set night brightness registers (contrast, pre-charge, VCOMH)
//   contrast n  — set the display contrast right now (diagnostics)
//   touch       — raw touch/button reading, for threshold tuning
//   weather     — print weather data and request an update
//   ntp         — force an NTP sync
//   wifi reset  — forget the saved network and reboot into the setup portal
//   screen test — show the display test pattern for 5 s
namespace serial_cmd {

void loop();

// True while a command asked to pause normal screen updates.
bool screenPaused();

}  // namespace serial_cmd

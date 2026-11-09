#pragma once

// Settings web page on the home network: http://esp32-clock.local/ (mDNS) or
// the IP shown on the status screen. Runs only while Wi-Fi is connected; in the
// setup portal port 80 belongs to WiFiManager.
//   GET  /        status + settings form
//   POST /save    store settings (validated) and apply them at once
//   POST /action  do=weather|ntp|restart|wifireset
namespace web {

// Call every loop(). Starts/stops the server with the network and handles requests.
void loop(bool networkUp);

}  // namespace web

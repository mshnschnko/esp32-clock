#include "diag/serial_cmd.h"

#include <Arduino.h>

#include "clock/time_service.h"
#include "display/display.h"
#include "display/ui.h"
#include "input/input.h"
#include "net/wifi_conn.h"
#include "settings/settings.h"
#include "weather/weather_client.h"

namespace serial_cmd {

namespace {

constexpr uint32_t kScreenTestMs = 5000;

String line;
uint32_t pausedUntilMs = 0;

void printStatus() {
  const net::State ns = net::state();
  Serial.printf("Wi-Fi: %s", net::stateName(ns));
  if (ns == net::State::Connected) {
    Serial.printf(", IP %s, RSSI %d dBm", net::localIp().toString().c_str(), net::rssi());
  }
  if (ns == net::State::Portal) {
    Serial.printf(", portal closes in %lu s", static_cast<unsigned long>(net::portalSecondsLeft()));
  }
  Serial.println();

  Serial.printf("Time: %s", timekeeping::statusName(timekeeping::status()));
  struct tm t;
  if (timekeeping::localNow(t)) {
    Serial.printf(", %04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec);
  }
  const int32_t ago = timekeeping::secondsSinceSync();
  if (ago >= 0) Serial.printf(", last NTP sync %ld s ago", static_cast<long>(ago));
  Serial.println();

  Serial.printf("Uptime: %lu s, free heap: %lu bytes\n",
                static_cast<unsigned long>(millis() / 1000),
                static_cast<unsigned long>(ESP.getFreeHeap()));
}

// "dim" prints the night brightness registers, "dim <contrast> <precharge> <vcomh>"
// sets them (decimal or 0x.. hex) and applies at once if night mode is on.
void setDim(const String& cmd) {
  display::Levels l = display::nightLevelsNow();
  int c, p, v;
  if (sscanf(cmd.c_str(), "dim %i %i %i", &c, &p, &v) == 3) {
    l = {static_cast<uint8_t>(c), static_cast<uint8_t>(p), static_cast<uint8_t>(v)};
    display::setNightLevels(l);
  } else if (cmd != "dim") {
    Serial.println("Usage: dim <contrast> <precharge> <vcomh>, e.g. dim 0 0x22 0x20");
  }
  Serial.printf("[display] night levels: contrast 0x%02X, precharge 0x%02X, vcomh 0x%02X "
                "(day: 0xCF 0xF1 0x40)\n",
                l.contrast, l.precharge, l.vcomh);
}

void execute(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.isEmpty()) return;

  if (cmd == "help") {
    Serial.println(
        "Commands: help, status, settings, next, hold, face, night, dim, contrast, touch, weather, "
        "ntp, "
        "wifi reset, "
        "screen test");
  } else if (cmd == "dim" || cmd.startsWith("dim ")) {
    setDim(cmd);
  } else if (cmd.startsWith("contrast ")) {
    int v;
    if (sscanf(cmd.c_str(), "contrast %i", &v) == 1 && v >= 0 && v <= 255) {
      Serial.println("WARNING: on our panel this may blank the screen until power-off");
      display::setContrastNow(static_cast<uint8_t>(v));
    } else {
      Serial.println("Usage: contrast <0..255>, e.g. contrast 0x10");
    }
  } else if (cmd == "settings") {
    settings::print();
  } else if (cmd == "night") {
    ui::cycleNightOverride();
  } else if (cmd == "face") {
    ui::toggleFace();
  } else if (cmd == "weather") {
    weather::printSnapshot();
    weather::requestUpdate();
  } else if (cmd == "next") {
    input::inject(input::Event::Click);
  } else if (cmd == "hold") {
    input::inject(input::Event::LongPress);
  } else if (cmd == "touch") {
    input::printDebug();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "ntp") {
    timekeeping::forceSync();
  } else if (cmd == "wifi reset") {
    net::resetCredentials();
    Serial.println("Rebooting...");
    Serial.flush();
    delay(200);
    ESP.restart();
  } else if (cmd == "screen test") {
    display::drawTestPattern();
    pausedUntilMs = millis() + kScreenTestMs;
  } else {
    Serial.printf("Unknown command \"%s\", type help\n", cmd.c_str());
  }
}

}  // namespace

void loop() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      execute(line);
      line = "";
    } else if (line.length() < 64) {
      line += c;
    }
  }
}

bool screenPaused() {
  return pausedUntilMs != 0 && static_cast<int32_t>(pausedUntilMs - millis()) > 0;
}

}  // namespace serial_cmd

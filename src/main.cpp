// ESP32 Wi-Fi clock: NTP time with DS3231 backup, Wi-Fi setup portal,
// digital/analog display modes switched by touch (or a button).
// See docs/idea.md. Every module is non-blocking; loop() must stay fast.

#include <Arduino.h>
#include <Wire.h>

#include "clock/rtc_clock.h"
#include "clock/time_service.h"
#include "config.h"
#include "diag/i2c_diag.h"
#include "diag/serial_cmd.h"
#include "display/display.h"
#include "display/screen_state.h"
#include "display/ui.h"
#include "input/input.h"
#include "net/wifi_conn.h"

namespace {

constexpr uint32_t kTempReadIntervalMs = 10000;  // DS3231 updates it every 64 s
constexpr uint32_t kLiveCheckIntervalMs = 2000;

i2c_diag::ScanResult bus;
bool rtcOk = false;

bool tempValid = false;
float tempC = 0;
uint32_t lastTempReadMs = 0;

void updateTemperature() {
  if (!rtcOk) return;
  if (tempValid && millis() - lastTempReadMs < kTempReadIntervalMs) return;
  tempC = rtc_clock::temperature();
  tempValid = true;
  lastTempReadMs = millis();
}

void drawScreen(time_t now) {
  screens::State s;
  s.timeStatus = timekeeping::status();
  s.timeValid = timekeeping::localNow(s.local);
  s.secondsSinceSync = timekeeping::secondsSinceSync();
  s.netState = net::state();
  if (s.netState == net::State::Connected) {
    s.rssi = net::rssi();
    s.ip = net::localIp();
  }
  s.portalSecondsLeft = net::portalSecondsLeft();
  s.tempValid = tempValid;
  s.tempC = tempC;
  s.uptimeS = millis() / 1000;
  s.blink = (now % 2) == 0;

  ui::draw(s);
}

}  // namespace

void setup() {
  Serial.begin(config::kSerialBaud);
  delay(500);
  Serial.println();
  Serial.println("=== esp32-clock ===");

  Wire.begin(config::kI2cSda, config::kI2cScl, config::kI2cFreqHz);
  bus = i2c_diag::scan();
  if (bus.count == 0) {
    i2c_diag::probeWiring();
    return;  // nothing to run without devices; loop() keeps diagnosing
  }

  if (bus.oled) display::begin();
  rtcOk = bus.rtc && rtc_clock::begin();

  input::begin();
  ui::begin();

  timekeeping::begin(rtcOk);
  net::begin();
  Serial.println("Type 'help' for serial commands");
}

void loop() {
  if (bus.count == 0) {
    static uint32_t lastLiveMs = 0;
    if (millis() - lastLiveMs >= kLiveCheckIntervalMs) {
      lastLiveMs = millis();
      i2c_diag::liveCheck();
    }
    return;
  }

  net::loop();
  timekeeping::loop(net::isConnected());
  serial_cmd::loop();
  updateTemperature();

  const input::Event event = input::poll();
  if (event != input::Event::None) Serial.printf("[input] %s\n", input::eventName(event));
  const bool forceRedraw = ui::handle(event);

  // Redraw on every new second of the system clock (keeps the seconds display
  // in phase with real time), right after user input, and at least once a second.
  static time_t lastShown = 0;
  static uint32_t lastDrawMs = 0;
  const time_t now = time(nullptr);
  if (bus.oled && !serial_cmd::screenPaused() &&
      (forceRedraw || now != lastShown || millis() - lastDrawMs >= 1000)) {
    lastShown = now;
    lastDrawMs = millis();
    drawScreen(now);
  }

  delay(2);
}

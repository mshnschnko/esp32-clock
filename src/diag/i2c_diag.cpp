#include "diag/i2c_diag.h"

#include <Arduino.h>
#include <Wire.h>

#include "config.h"

namespace i2c_diag {

namespace {

// The modules have pull-up resistors to 3.3 V on SDA/SCL, so a GPIO wired to
// them reads HIGH even with the internal pull-down enabled.
constexpr uint8_t kProbePins[] = {4,  5,  12, 13, 14, 15, 16, 17, 18,
                                  19, 21, 22, 23, 25, 26, 27, 32, 33};

const char* deviceName(uint8_t addr) {
  switch (addr) {
    case config::kOledAddr: return "OLED";
    case config::kRtcAddr: return "DS3231";
    case config::kEepromAddr: return "EEPROM AT24C32";
    default: return "unknown";
  }
}

int countDevices() {
  int count = 0;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) ++count;
  }
  return count;
}

// Wire must be stopped. Returns the number of pins written to `pulled`.
int findPulledUpPins(uint8_t* pulled, int maxPins) {
  int n = 0;
  for (uint8_t pin : kProbePins) {
    pinMode(pin, INPUT_PULLDOWN);
    delay(2);
    const bool high = digitalRead(pin) == HIGH;
    pinMode(pin, INPUT);
    if (high && n < maxPins) pulled[n++] = pin;
  }
  return n;
}

void restartWire() { Wire.begin(config::kI2cSda, config::kI2cScl, config::kI2cFreqHz); }

}  // namespace

ScanResult scan() {
  ScanResult r;
  Serial.println("I2C scan:");
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() != 0) continue;
    ++r.count;
    Serial.printf("  0x%02X  %s\n", addr, deviceName(addr));
    if (addr == config::kOledAddr) r.oled = true;
    if (addr == config::kRtcAddr) r.rtc = true;
    if (addr == config::kEepromAddr) r.eeprom = true;
  }
  Serial.printf("I2C scan done: %d device(s). OLED %s, DS3231 %s, EEPROM %s\n", r.count,
                r.oled ? "OK" : "MISSING", r.rtc ? "OK" : "MISSING", r.eeprom ? "OK" : "MISSING");
  return r;
}

void probeWiring() {
  Wire.end();

  Serial.println("Wiring probe: GPIOs pulled up by module resistors:");
  uint8_t pulled[8];
  const int n = findPulledUpPins(pulled, 8);
  for (int i = 0; i < n; ++i) Serial.printf("  GPIO%d\n", pulled[i]);
  if (n == 0) {
    Serial.println("  none: SDA/SCL wires reach no GPIO (wrong header row/pin or breadboard column)");
  }

  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      if (i == j) continue;
      Wire.begin(pulled[i], pulled[j], 100000);
      const int count = countDevices();
      Wire.end();
      Serial.printf("  try SDA=GPIO%d SCL=GPIO%d: %d device(s)\n", pulled[i], pulled[j], count);
    }
  }

  restartWire();
}

void liveCheck() {
  Wire.end();
  uint8_t pulled[8];
  const int n = findPulledUpPins(pulled, 8);
  restartWire();
  const int devices = countDevices();

  Serial.print("[live] HIGH pins:");
  if (n == 0) Serial.print(" none");
  for (int i = 0; i < n; ++i) Serial.printf(" GPIO%d", pulled[i]);
  Serial.printf(" | devices on SDA=%d SCL=%d: %d", config::kI2cSda, config::kI2cScl, devices);
  Serial.println(devices > 0 ? "  -> found! press EN to restart" : "");
}

}  // namespace i2c_diag

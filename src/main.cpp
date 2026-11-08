// Stage 1 hardware check firmware:
//   1. I2C scan (expects OLED 0x3C, DS3231 0x68, EEPROM 0x57);
//   2. display test pattern for a few seconds;
//   3. DS3231 time and temperature once a second on Serial and the display.

#include <Arduino.h>
#include <Wire.h>

#include "clock/rtc_clock.h"
#include "config.h"
#include "display/display.h"

namespace {

constexpr uint32_t kTestPatternMs = 5000;

bool oledFound = false;
bool rtcFound = false;
bool eepromFound = false;
bool rtcOk = false;

const char* deviceName(uint8_t addr) {
  switch (addr) {
    case config::kOledAddr: return "OLED";
    case config::kRtcAddr: return "DS3231";
    case config::kEepromAddr: return "EEPROM AT24C32";
    default: return "unknown";
  }
}

int scanI2c() {
  Serial.println("I2C scan:");
  int count = 0;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() != 0) continue;
    ++count;
    Serial.printf("  0x%02X  %s\n", addr, deviceName(addr));
    if (addr == config::kOledAddr) oledFound = true;
    if (addr == config::kRtcAddr) rtcFound = true;
    if (addr == config::kEepromAddr) eepromFound = true;
  }
  Serial.printf("I2C scan done: %d device(s). OLED %s, DS3231 %s, EEPROM %s\n", count,
                oledFound ? "OK" : "MISSING", rtcFound ? "OK" : "MISSING",
                eepromFound ? "OK" : "MISSING");
  return count;
}

// Wiring diagnostics, used when the scan finds nothing.
// The modules have pull-up resistors to 3.3 V on SDA/SCL, so a GPIO wired to
// them reads HIGH even with the internal pull-down enabled. This reveals which
// GPIOs the SDA/SCL wires actually land on; then every pair of such pins is
// scanned in both directions (catches swapped SDA/SCL).
constexpr uint8_t kProbePins[] = {4,  5,  12, 13, 14, 15, 16, 17, 18,
                                  19, 21, 22, 23, 25, 26, 27, 32, 33};

int countDevices() {
  int count = 0;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) ++count;
  }
  return count;
}

// Fills `pulled` with GPIOs that read HIGH against the internal pull-down.
// Wire must be stopped. Returns the number of such pins.
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

void probeI2cWiring() {
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

  Wire.begin(config::kI2cSda, config::kI2cScl, config::kI2cFreqHz);
}

// Live wiring check while the bus is empty: lets the user move wires and see
// the effect every 2 s without resetting the board.
void liveWiringCheck() {
  Wire.end();
  uint8_t pulled[8];
  const int n = findPulledUpPins(pulled, 8);
  Wire.begin(config::kI2cSda, config::kI2cScl, config::kI2cFreqHz);
  const int devices = countDevices();

  Serial.print("[live] HIGH pins:");
  if (n == 0) Serial.print(" none");
  for (int i = 0; i < n; ++i) Serial.printf(" GPIO%d", pulled[i]);
  Serial.printf(" | devices on SDA=%d SCL=%d: %d", config::kI2cSda, config::kI2cScl, devices);
  Serial.println(devices > 0 ? "  -> found! press EN to restart" : "");
}

void setupRtc() {
  if (!rtcFound || !rtc_clock::begin()) {
    Serial.println("DS3231: not available");
    return;
  }
  rtcOk = true;
  if (rtc_clock::lostPower()) {
    Serial.println("DS3231: lost power, setting time from firmware build time");
    rtc_clock::adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("DS3231: time is valid (kept by battery)");
  }
}

void drawStatusScreen(const DateTime& t, float tempC) {
  U8G2& g = display::gfx();
  char buf[24];

  g.clearBuffer();

  if (rtcOk) {
    g.setFont(u8g2_font_logisoso22_tn);
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hour(), t.minute(), t.second());
    g.drawStr((g.getDisplayWidth() - g.getStrWidth(buf)) / 2, 26, buf);

    g.setFont(u8g2_font_6x10_tr);
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d", t.day(), t.month(), t.year());
    g.drawStr(0, 42, buf);
    snprintf(buf, sizeof(buf), "%.2f C", tempC);
    g.drawStr(g.getDisplayWidth() - g.getStrWidth(buf), 42, buf);
  } else {
    g.setFont(u8g2_font_6x10_tr);
    g.drawStr(0, 20, "DS3231 not found");
  }

  g.setFont(u8g2_font_5x7_tr);
  snprintf(buf, sizeof(buf), "OLED RTC%s EEP%s", rtcFound ? "+" : "-", eepromFound ? "+" : "-");
  g.drawStr(0, 54, buf);
  snprintf(buf, sizeof(buf), "%s  up %lus", display::driverName(), millis() / 1000);
  g.drawStr(0, 63, buf);

  g.sendBuffer();
}

}  // namespace

void setup() {
  Serial.begin(config::kSerialBaud);
  delay(500);
  Serial.println();
  Serial.println("=== esp32-clock: hardware check ===");

  Wire.begin(config::kI2cSda, config::kI2cScl, config::kI2cFreqHz);
  if (scanI2c() == 0) probeI2cWiring();

  if (oledFound) {
    display::begin();
    display::drawTestPattern();
    Serial.printf("Display: test pattern drawn with %s driver\n", display::driverName());
  }

  setupRtc();

  if (oledFound) delay(kTestPatternMs);
}

void loop() {
  static uint32_t lastUpdateMs = 0;
  if (lastUpdateMs != 0 && millis() - lastUpdateMs < 1000) return;
  lastUpdateMs = millis();

  if (!oledFound && !rtcFound && !eepromFound) {
    static uint32_t tick = 0;
    if (tick++ % 2 == 0) liveWiringCheck();
    return;
  }

  DateTime t;
  float tempC = 0;
  if (rtcOk) {
    t = rtc_clock::now();
    tempC = rtc_clock::temperature();
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d  %.2f C\n", t.year(), t.month(), t.day(),
                  t.hour(), t.minute(), t.second(), tempC);
  }
  if (oledFound) drawStatusScreen(t, tempC);
}

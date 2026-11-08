#pragma once

// I2C bus diagnostics: startup scan and wiring probe (see docs/wiring.md).
namespace i2c_diag {

struct ScanResult {
  int count = 0;
  bool oled = false;
  bool rtc = false;
  bool eeprom = false;
};

// Scans the bus and logs every device found. Wire must be started.
ScanResult scan();

// One-shot wiring probe, used when the scan finds nothing: lists GPIOs pulled up
// by module resistors and tries every pair of them as SDA/SCL.
void probeWiring();

// Short live line for repeated calls while the bus is empty:
// "[live] HIGH pins: ... | devices on SDA=21 SCL=22: N".
void liveCheck();

}  // namespace i2c_diag

#pragma once

#include <stdint.h>

// Hardware configuration. See docs/hardware.md and docs/wiring.md.

// Display controller: 1 = SH1106 (1.3" modules), 0 = SSD1306 (0.96" modules).
// Wrong choice shows up as a picture shifted by 2 px or noise along the right edge.
#define DISPLAY_SH1106 1

namespace config {

constexpr uint32_t kSerialBaud = 115200;

// I2C bus shared by the OLED and the DS3231 module.
constexpr int kI2cSda = 21;
constexpr int kI2cScl = 22;
constexpr uint32_t kI2cFreqHz = 400000;

// 7-bit I2C addresses.
constexpr uint8_t kOledAddr = 0x3C;    // "0x78" on the module's ADDRESS_SELECT
constexpr uint8_t kRtcAddr = 0x68;     // DS3231
constexpr uint8_t kEepromAddr = 0x57;  // AT24C32 on the RTC module

}  // namespace config

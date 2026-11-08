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

// --- Time ---
// POSIX TZ string. "MSK-3" = UTC+3 without DST. The DS3231 always keeps UTC.
constexpr const char* kTimezone = "MSK-3";
constexpr const char* kNtpServer1 = "pool.ntp.org";
constexpr const char* kNtpServer2 = "time.google.com";
constexpr uint32_t kNtpSyncIntervalMs = 60UL * 60 * 1000;    // SNTP re-sync period
// Reload the system clock from the DS3231 if there was no NTP sync for this long.
// Must be longer than kNtpSyncIntervalMs.
constexpr uint32_t kRtcResyncIntervalMs = 90UL * 60 * 1000;

// --- Wi-Fi ---
// Home network credentials are NOT here: they are entered via the setup portal
// and stored by the ESP32 in flash.
constexpr const char* kHostname = "esp32-clock";
constexpr const char* kApName = "esp32-clock";       // setup access point
constexpr const char* kApPassword = "clock-setup";   // shown on the OLED, 8+ chars
constexpr uint32_t kWifiConnectTimeoutMs = 15000;    // then open the setup portal
constexpr uint32_t kPortalTimeoutS = 300;            // then close it, keep retrying
constexpr uint32_t kWifiRetryIntervalMs = 30000;     // reconnect attempts while offline

}  // namespace config

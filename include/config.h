#pragma once

#include <stdint.h>

// Hardware configuration. See docs/hardware.md and docs/wiring.md.
// Time zone, weather location and auto-return time below are DEFAULTS: the
// actual values are edited on the web page and kept in NVS (src/settings).

// Display controller: 1 = SH1106 (1.3" modules), 0 = SSD1306 (0.96" modules).
// Wrong choice shows up as a picture shifted by 2 px or noise along the right edge.
#define DISPLAY_SH1106 1

// Mode switch input. The rest of the firmware only sees Click/LongPress events,
// so switching to a button is just this line (+ wiring, see docs/wiring.md).
#define INPUT_SOURCE_TOUCH 1   // wire on a touch-capable GPIO, touch its end
#define INPUT_SOURCE_BUTTON 2  // push button between the GPIO and GND
#define INPUT_SOURCE INPUT_SOURCE_TOUCH

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

// --- Input (mode switch) ---
constexpr uint8_t kInputPin = 32;        // GPIO32 = touch channel T9; later the button pin
constexpr float kTouchPressRatio = 0.7f;    // touched: reading < baseline * ratio
constexpr float kTouchReleaseRatio = 0.8f;  // released: reading > baseline * ratio
constexpr uint32_t kDebounceMs = 30;
constexpr uint32_t kLongPressMs = 800;
constexpr uint32_t kStatusScreenMs = 10000;  // status screen auto-close
constexpr uint32_t kAutoReturnMs = 15000;    // away from the clock this long without input -> clock

// --- Night mode ---
// DISABLED: on our GME12864-77 any change of the brightness registers blanks the
// panel until power-off (see docs/roadmap.md, stage 6). With false no brightness
// command is ever sent and the night screen is never shown.
constexpr bool kNightModeEnabled = false;
// Local time, minutes since midnight; the range may cross midnight.
constexpr uint16_t kNightStartMin = 23 * 60;  // 23:00
constexpr uint16_t kNightEndMin = 7 * 60;     // 07:00
constexpr uint32_t kNightWakeMs = 15000;      // a touch at night shows normal screens this long
// Night brightness (SH1106 registers). Contrast alone dims only a little; lower
// pre-charge/VCOMH dim more, but too low turns the panel completely dark
// (0x11/0x00 did). Tune live with the serial command "dim <contrast> <precharge> <vcomh>".
constexpr uint8_t kNightContrast = 0x20;   // day: 0xCF; 0x00 suspected to blank the panel
constexpr uint8_t kNightPrecharge = 0xF1;  // day: 0xF1
constexpr uint8_t kNightVcomh = 0x40;      // day: 0x40

// --- Time ---
// POSIX TZ string. "MSK-3" = UTC+3 without DST. The DS3231 always keeps UTC.
constexpr const char* kTimezone = "MSK-3";
constexpr const char* kNtpServer1 = "pool.ntp.org";
constexpr const char* kNtpServer2 = "time.google.com";
constexpr uint32_t kNtpSyncIntervalMs = 60UL * 60 * 1000;    // SNTP re-sync period
// Reload the system clock from the DS3231 if there was no NTP sync for this long.
// Must be longer than kNtpSyncIntervalMs.
constexpr uint32_t kRtcResyncIntervalMs = 90UL * 60 * 1000;

// --- Weather (MET Norway Locationforecast, no API key) ---
// Terms: https://api.met.no/doc/TermsOfService — identify the app with contact
// info in the User-Agent, coordinates with at most 4 decimals, no frequent polling.
constexpr float kWeatherLat = 59.9343f;  // St. Petersburg
constexpr float kWeatherLon = 30.3351f;
constexpr const char* kWeatherPlace = "St. Petersburg";
constexpr const char* kWeatherUserAgent = "esp32-clock/1.0 github.com/mshnschnko/esp32-clock";
constexpr uint32_t kWeatherUpdateMs = 30UL * 60 * 1000;  // MET updates about hourly
constexpr uint32_t kWeatherRetryMinMs = 60UL * 1000;  // doubles on each failure, up to kWeatherUpdateMs
constexpr uint32_t kWeatherStaleS = 3UL * 60 * 60;    // older data is marked with "!"

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

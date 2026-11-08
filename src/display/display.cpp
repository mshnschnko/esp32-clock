#include "display/display.h"

#include "config.h"

namespace display {

namespace {

#if DISPLAY_SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
constexpr const char* kDriverName = "SH1106";
#else
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
constexpr const char* kDriverName = "SSD1306";
#endif

}  // namespace

void begin() {
  u8g2.setI2CAddress(config::kOledAddr << 1);  // U8g2 expects the 8-bit address
  u8g2.setBusClock(config::kI2cFreqHz);
  u8g2.begin();
}

const char* driverName() { return kDriverName; }

U8G2& gfx() { return u8g2; }

namespace {

// SH1106/SSD1306 commands; day values = U8g2 init sequence
// (u8x8_d_ssd1306_128x64_noname_init_seq, shared by the SH1106 driver).
constexpr uint8_t kCmdPrecharge = 0xD9, kCmdVcomh = 0xDB;  // contrast: setContrast() (0x81)
constexpr Levels kDay = {0xCF, 0xF1, 0x40};

Levels nightLevels = {config::kNightContrast, config::kNightPrecharge, config::kNightVcomh};
int currentNight = -1;  // unknown at start: always apply once

void apply(const Levels& l) {
  u8g2.setContrast(l.contrast);  // U8g2 API: sends 0x81 <value>
  u8g2.sendF("ca", kCmdPrecharge, l.precharge);
  u8g2.sendF("ca", kCmdVcomh, l.vcomh);
  Serial.printf("[display] contrast 0x%02X, precharge 0x%02X, vcomh 0x%02X\n", l.contrast,
                l.precharge, l.vcomh);
}

}  // namespace

void setNight(bool night) {
  if (currentNight == static_cast<int>(night)) return;
  currentNight = night;
  apply(night ? nightLevels : kDay);
}

void setNightLevels(const Levels& l) {
  nightLevels = l;
  if (currentNight == 1) apply(nightLevels);
}

Levels nightLevelsNow() { return nightLevels; }

void setContrastNow(uint8_t value) {
  u8g2.setContrast(value);
  Serial.printf("[display] contrast 0x%02X (until the next day/night switch)\n", value);
}

void drawTestPattern() {
  const int w = u8g2.getDisplayWidth();
  const int h = u8g2.getDisplayHeight();

  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, w, h);
  u8g2.drawCircle(w / 2, h / 2, 30);
  u8g2.drawLine(w / 2, h / 2 - 4, w / 2, h / 2 + 4);
  u8g2.drawLine(w / 2 - 4, h / 2, w / 2 + 4, h / 2);

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(3, 9, "TL");
  u8g2.drawStr(w - 13, 9, "TR");
  u8g2.drawStr(3, h - 3, "BL");
  u8g2.drawStr(w - 13, h - 3, "BR");
  u8g2.drawStr(w - 3 - u8g2.getStrWidth(kDriverName), h / 2 + 3, kDriverName);
  u8g2.sendBuffer();
}

}  // namespace display

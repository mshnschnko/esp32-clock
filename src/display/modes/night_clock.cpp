#include "display/modes/night_clock.h"

#include <esp_random.h>
#include <stdio.h>

#include "display/display.h"

namespace modes::night_clock {

namespace {

const uint8_t* const kFont = u8g2_font_logisoso20_tn;

int lastMinute = -1;
int posX = 0;
int posY = 0;  // baseline

void pickPosition(U8G2& g, int textW) {
  const int ascent = g.getAscent();
  const int maxX = g.getDisplayWidth() - textW;
  const int minY = ascent;               // top of the digits at y = 0
  const int maxY = g.getDisplayHeight() - 1;
  posX = maxX > 0 ? static_cast<int>(esp_random() % (maxX + 1)) : 0;
  posY = minY + static_cast<int>(esp_random() % (maxY - minY + 1));
}

}  // namespace

void draw(const screens::State& s) {
  U8G2& g = display::gfx();
  g.clearBuffer();

  if (s.timeValid) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", s.local.tm_hour, s.local.tm_min);
    g.setFont(kFont);
    if (s.local.tm_min != lastMinute) {
      lastMinute = s.local.tm_min;
      pickPosition(g, g.getStrWidth(buf));
    }
    g.drawStr(posX, posY, buf);
  }

  g.sendBuffer();
}

}  // namespace modes::night_clock

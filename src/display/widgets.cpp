#include "display/widgets.h"

namespace widgets {

const char* const kWeekdays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void drawWifiIcon(U8G2& g, int x, int y, const screens::State& s) {
  constexpr int kBarW = 2, kGap = 1;
  constexpr int kHeights[] = {3, 5, 7};

  int filled = 0;
  bool crossed = false;
  switch (s.netState) {
    case net::State::Connected:
      filled = s.rssi > -60 ? 3 : (s.rssi > -75 ? 2 : 1);
      break;
    case net::State::Connecting:
      filled = s.blink ? 3 : 0;
      break;
    case net::State::Offline:
      crossed = true;
      break;
    case net::State::Portal:
      g.setFont(u8g2_font_5x7_tr);
      g.drawStr(x, y, "AP");
      return;
  }

  for (int i = 0; i < 3; ++i) {
    const int bx = x + i * (kBarW + kGap);
    const int h = kHeights[i];
    if (i < filled) {
      g.drawBox(bx, y - h, kBarW, h);
    } else {
      g.drawFrame(bx, y - h, kBarW, h);
    }
  }
  if (crossed) {
    g.drawLine(x + 10, y - 6, x + 15, y - 1);
    g.drawLine(x + 10, y - 1, x + 15, y - 6);
  }
}

const char* timeSourceLabel(const screens::State& s) {
  switch (s.timeStatus) {
    case timekeeping::Status::Ntp: return "NTP";
    case timekeeping::Status::Rtc: return "RTC";
    case timekeeping::Status::Unverified: return s.blink ? "RTC?" : "RTC";
    case timekeeping::Status::NotSet: return "";
  }
  return "";
}

void drawStatusBar(U8G2& g, const screens::State& s) {
  drawWifiIcon(g, 0, 8, s);
  g.setFont(u8g2_font_5x7_tr);
  drawStrRight(g, g.getDisplayWidth(), 7, timeSourceLabel(s));
  g.drawHLine(0, 10, g.getDisplayWidth());
}

void drawStrRight(U8G2& g, int x, int y, const char* text) {
  g.drawStr(x - g.getStrWidth(text), y, text);
}

}  // namespace widgets

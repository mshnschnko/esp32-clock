#include "display/screens.h"

#include <stdio.h>

#include "config.h"
#include "display/display.h"

namespace screens {

namespace {

constexpr const char* kWeekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Three signal bars, 8 px high, bottom-left corner at (x, y).
void drawWifiIcon(U8G2& g, int x, int y, const State& s) {
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

const char* timeSourceLabel(const State& s) {
  switch (s.timeStatus) {
    case timekeeping::Status::Ntp: return "NTP";
    case timekeeping::Status::Rtc: return "RTC";
    case timekeeping::Status::Unverified: return s.blink ? "RTC?" : "RTC";
    case timekeeping::Status::NotSet: return "";
  }
  return "";
}

void drawStatusBar(U8G2& g, const State& s) {
  drawWifiIcon(g, 0, 8, s);

  g.setFont(u8g2_font_5x7_tr);
  const char* src = timeSourceLabel(s);
  g.drawStr(g.getDisplayWidth() - g.getStrWidth(src), 7, src);

  g.drawHLine(0, 10, g.getDisplayWidth());
}

void drawBottomLine(U8G2& g, const State& s) {
  char buf[32];
  switch (s.netState) {
    case net::State::Connected:
      snprintf(buf, sizeof(buf), "%s", s.ip.toString().c_str());
      break;
    case net::State::Connecting:
      snprintf(buf, sizeof(buf), "Wi-Fi connecting...");
      break;
    case net::State::Offline:
      snprintf(buf, sizeof(buf), "Wi-Fi offline, retrying");
      break;
    case net::State::Portal:
      snprintf(buf, sizeof(buf), "Wi-Fi setup");
      break;
  }
  if (s.timeStatus == timekeeping::Status::Unverified ||
      s.timeStatus == timekeeping::Status::NotSet) {
    if (s.netState == net::State::Connected) snprintf(buf, sizeof(buf), "Waiting for NTP...");
  }
  g.setFont(u8g2_font_5x7_tr);
  g.drawStr(0, 63, buf);
}

}  // namespace

void drawClock(const State& s) {
  U8G2& g = display::gfx();
  char buf[24];

  g.clearBuffer();
  drawStatusBar(g, s);

  g.setFont(u8g2_font_logisoso22_tn);
  if (s.timeValid) {
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", s.local.tm_hour, s.local.tm_min, s.local.tm_sec);
  } else {
    snprintf(buf, sizeof(buf), "%s", s.blink ? "--:--" : "");
  }
  g.drawStr((g.getDisplayWidth() - g.getStrWidth(buf)) / 2, 37, buf);

  g.setFont(u8g2_font_6x10_tf);
  if (s.timeValid) {
    snprintf(buf, sizeof(buf), "%s %02d.%02d.%04d", kWeekdays[s.local.tm_wday % 7],
             s.local.tm_mday, s.local.tm_mon + 1, s.local.tm_year + 1900);
    g.drawStr(0, 51, buf);
  }
  if (s.tempValid) {
    snprintf(buf, sizeof(buf), "%.1f\xC2\xB0" "C", s.tempC);  // UTF-8 degree sign
    g.drawUTF8(g.getDisplayWidth() - g.getUTF8Width(buf), 51, buf);
  }

  drawBottomLine(g, s);
  g.sendBuffer();
}

void drawPortal(const State& s) {
  U8G2& g = display::gfx();
  char buf[32];

  g.clearBuffer();
  g.setFont(u8g2_font_6x10_tf);
  g.drawStr(0, 8, "Wi-Fi setup");
  if (s.timeValid) {
    snprintf(buf, sizeof(buf), "%02d:%02d", s.local.tm_hour, s.local.tm_min);
    g.drawStr(g.getDisplayWidth() - g.getStrWidth(buf), 8, buf);
  }
  g.drawHLine(0, 10, g.getDisplayWidth());

  snprintf(buf, sizeof(buf), "Net:  %s", config::kApName);
  g.drawStr(0, 22, buf);
  snprintf(buf, sizeof(buf), "Pass: %s", config::kApPassword);
  g.drawStr(0, 33, buf);
  g.drawStr(0, 44, "Open: 192.168.4.1");

  g.setFont(u8g2_font_5x7_tr);
  snprintf(buf, sizeof(buf), "closes in %lu:%02lu",
           static_cast<unsigned long>(s.portalSecondsLeft / 60),
           static_cast<unsigned long>(s.portalSecondsLeft % 60));
  g.drawStr(0, 63, buf);

  g.sendBuffer();
}

}  // namespace screens

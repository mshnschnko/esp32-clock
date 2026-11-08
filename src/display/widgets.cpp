#include "display/widgets.h"

#include <math.h>
#include <stdio.h>

#include "config.h"
#include "display/weather_icons.h"

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

void drawStrCentered(U8G2& g, int cx, int y, const char* text) {
  g.drawStr(cx - g.getStrWidth(text) / 2, y, text);
}

int drawTemp(U8G2& g, int x, int y, float tempC, bool measureOnly) {
  char buf[8];
  if (isnan(tempC)) {
    snprintf(buf, sizeof(buf), "--");
  } else {
    long t = lroundf(tempC);
    snprintf(buf, sizeof(buf), "%ld", t == 0 ? 0L : t);  // no "-0"
  }
  const int textW = g.getStrWidth(buf);
  const int ascent = g.getAscent();
  const int r = ascent >= 16 ? 3 : (ascent >= 9 ? 2 : 1);  // degree circle radius
  if (!measureOnly) {
    g.drawStr(x, y, buf);
    g.drawCircle(x + textW + 1 + r, y - ascent + r, r);
  }
  return textW + 2 + 2 * r;
}

void formatAge(char* buf, size_t size, int32_t seconds, bool markStale) {
  if (seconds < 0) {
    snprintf(buf, size, "--");
    return;
  }
  const char* mark = (markStale && static_cast<uint32_t>(seconds) > config::kWeatherStaleS) ? "!" : "";
  const unsigned long s = static_cast<unsigned long>(seconds);
  if (s < 60) {
    snprintf(buf, size, "%s%lus", mark, s);
  } else if (s < 3600) {
    snprintf(buf, size, "%s%lum", mark, s / 60);
  } else if (s < 86400) {
    snprintf(buf, size, "%s%luh", mark, s / 3600);
  } else {
    snprintf(buf, size, "%s%lud", mark, s / 86400);
  }
}

void drawOutdoor(U8G2& g, int right, int y, const screens::State& s) {
  if (!s.weather.valid) return;
  constexpr int kIcon = 10;
  g.setFont(u8g2_font_6x10_tr);
  const int tempW = drawTemp(g, 0, 0, s.weather.temp, /*measureOnly=*/true);
  const int tempX = right - tempW;
  drawTemp(g, tempX, y, s.weather.temp);
  weather_icons::draw(g, tempX - kIcon - 2, y - kIcon + 1, kIcon, weather::iconFor(s.weather.cond),
                      s.weather.isDay);
}

void drawWeatherHeader(U8G2& g, const screens::State& s, const char* title) {
  char right[16];
  char age[8];
  g.setFont(u8g2_font_5x7_tr);
  g.drawStr(0, 7, title);

  formatAge(age, sizeof(age), s.weather.ageS, /*markStale=*/true);
  if (s.timeValid) {
    snprintf(right, sizeof(right), "%s  %02d:%02d", age, s.local.tm_hour, s.local.tm_min);
  } else {
    snprintf(right, sizeof(right), "%s", age);
  }
  drawStrRight(g, g.getDisplayWidth(), 7, right);
  g.drawHLine(0, 9, g.getDisplayWidth());
}

void drawNoWeather(U8G2& g, const screens::State& s) {
  const int cx = g.getDisplayWidth() / 2;
  g.setFont(u8g2_font_6x10_tr);
  drawStrCentered(g, cx, 32, "No weather yet");

  const char* reason = "Waiting...";
  if (s.weather.loading) {
    reason = "Loading...";
  } else if (s.netState != net::State::Connected) {
    reason = "No Wi-Fi";
  } else if (s.weather.error[0]) {
    reason = s.weather.error;
  }
  g.setFont(u8g2_font_5x7_tr);
  drawStrCentered(g, cx, 46, reason);
}

}  // namespace widgets

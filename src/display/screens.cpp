#include "display/screens.h"

#include <stdio.h>

#include "config.h"
#include "display/display.h"
#include "display/widgets.h"

namespace screens {

namespace {

void drawTitle(U8G2& g, const State& s, const char* title) {
  char buf[8];
  g.setFont(u8g2_font_6x10_tf);
  g.drawStr(0, 8, title);
  if (s.timeValid) {
    snprintf(buf, sizeof(buf), "%02d:%02d", s.local.tm_hour, s.local.tm_min);
    widgets::drawStrRight(g, g.getDisplayWidth(), 8, buf);
  }
  g.drawHLine(0, 10, g.getDisplayWidth());
}

// Short labels: the status screen fits 25 characters per line.
const char* shortTimeStatus(timekeeping::Status st) {
  switch (st) {
    case timekeeping::Status::NotSet: return "not set";
    case timekeeping::Status::Unverified: return "RTC (unsynced)";
    case timekeeping::Status::Rtc: return "RTC";
    case timekeeping::Status::Ntp: return "NTP";
  }
  return "?";
}

// "45s", "12m", "3h", "2d"
void formatAgo(char* buf, size_t size, uint32_t seconds) {
  if (seconds < 60) {
    snprintf(buf, size, "%lus", static_cast<unsigned long>(seconds));
  } else if (seconds < 3600) {
    snprintf(buf, size, "%lum", static_cast<unsigned long>(seconds / 60));
  } else if (seconds < 86400) {
    snprintf(buf, size, "%luh", static_cast<unsigned long>(seconds / 3600));
  } else {
    snprintf(buf, size, "%lud", static_cast<unsigned long>(seconds / 86400));
  }
}

}  // namespace

void drawPortal(const State& s) {
  U8G2& g = display::gfx();
  char buf[32];

  g.clearBuffer();
  drawTitle(g, s, "Wi-Fi setup");

  g.setFont(u8g2_font_6x10_tf);
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

void drawStatus(const State& s) {
  U8G2& g = display::gfx();
  char buf[32];
  char ago[8];
  int y = 19;
  constexpr int kLine = 8;

  g.clearBuffer();
  drawTitle(g, s, "Status");
  g.setFont(u8g2_font_5x7_tr);

  if (s.netState == net::State::Connected) {
    snprintf(buf, sizeof(buf), "Wi-Fi: %s %d dBm", net::stateName(s.netState), s.rssi);
    g.drawStr(0, y, buf);
    y += kLine;
    snprintf(buf, sizeof(buf), "IP:    %s", s.ip.toString().c_str());
    g.drawStr(0, y, buf);
    y += kLine;
  } else {
    snprintf(buf, sizeof(buf), "Wi-Fi: %s", net::stateName(s.netState));
    g.drawStr(0, y, buf);
    y += kLine;
  }

  if (s.secondsSinceSync >= 0) {
    formatAgo(ago, sizeof(ago), static_cast<uint32_t>(s.secondsSinceSync));
    snprintf(buf, sizeof(buf), "Time:  %s, sync %s ago", shortTimeStatus(s.timeStatus), ago);
  } else {
    snprintf(buf, sizeof(buf), "Time:  %s", shortTimeStatus(s.timeStatus));
  }
  g.drawStr(0, y, buf);
  y += kLine;

  if (s.weather.loading) {
    snprintf(buf, sizeof(buf), "Wthr:  loading...");
  } else if (s.weather.error[0]) {
    snprintf(buf, sizeof(buf), "Wthr:  %s", s.weather.error);  // long text is clipped at the edge
  } else if (s.weather.valid) {
    formatAgo(ago, sizeof(ago), static_cast<uint32_t>(s.weather.ageS));
    snprintf(buf, sizeof(buf), "Wthr:  updated %s ago", ago);
  } else {
    snprintf(buf, sizeof(buf), "Wthr:  no data");
  }
  g.drawStr(0, y, buf);
  y += kLine;

  const uint32_t up = s.uptimeS;
  snprintf(buf, sizeof(buf), "Up:    %lud %02lu:%02lu:%02lu", static_cast<unsigned long>(up / 86400),
           static_cast<unsigned long>(up / 3600 % 24), static_cast<unsigned long>(up / 60 % 60),
           static_cast<unsigned long>(up % 60));
  g.drawStr(0, y, buf);
  y += kLine;

  if (s.tempValid) {
    snprintf(buf, sizeof(buf), "Chip:  %.2f C (DS3231)", s.tempC);
    g.drawStr(0, y, buf);
  }

  g.sendBuffer();
}

}  // namespace screens

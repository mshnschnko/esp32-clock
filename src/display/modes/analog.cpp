#include "display/modes/analog.h"

#include <math.h>
#include <stdio.h>

#include "display/display.h"
#include "display/widgets.h"

namespace modes::analog {

namespace {

// Dial geometry.
constexpr int kCx = 31;
constexpr int kCy = 32;
constexpr int kRadius = 30;
constexpr int kMinorTick = 4;   // tick length for 1, 2, 4, 5, ...
constexpr int kMajorTick = 7;   // tick length for 12, 3, 6, 9
constexpr int kHourLen = 15;
constexpr float kHourHalfWidth = 2.0f;
constexpr int kMinuteLen = 24;
constexpr float kMinuteHalfWidth = 1.2f;

// Right info panel.
constexpr int kPanelX = 66;

constexpr float kPi = 3.14159265f;

// Point at `len` from the dial center in direction `angleDeg`
// (0 = 12 o'clock, clockwise).
void polar(float angleDeg, float len, int& x, int& y) {
  const float a = angleDeg * kPi / 180.0f;
  x = kCx + static_cast<int>(lroundf(len * sinf(a)));
  y = kCy - static_cast<int>(lroundf(len * cosf(a)));
}

void drawTicks(U8G2& g) {
  for (int i = 0; i < 12; ++i) {
    const float angle = i * 30.0f;
    const bool major = i % 3 == 0;
    const int len = major ? kMajorTick : kMinorTick;
    int x0, y0, x1, y1;
    polar(angle, kRadius - len, x0, y0);
    polar(angle, kRadius - 1, x1, y1);
    g.drawLine(x0, y0, x1, y1);
    if (major) {
      // Ticks at 12/3/6/9 are axis-aligned: thicken by a parallel line.
      const int dx = (i % 6 == 0) ? 1 : 0;  // 12 and 6: vertical -> shift in x
      const int dy = 1 - dx;                // 3 and 9: horizontal -> shift in y
      g.drawLine(x0 + dx, y0 + dy, x1 + dx, y1 + dy);
    }
  }
}

// Hand as a filled triangle: wide at the center, pointed at the tip.
void drawHand(U8G2& g, float angleDeg, int len, float halfWidth) {
  int tipX, tipY, lX, lY, rX, rY;
  polar(angleDeg, len, tipX, tipY);
  polar(angleDeg - 90.0f, halfWidth, lX, lY);
  polar(angleDeg + 90.0f, halfWidth, rX, rY);
  g.drawTriangle(tipX, tipY, lX, lY, rX, rY);
  g.drawLine(kCx, kCy, tipX, tipY);  // keeps thin hands visible at any angle
}

void drawDial(U8G2& g, const screens::State& s) {
  g.drawCircle(kCx, kCy, kRadius);
  drawTicks(g);
  if (s.timeValid) {
    const float minutes = s.local.tm_min + s.local.tm_sec / 60.0f;
    const float hours = (s.local.tm_hour % 12) + minutes / 60.0f;
    drawHand(g, hours * 30.0f, kHourLen, kHourHalfWidth);
    drawHand(g, minutes * 6.0f, kMinuteLen, kMinuteHalfWidth);
  }
  g.drawDisc(kCx, kCy, 2);
}

void drawCentered(U8G2& g, int y, const char* text, bool utf8 = false) {
  const int w = g.getDisplayWidth() - kPanelX;
  const int textW = utf8 ? g.getUTF8Width(text) : g.getStrWidth(text);
  const int x = kPanelX + (w - textW) / 2;
  if (utf8) {
    g.drawUTF8(x, y, text);
  } else {
    g.drawStr(x, y, text);
  }
}

void drawPanel(U8G2& g, const screens::State& s) {
  char buf[24];

  widgets::drawWifiIcon(g, kPanelX, 8, s);
  g.setFont(u8g2_font_5x7_tr);
  widgets::drawStrRight(g, g.getDisplayWidth(), 7, widgets::timeSourceLabel(s));
  g.drawHLine(kPanelX, 10, g.getDisplayWidth() - kPanelX);

  g.setFont(u8g2_font_logisoso16_tn);
  if (s.timeValid) {
    snprintf(buf, sizeof(buf), "%02d:%02d", s.local.tm_hour, s.local.tm_min);
  } else {
    snprintf(buf, sizeof(buf), "%s", s.blink ? "--:--" : "");
  }
  drawCentered(g, 31, buf);

  g.setFont(u8g2_font_6x10_tf);
  if (s.timeValid) {
    drawCentered(g, 43, widgets::kWeekdays[s.local.tm_wday % 7]);
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d", s.local.tm_mday, s.local.tm_mon + 1,
             s.local.tm_year + 1900);
    drawCentered(g, 53, buf);
  }
  if (s.tempValid) {
    snprintf(buf, sizeof(buf), "%.0f\xC2\xB0" "C", s.tempC);  // UTF-8 degree sign
    drawCentered(g, 63, buf, true);
  }
}

}  // namespace

void draw(const screens::State& s) {
  U8G2& g = display::gfx();
  g.clearBuffer();
  drawDial(g, s);
  drawPanel(g, s);
  g.sendBuffer();
}

}  // namespace modes::analog

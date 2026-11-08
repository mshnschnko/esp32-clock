#include "display/modes/digital.h"

#include <stdio.h>

#include "display/display.h"
#include "display/widgets.h"

namespace modes::digital {

namespace {

// Large/small font pairs, largest first. The first pair where
// "HH:MM" (centered on the colon) plus ":SS" fits the width is used.
struct FontPair {
  const uint8_t* big;
  const uint8_t* small;
  int bigHeight;
};
// Seconds font: 6x12 has ~8 px digits (half of logisoso16).
const FontPair kFontPairs[] = {
    {u8g2_font_logisoso32_tn, u8g2_font_6x12_tn, 32},
    {u8g2_font_logisoso28_tn, u8g2_font_6x12_tn, 28},
    {u8g2_font_logisoso26_tn, u8g2_font_6x12_tn, 26},
    {u8g2_font_logisoso24_tn, u8g2_font_6x12_tn, 24},
};

constexpr int kTimeTop = 13;  // below the status bar
constexpr int kBottomBaseline = 63;

// Returns the right edge of ":SS" for this pair (used to check the fit).
int layoutRightEdge(U8G2& g, const FontPair& p) {
  g.setFont(p.big);
  const int colonW = g.getStrWidth(":");
  const int mmW = g.getStrWidth("88");
  g.setFont(p.small);
  const int secW = g.getStrWidth(":88");
  return g.getDisplayWidth() / 2 + (colonW + 1) / 2 + mmW + 1 + secW;
}

const FontPair& pickFonts(U8G2& g) {
  static const FontPair* chosen = nullptr;
  if (chosen) return *chosen;
  chosen = &kFontPairs[sizeof(kFontPairs) / sizeof(kFontPairs[0]) - 1];
  for (const FontPair& p : kFontPairs) {
    if (layoutRightEdge(g, p) <= g.getDisplayWidth()) {
      chosen = &p;
      break;
    }
  }
  return *chosen;
}

void drawTime(U8G2& g, const screens::State& s) {
  const FontPair& f = pickFonts(g);
  const int baseline = kTimeTop + f.bigHeight;
  const int cx = g.getDisplayWidth() / 2;
  char buf[8];

  g.setFont(f.big);
  const int colonW = g.getStrWidth(":");
  const int colonX = cx - colonW / 2;

  if (!s.timeValid) {
    if (s.blink) {
      g.drawStr(colonX, baseline, ":");
      widgets::drawStrRight(g, colonX, baseline, "--");
      g.drawStr(colonX + colonW, baseline, "--");
    }
    return;
  }

  snprintf(buf, sizeof(buf), "%02d", s.local.tm_hour);
  widgets::drawStrRight(g, colonX, baseline, buf);
  g.drawStr(colonX, baseline, ":");
  snprintf(buf, sizeof(buf), "%02d", s.local.tm_min);
  const int mmX = colonX + colonW;
  g.drawStr(mmX, baseline, buf);
  const int secX = mmX + g.getStrWidth(buf) + 1;

  g.setFont(f.small);
  snprintf(buf, sizeof(buf), ":%02d", s.local.tm_sec);
  g.drawStr(secX, baseline, buf);
}

void drawBottom(U8G2& g, const screens::State& s) {
  char buf[24];
  g.setFont(u8g2_font_6x10_tf);
  if (s.timeValid) {
    snprintf(buf, sizeof(buf), "%s %02d.%02d.%04d", widgets::kWeekdays[s.local.tm_wday % 7],
             s.local.tm_mday, s.local.tm_mon + 1, s.local.tm_year + 1900);
    g.drawStr(0, kBottomBaseline, buf);
  }
  if (s.tempValid) {
    snprintf(buf, sizeof(buf), "%.0f\xC2\xB0" "C", s.tempC);  // UTF-8 degree sign
    g.drawUTF8(g.getDisplayWidth() - g.getUTF8Width(buf), kBottomBaseline, buf);
  }
}

}  // namespace

void draw(const screens::State& s) {
  U8G2& g = display::gfx();
  g.clearBuffer();
  widgets::drawStatusBar(g, s);
  drawTime(g, s);
  drawBottom(g, s);
  g.sendBuffer();
}

}  // namespace modes::digital

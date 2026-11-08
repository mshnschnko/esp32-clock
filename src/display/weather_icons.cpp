#include "display/weather_icons.h"

#include <math.h>

namespace weather_icons {

namespace {

constexpr float kPi = 3.14159265f;

int scaled(int size, float k) { return static_cast<int>(lroundf(size * k)); }

void sun(U8G2& g, int cx, int cy, int r, bool rays) {
  g.drawDisc(cx, cy, r);
  if (!rays) return;
  const int r0 = r + 2;
  const int r1 = r + 2 + (r > 4 ? r / 2 : 1);
  for (int i = 0; i < 8; ++i) {
    const float a = i * kPi / 4;
    g.drawLine(cx + lroundf(r0 * cosf(a)), cy + lroundf(r0 * sinf(a)),
               cx + lroundf(r1 * cosf(a)), cy + lroundf(r1 * sinf(a)));
  }
}

// Crescent: a disc with an offset disc erased from it.
void moon(U8G2& g, int cx, int cy, int r) {
  g.drawDisc(cx, cy, r);
  g.setDrawColor(0);
  const int off = r / 2 + 1;
  g.drawDisc(cx + off, cy - off, r);
  g.setDrawColor(1);
}

// Filled cloud inside the box (x, y, w, h): two side bumps, one big top bump, flat bottom.
void cloudShape(U8G2& g, int x, int y, int w, int h) {
  const int rl = h * 38 / 100;
  const int rb = h / 2;
  const int rr = h * 32 / 100;
  const int base = y + h - 1;
  g.drawDisc(x + rl, base - rl, rl);
  g.drawDisc(x + w * 45 / 100, y + rb, rb);
  g.drawDisc(x + w - rr - 1, base - rr, rr);
  g.drawBox(x + rl, base - rl, w - rl - rr - 1, rl + 1);
}

// Cloud; with `outline` a 1 px black border is cut around it so it stands out
// over a sun or moon behind it. U8g2 coordinates are unsigned: the outline needs
// x >= 1 and y >= 1.
void cloud(U8G2& g, int x, int y, int w, int h, bool outline = false) {
  if (outline) {
    g.setDrawColor(0);
    cloudShape(g, x - 1, y - 1, w + 2, h + 2);
    g.setDrawColor(1);
  }
  cloudShape(g, x, y, w, h);
}

void sky(U8G2& g, int cx, int cy, int r, bool isDay, bool rays) {
  if (isDay) {
    sun(g, cx, cy, r, rays);
  } else {
    moon(g, cx, cy, r);
  }
}

// Precipitation below a cloud: three marks spread across the width.
enum class Drops { Rain, Drizzle, Snow };

void drops(U8G2& g, int x, int y, int size, Drops kind) {
  const int top = y + scaled(size, 0.68f);
  const int len = size >= 14 ? scaled(size, 0.22f) : 2;
  for (int i = 0; i < 3; ++i) {
    const int px = x + scaled(size, 0.25f + i * 0.25f);
    switch (kind) {
      case Drops::Rain:
        g.drawLine(px, top, px - len / 2, top + len);
        break;
      case Drops::Drizzle:
        g.drawPixel(px, top + 1);
        if (size >= 14) g.drawPixel(px - 1, top + len);
        break;
      case Drops::Snow:
        if (size >= 14) {
          const int c = top + len / 2;
          g.drawLine(px - 2, c, px + 2, c);
          g.drawLine(px, c - 2, px, c + 2);
          g.drawPixel(px - 1, c - 1);
          g.drawPixel(px + 1, c + 1);
          g.drawPixel(px - 1, c + 1);
          g.drawPixel(px + 1, c - 1);
        } else {
          g.drawPixel(px, top + 1);
          g.drawPixel(px - 1, top + 2);
          g.drawPixel(px + 1, top + 2);
          g.drawPixel(px, top + 3);
        }
        break;
    }
  }
}

void lightning(U8G2& g, int x, int y, int size) {
  const int top = y + scaled(size, 0.62f);
  const int cx = x + size / 2;
  const int h = size - (top - y) - 1;
  // Zigzag: down-left, right, down-left.
  const int x1 = cx - h / 3, y1 = top + h / 2;
  const int x2 = cx + h / 4, y2 = y1;
  const int x3 = cx - h / 4, y3 = top + h;
  g.drawLine(cx + h / 6, top, x1, y1);
  g.drawLine(x1, y1, x2, y2);
  g.drawLine(x2, y2, x3, y3);
  if (size >= 14) {
    g.drawLine(cx + h / 6 + 1, top, x1 + 1, y1);
    g.drawLine(x2 + 1, y2, x3 + 1, y3);
  }
}

// Three horizontal bands, alternately shifted left and right.
void fog(U8G2& g, int x, int y, int size) {
  const int gap = size >= 14 ? scaled(size, 0.18f) : 3;
  const int y0 = y + scaled(size, 0.3f);
  const int shift = scaled(size, 0.15f);
  const int len = size - shift;
  const int thickness = size >= 20 ? 2 : 1;
  for (int i = 0; i < 3; ++i) {
    const int lx = x + (i % 2 ? shift : 0);
    for (int t = 0; t < thickness; ++t) g.drawHLine(lx, y0 + i * gap + t, len);
  }
}

}  // namespace

void draw(U8G2& g, int x, int y, int size, weather::Icon icon, bool isDay) {
  using weather::Icon;
  const bool rays = size >= 14;

  switch (icon) {
    case Icon::Clear:
      sky(g, x + size / 2, y + size / 2, scaled(size, rays ? 0.22f : 0.38f), isDay, rays);
      break;

    case Icon::PartlyCloudy:
      sky(g, x + scaled(size, 0.36f), y + scaled(size, 0.34f), scaled(size, 0.2f), isDay, rays);
      cloud(g, x + scaled(size, 0.22f), y + scaled(size, 0.42f), scaled(size, 0.76f),
            scaled(size, 0.5f), /*outline=*/true);
      break;

    case Icon::Cloudy:
      cloud(g, x, y + scaled(size, 0.2f), size, scaled(size, 0.6f));
      break;

    case Icon::Fog:
      fog(g, x, y, size);
      break;

    case Icon::Drizzle:
    case Icon::Rain:
    case Icon::Snow:
    case Icon::Thunder:
      cloud(g, x, y, size, scaled(size, 0.58f));
      if (icon == Icon::Thunder) {
        lightning(g, x, y, size);
      } else {
        drops(g, x, y, size,
              icon == Icon::Rain ? Drops::Rain : icon == Icon::Snow ? Drops::Snow : Drops::Drizzle);
      }
      break;

    case Icon::Unknown:
      g.drawFrame(x + size / 4, y + size / 4, size / 2, size / 2);
      break;
  }
}

}  // namespace weather_icons

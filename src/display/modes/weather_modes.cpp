#include "display/modes/weather_modes.h"

#include <math.h>
#include <stdio.h>

#include "config.h"
#include "display/display.h"
#include "display/weather_icons.h"
#include "display/widgets.h"
#include "settings/settings.h"
#include "weather/weather_codes.h"

namespace modes {

namespace {

constexpr int kColumns = 4;
constexpr int kColumnW = 32;  // 128 / 4

const char* compass(int deg) {
  static const char* const kPoints[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  const int idx = ((deg % 360 + 360 + 22) / 45) % 8;
  return kPoints[idx];
}

void drawColumnSeparators(U8G2& g) {
  for (int c = 1; c < kColumns; ++c) {
    const int x = c * kColumnW - 1;
    for (int y = 12; y < g.getDisplayHeight(); y += 2) g.drawPixel(x, y);
  }
}

// Temperature centered on cx in the current font.
void drawTempCentered(U8G2& g, int cx, int y, float temp) {
  const int w = widgets::drawTemp(g, 0, 0, temp, /*measureOnly=*/true);
  widgets::drawTemp(g, cx - w / 2, y, temp);
}

// Precipitation amount: "0mm", "0.4mm", "12mm" (fits a 32 px column in 5x7).
void drawPrecip(U8G2& g, int cx, int y, float mm) {
  char buf[8];
  if (isnan(mm)) {
    snprintf(buf, sizeof(buf), "--");
  } else if (mm < 0.05f) {
    snprintf(buf, sizeof(buf), "0mm");
  } else if (mm < 10.0f) {
    snprintf(buf, sizeof(buf), "%.1fmm", mm);
  } else {
    snprintf(buf, sizeof(buf), "%.0fmm", mm);
  }
  widgets::drawStrCentered(g, cx, y, buf);
}

// Common frame for all weather modes: header, then either content or "no data".
template <typename DrawContent>
void drawWeatherScreen(const screens::State& s, const char* title, DrawContent content) {
  U8G2& g = display::gfx();
  g.clearBuffer();
  widgets::drawWeatherHeader(g, s, title);
  if (s.weather.valid) {
    content(g);
  } else {
    widgets::drawNoWeather(g, s);
  }
  g.sendBuffer();
}

}  // namespace

namespace weather_now {

void draw(const screens::State& s) {
  drawWeatherScreen(s, settings::get().place, [&](U8G2& g) {
    const weather::Data& w = s.weather;
    char buf[32];

    weather_icons::draw(g, 2, 12, 32, weather::iconFor(w.cond), w.isDay);

    constexpr int kTextX = 40;
    g.setFont(u8g2_font_logisoso24_tn);
    widgets::drawTemp(g, kTextX, 37, w.temp);

    g.setFont(u8g2_font_5x7_tr);
    g.drawStr(kTextX, 46, "feels");
    widgets::drawTemp(g, kTextX + g.getStrWidth("feels "), 46, w.feelsLike);

    g.setFont(u8g2_font_6x10_tr);
    g.drawStr(0, 55, weather::describe(w.cond));

    g.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "Wind %.0f m/s %s", w.windSpeed, compass(w.windDir));
    g.drawStr(0, 63, buf);
    snprintf(buf, sizeof(buf), "Hum %d%%", w.humidity);
    widgets::drawStrRight(g, g.getDisplayWidth(), 63, buf);
  });
}

}  // namespace weather_now

namespace weather_hours {

void draw(const screens::State& s) {
  drawWeatherScreen(s, "Next hours", [&](U8G2& g) {
    char buf[8];
    drawColumnSeparators(g);
    for (int i = 0; i < weather::kHourPoints && i < kColumns; ++i) {
      const weather::Hour& h = s.weather.hours[i];
      const int cx = i * kColumnW + kColumnW / 2;

      struct tm t;
      localtime_r(&h.time, &t);
      snprintf(buf, sizeof(buf), "%02d:00", t.tm_hour);
      g.setFont(u8g2_font_5x7_tr);
      widgets::drawStrCentered(g, cx, 18, buf);

      weather_icons::draw(g, cx - 8, 21, 16, weather::iconFor(h.cond), h.isDay);

      g.setFont(u8g2_font_6x10_tr);
      drawTempCentered(g, cx, 49, h.temp);

      g.setFont(u8g2_font_5x7_tr);
      drawPrecip(g, cx, 61, h.precipMm);
    }
  });
}

}  // namespace weather_hours

namespace weather_days {

void draw(const screens::State& s) {
  drawWeatherScreen(s, "Forecast", [&](U8G2& g) {
    drawColumnSeparators(g);
    for (int i = 0; i < weather::kDayPoints && i < kColumns; ++i) {
      const weather::Day& d = s.weather.days[i];
      const int cx = i * kColumnW + kColumnW / 2;

      struct tm t;
      localtime_r(&d.date, &t);
      g.setFont(u8g2_font_5x7_tr);
      widgets::drawStrCentered(g, cx, 18, i == 0 ? "Today" : widgets::kWeekdays[t.tm_wday % 7]);

      weather_icons::draw(g, cx - 8, 21, 16, weather::iconFor(d.cond), /*isDay=*/true);

      g.setFont(u8g2_font_6x10_tr);
      drawTempCentered(g, cx, 47, d.tempMax);
      g.setFont(u8g2_font_5x7_tr);
      drawTempCentered(g, cx, 55, d.tempMin);
      drawPrecip(g, cx, 63, d.precipMm);
    }
  });
}

}  // namespace weather_days

}  // namespace modes

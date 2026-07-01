#include "display.h"

#include <TFT_eSPI.h>
#include <math.h>

#include "icons.h"
#include "theme.h"

namespace display {

namespace {

TFT_eSPI tft;

constexpr int16_t W = 320;
constexpr int16_t H = 240;

// Card rectangles (x, y, w, h).
constexpr int HDR_X = 6, HDR_Y = 6, HDR_W = 308, HDR_H = 72;
constexpr int CHT_X = 6, CHT_Y = 82, CHT_W = 308, CHT_H = 66;
constexpr int FCT_X = 6, FCT_Y = 152, FCT_W = 308, FCT_H = 72;

// Chart plot rectangle.
constexpr int PL = 40, PR = 300, PT = CHT_Y + 22, PB = CHT_Y + 52;

// Vertical gradient background.
void fillVGradient(int x, int y, int w, int h, uint16_t top, uint16_t bottom) {
  for (int i = 0; i < h; i++)
    tft.drawFastHLine(x, y + i, w, theme::lerp565(top, bottom, (float)i / h));
}

void drawCard(int x, int y, int w, int h, uint16_t fill) {
  tft.fillRoundRect(x, y, w, h, 7, fill);
  tft.drawRoundRect(x, y, w, h, 7, theme::CARD_EDGE);
}

// Draw an integer temperature with a small degree ring, centred on cx.
// (TFT fonts lack a degree glyph, so we draw the ring ourselves.)
void drawTemp(int cx, int topY, int font, int temp, uint16_t color, bool big = false) {
  tft.setTextFont(font);
  tft.setTextColor(color);
  tft.setTextDatum(TL_DATUM);
  String s = String(temp);
  int w = tft.textWidth(s);
  int ringR = big ? 4 : 2;
  int total = w + ringR * 2 + 3;
  int x = cx - total / 2;
  tft.drawString(s, x, topY);
  int rx = x + w + ringR + 2;
  int ry = topY + ringR + (big ? 4 : 2);
  tft.drawCircle(rx, ry, ringR, color);
  if (big) tft.drawCircle(rx, ry, ringR - 1, color);
}

// Small drop + "NN%" rain-chance chip, centred on cx.
void rainChip(int cx, int y, int pct) {
  tft.setTextFont(1);
  String s = String(pct) + "%";
  int tw = tft.textWidth(s);
  int total = tw + 9;
  int x0 = cx - total / 2;
  icons::drawTeardrop(tft, x0 + 3, y + 4, 3, theme::DROP);
  tft.setTextColor(theme::CYAN);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(s, x0 + 9, y);
}

bool isNight(const char* hhmm) {
  if (!hhmm || strlen(hhmm) < 2) return false;
  int h = (hhmm[0] - '0') * 10 + (hhmm[1] - '0');
  return h < 7 || h >= 21;
}

void drawHeader(const buienradar::CurrentWeather& current,
                const char* locationName, bool night) {
  drawCard(HDR_X, HDR_Y, HDR_W, HDR_H, theme::CARD_HDR);

  tft.setTextFont(2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(theme::CYAN);
  tft.drawString(locationName, 16, 10);

  if (!current.valid) {
    tft.setTextColor(theme::TEXT_DIM);
    tft.drawString("Weather data unavailable", 18, 34);
    return;
  }

  // Big current temperature (color-coded).
  drawTemp(62, 22, 6, (int)lroundf(current.temperature),
           theme::tempColor(current.temperature), true);

  // Condition label (English, from the enum).
  tft.setTextFont(2);
  tft.setTextColor(theme::TEXT_DIM);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(buienradar::conditionLabel(current.condition), 18, 60);

  // Metrics column.
  const int mx = 150;
  tft.setTextFont(1);
  tft.setTextColor(theme::TEXT_DIM);
  tft.drawString("feels", mx, 22);
  drawTemp(mx + 62, 18, 2, (int)lroundf(current.feelTemperature), theme::TEXT);

  // wind glyph
  tft.drawFastHLine(mx + 2, 45, 8, theme::TEXT_DIM);
  tft.fillTriangle(mx + 10, 43, mx + 10, 47, mx + 13, 45, theme::TEXT_DIM);
  tft.setTextFont(2);
  tft.setTextColor(theme::TEXT);
  char buf[24];
  snprintf(buf, sizeof(buf), "%s %.0f m/s", current.windDirection.c_str(),
           current.windSpeedMs);
  tft.drawString(buf, mx + 18, 39);

  // humidity glyph
  icons::drawTeardrop(tft, mx + 6, 63, 3, theme::DROP);
  snprintf(buf, sizeof(buf), "%d%%", current.humidity);
  tft.drawString(buf, mx + 18, 55);

  icons::drawWeatherIcon(tft, 272, 44, 22, current.condition, night);
}

void drawChart(const buienradar::Nowcast& nowcast) {
  drawCard(CHT_X, CHT_Y, CHT_W, CHT_H, theme::CARD);

  tft.setTextFont(2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(theme::CYAN);
  tft.drawString("Rain - next 2 hours", 16, CHT_Y + 4);
  tft.setTextFont(1);
  tft.setTextColor(theme::TEXT_DIM);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("mm/h", W - 16, CHT_Y + 6);

  if (!nowcast.valid || nowcast.count == 0) {
    tft.setTextFont(2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(theme::TEXT_DIM);
    tft.drawString("Nowcast unavailable", W / 2, CHT_Y + 34);
    return;
  }

  float scaleMax = nowcast.maxMmPerHour();
  if (scaleMax < 1.0f) scaleMax = 1.0f;
  scaleMax = ceilf(scaleMax);
  const int plotH = PB - PT;

  tft.setTextFont(1);
  tft.setTextColor(theme::TEXT_DIM);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(String((int)scaleMax), PL - 5, PT - 4);
  tft.drawString("0", PL - 5, PB - 8);
  tft.drawFastHLine(PL, PT, PR - PL, theme::GRID_TOP);
  tft.drawFastHLine(PL, PB, PR - PL, theme::GRID_BASE);

  if (!nowcast.anyRain()) {
    tft.setTextFont(4);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(theme::LIME);
    tft.drawString("Dry", (PL + PR) / 2, (PT + PB) / 2);
  } else {
    float barW = (float)(PR - PL) / nowcast.count;
    for (size_t i = 0; i < nowcast.count; i++) {
      float mm = nowcast.slots[i].mmPerHour;
      int bh = (int)roundf((mm / scaleMax) * plotH);
      if (mm > 0 && bh < 1) bh = 1;
      int x = PL + (int)(i * barW);
      int w = (int)ceilf(barW) - 1;
      if (w < 1) w = 1;
      if (bh > 0) tft.fillRect(x, PB - bh, w, bh, theme::rainColor(mm));
    }
  }

  // X-axis time labels.
  tft.setTextFont(1);
  tft.setTextColor(theme::TEXT_DIM);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(nowcast.slots[0].time, PL, PB + 2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(nowcast.slots[nowcast.count / 2].time, (PL + PR) / 2, PB + 2);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(nowcast.slots[nowcast.count - 1].time, PR, PB + 2);
}

void drawForecast(const buienradar::Forecast& forecast) {
  drawCard(FCT_X, FCT_Y, FCT_W, FCT_H, theme::CARD);

  if (!forecast.valid || forecast.count == 0) {
    tft.setTextFont(2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(theme::TEXT_DIM);
    tft.drawString("Forecast unavailable", W / 2, FCT_Y + FCT_H / 2);
    return;
  }

  int n = forecast.count;
  float colW = (float)FCT_W / n;
  for (int i = 0; i < n; i++) {
    const buienradar::DayForecast& d = forecast.days[i];
    int cx = FCT_X + (int)(colW * i + colW / 2);
    if (i > 0)
      tft.drawFastVLine(FCT_X + (int)(colW * i), FCT_Y + 6, FCT_H - 12, theme::SEP);

    tft.setTextFont(2);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(theme::CYAN);
    tft.drawString(d.weekday, cx, FCT_Y + 5);

    icons::drawWeatherIcon(tft, cx, FCT_Y + 29, 10, d.condition, false);

    drawTemp(cx - 15, FCT_Y + 45, 2, (int)lroundf(d.maxTemp), theme::tempColor(d.maxTemp));
    drawTemp(cx + 15, FCT_Y + 45, 2, (int)lroundf(d.minTemp), theme::tempColor(d.minTemp));

    rainChip(cx, FCT_Y + 60, d.rainChance);
  }
}

void drawFooter(const char* updatedHHMM, bool stale) {
  tft.setTextFont(1);
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(stale ? theme::ORANGE : theme::FOOTER);
  String s = String("Source: Buienradar.nl   -   ") + (stale ? "! " : "") +
             "updated " + updatedHHMM;
  tft.drawString(s, 8, H - 2);
}

}  // namespace

void init() {
  tft.init();
  tft.setRotation(1);  // landscape 320x240
  fillVGradient(0, 0, W, H, theme::BG_TOP, theme::BG_BOTTOM);
}

void showBoot(const char* line1, const char* line2) {
  fillVGradient(0, 0, W, H, theme::BG_TOP, theme::BG_BOTTOM);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(theme::CYAN);
  tft.drawString("Buienradar", W / 2, H / 2 - 30);
  tft.setTextFont(2);
  tft.setTextColor(theme::TEXT);
  if (line1) tft.drawString(line1, W / 2, H / 2 + 4);
  if (line2) {
    tft.setTextColor(theme::TEXT_DIM);
    tft.drawString(line2, W / 2, H / 2 + 24);
  }
}

void render(const buienradar::CurrentWeather& current,
            const buienradar::Forecast& forecast,
            const buienradar::Nowcast& nowcast, const char* locationName,
            const char* updatedHHMM, bool stale) {
  fillVGradient(0, 0, W, H, theme::BG_TOP, theme::BG_BOTTOM);
  drawHeader(current, locationName, isNight(updatedHHMM));
  drawChart(nowcast);
  drawForecast(forecast);
  drawFooter(updatedHHMM, stale);
}

}  // namespace display

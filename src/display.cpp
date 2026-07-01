#include "display.h"

#include <TFT_eSPI.h>
#include <math.h>

#include "icons.h"

namespace display {

namespace {

TFT_eSPI tft;

constexpr int16_t W = 320;
constexpr int16_t H = 240;

// Region boundaries (landscape).
constexpr int HEADER_BOTTOM = 52;
constexpr int CHART_TOP = 54;
constexpr int CHART_BOTTOM = 150;
constexpr int FORECAST_TOP = 152;
constexpr int FOOTER_TOP = 231;

// Chart plot rectangle.
constexpr int PLOT_LEFT = 34;
constexpr int PLOT_RIGHT = 315;
constexpr int PLOT_TOP = 72;
constexpr int PLOT_BASE = 140;  // y of the "dry" baseline

const uint16_t COL_BG = TFT_BLACK;
const uint16_t COL_TEXT = TFT_WHITE;
const uint16_t COL_DIM = TFT_LIGHTGREY;
const uint16_t COL_ACCENT = TFT_CYAN;
const uint16_t COL_WARN = TFT_ORANGE;

// Draw an integer temperature with a small degree ring, centred on cx.
void drawTemp(int cx, int topY, int font, int temp, uint16_t color) {
  tft.setTextFont(font);
  tft.setTextColor(color, COL_BG);
  tft.setTextDatum(TL_DATUM);
  String s = String(temp);
  int w = tft.textWidth(s);
  int ringR = (font >= 6) ? 4 : 2;
  int total = w + ringR * 2 + 3;
  int x = cx - total / 2;
  tft.drawString(s, x, topY);
  int ringX = x + w + ringR + 2;
  int ringY = topY + ringR + 2;
  tft.drawCircle(ringX, ringY, ringR, color);
}

uint16_t rainColor(float mm) {
  if (mm < 0.5f) return TFT_CYAN;
  if (mm < 2.0f) return 0x05FF;   // light blue
  if (mm < 5.0f) return TFT_BLUE;
  return 0x781F;                  // purple = heavy
}

void drawHeader(const buienradar::CurrentWeather& current,
                const char* locationName, const char* updatedHHMM, bool stale) {
  // Location (left) and update time (right).
  tft.setTextFont(2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString(locationName, 4, 2);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(stale ? COL_WARN : COL_DIM, COL_BG);
  String upd = (stale ? String("! ") : String("")) + "bijgewerkt " +
               String(updatedHHMM);
  tft.drawString(upd, W - 4, 2);

  // Big current temperature (left).
  if (current.valid) {
    drawTemp(46, 16, 6, (int)lroundf(current.temperature), COL_TEXT);

    // Secondary details next to the temperature.
    tft.setTextFont(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COL_DIM, COL_BG);
    char buf[32];
    snprintf(buf, sizeof(buf), "voelt %d C", (int)lroundf(current.feelTemperature));
    tft.drawString(buf, 92, 20);
    snprintf(buf, sizeof(buf), "wind %s %.0f m/s",
             current.windDirection.c_str(), current.windSpeedMs);
    tft.drawString(buf, 92, 32);
    snprintf(buf, sizeof(buf), "RV %d%%", current.humidity);
    tft.drawString(buf, 92, 44);

    // Condition icon (right).
    icons::drawWeatherIcon(tft, 285, 30, 16, current.condition);
  } else {
    tft.setTextFont(2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COL_DIM, COL_BG);
    tft.drawString("Actueel weer niet beschikbaar", 4, 24);
  }

  tft.drawFastHLine(0, HEADER_BOTTOM, W, TFT_DARKGREY);
}

void drawChart(const buienradar::Nowcast& nowcast) {
  tft.setTextFont(2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Neerslag komende 2 uur", 4, CHART_TOP + 1);

  if (!nowcast.valid || nowcast.count == 0) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COL_DIM, COL_BG);
    tft.drawString("Nowcast niet beschikbaar", W / 2, (PLOT_TOP + PLOT_BASE) / 2);
    return;
  }

  // Y scale: at least 1 mm/h so light rain is visible; grow for heavier rain.
  float scaleMax = nowcast.maxMmPerHour();
  if (scaleMax < 1.0f) scaleMax = 1.0f;
  scaleMax = ceilf(scaleMax);

  const int plotH = PLOT_BASE - PLOT_TOP;

  // Y-axis labels + gridlines.
  tft.setTextFont(1);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString(String((int)scaleMax), PLOT_LEFT - 3, PLOT_TOP - 3);
  tft.drawString("0", PLOT_LEFT - 3, PLOT_BASE - 4);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("mm/u", 2, PLOT_TOP - 12);
  tft.drawFastHLine(PLOT_LEFT, PLOT_TOP, PLOT_RIGHT - PLOT_LEFT, 0x2104);
  tft.drawFastHLine(PLOT_LEFT, PLOT_BASE, PLOT_RIGHT - PLOT_LEFT, TFT_DARKGREY);

  if (!nowcast.anyRain()) {
    tft.setTextFont(2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.drawString("Droog", (PLOT_LEFT + PLOT_RIGHT) / 2, (PLOT_TOP + PLOT_BASE) / 2);
  } else {
    int plotW = PLOT_RIGHT - PLOT_LEFT;
    float barW = (float)plotW / nowcast.count;
    for (size_t i = 0; i < nowcast.count; i++) {
      float mm = nowcast.slots[i].mmPerHour;
      int bh = (int)roundf((mm / scaleMax) * plotH);
      if (mm > 0 && bh < 1) bh = 1;
      int x = PLOT_LEFT + (int)(i * barW);
      int w = (int)ceilf(barW) - 1;
      if (w < 1) w = 1;
      if (bh > 0)
        tft.fillRect(x, PLOT_BASE - bh, w, bh, rainColor(mm));
    }
  }

  // X-axis time labels: start, middle, end.
  tft.setTextFont(1);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(nowcast.slots[0].time, PLOT_LEFT, PLOT_BASE + 3);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(nowcast.slots[nowcast.count / 2].time,
                 (PLOT_LEFT + PLOT_RIGHT) / 2, PLOT_BASE + 3);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(nowcast.slots[nowcast.count - 1].time, PLOT_RIGHT, PLOT_BASE + 3);
}

void drawForecast(const buienradar::Forecast& forecast) {
  if (!forecast.valid || forecast.count == 0) {
    tft.setTextFont(2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COL_DIM, COL_BG);
    tft.drawString("Verwachting niet beschikbaar", W / 2, FORECAST_TOP + 30);
    return;
  }

  int cols = forecast.count;
  int colW = W / cols;
  for (int i = 0; i < cols; i++) {
    const buienradar::DayForecast& d = forecast.days[i];
    int cx = i * colW + colW / 2;
    if (i > 0) tft.drawFastVLine(i * colW, FORECAST_TOP + 2, FOOTER_TOP - FORECAST_TOP - 4, 0x2104);

    tft.setTextFont(2);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.drawString(d.weekday, cx, FORECAST_TOP + 2);

    icons::drawWeatherIcon(tft, cx, FORECAST_TOP + 32, 11, d.condition);

    drawTemp(cx - 12, FORECAST_TOP + 46, 2, (int)lroundf(d.maxTemp), COL_TEXT);
    drawTemp(cx + 14, FORECAST_TOP + 46, 2, (int)lroundf(d.minTemp), COL_DIM);

    tft.setTextFont(1);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COL_ACCENT, COL_BG);
    char buf[12];
    snprintf(buf, sizeof(buf), "%d%%", d.rainChance);
    tft.drawString(buf, cx, FORECAST_TOP + 64);
  }
}

}  // namespace

void init() {
  tft.init();
  tft.setRotation(1);  // landscape 320x240
  tft.fillScreen(COL_BG);
  tft.setTextColor(COL_TEXT, COL_BG);
}

void showBoot(const char* line1, const char* line2) {
  tft.fillScreen(COL_BG);
  tft.setTextFont(4);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString("Buienradar", W / 2, H / 2 - 30);
  tft.setTextFont(2);
  tft.setTextColor(COL_TEXT, COL_BG);
  if (line1) tft.drawString(line1, W / 2, H / 2 + 4);
  if (line2) {
    tft.setTextColor(COL_DIM, COL_BG);
    tft.drawString(line2, W / 2, H / 2 + 24);
  }
}

void render(const buienradar::CurrentWeather& current,
            const buienradar::Forecast& forecast,
            const buienradar::Nowcast& nowcast, const char* locationName,
            const char* updatedHHMM, bool stale) {
  tft.fillScreen(COL_BG);
  drawHeader(current, locationName, updatedHHMM, stale);
  drawChart(nowcast);
  drawForecast(forecast);

  // Footer attribution (required by Buienradar's terms).
  tft.setTextFont(1);
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(TFT_DARKGREY, COL_BG);
  tft.drawString("Bron: Buienradar.nl", 4, H - 1);
}

}  // namespace display

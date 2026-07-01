#pragma once
#include <TFT_eSPI.h>
#include <math.h>

#include "buienradar.h"

// -----------------------------------------------------------------------------
// Lightweight weather icons drawn with TFT primitives (no bitmap assets).
// Each icon is drawn centred on (cx, cy) fitting roughly within radius r.
// -----------------------------------------------------------------------------

namespace icons {

inline void drawSun(TFT_eSPI& tft, int cx, int cy, int r, uint16_t color) {
  for (int a = 0; a < 360; a += 45) {
    float rad = a * 0.01745329f;
    int x0 = cx + (int)(cosf(rad) * (r + 2));
    int y0 = cy + (int)(sinf(rad) * (r + 2));
    int x1 = cx + (int)(cosf(rad) * (r + r / 2 + 3));
    int y1 = cy + (int)(sinf(rad) * (r + r / 2 + 3));
    tft.drawLine(x0, y0, x1, y1, color);
  }
  tft.fillCircle(cx, cy, r, color);
}

inline void drawCloud(TFT_eSPI& tft, int cx, int cy, int r, uint16_t color) {
  tft.fillCircle(cx - r / 2, cy, (r * 5) / 9, color);
  tft.fillCircle(cx + r / 2, cy, (r * 3) / 5, color);
  tft.fillCircle(cx, cy - (r * 2) / 5, (r * 3) / 5, color);
  tft.fillRect(cx - r, cy, 2 * r, (r * 3) / 5, color);
}

inline void drawRainStreaks(TFT_eSPI& tft, int cx, int cy, int r,
                            uint16_t color, int count) {
  int baseY = cy + (r * 3) / 5;
  int step = (2 * r) / (count + 1);
  for (int i = 1; i <= count; i++) {
    int x = cx - r + i * step;
    tft.drawLine(x, baseY, x - r / 4, baseY + r / 2, color);
  }
}

inline void drawWeatherIcon(TFT_eSPI& tft, int cx, int cy, int r,
                            buienradar::Condition cond) {
  using buienradar::Condition;
  const uint16_t cloudLight = TFT_SILVER;
  const uint16_t cloudDark = TFT_DARKGREY;
  const uint16_t sunColor = TFT_YELLOW;
  const uint16_t rainColor = TFT_CYAN;

  switch (cond) {
    case Condition::Clear:
      drawSun(tft, cx, cy, (r * 2) / 3, sunColor);
      break;
    case Condition::Partly:
      drawSun(tft, cx + r / 3, cy - r / 3, r / 2, sunColor);
      drawCloud(tft, cx - r / 4, cy + r / 5, (r * 3) / 4, cloudLight);
      break;
    case Condition::Cloudy:
      drawCloud(tft, cx, cy, r, cloudLight);
      break;
    case Condition::Fog:
      drawCloud(tft, cx, cy - r / 4, (r * 4) / 5, cloudLight);
      for (int i = 0; i < 3; i++)
        tft.drawFastHLine(cx - r, cy + r / 2 + i * (r / 4), 2 * r, cloudDark);
      break;
    case Condition::Rain:
      drawCloud(tft, cx, cy - r / 5, r, cloudLight);
      drawRainStreaks(tft, cx, cy - r / 5, r, rainColor, 3);
      break;
    case Condition::HeavyRain:
      drawCloud(tft, cx, cy - r / 5, r, cloudDark);
      drawRainStreaks(tft, cx, cy - r / 5, r, rainColor, 5);
      break;
    case Condition::Snow:
      drawCloud(tft, cx, cy - r / 5, r, cloudLight);
      for (int i = 0; i < 3; i++) {
        int x = cx - r / 2 + i * (r / 2);
        tft.fillCircle(x, cy + (r * 3) / 5, 2, TFT_WHITE);
      }
      break;
    case Condition::Thunder:
      drawCloud(tft, cx, cy - r / 5, r, cloudDark);
      tft.fillTriangle(cx - 2, cy + r / 3, cx + r / 4, cy + r / 3,
                       cx - r / 5, cy + r, sunColor);
      break;
    case Condition::Unknown:
    default:
      drawCloud(tft, cx, cy, r, cloudLight);
      tft.setTextColor(TFT_DARKGREY);
      tft.drawCentreString("?", cx, cy - 4, 2);
      break;
  }
}

}  // namespace icons

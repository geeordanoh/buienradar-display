#pragma once
#include <TFT_eSPI.h>
#include <math.h>

#include "buienradar.h"
#include "theme.h"

// -----------------------------------------------------------------------------
// Colorful weather icons drawn with TFT primitives (no bitmap assets), tuned to
// pop on the dark cards. Each icon is centred on (cx, cy) fitting radius r.
// Geometry mirrors the approved host mockups.
// -----------------------------------------------------------------------------

namespace icons {

inline void drawSun(TFT_eSPI& tft, int cx, int cy, int r) {
  for (int a = 0; a < 360; a += 45) {
    float rad = a * 0.01745329f, pr = rad + 0.16f, mr = rad - 0.16f;
    int b1x = cx + cosf(mr) * (r + 1), b1y = cy + sinf(mr) * (r + 1);
    int b2x = cx + cosf(pr) * (r + 1), b2y = cy + sinf(pr) * (r + 1);
    int tx = cx + cosf(rad) * (r + r * 0.8f), ty = cy + sinf(rad) * (r + r * 0.8f);
    tft.fillTriangle(b1x, b1y, b2x, b2y, tx, ty, theme::SUN_CORE);
  }
  tft.fillCircle(cx, cy, r, theme::SUN_EDGE);
  tft.fillCircle(cx, cy, r - 2, theme::SUN_CORE);
}

inline void drawMoon(TFT_eSPI& tft, int cx, int cy, int r) {
  tft.fillCircle(cx, cy, r, theme::MOON);
  tft.fillCircle(cx - r / 3, cy - r / 3, r / 6 + 1, theme::MOON_CR);
  tft.fillCircle(cx + r / 4, cy + r / 5, r / 8 + 1, theme::MOON_CR);
}

inline void cloudPuffs(TFT_eSPI& tft, int cx, int cy, int r, uint16_t c) {
  int baseH = (r * 3) / 5;
  tft.fillCircle(cx - r / 2, cy, (r * 5) / 9, c);
  tft.fillCircle(cx + r / 2, cy, (r * 3) / 5, c);
  tft.fillCircle(cx, cy - (r * 2) / 5, (r * 3) / 5, c);
  tft.fillRect(cx - r, cy, 2 * r, baseH, c);
}

inline void drawCloud(TFT_eSPI& tft, int cx, int cy, int r,
                      uint16_t body = theme::CLOUD_W,
                      uint16_t shadow = theme::CLOUD_SH) {
  cloudPuffs(tft, cx, cy + 2, r, shadow);
  cloudPuffs(tft, cx, cy, r, body);
}

inline void drawTeardrop(TFT_eSPI& tft, int cx, int cy, int s, uint16_t color) {
  tft.fillCircle(cx, cy, s, color);
  tft.fillTriangle(cx - s, cy, cx + s, cy, cx, cy - 2 * s, color);
}

inline void drawSnowflake(TFT_eSPI& tft, int cx, int cy, int s, uint16_t color) {
  int d = (s * 7) / 10;
  tft.drawLine(cx - s, cy, cx + s, cy, color);
  tft.drawLine(cx, cy - s, cx, cy + s, color);
  tft.drawLine(cx - d, cy - d, cx + d, cy + d, color);
  tft.drawLine(cx - d, cy + d, cx + d, cy - d, color);
}

inline void drawBolt(TFT_eSPI& tft, int cx, int cy, int s, uint16_t color) {
  tft.fillTriangle(cx - s / 4, cy - s, cx + s / 4, cy - s / 8, cx - s / 8,
                   cy - s / 8, color);
  tft.fillTriangle(cx - s / 8, cy - s / 8, cx + s / 6, cy - s / 8, cx - s / 6,
                   cy + s, color);
}

inline void drawWeatherIcon(TFT_eSPI& tft, int cx, int cy, int r,
                            buienradar::Condition cond, bool night = false) {
  using buienradar::Condition;
  switch (cond) {
    case Condition::Clear:
      if (night) drawMoon(tft, cx, cy, (r * 7) / 10);
      else drawSun(tft, cx, cy, (r * 7) / 10);
      break;
    case Condition::Partly: {
      int sr = (r * 11) / 20;
      if (night) drawMoon(tft, cx + r / 2, cy - r / 2, sr);
      else drawSun(tft, cx + r / 2, cy - r / 2, sr);
      drawCloud(tft, cx - r / 5, cy + r / 6, (r * 4) / 5);
      break;
    }
    case Condition::Cloudy:
      drawCloud(tft, cx, cy, r);
      break;
    case Condition::Fog:
      drawCloud(tft, cx, cy - r / 4, (r * 4) / 5);
      for (int i = 0; i < 3; i++)
        tft.drawFastHLine(cx - r, cy + r / 2 + i * (r / 4), 2 * r, theme::CLOUD_DK);
      break;
    case Condition::Rain:
      drawCloud(tft, cx, cy - r / 5, r);
      for (int i = 0; i < 3; i++)
        drawTeardrop(tft, cx - r / 2 + i * (r / 2), cy + (r * 3) / 5,
                     r / 6 < 2 ? 2 : r / 6, theme::DROP);
      break;
    case Condition::HeavyRain:
      drawCloud(tft, cx, cy - r / 5, r, theme::CLOUD_SH, theme::CLOUD_DK);
      for (int i = 0; i < 5; i++)
        drawTeardrop(tft, cx - r + i * (r / 2) + 2, cy + (r * 3) / 5,
                     r / 6 < 2 ? 2 : r / 6, theme::DROP_DK);
      break;
    case Condition::Snow:
      drawCloud(tft, cx, cy - r / 5, r);
      for (int i = 0; i < 3; i++)
        drawSnowflake(tft, cx - r / 2 + i * (r / 2), cy + (r * 3) / 5,
                      r / 5 < 3 ? 3 : r / 5, theme::SNOW);
      break;
    case Condition::Thunder:
      drawCloud(tft, cx, cy - r / 5, r, theme::CLOUD_SH, theme::CLOUD_DK);
      drawBolt(tft, cx, cy + r / 2, (r * 7) / 10, theme::BOLT);
      break;
    case Condition::Unknown:
    default:
      drawCloud(tft, cx, cy, r);
      tft.setTextColor(theme::CLOUD_DK);
      tft.drawCentreString("?", cx, cy - 4, 2);
      break;
  }
}

}  // namespace icons

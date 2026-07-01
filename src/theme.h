#pragma once
#include <stdint.h>

// -----------------------------------------------------------------------------
// Dark-mode palette + color helpers, shared by display.cpp and icons.h.
// Values mirror the approved host mockups. RGB565 throughout.
// -----------------------------------------------------------------------------

namespace theme {

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Backgrounds & cards
constexpr uint16_t BG_TOP     = rgb565(13, 17, 23);
constexpr uint16_t BG_BOTTOM  = rgb565(26, 32, 44);
constexpr uint16_t CARD       = rgb565(28, 34, 50);
constexpr uint16_t CARD_HDR   = rgb565(34, 41, 60);
constexpr uint16_t CARD_EDGE  = rgb565(64, 74, 102);
constexpr uint16_t GRID_TOP   = rgb565(40, 46, 62);
constexpr uint16_t GRID_BASE  = rgb565(70, 78, 100);
constexpr uint16_t SEP        = rgb565(44, 50, 68);

// Text
constexpr uint16_t TEXT       = rgb565(238, 241, 246);
constexpr uint16_t TEXT_DIM   = rgb565(150, 160, 182);
constexpr uint16_t FOOTER     = rgb565(120, 130, 150);

// Accents
constexpr uint16_t CYAN       = rgb565(0, 200, 255);
constexpr uint16_t ORANGE     = rgb565(255, 159, 40);
constexpr uint16_t LIME       = rgb565(170, 232, 90);

// Icon colors
constexpr uint16_t SUN_CORE   = rgb565(255, 214, 60);
constexpr uint16_t SUN_EDGE   = rgb565(255, 146, 28);
constexpr uint16_t CLOUD_W    = rgb565(238, 242, 248);
constexpr uint16_t CLOUD_SH   = rgb565(168, 178, 198);
constexpr uint16_t CLOUD_DK   = rgb565(116, 126, 148);
constexpr uint16_t DROP       = rgb565(74, 154, 255);
constexpr uint16_t DROP_DK    = rgb565(42, 112, 224);
constexpr uint16_t SNOW       = rgb565(236, 246, 255);
constexpr uint16_t MOON       = rgb565(240, 238, 212);
constexpr uint16_t MOON_CR    = rgb565(214, 212, 180);
constexpr uint16_t BOLT       = rgb565(255, 208, 48);

// Blend two RGB565 colors (t in 0..1).
inline uint16_t lerp565(uint16_t a, uint16_t b, float t) {
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
  int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
  int rr = ar + (int)((br - ar) * t + 0.5f);
  int rg = ag + (int)((bg - ag) * t + 0.5f);
  int rb = ab + (int)((bb - ab) * t + 0.5f);
  return (uint16_t)((rr << 11) | (rg << 5) | rb);
}

// Temperature -> color ramp (cold blue ... hot red-orange).
inline uint16_t tempColor(float c) {
  static const float T[] = {-10, 0, 8, 15, 21, 27, 35};
  static const uint16_t C[] = {
      rgb565(90, 150, 240),  rgb565(70, 180, 240),  rgb565(80, 210, 180),
      rgb565(150, 220, 90),  rgb565(245, 205, 70),  rgb565(250, 150, 50),
      rgb565(240, 80, 60)};
  const int n = 7;
  if (c <= T[0]) return C[0];
  if (c >= T[n - 1]) return C[n - 1];
  for (int i = 0; i < n - 1; i++) {
    if (c <= T[i + 1]) return lerp565(C[i], C[i + 1], (c - T[i]) / (T[i + 1] - T[i]));
  }
  return C[n - 1];
}

// Precipitation intensity (mm/h) -> bar color (light blue ... purple).
inline uint16_t rainColor(float mm) {
  if (mm < 0.2f) return rgb565(120, 200, 255);
  if (mm < 1.0f) return rgb565(74, 160, 255);
  if (mm < 3.0f) return rgb565(44, 124, 242);
  if (mm < 6.0f) return rgb565(96, 96, 232);
  return rgb565(168, 86, 224);
}

}  // namespace theme

#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Build-time configuration.
//
// These values are injected as compiler defines from the shell environment by
// platformio.ini (see secrets.ini.example). Sensible fallbacks keep the build
// compiling even when the variables are not set.
// -----------------------------------------------------------------------------

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif
#ifndef LATITUDE_STR
#define LATITUDE_STR ""
#endif
#ifndef LONGITUDE_STR
#define LONGITUDE_STR ""
#endif
#ifndef LOCATION_NAME
#define LOCATION_NAME ""
#endif

namespace config {

inline const char* wifiSsid() { return WIFI_SSID; }
inline const char* wifiPass() { return WIFI_PASS; }

// Fall back to Amsterdam if no coordinates were provided at flash time.
inline float latitude() {
  float v = atof(LATITUDE_STR);
  return v != 0.0f ? v : 52.3676f;
}
inline float longitude() {
  float v = atof(LONGITUDE_STR);
  return v != 0.0f ? v : 4.9041f;
}

inline const char* locationName() {
  return strlen(LOCATION_NAME) ? LOCATION_NAME : "Weer";
}

// Refresh cadence: fetch + redraw every 5 minutes.
constexpr uint32_t REFRESH_INTERVAL_MS = 5UL * 60UL * 1000UL;

// Europe/Amsterdam (CET/CEST) for the "last updated" clock.
constexpr const char* TZ_INFO = "CET-1CEST,M3.5.0,M10.5.0/3";

}  // namespace config

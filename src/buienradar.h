#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Buienradar data model + fetchers.
//
// Two endpoints are used:
//   1. https://gpsgadget.buienradar.nl/data/raintext?lat=..&lon=..
//      -> ~24 lines "value|HH:MM" (5-min steps, 2h nowcast).
//   2. https://data.buienradar.nl/2.0/feed/json
//      -> current observations (nearest station) + 5-day forecast.
//
// Data usage note: Buienradar data is free for NON-COMMERCIAL use only and
// requires attribution / a link back to buienradar.nl.
// -----------------------------------------------------------------------------

namespace buienradar {

// Normalised weather condition, derived from the Dutch weather description so
// the display can pick an icon independent of Buienradar's own icon codes.
enum class Condition : uint8_t {
  Unknown,
  Clear,      // zonnig / onbewolkt / helder
  Partly,     // half / afwisselend bewolkt
  Cloudy,     // (zwaar) bewolkt
  Fog,        // mist / nevel
  Rain,       // regen / buien / motregen
  HeavyRain,  // zware regen / hevige buien
  Snow,       // sneeuw / hagel
  Thunder,    // onweer
};

Condition conditionFromDescription(const String& description);

// One 5-minute nowcast slot.
struct RainSlot {
  uint8_t value = 0;      // raw 0..255 from Buienradar
  float mmPerHour = 0.0f; // converted precipitation intensity
  char time[6] = "--:--"; // "HH:MM"
};

constexpr size_t MAX_RAIN_SLOTS = 30;  // feed gives ~24-25

struct Nowcast {
  RainSlot slots[MAX_RAIN_SLOTS];
  size_t count = 0;
  bool valid = false;
  float maxMmPerHour() const;
  bool anyRain() const;  // true if any slot > 0
};

// Current conditions from the nearest weather station.
struct CurrentWeather {
  bool valid = false;
  float temperature = 0.0f;
  float feelTemperature = 0.0f;
  int humidity = 0;
  float windSpeedMs = 0.0f;     // m/s
  String windDirection;         // e.g. "ZW"
  float airPressure = 0.0f;     // hPa
  String description;
  Condition condition = Condition::Unknown;
};

// One day of the multi-day forecast.
struct DayForecast {
  bool valid = false;
  char weekday[3] = "--";  // Dutch abbrev: Ma/Di/Wo/Do/Vr/Za/Zo
  float minTemp = 0.0f;
  float maxTemp = 0.0f;
  int rainChance = 0;      // %
  int sunChance = 0;       // %
  String description;
  Condition condition = Condition::Unknown;
};

constexpr size_t MAX_FORECAST_DAYS = 5;

struct Forecast {
  DayForecast days[MAX_FORECAST_DAYS];
  size_t count = 0;
  bool valid = false;
};

// Convert a raw Buienradar rain value (0..255) to mm/hour.
float rainValueToMmPerHour(int value);

// Fetch the 2-hour rain nowcast for the given coordinates. Returns false on
// network/parse failure; on success fills `out`.
bool fetchNowcast(float lat, float lon, Nowcast& out);

// Fetch the JSON feed and extract the nearest-station current weather plus the
// 5-day forecast. Returns false on failure.
bool fetchWeather(float lat, float lon, CurrentWeather& current, Forecast& forecast);

}  // namespace buienradar

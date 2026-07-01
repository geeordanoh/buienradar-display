#pragma once
#include "buienradar.h"

// -----------------------------------------------------------------------------
// Rendering for the CYD (landscape 320x240):
//   - header:   location, current temperature, condition icon, last update
//   - middle:   2-hour rain nowcast chart
//   - bottom:   5-day forecast with weather icons
// -----------------------------------------------------------------------------

namespace display {

void init();

// Full-screen boot / connection message.
void showBoot(const char* line1, const char* line2 = nullptr);

// Render everything. `updatedHHMM` is a "HH:MM" string; `stale` flags that the
// last fetch failed and the shown data is old.
void render(const buienradar::CurrentWeather& current,
            const buienradar::Forecast& forecast,
            const buienradar::Nowcast& nowcast, const char* locationName,
            const char* updatedHHMM, bool stale);

}  // namespace display

# buienradar-display

Firmware for the **ESP32-2432S028R** ("Cheap Yellow Display" / CYD) — a 2.8"
240×320 ILI9341 TFT with an onboard ESP32 — that shows Dutch weather from
[Buienradar](https://www.buienradar.nl):

- **Header** — current temperature, "feels like", wind, humidity and a condition
  icon for your location.
- **Middle** — a chart of the **2-hour rain nowcast** (precipitation intensity in
  mm/h, in 5-minute steps).
- **Bottom** — a **5-day forecast** with weather icons, max/min temperature and
  rain chance.

The display refreshes every **5 minutes**. WiFi credentials and the location are
compiled in when you flash the firmware.

## What data is fetched

Two public Buienradar endpoints:

1. **Rain nowcast** — `https://gpsgadget.buienradar.nl/data/raintext?lat=..&lon=..`
   Returns ~24 lines of `value|HH:MM` (5-min steps, 2 hours). Each raw value
   (0–255) is converted to mm/h with `10^((value − 109) / 32)`.
2. **Observations + forecast** — `https://data.buienradar.nl/2.0/feed/json`
   The nearest weather station (temperature, feels-like, humidity, wind,
   pressure, description) plus the 5-day forecast. Parsed on-device with an
   ArduinoJson filter so only the needed fields are kept in RAM.

> ⚠️ **Data usage:** Buienradar data is free for **non-commercial** use only and
> requires attribution / a link to buienradar.nl. The footer credits the source
> on-screen; keep it if you redistribute this.

## Hardware

ESP32-2432S028R (DIYmalls "2.8 inch Yellow Display"). No wiring needed — the
ESP32 and TFT are on one board. Pin mapping is set in `platformio.ini` via
`TFT_eSPI` build flags, so **no library files need editing**.

Some CYD units use a slightly different panel. If colours look wrong or the
screen stays blank, swap the driver flag in `platformio.ini`:
`-DILI9341_2_DRIVER=1` → try `-DILI9341_DRIVER=1` or `-DST7789_DRIVER=1`.

## Build & flash

Requires [PlatformIO](https://platformio.org/) (`pip install platformio`).

Credentials and location come from **environment variables** at flash time —
nothing secret is committed. Copy the example and fill it in:

```bash
cp secrets.ini.example secrets.sh   # then edit secrets.sh
source secrets.sh
pio run -t upload
```

Or pass them inline:

```bash
WIFI_SSID="MyNetwork" WIFI_PASS="hunter2" \
LATITUDE=52.0907 LONGITUDE=5.1214 LOCATION_NAME="Utrecht" \
pio run -t upload
```

Variables:

| Variable        | Meaning                                   |
| --------------- | ----------------------------------------- |
| `WIFI_SSID`     | WiFi network name                         |
| `WIFI_PASS`     | WiFi password                             |
| `LATITUDE`      | Location latitude (decimal degrees)       |
| `LONGITUDE`     | Location longitude (decimal degrees)      |
| `LOCATION_NAME` | Short label shown in the header           |

The project still **compiles** with these unset (falls back to empty WiFi +
Amsterdam coordinates), but it won't connect until real credentials are flashed.

Serial monitor: `pio device monitor` (115200 baud).

## Project layout

| File                 | Purpose                                                  |
| -------------------- | -------------------------------------------------------- |
| `platformio.ini`     | Board, libraries, TFT pin config, flash-time env vars    |
| `src/config.h`       | Reads build-time config with safe fallbacks              |
| `src/buienradar.*`   | Fetch + parse both Buienradar endpoints into structs     |
| `src/display.*`      | Screen layout: header, rain chart, forecast row          |
| `src/icons.h`        | Weather icons drawn with TFT primitives                  |
| `src/main.cpp`       | WiFi/NTP setup, 5-minute refresh loop                    |

## Notes

- TLS certificate verification is skipped (`setInsecure()`) for simplicity. To
  pin Buienradar's root CA instead, replace `setInsecure()` in
  `src/buienradar.cpp` with `setCACert(...)`.
- On a failed refresh the last-good data stays on screen with a `!` warning next
  to the timestamp, rather than blanking.
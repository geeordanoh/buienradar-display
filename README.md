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

Credentials and location are compiled in **at flash time** from environment
variables — nothing secret is committed.

### 1. Prerequisites

- **Python 3** and **PlatformIO Core**: `pip install platformio` (gives you the
  `pio` command). Or use the PlatformIO IDE extension for VS Code.
- A **USB data cable** (charge-only cables won't enumerate a serial port).

### 2. Install the USB-serial driver

The ESP32-2432S028R talks to your computer over a **CH340** USB-serial chip:

- **Linux** — built into the kernel, nothing to install.
- **macOS / older Windows** — install the [WCH CH340 driver](https://www.wch-ic.com/downloads/CH341SER_ZIP.html).

The board has **both a micro-USB and a USB-C** port. Use whichever is wired to
the CH340 — on most units either works; if the port isn't detected, try the
other connector.

### 3. Set your WiFi + location

Copy the example, fill it in, and source it:

```bash
cp secrets.ini.example secrets.sh   # then edit secrets.sh
source secrets.sh
```

Or pass the variables inline on the flash command (see step 4). Variables:

| Variable        | Meaning                                   |
| --------------- | ----------------------------------------- |
| `WIFI_SSID`     | WiFi network name                         |
| `WIFI_PASS`     | WiFi password                             |
| `LATITUDE`      | Location latitude (decimal degrees)       |
| `LONGITUDE`     | Location longitude (decimal degrees)      |
| `LOCATION_NAME` | Short label shown in the header           |

The project still **compiles** with these unset (falls back to empty WiFi +
Amsterdam coordinates), but it won't connect until real credentials are flashed.

### 4. Flash the firmware

Plug the board in, then build + upload:

```bash
# after `source secrets.sh`
pio run -e cyd -t upload
```

…or all in one line without a secrets file:

```bash
WIFI_SSID="MyNetwork" WIFI_PASS="hunter2" \
LATITUDE=52.0907 LONGITUDE=5.1214 LOCATION_NAME="Utrecht" \
pio run -e cyd -t upload
```

PlatformIO auto-detects the serial port. If you have several serial devices
connected, name it explicitly:

```bash
pio run -e cyd -t upload --upload-port /dev/ttyUSB0     # Linux
pio run -e cyd -t upload --upload-port COM5             # Windows
pio run -e cyd -t upload --upload-port /dev/cu.usbserial-1420  # macOS
```

The ESP32 is normally reset into its bootloader automatically (via DTR/RTS). If
you see `Failed to connect`, do the manual dance: **hold BOOT**, tap **RST** (or
start the upload), then **release BOOT** once `Connecting…` appears.

### 5. Verify

```bash
pio device monitor -b 115200
```

You should see the boot screen → "Connecting to WiFi" → live weather within a
few seconds, and the display then redraws every 5 minutes.

### Troubleshooting

- **Port not found / not listed** — wrong cable (must be data), missing CH340
  driver (step 2), or try the other USB connector.
- **Permission denied on Linux** — add yourself to the serial group:
  `sudo usermod -aG dialout $USER`, then log out/in (or run the command with
  `sudo` once).
- **`Failed to connect to ESP32`** — use the BOOT/RST fallback in step 4.
- **Blank screen or wrong colours** — your unit may use a different panel; swap
  the driver flag in `platformio.ini` (`-DILI9341_2_DRIVER=1` →
  `-DILI9341_DRIVER=1` or `-DST7789_DRIVER=1`) and re-flash.

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
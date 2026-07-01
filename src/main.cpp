#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "buienradar.h"
#include "config.h"
#include "display.h"

namespace {

// Last successfully fetched data (kept so a failed refresh shows stale data
// instead of a blank screen).
buienradar::CurrentWeather g_current;
buienradar::Forecast g_forecast;
buienradar::Nowcast g_nowcast;

char g_updated[6] = "--:--";
bool g_haveData = false;
uint32_t g_lastFetchMs = 0;
bool g_firstRun = true;

bool wifiConnect(uint32_t timeoutMs) {
  if (WiFi.status() == WL_CONNECTED) return true;
  if (strlen(config::wifiSsid()) == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(config::wifiSsid(), config::wifiPass());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

void updateClockString() {
  struct tm t;
  if (getLocalTime(&t, 200)) {
    strftime(g_updated, sizeof(g_updated), "%H:%M", &t);
  }
}

// Fetch both endpoints; update globals on success. Returns true if fresh data
// was obtained this cycle.
bool refresh() {
  if (!wifiConnect(15000)) {
    Serial.println("[main] WiFi not connected");
    return false;
  }

  buienradar::Nowcast now;
  buienradar::CurrentWeather cur;
  buienradar::Forecast fc;

  bool okNow = buienradar::fetchNowcast(config::latitude(), config::longitude(), now);
  bool okWx = buienradar::fetchWeather(config::latitude(), config::longitude(), cur, fc);

  if (okNow) g_nowcast = now;
  if (okWx) {
    g_current = cur;
    g_forecast = fc;
  }

  bool any = okNow || okWx;
  if (any) {
    g_haveData = true;
    updateClockString();
  }
  Serial.printf("[main] refresh nowcast=%d weather=%d\n", okNow, okWx);
  return any;
}

void doUpdateAndRender() {
  bool fresh = refresh();
  bool stale = g_haveData && !fresh;
  display::render(g_current, g_forecast, g_nowcast, config::locationName(),
                  g_updated, stale);
  g_lastFetchMs = millis();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[main] Buienradar display starting");

  display::init();
  display::showBoot("Verbinden met WiFi...", config::wifiSsid());

  if (wifiConnect(20000)) {
    Serial.printf("[main] WiFi connected: %s\n", WiFi.localIP().toString().c_str());
    configTzTime(config::TZ_INFO, "pool.ntp.org", "time.google.com");
    display::showBoot("Weergegevens ophalen...");
  } else {
    display::showBoot("WiFi mislukt", "controleer credentials");
    delay(2000);
  }

  doUpdateAndRender();
  g_firstRun = false;
}

void loop() {
  if (!g_firstRun &&
      (uint32_t)(millis() - g_lastFetchMs) >= config::REFRESH_INTERVAL_MS) {
    doUpdateAndRender();
  }
  delay(500);
}

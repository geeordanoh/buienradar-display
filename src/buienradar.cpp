#include "buienradar.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <math.h>

namespace buienradar {

namespace {

constexpr const char* NOWCAST_URL =
    "https://gpsgadget.buienradar.nl/data/raintext";
constexpr const char* FEED_URL = "https://data.buienradar.nl/2.0/feed/json";
constexpr uint16_t HTTP_TIMEOUT_MS = 12000;

// Lowercase, accent-free contains helper for Dutch descriptions.
bool has(const String& haystack, const char* needle) {
  return haystack.indexOf(needle) >= 0;
}

// English weekday abbreviation from a "YYYY-MM-DDT..." date string via Zeller.
void weekdayFromDate(const String& date, char out[4]) {
  strcpy(out, "--");
  if (date.length() < 10) return;
  int y = date.substring(0, 4).toInt();
  int m = date.substring(5, 7).toInt();
  int d = date.substring(8, 10).toInt();
  if (y == 0 || m == 0 || d == 0) return;
  if (m < 3) {  // Zeller: Jan/Feb counted as months 13/14 of prev year
    m += 12;
    y -= 1;
  }
  int K = y % 100;
  int J = y / 100;
  int h = (d + (13 * (m + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
  // h: 0=Sat,1=Sun,2=Mon,...6=Fri
  static const char* names[7] = {"Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri"};
  strncpy(out, names[h], 3);
  out[3] = '\0';
}

}  // namespace

Condition conditionFromDescription(const String& raw) {
  String d = raw;
  d.toLowerCase();
  if (d.isEmpty()) return Condition::Unknown;
  if (has(d, "onweer")) return Condition::Thunder;
  if (has(d, "sneeuw") || has(d, "hagel") || has(d, "winters"))
    return Condition::Snow;
  if (has(d, "zware regen") || has(d, "hevige") || has(d, "zware buien"))
    return Condition::HeavyRain;
  if (has(d, "regen") || has(d, "buien") || has(d, "motregen"))
    return Condition::Rain;
  if (has(d, "mist") || has(d, "nevel")) return Condition::Fog;
  if (has(d, "half") || has(d, "afwisselend") || has(d, "wolkenvelden"))
    return Condition::Partly;
  if (has(d, "bewolkt") || has(d, "betrokken")) return Condition::Cloudy;
  if (has(d, "zonnig") || has(d, "onbewolkt") || has(d, "helder"))
    return Condition::Clear;
  return Condition::Unknown;
}

const char* conditionLabel(Condition c) {
  switch (c) {
    case Condition::Clear:     return "Clear";
    case Condition::Partly:    return "Partly cloudy";
    case Condition::Cloudy:    return "Cloudy";
    case Condition::Fog:       return "Fog";
    case Condition::Rain:      return "Rain";
    case Condition::HeavyRain: return "Heavy rain";
    case Condition::Snow:      return "Snow";
    case Condition::Thunder:   return "Thunderstorm";
    case Condition::Unknown:
    default:                   return "--";
  }
}

float rainValueToMmPerHour(int value) {
  if (value <= 0) return 0.0f;
  return powf(10.0f, (value - 109) / 32.0f);
}

float Nowcast::maxMmPerHour() const {
  float m = 0.0f;
  for (size_t i = 0; i < count; i++)
    if (slots[i].mmPerHour > m) m = slots[i].mmPerHour;
  return m;
}

bool Nowcast::anyRain() const {
  for (size_t i = 0; i < count; i++)
    if (slots[i].value > 0) return true;
  return false;
}

bool fetchNowcast(float lat, float lon, Nowcast& out) {
  out.count = 0;
  out.valid = false;

  WiFiClientSecure client;
  client.setInsecure();  // hobby device; see README for pinning a root CA
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);

  String url = String(NOWCAST_URL) + "?lat=" + String(lat, 4) +
               "&lon=" + String(lon, 4);
  if (!http.begin(client, url)) return false;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  int start = 0;
  while (start < (int)body.length() && out.count < MAX_RAIN_SLOTS) {
    int nl = body.indexOf('\n', start);
    String line = (nl < 0) ? body.substring(start) : body.substring(start, nl);
    line.trim();
    int bar = line.indexOf('|');
    if (bar > 0) {
      int value = line.substring(0, bar).toInt();
      String t = line.substring(bar + 1);
      t.trim();
      RainSlot& s = out.slots[out.count];
      s.value = (uint8_t)constrain(value, 0, 255);
      s.mmPerHour = rainValueToMmPerHour(value);
      strncpy(s.time, t.c_str(), sizeof(s.time) - 1);
      s.time[sizeof(s.time) - 1] = '\0';
      out.count++;
    }
    if (nl < 0) break;
    start = nl + 1;
  }

  out.valid = out.count > 0;
  return out.valid;
}

bool fetchWeather(float lat, float lon, CurrentWeather& current,
                  Forecast& forecast) {
  current = CurrentWeather{};
  forecast = Forecast{};

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(client, FEED_URL)) return false;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  // The feed is large; a filter keeps only the fields we need so it fits RAM.
  // For an array, element [0] is the template applied to every member.
  JsonDocument filter;
  JsonObject stf = filter["actual"]["stationmeasurements"][0].to<JsonObject>();
  stf["lat"] = true;
  stf["lon"] = true;
  stf["temperature"] = true;
  stf["feeltemperature"] = true;
  stf["humidity"] = true;
  stf["windspeed"] = true;
  stf["winddirection"] = true;
  stf["airpressure"] = true;
  stf["weatherdescription"] = true;

  JsonObject fcf = filter["forecast"]["fivedayforecast"][0].to<JsonObject>();
  fcf["day"] = true;
  fcf["mintemperature"] = true;
  fcf["maxtemperature"] = true;
  fcf["rainChance"] = true;
  fcf["sunChance"] = true;
  fcf["weatherdescription"] = true;

  JsonDocument doc;
  DeserializationError err =
      deserializeJson(doc, http.getStream(),
                      DeserializationOption::Filter(filter),
                      DeserializationOption::NestingLimit(12));
  http.end();
  if (err) {
    Serial.printf("[buienradar] feed parse error: %s\n", err.c_str());
    return false;
  }

  // ---- nearest station -> current weather ----
  JsonArray stations = doc["actual"]["stationmeasurements"].as<JsonArray>();
  float bestDist = 1e12f;
  JsonObject best;
  for (JsonObject st : stations) {
    if (st["temperature"].isNull()) continue;
    float slat = st["lat"] | 1000.0f;
    float slon = st["lon"] | 1000.0f;
    if (slat > 900.0f || slon > 900.0f) continue;
    float dlat = slat - lat;
    float dlon = (slon - lon) * cosf(lat * 0.01745329f);
    float dist = dlat * dlat + dlon * dlon;
    if (dist < bestDist) {
      bestDist = dist;
      best = st;
    }
  }
  if (!best.isNull()) {
    current.valid = true;
    current.temperature = best["temperature"] | 0.0f;
    current.feelTemperature = best["feeltemperature"] | current.temperature;
    current.humidity = best["humidity"] | 0;
    current.windSpeedMs = best["windspeed"] | 0.0f;
    current.windDirection = (const char*)(best["winddirection"] | "");
    current.airPressure = best["airpressure"] | 0.0f;
    current.description = (const char*)(best["weatherdescription"] | "");
    current.condition = conditionFromDescription(current.description);
  }

  // ---- 5-day forecast ----
  JsonArray days = doc["forecast"]["fivedayforecast"].as<JsonArray>();
  for (JsonObject day : days) {
    if (forecast.count >= MAX_FORECAST_DAYS) break;
    DayForecast& df = forecast.days[forecast.count];
    df.valid = true;
    weekdayFromDate((const char*)(day["day"] | ""), df.weekday);
    df.minTemp = day["mintemperature"].as<float>();
    df.maxTemp = day["maxtemperature"].as<float>();
    df.rainChance = day["rainChance"] | 0;
    df.sunChance = day["sunChance"] | 0;
    df.description = (const char*)(day["weatherdescription"] | "");
    df.condition = conditionFromDescription(df.description);
    forecast.count++;
  }
  forecast.valid = forecast.count > 0;

  return current.valid || forecast.valid;
}

}  // namespace buienradar

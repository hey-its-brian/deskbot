#include "WeatherClient.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

// Pull the number that follows `"key":` out of a JSON body. Enough for a
// fixed, well-behaved API; not a JSON parser.
bool findNumber(const char* body, const char* key, double& out) {
  const char* p = strstr(body, key);
  if (!p) return false;
  p += strlen(key);
  while (*p == ' ' || *p == ':') ++p;
  char* end = nullptr;
  out = strtod(p, &end);
  return end != p;
}

}  // namespace

void WeatherClient::begin() {
  if (lock_) return;
  lock_ = xSemaphoreCreateMutex();
  // TLS wants a generous stack; 12 KB is comfortable for HTTPClient+mbedTLS.
  xTaskCreate(taskEntry, "weather", 12 * 1024, this, 1, &task_);
}

void WeatherClient::configure(float lat, float lon, char unit, uint16_t intervalMin,
                              bool enabled) {
  const bool moved = (lat != lat_) || (lon != lon_) || (unit != unit_);
  lat_ = lat;
  lon_ = lon;
  unit_ = (unit == 'F') ? 'F' : 'C';
  if (intervalMin < 5) intervalMin = 5;
  intervalMs_ = (uint32_t)intervalMin * 60UL * 1000UL;
  const bool turnedOn = enabled && !enabled_;
  enabled_ = enabled;
  if (enabled_ && (moved || turnedOn)) requestNow();
}

void WeatherClient::requestNow() {
  if (!enabled_) return;
  wantFetch_ = true;
}

void WeatherClient::loop(uint32_t nowMs, bool netUp) {
  netUp_ = netUp;
  if (!enabled_ || !netUp) return;
  if (nextFetchMs_ == 0 || (int32_t)(nowMs - nextFetchMs_) >= 0) {
    wantFetch_ = true;
    nextFetchMs_ = nowMs + intervalMs_;
  }
}

bool WeatherClient::takeFresh(WeatherReport& out) {
  if (!fresh_ || !lock_) return false;
  xSemaphoreTake(lock_, portMAX_DELAY);
  out = report_;
  fresh_ = false;
  xSemaphoreGive(lock_);
  return true;
}

WeatherReport WeatherClient::current() {
  WeatherReport r;
  if (!lock_) return r;
  xSemaphoreTake(lock_, portMAX_DELAY);
  r = report_;
  xSemaphoreGive(lock_);
  return r;
}

void WeatherClient::taskEntry(void* self) {
  static_cast<WeatherClient*>(self)->task();
}

void WeatherClient::task() {
  for (;;) {
    if (wantFetch_ && netUp_) {
      wantFetch_ = false;
      busy_ = true;
      WeatherReport r;
      const bool ok = fetch(r);
      xSemaphoreTake(lock_, portMAX_DELAY);
      if (ok) {
        report_ = r;
      } else {
        report_.httpStatus = r.httpStatus;   // keep the old reading, note the failure
      }
      fresh_ = true;
      xSemaphoreGive(lock_);
      busy_ = false;
      // A failed fetch retries in two minutes rather than waiting a full interval.
      if (!ok) nextFetchMs_ = millis() + 120000UL;
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

bool WeatherClient::fetch(WeatherReport& r) {
  char url[224];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,weather_code,is_day&temperature_unit=%s",
           (double)lat_, (double)lon_, unit_ == 'F' ? "fahrenheit" : "celsius");

  WiFiClientSecure client;
  // Open-Meteo is public weather data; skipping certificate validation keeps
  // a CA bundle out of the firmware. Nothing sensitive crosses this link.
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(6000);
  http.setTimeout(8000);
  if (!http.begin(client, url)) {
    r.httpStatus = -1;
    return false;
  }
  const int status = http.GET();
  r.httpStatus = status;
  bool ok = false;
  if (status == 200) {
    String body = http.getString();
    const char* cur = strstr(body.c_str(), "\"current\"");
    double temp = 0, code = -1, day = 1;
    if (cur && findNumber(cur, "\"temperature_2m\"", temp) &&
        findNumber(cur, "\"weather_code\"", code)) {
      findNumber(cur, "\"is_day\"", day);
      r.valid = true;
      r.temp = (int)lround(temp);
      r.unit = unit_;
      r.code = (int)code;
      r.kind = db::weatherFromWmo(r.code);
      r.isDay = day >= 0.5;
      r.fetchedMs = millis();
      ok = true;
    }
  }
  http.end();
  return ok;
}

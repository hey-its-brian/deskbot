// ---------------------------------------------------------------------------
//  WeatherClient.h - fetches current conditions from Open-Meteo on its own
//  FreeRTOS task, so the TLS handshake (~1 s on a C3) never stalls the face.
//  The main loop only ever sees a finished report.
// ---------------------------------------------------------------------------
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "Weather.h"

struct WeatherReport {
  bool valid = false;
  db::WeatherKind kind = db::WX_UNKNOWN;
  bool isDay = true;
  int temp = 0;
  char unit = 'C';
  int code = -1;              // raw WMO code
  int httpStatus = 0;         // last HTTP status, or a negative client error
  uint32_t fetchedMs = 0;     // millis() when this report arrived
};

class WeatherClient {
 public:
  void begin();
  void configure(float lat, float lon, char unit, uint16_t intervalMin, bool enabled);

  void requestNow();                       // fetch as soon as the network allows
  void loop(uint32_t nowMs, bool netUp);   // schedules the periodic fetch

  bool takeFresh(WeatherReport& out);      // true once for each new report
  WeatherReport current();
  bool busy() const { return busy_; }
  bool enabled() const { return enabled_; }

 private:
  static void taskEntry(void* self);
  void task();
  bool fetch(WeatherReport& r);

  SemaphoreHandle_t lock_ = nullptr;
  TaskHandle_t task_ = nullptr;
  volatile bool wantFetch_ = false;
  volatile bool busy_ = false;
  volatile bool fresh_ = false;
  volatile bool netUp_ = false;

  WeatherReport report_;
  float lat_ = 0.0f, lon_ = 0.0f;
  char unit_ = 'C';
  uint32_t intervalMs_ = 15UL * 60UL * 1000UL;
  uint32_t nextFetchMs_ = 0;
  bool enabled_ = false;
};

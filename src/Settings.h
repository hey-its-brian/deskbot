// ---------------------------------------------------------------------------
//  Settings.h - the knobs that can change at runtime, kept in flash (NVS) so
//  they survive a reboot. Everything here has a compile-time default in
//  config.h; the web app and the `set` serial command edit these.
// ---------------------------------------------------------------------------
#pragma once

#include <stdint.h>

#include "config.h"

struct Settings {
  float boredS = DB_IDLE_BORED_S;
  float sleepS = DB_IDLE_SLEEP_S;
  uint8_t brightness = 255;             // panel contrast
  bool drift = DB_BURN_IN_DRIFT ? true : false;
  bool wxEnabled = DB_WEATHER_ENABLED ? true : false;
  float lat = DB_WEATHER_LAT;
  float lon = DB_WEATHER_LON;
  uint16_t wxIntervalMin = DB_WEATHER_INTERVAL_MIN;
  uint16_t wxGlanceMin = DB_WEATHER_GLANCE_MIN;
  char units = DB_WEATHER_UNITS;

  // Pull every field back into a sane range - inputs come from the network.
  void clampAll();
};

// Flash-backed store (NVS).
namespace nvs {
void load(Settings& s);
void save(const Settings& s);
}

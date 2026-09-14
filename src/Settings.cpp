#include "Settings.h"

#include <Preferences.h>

void Settings::clampAll() {
  if (!(boredS >= 5.0f)) boredS = 5.0f;          // also catches NaN
  if (boredS > 86400.0f) boredS = 86400.0f;
  if (!(sleepS >= boredS)) sleepS = boredS;
  if (sleepS > 86400.0f) sleepS = 86400.0f;
  if (!(lat >= -90.0f)) lat = -90.0f;
  if (lat > 90.0f) lat = 90.0f;
  if (!(lon >= -180.0f)) lon = -180.0f;
  if (lon > 180.0f) lon = 180.0f;
  if (wxIntervalMin < 5) wxIntervalMin = 5;
  if (wxIntervalMin > 1440) wxIntervalMin = 1440;
  if (wxGlanceMin > 1440) wxGlanceMin = 1440;
  if (units != 'C' && units != 'F') units = 'F';
}

namespace nvs {

static const char* kNamespace = "buddy";

void load(Settings& s) {
  Preferences p;
  if (!p.begin(kNamespace, true)) return;  // nothing stored yet: keep defaults
  s.boredS = p.getFloat("bored", s.boredS);
  s.sleepS = p.getFloat("sleep", s.sleepS);
  s.brightness = p.getUChar("bright", s.brightness);
  s.drift = p.getBool("drift", s.drift);
  s.wxEnabled = p.getBool("wxon", s.wxEnabled);
  s.lat = p.getFloat("lat", s.lat);
  s.lon = p.getFloat("lon", s.lon);
  s.wxIntervalMin = p.getUShort("wxint", s.wxIntervalMin);
  s.wxGlanceMin = p.getUShort("wxgl", s.wxGlanceMin);
  s.units = (char)p.getChar("units", s.units);
  p.end();
  s.clampAll();
}

void save(const Settings& s) {
  Preferences p;
  if (!p.begin(kNamespace, false)) return;
  p.putFloat("bored", s.boredS);
  p.putFloat("sleep", s.sleepS);
  p.putUChar("bright", s.brightness);
  p.putBool("drift", s.drift);
  p.putBool("wxon", s.wxEnabled);
  p.putFloat("lat", s.lat);
  p.putFloat("lon", s.lon);
  p.putUShort("wxint", s.wxIntervalMin);
  p.putUShort("wxgl", s.wxGlanceMin);
  p.putChar("units", (int8_t)s.units);
  p.end();
}

}  // namespace nvs

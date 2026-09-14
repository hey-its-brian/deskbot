#include "Weather.h"

namespace db {
namespace {

const char* const kNames[WX_COUNT] = {"unknown", "clear",  "partly", "cloudy",
                                      "fog",     "rain",   "snow",   "storm"};

bool ciEqual(const char* a, const char* b) {
  while (*a && *b) {
    char ca = *a, cb = *b;
    if (ca >= 'A' && ca <= 'Z') ca = (char)(ca + 32);
    if (cb >= 'A' && cb <= 'Z') cb = (char)(cb + 32);
    if (ca != cb) return false;
    ++a; ++b;
  }
  return *a == 0 && *b == 0;
}

}  // namespace

const char* weatherName(WeatherKind k) {
  return (k < WX_COUNT) ? kNames[k] : "?";
}

bool weatherFromName(const char* name, WeatherKind& out) {
  for (uint8_t i = 0; i < WX_COUNT; ++i) {
    if (ciEqual(name, kNames[i])) { out = (WeatherKind)i; return true; }
  }
  return false;
}

WeatherKind weatherFromWmo(int code) {
  if (code == 0) return WX_CLEAR;
  if (code >= 1 && code <= 2) return WX_PARTLY;
  if (code == 3) return WX_CLOUDY;
  if (code == 45 || code == 48) return WX_FOG;
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return WX_RAIN;
  if ((code >= 71 && code <= 77) || code == 85 || code == 86) return WX_SNOW;
  if (code >= 95 && code <= 99) return WX_STORM;
  return WX_UNKNOWN;
}

}  // namespace db

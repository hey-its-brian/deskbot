// ---------------------------------------------------------------------------
//  Weather.h - the handful of conditions the face knows how to react to,
//  and the mapping from WMO weather codes (what Open-Meteo returns) onto
//  them. Arduino-free: the host preview renders weather faces too.
// ---------------------------------------------------------------------------
#pragma once

#include <stdint.h>

namespace db {

enum WeatherKind : uint8_t {
  WX_UNKNOWN = 0,
  WX_CLEAR,     // sun by day, moon and stars by night
  WX_PARTLY,    // sun with a cloud
  WX_CLOUDY,    // overcast: clouds drift across the top
  WX_FOG,       // peering through drifting streaks
  WX_RAIN,      // streaks falling, a bit glum
  WX_SNOW,      // slow flakes, watches them with interest
  WX_STORM,     // rain plus lightning; each bolt makes it jump
  WX_COUNT
};

const char* weatherName(WeatherKind k);
bool weatherFromName(const char* name, WeatherKind& out);

// WMO 4677 codes, as used by Open-Meteo's `weather_code`.
WeatherKind weatherFromWmo(int code);

}  // namespace db

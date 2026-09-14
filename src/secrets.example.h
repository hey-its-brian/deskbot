// Copy to src/secrets.h (gitignored) and fill in. Anything left out falls
// back to the defaults in config.h.
#pragma once

#define DB_WIFI_SSID     "your-network"
#define DB_WIFI_PASS     "your-password"

// Strongly recommended once OTA is on: anyone on the LAN could otherwise
// push firmware to it. Same password goes in platformio.ini's upload_flags.
#define DB_OTA_PASSWORD  ""

// Leave both empty for no login on the web app (fine on a home LAN).
#define DB_WEB_USER      ""
#define DB_WEB_PASSWORD  ""

// Starting location for the weather; changeable later from the web app.
#define DB_WEATHER_LAT   0.0f
#define DB_WEATHER_LON   0.0f

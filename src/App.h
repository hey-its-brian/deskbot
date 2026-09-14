// ---------------------------------------------------------------------------
//  App.h - the handful of objects the firmware modules share. Defined in
//  main.cpp; the web app and the serial console talk to the buddy through
//  exactly the same functions.
// ---------------------------------------------------------------------------
#pragma once

#include <Print.h>

#include "Face.h"
#include "Personality.h"
#include "Settings.h"
#include "WeatherClient.h"

extern db::Face face;
extern db::Personality brain;
extern Settings settings;
extern WeatherClient weather;

// One command vocabulary for serial and the web: see docs/SERIAL.md.
void runCommand(char* line, Print& out);

// Push `settings` into the face, the personality, the panel and the weather
// client, and write it to flash.
void applySettings();
void saveSettings();

// Show the current weather now (if there is any), for `seconds`.
bool showWeatherNow(float seconds);

uint32_t uptimeSeconds();

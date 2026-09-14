// ---------------------------------------------------------------------------
//  Gfx.h - one drawing surface, two backends.
//
//  On the ESP32 `Canvas` is Adafruit_GFX (Adafruit_SSD1306 derives from it).
//  On the host it is a stub that rasterises into a plain 128x64 bitmap so the
//  face can be rendered to PNG without hardware. The face renderer only ever
//  uses the handful of primitives both backends implement.
// ---------------------------------------------------------------------------
#pragma once

#ifdef DESKBUDDY_HOST
#include "HostGfx.h"
typedef db::HostGfx Canvas;
#else
#include <Adafruit_GFX.h>
typedef Adafruit_GFX Canvas;
#endif

// Matches SSD1306_BLACK / SSD1306_WHITE.
#define DB_BLACK 0
#define DB_WHITE 1

#define DB_SCREEN_W 128
#define DB_SCREEN_H 64

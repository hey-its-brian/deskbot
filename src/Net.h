// ---------------------------------------------------------------------------
//  Net.h - WiFi, mDNS and over-the-air updates. Everything is non-blocking;
//  the face never waits for the network.
// ---------------------------------------------------------------------------
#pragma once

#include <Arduino.h>

namespace net {

typedef void (*OtaProgressFn)(unsigned percent);

void begin();                       // no-op without credentials
void loop();                        // call every frame

bool enabled();                     // credentials present
bool up();                          // associated and has an address
bool everUp();                      // has connected at least once this boot
const char* hostname();
String ip();
int rssi();

// Called with 0..100 while an OTA image is being received, so the panel can
// show something better than a frozen face.
void setOtaProgress(OtaProgressFn fn);

// Why the last connection attempt failed, in words ("" if it hasn't).
const char* lastFailure();

// Blocking scan (~2-3 s): prints every network in range to `out`, so a
// "connecting..." that never finishes can be diagnosed from the console.
void scan(Print& out);

}  // namespace net

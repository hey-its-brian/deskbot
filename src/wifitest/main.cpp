// ---------------------------------------------------------------------------
//  wifitest - the smallest possible WiFi sketch, for when the buddy won't
//  connect and you need to know whether it is the network or the firmware.
//
//    pio run -e wifitest -t upload && pio device monitor
//
//  It uses the same src/secrets.h, scans, then tries to connect with the
//  ESP-IDF's own WiFi logging turned all the way up (CORE_DEBUG_LEVEL=5), so
//  the console shows the raw state machine: auth -> assoc -> handshake, and
//  the exact reason if any step is refused. Nothing else runs.
// ---------------------------------------------------------------------------
#include <Arduino.h>
#include <WiFi.h>

#include "../config.h"

static const char* authText(wifi_auth_mode_t m) {
  switch (m) {
    case WIFI_AUTH_OPEN:            return "open";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-enterprise";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
    default:                        return "?";
  }
}

void setup() {
  Serial.begin(115200);
  delay(2500);  // give the USB console time to attach
  Serial.println();
  Serial.println(F("=== wifitest ==="));
  Serial.print(F("ssid: \"")); Serial.print(DB_WIFI_SSID); Serial.println('"');
  Serial.print(F("pass length: ")); Serial.println((int)strlen(DB_WIFI_PASS));

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(300);

  Serial.println(F("scanning (2.4 GHz only - the C3 has no 5 GHz)..."));
  const int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    Serial.println(F("  nothing seen at all"));
  }
  bool seen = false;
  for (int i = 0; i < n; ++i) {
    const String ssid = WiFi.SSID(i);
    Serial.print(F("  "));
    Serial.print(ssid.length() ? ssid : String("<hidden>"));
    Serial.print(F("  ch")); Serial.print(WiFi.channel(i));
    Serial.print(F("  ")); Serial.print(WiFi.RSSI(i)); Serial.print(F(" dBm  "));
    Serial.print(authText(WiFi.encryptionType(i)));
    if (ssid == DB_WIFI_SSID) { seen = true; Serial.print(F("   <- configured")); }
    Serial.println();
  }
  WiFi.scanDelete();
  if (!seen) {
    Serial.println(F("configured SSID NOT in the scan: typo, hidden, 5 GHz-only, or out of range"));
  }

  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    Serial.print(F("event: disconnected, reason "));
    Serial.println((int)info.wifi_sta_disconnected.reason);
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  Serial.println(F("connecting..."));
  WiFi.begin(DB_WIFI_SSID, DB_WIFI_PASS);
}

void loop() {
  static uint32_t last = 0;
  static uint32_t started = millis();
  if (millis() - last < 2000) return;
  last = millis();
  const wl_status_t st = WiFi.status();
  Serial.print((millis() - started) / 1000); Serial.print(F("s  status=")); Serial.print((int)st);
  if (st == WL_CONNECTED) {
    Serial.print(F("  CONNECTED  ")); Serial.print(WiFi.localIP().toString());
    Serial.print(F("  ")); Serial.print(WiFi.RSSI()); Serial.print(F(" dBm  ch")); Serial.print(WiFi.channel());
  }
  Serial.println();
}

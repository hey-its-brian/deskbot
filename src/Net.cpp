#include "Net.h"

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "config.h"

namespace net {
namespace {

bool enabled_ = false;
bool servicesUp_ = false;
bool everUp_ = false;
uint32_t lastPoll_ = 0;
OtaProgressFn otaProgress_ = nullptr;

// Filled in from the WiFi event task; read and reported from loop().
volatile uint8_t lastReason_ = 0;
volatile bool reasonPending_ = false;
uint8_t reportedReason_ = 0;
uint32_t lastReasonPrint_ = 0;

// wifi_err_reason_t, the ones that actually come up in practice.
const char* reasonText(uint8_t r) {
  switch (r) {
    case 2:   return "auth expired";
    case 4:   return "association expired";
    case 8:   return "left the AP";
    case 15:  return "4-way handshake timed out - wrong password, or very weak signal";
    case 200: return "beacon timeout - lost the AP, weak signal";
    case 201: return "no AP found - SSID typo, 5 GHz-only network, or out of range";
    case 202: return "auth failed - password rejected";
    case 203: return "association failed";
    case 204: return "handshake timed out - wrong password";
    case 205: return "connection failed";
    case 210: return "no AP with compatible security - WPA3-only network?";
    case 211: return "no AP in auth-mode threshold";
    case 212: return "no AP in RSSI threshold";
    default:  return "see wifi_err_reason_t";
  }
}

const char* authText(wifi_auth_mode_t m) {
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

void startServices() {
  ArduinoOTA.setHostname(DB_HOSTNAME);
  if (DB_OTA_PASSWORD[0]) ArduinoOTA.setPassword(DB_OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println(F("OTA: receiving firmware"));
    if (otaProgress_) otaProgress_(0);
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    static unsigned lastPct = 255;
    const unsigned pct = total ? (unsigned)((uint64_t)done * 100 / total) : 0;
    if (pct != lastPct) {
      lastPct = pct;
      if (otaProgress_) otaProgress_(pct);
    }
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("OTA: done, rebooting"));
    if (otaProgress_) otaProgress_(100);
  });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.print(F("OTA: error "));
    Serial.println((int)e);
  });
  ArduinoOTA.begin();  // also brings up mDNS for the hostname
  MDNS.addService("http", "tcp", DB_WEB_PORT);
  servicesUp_ = true;
}

}  // namespace

void begin() {
  enabled_ = DB_WIFI_SSID[0] != 0;
  if (!enabled_) {
    Serial.println(F("wifi: no credentials (src/secrets.h) - running offline"));
    return;
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(DB_HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(DB_WIFI_POWER_SAVE ? true : false);
  WiFi.onEvent(
      [](WiFiEvent_t, WiFiEventInfo_t info) {
        lastReason_ = info.wifi_sta_disconnected.reason;
        reasonPending_ = true;
      },
      ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(DB_WIFI_SSID, DB_WIFI_PASS);
#if DB_WIFI_TX_POWER_8_5DBM
  WiFi.setTxPower(WIFI_POWER_8_5dBm);   // has to follow begin()
#endif
  Serial.print(F("wifi: connecting to "));
  Serial.println(DB_WIFI_SSID);
}

const char* lastFailure() { return lastReason_ ? reasonText(lastReason_) : ""; }

void scan(Print& out) {
  if (!enabled_ && WiFi.getMode() == WIFI_MODE_NULL) WiFi.mode(WIFI_STA);
  out.println(F("scanning..."));
  const int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    out.println(F("no networks seen - antenna? try next to the access point"));
    return;
  }
  for (int i = 0; i < n; ++i) {
    const String ssid = WiFi.SSID(i);
    out.print(ssid.length() ? ssid : String("<hidden>"));
    out.print(F("  ch")); out.print(WiFi.channel(i));
    out.print(F("  ")); out.print(WiFi.RSSI(i)); out.print(F(" dBm  "));
    out.print(authText(WiFi.encryptionType(i)));
    if (ssid == DB_WIFI_SSID) out.print(F("   <- configured"));
    out.println();
  }
  WiFi.scanDelete();
}

void loop() {
  if (!enabled_) return;
  const uint32_t now = millis();
  if (now - lastPoll_ >= 500) {
    lastPoll_ = now;
    const bool connected = (WiFi.status() == WL_CONNECTED);
    static bool wasConnected = false;
    if (connected && !wasConnected) {
      everUp_ = true;
      Serial.print(F("wifi: up, http://"));
      Serial.print(DB_HOSTNAME);
      Serial.print(F(".local  ("));
      Serial.print(WiFi.localIP().toString());
      Serial.println(')');
      if (!servicesUp_) startServices();
    } else if (!connected && wasConnected) {
      Serial.println(F("wifi: lost, reconnecting"));
    }
    wasConnected = connected;

    // Say why it is not connecting: once per distinct reason, then a
    // reminder every 15 s so a stuck "connecting" is never silent.
    if (reasonPending_ && !connected) {
      const uint8_t r = lastReason_;
      if (r != reportedReason_ || now - lastReasonPrint_ > 15000) {
        reportedReason_ = r;
        lastReasonPrint_ = now;
        Serial.print(F("wifi: not connected, reason "));
        Serial.print(r);
        Serial.print(F(": "));
        Serial.println(reasonText(r));
      }
      reasonPending_ = false;
    }
  }
  if (servicesUp_) ArduinoOTA.handle();
}

bool enabled() { return enabled_; }
bool up() { return enabled_ && WiFi.status() == WL_CONNECTED; }
bool everUp() { return everUp_; }
const char* hostname() { return DB_HOSTNAME; }
String ip() { return up() ? WiFi.localIP().toString() : String(""); }
int rssi() { return up() ? WiFi.RSSI() : 0; }
void setOtaProgress(OtaProgressFn fn) { otaProgress_ = fn; }

}  // namespace net

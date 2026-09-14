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
  WiFi.begin(DB_WIFI_SSID, DB_WIFI_PASS);
  Serial.print(F("wifi: connecting to "));
  Serial.println(DB_WIFI_SSID);
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

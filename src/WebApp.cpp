#include "WebApp.h"

#include <WebServer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "App.h"
#include "Net.h"
#include "WebPage.h"
#include "config.h"

namespace web {
namespace {

WebServer server(DB_WEB_PORT);
bool started = false;

// Collects Print output into a String, so runCommand() can answer over HTTP
// exactly as it does over serial.
class StringPrint : public Print {
 public:
  size_t write(uint8_t c) override { s += (char)c; return 1; }
  String s;
};

// JSON string escaping for the few free-text values we send back.
String jsonEscape(const String& in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in[i];
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) { /* drop other control bytes */ }
        else out += c;
    }
  }
  return out;
}

bool authorised() {
  if (!DB_WEB_PASSWORD[0]) return true;
  if (server.authenticate(DB_WEB_USER, DB_WEB_PASSWORD)) return true;
  server.requestAuthentication();
  return false;
}

void sendJson(int code, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void handleRoot() {
  if (!authorised()) return;
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", kWebPage);
}

void handleStatus() {
  if (!authorised()) return;
  const WeatherReport w = weather.current();
  const uint32_t now = millis();
  char buf[640];
  snprintf(buf, sizeof(buf),
           "{\"version\":\"%s\",\"hostname\":\"%s\",\"ip\":\"%s\",\"rssi\":%d,"
           "\"uptime\":%lu,\"heap\":%lu,"
           "\"emotion\":\"%s\",\"auto\":%s,\"asleep\":%s,\"petting\":%s,\"idle\":%.0f,"
           "\"weather\":{\"enabled\":%s,\"valid\":%s,\"kind\":\"%s\",\"isDay\":%s,"
           "\"temp\":%d,\"unit\":\"%c\",\"code\":%d,\"age\":%lu,\"busy\":%s,\"status\":%d},"
           "\"settings\":{\"bored\":%.0f,\"sleep\":%.0f,\"brightness\":%u,\"drift\":%s,"
           "\"weather\":%s,\"lat\":%.4f,\"lon\":%.4f,\"units\":\"%c\",\"interval\":%u,"
           "\"glance\":%u}}",
           DB_VERSION, net::hostname(), net::ip().c_str(), net::rssi(),
           (unsigned long)uptimeSeconds(), (unsigned long)ESP.getFreeHeap(),
           db::emotionName(face.emotion()), brain.autoMood() ? "true" : "false",
           brain.asleep() ? "true" : "false", brain.petting() ? "true" : "false",
           (double)brain.idleSeconds(),
           weather.enabled() ? "true" : "false", w.valid ? "true" : "false",
           db::weatherName(w.kind), w.isDay ? "true" : "false", w.temp, w.unit, w.code,
           (unsigned long)(w.valid ? (now - w.fetchedMs) / 1000UL : 0UL),
           weather.busy() ? "true" : "false", w.httpStatus,
           (double)settings.boredS, (double)settings.sleepS, settings.brightness,
           settings.drift ? "true" : "false", settings.wxEnabled ? "true" : "false",
           (double)settings.lat, (double)settings.lon, settings.units,
           settings.wxIntervalMin, settings.wxGlanceMin);
  sendJson(200, String(buf));
}

void handleCmd() {
  if (!authorised()) return;
  String c = server.arg("c");
  if (c.length() == 0 || c.length() > 47) {
    sendJson(400, F("{\"ok\":false,\"error\":\"command must be 1-47 characters\"}"));
    return;
  }
  char line[48];
  strncpy(line, c.c_str(), sizeof(line) - 1);
  line[sizeof(line) - 1] = 0;
  StringPrint out;
  runCommand(line, out);
  sendJson(200, "{\"ok\":true,\"out\":\"" + jsonEscape(out.s) + "\"}");
}

void handleSettings() {
  if (!authorised()) return;
  Settings s = settings;
  if (server.hasArg("bored"))      s.boredS = server.arg("bored").toFloat();
  if (server.hasArg("sleep"))      s.sleepS = server.arg("sleep").toFloat();
  if (server.hasArg("brightness")) {
    long b = server.arg("brightness").toInt();
    if (b < 1) b = 1;
    if (b > 255) b = 255;
    s.brightness = (uint8_t)b;
  }
  if (server.hasArg("drift"))      s.drift = server.arg("drift") == "1";
  if (server.hasArg("weather"))    s.wxEnabled = server.arg("weather") == "1";
  if (server.hasArg("lat"))        s.lat = server.arg("lat").toFloat();
  if (server.hasArg("lon"))        s.lon = server.arg("lon").toFloat();
  if (server.hasArg("units"))      s.units = server.arg("units")[0];
  if (server.hasArg("interval"))   s.wxIntervalMin = (uint16_t)server.arg("interval").toInt();
  if (server.hasArg("glance"))     s.wxGlanceMin = (uint16_t)server.arg("glance").toInt();
  s.clampAll();
  settings = s;
  applySettings();
  saveSettings();
  sendJson(200, F("{\"ok\":true}"));
}

void handleNotify() {
  if (!authorised()) return;
  db::Emotion e;
  if (!db::emotionFromName(server.arg("emotion").c_str(), e)) {
    sendJson(400, F("{\"ok\":false,\"error\":\"unknown emotion\"}"));
    return;
  }
  float hold = server.hasArg("hold") ? server.arg("hold").toFloat() : 4.0f;
  if (!(hold >= 0.5f)) hold = 0.5f;
  if (hold > 120.0f) hold = 120.0f;
  brain.wake(false);
  face.flash(e, hold, face.emotion());
  brain.holdMood(hold + 0.5f);
  sendJson(200, F("{\"ok\":true}"));
}

void handleNotFound() {
  server.send(404, "text/plain", "not found");
}

}  // namespace

void begin() {
  if (started) return;
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/cmd", HTTP_POST, handleCmd);
  server.on("/api/settings", HTTP_POST, handleSettings);
  server.on("/api/settings", HTTP_GET, handleStatus);
  server.on("/api/notify", HTTP_POST, handleNotify);
  server.onNotFound(handleNotFound);
  server.begin();
  started = true;
}

void loop() {
  if (!started) {
    if (net::up()) begin();
    return;
  }
  server.handleClient();
}

}  // namespace web

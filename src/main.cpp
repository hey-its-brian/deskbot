// ---------------------------------------------------------------------------
//  Desk Buddy - an EMO-style animated face for an ESP32-C3 Super Mini and a
//  128x64 SSD1306 OLED.
//
//  Wiring, behaviour and tuning all live in config.h. The face itself is in
//  Face.cpp; what it decides to do is in Personality.cpp. Networking (WiFi,
//  OTA, the web app, weather) is optional and switched on by src/secrets.h.
// ---------------------------------------------------------------------------
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>
#include <string.h>

#include "App.h"
#include "Button.h"
#include "Face.h"
#include "Net.h"
#include "Personality.h"
#include "Settings.h"
#include "WeatherClient.h"
#include "WebApp.h"
#include "config.h"

// The library re-applies its own bus clock around every transaction, so the
// speed has to go in here - a Wire.setClock() in setup() would be overridden.
static Adafruit_SSD1306 display(DB_SCREEN_W, DB_SCREEN_H, &Wire, -1,
                                DB_I2C_CLOCK, DB_I2C_CLOCK);

// Shared with WebApp.cpp through App.h.
db::Face face;
db::Personality brain;
Settings settings;
WeatherClient weather;

static Button button(DB_PIN_BUTTON, DB_BUTTON_ACTIVE_LOW);
static Button touch(DB_PIN_TOUCH, !DB_TOUCH_ACTIVE_HIGH);

// `pet` / `tap` over serial pretend a finger is on the pad until this time.
// The deadline is only ever compared while the pretend touch is live: an
// unsigned millis() difference flips sign after 24.9 days of uptime, and a
// bare `until > now` test would hallucinate a finger for the next 24.9 days.
static uint32_t simTouchUntil = 0;
static bool simTouching = false;

static uint8_t oledAddress = 0x3C;
static bool displayReady = false;
static bool dimmed = false;

static uint32_t lastGlanceMs = 0;
static db::WeatherKind lastKind = db::WX_UNKNOWN;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------
static void led(bool on) {
#if DB_LED_ENABLED
  digitalWrite(DB_PIN_LED, (DB_LED_ACTIVE_LOW ? !on : on) ? HIGH : LOW);
#else
  (void)on;
#endif
}

uint32_t uptimeSeconds() { return millis() / 1000UL; }

// Most of these modules are 0x3C, a few are 0x3D. Just ask the bus.
static uint8_t probeAddress() {
#if DB_OLED_ADDRESS
  return DB_OLED_ADDRESS;
#else
  const uint8_t candidates[] = {0x3C, 0x3D};
  for (uint8_t i = 0; i < 2; ++i) {
    Wire.beginTransmission(candidates[i]);
    if (Wire.endTransmission() == 0) return candidates[i];
  }
  return 0;  // nothing home
#endif
}

static void applyBrightness() {
  if (!displayReady || dimmed) return;
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(settings.brightness);
}

static bool startDisplay() {
  const uint8_t addr = probeAddress();
  if (addr == 0) return false;
  oledAddress = addr;
  if (!display.begin(SSD1306_SWITCHCAPVCC, addr)) return false;
  display.setRotation(DB_FLIP_DISPLAY ? 2 : 0);
  display.clearDisplay();
  display.display();
  return true;
}

// A little "powering up" flourish: a line that stretches out, then the eyes
// open from it.
static void bootAnimation() {
  for (int i = 0; i <= 18; ++i) {
    const int w = 4 + (i * 76) / 18;
    display.clearDisplay();
    display.fillRoundRect(DB_SCREEN_W / 2 - w / 2, DB_SCREEN_H / 2 - 1, w, 3, 1,
                          SSD1306_WHITE);
    display.display();
    delay(14);
  }
  face.setEmotion(db::EMOTION_SLEEPY, true);
  face.flash(db::EMOTION_EXCITED, 1.4f, db::EMOTION_NEUTRAL);
}

// The face is frozen while an OTA image comes in, so draw a progress bar
// instead of leaving it staring.
static void otaProgress(unsigned pct) {
  if (!displayReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 16);
  display.print(F("updating..."));
  display.drawRoundRect(14, 34, 100, 10, 3, SSD1306_WHITE);
  display.fillRoundRect(16, 36, (int16_t)((96 * pct) / 100), 6, 2, SSD1306_WHITE);
  display.display();
}

// ---------------------------------------------------------------------------
//  Settings and weather glue
// ---------------------------------------------------------------------------
void applySettings() {
  settings.clampAll();
  brain.setIdleTimes(settings.boredS, settings.sleepS);
  face.setBurnInDrift(settings.drift);
  applyBrightness();
  weather.configure(settings.lat, settings.lon, settings.units, settings.wxIntervalMin,
                    settings.wxEnabled);
}

void saveSettings() { nvs::save(settings); }

bool showWeatherNow(float seconds) {
  const WeatherReport w = weather.current();
  if (!w.valid) return false;
  brain.wake(false);
  face.showWeather(w.kind, w.isDay, true, w.temp, w.unit, seconds);
  brain.holdMood(seconds + 0.5f);
  lastGlanceMs = millis();
  return true;
}

static void pollWeather() {
  const uint32_t now = millis();
  weather.loop(now, net::up());

  WeatherReport r;
  if (weather.takeFresh(r)) {
    if (r.valid) {
      Serial.print(F("weather: "));
      Serial.print(db::weatherName(r.kind));
      Serial.print(r.isDay ? F(" (day) ") : F(" (night) "));
      Serial.print(r.temp);
      Serial.print((char)0xB0);
      Serial.println(r.unit);
      // Conditions changing is worth a look; the very first reading too.
      if (r.kind != lastKind && !brain.asleep()) showWeatherNow(DB_WEATHER_SHOW_S);
      lastKind = r.kind;
    } else {
      Serial.print(F("weather: fetch failed, http "));
      Serial.println(r.httpStatus);
    }
  }

  // The unprompted glance, every so often.
  if (settings.wxEnabled && settings.wxGlanceMin > 0 && !brain.asleep() &&
      !brain.petting() && !face.weatherShowing() &&
      (now - lastGlanceMs) >= (uint32_t)settings.wxGlanceMin * 60000UL) {
    if (!showWeatherNow(DB_WEATHER_SHOW_S)) lastGlanceMs = now;  // nothing yet; try later
  }
}

// ---------------------------------------------------------------------------
//  Commands - shared by the serial console and the web app. docs/SERIAL.md.
// ---------------------------------------------------------------------------
static void printHelp(Print& out) {
  out.println(F("desk buddy commands:"));
  out.println(F("  <emotion>        neutral happy excited sad angry surprised"));
  out.println(F("                   sleepy love curious suspicious dizzy bored"));
  out.println(F("  list             list emotions"));
  out.println(F("  auto on|off      autonomous mood changes (default on)"));
  out.println(F("  blink | wink l|r"));
  out.println(F("  look <x> <y>     gaze direction, -1..1"));
  out.println(F("  jolt             startle shake"));
  out.println(F("  poke             same as pressing the button"));
  out.println(F("  tap | pet        same as touching / holding the touch pad"));
  out.println(F("  sleep | wake"));
  out.println(F("  weather          show the current weather now"));
  out.println(F("  weather refresh  fetch it again"));
  out.println(F("  set <key> <val>  bored sleep brightness drift weather lat lon"));
  out.println(F("                   units interval glance   (saved to flash)"));
  out.println(F("  net              wifi state"));
  out.println(F("  status"));
}

static bool setSetting(const char* key, const char* val, Print& out) {
  if (!key || !val) return false;
  const bool on = !strcmp(val, "on") || !strcmp(val, "1") || !strcmp(val, "true");
  if (!strcmp(key, "bored"))           settings.boredS = (float)atof(val);
  else if (!strcmp(key, "sleep"))      settings.sleepS = (float)atof(val);
  else if (!strcmp(key, "brightness")) {
    long b = atol(val);
    settings.brightness = (uint8_t)(b < 1 ? 1 : (b > 255 ? 255 : b));
  }
  else if (!strcmp(key, "drift"))      settings.drift = on;
  else if (!strcmp(key, "weather"))    settings.wxEnabled = on;
  else if (!strcmp(key, "lat"))        settings.lat = (float)atof(val);
  else if (!strcmp(key, "lon"))        settings.lon = (float)atof(val);
  else if (!strcmp(key, "units"))      settings.units = (val[0] == 'c' || val[0] == 'C') ? 'C' : 'F';
  else if (!strcmp(key, "interval"))   settings.wxIntervalMin = (uint16_t)atol(val);
  else if (!strcmp(key, "glance"))     settings.wxGlanceMin = (uint16_t)atol(val);
  else {
    out.println(F("? unknown setting"));
    return false;
  }
  applySettings();
  saveSettings();
  out.print(F("set ")); out.print(key); out.print(' '); out.println(val);
  return true;
}

void runCommand(char* line, Print& out) {
  while (*line == ' ') ++line;
  if (*line == 0) return;

  char* arg = strchr(line, ' ');
  if (arg) { *arg++ = 0; while (*arg == ' ') ++arg; }

  db::Emotion e;
  if (db::emotionFromName(line, e)) {
    brain.setAutoMood(false);
    brain.wake(false);
    face.setEmotion(e);
    out.print(F("emotion: ")); out.println(db::emotionName(e));
    out.println(F("(auto mood off - 'auto on' to hand control back)"));
  } else if (!strcmp(line, "help") || !strcmp(line, "?")) {
    printHelp(out);
  } else if (!strcmp(line, "list")) {
    for (uint8_t i = 0; i < db::EMOTION_COUNT; ++i) {
      out.print(db::emotionName((db::Emotion)i));
      out.print((i + 1 == db::EMOTION_COUNT) ? '\n' : ' ');
    }
  } else if (!strcmp(line, "auto")) {
    const bool on = arg && !strcmp(arg, "on");
    brain.setAutoMood(on);
    out.print(F("auto mood: ")); out.println(on ? F("on") : F("off"));
  } else if (!strcmp(line, "blink")) {
    face.blink();
  } else if (!strcmp(line, "wink")) {
    face.wink(!arg || *arg == 'l');
  } else if (!strcmp(line, "look")) {
    float x = 0, y = 0;
    if (arg) {
      x = (float)atof(arg);
      char* second = strchr(arg, ' ');
      if (second) y = (float)atof(second + 1);
    }
    face.look(x, y, 2.5f);
  } else if (!strcmp(line, "jolt")) {
    face.jolt(1.0f);
  } else if (!strcmp(line, "poke")) {
    brain.onInteraction(db::TOUCH_POKE);
  } else if (!strcmp(line, "tap")) {
    simTouchUntil = millis() + 120;
    simTouching = true;
  } else if (!strcmp(line, "pet")) {
    simTouchUntil = millis() + 3500;
    simTouching = true;
  } else if (!strcmp(line, "sleep")) {
    brain.sleep();
  } else if (!strcmp(line, "wake")) {
    brain.wake(false);
  } else if (!strcmp(line, "weather")) {
    if (arg && !strcmp(arg, "refresh")) {
      weather.requestNow();
      out.println(net::up() ? F("weather: fetching") : F("weather: no network"));
    } else if (!weather.enabled()) {
      out.println(F("weather: off ('set weather on', and a lat/lon)"));
    } else if (!showWeatherNow(DB_WEATHER_SHOW_S)) {
      out.println(F("weather: no reading yet"));
    }
  } else if (!strcmp(line, "set")) {
    char* val = arg ? strchr(arg, ' ') : nullptr;
    if (val) { *val++ = 0; while (*val == ' ') ++val; }
    if (!arg || !val || !*val) out.println(F("usage: set <key> <value>"));
    else setSetting(arg, val, out);
  } else if (!strcmp(line, "net")) {
    if (!net::enabled()) out.println(F("wifi: off (no src/secrets.h)"));
    else if (!net::up()) out.println(F("wifi: connecting"));
    else {
      out.print(F("wifi: http://")); out.print(net::hostname()); out.print(F(".local  "));
      out.print(net::ip()); out.print(F("  ")); out.print(net::rssi()); out.println(F(" dBm"));
    }
  } else if (!strcmp(line, "drift")) {
    settings.drift = !arg || strcmp(arg, "off") != 0;
    applySettings();
    saveSettings();
  } else if (!strcmp(line, "status")) {
    const WeatherReport w = weather.current();
    out.print(F("v" DB_VERSION " emotion=")); out.print(db::emotionName(face.emotion()));
    out.print(F(" auto=")); out.print(brain.autoMood() ? 1 : 0);
    out.print(F(" asleep=")); out.print(brain.asleep() ? 1 : 0);
    out.print(F(" petting=")); out.print(brain.petting() ? 1 : 0);
    out.print(F(" idle=")); out.print(brain.idleSeconds(), 1);
    out.print(F("s oled=0x")); out.print(oledAddress, HEX);
    out.print(F(" ip=")); out.print(net::up() ? net::ip() : String("-"));
    out.print(F(" weather="));
    if (w.valid) { out.print(db::weatherName(w.kind)); out.print(' '); out.print(w.temp); out.print(w.unit); }
    else out.print('-');
    out.println();
  } else {
    // Echo only printable ASCII: this goes straight back into whatever
    // terminal is attached, and a stray escape sequence could redraw it.
    out.print(F("? "));
    for (const char* c = line; *c; ++c) {
      if (*c >= 0x20 && *c < 0x7F) out.print(*c);
    }
    out.println(F("  (try 'help')"));
  }
}

#if DB_SERIAL_CONTROL
static char cmdBuf[48];
static uint8_t cmdLen = 0;

static void pollSerial() {
  // A host that floods the port must not be able to stall the frame loop.
  uint8_t budget = 64;
  while (Serial.available() && budget--) {
    const char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      cmdBuf[cmdLen] = 0;
      runCommand(cmdBuf, Serial);
      cmdLen = 0;
    } else if (cmdLen < sizeof(cmdBuf) - 1) {
      cmdBuf[cmdLen++] = c;
    }
  }
}
#endif  // DB_SERIAL_CONTROL

// ---------------------------------------------------------------------------
//  Arduino entry points
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

#if DB_LED_ENABLED
  pinMode(DB_PIN_LED, OUTPUT);
#endif
  led(false);

  button.begin();
  touch.begin();

  Wire.begin(DB_PIN_SDA, DB_PIN_SCL);
  Wire.setClock(DB_I2C_CLOCK);

  // Seed from the chip ID plus boot timing, so two buddies on a desk don't
  // blink in lockstep.
  const uint32_t seed =
      (uint32_t)micros() ^ (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFFull);
  face.begin(seed);
  brain.begin(face, seed);

  nvs::load(settings);
  weather.begin();
  applySettings();

  displayReady = startDisplay();
  if (displayReady) {
    Serial.print(F("SSD1306 found at 0x"));
    Serial.println(oledAddress, HEX);
    applyBrightness();
    bootAnimation();
  }

  net::setOtaProgress(otaProgress);
  net::begin();
}

void loop() {
  // Keep retrying rather than bricking the sketch - handy while you are still
  // poking at the wiring.
  if (!displayReady) {
    static uint32_t lastTry = 0;
    if (millis() - lastTry > 1500) {
      lastTry = millis();
      Serial.print(F("no SSD1306 on SDA="));
      Serial.print(DB_PIN_SDA);
      Serial.print(F(" SCL="));
      Serial.print(DB_PIN_SCL);
      Serial.println(F(" - check wiring/address"));
      led(true);
      displayReady = startDisplay();
      led(false);
      if (displayReady) { applyBrightness(); bootAnimation(); }
    }
    net::loop();
    delay(10);
    return;
  }

  // Ask the panel if it is still there now and then, so an unplugged or
  // glitched OLED goes back through the retry path instead of being fed
  // frames forever.
  static uint32_t lastProbe = 0;
  if (millis() - lastProbe > 2000) {
    lastProbe = millis();
    Wire.beginTransmission(oledAddress);
    if (Wire.endTransmission() != 0) {
      displayReady = false;
      Serial.println(F("lost the SSD1306 - will retry"));
      return;
    }
  }

  static uint32_t prevUs = micros();
  const uint32_t nowUs = micros();
  float dt = (float)(uint32_t)(nowUs - prevUs) * 1e-6f;
  prevUs = nowUs;

#if DB_SERIAL_CONTROL
  pollSerial();
#endif
  net::loop();
  web::loop();
  pollWeather();

  button.update(millis());
  if (button.takeSingle()) brain.onInteraction(db::TOUCH_POKE);
  if (button.takeDouble()) brain.onInteraction(db::TOUCH_DOUBLE);
  if (button.takeLong())   brain.onInteraction(db::TOUCH_HOLD);

  touch.update(millis());
  if (simTouching && (int32_t)(simTouchUntil - millis()) <= 0) simTouching = false;
  brain.setTouch(touch.isDown() || simTouching, dt);

  brain.update(dt);
  face.update(dt);

  // Dozing dims the panel: easier on the eyes in a dark room and easier on
  // the OLED.
  if (brain.asleep() != dimmed) {
    dimmed = brain.asleep();
    display.dim(dimmed);
    if (!dimmed) applyBrightness();   // dim(false) resets the contrast
  }

  face.draw(display);
  display.display();

  const uint32_t frameUs = 1000000UL / DB_TARGET_FPS;
  const uint32_t spent = micros() - nowUs;
  if (spent < frameUs) {
    const uint32_t remain = frameUs - spent;
    if (remain > 2000) delay(remain / 1000);  // yields to the idle task
    else delayMicroseconds(remain);
  }
}

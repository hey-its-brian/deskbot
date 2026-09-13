// ---------------------------------------------------------------------------
//  Desk Buddy - an EMO-style animated face for an ESP32-C3 Super Mini and a
//  128x64 SSD1306 OLED.
//
//  Wiring, behaviour and tuning all live in config.h.
//  The face itself is in Face.cpp; what it decides to do is in Personality.cpp.
// ---------------------------------------------------------------------------
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>

#include "Button.h"
#include "Face.h"
#include "Personality.h"
#include "config.h"

static Adafruit_SSD1306 display(DB_SCREEN_W, DB_SCREEN_H, &Wire, -1);
static db::Face face;
static db::Personality brain;
static Button button(DB_PIN_BUTTON, DB_BUTTON_ACTIVE_LOW);

static uint8_t oledAddress = 0x3C;
static bool displayReady = false;
static bool dimmed = false;

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

// ---------------------------------------------------------------------------
//  Serial control - see docs/SERIAL.md
// ---------------------------------------------------------------------------
#if DB_SERIAL_CONTROL
static char cmdBuf[48];
static uint8_t cmdLen = 0;

static void printHelp() {
  Serial.println(F("desk buddy commands:"));
  Serial.println(F("  <emotion>        neutral happy excited sad angry surprised"));
  Serial.println(F("                   sleepy love curious suspicious dizzy bored"));
  Serial.println(F("  list             list emotions"));
  Serial.println(F("  auto on|off      autonomous mood changes (default on)"));
  Serial.println(F("  blink | wink l|r"));
  Serial.println(F("  look <x> <y>     gaze direction, -1..1"));
  Serial.println(F("  jolt             startle shake"));
  Serial.println(F("  poke             same as pressing the button"));
  Serial.println(F("  sleep | wake"));
  Serial.println(F("  drift on|off     slow anti burn-in wander"));
  Serial.println(F("  status"));
}

static void runCommand(char* line) {
  while (*line == ' ') ++line;
  if (*line == 0) return;

  char* arg = strchr(line, ' ');
  if (arg) { *arg++ = 0; while (*arg == ' ') ++arg; }

  db::Emotion e;
  if (db::emotionFromName(line, e)) {
    brain.setAutoMood(false);
    brain.wake(false);
    face.setEmotion(e);
    Serial.print(F("emotion: ")); Serial.println(db::emotionName(e));
    Serial.println(F("(auto mood off - 'auto on' to hand control back)"));
  } else if (!strcmp(line, "help") || !strcmp(line, "?")) {
    printHelp();
  } else if (!strcmp(line, "list")) {
    for (uint8_t i = 0; i < db::EMOTION_COUNT; ++i) {
      Serial.print(db::emotionName((db::Emotion)i));
      Serial.print((i + 1 == db::EMOTION_COUNT) ? '\n' : ' ');
    }
  } else if (!strcmp(line, "auto")) {
    const bool on = arg && !strcmp(arg, "on");
    brain.setAutoMood(on);
    Serial.print(F("auto mood: ")); Serial.println(on ? F("on") : F("off"));
  } else if (!strcmp(line, "blink")) {
    face.blink();
  } else if (!strcmp(line, "wink")) {
    face.wink(!arg || *arg == 'l');
  } else if (!strcmp(line, "look")) {
    float x = 0, y = 0;
    if (arg) {
      x = atof(arg);
      char* second = strchr(arg, ' ');
      if (second) y = atof(second + 1);
    }
    face.look(x, y, 2.5f);
  } else if (!strcmp(line, "jolt")) {
    face.jolt(1.0f);
  } else if (!strcmp(line, "poke")) {
    brain.onInteraction(db::TOUCH_POKE);
  } else if (!strcmp(line, "sleep")) {
    brain.sleep();
  } else if (!strcmp(line, "wake")) {
    brain.wake(false);
  } else if (!strcmp(line, "drift")) {
    face.setBurnInDrift(!arg || strcmp(arg, "off") != 0);
  } else if (!strcmp(line, "status")) {
    Serial.print(F("emotion=")); Serial.print(db::emotionName(face.emotion()));
    Serial.print(F(" auto=")); Serial.print(brain.autoMood() ? 1 : 0);
    Serial.print(F(" asleep=")); Serial.print(brain.asleep() ? 1 : 0);
    Serial.print(F(" idle=")); Serial.print(brain.idleSeconds(), 1);
    Serial.print(F("s oled=0x")); Serial.println(oledAddress, HEX);
  } else {
    Serial.print(F("? ")); Serial.print(line);
    Serial.println(F("  (try 'help')"));
  }
}

static void pollSerial() {
  while (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      cmdBuf[cmdLen] = 0;
      runCommand(cmdBuf);
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

  Wire.begin(DB_PIN_SDA, DB_PIN_SCL);
  Wire.setClock(DB_I2C_CLOCK);

  // Seed from the chip ID plus boot timing, so two buddies on a desk don't
  // blink in lockstep.
  const uint32_t seed =
      (uint32_t)micros() ^ (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFFull);
  face.begin(seed);
  face.setBurnInDrift(DB_BURN_IN_DRIFT ? true : false);
  brain.begin(face, seed);

  displayReady = startDisplay();
  if (displayReady) {
    Serial.print(F("SSD1306 found at 0x"));
    Serial.println(oledAddress, HEX);
    bootAnimation();
  }
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
      if (displayReady) bootAnimation();
    }
    delay(10);
    return;
  }

  static uint32_t prevUs = micros();
  const uint32_t nowUs = micros();
  float dt = (float)(uint32_t)(nowUs - prevUs) * 1e-6f;
  prevUs = nowUs;

#if DB_SERIAL_CONTROL
  pollSerial();
#endif

  button.update(millis());
  if (button.takeSingle()) brain.onInteraction(db::TOUCH_POKE);
  if (button.takeDouble()) brain.onInteraction(db::TOUCH_DOUBLE);
  if (button.takeLong())   brain.onInteraction(db::TOUCH_HOLD);

  brain.update(dt);
  face.update(dt);

  // Dozing dims the panel: easier on the eyes in a dark room and easier on
  // the OLED.
  if (brain.asleep() != dimmed) {
    dimmed = brain.asleep();
    display.dim(dimmed);
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

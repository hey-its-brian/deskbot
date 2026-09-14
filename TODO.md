# To do

Ideas that are worth building but aren't built yet. Each one has enough
detail to pick up cold.

## Motion sensing - look towards movement

**Sensor.** One PIR can't tell direction; it only says "something warm moved
in my cone". Two mini PIRs (AM312: 10 mm, 3.3 V, digital out, ~$1) angled
left and right in the shell give left / right / both. A VL53L5CX multizone
ToF (8x8 depth grid, I2C, ~$15) would give real direction *and* distance for
EMO-grade tracking, at the cost of a bigger library and a clear window.

**Wiring.** GPIO1 and GPIO3 are free. `INPUT_PULLDOWN`, `-1` disables, same
pattern as the touch pad in `src/config.h`.

**Behaviour.** On a fresh rising edge on one side, ~60% chance of a glance
that way via `Face::look()` for 2-3 s, occasionally with a curious tilt
(`Face::flash(EMOTION_CURIOUS, ...)`); ~5 s cooldown so it never gets
twitchy. Motion while asleep wakes it gently (`Personality::wake(false)`),
curious rather than startled. Sustained presence should slow the boredom
clock a little - someone being in the room is different from being ignored.
Ignore both sensors for the first 60 s after boot; PIRs trigger randomly
while they settle.

**Case.** Unlike the touch pad, a PIR needs a clear window - the white Fresnel
dome has to see out. Two 10 mm holes in the shell sides, angled ~30 degrees
outward.

**Hooks already in place.** `Face::look(x, y, hold)`, `Personality::wake()`,
`Personality::idleSeconds()`; a third `Button` instance per PIR gets the
debouncing for free.

## WiFi + web app

Settings and basic interaction from a phone or laptop, no USB.

**Firmware.** The C3 has WiFi. Keep one control vocabulary: the serial
parser in `src/main.cpp` (`runCommand()`) already understands `happy`,
`look -1 0.3`, `pet`, `auto off`; a web endpoint should feed it the same
strings rather than grow a second command set. `ESPAsyncWebServer` or the
core `WebServer`, mDNS so it answers at `deskbuddy.local`, and a single
static page served from PROGMEM - emotion buttons, poke / pet, auto on/off,
sleep and boredom timings, panel brightness. Credentials in a gitignored
`secrets.h`, or WiFiManager's captive portal for first-time setup.

**Do OTA at the same time** (`ArduinoOTA`): once it's on the network there's
no reason to keep walking a USB cable to it. `pio run -t upload
--upload-port deskbuddy.local`.

**Watch out for:** WiFi costs ~80 mA in bursts and the radio shares the
core with rendering - keep the web handlers short and never block in them,
or the frame rate stutters. `WiFi.setSleep(false)` if latency matters more
than power.

## Weather faces

Rain, sun, snow, cloudy, storm. Needs WiFi first.

**Data.** [Open-Meteo](https://open-meteo.com) - free, no API key, plain
JSON, a `weather_code` field that maps cleanly onto a handful of
conditions. Lat/lon in `config.h`, fetch every 15 minutes over HTTPS
(`WiFiClientSecure`). Parse the few fields by hand or with `ArduinoJson`.

**Faces.** Most of it is particles, which `src/Effects.cpp` already does:
new `FxType`s for `FX_RAIN` (fast falling streaks, whole screen),
`FX_SNOW` (slow drifting dots), `FX_CLOUD` (a lumpy blob drifting across
the top), `FX_RAY` (sun rays / sparkles). Pair each with an expression:
rain -> sad-ish neutral looking up, sun -> happy, snow -> sleepy and cosy,
cloudy -> bored, storm -> angry with a lightning flash (invert the panel
for one frame).

**When to show it.** Not permanently - it's a face, not a widget. A
"glance at the window" every 20 minutes or so: 4-5 s of the weather face,
then back to normal. Plus on demand: `weather` over serial / the web page,
and maybe a double-tap on the touch pad.

## Microphone / "hey buddy"

Two very different projects hide behind this one, and the C3 decides which.

**What the C3 can do:** sound *detection*. A MAX9814 (analog, GPIO0 ADC) or
INMP441 (I2S) mic, and it reacts to loud noises - startles at a clap, turns
towards a sudden sound, gets a "listening" face when you talk near it. A
clap pattern (double clap) as a command. Cheap, robust, and genuinely
charming on its own.

**What the C3 cannot really do:** on-device wake words. Espressif's ESP-SR
(WakeNet) targets the ESP32-S3's vector instructions; the C3 has one
RISC-V core, no PSRAM, and no headroom next to the renderer. Two routes to
real "hey buddy":

1. **Stream to a server.** Send I2S audio over WiFi to a box running
   openWakeWord + Whisper, or plug into Home Assistant's Assist pipeline.
   The buddy is then a microphone and a face; the brains are elsewhere.
   Works today on a C3.
2. **Move to an ESP32-S3.** The Seeed XIAO ESP32S3 Sense (~$14) has an
   I2S mic *and* a camera on board, runs ESP-SR wake words natively, and
   the camera would also cover the motion-sensing item above - face
   tracking instead of PIRs. All of this firmware is plain Arduino and
   would move across with a pin-map change in `config.h`.

Start with detection. If it's still fun, do route 2.

## Suggested

- **Bambu P1S print watcher.** The P1S in LAN-only mode publishes MQTT
  over TLS (port 8883, its LAN access code as the password, topic
  `device/<serial>/report`). Progress percent, layer, remaining time and
  state are all in there. The buddy could watch a print: pupils-free
  progress shown as the eyes slowly "filling", excited when it finishes,
  sad on a failure, and a glance at the printer (look left) now and then
  while it runs. `PubSubClient` + `WiFiClientSecure`. Needs WiFi.
- **Chirps.** EMO makes sounds. A passive piezo on a PWM pin gives beeps
  and boops per emotion - a rising two-note on wake, a descending one on
  sleep, a trill when petted. Very cheap, adds a surprising amount of
  character. Volume-limited by design; the panel is on a desk.
- **Time awareness.** NTP once WiFi is up: sleepy after a configured
  bedtime, a proper wake-up in the morning, a bigger reaction to the first
  interaction of the day. A quiet-hours window where it doesn't chirp.
- **Notifications in.** One HTTP endpoint, `/notify?emotion=happy&hold=5`,
  and anything on the network can make a face - a build finishing, a
  doorbell, a calendar reminder. Falls straight out of the web app.
- **Home Assistant.** MQTT discovery so the emotion and sleep state show up
  as entities, and automations can drive it. Mostly glue once MQTT exists
  for the printer.
- **Focus timer.** A pomodoro mode: eyes narrow into a concentrating face
  while a block runs, the panel edge fills as a progress bar, and it gets
  excited when the break starts. Start/stop from the touch pad.
- **Display off at night, properly.** `SSD1306_DISPLAYOFF` after a long
  sleep rather than dimming - the panel will last years longer.

## Smaller things

- Light sensor (LDR on GPIO0, ADC) so it dims in a dark room and sleeps at
  night instead of on a timer.
- `make clean` could also regenerate the previews rather than only deleting
  build output.

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

## Upgrade to the XIAO ESP32-S3 Sense

The board that unlocks the rest of this list: dual-core 240 MHz, 8 MB PSRAM,
mic and camera on board, native USB-C. Same XIAO footprint as the C6 but
nothing like it in capability.

**Port.** `platformio.ini` board line -> `seeed_xiao_esp32s3`, the platform
line to a version with S3 support, and the pin map in `config.h`. With the
camera board fitted the free pins are roughly D0-D5 plus I2C: OLED on the
I2C pair (D4/D5 = GPIO5/GPIO6, so the wiring numbers even stay the same),
touch pad on D1, a piezo on D2. Skip the SD slot; it costs pins.

**Then it enables:** on-device wake words (ESP-SR), face tracking with the
camera instead of PIRs for the motion item above, and rendering on one core
with WiFi/MQTT/TLS on the other so the face never stutters.

**Case.** The shell needs a camera window and a redesigned MCU cradle for the
two-board XIAO stack (21 x 17.5 mm). Keep the vent slots: with the camera on
it draws ~250-300 mA and runs warm; power the camera down except during the
moments it is actually looking.

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

## Phone alerts over Bluetooth - the "buzzing" face

Pair it with the phone and have it shake when a notification comes in.

**iPhone: ANCS.** Apple Notification Center Service is how smartwatches get
notifications - the phone is the server, the buddy is a BLE peripheral that
subscribes. Pair once in Settings > Bluetooth and iOS pushes every alert
(app, title, a snippet) with nothing installed on the phone. The C3 has BLE
5.0; use NimBLE-Arduino and an ANCS client (there are working ESP32 ANCS
examples to start from). WiFi and BLE share the radio on the C3 - it works,
but keep BLE connection intervals relaxed so the web app stays responsive.

**Android** has no ANCS equivalent. Two routes: a small companion app
(BLE notification listener - Gadgetbridge-style), or skip Bluetooth and use
the `/api/notify` endpoint from a phone automation app (MacroDroid,
Tasker) over WiFi, which already works today.

**The face.** `Face::buzz(seconds)`: a sustained, small-amplitude, high
frequency shake - the startle `jolt()` is a damped version of exactly this -
with wide eyes, plus a little phone/bell glyph in the corner and maybe a
FX_STAR "ring" ripple. Different apps could get different faces later
(messages -> happy, calendar -> surprised, a call -> the buzz until answered).
Quiet hours should silence it.

**Also possible once paired:** the phone's presence as a "you're home"
signal, and phone battery on the face if you like that sort of thing.

## Suggested

- **Bambu P1S print watcher.** The P1S in LAN-only mode publishes MQTT
  over TLS (port 8883, its LAN access code as the password, topic
  `device/<serial>/report`). Progress percent, layer, remaining time and
  state are all in there. The buddy could watch a print: progress shown as
  the eyes slowly "filling", excited when it finishes, sad on a failure, and
  a glance at the printer (look left) now and then while it runs.
  `PubSubClient` + `WiFiClientSecure`. WiFi is in place now; **this is next.**
- **Chirps.** EMO makes sounds. A passive piezo on a PWM pin gives beeps
  and boops per emotion - a rising two-note on wake, a descending one on
  sleep, a trill when petted. Very cheap, adds a surprising amount of
  character. Volume-limited by design; the panel is on a desk.
- **Time awareness.** NTP (WiFi is up now): sleepy after a configured
  bedtime, a proper wake-up in the morning, a bigger reaction to the first
  interaction of the day. A quiet-hours window where it doesn't chirp.
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

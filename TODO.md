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

## Smaller things

- Light sensor (LDR on GPIO0, ADC) so it dims in a dark room and sleeps at
  night instead of on a timer.
- A fully-off display after a long sleep (`SSD1306_DISPLAYOFF`), not just dim.
- `make clean` could also regenerate the previews rather than only deleting
  build output.

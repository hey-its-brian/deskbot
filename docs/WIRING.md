# Wiring

Four wires. That's the whole build.

| SSD1306 pin | ESP32-C3 Super Mini | Note |
| --- | --- | --- |
| `VCC` | `3V3` | the panel is 3.3 V native - do not feed it 5 V |
| `GND` | `GND` | |
| `SDA` | `GPIO5` | `DB_PIN_SDA` in `src/config.h` |
| `SCL` | `GPIO6` | `DB_PIN_SCL` |

Optional, and only if you want a button that isn't the onboard one:

| Button | ESP32-C3 | Note |
| --- | --- | --- |
| one leg | `GPIO9` | `DB_PIN_BUTTON`, internal pull-up |
| other leg | `GND` | |

## Touch pad (optional, recommended)

A **TTP223** capacitive touch module - the little 3-pin board that is nearly
every "touch sensor button" sold for microcontrollers. The C3 has no native
touch pins, so a module like this is the right way to do it anyway.

| TTP223 | ESP32-C3 | Note |
| --- | --- | --- |
| `VCC` | `3V3` | it runs 2.0-5.5 V; 3.3 V keeps the signal at C3 levels |
| `GND` | `GND` | |
| `I/O` (or `SIG`, `OUT`) | `GPIO4` | `DB_PIN_TOUCH` |

The module drives the line **HIGH while touched** and the firmware enables
the C3's internal pull-down, so it idles low even if unplugged.

Two things worth knowing about these modules:

- The two solder-jumper pads on the back, **A** and **B**, change how it
  behaves: A flips the output polarity, B turns it into a toggle (touch once
  on, touch again off). **Leave both open.** In toggle mode the tap-vs-hold
  logic can't work.
- It senses through plastic, but the module's own ~10 mm pad is marginal
  through a case wall. Sensitivity is set by **electrode area** and by the
  *Cs* capacitor on the board (less capacitance = more sensitive; an empty
  *Cs* footprint is already maximum). For a case, wire a 25 x 25 mm piece
  of copper or aluminium tape to the module's sensing pad and press it flat
  against the inside of the top - the printed case has a recess there with
  the wall thinned to 1.2 mm. That is how EMO-style head pats are done. If
  it then triggers on its own, move it away from the ESP32's antenna end
  or add a ground plane (a bit of foil under GND) behind it.

Set `DB_PIN_TOUCH` to `-1` if you don't have one; nothing else changes.

| Touch | What happens |
| --- | --- |
| tap | it looks at you and brightens up |
| double tap | excited |
| hold | petting: squints up at your hand, hearts, then heart-eyes at ~3 s |

## Why GPIO5 and GPIO6

The Arduino core's *default* I2C pins on the C3 are GPIO8 and GPIO9, and on
this particular board both are already spoken for: GPIO8 drives the onboard
blue LED and GPIO9 is the BOOT button (and a strapping pin, so it needs to be
high at reset). I2C would mostly work there, and then misbehave the one time
you press BOOT. GPIO5/GPIO6 are plain GPIOs with nothing attached.

Any free pins work - change `DB_PIN_SDA` / `DB_PIN_SCL` and rebuild.

## The button

`DB_PIN_BUTTON` defaults to GPIO9, the **onboard BOOT button**, so you get
interaction with zero extra parts:

| Press | What happens |
| --- | --- |
| short | poke - it startles, then cheers up |
| double | hearts |
| long (0.7 s) | sleep / wake |

One caveat that comes with reusing BOOT: if you hold it *while the board
resets*, the chip enters USB download mode instead of running the sketch.
Harmless - just press reset again, or unplug and replug.

Set `DB_PIN_BUTTON` to `-1` to disable button handling entirely.

## I2C speed

`DB_I2C_CLOCK` defaults to 800 kHz. A full 128x64 frame is 1024 bytes, so at
800 kHz a frame push costs about 13 ms - that is what leaves room for 30 fps.
At the more usual 400 kHz you are looking at ~26 ms per frame and the
animation gets choppy.

If the panel glitches, tears, or shows garbage: drop to `400000UL` first.
Long jumper wires and 800 kHz do not get along.

## Power

USB-C into the Super Mini. The whole thing draws well under 100 mA, so any
phone charger or a spare PC port is plenty. A USB battery pack works if you
want it untethered, though there is no battery management on this board - it
is a "plug it in" desk object, not a portable robot.

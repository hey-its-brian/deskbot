// ---------------------------------------------------------------------------
//  config.h - wiring and behaviour knobs. Only plain #defines live here, so
//  the file is safe to include from the Arduino-free modules too.
// ---------------------------------------------------------------------------
#pragma once

// --- I2C to the SSD1306 ----------------------------------------------------
//
// The ESP32-C3 can put I2C on almost any pin. GPIO5 / GPIO6 are chosen here
// because they are plain GPIOs on the Super Mini: the Arduino core's *default*
// C3 pins are GPIO8 (SDA) and GPIO9 (SCL), and on this board GPIO8 also drives
// the onboard LED while GPIO9 is the BOOT button and a strapping pin. Keeping
// I2C off both avoids a whole category of "works until I press BOOT" bugs.
#define DB_PIN_SDA 5
#define DB_PIN_SCL 6

// 0 = probe for the display at 0x3C then 0x3D at boot. Set to 0x3C or 0x3D to
// skip the probe. Most of these 0.96" modules are 0x3C.
#define DB_OLED_ADDRESS 0

// 800 kHz keeps a 128x64 frame push down to ~13 ms, which is what makes the
// animation smooth. If your wiring is long or the panel glitches, drop to
// 400000UL.
#define DB_I2C_CLOCK 800000UL

// --- Button ----------------------------------------------------------------
//
// GPIO9 is the Super Mini's onboard BOOT button, so you get a "poke me" button
// with no extra wiring. Wire your own button from any free pin to GND and
// change this if you'd rather have one on the front of the case.
// Set to -1 to disable button handling entirely.
#define DB_PIN_BUTTON 9
#define DB_BUTTON_ACTIVE_LOW 1

// --- Touch sensor ----------------------------------------------------------
//
// A TTP223-style capacitive touch module: VCC -> 3V3, GND -> GND, SIG (I/O)
// -> this pin. The module drives the line HIGH while touched (its default;
// leave the A/B solder pads on the back open). Works through 2-3 mm of PLA,
// so it can hide under the top of the case and become the buddy's "head".
// Set to -1 if you don't have one.
#define DB_PIN_TOUCH 4
#define DB_TOUCH_ACTIVE_HIGH 1

// A touch shorter than this is a tap (hello); longer is petting.
#define DB_PET_HOLD_S 0.35f

// --- Onboard LED -----------------------------------------------------------
//
// GPIO8, active LOW on the Super Mini. Off by default: it is a very bright
// blue and this is supposed to live on a desk.
#define DB_PIN_LED 8
#define DB_LED_ACTIVE_LOW 1
#define DB_LED_ENABLED 0

// --- Timing ----------------------------------------------------------------
#define DB_TARGET_FPS 30

// Seconds of no interaction before the buddy gets visibly bored, and before
// it nods off. Sleep dims the panel, which also protects it from burn-in.
#define DB_IDLE_BORED_S 90.0f
#define DB_IDLE_SLEEP_S 300.0f

// Slow sub-pixel wander of the whole face. OLEDs do burn in; leave this on if
// the buddy runs all day.
#define DB_BURN_IN_DRIFT 1

// Flip the image if you mount the panel upside down in the case.
#define DB_FLIP_DISPLAY 0

// Serial control (see docs/SERIAL.md). Costs nothing when unused.
#define DB_SERIAL_CONTROL 1

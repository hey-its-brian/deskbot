# Serial control

The Super Mini's native USB shows up as a CDC serial port, so you can drive
the face from your Mac or Linux box without any extra hardware. 115200 baud.

```sh
pio device monitor            # or: screen /dev/tty.usbmodem* 115200
```

Type `help` for the list. Everything is line-based, so it also works from a
script:

```sh
echo "happy" > /dev/tty.usbmodem101      # macOS
echo "happy" > /dev/ttyACM0              # Linux
```

| Command | Effect |
| --- | --- |
| `neutral` `happy` `excited` `sad` `angry` `surprised` `sleepy` `love` `curious` `suspicious` `dizzy` `bored` | set the expression (also turns autonomy off) |
| `list` | print the emotion names |
| `auto on` / `auto off` | hand control back to the personality, or take it |
| `blink` | one blink |
| `wink l` / `wink r` | wink |
| `look <x> <y>` | gaze direction, each `-1..1`, e.g. `look -1 0.3` |
| `jolt` | startle shake |
| `poke` | exactly as if you pressed the button |
| `tap` / `pet` | exactly as if you tapped / held the touch pad (pet holds 3.5 s) |
| `sleep` / `wake` | |
| `drift on` / `drift off` | slow anti burn-in wander (saved) |
| `weather` | show the current weather now |
| `weather refresh` | fetch it again |
| `set <key> <value>` | change a setting and save it: `bored`, `sleep`, `brightness`, `drift`, `weather`, `lat`, `lon`, `units`, `interval`, `glance` |
| `net` | WiFi state and address, or why the last attempt failed |
| `net scan` | list every network in range: channel, signal, security |
| `status` | version, emotion, idle time, I2C address, IP, last weather |

The same commands work over the network - `POST /api/cmd` with `c=<command>`;
see [WIFI.md](WIFI.md).

Naming an emotion switches autonomy **off** so your expression sticks. `auto
on` gives the buddy its own head back.

## Hooking it up to something else

Because it is plain line-based serial, anything that can write to a tty can
drive it - a shell script, a cron job, a Home Assistant command line switch,
a CI notifier:

```sh
# grumpy on a failing build
if ! make test >/dev/null 2>&1; then
  echo "angry" > /dev/ttyACM0
else
  echo "happy" > /dev/ttyACM0
fi
```

Turn the interface off entirely with `#define DB_SERIAL_CONTROL 0` in
`src/config.h`.

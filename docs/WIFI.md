# WiFi, the web app, OTA and weather

All of this is optional. Without a `src/secrets.h` the buddy runs offline,
exactly as before.

## Setup

```sh
cp src/secrets.example.h src/secrets.h
```

Fill in `DB_WIFI_SSID` and `DB_WIFI_PASS`. **2.4 GHz only** - the ESP32-C3
has no 5 GHz radio, so if your SSID is 5 GHz-only it will never connect. Then
flash over USB once:

```sh
pio run -t upload
pio device monitor
```

The console prints the address when it is up:

```
wifi: connecting to yournetwork
wifi: up, http://deskbuddy.local  (192.168.1.42)
```

`secrets.h` is gitignored. Everything else - location, timings, brightness -
is set from the web page and kept in flash, so the file really is just
credentials.

## The web app

Open `http://deskbuddy.local` (or the IP) from anything on the same network.
It is a single page with no external assets, so it works with no internet.

- **Right now** - what it is feeling, and poke / pet / tap / blink / startle /
  sleep buttons. The auto-mood toggle is here too.
- **Expression** - the twelve faces. Picking one takes it off autopilot, the
  same as naming one over serial.
- **Weather** - the last reading, *Show now* and *Refresh*.
- **Settings** - bored / sleep timings, brightness, drift, and the weather
  configuration. *Save* writes them to flash.

It refreshes every two seconds, so several people can have it open.

Set `DB_WEB_USER` / `DB_WEB_PASSWORD` in `secrets.h` for a login prompt. On a
home LAN it is fine without one; **do not port-forward it** - it is HTTP with
optional basic auth, designed for a trusted network.

### API

Everything the page does is four endpoints, so scripts and automations can do
the same:

```sh
# any serial command - see SERIAL.md
curl -X POST -d 'c=happy'         http://deskbuddy.local/api/cmd
curl -X POST -d 'c=look -1 0.3'   http://deskbuddy.local/api/cmd
curl -X POST -d 'c=set glance 30' http://deskbuddy.local/api/cmd

# show a face for a while, then go back to whatever it was doing
curl -X POST -d 'emotion=angry&hold=5' http://deskbuddy.local/api/notify

# everything the page shows, as JSON
curl http://deskbuddy.local/api/status

# settings (same fields as the form)
curl -X POST -d 'bored=120&sleep=600&brightness=200' http://deskbuddy.local/api/settings
```

`/api/notify` is the hook for "make a face when X happens": a build finishing,
a doorbell, a calendar reminder. `hold` is seconds, 0.5-120.

## Updating over the air

Once it is on the network there is no reason to walk a USB cable to it:

```sh
pio run -e esp32-c3-supermini-ota -t upload
```

The panel shows a progress bar while the image comes in, then it reboots.
Set `DB_OTA_PASSWORD` in `secrets.h` and uncomment `upload_flags` in
`platformio.ini` to match - otherwise anyone on the LAN can push firmware
to it. If `deskbuddy.local` does not resolve from your machine (Linux without
Avahi, some Windows setups), use the IP: `--upload-port 192.168.1.42`.

## Weather

Conditions come from [Open-Meteo](https://open-meteo.com) - free, no API
key. Set your latitude and longitude in the web app (or `set lat 51.5`,
`set lon -0.12` over serial), pick °C or °F, and it fetches every 15 minutes
by default.

It is a face, not a widget: the weather shows as a **glance** - six seconds
of the matching expression, the conditions animated around it, and the
temperature small in the corner - then it goes back to whatever it was
doing. A glance happens:

- every 20 minutes (`glance` in settings; 0 turns the unprompted ones off),
- whenever the conditions change,
- on demand: `weather` over serial, or *Show now* on the web page.

| Condition | What it does |
| --- | --- |
| clear, day | happy, sun in the corner |
| clear, night | calm, moon, the odd twinkle |
| partly cloudy | neutral, sun and a cloud |
| overcast | bored, clouds drifting across the top |
| fog | squinting through drifting wisps |
| rain | glum, rain streaking down |
| snow | curious, watching the flakes |
| thunderstorm | rain, and it jumps at every lightning bolt |

The fetch runs on its own task, so the ~1 s TLS handshake never freezes the
face. Certificate validation is off for that one connection: it is public
weather data, and a CA bundle would be more firmware than the feature.

## Troubleshooting

**Still stuck after all of the below?** Flash the bare WiFi test instead of
the buddy: `pio run -e wifitest -t upload && pio device monitor`. It is
thirty lines of nothing but "scan, then connect", with the ESP-IDF's own
WiFi logging at maximum, so the console shows the raw auth -> assoc ->
handshake sequence and exactly where it stops. If *that* connects and the
buddy does not, it is the firmware and worth reporting; if neither does,
it is the network.


- **Never gets an address**: the console now says why - `wifi: not
  connected, reason 201: no AP found ...` and so on, once per distinct
  reason. `net` over serial repeats the last one; `net scan` lists every
  network in range with channel, signal and security, and marks the
  configured one, so you can see at once whether the board can even hear
  your AP. The usual causes: a 5 GHz-only SSID (the C3 is 2.4 GHz only);
  a `"` or `\` in the password that needs escaping in the C string
  (`\"`, `\\`); a WPA3-only network (mixed WPA2/WPA3 is fine); and the
  Super Mini's antenna - many of these boards will not associate at full
  transmit power unless they are next to the AP, so the firmware backs the
  radio off to 8.5 dBm (`DB_WIFI_TX_POWER_8_5DBM`).
- **`deskbuddy.local` doesn't resolve**: macOS and iOS do this natively;
  Linux needs `avahi-daemon`; use the IP from the serial console otherwise.
- **`weather: fetch failed, http -1`**: DNS or TLS could not get through -
  usually the network blocks outbound HTTPS from IoT devices, or the clock
  is wildly off. `http 400` means the lat/lon is out of range.
- **Face stutters when you open the page**: it shouldn't; the handlers are
  tiny. If it does, set `DB_WIFI_POWER_SAVE` back to 1 - some access points
  misbehave with modem sleep off.

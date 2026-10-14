# Desk Buddy - project rules

## No telemetry, ever

Nothing in this repo, and nothing added to it, may report usage, crashes,
analytics or any other data back to a company or third party.

- Never add telemetry, analytics, crash reporting or "phone home" code to the
  firmware, the host tools, the web page or the build.
- When a tool or dependency ships with telemetry on by default, turn it off
  in the repo (env var, config file, Makefile) so it stays off for everyone
  who clones it. Document the one-time user-level switch too if there is one.
  Current state: PlatformIO Core telemetry is disabled via
  `PLATFORMIO_SETTING_ENABLE_TELEMETRY=No` in the Makefile and documented in
  the README.
- Before adding a dependency, check whether it has telemetry. Prefer one that
  does not. If the only good option does, disable it as above and say so in
  the commit message.
- The web page served by the buddy must stay self-contained: no CDN scripts,
  fonts, trackers or external images.
- The only outbound network traffic the firmware makes is what the owner
  asked for: the Open-Meteo weather fetch, mDNS and OTA on the local network.
  Any new outbound call needs a clear reason, must be documented in the
  README, and must be off when the feature is off.

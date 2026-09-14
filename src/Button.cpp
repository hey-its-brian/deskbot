#include "Button.h"

void Button::update(uint32_t now) {
  if (pin_ < 0) return;

  const bool raw = activeLow_ ? (digitalRead(pin_) == LOW) : (digitalRead(pin_) == HIGH);
  if (raw != lastRaw_) {
    lastRaw_ = raw;
    lastChange_ = now;
  }

  if ((now - lastChange_) >= kDebounceMs && raw != stable_) {
    stable_ = raw;
    if (stable_) {                       // pressed
      pressStart_ = now;
      longFired_ = false;
    } else {                             // released
      if (!longFired_) {
        if (clickPending_ && (now - lastRelease_) <= kDoubleMs) {
          evDouble_ = true;
          clickPending_ = false;
        } else {
          clickPending_ = true;
          lastRelease_ = now;
        }
      }
    }
  }

  // Long press fires while the button is still held down.
  if (stable_ && !longFired_ && (now - pressStart_) >= kLongMs) {
    longFired_ = true;
    clickPending_ = false;
    evLong_ = true;
  }

  // A click only becomes a *single* click once the double-click window closes.
  if (clickPending_ && !stable_ && (now - lastRelease_) > kDoubleMs) {
    clickPending_ = false;
    evSingle_ = true;
  }
}

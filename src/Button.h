// ---------------------------------------------------------------------------
//  Button.h - debounce + single / double / long press detection.
// ---------------------------------------------------------------------------
#pragma once

#include <Arduino.h>

class Button {
 public:
  Button(int pin, bool activeLow) : pin_(pin), activeLow_(activeLow) {}

  void begin() {
    if (pin_ < 0) return;
    pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT_PULLDOWN);
  }

  void update(uint32_t now);

  // Each returns true once, then clears itself.
  bool takeSingle() { return take(evSingle_); }
  bool takeDouble() { return take(evDouble_); }
  bool takeLong()   { return take(evLong_); }

  bool isDown() const { return stable_; }

 private:
  static const uint32_t kDebounceMs = 25;
  static const uint32_t kLongMs = 700;
  static const uint32_t kDoubleMs = 320;

  static bool take(bool& flag) {
    const bool v = flag;
    flag = false;
    return v;
  }

  int pin_;
  bool activeLow_;
  bool stable_ = false;
  bool lastRaw_ = false;
  uint32_t lastChange_ = 0;
  uint32_t pressStart_ = 0;
  uint32_t lastRelease_ = 0;
  bool longFired_ = false;
  bool clickPending_ = false;
  bool evSingle_ = false, evDouble_ = false, evLong_ = false;
};

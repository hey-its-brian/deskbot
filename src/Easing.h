// ---------------------------------------------------------------------------
//  Easing.h - tiny math helpers + a deterministic RNG.
//
//  Nothing in here touches the Arduino API, so the same code compiles for the
//  ESP32 and for the host-side preview harness in tools/host.
// ---------------------------------------------------------------------------
#pragma once

#include <math.h>
#include <stdint.h>

namespace db {

inline float clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// Frame-rate independent exponential smoothing.
// `tau` is roughly "seconds to cover 63% of the remaining distance".
inline float approach(float cur, float tgt, float tau, float dt) {
  if (tau <= 0.0001f) return tgt;
  return cur + (tgt - cur) * (1.0f - expf(-dt / tau));
}

// Smoothstep: 0 -> 1 with zero velocity at both ends.
inline float easeInOut(float t) {
  t = clampf(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

// Overshoots slightly past 1.0 before settling - gives motion a bit of snap.
inline float easeOutBack(float t) {
  t = clampf(t, 0.0f, 1.0f);
  const float c1 = 1.70158f, c3 = c1 + 1.0f;
  const float u = t - 1.0f;
  return 1.0f + c3 * u * u * u + c1 * u * u;
}

// Decaying wobble, used for the startle shake.
inline float damped(float t, float freq, float decay) {
  return sinf(t * freq) * expf(-t * decay);
}

// xorshift32. Small, fast, and identical on the board and on the host, so a
// preview render matches what the hardware will actually do.
class Rng {
 public:
  explicit Rng(uint32_t s = 0x1234567u) { seed(s); }

  void seed(uint32_t s) { s_ = s ? s : 0x1234567u; }

  uint32_t next() {
    s_ ^= s_ << 13;
    s_ ^= s_ >> 17;
    s_ ^= s_ << 5;
    return s_;
  }

  float f() { return (float)(next() >> 8) * (1.0f / 16777216.0f); }  // [0,1)
  float range(float a, float b) { return a + (b - a) * f(); }
  bool chance(float p) { return f() < p; }

  int irange(int a, int b) {  // inclusive, tolerates b < a
    if (b < a) { int t = a; a = b; b = t; }
    return a + (int)(next() % (uint32_t)(b - a + 1));
  }

 private:
  uint32_t s_;
};

}  // namespace db

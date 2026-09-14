// ---------------------------------------------------------------------------
//  Effects.h - the little things that float around the face: sleep Z's,
//  hearts, a nervous sweat drop, exclamation marks, tears.
// ---------------------------------------------------------------------------
#pragma once

#include "Easing.h"
#include "Gfx.h"

namespace db {

enum FxType : uint8_t {
  FX_HEART = 0,
  FX_ZZZ,
  FX_SWEAT,
  FX_EXCLAIM,
  FX_TEAR,
  FX_NOTE,
  FX_STAR
};

class Effects {
 public:
  static const int MAX_PARTICLES = 8;

  void clear();
  void spawn(FxType type, float x, float y, float vx, float vy, float life, float size);
  void update(float dt);
  void draw(Canvas& g) const;

  int countOf(FxType type) const;
  int active() const;

 private:
  struct Particle {
    float x, y, vx, vy;
    float age, life, size;
    FxType type;
    bool alive;
  };
  Particle p_[MAX_PARTICLES] = {};
};

}  // namespace db

#include "Effects.h"

#include <math.h>

#include "Shapes.h"

namespace db {

void Effects::clear() {
  for (int i = 0; i < MAX_PARTICLES; ++i) p_[i].alive = false;
}

void Effects::spawn(FxType type, float x, float y, float vx, float vy,
                    float life, float size) {
  if (!(life > 0.0f)) return;  // age/life is divided later; also rejects NaN
  int slot = -1;
  float oldest = -1.0f;
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    if (!p_[i].alive) { slot = i; break; }
    if (p_[i].age > oldest) { oldest = p_[i].age; slot = i; }  // recycle
  }
  if (slot < 0) return;
  Particle& q = p_[slot];
  q.x = x; q.y = y; q.vx = vx; q.vy = vy;
  q.age = 0.0f; q.life = life; q.size = size;
  q.type = type; q.alive = true;
}

void Effects::update(float dt) {
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    Particle& q = p_[i];
    if (!q.alive) continue;
    q.age += dt;
    if (q.age >= q.life) { q.alive = false; continue; }
    q.x += q.vx * dt;
    q.y += q.vy * dt;
    if (q.type == FX_TEAR || q.type == FX_SWEAT) q.vy += 26.0f * dt;  // gravity
    if (q.type == FX_HEART || q.type == FX_ZZZ || q.type == FX_NOTE) {
      q.x += sinf(q.age * 3.4f + (float)i) * 9.0f * dt;              // drift
    }
    if (q.type == FX_SNOW) q.x += sinf(q.age * 2.1f + (float)i) * 6.0f * dt;
  }
}

void Effects::draw(Canvas& g) const {
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    const Particle& q = p_[i];
    if (!q.alive) continue;
    const float t = q.age / q.life;
    if (t > 0.92f && q.type < FX_RAIN) continue;  // pop out just before death

    // Grow in, shrink out - except weather, which just travels off-screen.
    float s = q.size;
    const bool weather = (q.type >= FX_RAIN);
    if (!weather) {
      if (t < 0.18f) s *= easeOutBack(t / 0.18f);
      else if (t > 0.7f) s *= 1.0f - (t - 0.7f) / 0.3f;
    }
    const int size = (int)lroundf(s);
    if (size < 1) continue;

    const int x = (int)lroundf(q.x);
    const int y = (int)lroundf(q.y);

    switch (q.type) {
      case FX_HEART:   drawHeart(g, x, y, size * 2, size * 2, DB_WHITE); break;
      case FX_ZZZ:     drawZ(g, x, y, size, DB_WHITE); break;
      case FX_SWEAT:
      case FX_TEAR:    drawDroplet(g, x, y, (size + 1) / 2, DB_WHITE); break;
      case FX_EXCLAIM: drawExclaim(g, x, y, size * 2, DB_WHITE); break;
      case FX_NOTE:    drawNote(g, x, y, size, DB_WHITE); break;
      case FX_STAR:    drawStar(g, x, y, size, DB_WHITE); break;
      case FX_RAIN:    g.drawFastVLine(x, y, size, DB_WHITE); break;
      case FX_SNOW:
        if (size >= 2) drawStar(g, x, y, 1, DB_WHITE);
        else g.drawPixel(x, y, DB_WHITE);
        break;
      case FX_CLOUD:   drawCloud(g, x, y, size * 4, DB_WHITE); break;
      case FX_FOG:
        for (int i = 0; i < size; i += 4) g.drawFastHLine(x + i, y, 2, DB_WHITE);
        break;
      case FX_BOLT:    drawBolt(g, x, y, size, DB_WHITE); break;
    }
  }
}

int Effects::countOf(FxType type) const {
  int n = 0;
  for (int i = 0; i < MAX_PARTICLES; ++i)
    if (p_[i].alive && p_[i].type == type) ++n;
  return n;
}

int Effects::active() const {
  int n = 0;
  for (int i = 0; i < MAX_PARTICLES; ++i) if (p_[i].alive) ++n;
  return n;
}

}  // namespace db

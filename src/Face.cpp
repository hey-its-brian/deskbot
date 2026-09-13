#include "Face.h"

#include <math.h>

#include "Shapes.h"

namespace db {
namespace {

const char* const kNames[EMOTION_COUNT] = {
    "neutral", "happy",  "excited",    "sad",   "angry", "surprised",
    "sleepy",  "love",   "curious",    "suspicious", "dizzy", "bored"};

bool ciEqual(const char* a, const char* b) {
  while (*a && *b) {
    char ca = *a, cb = *b;
    if (ca >= 'A' && ca <= 'Z') ca = (char)(ca + 32);
    if (cb >= 'A' && cb <= 'Z') cb = (char)(cb + 32);
    if (ca != cb) return false;
    ++a; ++b;
  }
  return *a == 0 && *b == 0;
}

void defaults(Pose& p) {
  p.w = 36.0f;  p.h = 36.0f;  p.radius = 11.0f;  p.spacing = 52.0f;
  p.lidTop = 0.0f;  p.lidBot = 0.0f;  p.slant = 0.0f;  p.arc = 0.0f;
  p.offY = 0.0f;  p.tilt = 0.0f;  p.scaleL = 1.0f;  p.scaleR = 1.0f;
  p.gazeX = 0.0f;  p.gazeY = 0.0f;  p.sparkle = 0.0f;  p.bob = 1.2f;
  p.style = STYLE_EYES;
}

}  // namespace

const char* emotionName(Emotion e) {
  return (e < EMOTION_COUNT) ? kNames[e] : "?";
}

bool emotionFromName(const char* name, Emotion& out) {
  for (uint8_t i = 0; i < EMOTION_COUNT; ++i) {
    if (ciEqual(name, kNames[i])) { out = (Emotion)i; return true; }
  }
  return false;
}

// ---------------------------------------------------------------------------
//  The expression table. Everything the face can look like lives here, so
//  this is the place to tweak if you want a grumpier or sleepier buddy.
// ---------------------------------------------------------------------------
void poseFor(Emotion e, Pose& p) {
  defaults(p);
  switch (e) {
    case EMOTION_NEUTRAL:
      p.sparkle = 1.0f;
      break;

    case EMOTION_HAPPY:              // eyes squeeze up into two happy domes
      p.w = 38.0f; p.h = 34.0f; p.radius = 16.0f; p.arc = 0.42f;
      p.bob = 1.7f;
      break;

    case EMOTION_EXCITED:            // big, wide, bouncing
      p.w = 40.0f; p.h = 40.0f; p.radius = 14.0f; p.spacing = 54.0f;
      p.arc = 0.16f; p.sparkle = 1.0f; p.bob = 2.8f;
      break;

    case EMOTION_SAD:                // inner brows up, looking at the floor
      p.w = 34.0f; p.h = 30.0f; p.radius = 10.0f;
      p.slant = -7.0f; p.lidTop = 0.22f; p.offY = 4.0f; p.gazeY = 0.30f;
      p.bob = 0.8f;
      break;

    case EMOTION_ANGRY:              // brows jammed down towards the nose
      p.w = 38.0f; p.h = 30.0f; p.radius = 8.0f; p.spacing = 48.0f;
      p.slant = 10.0f; p.lidTop = 0.30f; p.bob = 1.0f;
      break;

    case EMOTION_SURPRISED:          // wide open
      p.w = 44.0f; p.h = 44.0f; p.radius = 16.0f; p.spacing = 54.0f;
      p.offY = -1.0f; p.sparkle = 1.0f; p.bob = 0.5f;
      break;

    case EMOTION_SLEEPY:             // heavy lids
      p.w = 36.0f; p.h = 30.0f; p.radius = 10.0f;
      p.lidTop = 0.56f; p.lidBot = 0.08f; p.offY = 4.0f; p.bob = 0.6f;
      break;

    case EMOTION_LOVE:
      p.w = 34.0f; p.h = 32.0f; p.bob = 2.0f; p.style = STYLE_HEART;
      break;

    case EMOTION_CURIOUS:            // head tilt + one eye wider
      p.scaleL = 1.15f; p.scaleR = 0.85f; p.tilt = 5.0f;
      p.lidTop = 0.12f; p.slant = -3.0f; p.gazeX = 0.25f; p.sparkle = 1.0f;
      break;

    case EMOTION_SUSPICIOUS:         // narrowed, side-eye
      p.w = 38.0f; p.h = 26.0f; p.radius = 7.0f; p.spacing = 50.0f;
      p.lidTop = 0.42f; p.lidBot = 0.18f; p.slant = 4.0f; p.gazeX = -0.40f;
      p.bob = 0.7f;
      break;

    case EMOTION_DIZZY:
      p.w = 34.0f; p.h = 34.0f; p.bob = 1.6f; p.style = STYLE_SPIRAL;
      break;

    case EMOTION_BORED:              // half-lidded, staring off sideways
      p.w = 36.0f; p.h = 26.0f; p.radius = 9.0f;
      p.lidTop = 0.45f; p.offY = 3.0f; p.gazeX = -0.5f; p.gazeY = 0.2f;
      p.bob = 0.7f;
      break;

    default:
      break;
  }
}

// ---------------------------------------------------------------------------
//  Lifecycle
// ---------------------------------------------------------------------------
void Face::begin(uint32_t seed) {
  rng_.seed(seed);
  poseFor(EMOTION_NEUTRAL, tgt_);
  cur_ = tgt_;
  emotion_ = EMOTION_NEUTRAL;
  nextBlink_ = rng_.range(1.5f, 3.0f);
  nextSaccade_ = rng_.range(0.8f, 2.0f);
  fx_.clear();
}

void Face::applyPose(Emotion e, bool immediate) {
  emotion_ = e;
  poseFor(e, tgt_);
  if (immediate) {
    cur_ = tgt_;
  } else {
    cur_.style = tgt_.style;  // shape swaps instantly, behind closed lids
  }

  // One-shot punctuation for expressions that deserve it.
  switch (e) {
    case EMOTION_SURPRISED:
      fx_.spawn(FX_EXCLAIM, 18.0f, 12.0f, 0.0f, -3.0f, 0.85f, 5.0f);
      fx_.spawn(FX_EXCLAIM, 110.0f, 12.0f, 0.0f, -3.0f, 0.85f, 5.0f);
      break;
    case EMOTION_ANGRY:
      fx_.spawn(FX_SWEAT, 112.0f, 16.0f, 6.0f, 8.0f, 1.1f, 5.0f);
      break;
    case EMOTION_SAD:
      fx_.spawn(FX_TEAR, 22.0f, 54.0f, -2.0f, 16.0f, 0.7f, 5.0f);
      break;
    default:
      break;
  }
  fxTimer_ = 0.25f;
}

void Face::setEmotion(Emotion e, bool immediate) {
  if (e >= EMOTION_COUNT) return;
  holding_ = false;
  if (immediate) {
    hasPending_ = false;
    applyPose(e, true);
    return;
  }
  if (e == emotion_ && !hasPending_) return;
  pending_ = e;
  hasPending_ = true;
  if (blinkPhase_ == 0) blink();  // hide the change behind a blink
}

void Face::flash(Emotion e, float holdSeconds, Emotion next) {
  setEmotion(e);
  afterHold_ = next;
  holdT_ = holdSeconds;
  holding_ = true;
}

void Face::blink() {
  if (blinkPhase_ != 0) return;
  blinkPhase_ = 1;
  blinkT_ = 0.0f;
}

void Face::wink(bool leftEye) {
  if (winkT_ >= 0.0f) return;
  winkEye_ = leftEye ? 0 : 1;
  winkT_ = 0.0f;
}

void Face::look(float x, float y, float holdSeconds) {
  gazeTX_ = clampf(x, -1.0f, 1.0f);
  gazeTY_ = clampf(y, -1.0f, 1.0f);
  gazeHold_ = holdSeconds;
  nextSaccade_ = holdSeconds + rng_.range(0.3f, 1.2f);
}

void Face::jolt(float strength) {
  shakeT_ = 0.0f;
  shakeAmt_ = clampf(strength, 0.0f, 2.0f);
}

// ---------------------------------------------------------------------------
//  Per-frame update
// ---------------------------------------------------------------------------
void Face::spawnMoodEffects(float dt) {
  fxTimer_ -= dt;
  if (fxTimer_ > 0.0f) return;

  switch (emotion_) {
    case EMOTION_SLEEPY:
      fx_.spawn(FX_ZZZ, rng_.range(92.0f, 104.0f), 24.0f,
                rng_.range(2.0f, 6.0f), -7.5f, 2.6f, rng_.range(4.0f, 7.0f));
      fxTimer_ = 1.2f;
      break;
    case EMOTION_LOVE:
      fx_.spawn(FX_HEART, rng_.range(14.0f, 114.0f), 58.0f,
                rng_.range(-6.0f, 6.0f), -13.0f, 2.3f, rng_.range(3.0f, 5.0f));
      fxTimer_ = 0.6f;
      break;
    case EMOTION_EXCITED:
      fx_.spawn(FX_STAR, rng_.range(8.0f, 120.0f), rng_.range(4.0f, 18.0f),
                0.0f, -5.0f, 0.9f, rng_.range(2.0f, 4.0f));
      fxTimer_ = 0.38f;
      break;
    case EMOTION_DIZZY:
      fx_.spawn(FX_STAR, rng_.range(10.0f, 118.0f), rng_.range(4.0f, 14.0f),
                rng_.range(-8.0f, 8.0f), -2.0f, 1.1f, 3.0f);
      fxTimer_ = 0.5f;
      break;
    case EMOTION_SAD:
      fx_.spawn(FX_TEAR, rng_.chance(0.5f) ? 22.0f : 106.0f, 54.0f,
                0.0f, 15.0f, 0.7f, 5.0f);
      fxTimer_ = 2.4f;
      break;
    case EMOTION_ANGRY:
      fx_.spawn(FX_SWEAT, 112.0f, 16.0f, 6.0f, 9.0f, 1.1f, 5.0f);
      fxTimer_ = 3.0f;
      break;
    default:
      fxTimer_ = 0.6f;
      break;
  }
}

void Face::update(float dt) {
  if (dt > 0.1f) dt = 0.1f;  // a hiccup shouldn't fling the animation forward
  if (dt < 0.0f) dt = 0.0f;
  t_ += dt;

  breathe_ += dt * 1.9f;  if (breathe_ > 6.2832f) breathe_ -= 6.2832f;
  beat_    += dt * 5.2f;  if (beat_    > 6.2832f) beat_    -= 6.2832f;
  spin_    += dt * 3.4f;  if (spin_    > 6.2832f) spin_    -= 6.2832f;

  if (holding_) {
    holdT_ -= dt;
    if (holdT_ <= 0.0f) { holding_ = false; setEmotion(afterHold_); }
  }

  if (shakeT_ >= 0.0f) {
    shakeT_ += dt;
    if (shakeT_ > 0.85f) shakeT_ = -1.0f;
  }

  // ---- blink -------------------------------------------------------------
  const float kClose = 0.075f, kOpen = 0.11f;
  switch (blinkPhase_) {
    case 0:
      nextBlink_ -= dt;
      if (autoBlink_ && nextBlink_ <= 0.0f) blink();
      break;
    case 1:
      blinkT_ += dt;
      blinkAmt_ = easeInOut(blinkT_ / kClose);
      if (blinkT_ >= kClose) {
        blinkAmt_ = 1.0f;
        if (hasPending_) { hasPending_ = false; applyPose(pending_, false); }
        blinkPhase_ = 2;
        blinkT_ = 0.0f;
      }
      break;
    default:
      blinkT_ += dt;
      blinkAmt_ = 1.0f - easeInOut(blinkT_ / kOpen);
      if (blinkT_ >= kOpen) {
        blinkAmt_ = 0.0f;
        blinkPhase_ = 0;
        // Humans blink in clusters; so does the buddy.
        nextBlink_ = rng_.chance(0.18f) ? rng_.range(0.18f, 0.4f)
                                        : rng_.range(2.2f, 6.5f);
      }
      break;
  }

  // ---- wink --------------------------------------------------------------
  if (winkT_ >= 0.0f) {
    winkT_ += dt;
    float a;
    if (winkT_ < 0.09f)      a = easeInOut(winkT_ / 0.09f);
    else if (winkT_ < 0.30f) a = 1.0f;
    else if (winkT_ < 0.44f) a = 1.0f - easeInOut((winkT_ - 0.30f) / 0.14f);
    else { a = 0.0f; winkT_ = -1.0f; winkEye_ = -1; }
    winkL_ = (winkEye_ == 0) ? a : 0.0f;
    winkR_ = (winkEye_ == 1) ? a : 0.0f;
  }

  // ---- gaze --------------------------------------------------------------
  if (gazeHold_ > 0.0f) gazeHold_ -= dt;
  if (autoGaze_ && gazeHold_ <= 0.0f) {
    nextSaccade_ -= dt;
    if (nextSaccade_ <= 0.0f) {
      if (rng_.chance(0.34f)) {            // look back at whoever is sitting there
        gazeTX_ = 0.0f; gazeTY_ = 0.0f;
      } else {
        gazeTX_ = rng_.range(-1.0f, 1.0f);
        gazeTY_ = rng_.range(-0.7f, 0.7f);
      }
      nextSaccade_ = rng_.range(0.9f, 3.2f);
      if (rng_.chance(0.22f)) blink();     // a blink often rides along with a glance
    }
  }
  gazeX_ = approach(gazeX_, gazeTX_ + cur_.gazeX, 0.085f, dt);
  gazeY_ = approach(gazeY_, gazeTY_ + cur_.gazeY, 0.085f, dt);

  // ---- ease the live pose towards the target -----------------------------
  cur_.w       = approach(cur_.w,       tgt_.w,       0.10f, dt);
  cur_.h       = approach(cur_.h,       tgt_.h,       0.10f, dt);
  cur_.radius  = approach(cur_.radius,  tgt_.radius,  0.10f, dt);
  cur_.spacing = approach(cur_.spacing, tgt_.spacing, 0.12f, dt);
  cur_.lidTop  = approach(cur_.lidTop,  tgt_.lidTop,  0.075f, dt);
  cur_.lidBot  = approach(cur_.lidBot,  tgt_.lidBot,  0.075f, dt);
  cur_.slant   = approach(cur_.slant,   tgt_.slant,   0.09f, dt);
  cur_.arc     = approach(cur_.arc,     tgt_.arc,     0.09f, dt);
  cur_.offY    = approach(cur_.offY,    tgt_.offY,    0.13f, dt);
  cur_.tilt    = approach(cur_.tilt,    tgt_.tilt,    0.13f, dt);
  cur_.scaleL  = approach(cur_.scaleL,  tgt_.scaleL,  0.11f, dt);
  cur_.scaleR  = approach(cur_.scaleR,  tgt_.scaleR,  0.11f, dt);
  cur_.gazeX   = approach(cur_.gazeX,   tgt_.gazeX,   0.20f, dt);
  cur_.gazeY   = approach(cur_.gazeY,   tgt_.gazeY,   0.20f, dt);
  cur_.sparkle = approach(cur_.sparkle, tgt_.sparkle, 0.10f, dt);
  cur_.bob     = approach(cur_.bob,     tgt_.bob,     0.30f, dt);

  spawnMoodEffects(dt);
  fx_.update(dt);
}

// ---------------------------------------------------------------------------
//  Rendering
//
//  An eye is one white rounded rectangle that then gets *carved* by black
//  column fills: a top lid (optionally slanted, which is what reads as an
//  eyebrow), a bottom lid, and a circular bite out of the bottom that turns
//  the eye into a happy dome. Doing it column by column instead of with
//  polygons means no seams and no overdraw of the neighbouring eye.
// ---------------------------------------------------------------------------
void Face::drawEye(Canvas& g, float cxf, float cyf, float scale, float closed,
                   bool innerIsRight) const {
  const float wf = cur_.w * scale;
  const float hf = cur_.h * scale * (1.0f - clampf(closed, 0.0f, 1.0f));

  int w = (int)lroundf(wf);
  int h = (int)lroundf(hf);
  if (w < 3) w = 3;
  if (h < 2) h = 2;

  const int cx = (int)lroundf(cxf);
  const int cy = (int)lroundf(cyf);
  const int x0 = cx - w / 2;
  const int y0 = cy - h / 2;

  int r = (int)lroundf(cur_.radius * scale);
  const int maxR = ((w < h ? w : h) - 1) / 2;
  if (r > maxR) r = maxR;
  if (r < 0) r = 0;

  g.fillRoundRect(x0, y0, w, h, r, DB_WHITE);
  if (h <= 3) return;  // already a slit; nothing left to carve

  const float lidTopPx = cur_.lidTop * (float)h;
  const float lidBotPx = cur_.lidBot * (float)h;
  const float slant = cur_.slant * scale;

  const bool doTop = (lidTopPx > 0.2f) || (fabsf(slant) > 0.2f);
  const bool doBot = (lidBotPx > 0.2f);
  const bool doArc = (cur_.arc > 0.01f);

  // The "happy" bite. The mask circle sits *above* the eye, so it eats more
  // at the outer edges than in the middle - that leaves a rainbow arch, which
  // is what reads as a smiling eye. (A circle below the eye does the opposite
  // and leaves the shape with two little legs.)
  float arcR = 0.0f, arcCY = 0.0f;
  if (doArc) {
    arcR = wf * 0.72f + 4.0f;
    arcCY = (float)(y0 + h) - cur_.arc * (float)h - arcR;
  }

  for (int x = x0; x < x0 + w; ++x) {
    const float t = (w > 1) ? (float)(x - x0) / (float)(w - 1) : 0.0f;
    const float inner = innerIsRight ? t : (1.0f - t);

    if (doTop) {
      int n = (int)lroundf(lidTopPx + slant * (inner - 0.5f));
      if (n > h) n = h;
      if (n > 0) g.drawFastVLine(x, y0, n, DB_BLACK);
    }
    if (doBot) {
      int n = (int)lroundf(lidBotPx);
      if (n > h) n = h;
      if (n > 0) g.drawFastVLine(x, y0 + h - n, n, DB_BLACK);
    }
    if (doArc) {
      const float dx = (float)x - cxf;
      const float d2 = arcR * arcR - dx * dx;
      const float edge = (d2 > 0.0f) ? (arcCY + sqrtf(d2)) : arcCY;
      int ty = (int)ceilf(edge);
      if (ty < y0) ty = y0;
      if (ty < y0 + h) g.drawFastVLine(x, ty, y0 + h - ty, DB_BLACK);
    }
  }

  // Highlight glint, skipped when the eye is squinting or curved away.
  if (cur_.sparkle > 0.35f && cur_.arc < 0.25f && h >= 14 && w >= 14) {
    int sw = w / 5; if (sw < 3) sw = 3;
    int sh = h / 5; if (sh < 3) sh = 3;
    const int sx = innerIsRight ? (x0 + w / 6) : (x0 + w - w / 6 - sw);
    const int sy = y0 + (int)lroundf(lidTopPx) + h / 7;
    if (sy + sh < y0 + h - 2) g.fillRoundRect(sx, sy, sw, sh, 1, DB_BLACK);
  }
}

void Face::draw(Canvas& g) {
  g.fillScreen(DB_BLACK);

  // Very slow wander so a face left running all day doesn't etch itself into
  // the panel. Two incommensurate periods => it never repeats exactly.
  float dx = 0.0f, dy = 0.0f;
  if (drift_) {
    dx = sinf(t_ * 0.0647f) * 3.0f;
    dy = cosf(t_ * 0.0413f) * 2.0f;
  }

  float shakeX = 0.0f, shakeY = 0.0f;
  if (shakeT_ >= 0.0f) {
    shakeX = damped(shakeT_, 52.0f, 9.0f) * 5.0f * shakeAmt_;
    shakeY = damped(shakeT_, 41.0f, 11.0f) * 2.5f * shakeAmt_;
  }

  const float bob = sinf(breathe_) * cur_.bob;
  const float baseX = DB_SCREEN_W * 0.5f + gazeX_ * 10.0f + dx + shakeX;
  const float baseY = DB_SCREEN_H * 0.5f + cur_.offY + gazeY_ * 7.0f + bob + dy + shakeY;
  const float half = cur_.spacing * 0.5f;

  const float lx = baseX - half, ly = baseY + cur_.tilt;
  const float rx = baseX + half, ry = baseY - cur_.tilt;

  const float closedL = clampf(blinkAmt_ + winkL_, 0.0f, 1.0f);
  const float closedR = clampf(blinkAmt_ + winkR_, 0.0f, 1.0f);
  const bool lidsDown = (blinkAmt_ > 0.15f);

  if (cur_.style == STYLE_HEART && !lidsDown) {
    const float beat = 1.0f + sinf(beat_) * 0.09f;
    const int hw = (int)lroundf(cur_.w * beat);
    const int hh = (int)lroundf(cur_.h * beat);
    drawHeart(g, (int)lroundf(lx), (int)lroundf(ly), hw, hh, DB_WHITE);
    drawHeart(g, (int)lroundf(rx), (int)lroundf(ry), hw, hh, DB_WHITE);
  } else if (cur_.style == STYLE_SPIRAL && !lidsDown) {
    drawSpiral(g, (int)lroundf(lx), (int)lroundf(ly), cur_.w * 0.5f, spin_, DB_WHITE);
    drawSpiral(g, (int)lroundf(rx), (int)lroundf(ry), cur_.w * 0.5f, -spin_, DB_WHITE);
  } else {
    drawEye(g, lx, ly, cur_.scaleL, closedL, true);
    drawEye(g, rx, ry, cur_.scaleR, closedR, false);
  }

  fx_.draw(g);
}

}  // namespace db

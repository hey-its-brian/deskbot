// ---------------------------------------------------------------------------
//  Face.h - the animated eyes.
//
//  Every expression is a `Pose`: a set of numbers describing the eye
//  rectangles and the lids that cut into them. Changing emotion just changes
//  the target pose; the renderer eases the live pose towards it every frame,
//  so expressions always melt into each other instead of snapping.
//
//  Arduino-free by design - see tools/host for the desktop preview.
// ---------------------------------------------------------------------------
#pragma once

#include "Easing.h"
#include "Effects.h"
#include "Gfx.h"

namespace db {

enum Emotion : uint8_t {
  EMOTION_NEUTRAL = 0,
  EMOTION_HAPPY,
  EMOTION_EXCITED,
  EMOTION_SAD,
  EMOTION_ANGRY,
  EMOTION_SURPRISED,
  EMOTION_SLEEPY,
  EMOTION_LOVE,
  EMOTION_CURIOUS,
  EMOTION_SUSPICIOUS,
  EMOTION_DIZZY,
  EMOTION_BORED,
  EMOTION_COUNT
};

// How the eye itself is drawn. Rectangles cover almost everything; hearts and
// spirals are the two novelty shapes.
enum EyeStyle : uint8_t { STYLE_EYES = 0, STYLE_HEART, STYLE_SPIRAL };

struct Pose {
  float w, h;        // eye size in pixels
  float radius;      // corner rounding
  float spacing;     // centre-to-centre distance
  float lidTop;      // 0..1 of the eye covered from the top
  float lidBot;      // 0..1 covered from the bottom
  float slant;       // px: >0 drops the inner corner (angry), <0 the outer (sad)
  float arc;         // 0..1 dome cut out of the bottom (the "happy" curve)
  float offY;        // whole-face vertical offset
  float tilt;        // px: left eye down / right eye up, reads as a head tilt
  float scaleL;      // per-eye size multiplier (asymmetry = curiosity)
  float scaleR;
  float gazeX;       // resting gaze, -1..1
  float gazeY;
  float sparkle;     // 0..1 highlight glint
  float bob;         // idle breathing amplitude in px
  EyeStyle style;
};

const char* emotionName(Emotion e);
bool emotionFromName(const char* name, Emotion& out);
void poseFor(Emotion e, Pose& p);

class Face {
 public:
  void begin(uint32_t seed);

  // Expressions -------------------------------------------------------------
  void setEmotion(Emotion e, bool immediate = false);
  Emotion emotion() const { return emotion_; }
  // Show `e` for `holdSeconds`, then fall back to `next`.
  void flash(Emotion e, float holdSeconds, Emotion next);

  // Gestures ----------------------------------------------------------------
  void blink();
  void wink(bool leftEye);
  void look(float x, float y, float holdSeconds = 1.2f);  // -1..1 each axis
  void jolt(float strength = 1.0f);                       // startle shake
  void setAutoGaze(bool on) { autoGaze_ = on; }
  void setAutoBlink(bool on) { autoBlink_ = on; }

  Effects& fx() { return fx_; }

  // Frame -------------------------------------------------------------------
  void update(float dt);
  void draw(Canvas& g);

  // Screen-saver style wander, keeps a static face off one set of pixels.
  void setBurnInDrift(bool on) { drift_ = on; }

 private:
  void applyPose(Emotion e, bool immediate);
  void spawnMoodEffects(float dt);
  void drawEye(Canvas& g, float cx, float cy, float scale, float closed,
               bool innerIsRight) const;

  Pose cur_{}, tgt_{};
  Emotion emotion_ = EMOTION_NEUTRAL;
  Emotion pending_ = EMOTION_NEUTRAL;
  bool hasPending_ = false;

  // flash()
  Emotion afterHold_ = EMOTION_NEUTRAL;
  float holdT_ = 0.0f;
  bool holding_ = false;

  // blink state machine
  uint8_t blinkPhase_ = 0;  // 0 idle, 1 closing, 2 opening
  float blinkT_ = 0.0f;
  float blinkAmt_ = 0.0f;   // 0 open .. 1 shut
  float nextBlink_ = 2.0f;
  bool blinkQueued_ = false;  // an expression change is waiting on a blink
  float winkL_ = 0.0f, winkR_ = 0.0f;  // extra lid for a one-eyed blink
  float winkT_ = -1.0f;
  int8_t winkEye_ = -1;

  // gaze
  float gazeX_ = 0.0f, gazeY_ = 0.0f;
  float gazeTX_ = 0.0f, gazeTY_ = 0.0f;
  float nextSaccade_ = 1.0f;
  float gazeHold_ = 0.0f;
  bool autoGaze_ = true;
  bool autoBlink_ = true;

  // flourishes
  float shakeT_ = -1.0f, shakeAmt_ = 0.0f;
  float spin_ = 0.0f;      // spiral phase
  float beat_ = 0.0f;      // heart beat phase
  float breathe_ = 0.0f;   // idle bob phase
  float t_ = 0.0f;         // seconds since boot
  float fxTimer_ = 0.0f;
  bool drift_ = true;

  Rng rng_;
  Effects fx_;
};

}  // namespace db

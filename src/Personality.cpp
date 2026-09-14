#include "Personality.h"

#include "config.h"

namespace db {
namespace {

struct MoodWeight {
  Emotion e;
  uint8_t w;
};

// Weights add up to 100. Mostly calm, with the occasional bit of drama -
// a buddy that pulls a face every two seconds gets annoying fast.
const MoodWeight kMoods[] = {
    {EMOTION_NEUTRAL, 30}, {EMOTION_HAPPY, 16},   {EMOTION_CURIOUS, 12},
    {EMOTION_BORED, 8},    {EMOTION_EXCITED, 6},  {EMOTION_SLEEPY, 5},
    {EMOTION_SUSPICIOUS, 5}, {EMOTION_SURPRISED, 4}, {EMOTION_SAD, 4},
    {EMOTION_LOVE, 4},     {EMOTION_ANGRY, 3},    {EMOTION_DIZZY, 3},
};
const int kMoodCount = (int)(sizeof(kMoods) / sizeof(kMoods[0]));

}  // namespace

void Personality::setIdleTimes(float boredS, float sleepS) {
  boredS_ = boredS < 5.0f ? 5.0f : boredS;
  sleepS_ = sleepS < boredS_ ? boredS_ : sleepS;
}

void Personality::begin(Face& face, uint32_t seed) {
  face_ = &face;
  boredS_ = DB_IDLE_BORED_S;
  sleepS_ = DB_IDLE_SLEEP_S;
  rng_.seed(seed ^ 0xA5A5F00Du);
  idle_ = 0.0f;
  moodT_ = rng_.range(2.0f, 5.0f);
  sleepT_ = rng_.range(8.0f, 20.0f);
  winkT_ = rng_.range(10.0f, 25.0f);
}

Emotion Personality::pickMood() {
  // Ignored for a while? Let it show.
  if (idle_ > boredS_ && rng_.chance(0.6f)) {
    return rng_.chance(0.5f) ? EMOTION_BORED : EMOTION_SLEEPY;
  }
  // Lots of recent attention makes the happy end of the table more likely.
  if (energy_ > 0.8f && rng_.chance(0.35f)) {
    return rng_.chance(0.5f) ? EMOTION_HAPPY : EMOTION_EXCITED;
  }
  int roll = rng_.irange(0, 99);
  for (int i = 0; i < kMoodCount; ++i) {
    if (roll < kMoods[i].w) return kMoods[i].e;
    roll -= kMoods[i].w;
  }
  return EMOTION_NEUTRAL;
}

void Personality::sleep() {
  if (!face_ || asleep_) return;
  asleep_ = true;
  face_->setAutoGaze(false);
  face_->look(0.0f, 0.25f, 1.0e6f);
  face_->setEmotion(EMOTION_SLEEPY);
  sleepT_ = rng_.range(8.0f, 20.0f);
}

void Personality::wake(bool startled) {
  if (!face_) return;
  idle_ = 0.0f;
  if (!asleep_) return;
  asleep_ = false;
  face_->setAutoGaze(true);
  face_->look(0.0f, 0.0f, 1.0f);
  if (startled) {
    face_->jolt(1.0f);
    face_->flash(EMOTION_SURPRISED, 0.9f, EMOTION_HAPPY);
    moodT_ = 3.5f;
  } else {
    face_->setEmotion(EMOTION_NEUTRAL);
    moodT_ = 2.5f;
  }
}

void Personality::update(float dt) {
  if (!face_) return;
  if (idle_ < 1.0e6f) idle_ += dt;  // only ever compared against thresholds

  // Attention fades slowly.
  energy_ = approach(energy_, idle_ > boredS_ ? 0.25f : 0.55f, 30.0f, dt);

  if (asleep_) {
    sleepT_ -= dt;
    if (sleepT_ <= 0.0f) {          // a small twitch, so it still looks alive
      sleepT_ = rng_.range(9.0f, 22.0f);
      face_->jolt(0.22f);
    }
    return;
  }

  tapAgo_ += dt;
  if (petting_) return;      // undivided attention while being petted
  if (!autoMood_) return;

  // The odd wink when it's in a good mood.
  winkT_ -= dt;
  if (winkT_ <= 0.0f) {
    winkT_ = rng_.range(14.0f, 35.0f);
    const Emotion e = face_->emotion();
    if (e == EMOTION_HAPPY || e == EMOTION_EXCITED || e == EMOTION_NEUTRAL) {
      face_->wink(rng_.chance(0.5f));
    }
  }

  moodT_ -= dt;
  if (moodT_ <= 0.0f) {
    const Emotion e = pickMood();
    face_->setEmotion(e);
    moodT_ = (e == EMOTION_NEUTRAL) ? rng_.range(4.0f, 9.0f)
                                    : rng_.range(2.5f, 6.0f);
  }

  if (idle_ > sleepS_) sleep();
}

// ---------------------------------------------------------------------------
//  Touch pad
// ---------------------------------------------------------------------------
void Personality::setTouch(bool down, float dt) {
  if (!face_) return;

  if (down) {
    if (!touchDown_) {              // finger just landed
      touchDown_ = true;
      petT_ = 0.0f;
      idle_ = 0.0f;
      if (asleep_) wake(false);     // a pat is a gentle wake, unlike a poke
    }
    petT_ += dt;
    if (!petting_ && petT_ >= DB_PET_HOLD_S) beginPet();
    if (petting_) {
      heartT_ -= dt;
      if (heartT_ <= 0.0f) {
        heartT_ = rng_.range(0.35f, 0.6f);
        face_->fx().spawn(FX_HEART, rng_.range(28.0f, 100.0f), 15.0f,
                          rng_.range(-4.0f, 4.0f), -5.5f, 2.0f,
                          rng_.range(3.0f, 4.0f));
      }
      // Keep it up long enough and it's smitten.
      if (petT_ >= 2.8f && face_->emotion() != EMOTION_LOVE) {
        face_->setEmotion(EMOTION_LOVE);
      }
    }
  } else if (touchDown_) {          // finger lifted
    touchDown_ = false;
    if (petting_) endPet();
    else onTap();
  }
}

void Personality::onTap() {
  energy_ = clampf(energy_ + 0.15f, 0.0f, 1.0f);
  if (tapAgo_ < 0.4f) {             // second tap in quick succession: playful
    tapAgo_ = 99.0f;
    face_->jolt(0.4f);
    face_->flash(EMOTION_EXCITED, 2.5f, EMOTION_HAPPY);
    moodT_ = 5.0f;
    return;
  }
  tapAgo_ = 0.0f;
  face_->look(0.0f, 0.0f, 1.6f);    // "oh, hi": notices you, brightens up
  face_->flash(EMOTION_HAPPY, 2.2f, EMOTION_NEUTRAL);
  moodT_ = 3.0f;
}

void Personality::beginPet() {
  petting_ = true;
  heartT_ = 0.2f;
  energy_ = clampf(energy_ + 0.3f, 0.0f, 1.0f);
  face_->setAutoGaze(false);
  face_->look(0.0f, -0.35f, 1.0e6f);  // eyes up towards the hand
  face_->setEmotion(EMOTION_HAPPY);
}

void Personality::endPet() {
  petting_ = false;
  face_->setAutoGaze(true);
  face_->look(0.0f, 0.0f, 1.2f);
  face_->flash(EMOTION_HAPPY, 4.0f, EMOTION_NEUTRAL);  // contented afterglow
  moodT_ = 6.0f;
}

void Personality::onInteraction(Interaction what) {
  if (!face_) return;
  idle_ = 0.0f;
  energy_ = clampf(energy_ + 0.2f, 0.0f, 1.0f);

  switch (what) {
    case TOUCH_POKE:
      if (asleep_) { wake(true); return; }
      face_->jolt(0.8f);
      face_->flash(EMOTION_SURPRISED, 0.7f, EMOTION_HAPPY);
      moodT_ = 3.6f;
      break;

    case TOUCH_DOUBLE:
      if (asleep_) wake(false);
      face_->flash(EMOTION_LOVE, 3.0f, EMOTION_HAPPY);
      moodT_ = 5.5f;
      break;

    case TOUCH_HOLD:
      if (asleep_) wake(false); else sleep();
      break;
  }
}

}  // namespace db

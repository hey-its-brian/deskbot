// ---------------------------------------------------------------------------
//  Personality.h - what the buddy does when nobody is telling it anything.
//
//  Picks new moods on its own, gets bored, falls asleep if ignored, and
//  reacts to being poked. Arduino-free, like the face.
// ---------------------------------------------------------------------------
#pragma once

#include "Face.h"

namespace db {

enum Interaction : uint8_t {
  TOUCH_POKE = 0,   // single press: startle, then cheer up
  TOUCH_DOUBLE,     // double press: affection
  TOUCH_HOLD        // long press: sleep / wake
};

class Personality {
 public:
  void begin(Face& face, uint32_t seed);
  void update(float dt);
  void onInteraction(Interaction what);

  void setAutoMood(bool on) { autoMood_ = on; }
  bool autoMood() const { return autoMood_; }

  void sleep();
  void wake(bool startled = false);
  bool asleep() const { return asleep_; }

  float idleSeconds() const { return idle_; }
  // Rises with attention, decays when ignored. Drives how bouncy it feels.
  float energy() const { return energy_; }

 private:
  Emotion pickMood();

  Face* face_ = nullptr;
  Rng rng_;
  bool autoMood_ = true;
  bool asleep_ = false;
  float idle_ = 0.0f;
  float moodT_ = 3.0f;
  float energy_ = 0.6f;
  float sleepT_ = 10.0f;
  float winkT_ = 12.0f;
};

}  // namespace db

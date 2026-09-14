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

  // Capacitive pad. Call every frame with the current state: a short touch is
  // a tap (it says hello), a held one is petting (it melts).
  void setTouch(bool down, float dt);
  bool petting() const { return petting_; }

  void setAutoMood(bool on) { autoMood_ = on; }
  bool autoMood() const { return autoMood_; }

  void sleep();
  void wake(bool startled = false);
  bool asleep() const { return asleep_; }

  float idleSeconds() const { return idle_; }

  // Runtime-adjustable copies of DB_IDLE_BORED_S / DB_IDLE_SLEEP_S.
  void setIdleTimes(float boredS, float sleepS);
  float boredSeconds() const { return boredS_; }
  float sleepSeconds() const { return sleepS_; }

  // Keep the current expression at least this long before the mood engine
  // picks a new one - used while a weather glance is on screen.
  void holdMood(float seconds) { if (moodT_ < seconds) moodT_ = seconds; }
  // Rises with attention, decays when ignored. Drives how bouncy it feels.
  float energy() const { return energy_; }

 private:
  Emotion pickMood();
  void onTap();
  void beginPet();
  void endPet();

  Face* face_ = nullptr;
  Rng rng_;
  bool autoMood_ = true;
  bool asleep_ = false;
  float idle_ = 0.0f;
  float moodT_ = 3.0f;
  float energy_ = 0.6f;
  float sleepT_ = 10.0f;
  float boredS_ = 90.0f;
  float sleepS_ = 300.0f;
  float winkT_ = 12.0f;

  bool touchDown_ = false;
  bool petting_ = false;
  float petT_ = 0.0f;
  float heartT_ = 0.0f;
  float tapAgo_ = 99.0f;   // seconds since the last tap, for double-tap
};

}  // namespace db

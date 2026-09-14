// ---------------------------------------------------------------------------
//  preview.cpp - renders the real src/Face.cpp on a PC so you can iterate on
//  expressions without reflashing the board.
//
//    make preview                 # build + render everything into preview/
//    ./preview sheet   out.bin    # one settled frame per emotion
//    ./preview arc <name> out.bin # neutral -> <name> transition, 45 frames
//    ./preview anim <sec> out.bin # the autonomous personality, 30 fps
//
//  Frames are dumped raw (128*64 bytes, one byte per pixel) and turned into
//  PNGs by tools/host/frames_to_png.py.
// ---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Face.h"
#include "Personality.h"

using namespace db;

static const float kDt = 1.0f / 30.0f;

static void emit(FILE* f, const HostGfx& g) {
  fwrite(g.px, 1, sizeof(g.px), f);
}

static void step(Face& face, HostGfx& g, int frames, FILE* out, bool capture) {
  for (int i = 0; i < frames; ++i) {
    face.update(kDt);
    if (capture) { face.draw(g); emit(out, g); }
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: preview <sheet | arc <name> | anim <sec> | poke <n> | pet <n> | doze <n> | weather <kind> <n>> <out.bin>\n");
    return 2;
  }
  const char* mode = argv[1];
  HostGfx g;
  int frames = 0;

  if (strcmp(mode, "sheet") == 0) {
    FILE* out = fopen(argv[2], "wb");
    if (!out) return 1;
    for (uint8_t e = 0; e < EMOTION_COUNT; ++e) {
      Face face;
      face.begin(0xC0FFEEu + e);
      face.setAutoBlink(false);
      face.setAutoGaze(false);
      face.setBurnInDrift(false);
      face.setEmotion((Emotion)e, true);
      step(face, g, 24, out, false);   // let effects and the idle bob settle
      face.draw(g);
      emit(out, g);
      ++frames;
    }
    fclose(out);
  } else if (strcmp(mode, "arc") == 0 && argc >= 4) {
    Emotion target;
    if (!emotionFromName(argv[2], target)) {
      fprintf(stderr, "unknown emotion '%s'\n", argv[2]);
      return 2;
    }
    FILE* out = fopen(argv[3], "wb");
    if (!out) return 1;
    Face face;
    face.begin(0x1234u);
    face.setAutoBlink(false);
    face.setAutoGaze(false);
    face.setBurnInDrift(false);
    face.setEmotion(EMOTION_NEUTRAL, true);
    step(face, g, 6, out, true);
    face.setEmotion(target);           // blink-and-change, as on hardware
    step(face, g, 39, out, true);
    frames = 45;
    fclose(out);
  } else if (strcmp(mode, "anim") == 0 && argc >= 4) {
    // argv is untrusted enough: a huge or NaN duration makes the cast to int
    // below undefined, so pin it to something a preview could plausibly want.
    const float wanted = (float)atof(argv[2]);
    const float seconds = (wanted > 0.0f) ? ((wanted < 3600.0f) ? wanted : 3600.0f) : 0.0f;
    FILE* out = fopen(argv[3], "wb");
    if (!out) return 1;
    Face face;
    Personality brain;
    face.begin(0xBEEFu);
    brain.begin(face, 0xBEEFu);
    const int n = (int)(seconds * 30.0f);
    for (int i = 0; i < n; ++i) {
      brain.update(kDt);
      face.update(kDt);
      face.draw(g);
      emit(out, g);
    }
    frames = n;
    fclose(out);
  } else if (strcmp(mode, "poke") == 0 && argc >= 4) {
    // Idle for a moment, get poked, react.
    const int n = atoi(argv[2]);
    FILE* out = fopen(argv[3], "wb");
    if (!out) return 1;
    Face face;
    Personality brain;
    face.begin(0x51EEDu);
    brain.begin(face, 0x51EEDu);
    for (int i = 0; i < n; ++i) {
      if (i == 12) brain.onInteraction(TOUCH_POKE);
      brain.update(kDt);
      face.update(kDt);
      face.draw(g);
      emit(out, g);
    }
    frames = n;
    fclose(out);
  } else if (strcmp(mode, "pet") == 0 && argc >= 4) {
    // Finger lands on the pad at frame 12 and stays 3.5 s: long enough to
    // pass the 2.8 s heart-eyes threshold and let the blink reveal them.
    const int n = atoi(argv[2]);
    FILE* out = fopen(argv[3], "wb");
    if (!out) return 1;
    Face face;
    Personality brain;
    face.begin(0x9E7u);
    brain.begin(face, 0x9E7u);
    for (int i = 0; i < n; ++i) {
      brain.setTouch(i >= 12 && i < 12 + 105, kDt);
      brain.update(kDt);
      face.update(kDt);
      face.draw(g);
      emit(out, g);
    }
    frames = n;
    fclose(out);
  } else if (strcmp(mode, "weather") == 0 && argc >= 5) {
    // weather <kind> <frames> out.bin - a glance at the given conditions
    // (append "night" to the kind for the after-dark version: "clear-night").
    char kindName[24];
    strncpy(kindName, argv[2], sizeof(kindName) - 1);
    kindName[sizeof(kindName) - 1] = 0;
    bool day = true;
    char* dash = strchr(kindName, '-');
    if (dash) { *dash = 0; day = false; }
    WeatherKind kind;
    if (!weatherFromName(kindName, kind)) {
      fprintf(stderr, "unknown weather '%s'\n", argv[2]);
      return 2;
    }
    const int n = atoi(argv[3]);
    FILE* out = fopen(argv[4], "wb");
    if (!out) return 1;
    Face face;
    Personality brain;
    face.begin(0x5EA7u);
    brain.begin(face, 0x5EA7u);
    for (int i = 0; i < n; ++i) {
      if (i == 8) {
        face.showWeather(kind, day, true, day ? 72 : 48, 'F', (float)(n - 8) / 30.0f);
        brain.holdMood(10.0f);
      }
      brain.update(kDt);
      face.update(kDt);
      face.draw(g);
      emit(out, g);
    }
    frames = n;
    fclose(out);
  } else if (strcmp(mode, "doze") == 0 && argc >= 4) {
    // Nodding off, with the Z's.
    const int n = atoi(argv[2]);
    FILE* out = fopen(argv[3], "wb");
    if (!out) return 1;
    Face face;
    Personality brain;
    face.begin(0xD02Eu);
    brain.begin(face, 0xD02Eu);
    for (int i = 0; i < n; ++i) {
      if (i == 6) brain.sleep();
      brain.update(kDt);
      face.update(kDt);
      face.draw(g);
      emit(out, g);
    }
    frames = n;
    fclose(out);
  } else {
    fprintf(stderr, "bad arguments\n");
    return 2;
  }

  printf("%d\n", frames);  // frame count, for the PNG tool
  return 0;
}

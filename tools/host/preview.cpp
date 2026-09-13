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
    fprintf(stderr, "usage: preview <sheet | arc <name> | anim <sec> | poke <n> | doze <n>> <out.bin>\n");
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
    const float seconds = (float)atof(argv[2]);
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

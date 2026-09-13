// ---------------------------------------------------------------------------
//  Shapes.h - little vector doodles shared by the face and the effects layer.
// ---------------------------------------------------------------------------
#pragma once

#include "Gfx.h"

namespace db {

void drawHeart(Canvas& g, int cx, int cy, int w, int h, uint16_t colour);
void drawSpiral(Canvas& g, int cx, int cy, float radius, float phase, uint16_t colour);
void drawZ(Canvas& g, int x, int y, int size, uint16_t colour);
void drawExclaim(Canvas& g, int cx, int cy, int h, uint16_t colour);
void drawDroplet(Canvas& g, int cx, int cy, int size, uint16_t colour);
void drawStar(Canvas& g, int cx, int cy, int size, uint16_t colour);
void drawNote(Canvas& g, int cx, int cy, int size, uint16_t colour);

}  // namespace db

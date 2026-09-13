#include "Shapes.h"

#include <math.h>

namespace db {

void drawHeart(Canvas& g, int cx, int cy, int w, int h, uint16_t colour) {
  if (w < 3 || h < 3) {
    g.fillRect(cx - 1, cy - 1, 2, 2, colour);
    return;
  }
  const int lobeR = (w + 2) / 4;              // radius of the two top lobes
  const int lobeY = cy - h / 5;
  g.fillCircle(cx - lobeR, lobeY, lobeR, colour);
  g.fillCircle(cx + lobeR, lobeY, lobeR, colour);
  // The V below the lobes. Starting it level with the lobe centres keeps the
  // cleft at the top of the heart instead of filling it in.
  g.fillTriangle(cx - 2 * lobeR, lobeY,
                 cx + 2 * lobeR, lobeY,
                 cx, cy + h / 2, colour);
}

void drawSpiral(Canvas& g, int cx, int cy, float radius, float phase, uint16_t colour) {
  if (radius < 2.0f) return;
  const int steps = 110;
  const float turns = 2.6f;
  for (int i = 0; i <= steps; ++i) {
    const float t = (float)i / (float)steps;
    const float a = phase + t * turns * 6.28318f;
    const float r = t * radius;
    const int x = cx + (int)lroundf(cosf(a) * r);
    const int y = cy + (int)lroundf(sinf(a) * r);
    g.fillRect(x, y, 2, 2, colour);  // 2px wide so it survives a 1-bit panel
  }
}

void drawZ(Canvas& g, int x, int y, int size, uint16_t colour) {
  if (size < 3) size = 3;
  const int s = size;
  g.drawLine(x, y, x + s, y, colour);              // top bar
  g.drawLine(x + s, y, x, y + s, colour);          // diagonal
  g.drawLine(x, y + s, x + s, y + s, colour);      // bottom bar
  if (s >= 6) {  // embolden
    g.drawLine(x, y + 1, x + s, y + 1, colour);
    g.drawLine(x, y + s - 1, x + s, y + s - 1, colour);
  }
}

void drawExclaim(Canvas& g, int cx, int cy, int h, uint16_t colour) {
  if (h < 5) h = 5;
  const int barH = h - h / 4;
  g.fillRect(cx - 1, cy - h / 2, 2, barH, colour);
  g.fillRect(cx - 1, cy - h / 2 + barH + 1, 2, 2, colour);
}

void drawDroplet(Canvas& g, int cx, int cy, int size, uint16_t colour) {
  if (size < 2) size = 2;
  g.fillCircle(cx, cy, size, colour);
  g.fillTriangle(cx - size, cy - 1, cx + size, cy - 1, cx, cy - size * 2, colour);
}

void drawStar(Canvas& g, int cx, int cy, int size, uint16_t colour) {
  if (size < 1) size = 1;
  g.drawFastVLine(cx, cy - size, size * 2 + 1, colour);
  g.drawFastHLine(cx - size, cy, size * 2 + 1, colour);
  if (size >= 3) {
    g.drawPixel(cx - 1, cy - 1, colour);
    g.drawPixel(cx + 1, cy - 1, colour);
    g.drawPixel(cx - 1, cy + 1, colour);
    g.drawPixel(cx + 1, cy + 1, colour);
  }
}

void drawNote(Canvas& g, int cx, int cy, int size, uint16_t colour) {
  if (size < 3) size = 3;
  g.fillCircle(cx - size / 2, cy + size / 2, (size + 1) / 3, colour);
  g.fillRect(cx - size / 2 + (size + 1) / 3, cy - size, 2, size + size / 2, colour);
  g.fillRect(cx - size / 2 + (size + 1) / 3, cy - size, size / 2 + 1, 2, colour);
}

}  // namespace db

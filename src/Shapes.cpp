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

void drawSun(Canvas& g, int cx, int cy, int r, uint16_t colour) {
  if (r < 2) r = 2;
  g.fillCircle(cx, cy, r, colour);
  const int inner = r + 2, outer = r + 4;
  // Eight rays: the four axes and the four diagonals (scaled by ~0.7).
  g.drawFastVLine(cx, cy - outer, outer - inner + 1, colour);
  g.drawFastVLine(cx, cy + inner, outer - inner + 1, colour);
  g.drawFastHLine(cx - outer, cy, outer - inner + 1, colour);
  g.drawFastHLine(cx + inner, cy, outer - inner + 1, colour);
  const int di = (inner * 7) / 10, dO = (outer * 7) / 10;
  g.drawLine(cx - dO, cy - dO, cx - di, cy - di, colour);
  g.drawLine(cx + dO, cy - dO, cx + di, cy - di, colour);
  g.drawLine(cx - dO, cy + dO, cx - di, cy + di, colour);
  g.drawLine(cx + dO, cy + dO, cx + di, cy + di, colour);
}

void drawMoon(Canvas& g, int cx, int cy, int r, uint16_t colour) {
  if (r < 3) r = 3;
  g.fillCircle(cx, cy, r, colour);
  // Bite a second disc out of the upper-right to leave a crescent.
  g.fillCircle(cx + r / 2, cy - r / 3, r - 1, colour ? DB_BLACK : DB_WHITE);
}

void drawCloud(Canvas& g, int cx, int cy, int w, uint16_t colour) {
  if (w < 8) w = 8;
  const int r1 = w / 5, r2 = w / 4;
  g.fillCircle(cx - w / 4, cy, r1, colour);
  g.fillCircle(cx + w / 4, cy, r1, colour);
  g.fillCircle(cx, cy - w / 8, r2, colour);
  g.fillRect(cx - w / 4, cy, w / 2 + 1, r1 + 1, colour);
}

void drawBolt(Canvas& g, int x, int y, int h, uint16_t colour) {
  if (h < 6) h = 6;
  const int mid = y + h / 2;
  for (int t = 0; t < 2; ++t) {  // two passes = 2 px thick
    g.drawLine(x + t, y, x - 3 + t, mid, colour);
    g.drawLine(x - 3 + t, mid, x + 1 + t, mid, colour);
    g.drawLine(x + 1 + t, mid, x - 2 + t, y + h, colour);
  }
}

namespace {

// Each glyph is five rows of three bits, top row first.
struct Glyph { char c; uint8_t rows[5]; };
const Glyph kTiny[] = {
    {'0', {7, 5, 5, 5, 7}}, {'1', {2, 6, 2, 2, 7}}, {'2', {7, 1, 7, 4, 7}},
    {'3', {7, 1, 7, 1, 7}}, {'4', {5, 5, 7, 1, 1}}, {'5', {7, 4, 7, 1, 7}},
    {'6', {7, 4, 7, 5, 7}}, {'7', {7, 1, 1, 1, 1}}, {'8', {7, 5, 7, 5, 7}},
    {'9', {7, 5, 7, 1, 7}}, {'-', {0, 0, 7, 0, 0}}, {'*', {2, 5, 2, 0, 0}},
    {'C', {7, 4, 4, 4, 7}}, {'F', {7, 4, 7, 4, 4}}, {' ', {0, 0, 0, 0, 0}},
};

const Glyph* tinyGlyph(char c) {
  for (unsigned i = 0; i < sizeof(kTiny) / sizeof(kTiny[0]); ++i)
    if (kTiny[i].c == c) return &kTiny[i];
  return nullptr;
}

}  // namespace

int tinyTextWidth(const char* text, int scale) {
  int n = 0;
  for (const char* p = text; *p; ++p) ++n;
  return n > 0 ? (n * 4 - 1) * scale : 0;
}

int drawTinyText(Canvas& g, int x, int y, const char* text, int scale, uint16_t colour) {
  if (scale < 1) scale = 1;
  const int x0 = x;
  for (const char* p = text; *p; ++p) {
    const Glyph* gl = tinyGlyph(*p);
    if (gl) {
      for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
          if (gl->rows[row] & (4 >> col)) {
            g.fillRect(x + col * scale, y + row * scale, scale, scale, colour);
          }
        }
      }
    }
    x += 4 * scale;
  }
  return x - x0 - scale;
}

}  // namespace db

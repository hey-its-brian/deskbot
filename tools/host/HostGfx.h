// ---------------------------------------------------------------------------
//  HostGfx.h - a stand-in for Adafruit_GFX that rasterises into a plain
//  128x64 byte buffer, so src/Face.cpp can be compiled and rendered on a PC.
//
//  Only the primitives the face actually uses are implemented, and they are
//  pixel-approximations of Adafruit's - close enough to judge a design, not a
//  bit-exact emulator.
// ---------------------------------------------------------------------------
#pragma once

#include <math.h>
#include <stdint.h>
#include <string.h>

namespace db {

class HostGfx {
 public:
  static const int W = 128;
  static const int H = 64;
  uint8_t px[W * H];

  HostGfx() { memset(px, 0, sizeof(px)); }

  int16_t width() const { return W; }
  int16_t height() const { return H; }

  void drawPixel(int x, int y, uint16_t c) {
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    px[y * W + x] = (uint8_t)(c ? 1 : 0);
  }

  void fillScreen(uint16_t c) { memset(px, c ? 1 : 0, sizeof(px)); }

  void drawFastHLine(int x, int y, int len, uint16_t c) {
    for (int i = 0; i < len; ++i) drawPixel(x + i, y, c);
  }
  void drawFastVLine(int x, int y, int len, uint16_t c) {
    for (int i = 0; i < len; ++i) drawPixel(x, y + i, c);
  }
  void fillRect(int x, int y, int w, int h, uint16_t c) {
    for (int j = 0; j < h; ++j) drawFastHLine(x, y + j, w, c);
  }
  void drawRect(int x, int y, int w, int h, uint16_t c) {
    drawFastHLine(x, y, w, c);
    drawFastHLine(x, y + h - 1, w, c);
    drawFastVLine(x, y, h, c);
    drawFastVLine(x + w - 1, y, h, c);
  }

  void fillCircle(int cx, int cy, int r, uint16_t c) {
    if (r < 0) return;
    for (int dy = -r; dy <= r; ++dy) {
      const int dx = (int)floor(sqrt((double)(r * r - dy * dy)));
      drawFastHLine(cx - dx, cy + dy, 2 * dx + 1, c);
    }
  }
  void drawCircle(int cx, int cy, int r, uint16_t c) {
    for (int a = 0; a < 360; ++a) {
      const double t = a * 3.14159265 / 180.0;
      drawPixel(cx + (int)lround(cos(t) * r), cy + (int)lround(sin(t) * r), c);
    }
  }

  void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c) {
    if (w <= 0 || h <= 0) return;
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    if (r < 0) r = 0;
    fillRect(x + r, y, w - 2 * r, h, c);
    if (r > 0) {
      fillRect(x, y + r, r, h - 2 * r, c);
      fillRect(x + w - r, y + r, r, h - 2 * r, c);
      for (int dy = 0; dy <= r; ++dy) {
        const int dx = (int)floor(sqrt((double)(r * r - dy * dy)));
        drawFastHLine(x + r - dx, y + r - dy, dx, c);
        drawFastHLine(x + w - r, y + r - dy, dx, c);
        drawFastHLine(x + r - dx, y + h - r - 1 + dy, dx, c);
        drawFastHLine(x + w - r, y + h - r - 1 + dy, dx, c);
      }
    }
  }

  void drawLine(int x0, int y0, int x1, int y1, uint16_t c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
      drawPixel(x0, y0, c);
      if (x0 == x1 && y0 == y1) break;
      const int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c) {
    const int minX = min3(x0, x1, x2), maxX = max3(x0, x1, x2);
    const int minY = min3(y0, y1, y2), maxY = max3(y0, y1, y2);
    const double area = edge(x0, y0, x1, y1, x2, y2);
    if (fabs(area) < 1e-9) { drawLine(x0, y0, x1, y1, c); drawLine(x1, y1, x2, y2, c); return; }
    for (int y = minY; y <= maxY; ++y) {
      for (int x = minX; x <= maxX; ++x) {
        const double w0 = edge(x1, y1, x2, y2, x, y) / area;
        const double w1 = edge(x2, y2, x0, y0, x, y) / area;
        const double w2 = edge(x0, y0, x1, y1, x, y) / area;
        if (w0 >= -0.001 && w1 >= -0.001 && w2 >= -0.001) drawPixel(x, y, c);
      }
    }
  }

 private:
  static int abs(int v) { return v < 0 ? -v : v; }
  static int min3(int a, int b, int c) { int m = a < b ? a : b; return m < c ? m : c; }
  static int max3(int a, int b, int c) { int m = a > b ? a : b; return m > c ? m : c; }
  static double edge(int ax, int ay, int bx, int by, int cx, int cy) {
    return (double)(bx - ax) * (cy - ay) - (double)(by - ay) * (cx - ax);
  }
};

}  // namespace db

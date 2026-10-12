#pragma once

#include "platform.h"

static const int CHANNELS[7] = { 1, 0, 3, 1, 2, 0, 4 };
static const int ADAM7[7][4] = { { 0, 0, 8, 8 }, { 4, 0, 8, 8 }, { 0, 4, 4, 8 }, { 2, 0, 4, 4 }, { 0, 2, 2, 4 }, { 1, 0, 2, 2 }, { 0, 1, 1, 2 } };

static int paeth(int a, int b, int c) {
  int p = a + b - c;
  int pa = p > a ? p - a : a - p, pb = p > b ? p - b : b - p, pc = p > c ? p - c : c - p;
  return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

static void unfilter(int f, unsigned char *line, const unsigned char *prev, int stride, int bpp) {
  for (int i = 0; i < stride; i++) {
    int a = i >= bpp ? line[i - bpp] : 0;
    int b = prev ? prev[i] : 0;
    int c = i >= bpp && prev ? prev[i - bpp] : 0;
    if (f == 1) line[i] += a;
    else if (f == 2) line[i] += b;
    else if (f == 3) line[i] += (a + b) >> 1;
    else if (f == 4) line[i] += paeth(a, b, c);
  }
}

static int sample(const unsigned char *line, int i, int depth) {
  if (depth == 8) return line[i];
  if (depth == 16) return line[i * 2] << 8 | line[i * 2 + 1];
  int bit = i * depth;
  return line[bit >> 3] >> (8 - depth - (bit & 7)) & ((1 << depth) - 1);
}

typedef struct {
  int color, depth, n;
  double max;
  float unit[256];
  const unsigned char *palette, *trns;
  int trns_length;
} Png;

static void png_pixel(const Png *p, float *out, const unsigned char *line, int x) {
  if (p->color == 3) {
    int i = sample(line, x, p->depth);
    for (int k = 0; k < 3; k++) out[k] = p->palette[i * 3 + k] / 255.0;
    out[3] = p->trns && i < p->trns_length ? p->trns[i] / 255.0 : 1;
    return;
  }
  int gray = p->color == 0 || p->color == 4;
  int key = p->trns != 0;
  for (int k = 0; k < 3; k++) {
    int v = sample(line, x * p->n + (gray ? 0 : k), p->depth);
    out[k] = p->depth <= 8 ? p->unit[v] : v / p->max;
    if (key) key = v == (p->trns[gray ? 0 : k * 2] << 8 | p->trns[(gray ? 0 : k * 2) + 1]);
  }
  int a = p->color == 4 || p->color == 6 ? sample(line, x * p->n + p->n - 1, p->depth) : -1;
  out[3] = a < 0 ? !key : p->depth <= 8 ? p->unit[a] : a / p->max;
}

void tex_png_read(unsigned char *raw, int width, int height, int depth, int color, int interlace, const unsigned char *palette, const unsigned char *trns, int trns_length, float *out) {
  Png p = { color, depth, CHANNELS[color], (1 << depth) - 1, { 0 }, palette, trns, trns_length };
  for (int v = 0; v < 256; v++) p.unit[v] = v / p.max;
  int bpp = p.n * depth >> 3;
  if (bpp < 1) bpp = 1;
  int off = 0;
  for (int pass = 0; pass < (interlace ? 7 : 1); pass++) {
    int x0 = interlace ? ADAM7[pass][0] : 0, y0 = interlace ? ADAM7[pass][1] : 0;
    int dx = interlace ? ADAM7[pass][2] : 1, dy = interlace ? ADAM7[pass][3] : 1;
    int pw = (width - x0 + dx - 1) / dx, ph = (height - y0 + dy - 1) / dy;
    if (pw <= 0 || ph <= 0) continue;
    int stride = (pw * p.n * depth + 7) / 8;
    unsigned char *prev = 0;
    for (int y = 0; y < ph; y++) {
      unsigned char *line = raw + off + 1;
      unfilter(raw[off], line, prev, stride, bpp);
      off += 1 + stride;
      for (int x = 0; x < pw; x++) png_pixel(&p, out + ((y0 + y * dy) * width + x0 + x * dx) * 4, line, x);
      prev = line;
    }
  }
}

static int filter(int f, const unsigned char *cur, const unsigned char *prev, int i, int bpp) {
  int a = i >= bpp ? cur[i - bpp] : 0, b = prev[i], c = i >= bpp ? prev[i - bpp] : 0;
  int p = f == 1 ? a : f == 2 ? b : f == 3 ? (a + b) >> 1 : f == 4 ? paeth(a, b, c) : 0;
  return (cur[i] - p) & 255;
}

void tex_png_rows(const float *px, int w, int h, int n, int depth, int mode, unsigned char *out, unsigned char *scratch) {
  int size = depth / 8, stride = w * n * size, bpp = n * size;
  double max = depth == 16 ? 65535 : 255;
  unsigned char *prev = scratch, *cur = scratch + stride;
  for (int i = 0; i < stride; i++) prev[i] = 0;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      for (int k = 0; k < n; k++) {
        int v = (int)fround(clamp(px[(y * w + x) * 4 + k], 0, 1) * max), at = (x * n + k) * size;
        if (size == 2) cur[at] = v >> 8, cur[at + 1] = v & 255;
        else cur[at] = v;
      }
    }
    int best = mode < 0 ? 0 : mode;
    unsigned least = ~0u;
    for (int f = 0; f < 5 && mode < 0; f++) {
      unsigned sum = 0;
      for (int i = 0; i < stride && sum < least; i++) {
        int d = filter(f, cur, prev, i, bpp);
        sum += d < 128 ? d : 256 - d;
      }
      if (sum < least) least = sum, best = f;
    }
    unsigned char *row = out + y * (stride + 1);
    row[0] = best;
    for (int i = 0; i < stride; i++) row[i + 1] = filter(best, cur, prev, i, bpp);
    unsigned char *t = prev;
    prev = cur, cur = t;
  }
}

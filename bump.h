#pragma once

#include "platform.h"

static float bump_height(const float *px, int w, int h, int x, int y) {
  x = (x % w + w) % w, y = (y % h + h) % h;
  const float *p = px + (y * w + x) * 4;
  return 0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2];
}

void tex_bump_normal(const float *height, int w, int h, float scale, float *out) {
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      float nx = (bump_height(height, w, h, x - 1, y) - bump_height(height, w, h, x + 1, y)) * 0.5f * scale;
      float ny = (bump_height(height, w, h, x, y + 1) - bump_height(height, w, h, x, y - 1)) * 0.5f * scale;
      float len = fsqrtf(nx * nx + ny * ny + 1);
      float *o = out + (y * w + x) * 4;
      o[0] = nx / len * 0.5f + 0.5f, o[1] = ny / len * 0.5f + 0.5f, o[2] = 1 / len * 0.5f + 0.5f, o[3] = bump_height(height, w, h, x, y);
    }
  }
}

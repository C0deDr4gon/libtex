#pragma once

#include "platform.h"
#include "vec.h"

void tex_orient(const float *src, int w, int h, float *dst, int op) {
  int ow = op == 2 || op == 4 ? h : w, oh = op == 2 || op == 4 ? w : h;
  for (int y = 0; y < oh; y++) {
    for (int x = 0; x < ow; x++) {
      int sx = op == 0 || op == 3 ? w - 1 - x : op == 2 ? y : op == 4 ? w - 1 - y : x;
      int sy = op == 1 || op == 3 ? h - 1 - y : op == 2 ? h - 1 - x : op == 4 ? x : y;
      store(dst + (y * ow + x) * 4, load(src + (sy * w + sx) * 4));
    }
  }
}

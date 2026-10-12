#pragma once

#include "platform.h"
#include "vec.h"

void tex_swizzle(float *px, int count, const int *src, const float *scale, const float *offset) {
  unsigned char bytes[16];
  int fixed[4];
  for (int k = 0; k < 4; k++) {
    int s = src[k] < 0 ? 0 : src[k];
    for (int b = 0; b < 4; b++) bytes[k * 4 + b] = s * 4 + b;
    fixed[k] = src[k] < 0 ? -1 : 0;
  }
  ivec pick = iload(bytes), fix = iload(fixed);
  vec m = load(scale), o = load(offset);
  for (int i = 0; i < count; i++) {
    vec v = add(mul(vswizzle(load(px + i * 4), pick), m), o);
    store(px + i * 4, vselect(fix, o, v));
  }
}

#pragma once

#include "platform.h"
#include "vec.h"

void tex_premultiply(float *px, int count, int inverse) {
  ivec rgb = imake(-1, -1, -1, 0);
  for (int i = 0; i < count; i++) {
    vec v = load(px + i * 4), a = splat(lane(v, 3));
    vec out = inverse ? vdiv(v, a) : mul(v, a);
    ivec m = inverse ? iand(rgb, ne(a, splat(0))) : rgb;
    store(px + i * 4, vselect(m, out, v));
  }
}

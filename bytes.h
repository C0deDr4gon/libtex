#pragma once

#include "platform.h"
#include "vec.h"

int tex_to_bytes(const float *in, int count, unsigned char *out) {
  int i = 0;
  ivec same = isplat(-1);
  for (; i + 16 <= count; i += 16) {
    ivec q[4];
    for (int k = 0; k < 4; k++) {
      vec v = load(in + i + k * 4);
      q[k] = vtrunc(add(mul(vmin(vmax(v, splat(0)), splat(1)), splat(255)), splat(0.5f)));
      same = iand(same, ieq(bits(v), bits(vdiv(tofloat(q[k]), splat(255)))));
    }
    istore(out + i, narrow(q[0], q[1], q[2], q[3]));
  }
  int exact = all(same);
  for (; i < count; i++) {
    out[i] = (int)(clamp(in[i], 0, 1) * 255 + 0.5f);
    float back = out[i] / 255.0f;
    unsigned a, b;
    copy(&a, in + i, 4);
    copy(&b, &back, 4);
    exact &= a == b;
  }
  return exact;
}

void tex_from_bytes(const unsigned char *in, int count, float *out) {
  int i = 0;
  for (; i + 4 <= count; i += 4) store(out + i, vdiv(tofloatu(bytes4(iload4(in + i))), splat(255)));
  for (; i < count; i++) out[i] = in[i] / 255.0f;
}

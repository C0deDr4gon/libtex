#include "base.h"
#include "vec.h"

EXPORT("to_bytes") void to_bytes(const float *in, int count, unsigned char *out) {
  int i = 0;
  for (; i + 16 <= count; i += 16) {
    ivec q[4];
    for (int k = 0; k < 4; k++) q[k] = vtrunc(add(mul(vmin(vmax(load(in + i + k * 4), splat(0)), splat(1)), splat(255)), splat(0.5f)));
    istore(out + i, narrow(q[0], q[1], q[2], q[3]));
  }
  for (; i < count; i++) out[i] = (int)(clamp(in[i], 0, 1) * 255 + 0.5f);
}

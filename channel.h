#pragma once

static void encode_channel(const double *values, unsigned char *out, int sign, const tex_options *o);
static void decode_channel(const unsigned char *in, double *values, int sign);

#include "platform.h"
#include "bits.h"
#include "vec.h"
#include "codec.h"

static void channel_ramp(int a0, int a1, int sign, double *r) {
  double scale = sign ? 127 : 255;
  double a = a0 / scale;
  double b = a1 / scale;
  r[0] = a;
  r[1] = b;
  if (a0 > a1) {
    for (int i = 1; i < 7; i++) r[1 + i] = (a * (7 - i) + b * i) / 7;
    return;
  }
  for (int i = 1; i < 5; i++) r[1 + i] = (a * (5 - i) + b * i) / 5;
  r[6] = sign ? -1 : 0;
  r[7] = 1;
}

static int channel_pick(const float *vs, const double *r, unsigned char *out, int pos) {
  for (int i = 0; i < 16; i += 4) {
    vec v = load(vs + i);
    vec best = splat(1.0f / 0.0f);
    ivec index = isplat(0);
    for (int j = 0; j < 8; j++) {
      vec d = vabs(sub(v, splat(r[j])));
      ivec closer = lt(d, best);
      best = vselect(closer, d, best);
      index = iselect(closer, isplat(j), index);
    }
    pos = put(out, pos, ilane(index, 0), 3);
    pos = put(out, pos, ilane(index, 1), 3);
    pos = put(out, pos, ilane(index, 2), 3);
    pos = put(out, pos, ilane(index, 3), 3);
  }
  return pos;
}

static float channel_error(const float *vs, int a0, int a1, int sign, unsigned char *idx) {
  double r[8];
  channel_ramp(a0, a1, sign, r);
  float total = 0;
  for (int i = 0; i < 16; i++) {
    float best = 1.0f / 0.0f;
    for (int j = 0; j < 8; j++) {
      float d = vs[i] - r[j];
      if (d * d < best) best = d * d, idx[i] = j;
    }
    total += best;
  }
  return total;
}

typedef struct {
  int a0, a1;
  float e;
} End;

static int ok(int c0, int c1, int eight, int sign) {
  int min = sign ? -127 : 0, max = sign ? 127 : 255;
  return c0 >= min && c0 <= max && c1 >= min && c1 <= max && (c0 > c1) == eight;
}

static void channel_consider(const float *vs, End *best, int c0, int c1, int sign) {
  unsigned char idx[16];
  if (!ok(c0, c1, best->a0 > best->a1, sign)) return;
  float e = channel_error(vs, c0, c1, sign, idx);
  if (e < best->e) best->a0 = c0, best->a1 = c1, best->e = e;
}

static void channel_refit(const float *vs, End *best, int sign) {
  double scale = sign ? 127 : 255;
  int eight = best->a0 > best->a1;
  for (int k = 0; k < 2; k++) {
    unsigned char idx[16];
    channel_error(vs, best->a0, best->a1, sign, idx);
    double aa = 0, bb = 0, ab = 0, ax = 0, bx = 0;
    for (int i = 0; i < 16; i++) {
      if (!eight && idx[i] > 5) continue;
      double t = idx[i] < 2 ? idx[i] : (idx[i] - 1) / (eight ? 7.0 : 5.0);
      aa += (1 - t) * (1 - t), bb += t * t, ab += (1 - t) * t, ax += (1 - t) * vs[i], bx += t * vs[i];
    }
    double det = aa * bb - ab * ab;
    if (det < 1e-9) return;
    int c0 = (int)fround((ax * bb - bx * ab) / det * scale), c1 = (int)fround((bx * aa - ax * ab) / det * scale);
    float before = best->e;
    channel_consider(vs, best, c0, c1, sign);
    if (best->e >= before) return;
  }
}

static void search(const float *vs, End *best, int sign) {
  static const int PAIRS[4][2] = { { 1, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 } };
  channel_refit(vs, best, sign);
  int b0 = best->a0, b1 = best->a1;
  for (int d0 = -4; d0 <= 4; d0++) for (int d1 = -4; d1 <= 4; d1++) channel_consider(vs, best, b0 + d0, b1 + d1, sign);
  for (int n = 0; n < 16; n++) {
    float before = best->e;
    int c0 = best->a0, c1 = best->a1;
    for (int k = 0; k < 4; k++) channel_consider(vs, best, c0 + (k == 0) - (k == 1), c1 + (k == 2) - (k == 3), sign);
    for (int k = 0; k < 4; k++) channel_consider(vs, best, c0 + PAIRS[k][0], c1 + PAIRS[k][1], sign);
    if (best->e >= before) break;
  }
}

static void encode_channel(const double *values, unsigned char *out, int sign, const tex_options *o) {
  float vs[16];
  double r[8], hi = -1.0 / 0.0, lo = 1.0 / 0.0;
  double scale = sign ? 127 : 255;
  for (int i = 0; i < 16; i++) {
    vs[i] = clamp(sign ? values[i] * 2 - 1 : values[i], sign ? -1 : 0, 1);
    if (vs[i] > hi) hi = vs[i];
    if (vs[i] < lo) lo = vs[i];
  }
  int a0 = (int)fround(hi * scale);
  int a1 = (int)fround(lo * scale);
  if (!is_fast(o) && a0 != a1) {
    unsigned char idx[16];
    End best = { a0, a1, channel_error(vs, a0, a1, sign, idx) };
    for (int o = 0; o < 2; o++) {
      End e = { o ? a1 : a0, o ? a0 : a1, 0 };
      e.e = channel_error(vs, e.a0, e.a1, sign, idx);
      search(vs, &e, sign);
      if (e.e < best.e) best = e;
    }
    a0 = best.a0, a1 = best.a1;
  }
  channel_ramp(a0, a1, sign, r);
  int pos = put(out, 0, a0 & 255, 8);
  pos = put(out, pos, a1 & 255, 8);
  channel_pick(vs, r, out, pos);
}

static int byte(const unsigned char *in, int pos, int sign) {
  int v = get(in, pos, 8);
  if (!sign) return v;
  v = (signed char)v;
  return v < -127 ? -127 : v;
}

static void decode_channel(const unsigned char *in, double *values, int sign) {
  double r[8];
  channel_ramp(byte(in, 0, sign), byte(in, 8, sign), sign, r);
  for (int i = 0; i < 16; i++) {
    double v = r[get(in, 16 + i * 3, 3)];
    values[i] = sign ? (v + 1) / 2 : v;
  }
}

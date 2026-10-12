#include "base.h"
#include "bits.h"
#include "fit.h"
#include "color.h"
#include "codec.h"

typedef struct {
  int c0, c1, four;
  unsigned char idx[16];
  float error;
} Try;

static vec weight[16];
static unsigned char opaque[16];

static int q(float v, int max) { return (int)round(clamp(v, 0, 1) * max); }

static int to565(vec c) { return q(lane(c, 0), 31) << 11 | q(lane(c, 1), 63) << 5 | q(lane(c, 2), 31); }

static vec from565(int c) {
  return vmake((c >> 11 & 31) / 31.0f, (c >> 5 & 63) / 63.0f, (c & 31) / 31.0f, 1);
}

static void palette(int c0, int c1, int four, vec *pal) {
  pal[0] = from565(c0);
  pal[1] = from565(c1);
  vec d = sub(pal[1], pal[0]);
  pal[2] = add(pal[0], mul(d, splat(four ? 1.0f / 3 : 1.0f / 2)));
  pal[3] = four ? add(pal[0], mul(d, splat(2.0f / 3))) : splat(0);
}

static Try make(const vec *pixels, int c0, int c1, int three, int punch) {
  Try t;
  t.c0 = c0;
  t.c1 = c1;
  if (three ? t.c0 > t.c1 : t.c0 < t.c1) {
    int s = t.c0;
    t.c0 = t.c1;
    t.c1 = s;
  }
  t.four = !punch || t.c0 > t.c1;
  vec pal[4];
  palette(t.c0, t.c1, t.four, pal);
  Pal set;
  pal_init(&set, t.four ? 4 : 3);
  for (int i = 0; i < set.count; i++) pal_set(&set, i, pal[i]);
  t.error = 0;
  for (int i = 0; i < 16; i++) {
    if (!opaque[i]) {
      t.idx[i] = 3;
      continue;
    }
    t.idx[i] = pick_w(pixels[i], weight[i], &set, 3);
    vec d = keep(sub(pixels[i], pal[t.idx[i]]), mask(3));
    t.error += hsum(mul(mul(d, d), weight[i]));
  }
  return t;
}

static Try fit(const vec *pixels, vec a, vec b, int three, int punch) {
  return make(pixels, to565(a), to565(b), three, punch);
}

static int refine(const vec *pixels, const Try *t, vec *a, vec *b) {
  float ts[4] = { 0, 1, t->four ? 1.0f / 3 : 1.0f / 2, 2.0f / 3 };
  return lsqf(pixels, weight, opaque, t->idx, ts, a, b);
}

static Try line(const vec *pixels, vec a, vec b, int three, int punch, int rounds) {
  Try best = fit(pixels, a, b, three, punch), last = best;
  for (int i = 0; i < rounds; i++) {
    if (!refine(pixels, &last, &a, &b)) break;
    last = fit(pixels, a, b, three, punch);
    if (last.error < best.error) best = last;
  }
  return best;
}

static int moved(int c, int k, int d) {
  static const int SHIFT[3] = { 11, 5, 0 }, MAX[3] = { 31, 63, 31 };
  int v = (c >> SHIFT[k] & MAX[k]) + d;
  if (v < 0 || v > MAX[k]) return -1;
  return (c & ~(MAX[k] << SHIFT[k])) | v << SHIFT[k];
}

static void nudge(const vec *pixels, Try *best, int three, int punch) {
  static const int PAIRS[4][2] = { { 1, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 } };
  for (int round = 0; round < 16; round++) {
    int better = 0;
    for (int e = 0; e < 2; e++) for (int k = 0; k < 3; k++) for (int d = -1; d <= 1; d += 2) {
      int c = moved(e ? best->c1 : best->c0, k, d);
      if (c < 0) continue;
      Try t = make(pixels, e ? best->c0 : c, e ? c : best->c1, three, punch);
      if (t.error < best->error) *best = t, better = 1;
    }
    for (int k = 0; k < 3; k++) for (int p = 0; p < 4; p++) {
      int u = moved(best->c0, k, PAIRS[p][0]), v = moved(best->c1, k, PAIRS[p][1]);
      if (u < 0 || v < 0) continue;
      Try t = make(pixels, u, v, three, punch);
      if (t.error < best->error) *best = t, better = 1;
    }
    if (!better) break;
  }
}

void encode_color(const float *px, unsigned char *out, int punch) {
  vec pixels[16], pts[16], a = splat(0), b = splat(0);
  int count = 0, transparent = 0;
  for (int i = 0; i < 16; i++) {
    pixels[i] = load(px + i * 4);
    transparent |= punch && px[i * 4 + 3] < 0.5f;
  }
  for (int i = 0; i < 16; i++) {
    opaque[i] = !transparent || px[i * 4 + 3] >= 0.5f;
    float w = punch ? 1 : clamp(px[i * 4 + 3], 1.0f / 64, 1);
    weight[i] = mul(vmake(w, w, w, 1), load(tint));
    if (opaque[i]) pts[count++] = pixels[i];
  }
  if (count) endpoints(pts, count, 3, &a, &b);
  Try best = line(pixels, a, b, transparent, punch, count ? 2 : 0);
  if (fast || !count) {
    int pos = put(out, 0, best.c0, 16);
    pos = put(out, pos, best.c1, 16);
    for (int i = 0; i < 16; i++) pos = put(out, pos, best.idx[i], 2);
    return;
  }
  Try seen[4];
  float errors[4];
  int found = 0;
  for (int three = transparent; three <= (punch ? 1 : transparent); three++) {
    for (int kind = 0; kind < 6; kind++) {
      vec s = a, z = b;
      if (kind == 1) {
        vec d = mul(sub(b, a), splat(1.0f / 16));
        s = add(a, d), z = sub(b, d);
      }
      if (kind > 1) corner(pts, count, kind - 2, &s, &z);
      Try t = line(pixels, s, z, three, punch, 2);
      int at = rank(errors, &found, 4, t.error, seen, sizeof(Try));
      if (at >= 0) seen[at] = t;
    }
  }
  nudge(pixels, &best, transparent, punch);
  for (int k = 0; k < found; k++) {
    int three = punch && !seen[k].four;
    nudge(pixels, &seen[k], three, punch);
    if (seen[k].error < best.error) best = seen[k];
  }
  int pos = put(out, 0, best.c0, 16);
  pos = put(out, pos, best.c1, 16);
  for (int i = 0; i < 16; i++) pos = put(out, pos, best.idx[i], 2);
}

void decode_color(const unsigned char *in, float *px, int punch) {
  int c0 = get(in, 0, 16);
  int c1 = get(in, 16, 16);
  vec pal[4];
  palette(c0, c1, !punch || c0 > c1, pal);
  for (int i = 0; i < 16; i++) store(px + i * 4, pal[get(in, 32 + i * 2, 2)]);
}

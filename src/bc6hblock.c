#include "base.h"
#include "bits.h"
#include "half.h"
#include "bc6h.h"
#include "bc7tables.h"

static int sext(int v, int bits) { return (int)((unsigned)v << (32 - bits)) >> (32 - bits); }

int unquant6(int v, int bits, int sign) {
  if (!sign) {
    if (bits >= 15 || !v) return v;
    return v == (1 << bits) - 1 ? 0xffff : ((v << 16) + 0x8000) >> bits;
  }
  if (bits >= 16 || !v) return v;
  int s = v < 0 ? -v : v;
  int u = s >= (1 << (bits - 1)) - 1 ? 0x7fff : ((s << 15) + 0x4000) >> (bits - 1);
  return v < 0 ? -u : u;
}

int finish6(int c, int sign) {
  if (!sign) return (c * 31) >> 6;
  return c < 0 ? -((-c * 31) >> 5) : (c * 31) >> 5;
}

void endpoints6(const Mode6 *m, const int *vals, int sign, int q[4][3]) {
  for (int e = 0; e < m->ns * 2; e++) {
    for (int k = 0; k < 3; k++) {
      int v = vals[k * 4 + e];
      if (e && m->transformed) v = (q[0][k] + sext(v, m->delta[k])) & ((1 << m->base) - 1);
      q[e][k] = sign ? sext(v, m->base) : v;
    }
  }
}

void palette6(const Mode6 *m, int q[4][3], int s, int sign, int out[16][3]) {
  const int *w = weights(m->ns == 2 ? 3 : 4);
  int a[3], z[3];
  for (int k = 0; k < 3; k++) {
    a[k] = unquant6(q[s * 2][k], m->base, sign);
    z[k] = unquant6(q[s * 2 + 1][k], m->base, sign);
  }
  for (int i = 0; i < (m->ns == 2 ? 8 : 16); i++) for (int k = 0; k < 3; k++) out[i][k] = finish6(lerp(a[k], z[k], w[i]), sign);
}

void colors6(const Block6 *b, int sign, vec *out) {
  int q[4][3], pal[2][16][3];
  endpoints6(b->mode, b->vals, sign, q);
  for (int s = 0; s < b->mode->ns; s++) palette6(b->mode, q, s, sign, pal[s]);
  for (int i = 0; i < 16; i++) {
    int *c = pal[subset6(b->mode, b->partition, i)][b->idx[i]];
    out[i] = vmake(c[0], c[1], c[2], 0);
  }
}

void pack6(const Block6 *b, unsigned char *out) {
  const Mode6 *m = b->mode;
  int pos = put(out, 0, m->id, m->id < 2 ? 2 : 5);
  for (int f = 0; f < m->count; f++) pos = put(out, pos, b->vals[m->fields[f] >> 4] >> (m->fields[f] & 15) & 1, 1);
  if (m->ns == 2) pos = put(out, pos, b->partition, 5);
  int bits = m->ns == 2 ? 3 : 4;
  for (int i = 0; i < 16; i++) pos = put(out, pos, b->idx[i], bits - anchor6(m, b->partition, i));
}

static int unpack6(const unsigned char *in, Block6 *b) {
  int id = get(in, 0, 2), pos = 2;
  if (id > 1) id |= get(in, 2, 3) << 2, pos = 5;
  b->mode = 0;
  for (int i = 0; i < 14; i++) if (MODES6[i].id == id) b->mode = &MODES6[i];
  if (!b->mode) return 0;
  const Mode6 *m = b->mode;
  for (int i = 0; i < 12; i++) b->vals[i] = 0;
  for (int f = 0; f < m->count; f++) b->vals[m->fields[f] >> 4] |= get(in, pos++, 1) << (m->fields[f] & 15);
  b->partition = m->ns == 2 ? get(in, pos, 5) : 0;
  if (m->ns == 2) pos += 5;
  int bits = m->ns == 2 ? 3 : 4;
  for (int i = 0; i < 16; i++) {
    int n = bits - anchor6(m, b->partition, i);
    b->idx[i] = get(in, pos, n), pos += n;
  }
  return 1;
}

void bc6h_decode(const unsigned char *in, float *px, int sign) {
  Block6 b;
  vec c[16];
  if (!unpack6(in, &b)) {
    __builtin_memset(px, 0, 256);
    return;
  }
  colors6(&b, sign, c);
  for (int i = 0; i < 16; i++) {
    px[i * 4] = from_half(lane(c[i], 0));
    px[i * 4 + 1] = from_half(lane(c[i], 1));
    px[i * 4 + 2] = from_half(lane(c[i], 2));
    px[i * 4 + 3] = 1;
  }
}

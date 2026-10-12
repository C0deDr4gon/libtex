#pragma once

#include "platform.h"
#include "vec.h"

#define FAST 10


typedef struct {
  short count[16], symbol[288];
  unsigned short fast[1 << FAST];
} Huff;

typedef struct {
  const unsigned char *in;
  int length, pos, count;
  u64 bits;
  unsigned char *out;
  int cap, at;
} Flate;

static const short LBASE[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const short LEXT[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const short DBASE[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const short DEXT[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
static const unsigned char CLORDER[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

static void refill(Flate *s) {
  if (s->pos <= s->length - 8) {
    u64 v;
    copy(&v, s->in + s->pos, 8);
    s->bits |= v << s->count;
    s->pos += (63 - s->count) >> 3;
    s->count |= 56;
    return;
  }
  while (s->count <= 56) {
    s->bits |= (u64)(s->pos < s->length ? s->in[s->pos] : 0) << s->count;
    s->pos++;
    s->count += 8;
  }
}

static unsigned need(Flate *s, int n) {
  if (s->count < n) refill(s);
  unsigned v = (unsigned)(s->bits & (((u64)1 << n) - 1));
  s->bits >>= n;
  s->count -= n;
  return v;
}

static int huff(Huff *h, const unsigned char *len, int n) {
  short offs[16];
  zero(h, sizeof *h);
  for (int i = 0; i < n; i++) h->count[len[i]]++;
  h->count[0] = 0;
  int left = 1;
  for (int l = 1; l < 16; l++) {
    left = (left << 1) - h->count[l];
    if (left < 0) return 0;
  }
  offs[1] = 0;
  for (int l = 1; l < 15; l++) offs[l + 1] = offs[l] + h->count[l];
  for (int i = 0; i < n; i++) if (len[i]) h->symbol[offs[len[i]]++] = i;
  int code = 0, at = 0;
  for (int l = 1; l <= FAST; l++) {
    for (int k = 0; k < h->count[l]; k++, code++) {
      int r = 0;
      for (int b = 0; b < l; b++) r |= (code >> b & 1) << (l - 1 - b);
      for (; r < 1 << FAST; r += 1 << l) h->fast[r] = h->symbol[at] << 4 | l;
      at++;
    }
    code <<= 1;
  }
  return 1;
}

static int slow(u64 *bits, int *count, const Huff *h) {
  int code = 0, first = 0, index = 0;
  for (int l = 1; l < 16; l++) {
    code |= (int)(*bits & 1);
    *bits >>= 1;
    --*count;
    int c = h->count[l];
    if (code - c < first) return h->symbol[index + code - first];
    index += c;
    first = (first + c) << 1;
    code <<= 1;
  }
  return -1;
}

static int symbol(Flate *s, const Huff *h) {
  if (s->count < 16) refill(s);
  int e = h->fast[s->bits & ((1 << FAST) - 1)];
  if (!e) return slow(&s->bits, &s->count, h);
  need(s, e & 15);
  return e >> 4;
}

static int codes(Flate *s, const Huff *lit, const Huff *dist) {
  const unsigned char *in = s->in;
  unsigned char *out = s->out;
  u64 bits = s->bits;
  int count = s->count, pos = s->pos, at = s->at, cap = s->cap, end = s->length - 8, ok = 0;
  const u64 MASK = (1 << FAST) - 1;
  for (;;) {
    if (count < 48) {
      if (pos <= end) {
        u64 v;
        copy(&v, in + pos, 8);
        bits |= v << count;
        pos += (63 - count) >> 3;
        count |= 56;
      } else {
        for (; count <= 56; count += 8, pos++) bits |= (u64)(pos < s->length ? in[pos] : 0) << count;
      }
    }
    int e = lit->fast[bits & MASK], c;
    if (e) bits >>= e & 15, count -= e & 15, c = e >> 4;
    else c = slow(&bits, &count, lit);
    if (c < 256) {
      if (c < 0 || at >= cap) break;
      out[at++] = c;
      continue;
    }
    if (c == 256) {
      ok = 1;
      break;
    }
    c -= 257;
    if (c >= 29) break;
    int len = LBASE[c] + (int)(bits & ((1u << LEXT[c]) - 1));
    bits >>= LEXT[c], count -= LEXT[c];
    int d = dist->fast[bits & MASK];
    if (d) bits >>= d & 15, count -= d & 15, d >>= 4;
    else d = slow(&bits, &count, dist);
    if (d < 0 || d >= 30) break;
    int back = DBASE[d] + (int)(bits & ((1u << DEXT[d]) - 1));
    bits >>= DEXT[d], count -= DEXT[d];
    if (back > at || len > cap - at) break;
    unsigned char *o = out + at;
    at += len;
    if (back >= 16 && cap - at >= 16) {
      for (int k = 0; k < len; k += 16) istore(o + k, iload(o + k - back));
    } else if (back >= 8 && cap - at >= 8) {
      for (int k = 0; k < len; k += 8) copy(o + k, o + k - back, 8);
    } else {
      for (int k = 0; k < len; k++) o[k] = o[k - back];
    }
  }
  s->bits = bits, s->count = count, s->pos = pos, s->at = at;
  return ok;
}

static int fixed(Flate *s) {
  unsigned char len[320];
  Huff lit, dist;
  for (int i = 0; i < 288; i++) len[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
  for (int i = 0; i < 30; i++) len[288 + i] = 5;
  return huff(&lit, len, 288) && huff(&dist, len + 288, 30) && codes(s, &lit, &dist);
}

static int dynamic(Flate *s) {
  unsigned char len[320];
  Huff lit, dist;
  int nlen = need(s, 5) + 257, ndist = need(s, 5) + 1, ncode = need(s, 4) + 4;
  if (nlen > 286 || ndist > 30) return 0;
  zero(len, 19);
  for (int i = 0; i < ncode; i++) len[CLORDER[i]] = need(s, 3);
  if (!huff(&lit, len, 19)) return 0;
  for (int i = 0; i < nlen + ndist;) {
    int c = symbol(s, &lit);
    if (c < 0) return 0;
    if (c < 16) {
      len[i++] = c;
      continue;
    }
    int v = 0, n;
    if (c == 16) {
      if (!i) return 0;
      v = len[i - 1];
      n = 3 + need(s, 2);
    } else n = c == 17 ? 3 + need(s, 3) : 11 + need(s, 7);
    if (i + n > nlen + ndist) return 0;
    while (n--) len[i++] = v;
  }
  if (!len[256]) return 0;
  return huff(&lit, len, nlen) && huff(&dist, len + nlen, ndist) && codes(s, &lit, &dist);
}

static int stored(Flate *s) {
  need(s, s->count & 7);
  int len = need(s, 16), nlen = need(s, 16);
  if (len != (~nlen & 0xffff) || len > s->cap - s->at) return 0;
  while (len--) s->out[s->at++] = need(s, 8);
  return 1;
}

static int inflate(const unsigned char *in, int length, unsigned char *out, int cap) {
  if (length < 2 || (in[0] & 15) != 8 || (in[0] << 8 | in[1]) % 31) return -1;
  Flate s = { in + 2, length - 2, 0, 0, 0, out, cap, 0 };
  int last;
  do {
    last = need(&s, 1);
    int type = need(&s, 2);
    int ok = type == 0 ? stored(&s) : type == 1 ? fixed(&s) : type == 2 ? dynamic(&s) : 0;
    if (!ok || s.pos - s.count / 8 > s.length) return -1;
  } while (!last);
  return s.at;
}

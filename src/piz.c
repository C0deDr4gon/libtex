#include "base.h"
#include "piz.h"

#define DECBITS 14
#define DECSIZE (1 << DECBITS)

typedef unsigned long long u64;

static unsigned char len[65537];
static u64 code[65537];
static unsigned char quick[DECSIZE];
static int symbol[DECSIZE], head[DECSIZE], next[65537];
static unsigned char bitmap[8192];
static unsigned short lut[65536];

typedef struct {
  const unsigned char *data;
  int p, length;
  u64 c;
  int lc;
} Bits;

static void fill(Bits *b) {
  b->c = b->c << 8 | (b->p < b->length ? b->data[b->p] : 0);
  b->p++;
  b->lc += 8;
}

static unsigned take(Bits *b, int n) {
  b->lc -= n;
  unsigned v = b->c >> b->lc & ((1ull << n) - 1);
  b->c &= (1ull << b->lc) - 1;
  return v;
}

static unsigned bits(Bits *b, int n) {
  while (b->lc < n) fill(b);
  return take(b, n);
}

static int emit(Bits *b, unsigned short *out, int *o, int count, int s, int im) {
  if (s != im) {
    if (*o < count) out[(*o)++] = s;
    return 1;
  }
  if (b->lc < 8) fill(b);
  int run = take(b, 8);
  if (!*o || *o + run > count) return 0;
  for (int k = 0; k < run; k++) out[*o + k] = out[*o - 1];
  *o += run;
  return 1;
}

static int huffman(const unsigned char *data, int length, unsigned short *out, int count) {
  if (length < 20) return 0;
  int im = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
  int iM = data[4] | data[5] << 8 | data[6] << 16 | data[7] << 24;
  int nbits = data[12] | data[13] << 8 | data[14] << 16 | data[15] << 24;
  if (im < 0 || iM > 65536 || im > iM) return 0;
  Bits b = { data, 20, length, 0, 0 };
  __builtin_memset(len, 0, sizeof len);
  for (int i = im; i <= iM; i++) {
    int l = bits(&b, 6);
    if (l == 63) i += bits(&b, 8) + 5;
    else if (l >= 59) i += l - 58;
    else len[i] = l;
  }
  u64 counts[59] = { 0 };
  for (int i = im; i <= iM; i++) counts[len[i]]++;
  counts[0] = 0;
  u64 n = 0;
  for (int l = 58; l > 0; l--) {
    u64 m = (n + counts[l]) >> 1;
    counts[l] = n;
    n = m;
  }
  for (int i = im; i <= iM; i++) if (len[i]) code[i] = counts[len[i]]++;
  __builtin_memset(quick, 0, sizeof quick);
  for (int k = 0; k < DECSIZE; k++) head[k] = -1;
  for (int i = iM; i >= im; i--) {
    int l = len[i];
    if (l > DECBITS) {
      int k = code[i] >> (l - DECBITS) & (DECSIZE - 1);
      next[i] = head[k];
      head[k] = i;
    } else if (l) {
      int start = code[i] << (DECBITS - l);
      for (int k = start; k < start + (1 << (DECBITS - l)); k++) quick[k] = l, symbol[k] = i;
    }
  }
  b.c = 0;
  b.lc = 0;
  int end = b.p + (nbits + 7) / 8, o = 0;
  while (b.p < end) {
    fill(&b);
    while (b.lc >= DECBITS) {
      int k = b.c >> (b.lc - DECBITS) & (DECSIZE - 1);
      if (quick[k]) {
        take(&b, quick[k]);
        if (!emit(&b, out, &o, count, symbol[k], iM)) return 0;
        continue;
      }
      int s = head[k];
      for (; s >= 0; s = next[s]) {
        while (b.lc < len[s] && b.p < end) fill(&b);
        if (b.lc >= len[s] && (b.c >> (b.lc - len[s]) & ((1ull << len[s]) - 1)) == code[s]) break;
      }
      if (s < 0) return 0;
      take(&b, len[s]);
      if (!emit(&b, out, &o, count, s, iM)) return 0;
    }
  }
  int drop = (8 - nbits) & 7;
  b.c >>= drop;
  b.lc -= drop;
  while (b.lc > 0) {
    int k = b.c << (DECBITS - b.lc) & (DECSIZE - 1);
    if (!quick[k] || quick[k] > b.lc) return 0;
    take(&b, quick[k]);
    if (!emit(&b, out, &o, count, symbol[k], iM)) return 0;
  }
  return 1;
}

static void dec14(unsigned short *x, unsigned short *y) {
  int hs = (short)*y;
  int a = (short)*x + (hs & 1) + (hs >> 1);
  *x = a;
  *y = a - hs;
}

static void dec16(unsigned short *x, unsigned short *y) {
  int l = *x, h = *y;
  int b = (l - (h >> 1)) & 0xffff;
  *x = (h + b - 32768) & 0xffff;
  *y = b;
}

static void wavelet(unsigned short *buf, int start, int nx, int ox, int ny, int oy, int max) {
  void (*dec)(unsigned short *, unsigned short *) = max < 1 << 14 ? dec14 : dec16;
  int p = 1;
  while (p <= (nx < ny ? nx : ny)) p <<= 1;
  p >>= 1;
  int p2 = p;
  p >>= 1;
  while (p >= 1) {
    int ox1 = ox * p, oy1 = oy * p, ox2 = ox * p2, oy2 = oy * p2;
    int py = start;
    for (; py <= start + oy * (ny - p2); py += oy2) {
      int px = py;
      for (; px <= py + ox * (nx - p2); px += ox2) {
        int p01 = px + ox1, p10 = px + oy1, p11 = p10 + ox1;
        dec(&buf[px], &buf[p10]);
        dec(&buf[p01], &buf[p11]);
        dec(&buf[px], &buf[p01]);
        dec(&buf[p10], &buf[p11]);
      }
      if (nx & p) dec(&buf[px], &buf[px + oy1]);
    }
    if (ny & p) for (int px = py; px <= py + ox * (nx - p2); px += ox2) dec(&buf[px], &buf[px + ox1]);
    p2 = p;
    p >>= 1;
  }
}

int piz(const unsigned char *data, int length, unsigned short *buf, unsigned char *out, int size, int width, int lines, const int *chans, int count) {
  int min = data[0] | data[1] << 8, max = data[2] | data[3] << 8, p = 4;
  __builtin_memset(bitmap, 0, sizeof bitmap);
  if (min <= max) {
    if (max >= 8192 || p + max - min + 1 > length) return 0;
    for (int i = min; i <= max; i++) bitmap[i] = data[p++];
  }
  int k = 0;
  for (int i = 0; i < 65536; i++) if (!i || bitmap[i >> 3] & 1 << (i & 7)) lut[k++] = i;
  int n = data[p] | data[p + 1] << 8 | data[p + 2] << 16 | data[p + 3] << 24;
  if (n < 0 || p + 4 + n > length) return 0;
  __builtin_memset(buf, 0, size);
  if (!huffman(data + p + 4, n, buf, size / 2)) return 0;
  int at = 0;
  for (int c = 0; c < count; c++) {
    int n = chans[c * 2] == 1 ? 1 : 2;
    for (int j = 0; j < n; j++) wavelet(buf, at + j, width, n, lines, width * n, k - 1);
    at += width * lines * n;
  }
  for (int i = 0; i < size / 2; i++) buf[i] = lut[buf[i]];
  rows(buf, out, width, lines, chans, count);
  return 1;
}

void rows(const unsigned short *buf, unsigned char *out, int width, int lines, const int *chans, int count) {
  int o = 0;
  for (int y = 0; y < lines; y++) {
    int at = 0;
    for (int c = 0; c < count; c++) {
      int n = chans[c * 2] == 1 ? 1 : 2;
      for (int i = 0; i < width * n; i++) {
        int v = buf[at + y * width * n + i];
        out[o++] = v;
        out[o++] = v >> 8;
      }
      at += width * lines * n;
    }
  }
}

#include "base.h"
#include "half.h"
#include "piz.h"

static const int SIZE[3] = { 4, 2, 4 };

static void unrle(const unsigned char *in, int length, unsigned char *out, int size) {
  int i = 0, o = 0;
  while (i < length && o < size) {
    int n = (signed char)in[i++];
    if (n < 0) {
      for (int k = 0; k < -n && o < size; k++) out[o++] = in[i + k];
      i -= n;
    } else {
      for (int k = 0; k <= n && o < size; k++) out[o++] = in[i];
      i++;
    }
  }
}

static void unpredict(unsigned char *t, int length, unsigned char *out) {
  for (int i = 1; i < length; i++) t[i] = t[i - 1] + t[i] - 128;
  int half = (length + 1) >> 1;
  for (int i = 0; i < length; i++) out[i] = i & 1 ? t[half + (i >> 1)] : t[i >> 1];
}

static void pxr24(const unsigned char *tmp, int length, unsigned char *out, int width, int rows, const int *chans, int count) {
  int t = 0, o = 0;
  for (int y = 0; y < rows; y++) {
    for (int c = 0; c < count; c++) {
      int type = chans[c * 2], bytes = type == 0 ? 4 : type == 1 ? 2 : 3;
      unsigned pixel = 0;
      for (int x = 0; x < width; x++) {
        unsigned diff = 0;
        for (int b = 0; b < bytes; b++) diff = diff << 8 | (t + b * width + x < length ? tmp[t + b * width + x] : 0);
        if (type == 2) diff <<= 8;
        pixel += diff;
        out[o++] = pixel;
        out[o++] = pixel >> 8;
        if (type == 1) {
          pixel &= 0xffff;
          continue;
        }
        out[o++] = pixel >> 16;
        out[o++] = pixel >> 24;
      }
      t += bytes * width;
    }
  }
}

static float read(const unsigned char *p, int type, const float *half) {
  if (type == 2) {
    float v;
    __builtin_memcpy(&v, p, 4);
    return v;
  }
  if (type == 0) return (unsigned)(p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24);
  return half[p[0] | p[1] << 8];
}

EXPORT("exr_fill") void exr_fill(float *out, int count) {
  for (int i = 0; i < count; i++) {
    out[i * 4] = out[i * 4 + 1] = out[i * 4 + 2] = 0;
    out[i * 4 + 3] = 1;
  }
}

EXPORT("exr_chunk") int exr_chunk(int kind, unsigned char *src, int length, unsigned char *tmp, unsigned char *dst, int size, const int *chans, int count, float *out, int full, int x0, int y0, int width, int rows) {
  const unsigned char *data = src;
  if (kind == 1) {
    __builtin_memset(tmp, 0, size);
    unrle(src, length, tmp, size);
    unpredict(tmp, size, dst);
    data = dst;
  }
  if (kind == 2) {
    unpredict(src, length, dst);
    data = dst;
  }
  if (kind == 3) {
    if (!piz(src, length, (unsigned short *)tmp, dst, size, width, rows, chans, count)) return 0;
    data = dst;
  }
  if (kind == 4) {
    pxr24(src, length, dst, width, rows, chans, count);
    data = dst;
  }
  if (kind == 5) {
    if (!b44(src, length, (unsigned short *)tmp, dst, width, rows, chans, count)) return 0;
    data = dst;
  }
  const float *half = half_table();
  int pos = 0;
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < count; c++) {
      int type = chans[c * 2], mask = chans[c * 2 + 1];
      if (!mask) {
        pos += SIZE[type] * width;
        continue;
      }
      for (int x = 0; x < width; x++, pos += SIZE[type]) {
        float v = read(data + pos, type, half);
        float *o = out + ((y0 + r) * full + x0 + x) * 4;
        for (int k = 0; k < 4; k++) if (mask >> k & 1) o[k] = v;
      }
    }
  }
  return 1;
}

EXPORT("half_plane") void half_plane(const float *px, int count, int k, unsigned short *out) {
  for (int i = 0; i < count; i++) out[i] = to_half(px[i * 4 + k], 0);
}

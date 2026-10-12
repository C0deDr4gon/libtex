#include "base.h"
#include "codec.h"

int fast;

float tint[4] = { 1, 1, 1, 1 };

EXPORT("set_fast") void set_fast(int level) { fast = level; }

EXPORT("set_tint") void set_tint(float r, float g, float b) { tint[0] = r, tint[1] = g, tint[2] = b; }

static const Codec *CODECS[] = { &NONE, &BC1, &BC2, &BC3, &BC4, &BC4_SNORM, &BC5, &BC5_SNORM, &BC6H_UF16, &BC6H_SF16, &BC7 };

static void load(const float *src, int width, int height, int bx, int by, float *px) {
  for (int i = 0; i < 16; i++) {
    int x = bx * 4 + (i & 3);
    int y = by * 4 + (i >> 2);
    if (x > width - 1) x = width - 1;
    if (y > height - 1) y = height - 1;
    for (int k = 0; k < 4; k++) px[i * 4 + k] = src[(y * width + x) * 4 + k];
  }
}

static void store(float *dst, int width, int height, int bx, int by, const float *px) {
  for (int i = 0; i < 16; i++) {
    int x = bx * 4 + (i & 3);
    int y = by * 4 + (i >> 2);
    if (x < width && y < height) for (int k = 0; k < 4; k++) dst[(y * width + x) * 4 + k] = px[i * 4 + k];
  }
}

EXPORT("roundtrip") int roundtrip(int id, const float *src, float *dst, unsigned char *out, int width, int height) {
  const Codec *codec = CODECS[id];
  float px[64];
  unsigned char bytes[256];
  int bw = (width + 3) / 4;
  int bh = (height + 3) / 4;
  for (int by = 0; by < bh; by++) {
    for (int bx = 0; bx < bw; bx++) {
      load(src, width, height, bx, by, px);
      __builtin_memset(bytes, 0, 16);
      codec->encode(px, bytes);
      codec->decode(bytes, px);
      if (out) __builtin_memcpy(out + (by * bw + bx) * codec->size, bytes, codec->size);
      store(dst, width, height, bx, by, px);
    }
  }
  return bw * bh;
}

EXPORT("decode") void decode(int id, const unsigned char *in, int count, float *out) {
  const Codec *codec = CODECS[id];
  for (int b = 0; b < count; b++) codec->decode(in + b * codec->size, out + b * 64);
}

EXPORT("unpack") void unpack(int id, const unsigned char *in, float *dst, int width, int height) {
  const Codec *codec = CODECS[id];
  float px[64];
  int bw = (width + 3) / 4;
  for (int by = 0; by < (height + 3) / 4; by++) {
    for (int bx = 0; bx < bw; bx++) {
      codec->decode(in + (by * bw + bx) * codec->size, px);
      store(dst, width, height, bx, by, px);
    }
  }
}

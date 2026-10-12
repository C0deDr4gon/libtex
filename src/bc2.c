#include "base.h"
#include "codec.h"
#include "color.h"

static void encode(const float *px, unsigned char *out) {
  for (int i = 0; i < 16; i++) out[i >> 1] |= (int)round(clamp(px[i * 4 + 3], 0, 1) * 15) << (i & 1) * 4;
  encode_color(px, out + 8, 0);
}

static void decode(const unsigned char *in, float *px) {
  decode_color(in + 8, px, 0);
  for (int i = 0; i < 16; i++) px[i * 4 + 3] = (in[i >> 1] >> (i & 1) * 4 & 15) / 15.0;
}

const Codec BC2 = { encode, decode, 16 };

#include "codec.h"
#include "color.h"
#include "channel.h"

static void encode(const float *px, unsigned char *out) {
  double alpha[16];
  for (int i = 0; i < 16; i++) alpha[i] = px[i * 4 + 3];
  encode_channel(alpha, out, 0);
  encode_color(px, out + 8, 0);
}

static void decode(const unsigned char *in, float *px) {
  double alpha[16];
  decode_color(in + 8, px, 0);
  decode_channel(in, alpha, 0);
  for (int i = 0; i < 16; i++) px[i * 4 + 3] = alpha[i];
}

const Codec BC3 = { encode, decode, 16 };

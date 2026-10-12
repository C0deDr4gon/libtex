#pragma once

#include "codec.h"
#include "color.h"
#include "channel.h"

static void bc3_encode(const float *px, unsigned char *out, const tex_options *o) {
  double alpha[16];
  for (int i = 0; i < 16; i++) alpha[i] = px[i * 4 + 3];
  encode_channel(alpha, out, 0, o);
  encode_color(px, out + 8, 0, o);
}

static void bc3_decode(const unsigned char *in, float *px) {
  double alpha[16];
  decode_color(in + 8, px, 0);
  decode_channel(in, alpha, 0);
  for (int i = 0; i < 16; i++) px[i * 4 + 3] = alpha[i];
}

static const Codec BC3 = { bc3_encode, bc3_decode, 16 };

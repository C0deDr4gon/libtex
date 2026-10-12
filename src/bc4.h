#pragma once

#include "codec.h"
#include "channel.h"

static void bc4_encode(const float *px, unsigned char *out, int sign) {
  double r[16];
  for (int i = 0; i < 16; i++) r[i] = px[i * 4];
  encode_channel(r, out, sign);
}

static void bc4_decode(const unsigned char *in, float *px, int sign) {
  double r[16];
  decode_channel(in, r, sign);
  for (int i = 0; i < 16; i++) {
    px[i * 4] = r[i];
    px[i * 4 + 1] = px[i * 4 + 2] = 0;
    px[i * 4 + 3] = 1;
  }
}

static void bc4_encode_u(const float *px, unsigned char *out) { bc4_encode(px, out, 0); }
static void bc4_decode_u(const unsigned char *in, float *px) { bc4_decode(in, px, 0); }
static void bc4_encode_s(const float *px, unsigned char *out) { bc4_encode(px, out, 1); }
static void bc4_decode_s(const unsigned char *in, float *px) { bc4_decode(in, px, 1); }

static const Codec BC4 = { bc4_encode_u, bc4_decode_u, 8 };
static const Codec BC4_SNORM = { bc4_encode_s, bc4_decode_s, 8 };

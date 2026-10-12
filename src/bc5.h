#pragma once

#include "codec.h"
#include "channel.h"

static void bc5_encode(const float *px, unsigned char *out, int sign) {
  double r[16], g[16];
  for (int i = 0; i < 16; i++) {
    r[i] = px[i * 4];
    g[i] = px[i * 4 + 1];
  }
  encode_channel(r, out, sign);
  encode_channel(g, out + 8, sign);
}

static void bc5_decode(const unsigned char *in, float *px, int sign) {
  double r[16], g[16];
  decode_channel(in, r, sign);
  decode_channel(in + 8, g, sign);
  for (int i = 0; i < 16; i++) {
    px[i * 4] = r[i];
    px[i * 4 + 1] = g[i];
    px[i * 4 + 2] = 0;
    px[i * 4 + 3] = 1;
  }
}

static void bc5_encode_u(const float *px, unsigned char *out) { bc5_encode(px, out, 0); }
static void bc5_decode_u(const unsigned char *in, float *px) { bc5_decode(in, px, 0); }
static void bc5_encode_s(const float *px, unsigned char *out) { bc5_encode(px, out, 1); }
static void bc5_decode_s(const unsigned char *in, float *px) { bc5_decode(in, px, 1); }

static const Codec BC5 = { bc5_encode_u, bc5_decode_u, 16 };
static const Codec BC5_SNORM = { bc5_encode_s, bc5_decode_s, 16 };

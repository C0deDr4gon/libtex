#include "codec.h"
#include "channel.h"

static void encode(const float *px, unsigned char *out, int sign) {
  double r[16], g[16];
  for (int i = 0; i < 16; i++) {
    r[i] = px[i * 4];
    g[i] = px[i * 4 + 1];
  }
  encode_channel(r, out, sign);
  encode_channel(g, out + 8, sign);
}

static void decode(const unsigned char *in, float *px, int sign) {
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

static void encode_u(const float *px, unsigned char *out) { encode(px, out, 0); }
static void decode_u(const unsigned char *in, float *px) { decode(in, px, 0); }
static void encode_s(const float *px, unsigned char *out) { encode(px, out, 1); }
static void decode_s(const unsigned char *in, float *px) { decode(in, px, 1); }

const Codec BC5 = { encode_u, decode_u, 16 };
const Codec BC5_SNORM = { encode_s, decode_s, 16 };

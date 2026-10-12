#include "codec.h"

static void encode(const float *px, unsigned char *out) {
  __builtin_memcpy(out, px, 256);
}

static void decode(const unsigned char *in, float *px) {
  __builtin_memcpy(px, in, 256);
}

const Codec NONE = { encode, decode, 256 };

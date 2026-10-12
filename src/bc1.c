#include "codec.h"
#include "color.h"

static void encode(const float *px, unsigned char *out) { encode_color(px, out, 1); }
static void decode(const unsigned char *in, float *px) { decode_color(in, px, 1); }

const Codec BC1 = { encode, decode, 8 };

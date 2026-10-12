#pragma once

#include "codec.h"
#include "color.h"

static void bc1_encode(const float *px, unsigned char *out) { encode_color(px, out, 1); }
static void bc1_decode(const unsigned char *in, float *px) { decode_color(in, px, 1); }

static const Codec BC1 = { bc1_encode, bc1_decode, 8 };

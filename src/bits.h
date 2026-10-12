#pragma once

extern const int WEIGHTS[16], W2[4], W3[8];

const int *weights(int bits);

static inline int lerp(int a, int b, int w) { return (a * (64 - w) + b * w + 32) >> 6; }

#include "vec.h"

static inline ivec lerp4(ivec a, ivec b, ivec w) {
  return ishr(iadd(iadd(imul(a, isub(isplat(64), w)), imul(b, w)), isplat(32)), 6);
}

int put(unsigned char *buf, int pos, unsigned v, int n);
unsigned get(const unsigned char *buf, int pos, int n);

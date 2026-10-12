#pragma once

typedef struct {
  void (*encode)(const float *px, unsigned char *out);
  void (*decode)(const unsigned char *in, float *px);
  int size;
} Codec;

static int fast;
static float tint[4] = { 1, 1, 1, 1 };



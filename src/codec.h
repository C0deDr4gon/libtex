#pragma once

typedef struct {
  void (*encode)(const float *px, unsigned char *out);
  void (*decode)(const unsigned char *in, float *px);
  int size;
} Codec;

extern int fast;
extern float tint[4];

extern const Codec NONE, BC1, BC2, BC3, BC4, BC4_SNORM, BC5, BC5_SNORM, BC6H_UF16, BC6H_SF16, BC7;

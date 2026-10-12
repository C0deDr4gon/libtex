#pragma once

void encode_color(const float *px, unsigned char *out, int punch);
void decode_color(const unsigned char *in, float *px, int punch);

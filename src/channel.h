#pragma once

void encode_channel(const double *values, unsigned char *out, int sign);
void decode_channel(const unsigned char *in, double *values, int sign);

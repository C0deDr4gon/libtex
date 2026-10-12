#pragma once

int piz(const unsigned char *data, int length, unsigned short *buf, unsigned char *out, int size, int width, int lines, const int *chans, int count);
void rows(const unsigned short *buf, unsigned char *out, int width, int lines, const int *chans, int count);
int b44(const unsigned char *data, int length, unsigned short *buf, unsigned char *out, int width, int lines, const int *chans, int count);

#pragma once

#include "platform.h"
#include "piz.h"

static const unsigned char ORDER[15] = { 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 };

static int block(const unsigned char *data, int length, int p, unsigned short *s) {
  if (p + 3 > length) return 0;
  s[0] = data[p] << 8 | data[p + 1];
  if (data[p + 2] >= 52) {
    for (int i = 1; i < 16; i++) s[i] = s[0];
  } else {
    if (p + 14 > length) return 0;
    int shift = data[p + 2] >> 2, bias = 0x20 << shift;
    for (int i = 0; i < 15; i++) {
      int bit = 22 + i * 6, d = ORDER[i];
      int v = (data[p + (bit >> 3)] << 8 | data[p + (bit >> 3) + 1]) >> (10 - (bit & 7)) & 63;
      s[d] = s[d % 4 ? d - 1 : d - 4] + (v << shift) - bias;
    }
  }
  for (int i = 0; i < 16; i++) s[i] = s[i] & 0x8000 ? s[i] & 0x7fff : ~s[i] & 0xffff;
  return data[p + 2] >= 52 ? 3 : 14;
}

static int b44(const unsigned char *data, int length, unsigned short *buf, unsigned char *out, int width, int lines, const int *chans, int count) {
  int p = 0;
  unsigned short *plane = buf;
  for (int c = 0; c < count; c++) {
    if (chans[c * 2] != 1) {
      for (int i = 0; i < width * lines * 2; i++, p += 2) plane[i] = p + 1 < length ? data[p] | data[p + 1] << 8 : 0;
      plane += width * lines * 2;
      continue;
    }
    for (int y = 0; y < lines; y += 4) {
      for (int x = 0; x < width; x += 4) {
        unsigned short s[16];
        int used = block(data, length, p, s);
        if (!used) return 0;
        p += used;
        for (int i = 0; i < 16; i++) {
          int px = x + (i & 3), py = y + (i >> 2);
          if (px < width && py < lines) plane[py * width + px] = s[i];
        }
      }
    }
    plane += width * lines;
  }
  rows(buf, out, width, lines, chans, count);
  return 1;
}

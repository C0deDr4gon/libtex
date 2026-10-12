#include "base.h"
#include "bits.h"

const int WEIGHTS[16] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };
const int W2[4] = { 0, 21, 43, 64 };
const int W3[8] = { 0, 9, 18, 27, 37, 46, 55, 64 };

const int *weights(int bits) { return bits == 2 ? W2 : bits == 3 ? W3 : WEIGHTS; }

EXPORT("put") int put(unsigned char *buf, int pos, unsigned v, int n) {
  for (int i = 0; i < n; i++, pos++) if (v >> i & 1) buf[pos >> 3] |= 1 << (pos & 7);
  return pos;
}

EXPORT("get") unsigned get(const unsigned char *buf, int pos, int n) {
  unsigned v = 0;
  for (int i = 0; i < n; i++, pos++) v |= (unsigned)(buf[pos >> 3] >> (pos & 7) & 1) << i;
  return v;
}

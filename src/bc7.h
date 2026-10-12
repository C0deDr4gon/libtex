#pragma once

#include "vec.h"
#include "bc7tables.h"

typedef struct {
  int ns, pb, rb, isb, cb, ab, epb, spb, ib, ib2;
} Mode;

typedef struct {
  int mode, partition, rotation, isel;
  int ep[6][4];
  int pbits[6];
  unsigned char idx[16], idx2[16];
} Block;

extern const Mode MODES[8];

static inline int subset(const Mode *m, int p, int i) {
  return m->ns == 1 ? 0 : m->ns == 2 ? P2[p][i] : P3[p][i];
}

static inline int anchor(const Mode *m, int p, int s) {
  return !s ? 0 : m->ns == 2 ? A2[p] : s == 1 ? A3A[p] : A3B[p];
}
int bits_of(const Mode *m, int c);
int pbit_of(const Block *b, const Mode *m, int e);
int unquant(int q, int bits, int p);
void expand_endpoint(const int *e, const Mode *m, int p, int *out);
vec rotate(vec c, int r);
void colors(const Block *b, vec *out);
void pack(const Block *b, unsigned char *out);

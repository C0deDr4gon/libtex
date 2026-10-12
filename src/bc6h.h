#pragma once

#include "vec.h"
#include "bc7tables.h"

typedef struct {
  int id, base, delta[3], ns, transformed, count;
  unsigned char fields[80];
} Mode6;

typedef struct {
  const Mode6 *mode;
  int partition;
  int vals[12];
  int q[4][3];
  unsigned char idx[16];
} Block6;

extern const Mode6 MODES6[14];

static inline int subset6(const Mode6 *m, int p, int i) { return m->ns == 2 ? P2[p][i] : 0; }
static inline int anchor6(const Mode6 *m, int p, int i) { return !i || (m->ns == 2 && i == A2[p]); }
int unquant6(int v, int bits, int sign);
int finish6(int c, int sign);
void endpoints6(const Mode6 *m, const int *vals, int sign, int q[4][3]);
void palette6(const Mode6 *m, int q[4][3], int s, int sign, int out[16][3]);
void colors6(const Block6 *b, int sign, vec *out);
void pack6(const Block6 *b, unsigned char *out);

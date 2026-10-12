#pragma once

#include "vec.h"

void endpoints(const vec *pts, int count, int n, vec *hi, vec *lo);
typedef struct {
  float c[4][16] __attribute__((aligned(16)));
  int count;
} Pal;

void pal_init(Pal *pal, int count);
int pick(vec p, const Pal *pal, int n);
int pick_w(vec p, vec w, const Pal *pal, int n);

typedef struct {
  vec v[4], w[4];
} Probe;

void probe(Probe *p, vec px, vec w);
int pick_p(const Probe *p, const Pal *pal, int n);

static inline void pal_set(Pal *pal, int i, vec v) {
  pal->c[0][i] = lane(v, 0);
  pal->c[1][i] = lane(v, 1);
  pal->c[2][i] = lane(v, 2);
  pal->c[3][i] = lane(v, 3);
}
void spreads(const vec *px, int ns, int count, float *out);
int lsq(const vec *px, const vec *w, const unsigned char *in, const unsigned char *idx, const int *wt, vec *a, vec *z);
int lsqf(const vec *px, const vec *w, const unsigned char *in, const unsigned char *idx, const float *ts, vec *a, vec *z);
int rank(float *errors, int *found, int cap, float e, void *items, int size);
void corner(const vec *pts, int count, int kind, vec *a, vec *z);

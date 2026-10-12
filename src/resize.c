#include "base.h"
#include "vec.h"

static double sinpi(double x) {
  x -= 2 * floor((x + 1) / 2);
  if (x > 0.5) x = 1 - x;
  if (x < -0.5) x = -1 - x;
  double t = x * 3.141592653589793, t2 = t * t;
  return t * (1 - t2 / 6 * (1 - t2 / 20 * (1 - t2 / 42 * (1 - t2 / 72 * (1 - t2 / 110)))));
}

static double sinc(double x) { return x == 0 ? 1 : sinpi(x) / (3.141592653589793 * x); }

static const double SUPPORT[3] = { 0.5, 1, 3 };

static double kernel(int filter, double x) {
  double a = x < 0 ? -x : x;
  if (filter == 0) return x >= -0.5 && x < 0.5;
  if (filter == 1) return a < 1 ? 1 - a : 0;
  return a < 3 ? sinc(x) * sinc(x / 3) : 0;
}

typedef struct {
  int *first, *count, taps;
  float *weights;
} Plan;

static int plan(int in, int out, int filter, Plan *p) {
  double scale = (double)in / out, fs = scale > 1 ? scale : 1, radius = SUPPORT[filter] * fs;
  p->taps = (int)(2 * radius) + 3;
  p->first = alloc(out * 4);
  p->count = alloc(out * 4);
  p->weights = alloc(out * p->taps * 4);
  if (!p->first || !p->count || !p->weights) return 0;
  for (int o = 0; o < out; o++) {
    double c = (o + 0.5) * scale - 0.5, sum = 0;
    int lo = (int)floor(c - radius), n = 0;
    float *w = p->weights + o * p->taps;
    for (int i = lo; i <= lo + p->taps - 1; i++) sum += w[n++] = kernel(filter, (i - c) / fs);
    for (int k = 0; k < n; k++) w[k] = sum ? w[k] / sum : 0;
    p->first[o] = lo;
    p->count[o] = n;
  }
  return 1;
}

static int edge(int i, int n) { return i < 0 ? 0 : i >= n ? n - 1 : i; }

EXPORT("resize") int resize(const float *src, int w, int h, float *dst, int nw, int nh, int filter) {
  Plan x, y;
  float *tmp = alloc(nw * h * 16);
  if (!tmp || !plan(w, nw, filter, &x) || !plan(h, nh, filter, &y)) return 0;
  for (int r = 0; r < h; r++) {
    for (int o = 0; o < nw; o++) {
      vec s = splat(0);
      const float *wt = x.weights + o * x.taps;
      for (int k = 0; k < x.count[o]; k++) if (wt[k]) s = add(s, mul(load(src + (r * w + edge(x.first[o] + k, w)) * 4), splat(wt[k])));
      store(tmp + (r * nw + o) * 4, s);
    }
  }
  for (int o = 0; o < nh; o++) {
    const float *wt = y.weights + o * y.taps;
    for (int c = 0; c < nw; c++) {
      vec s = splat(0);
      for (int k = 0; k < y.count[o]; k++) if (wt[k]) s = add(s, mul(load(tmp + (edge(y.first[o] + k, h) * nw + c) * 4), splat(wt[k])));
      store(dst + (o * nw + c) * 4, s);
    }
  }
  return 1;
}

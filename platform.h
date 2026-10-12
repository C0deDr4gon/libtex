#pragma once

#define ALIGN16 __attribute__((aligned(16)))

typedef unsigned long long u64;

static inline double fsqrt(double v) { return __builtin_sqrt(v); }
static inline float fsqrtf(float v) { return __builtin_sqrtf(v); }
static inline double ffloor(double v) { return __builtin_floor(v); }
static inline double fceil(double v) { return __builtin_ceil(v); }
static inline double fround(double v) { return ffloor(v + 0.5); }
static inline double clamp(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline double fnan(void) { return __builtin_nan(""); }
static inline double finf(void) { return __builtin_inf(); }
static inline void copy(void *d, const void *s, unsigned n) { __builtin_memcpy(d, s, n); }
static inline void zero(void *d, unsigned n) { __builtin_memset(d, 0, n); }

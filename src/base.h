#pragma once

#define EXPORT(name) __attribute__((export_name(name)))

static inline double sqrt(double v) { return __builtin_sqrt(v); }
static inline double floor(double v) { return __builtin_floor(v); }
static inline double round(double v) { return floor(v + 0.5); }
static inline double clamp(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }

void *alloc(unsigned size);

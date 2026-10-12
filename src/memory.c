#include "base.h"

extern unsigned char __heap_base;

static unsigned top = 0;

EXPORT("reset") void reset(void) {
  top = (unsigned)&__heap_base;
}

EXPORT("alloc") void *alloc(unsigned size) {
  if (!top) reset();
  unsigned at = (top + 15) & ~15u;
  unsigned end = at + size;
  unsigned have = __builtin_wasm_memory_size(0) << 16;
  if (end > have && __builtin_wasm_memory_grow(0, (end - have + 65535) >> 16) < 0) return 0;
  top = end;
  return (void *)at;
}

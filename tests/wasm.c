#include "hash.h"

extern unsigned char __heap_base;

static unsigned top = 0;

void reset(void) { top = (unsigned)&__heap_base; }

void *alloc(unsigned size) {
  if (!top) reset();
  unsigned at = (top + 15) & ~15u;
  unsigned end = at + size;
  unsigned have = __builtin_wasm_memory_size(0) << 16;
  if (end > have && __builtin_wasm_memory_grow(0, (end - have + 65535) >> 16) < 0) return 0;
  zero((void *)at, size);
  top = end;
  return (void *)at;
}

void init(void) { tex_init(); }

unsigned run(const unsigned char *file, int length, int kind) { return hash_run(file, length, kind, alloc); }

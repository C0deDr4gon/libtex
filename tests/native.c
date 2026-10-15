#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

static void *hash_alloc(unsigned size) { return calloc(size + 16, 1); }

static int kind(const char *name) {
  const char *dot = strrchr(name, '.');
  return dot && !strcmp(dot, ".png") ? 0 : dot && !strcmp(dot, ".tga") ? 1 : 2;
}

int main(int argc, char **argv) {
  tex_init();
  printf("synthetic %08x\n", hash_run(0, 0, 0, hash_alloc));
  for (int i = 1; i < argc; i++) {
    FILE *f = fopen(argv[i], "rb");
    if (!f) return 1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *b = (unsigned char *)hash_alloc((unsigned)n);
    if (fread(b, 1, (size_t)n, f) != (size_t)n) return 1;
    fclose(f);
    const char *base = strrchr(argv[i], '/');
    printf("%s %08x\n", base ? base + 1 : argv[i], hash_run(b, (int)n, kind(argv[i]), hash_alloc));
  }
  return 0;
}

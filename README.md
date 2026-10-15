# libtex

Header only C texture library. No libc, no alloc, no deps.

- BC1, BC2, BC3, BC4, BC5 (+ SNORM), BC6H (UF16 / SF16), BC7: encode (Best / Fast) + decode
- PNG, TGA, EXR read to float RGBA. PNG rows / TGA header + pixels / EXR half planes for writers
- Mips, resize (box / bilinear / Lanczos), swizzle, premultiply, flip / rotate

## Use

```sh
git submodule add https://github.com/C0deDr4gon/libtex ext/libtex
```

One C file:

```c
#define LIBTEX_IMPLEMENTATION
#include "ext/libtex/libtex.h"
```

Call `tex_init()` once before use. Every buffer comes from the caller (`tex_*_info` / `tex_*_scratch` give sizes).

```c
tex_info info;
if (tex_tga_info(bytes, length, &info) == TEX_OK) {
  float *px = /* info.width * info.height * 4 floats */;
  void *scratch = /* info.scratch bytes */;
  tex_tga_read(bytes, length, px, scratch);
}
```

Build:
- Wasm: clang `--target=wasm32 -msimd128 -ffreestanding -nostdlib`
- Native: any C11 compiler (clang, gcc, MSVC). SSE4.1 path with `-msse4.1` / `-mavx2` / MSVC `/arch:AVX` or `/arch:AVX2`, else plain C. Define `LIBTEX_SCALAR` to force plain C
- No FP contraction (`-ffp-contract=off`, MSVC default `/fp:precise`): output then is bit identical to wasm

Test: `node tests/run.js [textures dir]` builds wasm + native SSE / AVX2 / scalar, compares hashes.

## License

MIT

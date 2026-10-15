# Plan

Human check each step. Claude do next step only after ok.

## Next
- [x] Move from Texel ext/libtex (history kept). Texel uses it as submodule.
- [ ] Native libtex: SSE4.1 backend in vec.h (+ scalar fallback), MSVC + clang / gcc native build (platform.h), native hash test vs wasm (blocks bit identical). Needed by NoiseBuilder.
- [ ] TGA write from bytes (caller quantizes; NoiseBuilder floor(v * 256)) + header writer, so users need no own TGA code.
- [ ] Wider SIMD in encoders (BC7 / BC6H index pick, 8 pixels per step) where blocks stay bit identical.

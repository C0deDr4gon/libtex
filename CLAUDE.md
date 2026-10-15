# libtex

Header only C texture library. BC1-5 / BC6H / BC7 encode + decode, PNG / TGA / EXR read + write helpers, mips, resize, small pixel ops. Used by Texel (wasm), Meshlet (wasm), NoiseBuilder (native) as git submodule.

## Rules
- Chat short. Caveman talk. Show code, not words.
- Work one step. Stop. Human check. Then next step.
- PLAN.md = steps with checkboxes. Do only next unchecked step. Tick box in same commit.
- No comments in code.
- Code simple. Code clean. Small files. Includes at file top (platform.h, vec.h, rest). Define before use, no forward declarations.
- Tests live in users (Texel tests/ hash + quality floors). Change here -> run Texel tests.
- Speed work must keep blocks bit identical (hash check old vs new build, every format, Best + Fast).
- CLAUDE.md lines factual. Update Layout when files change.

## Stack
- C, header only (libtex.h, LIBTEX_IMPLEMENTATION pulls in the other .h files as one unit). Public names tex_*, everything else static.
- No alloc: caller gives every buffer (scratch sizes from tex_*_scratch). No mutable globals: encoder state in a per call context on the stack (Enc7 / Enc6 / Enc1), lookup tables filled once by tex_init() (call before use / threads). Only exception: inflate Huffman tables LIT / DIST are THREAD (thread local). Thread safe after tex_init.
- No libc, freestanding. Only platform.h and vec.h use __builtin_* / __attribute__ / wasm_*.
- Error metric: plain RGBA (x tint per channel). No alpha weighting.
- SIMD: only vec.h names wasm_* / v128_t. Struct types vec / ivec / dvec, MSVC safe (no vector extensions, fixed shuffles as macros). One pixel = one vec (r g b a float). Now wasm simd128 only (clang --target=wasm32 -msimd128).
- One file per image format: png.h / tga.h / exr.h hold all code for that format (file, pixels, write). BC1-5 all in bc.h. Shared: inflate.h, half.h. New codec: Codec in its file, add to CODECS (roundtrip.h) + tex_format, same order (users mirror the order).
- Floats out RGBA. png / tga 0..1 (same as v / max), exr HDR.

## Layout
- CLAUDE.md: rules, stack, layout.
- PLAN.md: steps with checkboxes.
- README.md: what, use, API.
- LICENSE: MIT.
- libtex.h: public API: tex_format, tex_error, tex_info, encode / decode blocks, score, mips, edits, png / tga / exr file read.
- platform.h: ALIGN16, THREAD (_Thread_local), fsqrt, ffloor, fceil, fround, clamp, fnan, finf, copy, zero.
- vec.h: SIMD layer: vec (4 float), ivec (4 int / mask), dvec (2 double). Math, compare, vselect, lanes, shuffles, int ops, iload4 / istore4, convert, narrow / widen bytes, vswizzle, double ops, mask(n), hsum, dot, log2_1p.
- codec.h: Codec { encode(px, out, tex_options), decode, size } per format. tex_options { quality Best / Fast, weight r g b }. NULL = TEX_DEFAULT. is_fast(o), tint(o).
- roundtrip.h: tex_encode block loop, tex_decode / tex_decode_blocks, CODECS table (id = tex_format).
- none.h: NONE raw copy, test codec.
- png.h: PNG file read (chunks, inflate, unfilter, all color types + depths, palette, tRNS, Adam7), png_rows write.
- tga.h: TGA file read (types 1-3 / 9-11, RLE, color map, 15/16/24/32 bit, origin, extension alpha), write helpers (tga_gray, tga_pixels raw / RLE).
- exr.h: EXR file read (scanline / tiled / multipart, RLE, ZIP / ZIPS, PIZ, PXR24, B44 / B44A), half_plane write.
- inflate.h: zlib inflate (stored / fixed / dynamic), fast loop + simple tail.
- half.h: to_half / from_half, half_table.
- score.h: error sum + HDR peak.
- util.h: bytes <-> floats, swizzle, premultiply, orient.
- resize.h: separable resample (Box / triangle / Lanczos3).
- mip.h: alpha weighted box mip chain, odd sizes exact.
- bits.h: put / get bits, weights, lerp.
- fit.h: PCA endpoints, palette pick, least squares refit, rank, corner. Shared by BC7 + BC6H + BC1-3.
- init.h: tex_init: half table, partition masks, tga unit table.
- partition.h: partition scores.
- bc.h: BC1-5 encode + decode.
- bc7.h / bc7enc.h / bc7tables.h: BC7 format, encoder, tables.
- bc6h.h / bc6henc.h: BC6H format, encoder.

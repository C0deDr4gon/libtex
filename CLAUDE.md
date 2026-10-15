# libtex

Header only C texture library. BC1-5 / BC6H / BC7 encode + decode, PNG / TGA / EXR read + write helpers, mips, resize, small pixel ops. Used by Texel (wasm), Meshlet (wasm), NoiseBuilder (native): headers + LICENSE copied into their ext/libtex (repo private, no submodule). Change here -> copy to users.

## Rules
- Chat short. Caveman talk. Show code, not words.
- Work one step. Stop. Human check. Then next step.
- PLAN.md = steps with checkboxes. Do only next unchecked step. Tick box in same commit.
- No comments in code.
- Code simple. Code clean. Small files. Includes at file top (platform.h, vec.h, rest). Define before use, no forward declarations.
- Tests: `node tests/run.js [dir]` = native (SSE4.1, AVX2, scalar) vs wasm hash, must be same. Quality floors live in Texel tests. Change here -> run both.
- Speed work must keep blocks bit identical (hash check old vs new build, every format, Best + Fast).
- CLAUDE.md lines factual. Update Layout when files change.

## Stack
- C, header only (libtex.h, LIBTEX_IMPLEMENTATION pulls in the other .h files as one unit). Public names tex_*, everything else static.
- No alloc: caller gives every buffer (scratch sizes from tex_*_scratch). No mutable globals: encoder state in a per call context on the stack (Enc7 / Enc6 / Enc1), lookup tables filled once by tex_init() (call before use / threads). Only exception: inflate Huffman tables LIT / DIST are THREAD (thread local, fixed address = shorter decode chain in wasm). Thread safe after tex_init.
- Wasm: no libc, freestanding. Native: math.h / string.h only (platform.h). Only platform.h and vec*.h use __builtin_* / __attribute__ / __declspec / intrinsics.
- Native = wasm bit for bit: backends copy wasm semantics (min / max: NaN out, -0 < +0; trunc saturates, NaN -> 0; swizzle index >= 16 -> 0; select bitwise). Build with no FP contraction (-ffp-contract=off; MSVC default /fp:precise). No constant 1 / 0 (MSVC C2124): finf().
- Error metric: plain RGBA (x tint per channel). No alpha weighting.
- SIMD: vec.h picks backend: vec_wasm.h (__wasm_simd128__), vec_sse.h (__SSE4_1__ or __AVX__, not LIBTEX_SCALAR), else vec_scalar.h. Struct types vec / ivec / dvec, MSVC safe (no vector extensions, fixed shuffles as macros). One pixel = one vec (r g b a float).
- One file per image format: png.h / tga.h / exr.h hold all code for that format (file, pixels, write). BC1-5 all in bc.h. Shared: inflate.h, half.h. New codec: Codec in its file, add to CODECS (roundtrip.h) + tex_format, same order (users mirror the order).
- Floats out RGBA. png / tga 0..1 (same as v / max), exr HDR.

## Layout
- CLAUDE.md: rules, stack, layout.
- PLAN.md: steps with checkboxes.
- README.md: what, use, API.
- LICENSE: MIT.
- libtex.h: public API: tex_format, tex_error, tex_info, encode / decode blocks, score, mips, edits, png / tga / exr file read (tex_info { width, height, bits, compression, scratch }, tex_error)
- platform.h: wasm (__builtin_*) or native (math.h / string.h, MSVC __declspec): ALIGN16, THREAD, fsqrt, ffloor, fceil, fnan, finf, copy, zero; fround, clamp, u64
- vec.h: SIMD layer: backend pick + shared mask(n), hsum, dot, log2_1p. vec (4 float), ivec (4 int / mask), dvec (2 double)
- vec_wasm.h / vec_sse.h / vec_scalar.h: backends, same names: math, compare, vselect, lanes, shuffles, int ops, iload4 / istore4 (32 bit), convert, narrow / widen bytes, vswizzle, double ops
- tests/hash.h: hash of decode (file or synthetic) + every codec Best / Fast (blocks, decoded, score) + mips, resize, orient, swizzle, premultiply, bytes, tga pixels, half plane
- tests/native.c: native driver (prints hashes). tests/wasm.c: wasm driver (bump alloc, init, run)
- tests/run.js: build wasm + native sse / avx2 / scalar (CC, default clang), hash Texel textures (default ../Texel/tests/textures), compare
- codec.h: Codec { encode(px, out, tex_options), decode, size } per format. tex_options { quality Best / Fast, weight r g b } for BC7 / BC6H / BC1-3 color + score. NULL = TEX_DEFAULT. is_fast(o), tint(o)
- roundtrip.h: tex_encode: block loop, optional block bytes out. tex_decode / tex_decode_blocks: blocks -> pixels. CODECS table, id = tex_format
- none.h: NONE: raw copy, test codec
- png.h: all PNG. File: tex_png_info / tex_png_read: chunks (IHDR / PLTE / tRNS / IDAT, bounds checked), depth / color check, IDAT joined + inflated in scratch (palette 768 + trns 256 + idat + raw), png_decode. Pixels: read (png_decode): unfilter per row (filter picked once per row, paeth SIMD 4 bytes per step for bpp >= 3, bpp 3 lane 3 masked), all color types + depths, palette, tRNS, Adam7. Fast row: 8 / 16 bit gray / GA / RGB / RGBA, no palette / key, not interlaced: ints -> SIMD float / max (bit same as v / max double). Else per pixel. write: png_rows float -> 8 / 16 bit rows, filter per row (min sum abs, or fixed mode)
- tga.h: all TGA. File: tex_tga_info / tex_tga_read: header (types 1-3 / 9-11, id, color map bounds), extension area alpha / premultiplied, tga_decode. Scratch = w * h * bytes per pixel (RLE / short files). Pixels: read (tga_decode): color map index out of range -> black, RLE, all types, 15/16 bit, interleave, flip, premultiplied. SIMD row path for 8/24/32 bit. Same floats as v / 255.0. write: tex_tga_header (18 bytes: type 3 gray for n 1 / 2, else 2, +8 RLE, top left, alpha bits for n 2 / 4), SIMD float -> bytes, tga_gray (r = g = b -> 8 bit gray or 16 bit gray + alpha, type 3 / 11), tga_pixels rows raw or RLE (runs <= 128, never across rows)
- score.h: error sum (double) + HDR peak, log2_1p (vec.h), no libm
- util.h: small per pixel ops. Bytes: tex_to_bytes: float 0..1 -> bytes, SIMD, round to nearest, returns 1 if every float == byte / 255 (bits). tex_from_bytes: bytes -> byte / 255 floats, SIMD. Swizzle: per pixel: i8x16 swizzle, scale + offset, constants by mask. Alpha: premultiply rgb x a / unpremultiply rgb / a (a = 0 kept), alpha kept. Orient: flip / rotate: one vec copy per pixel, 90 / 270 swap size
- resize.h: tex_resize_scratch + tex_resize (tmp + plans in caller scratch). Separable resample, weight plan per row / column, filter stretched when shrinking, edge clamp. Box / triangle / Lanczos3 (sinpi, no libm). Channels apart, no clamp
- mip.h: tex_mips_scratch + tex_mips (taps in caller scratch). Alpha weighted box filter mip chain, odd sizes exact taps, SIMD per pixel
- inflate.h: unzlib: zlib inflate (stored / fixed / dynamic). 10 bit table of u64 entries (len | kind << 6 | extra << 9 | value << 16; kinds literal / length-dist / end / bad / slow / resolved = code + extra bits in table). Fast loop (16 input + 274 output bytes left): pointers, branchless refill, 4 literals per refill, refill again before distance, copy 16 byte SIMD (back >= 16 or len <= back) / 8 / 4 / 1, count -= whole entry. Simple tail loop near the ends (one symbol at a time). Beats Node zlib on literal heavy and real data, for PNG IDAT + EXR ZIP / ZIPS / PXR24
- exr.h: all EXR. File: tex_exr_info / tex_exr_read: whole EXR file (header, channels -> RGBA / Y, offsets, scanline / tiled / multipart, bounds checked). Scratch = Piz (PIZ only) + channel list + tmp + dst. Chunks: read: EXR chunk (exr_chunk): RLE, predictor + interleave, PIZ, PXR24, B44, half/float/uint to float, channels to RGBA. write: half_plane (one channel to halves). PIZ: EXR PIZ: Piz tables in exr scratch (PIZ only), huffman (64 bit reader), wavelet, lut. rows(): planes to row layout. B44: EXR B44 / B44A: 4x4 half blocks, uses rows()
- bits.h: put/get bits, WEIGHTS, W2, W3, weights(bits), lerp + lerp4 SIMD (bits.h)
- half.h: to_half/from_half, half_table (65536 floats), Inf/NaN, no libm
- fit.h: PCA endpoints, Pal + pick_w / pick_p (Probe = pixel + weight splat once per block), lsq / lsqf (weighted least squares endpoints, int or float index table), rank (sorted top N), corner (box diagonal start). Shared by BC7 + BC6H + BC1-3
- init.h: tex_init: half table, partition masks, tga unit table
- partition.h: spreads: partition scores, 4 partitions per vec (lane masks from P2 / P3)
- bc.h: all BC1-5. Color block (BC1/2/3): PCA + 2 least squares refits (lsqf). Best: start lines (PCA, inset, 4 box diagonals; BC1 opaque also 3 color mode), refit each, nudge PCA + best 4 (565 +-1 singles + pairs). Channel block (BC3/4/5): 4 pixels per vec. Best: both orders (8 / 6 value ramp), each: 1D least squares refit, +-4 window search, nudge singles + pairs. Codecs: static const Codec BC1 .. BC5_SNORM = { bcN_encode, bcN_decode, size }
- bc7.h: BC7 Mode, Block, modes, colors, pack, unpack, decode
- bc7enc.h: BC7 encode: modes 1 (best spread partition), 6, 5 first; Fast stops there. Best: brute force every partition of modes 0/1/2/3/7 + modes 4/5 all rotations, least squares refit + start lines (inset + 4 box diagonals) on best 8 candidates, nudge endpoints + pair moves + p-bits on best 8. Error summed during index pick, stops early once worse than needed. Nudge reuses picks of unchanged subsets. Refit covers modes 4/5 (color and alpha indices apart).
- bc7tables.h: BC7 partition + anchor tables, bytes
- bc6h.h: BC6H Mode6 (bit field table), MODES6 (14 modes, field byte = slot << 4 | bit), Block6, unquant, endpoints, palette, colors, pack, unpack, decode
- bc6henc.h: BC6H encode: error in half space weighted per channel by log2(1 + v) slope (pick, cost, refit; all slopes 0 (black block) -> tint weights). Fast: best spread partition, 7 modes. Best: every partition x all 14 modes, refit + start lines on best 8, nudge endpoints + pair moves. Anchor index too high flips endpoints. Error summed during index pick, stops early once worse than needed. Nudge reuses picks of unchanged subset

#ifndef LIBTEX_H
#define LIBTEX_H

typedef enum { TEX_NONE, TEX_BC1, TEX_BC2, TEX_BC3, TEX_BC4, TEX_BC4_SNORM, TEX_BC5, TEX_BC5_SNORM, TEX_BC6H_UF16, TEX_BC6H_SF16, TEX_BC7 } tex_format;

int tex_block_size(tex_format id);
int tex_encode(tex_format id, const float *src, int width, int height, unsigned char *out, float *dst);
void tex_decode(tex_format id, const unsigned char *in, int width, int height, float *dst);
void tex_decode_blocks(tex_format id, const unsigned char *in, int count, float *out);
void tex_score(const float *a, const float *b, int pixels, int channels, int hdr, double *out);

void tex_mips(const float *src, int width, int height, float *out);
void tex_premultiply(float *px, int count, int inverse);
void tex_swizzle(float *px, int count, const int *src, const float *scale, const float *offset);
void tex_orient(const float *src, int w, int h, float *dst, int op);
int tex_resize(const float *src, int w, int h, float *dst, int nw, int nh, int filter);
void tex_to_bytes(const float *in, int count, unsigned char *out);

void tex_png_read(unsigned char *raw, int width, int height, int depth, int color, int interlace, const unsigned char *palette, const unsigned char *trns, int trns_length, float *out);
void tex_png_rows(const float *px, int w, int h, int n, int depth, int mode, unsigned char *out, unsigned char *scratch);
void tex_tga_read(const unsigned char *buf, int length, int pos, int rle, unsigned char *scratch, int width, int height, int n, int kind, int desc, const unsigned char *cmap, int cmap_bytes, int cmap_start, int alpha, int premultiplied, float *out);
int tex_tga_gray(const float *px, int count);
int tex_tga_pixels(const float *px, int w, int h, int n, int rle, unsigned char *out, unsigned char *row);
int tex_exr_chunk(int kind, unsigned char *src, int length, unsigned char *tmp, unsigned char *dst, int size, const int *chans, int count, float *out, int full, int x0, int y0, int width, int rows);
void tex_exr_fill(float *out, int count);
void tex_half_plane(const float *px, int count, int k, unsigned short *out);

#endif

#if defined(LIBTEX_IMPLEMENTATION) && !defined(LIBTEX_DONE)
#define LIBTEX_DONE
#include "src/roundtrip.h"
#include "src/score.h"
#include "src/mip.h"
#include "src/alpha.h"
#include "src/swizzle.h"
#include "src/orient.h"
#include "src/resize.h"
#include "src/bytes.h"
#include "src/png.h"
#include "src/tga.h"
#include "src/exr.h"
#endif

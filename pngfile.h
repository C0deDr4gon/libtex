#pragma once

#include "png.h"
#include "inflate.h"

typedef struct {
  const unsigned char *b, *palette, *trns;
  int length, width, height, depth, color, interlace, palette_length, trns_length, idat, raw;
} PngFile;

static int be32(const unsigned char *p) { return (int)((unsigned)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]); }

static int is(const unsigned char *p, const char *s) { return p[0] == s[0] && p[1] == s[1] && p[2] == s[2] && p[3] == s[3]; }

static int depth_ok(int color, int depth) {
  if (color == 0) return depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16;
  if (color == 3) return depth == 1 || depth == 2 || depth == 4 || depth == 8;
  return (color == 2 || color == 4 || color == 6) && (depth == 8 || depth == 16);
}

static long long raw_size(const PngFile *f) {
  long long total = 0;
  int n = CHANNELS[f->color];
  for (int pass = 0; pass < (f->interlace ? 7 : 1); pass++) {
    int x0 = f->interlace ? ADAM7[pass][0] : 0, y0 = f->interlace ? ADAM7[pass][1] : 0;
    int dx = f->interlace ? ADAM7[pass][2] : 1, dy = f->interlace ? ADAM7[pass][3] : 1;
    long long pw = (f->width - x0 + dx - 1) / dx, ph = (f->height - y0 + dy - 1) / dy;
    if (pw > 0 && ph > 0) total += ph * (1 + (pw * n * f->depth + 7) / 8);
  }
  return total;
}

static int png_open(PngFile *f, const unsigned char *b, int length) {
  zero(f, sizeof *f);
  f->b = b, f->length = length;
  if (length < 8 || be32(b) != (int)0x89504e47 || be32(b + 4) != 0x0d0a1a0a) return TEX_ERROR_FORMAT;
  int pos = 8, header = 0;
  while (pos <= length - 8) {
    int len = be32(b + pos);
    const unsigned char *type = b + pos + 4, *data = b + pos + 8;
    if (len < 0 || len > length - pos - 8) return TEX_ERROR_DATA;
    if (is(type, "IHDR") && len >= 13) {
      f->width = be32(data), f->height = be32(data + 4);
      f->depth = data[8], f->color = data[9], f->interlace = data[12];
      header = data[10] == 0 && data[11] == 0 && f->interlace <= 1;
    } else if (is(type, "PLTE")) f->palette = data, f->palette_length = len;
    else if (is(type, "tRNS")) f->trns = data, f->trns_length = len;
    else if (is(type, "IDAT")) f->idat += len;
    else if (is(type, "IEND")) break;
    pos += 12 + len;
  }
  if (!header || !depth_ok(f->color, f->depth) || f->width <= 0 || f->height <= 0 || f->width > 65536 || f->height > 65536 || !f->idat) return TEX_ERROR_DATA;
  if (f->color == 3 && !f->palette) return TEX_ERROR_DATA;
  if (f->trns && f->trns_length < (f->color == 0 ? 2 : f->color == 2 ? 6 : 0)) f->trns = 0;
  if (f->trns && (f->color == 4 || f->color == 6)) f->trns = 0;
  long long raw = raw_size(f);
  if (raw > 0x40000000) return TEX_ERROR_DATA;
  f->raw = (int)raw;
  return TEX_OK;
}

int tex_png_info(const unsigned char *bytes, int length, tex_info *info) {
  PngFile f;
  int r = png_open(&f, bytes, length);
  zero(info, sizeof *info);
  if (r) return r;
  info->width = f.width, info->height = f.height, info->bits = f.depth;
  info->scratch = 1024 + f.idat + f.raw;
  return TEX_OK;
}

int tex_png_read(const unsigned char *bytes, int length, float *out, void *scratch) {
  PngFile f;
  int r = png_open(&f, bytes, length);
  if (r) return r;
  unsigned char *palette = scratch, *trns = palette + 768, *idat = trns + 256, *raw = idat + f.idat;
  zero(palette, 1024);
  if (f.palette) copy(palette, f.palette, f.palette_length < 768 ? f.palette_length : 768);
  int trns_length = f.trns_length < 256 ? f.trns_length : 256;
  if (f.trns) copy(trns, f.trns, trns_length);
  int at = 0;
  for (int pos = 8; pos <= length - 8;) {
    int len = be32(bytes + pos);
    if (is(bytes + pos + 4, "IDAT")) copy(idat + at, bytes + pos + 8, len), at += len;
    if (is(bytes + pos + 4, "IEND")) break;
    pos += 12 + len;
  }
  zero(raw, f.raw);
  if (unzlib(idat, f.idat, raw, f.raw) < 0) return TEX_ERROR_DATA;
  png_decode(raw, f.width, f.height, f.depth, f.color, f.interlace, palette, f.trns ? trns : 0, trns_length, out);
  return TEX_OK;
}

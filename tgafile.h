#pragma once

#include "tga.h"

typedef struct {
  int type, kind, width, height, n, desc, pos, cmap, cmap_bytes, cmap_start, cmap_count, alpha, premultiplied;
} TgaFile;

static int le16(const unsigned char *p) { return p[0] | p[1] << 8; }

static int tga_open(TgaFile *f, const unsigned char *b, int length) {
  zero(f, sizeof *f);
  if (length < 18) return TEX_ERROR_DATA;
  f->type = b[2], f->kind = b[2] & 7;
  if (f->type > 11 || f->kind < 1 || f->kind > 3) return TEX_ERROR_COMPRESSION;
  f->width = le16(b + 12), f->height = le16(b + 14);
  f->n = (b[16] + 7) / 8, f->desc = b[17];
  f->cmap_start = le16(b + 3), f->cmap_bytes = (b[7] + 7) / 8;
  f->cmap = 18 + b[0];
  f->cmap_count = b[1] ? le16(b + 5) : 0;
  f->pos = f->cmap + f->cmap_count * f->cmap_bytes;
  if (!f->width || !f->height || f->n < 1 || f->n > 4 || f->pos > length) return TEX_ERROR_DATA;
  if (f->kind == 1 && (!b[1] || f->n > 2 || f->cmap_bytes < 2 || f->cmap_bytes > 4)) return TEX_ERROR_DATA;
  int bytes = f->kind == 1 ? f->cmap_bytes : f->n;
  f->alpha = (f->desc & 15) > 0 || bytes == 4 || (f->kind == 3 && bytes == 2);
  if (length >= 26) {
    const unsigned char *sig = b + length - 18;
    static const char SIGNATURE[18] = "TRUEVISION-XFILE.";
    int match = 1;
    for (int i = 0; i < 18; i++) match &= sig[i] == (unsigned char)SIGNATURE[i];
    int ext = (int)(b[length - 26] | b[length - 25] << 8 | b[length - 24] << 16 | (unsigned)b[length - 23] << 24);
    if (match && ext > 0 && ext <= length - 495) {
      int a = b[ext + 494];
      f->alpha = a == 3 || a == 4;
      f->premultiplied = a == 4;
    }
  }
  return TEX_OK;
}

int tex_tga_info(const unsigned char *bytes, int length, tex_info *info) {
  TgaFile f;
  int r = tga_open(&f, bytes, length);
  zero(info, sizeof *info);
  info->compression = f.type;
  if (r) return r;
  info->width = f.width, info->height = f.height, info->bits = 8;
  info->scratch = f.width * f.height * f.n;
  return TEX_OK;
}

int tex_tga_read(const unsigned char *bytes, int length, float *out, void *scratch) {
  TgaFile f;
  int r = tga_open(&f, bytes, length);
  if (r) return r;
  tga_decode(bytes, length, f.pos, f.type & 8 ? 1 : 0, scratch, f.width, f.height, f.n, f.kind, f.desc, bytes + f.cmap, f.cmap_bytes, f.cmap_start, f.cmap_count, f.alpha, f.premultiplied, out);
  return TEX_OK;
}

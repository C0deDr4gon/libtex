#pragma once

#include "exr.h"
#include "inflate.h"

static const int LINES[10] = { 1, 1, 1, 16, 32, 16, 32, 32, 32, 256 };

typedef struct {
  const unsigned char *b;
  int length, multipart, tiled, deep, compression, chlist, count, x0, y0, width, height, tx, ty, chunks, table, stride, cap;
} Exr;

typedef struct {
  int x0, y0, w, rows, size, length, kind;
  const unsigned char *data;
} Chunk;

static int i32(const unsigned char *p) { return (int)(p[0] | p[1] << 8 | p[2] << 16 | (unsigned)p[3] << 24); }

static int fits(const Exr *e, int pos, int n) { return pos >= 0 && n >= 0 && pos <= e->length - n; }

static int text(const Exr *e, int pos) {
  while (pos < e->length && e->b[pos]) pos++;
  return pos < e->length ? pos + 1 : -1;
}

static int named(const Exr *e, int pos, const char *s) {
  while (*s && e->b[pos] == (unsigned char)*s) pos++, s++;
  return !*s && !e->b[pos];
}

static int starts(const Exr *e, int pos, int size, const char *s) {
  int n = 0;
  while (s[n] && n < size && e->b[pos + n] == (unsigned char)s[n]) n++;
  return !s[n];
}

static int chlist(Exr *e, int pos, int end) {
  e->chlist = pos;
  e->count = 0;
  e->stride = 0;
  while (pos < end && e->b[pos]) {
    int next = text(e, pos);
    if (next < 0 || next + 16 > end) return TEX_ERROR_DATA;
    int type = i32(e->b + next);
    if (type < 0 || type > 2) return TEX_ERROR_DATA;
    if (i32(e->b + next + 8) != 1 || i32(e->b + next + 12) != 1) return TEX_ERROR_SUBSAMPLED;
    e->stride += SIZE[type];
    e->count++;
    pos = next + 16;
  }
  return pos < end ? TEX_OK : TEX_ERROR_DATA;
}

static int attributes(Exr *e, int pos, int first, int *end) {
  while (pos < e->length && e->b[pos]) {
    int a = text(e, pos), b = a < 0 ? -1 : text(e, a);
    if (b < 0 || !fits(e, b, 4)) return TEX_ERROR_DATA;
    int size = i32(e->b + b), at = b + 4;
    if (!fits(e, at, size)) return TEX_ERROR_DATA;
    if (first && named(e, pos, "channels")) {
      int r = chlist(e, at, at + size);
      if (r) return r;
    }
    if (first && named(e, pos, "compression") && size >= 1) e->compression = e->b[at];
    if (first && named(e, pos, "dataWindow") && size >= 16) {
      e->x0 = i32(e->b + at), e->y0 = i32(e->b + at + 4);
      e->width = i32(e->b + at + 8) - e->x0 + 1, e->height = i32(e->b + at + 12) - e->y0 + 1;
    }
    if (first && named(e, pos, "tiles") && size >= 8) e->tx = i32(e->b + at), e->ty = i32(e->b + at + 4);
    if (first && named(e, pos, "type")) {
      e->deep |= starts(e, at, size, "deep");
      if (e->multipart) e->tiled = size == 10 && starts(e, at, size, "tiledimage");
    }
    pos = at + size;
  }
  if (pos >= e->length) return TEX_ERROR_DATA;
  *end = pos + 1;
  return TEX_OK;
}

static int chunk(const Exr *e, int i, Chunk *c) {
  int lo = i32(e->b + e->table + i * 8), hi = i32(e->b + e->table + i * 8 + 4);
  int at = lo + (e->multipart ? 4 : 0);
  if (hi || !fits(e, at, e->tiled ? 20 : 8)) return 0;
  c->x0 = e->tiled ? i32(e->b + at) * e->tx : 0;
  c->y0 = e->tiled ? i32(e->b + at + 4) * e->ty : i32(e->b + at) - e->y0;
  int p = at + (e->tiled ? 16 : 4);
  c->w = e->tiled ? (e->tx < e->width - c->x0 ? e->tx : e->width - c->x0) : e->width;
  int lines = e->tiled ? e->ty : LINES[e->compression];
  c->rows = lines < e->height - c->y0 ? lines : e->height - c->y0;
  if (c->x0 < 0 || c->y0 < 0 || c->w <= 0 || c->rows <= 0) return 0;
  c->size = c->rows * c->w * e->stride;
  c->length = i32(e->b + p);
  c->data = e->b + p + 4;
  if (!fits(e, p + 4, c->length)) return 0;
  int k = e->compression;
  c->kind = k == 0 || c->length == c->size ? 0 : k == 1 ? 1 : k <= 3 ? 2 : k == 4 ? 3 : k == 5 ? 4 : 5;
  return c->kind || c->length >= c->size;
}

static int exr_open(Exr *e, const unsigned char *b, int length) {
  zero(e, sizeof *e);
  e->b = b, e->length = length;
  if (length < 8 || i32(b) != 20000630) return TEX_ERROR_FORMAT;
  int flags = i32(b + 4), pos;
  e->multipart = !!(flags & 0x1000);
  e->tiled = !e->multipart && (flags & 0x200);
  e->deep = !!(flags & 0x800);
  int r = attributes(e, 8, 1, &pos);
  if (r) return r;
  while (e->multipart && pos < length && b[pos]) if ((r = attributes(e, pos, 0, &pos))) return r;
  if (e->multipart) pos++;
  if (e->deep) return TEX_ERROR_DEEP;
  if (e->compression > 9 || e->width <= 0 || e->height <= 0 || e->width > 65536 || e->height > 65536 || !e->count) return TEX_ERROR_DATA;
  if (e->compression >= 8) return TEX_ERROR_COMPRESSION;
  if (e->tiled && (e->tx <= 0 || e->ty <= 0)) return TEX_ERROR_DATA;
  e->chunks = e->tiled ? ((e->width + e->tx - 1) / e->tx) * ((e->height + e->ty - 1) / e->ty) : (e->height + LINES[e->compression] - 1) / LINES[e->compression];
  e->table = pos;
  if (!fits(e, pos, e->chunks * 8)) return TEX_ERROR_DATA;
  e->cap = 1;
  for (int i = 0; i < e->chunks; i++) {
    Chunk c;
    if (!chunk(e, i, &c)) return TEX_ERROR_DATA;
    if (c.size > e->cap) e->cap = c.size;
    if (c.length > e->cap) e->cap = c.length;
  }
  return TEX_OK;
}

static int exr_scratch(const Exr *e) {
  int piz = e->compression == 4 ? ((int)sizeof(Piz) + 15) & ~15 : 0;
  return piz + ((e->count * 8 + 15) & ~15) + ((e->cap + 15) & ~15) * 2;
}

static int find(const Exr *e, char c) {
  int best = -1, pos = e->chlist;
  for (int i = 0; i < e->count; i++) {
    int next = text(e, pos), n = next - 1 - pos;
    const unsigned char *s = e->b + pos;
    if (n == 1 && s[0] == c) return i;
    if (best < 0 && n >= 2 && s[n - 1] == c && s[n - 2] == '.') best = i;
    pos = next + 16;
  }
  return best;
}

int tex_exr_info(const unsigned char *bytes, int length, tex_info *info) {
  Exr e;
  int r = exr_open(&e, bytes, length);
  zero(info, sizeof *info);
  info->compression = e.compression;
  if (r) return r;
  info->width = e.width, info->height = e.height, info->scratch = exr_scratch(&e);
  info->bits = 16;
  for (int i = 0, pos = e.chlist; i < e.count; i++, pos = text(&e, pos) + 16) if (i32(bytes + text(&e, pos)) != 1) info->bits = 32;
  return TEX_OK;
}

int tex_exr_read(const unsigned char *bytes, int length, float *out, void *scratch) {
  Exr e;
  int r = exr_open(&e, bytes, length);
  if (r) return r;
  unsigned char *at = scratch;
  Piz *z = e.compression == 4 ? (Piz *)at : 0;
  if (z) at += ((int)sizeof(Piz) + 15) & ~15;
  int *chans = (int *)at;
  at += (e.count * 8 + 15) & ~15;
  unsigned char *tmp = at, *dst = at + ((e.cap + 15) & ~15);
  for (int i = 0, pos = e.chlist; i < e.count; i++, pos = text(&e, pos) + 16) chans[i * 2] = i32(bytes + text(&e, pos)), chans[i * 2 + 1] = 0;
  int r0 = find(&e, 'R'), y = find(&e, 'Y');
  int rgba[4] = { r0, find(&e, 'G'), find(&e, 'B'), find(&e, 'A') };
  if (r0 < 0 && y >= 0) rgba[0] = rgba[1] = rgba[2] = y;
  for (int k = 0; k < 4; k++) if (rgba[k] >= 0) chans[rgba[k] * 2 + 1] |= 1 << k;
  exr_fill(out, e.width * e.height);
  for (int i = 0; i < e.chunks; i++) {
    Chunk c;
    chunk(&e, i, &c);
    int len = c.length;
    if (c.kind == 2 || c.kind == 4) len = unzlib(c.data, c.length, tmp, e.cap);
    if (len < 0 || !exr_chunk(c.kind, c.data, len, tmp, dst, c.size, chans, e.count, out, e.width, c.x0, c.y0, c.w, c.rows, z)) return TEX_ERROR_DATA;
  }
  return TEX_OK;
}

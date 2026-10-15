#define LIBTEX_IMPLEMENTATION
#include "../libtex.h"

typedef void *(*Get)(unsigned size);

static unsigned fnv(unsigned h, const void *p, unsigned n) {
  const unsigned char *b = (const unsigned char *)p;
  for (unsigned i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
  return h;
}

static void synthetic(float *px, int w, int h) {
  unsigned s = 12345;
  for (int i = 0; i < w * h * 4; i++) {
    s = s * 1664525u + 1013904223u;
    int x = (i / 4) % w, y = (i / 4) / w, c = i % 4;
    float n = (float)(s >> 8) / 16777216.0f;
    px[i] = c == 3 ? (x < w / 2 ? 1.0f : n) : (float)((x * (c + 1) + y * (3 - c)) % 64) / 63.0f * 0.8f + n * 0.2f;
  }
}

static void crop(const float *src, int w, int h, float *dst, int cw, int ch) {
  for (int y = 0; y < ch; y++)
    for (int x = 0; x < cw; x++)
      for (int c = 0; c < 4; c++) dst[(y * cw + x) * 4 + c] = src[((y % h) * w + x % w) * 4 + c];
}

static unsigned codecs(unsigned h, const float *px, int w, int ht, Get get) {
  unsigned char *blocks = (unsigned char *)get(w * ht);
  float *out = (float *)get(w * ht * 16);
  for (int id = TEX_NONE; id <= TEX_BC7; id++)
    for (int q = TEX_BEST; q <= TEX_FAST; q++) {
      tex_options o = { (tex_quality)q, { 0.5f, 1, 0.25f } };
      int n = tex_encode((tex_format)id, &o, px, w, ht, id == TEX_NONE ? 0 : blocks, out);
      h = fnv(h, &n, 4);
      if (id != TEX_NONE) h = fnv(h, blocks, n * tex_block_size((tex_format)id));
      h = fnv(h, out, w * ht * 16);
      double score[4] = { 0 };
      tex_score(px, out, w * ht, 4, id >= TEX_BC6H_UF16 && id <= TEX_BC6H_SF16, &o, score);
      h = fnv(h, score, sizeof score);
    }
  return h;
}

static unsigned ops(unsigned h, const float *px, int w, int ht, Get get) {
  int n = w * ht;
  float *a = (float *)get(n * 16 * 4);
  tex_mips(px, w, ht, a, get(tex_mips_scratch(w, ht)));
  h = fnv(h, a, n * 16 / 3);
  for (int f = 0; f < 3; f++) {
    int nw = w * 3 / 2 + 1, nh = ht / 2 + 1;
    tex_resize(px, w, ht, a, nw, nh, f, get(tex_resize_scratch(w, ht, nw, nh, f)));
    h = fnv(h, a, nw * nh * 16);
  }
  for (int op = 0; op < 5; op++) {
    tex_orient(px, w, ht, a, op);
    h = fnv(h, a, n * 16);
  }
  copy(a, px, n * 16);
  int src[4] = { 2, 1, 0, 3 };
  float scale[4] = { 1, -1, 1, 1 }, offset[4] = { 0, 1, 0, 0 };
  tex_swizzle(a, n, src, scale, offset);
  tex_premultiply(a, n, 0);
  h = fnv(h, a, n * 16);
  tex_premultiply(a, n, 1);
  h = fnv(h, a, n * 16);
  unsigned char *b = (unsigned char *)get(n * 8 + ht * 8);
  int exact = tex_to_bytes(px, n * 4, b);
  h = fnv(h, &exact, 4);
  h = fnv(h, b, n * 4);
  tex_from_bytes(b, n * 4, a);
  h = fnv(h, a, n * 16);
  int gray = tex_tga_gray(px, n);
  int len = tex_tga_pixels(px, w, ht, 4, 1, b, b + n * 6);
  h = fnv(h, &gray, 4);
  h = fnv(h, b, len);
  len = tex_tga_header(w, ht, 4, 1, b);
  h = fnv(h, b, len);
  unsigned short *halves = (unsigned short *)get(n * 2);
  tex_half_plane(px, n, 1, halves);
  return fnv(h, halves, n * 2);
}

static unsigned hash_run(const unsigned char *file, int length, int kind, Get get) {
  int w = 37, ht = 29;
  float *px = 0;
  unsigned h = 2166136261u;
  if (file) {
    tex_info info;
    int e = kind == 0 ? tex_png_info(file, length, &info) : kind == 1 ? tex_tga_info(file, length, &info) : tex_exr_info(file, length, &info);
    h = fnv(h, &e, 4);
    if (e) return h;
    h = fnv(h, &info, sizeof info);
    w = info.width, ht = info.height;
    px = (float *)get(w * ht * 16);
    void *scratch = get(info.scratch);
    e = kind == 0 ? tex_png_read(file, length, px, scratch) : kind == 1 ? tex_tga_read(file, length, px, scratch) : tex_exr_read(file, length, px, scratch);
    h = fnv(h, &e, 4);
    if (e) return h;
    h = fnv(h, px, w * ht * 16);
  } else {
    px = (float *)get(w * ht * 16);
    synthetic(px, w, ht);
  }
  float *c = (float *)get(64 * 64 * 16);
  crop(px, w, ht, c, 64, 64);
  h = codecs(h, c, 64, 64, get);
  int ow = w < 128 ? w : 128, oh = ht < 96 ? ht : 96;
  float *o = (float *)get(ow * oh * 16);
  crop(px, w, ht, o, ow, oh);
  return ops(h, o, ow, oh, get);
}

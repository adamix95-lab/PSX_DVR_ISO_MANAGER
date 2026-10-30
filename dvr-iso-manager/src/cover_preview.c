#include "cover_preview.h"
#include "cover_index.h"

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <debug.h>
#include <png.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* PS2 debug framebuffer is GS-owned. Do not initialize gsKit here:
   it would replace the known-working MONO display setup.
   Each 7x8 MSX glyph #219 becomes one colored preview pixel. */
#define PREVIEW_W 11
#define PREVIEW_H 14
#define MAX_COVER_W 512
#define MAX_COVER_H 512

typedef struct {
  uint32_t r, g, b, n;
} color_accum;
typedef struct { int fd; } png_stream;
static color_accum sample[PREVIEW_W * PREVIEW_H];
static unsigned char scanline[MAX_COVER_W * 4];

static void read_png(png_structp png, png_bytep data, png_size_t bytes) {
  png_stream *s = (png_stream *)png_get_io_ptr(png);
  while (bytes) {
    int want = bytes > 1024 ? 1024 : (int)bytes;
    int n = fileXioRead(s->fd, data, want);
    if (n <= 0) png_error(png, "USB cover read failed");
    data += n;
    bytes -= (png_size_t)n;
  }
}

int desr_cover_preview_png(const char *game_id, const char *ext) {
  char path[80];
  png_structp png = NULL;
  png_infop info = NULL;
  png_uint_32 w, h, y;
  int bit_depth, color_type, channels, fd, gx, gy, x, index, n;
  png_stream stream;
  if (!desr_valid_game_id(game_id) || !ext || strcmp(ext, "png"))
    return 0;
  n = snprintf(path, sizeof(path),
               "mass0:/DESR_DVR/ART/%s_COV.png", game_id);
  if (n < 0 || n >= (int)sizeof(path)) return 0;
  fd = fileXioOpen(path, FIO_O_RDONLY, 0);
  if (fd < 0) return 0;

  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) { fileXioClose(fd); return 0; }
  info = png_create_info_struct(png);
  if (!info) { png_destroy_read_struct(&png, NULL, NULL);
               fileXioClose(fd); return 0; }
  /* libpng uses longjmp on corrupt input. Static scanline means no
     partially allocated image buffer survives an aborted decode. */
  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fileXioClose(fd);
    return 0;
  }
  stream.fd = fd;
  png_set_read_fn(png, &stream, read_png);
  png_read_info(png, info);
  w = png_get_image_width(png, info);
  h = png_get_image_height(png, info);
  if (!w || !h || w > MAX_COVER_W || h > MAX_COVER_H) {
    png_destroy_read_struct(&png, &info, NULL);
    fileXioClose(fd);
    return 0;
  }
  bit_depth = png_get_bit_depth(png, info);
  color_type = png_get_color_type(png, info);
  if (bit_depth == 16) png_set_strip_16(png);
  if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);
  if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
  if (!(color_type & PNG_COLOR_MASK_ALPHA) &&
      !png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_filler(png, 0xff, PNG_FILLER_AFTER);
  png_set_interlace_handling(png);
  png_read_update_info(png, info);
  channels = png_get_channels(png, info);
  if (channels != 4 || png_get_rowbytes(png, info) != w * 4) {
    png_destroy_read_struct(&png, &info, NULL);
    fileXioClose(fd);
    return 0;
  }

  memset(sample, 0, sizeof(sample));
  for (y = 0; y < h; y++) {
    png_read_row(png, scanline, NULL);
    gy = (int)(y * PREVIEW_H / h);
    for (x = 0; x < (int)w; x++) {
      const unsigned char *p = scanline + x * 4;
      gx = (int)((png_uint_32)x * PREVIEW_W / w);
      index = gy * PREVIEW_W + gx;
      /* Composite alpha against the black MONO background. */
      sample[index].r += (uint32_t)p[0] * p[3] / 255;
      sample[index].g += (uint32_t)p[1] * p[3] / 255;
      sample[index].b += (uint32_t)p[2] * p[3] / 255;
      sample[index].n++;
    }
  }
  png_read_end(png, NULL);
  png_destroy_read_struct(&png, &info, NULL);
  fileXioClose(fd);

  for (gy = 0; gy < PREVIEW_H; gy++) {
    for (gx = 0; gx < PREVIEW_W; gx++) {
      color_accum *p = &sample[gy * PREVIEW_W + gx];
      uint32_t color;
      if (!p->n) continue;
      color = ((p->r / p->n) << 16) |
              ((p->g / p->n) << 8) | (p->b / p->n);
      scr_putchar(62 * 7 + gx * 7, 8 * 8 + gy * 8, color, 219);
    }
  }
  return 1;
}

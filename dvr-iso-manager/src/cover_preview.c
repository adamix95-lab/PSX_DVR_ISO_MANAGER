#include "cover_preview.h"
#include "cover_index.h"

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <debug.h>
#include <ee_regs.h>
#include <kernel.h>
#include <png.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * OPL-like cover preview for the EXISTING PS2SDK MONO debug screen.
 *
 * PS2SDK ee/debug/src/scr_printf.c configures a 640x224 PSMCT32 framebuffer,
 * and scr_putchar uploads each 8x8 glyph through GIF DMA channel 2.
 * We use exactly that existing GS BITBLTBUF/TRXPOS/TRXREG/TRXDIR layout, but
 * upload one bounded 88x128 RGBA image instead of 11x14 solid MSX glyphs.
 *
 * Never call gsKit_init_screen, SetGsCrt or reset DMA/GS here: the working
 * console screen, IRX stack, ISO index and Quickboot remain untouched.
 *
 * Position: 62 text columns * 7 px, y = 8 rows * 8 px, end at y=192.
 * Debug footer starts at y=192, so cover pixels cannot overlap it.
 */
#define PREVIEW_W 88
#define PREVIEW_H 128
#define PREVIEW_X (62 * 7)
#define PREVIEW_Y (8 * 8)
#define PREVIEW_QWC (PREVIEW_W * PREVIEW_H * 4 / 16)
#define MAX_COVER_W 512
#define MAX_COVER_H 512

typedef struct { int fd; } png_stream;

/* Clone of the 6-QWC PS2SDK debug GIF blit setup, with variable transfer
 * dimensions and image packet length. The buffer layout is ABI-sensitive. */
typedef struct {
  uint64_t dd0[4];
  uint32_t dw0;
  uint16_t x, y;
  uint64_t dd1;
  uint32_t width, height;
  uint64_t dd2[5];
} cover_blit_setup;
typedef char cover_blit_setup_is_96_bytes[(sizeof(cover_blit_setup)==96)?1:-1];

static cover_blit_setup cover_setup_packet __attribute__((aligned(16)));
static uint32_t pixels[PREVIEW_W * PREVIEW_H] __attribute__((aligned(16)));
static unsigned char scanline[MAX_COVER_W * 4];

static const cover_blit_setup setup_template = {
  {0x1000000000000004ULL, 0xEULL, 0xA000000000000ULL, 0x50ULL},
  0, PREVIEW_X, PREVIEW_Y,
  0x51ULL, PREVIEW_W, PREVIEW_H,
  {0x52ULL, 0ULL, 0x53ULL,
   0x0800000000008000ULL | PREVIEW_QWC, 0ULL}
};

static void cover_dma_wait(void) {
  while ((*R_EE_D2_CHCR & 0x100) != 0) { }
}
static void cover_dma_submit(const void *buf, int qwc) {
  *R_EE_D2_QWC = (uint32_t)qwc;
  *R_EE_D2_MADR = (uint32_t)(uintptr_t)buf;
  *R_EE_D2_CHCR = 0x101; /* normal DMA: memory -> GIF */
}

static void cover_blit(void) {
  /* Existing scr_putchar also writes packets via UNCACHED_SEG. This
   * guarantees DMA sees the exact setup and pixels without cache flushes. */
  *(cover_blit_setup *)UNCACHED_SEG(&cover_setup_packet) = setup_template;
  cover_dma_wait();
  cover_dma_submit(&cover_setup_packet, 6);
  cover_dma_wait();
  cover_dma_submit(pixels, PREVIEW_QWC);
  cover_dma_wait();
}

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
  int bit_depth, color_type, channels, fd, next_y, x, n;
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
  if (!info) {
    png_destroy_read_struct(&png, NULL, NULL);
    fileXioClose(fd);
    return 0;
  }
  /* Never modify the displayed cover until the entire PNG decodes.
   * Invalid/truncated PNGs take the same text fallback as in v0.5. */
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
  if (!w || !h || w > MAX_COVER_W || h > MAX_COVER_H ||
      png_get_interlace_type(png, info) != PNG_INTERLACE_NONE) {
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
  png_read_update_info(png, info);
  channels = png_get_channels(png, info);
  if (channels != 4 || png_get_rowbytes(png, info) != w * 4) {
    png_destroy_read_struct(&png, &info, NULL);
    fileXioClose(fd);
    return 0;
  }

  /* Bounded nearest-neighbor scaling while decoding. No full-size source
   * bitmap allocation: at most 2 KB of PNG scanline + 44 KB of output.
   * Output is prebuilt in uncached RAM before any GS DMA is issued.
   * Center-of-source samples avoid the old 11x14 mosaic. */
  next_y = 0;
  for (y = 0; y < h; y++) {
    png_read_row(png, scanline, NULL);
    while (next_y < PREVIEW_H &&
           ((uint32_t)next_y * h + PREVIEW_H / 2) / PREVIEW_H == y) {
      for (x = 0; x < PREVIEW_W; x++) {
        uint32_t sx = ((uint32_t)x * w + PREVIEW_W / 2) / PREVIEW_W;
        const unsigned char *p;
        uint32_t color;
        if (sx >= w) sx = w - 1;
        p = scanline + sx * 4;
        /* Composite RGBA against original black UI background. */
        color = (((uint32_t)p[0] * p[3] / 255) << 16) |
                (((uint32_t)p[1] * p[3] / 255) << 8) |
                 ((uint32_t)p[2] * p[3] / 255);
        *(uint32_t *)UNCACHED_SEG(&pixels[next_y * PREVIEW_W + x]) = color;
      }
      next_y++;
    }
  }
  png_read_end(png, NULL);
  png_destroy_read_struct(&png, &info, NULL);
  fileXioClose(fd);
  if (next_y != PREVIEW_H) return 0;
  cover_blit();
  return 1;
}

#ifndef DESR_COVER_COLOR_H
#define DESR_COVER_COLOR_H

#include <stdint.h>

/* Portable RGB conversion for the PS2 GS PSMCT32 framebuffer.
   GS bytes (little endian): R, G, B, A. Return A=0 to match scr_putchar.
   Bilinear filtering uses 16-bit fractional x/y; PNG alpha is composited
   against the existing black MONO background before interpolation. */
uint32_t desr_gs_bilinear_rgba_black(
  const unsigned char *p00, const unsigned char *p01,
  const unsigned char *p10, const unsigned char *p11,
  uint32_t wx, uint32_t wy);

#endif

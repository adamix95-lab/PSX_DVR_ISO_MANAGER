#include "cover_color.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
  const unsigned char red[4] = {255,0,0,255};
  const unsigned char green[4] = {0,255,0,255};
  const unsigned char blue[4] = {0,0,255,255};
  const unsigned char black[4] = {0,0,0,255};
  const unsigned char transparent_red[4] = {255,0,0,0};
  const unsigned char half_red[4] = {255,0,0,128};

  /* Critical GS channel order: R low, G middle, B high. */
  assert(desr_gs_bilinear_rgba_black(red,red,red,red,0,0) == 0x000000ffU);
  assert(desr_gs_bilinear_rgba_black(green,green,green,green,0,0) == 0x0000ff00U);
  assert(desr_gs_bilinear_rgba_black(blue,blue,blue,blue,0,0) == 0x00ff0000U);
  assert(desr_gs_bilinear_rgba_black(transparent_red,transparent_red,
                                    transparent_red,transparent_red,0,0) == 0);
  assert(desr_gs_bilinear_rgba_black(half_red,half_red,half_red,half_red,0,0) == 128);
  /* Bilinear averaging (smoother than v0.5.1 nearest-neighbor). */
  assert(desr_gs_bilinear_rgba_black(red,blue,red,blue,32768,0) == 0x00800080U);
  assert(desr_gs_bilinear_rgba_black(red,red,blue,blue,0,32768) == 0x00800080U);
  assert(desr_gs_bilinear_rgba_black(red,black,black,black,32768,32768) == 64);
  puts("OK: GS RGB order, transparency, and 2D bilinear downsampling");
  return 0;
}

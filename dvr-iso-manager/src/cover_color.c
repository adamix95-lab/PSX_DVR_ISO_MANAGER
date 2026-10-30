#include "cover_color.h"

/* The GS expects R in bits 0..7 and B in 16..23.
   v0.5.1 originally packed 0xRRGGBB, accidentally swapping R and B. */
static uint32_t premultiplied(const unsigned char *p, int channel) {
  return ((uint32_t)p[channel] * (uint32_t)p[3] + 127U) / 255U;
}
static uint32_t lerp16(uint32_t a, uint32_t b, uint32_t fraction) {
  return (a * (65536U - fraction) + b * fraction + 32768U) >> 16;
}
uint32_t desr_gs_bilinear_rgba_black(
  const unsigned char *p00, const unsigned char *p01,
  const unsigned char *p10, const unsigned char *p11,
  uint32_t wx, uint32_t wy) {
  uint32_t color = 0, channel;
  if (!p00 || !p01 || !p10 || !p11 ||
      wx > 65535U || wy > 65535U) return 0;
  for (channel = 0; channel < 3; channel++) {
    uint32_t top = lerp16(premultiplied(p00, (int)channel),
                          premultiplied(p01, (int)channel), wx);
    uint32_t bottom = lerp16(premultiplied(p10, (int)channel),
                             premultiplied(p11, (int)channel), wx);
    color |= lerp16(top, bottom, wy) << (channel * 8);
  }
  return color;
}

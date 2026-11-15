#include "n6_normalize_png.h"
#include "probe_png.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Real 1x1 RGBA PNG, valid zlib and CRC (same fixture as test_probe_png). */
static const uint8_t src[] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,
0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,
0x08,0x06,0x00,0x00,0x00,0x1f,0x15,0xc4,
0x89,0x00,0x00,0x00,0x0d,0x49,0x44,0x41,
0x54,0x78,0x9c,0x63,0x10,0x54,0x32,0xfe,
0x0f,0x00,0x02,0x14,0x01,0x66,0x04,0xe0,
0xde,0x7f,0x00,0x00,0x00,0x00,0x49,0x45,
0x4e,0x44,0xae,0x42,0x60,0x82
};
int main(void) {
  const uint8_t *out=NULL;
  size_t n=0;
  uint8_t corrupt[sizeof(src)];
#ifdef N6G_PNG_WORKSPACE
  uint8_t wire[4096];
  memcpy(wire+128,src,sizeof(src));
  assert(n6_normalize_png_into(wire+128,sizeof(src),wire,sizeof(wire),&n)==0);
  out=wire;
#else
  assert(probe_png_check(src,sizeof(src))==0);
  assert(n6_normalize_png(src,sizeof(src),&out,&n)==0);
#endif
  assert(out && n>=57 && n<256*1024);
  assert(probe_png_check(out,n)==0);
  assert(out[12]=='I'&&out[13]=='H'&&out[14]=='D'&&out[15]=='R');
  assert(out[16]==0&&out[17]==0&&out[18]==0&&out[19]==140);
  assert(out[20]==0&&out[21]==0&&out[22]==0&&out[23]==200);
  assert(out[24]==8 && out[25]==2);
  memcpy(corrupt,src,sizeof(src));
  corrupt[50]^=1;
  out=NULL;n=0;
#ifdef N6G_PNG_WORKSPACE
  assert(n6_normalize_png_into(corrupt,sizeof(corrupt),wire,sizeof(wire),&n)<0);
  assert(n==0);
  assert(n6_normalize_png_into(src,sizeof(src),wire,16,&n)<0);
  assert(n==0);
  assert(n6_normalize_png_into(src,sizeof(src),wire,sizeof(wire),&n)==0);
  assert(probe_png_check(wire,n)==0);
#else
  assert(n6_normalize_png(corrupt,sizeof(corrupt),&out,&n)<0);
  assert(!out && n==0);
#endif
  printf("N6 NORMALIZE RGB 140x200 / PNG CRC PASS\n");
  return 0;
}

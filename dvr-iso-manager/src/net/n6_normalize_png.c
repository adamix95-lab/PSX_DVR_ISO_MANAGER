/* N6: PS2-safe RGB PNG normalizer for OPL and CosmicScale artwork.
 * Same output geometry/color mode as PC ART Server 0.8: 140x200 RGB.
 * Aspect fit on black canvas, no source stretching. Uses nearest-neighbor
 * resampling (not Pillow LANCZOS); avoids >1MB source allocations.
 * Source CRC, length, chunk structure checked before libpng decode.
 * No filesystem or network calls. Caller writes verified result to USB.
 */
#include "n6_normalize_png.h"
#include "probe_png.h"
#include <png.h>
#include <setjmp.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define TARGET_W 140U
#define TARGET_H 200U
#define MAX_W 512U
#define MAX_H 512U
#define OUT_CAP (256U*1024U)

typedef struct {const uint8_t *p;size_t size,at;} png_reader;
typedef struct {uint8_t *p;size_t cap,at;} png_writer;
static uint8_t scan_row[MAX_W*4U];
static uint8_t image_rgb[TARGET_W*TARGET_H*3U];
static uint8_t png_output[OUT_CAP];

static void on_read(png_structp png,png_bytep dst,png_size_t n) {
  png_reader *r=(png_reader*)png_get_io_ptr(png);
  if(!r||n>r->size-r->at)png_error(png,"N6 truncated input");
  memcpy(dst,r->p+r->at,n);
  r->at+=n;
}
static void on_write(png_structp png,png_bytep src,png_size_t n) {
  png_writer *w=(png_writer*)png_get_io_ptr(png);
  if(!w||n>w->cap-w->at)png_error(png,"N6 output overflow");
  memcpy(w->p+w->at,src,n);
  w->at+=n;
}
static void on_flush(png_structp png) {(void)png;}

int n6_normalize_png(const uint8_t *input,size_t bytes,
                     const uint8_t **output,size_t *output_len) {
  png_structp rd=NULL,wr=NULL;
  png_infop rinfo=NULL,winfo=NULL;
  png_reader reader={input,bytes,0U};
  png_writer writer={png_output,sizeof(png_output),0U};
  png_uint_32 width,height;
  int depth,color,channels;
  unsigned fit_w,fit_h,off_x,off_y;
  volatile int valid=0; /* defined across libpng longjmp error paths */
  if(!output||!output_len)return -1;
  *output=NULL;*output_len=0;
  if(probe_png_check(input,bytes)!=0)return -2;
  rd=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if(!rd)return -3;
  rinfo=png_create_info_struct(rd);
  if(!rinfo)goto done;
  if(setjmp(png_jmpbuf(rd)))goto done;
  png_set_read_fn(rd,&reader,on_read);
  png_read_info(rd,rinfo);
  width=png_get_image_width(rd,rinfo);
  height=png_get_image_height(rd,rinfo);
  if(!width||!height||width>MAX_W||height>MAX_H ||
     png_get_interlace_type(rd,rinfo)!=PNG_INTERLACE_NONE)goto done;
  depth=png_get_bit_depth(rd,rinfo);
  color=png_get_color_type(rd,rinfo);
  if(depth==16)png_set_strip_16(rd);
  if(color==PNG_COLOR_TYPE_PALETTE)png_set_palette_to_rgb(rd);
  if(color==PNG_COLOR_TYPE_GRAY ||
     color==PNG_COLOR_TYPE_GRAY_ALPHA)png_set_gray_to_rgb(rd);
  if(png_get_valid(rd,rinfo,PNG_INFO_tRNS))png_set_tRNS_to_alpha(rd);
  if(!(color & PNG_COLOR_MASK_ALPHA) &&
     !png_get_valid(rd,rinfo,PNG_INFO_tRNS))
    png_set_filler(rd,0xff,PNG_FILLER_AFTER);
  png_read_update_info(rd,rinfo);
  channels=png_get_channels(rd,rinfo);
  if(channels!=4 || png_get_rowbytes(rd,rinfo)!=width*4U)goto done;
  memset(image_rgb,0,sizeof(image_rgb));
  if((uint64_t)width*TARGET_H>=(uint64_t)height*TARGET_W) {
    fit_w=TARGET_W;
    fit_h=(unsigned)(((uint64_t)height*TARGET_W+width/2U)/width);
  } else {
    fit_h=TARGET_H;
    fit_w=(unsigned)(((uint64_t)width*TARGET_H+height/2U)/height);
  }
  if(!fit_w)fit_w=1;
  if(!fit_h)fit_h=1;
  if(fit_w>TARGET_W||fit_h>TARGET_H)goto done;
  off_x=(TARGET_W-fit_w)/2U;
  off_y=(TARGET_H-fit_h)/2U;
  /* Read every source scanline once. A source row can map to many output
   * rows. Avoid keeping the full image or decoding unbounded dimensions. */
  for(unsigned y=0,next=0;y<(unsigned)height;y++) {
    png_read_row(rd,scan_row,NULL);
    while(next<fit_h && ((uint64_t)next*height)/fit_h==y) {
      uint8_t *dst=image_rgb+((size_t)(off_y+next)*TARGET_W+off_x)*3U;
      for(unsigned x=0;x<fit_w;x++) {
        unsigned srcx=(unsigned)(((uint64_t)x*width)/fit_w);
        const uint8_t *p=scan_row+(size_t)srcx*4U;
        unsigned alpha=p[3];
        dst[x*3U+0U]=(uint8_t)((p[0]*alpha+127U)/255U);
        dst[x*3U+1U]=(uint8_t)((p[1]*alpha+127U)/255U);
        dst[x*3U+2U]=(uint8_t)((p[2]*alpha+127U)/255U);
      }
      next++;
    }
  }
  png_read_end(rd,NULL);
  png_destroy_read_struct(&rd,&rinfo,NULL);
  rd=NULL;rinfo=NULL;
  wr=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if(!wr)goto done;
  winfo=png_create_info_struct(wr);
  if(!winfo)goto done;
  if(setjmp(png_jmpbuf(wr)))goto done;
  png_set_write_fn(wr,&writer,on_write,on_flush);
  png_set_IHDR(wr,winfo,TARGET_W,TARGET_H,8,
               PNG_COLOR_TYPE_RGB,PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE,PNG_FILTER_TYPE_BASE);
  png_set_compression_level(wr,4);
  png_write_info(wr,winfo);
  for(unsigned y=0;y<TARGET_H;y++)
    png_write_row(wr,image_rgb+(size_t)y*TARGET_W*3U);
  png_write_end(wr,NULL);
  if(probe_png_check(png_output,writer.at)!=0)goto done;
  *output=png_output;
  *output_len=writer.at;
  valid=1;
done:
  if(rd)png_destroy_read_struct(&rd,&rinfo,NULL);
  if(wr)png_destroy_write_struct(&wr,&winfo);
  return valid?0:-4;
}

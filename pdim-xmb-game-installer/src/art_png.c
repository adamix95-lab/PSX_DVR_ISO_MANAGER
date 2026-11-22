/* Convert the PDIM HTTPS 140x200 RGB/RGBA artwork to indexed PNG8 XMB jackets.
 * Uses the same PS2SDK libpng as the hardware-tested PDIM cover preview.
 * This never modifies PP.DVRISO's source ART image.
 */
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include "sg_installer.h"
#define SRC_W 140
#define SRC_H 200
static unsigned char rgba[SRC_H][SRC_W*4] __attribute__((aligned(64)));
static unsigned char indices[SRC_W] __attribute__((aligned(64)));

typedef struct {int fd;} png_file;

static void read_bytes(png_structp png,png_bytep p,png_size_t len) {
  png_file *s=(png_file*)png_get_io_ptr(png);
  while(len>0) {
    int n=(int)(len>4096?4096:len);
    int got=fileXioRead(s->fd,p,n);
    if(got<=0||got>n)png_error(png,"truncated art source");
    p+=got;len-=got;
  }
}
static void write_bytes(png_structp png,png_bytep p,png_size_t len) {
  png_file *s=(png_file*)png_get_io_ptr(png);
  while(len>0) {
    int n=(int)(len>4096?4096:len);
    int got=fileXioWrite(s->fd,p,n);
    if(got<=0||got>n)png_error(png,"PFS art write failure");
    p+=got;len-=got;
  }
}
static void flush_bytes(png_structp png) {(void)png;}
static int decode_art(const char *name) {
  int fd=fileXioOpen(name,FIO_O_RDONLY,0);
  if(fd<0)return -1;
  png_structp png=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if(!png){fileXioClose(fd);return -1;}
  png_infop info=png_create_info_struct(png);
  if(!info){png_destroy_read_struct(&png,NULL,NULL);fileXioClose(fd);return -1;}
  png_file f={fd};
  if(setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png,&info,NULL);
    fileXioClose(fd);
    return -1;
  }
  png_set_read_fn(png,&f,read_bytes);
  png_read_info(png,info);
  png_uint_32 w=png_get_image_width(png,info);
  png_uint_32 h=png_get_image_height(png,info);
  int color=png_get_color_type(png,info),bit=png_get_bit_depth(png,info);
  if(w!=SRC_W||h!=SRC_H||png_get_interlace_type(png,info)!=PNG_INTERLACE_NONE)
    png_error(png,"expected normalized 140x200 PNG");
  if(bit==16)png_set_strip_16(png);
  if(color==PNG_COLOR_TYPE_PALETTE)png_set_palette_to_rgb(png);
  if(color==PNG_COLOR_TYPE_GRAY && bit<8)png_set_expand_gray_1_2_4_to_8(png);
  if(png_get_valid(png,info,PNG_INFO_tRNS))png_set_tRNS_to_alpha(png);
  if(color==PNG_COLOR_TYPE_GRAY||color==PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);
  if(!(color&PNG_COLOR_MASK_ALPHA) && !png_get_valid(png,info,PNG_INFO_tRNS))
    png_set_filler(png,0xff,PNG_FILLER_AFTER);
  png_read_update_info(png,info);
  if(png_get_channels(png,info)!=4||png_get_rowbytes(png,info)!=SRC_W*4)
    png_error(png,"unexpected channels");
  png_bytep rows[SRC_H];
  for(int y=0;y<SRC_H;y++)rows[y]=rgba[y];
  png_read_image(png,rows);
  png_read_end(png,NULL);
  png_destroy_read_struct(&png,&info,NULL);
  return fileXioClose(fd)<0?-1:0;
}
static int encode_png8(const char *path,int width,int height,
                       uint32_t *result_crc,uint32_t *result_size) {
  if(width>SRC_W||height>SRC_H||width<=0||height<=0)return -1;
  int fd=fileXioOpen(path,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_EXCL,0666);
  if(fd<0)return -1;
  png_structp png=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if(!png){fileXioClose(fd);return -1;}
  png_infop info=png_create_info_struct(png);
  if(!info){png_destroy_write_struct(&png,NULL);fileXioClose(fd);return -1;}
  png_file f={fd};
  if(setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png,&info);
    fileXioClose(fd);
    return -1; /* failed partial destination is never reused by stage 2 */
  }
  png_set_write_fn(png,&f,write_bytes,flush_bytes);
  png_set_IHDR(png,info,width,height,8,PNG_COLOR_TYPE_PALETTE,
               PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_BASE,PNG_FILTER_TYPE_BASE);
  png_color palette[256];
  for(int i=0;i<256;i++) {
    /* deterministic 3-3-2 palette; no unsafe dynamic allocations */
    palette[i].red=(png_byte)(((i>>5)&7)*255/7);
    palette[i].green=(png_byte)(((i>>2)&7)*255/7);
    palette[i].blue=(png_byte)((i&3)*255/3);
  }
  png_set_PLTE(png,info,palette,256);
  png_write_info(png,info);
  for(int y=0;y<height;y++) {
    int src_y=(y*SRC_H)/height;
    for(int x=0;x<width;x++) {
      int src_x=(x*SRC_W)/width;
      const unsigned char *p=&rgba[src_y][src_x*4];
      indices[x]=(unsigned char)((p[0]&0xe0)|((p[1]>>3)&0x1c)|(p[2]>>6));
    }
    png_write_row(png,indices);
  }
  png_write_end(png,info);
  png_destroy_write_struct(&png,&info);
  if(fileXioClose(fd)<0||fileXioSync("pfs0:",FXIO_WAIT)<0)return -1;
  if(sg_crc_file(path,result_size,result_crc)<0||*result_size<100)return -1;
  return 0;
}
int sg_png_make_pair(const char *source,uint32_t *crc1,uint32_t *len1,uint32_t *crc2,uint32_t *len2) {
  if(!source||!crc1||!len1||!crc2||!len2)return -1;
  if(decode_art(source)<0)return -1;
  if(encode_png8("pfs0:/res/jkt_001.png",140,200,crc1,len1)<0)return -1;
  if(encode_png8("pfs0:/res/jkt_002.png",74,108,crc2,len2)<0)return -1;
  return 0;
}

#include "probe_png.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Real 1x1 RGBA PNG with correct IDAT zlib bytes and PNG CRC32 checksums. */
static const uint8_t png[] = {
0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4,
0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41,
0x54, 0x78, 0x9c, 0x63, 0x10, 0x54, 0x32, 0xfe,
0x0f, 0x00, 0x02, 0x14, 0x01, 0x66, 0x04, 0xe0,
0xde, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};
int main(void) {
  uint8_t response[1024];
  const uint8_t *image;
  size_t size, n;
  char hdr[256];
  assert(sizeof(png)==70);
  assert(probe_png_check(png,sizeof(png))==0);
  n=snprintf(hdr,sizeof(hdr),
             "HTTP/1.0 200 OK\r\n"
             "Server: SimpleHTTP/0.6 Python/3.12\r\n"
             "Date: Fri, 30 Oct 2026 14:00:00 GMT\r\n"
             "Content-type: image/png\r\n"
             "Content-Length: %u\r\n"
             "Last-Modified: Fri, 30 Oct 2026 13:00:00 GMT\r\n\r\n",
             (unsigned)sizeof(png));
  assert(n<sizeof(hdr));
  memcpy(response,hdr,n);
  memcpy(response+n,png,sizeof(png));
  assert(probe_http_extract_png(response,n+sizeof(png),&image,&size)==0);
  assert(image==response+n && size==sizeof(png));
  response[n+55]^=1;
  assert(probe_http_extract_png(response,n+sizeof(png),&image,&size)<0);
  response[n+55]^=1;
  assert(probe_http_extract_png(response,n+sizeof(png)-1,&image,&size)<0);
  memcpy(response,"HTTP/1.0 404",12);
  assert(probe_http_extract_png(response,n+sizeof(png),&image,&size)<0);
  assert(probe_png_check(png,sizeof(png)-1)<0);
  puts("OK: complete HTTP Content-Length and CRC-validated PNG, strict failures");
  return 0;
}

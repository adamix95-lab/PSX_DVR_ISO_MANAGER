#include "v12_roughtime_wire.h"
#include <string.h>

static uint32_t le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
         ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static void put_le32(uint8_t *p,uint32_t v) {
  p[0]=(uint8_t)v;
  p[1]=(uint8_t)(v>>8);
  p[2]=(uint8_t)(v>>16);
  p[3]=(uint8_t)(v>>24);
}
static uint32_t tag(const char *s) {
  return ((uint32_t)(uint8_t)s[0]) |
         ((uint32_t)(uint8_t)s[1]<<8) |
         ((uint32_t)(uint8_t)s[2]<<16) |
         ((uint32_t)(uint8_t)s[3]<<24);
}
int v12_rt_make_request(uint8_t *out,size_t cap,const uint8_t nonce[32]) {
  size_t i;
  int nonzero=0;
  if(!out || !nonce || cap<V12_RT_REQUEST_BYTES) return -1;
  for(i=0;i<32;i++) nonzero|=nonce[i];
  if(!nonzero) return -1;
  memset(out,0,V12_RT_REQUEST_BYTES);
  memcpy(out,"ROUGHTIM",8);
  put_le32(out+8,V12_RT_REQUEST_BYTES-12U);
  /* Draft-ietf-ntp-roughtime-08, 3 tags (VER, NONC, ZZZZ), 1024 bytes.
   * Matches the public Cloudflare protocol vectors (Apache 2.0).
   */
  put_le32(out+12,3U);
  put_le32(out+16,4U);   /* VER length */
  put_le32(out+20,36U);  /* VER + NONC length */
  memcpy(out+24,"VER\0",4);
  memcpy(out+28,"NONC",4);
  memcpy(out+32,"ZZZZ",4);
  put_le32(out+36,UINT32_C(0x80000008));
  memcpy(out+40,nonce,32);
  return 0;
}
int v12_rt_check_reply_frame(const uint8_t *packet,size_t size,
                            unsigned *tag_count) {
  uint32_t n,i,offset=0,prev=0;
  size_t msgsize,base;
  int found_sig=0,found_srep=0,found_cert=0;
  if(tag_count) *tag_count=0;
  if(!packet || size<20 || size>V12_RT_REPLY_MAX_BYTES ||
     memcmp(packet,"ROUGHTIM",8)) return -1;
  if((size-12U)%4U || le32(packet+8)!=size-12U) return -1;
  msgsize=size-12U;
  n=le32(packet+12);
  if(n<1U || n>32U || (size_t)n>msgsize/8U) return -1;
  base=12U+8U*(size_t)n; /* start of values after n tags */
  if(base>size) return -1;
  for(i=0;i<n;i++) {
    uint32_t current,next;
    uint32_t tagv;
    size_t length;
    tagv=le32(packet+12U+4U+(size_t)(n-1U)*4U+(size_t)i*4U);
    if(i && tagv<=prev) return -1;
    prev=tagv;
    current=offset;
    next=(i+1U<n)?le32(packet+16U+(size_t)i*4U):
                     (uint32_t)(size-base);
    if(next<current || (next&3U) || next>size-base) return -1;
    length=(size_t)(next-current);
    if(tagv==tag("SIG\0")) {
      if(length!=64U) return -1;
      found_sig=1;
    } else if(tagv==tag("SREP")) {
      if(length<12U) return -1;
      found_srep=1;
    } else if(tagv==tag("CERT")) {
      if(length<12U) return -1;
      found_cert=1;
    }
    offset=next;
  }
  if(!found_sig || !found_srep || !found_cert) return -1;
  if(tag_count) *tag_count=n;
  return 0;
}

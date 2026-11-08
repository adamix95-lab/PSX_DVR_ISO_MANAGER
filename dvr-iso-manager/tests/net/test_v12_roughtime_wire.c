#include "v12_roughtime_wire.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put32(uint8_t *p,uint32_t v) {
  p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
  p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static void build_reply(uint8_t p[124]) {
  memset(p,0,124);
  memcpy(p,"ROUGHTIM",8);
  put32(p+8,112);   /* framed message payload */
  put32(p+12,3);    /* tags */
  put32(p+16,64);   /* SIG 64 bytes */
  put32(p+20,76);   /* SIG + SREP 12 bytes */
  memcpy(p+24,"SIG\0",4);
  memcpy(p+28,"SREP",4);
  memcpy(p+32,"CERT",4);
}
int main(void) {
  uint8_t req[1024],nonce[32],response[124],corrupt[124];
  unsigned tags=0;
  memset(nonce,0,sizeof(nonce));
  assert(v12_rt_make_request(req,sizeof(req),nonce)<0);
  for(unsigned i=0;i<32;i++) nonce[i]=(uint8_t)(i+1);
  assert(v12_rt_make_request(req,1023,nonce)<0);
  assert(v12_rt_make_request(req,1024,nonce)==0);
  assert(!memcmp(req,"ROUGHTIM",8));
  assert(req[8]==0xf4 && req[9]==0x03 && req[10]==0 && req[11]==0);
  assert(req[12]==3 && req[16]==4 && req[20]==36);
  assert(!memcmp(req+24,"VER\0NONCZZZZ",12));
  assert(req[36]==8 && req[37]==0 && req[38]==0 && req[39]==0x80);
  assert(!memcmp(req+40,nonce,32));
  for(unsigned i=72;i<1024;i++) assert(req[i]==0);

  build_reply(response);
  assert(v12_rt_check_reply_frame(response,124,&tags)==0 && tags==3);
  assert(v12_rt_check_reply_frame(response,123,&tags)<0 && tags==0);
  assert(v12_rt_check_reply_frame(response,13,&tags)<0);
  memcpy(corrupt,response,124);
  corrupt[0]='X';
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  memcpy(corrupt,response,124);
  put32(corrupt+8,111);
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  memcpy(corrupt,response,124);
  put32(corrupt+12,0); /* invalid count */
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  memcpy(corrupt,response,124);
  put32(corrupt+12,40); /* absurd count */
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  memcpy(corrupt,response,124);
  put32(corrupt+16,65); /* non-aligned offset */
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  memcpy(corrupt,response,124);
  put32(corrupt+20,60); /* decreasing offset */
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  memcpy(corrupt,response,124);
  memcpy(corrupt+32,"SREP",4); /* repeated tag */
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  memcpy(corrupt,response,124);
  memcpy(corrupt+24,"NONE",4); /* missing signature */
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  memcpy(corrupt,response,124);
  put32(corrupt+16,60); /* incorrect 60-byte signature */
  assert(v12_rt_check_reply_frame(corrupt,124,&tags)<0);
  puts("PASS: experimental Roughtime draft08 request + bounded reply framing (NOT AUTHENTICATED)");
  return 0;
}

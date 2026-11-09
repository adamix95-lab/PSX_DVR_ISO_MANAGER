#include "v12_roughtime_nonce.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int openssl_hmac(void *ctx,const uint8_t secret[32],
                        const uint8_t *data,size_t n,uint8_t out[32]) {
  unsigned len=0;
  (void)ctx;
  if(n>UINT32_MAX) return -1;
  if(!HMAC(EVP_sha256(),secret,32,data,n,out,&len)) return -1;
  return len==32?0:-1;
}
static int rejected_hmac(void *ctx,const uint8_t secret[32],
                         const uint8_t *data,size_t n,uint8_t out[32]) {
  (void)ctx;(void)secret;(void)data;(void)n;(void)out;
  return -1;
}
int main(void) {
  uint8_t secret[32],got[32]={0},other[32]={0},bad[32]={0};
  const uint8_t expected1[32]={
    0x6d,0xdb,0x06,0xa7,0x53,0x65,0xf6,0x04,
    0x30,0xc9,0xf4,0x38,0xee,0x0c,0x31,0xf2,
    0x98,0xd5,0xf7,0xb2,0x5c,0x69,0x64,0x09,
    0x20,0x6d,0x9a,0x45,0x0e,0x9a,0xb4,0xd6
  };
  for(unsigned i=0;i<32;i++) secret[i]=(uint8_t)(i+1);
  assert(v12_rt_nonce_derive(secret,1,openssl_hmac,NULL,got)==0);
  assert(!memcmp(expected1,got,32));
  assert(v12_rt_nonce_derive(secret,2,openssl_hmac,NULL,other)==0);
  assert(memcmp(got,other,32));
  assert(v12_rt_nonce_derive(secret,1,openssl_hmac,NULL,other)==0);
  assert(!memcmp(got,other,32)); /* Counter replay => nonce replay. */
  memset(other,0xA5,sizeof(other));
  assert(v12_rt_nonce_derive(secret,0,openssl_hmac,NULL,other)<0);
  assert(other[0]==0xA5);
  assert(v12_rt_nonce_derive(secret,UINT64_MAX,openssl_hmac,NULL,other)<0);
  assert(other[0]==0xA5);
  assert(v12_rt_nonce_derive(secret,1,rejected_hmac,NULL,other)<0);
  assert(other[0]==0xA5);
  assert(v12_rt_nonce_derive(bad,1,openssl_hmac,NULL,other)<0);
  assert(other[0]==0xA5);
  puts("PASS: N3B offline HMAC-SHA256 nonce derivation; counter persistence is still REQUIRED");
  return 0;
}

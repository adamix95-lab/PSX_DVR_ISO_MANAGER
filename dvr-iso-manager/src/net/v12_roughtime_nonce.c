#include "v12_roughtime_nonce.h"
#include <string.h>

/* 19 visible bytes plus a NUL separator domain-separate HMAC usage. */
static const uint8_t domain[]="PSX-DVR-RT-NONCE-v1";

int v12_rt_nonce_derive(const uint8_t secret[32],uint64_t counter,
                        v12_rt_hmac256 hmac,void *ctx,
                        uint8_t output[32]) {
  uint8_t message[sizeof(domain)+8U],digest[32];
  unsigned i,nonzero=0;
  if(!secret || !output || !hmac || counter==0 || counter==UINT64_MAX)
    return -1;
  for(i=0;i<32;i++) nonzero|=secret[i];
  if(!nonzero) return -1;
  memcpy(message,domain,sizeof(domain)); /* Includes separator NUL. */
  for(i=0;i<8;i++) message[sizeof(domain)+i]=(uint8_t)(counter>>(8U*i));
  if(hmac(ctx,secret,message,sizeof(message),digest)!=0) return -1;
  nonzero=0;
  for(i=0;i<32;i++) nonzero|=digest[i];
  if(!nonzero) return -1;
  memcpy(output,digest,32);
  memset(digest,0,sizeof(digest));
  return 0;
}

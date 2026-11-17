#include "v12_n4a_entropy.h"
#include <string.h>
void v12_n4a_wipe(void *buf,size_t n) {
  volatile uint8_t *p=(volatile uint8_t *)buf;
  if(!buf) return;
  while(n--) *p++=0;
}
static int nonzero(const uint8_t *p,size_t n) {
  uint8_t orv=0;
  for(size_t i=0;i<n;i++) orv|=p[i];
  return orv!=0;
}
int v12_n4a_entropy_split(const uint8_t *src,size_t count,
                          uint8_t nonce[32],uint8_t tls_seed[64]) {
  if(!src || !nonce || !tls_seed || count!=V12_N4A_ENT_BYTES ||
     !nonzero(src,32) || !nonzero(src+32,64) ||
     memcmp(src,src+32,32)==0 ||
     memcmp(src+32,src+64,32)==0)
    return -1;
  memcpy(nonce,src,32);
  memcpy(tls_seed,src+32,64);
  return 0;
}

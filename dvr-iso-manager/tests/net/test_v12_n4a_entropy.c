#include "v12_n4a_entropy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  uint8_t data[96],nonce[32],tls[64],zeros[96]={0};
  for(unsigned i=0;i<96;i++) data[i]=(uint8_t)(i+1U);
  assert(v12_n4a_entropy_split(data,96,nonce,tls)==0);
  assert(memcmp(nonce,data,32)==0 && memcmp(tls,data+32,64)==0);
  memset(nonce,0xaa,32);memset(tls,0xbb,64);
  assert(v12_n4a_entropy_split(data,95,nonce,tls)<0);
  for(unsigned i=0;i<32;i++) assert(nonce[i]==0xaa);
  for(unsigned i=0;i<64;i++) assert(tls[i]==0xbb);
  assert(v12_n4a_entropy_split(data,97,nonce,tls)<0);
  assert(v12_n4a_entropy_split(NULL,96,nonce,tls)<0);
  assert(v12_n4a_entropy_split(data,96,NULL,tls)<0);
  assert(v12_n4a_entropy_split(data,96,nonce,NULL)<0);
  assert(v12_n4a_entropy_split(zeros,96,nonce,tls)<0);
  memcpy(zeros,data,96);memset(zeros,0,32);
  assert(v12_n4a_entropy_split(zeros,96,nonce,tls)<0);
  memcpy(zeros,data,96);memset(zeros+32,0,64);
  assert(v12_n4a_entropy_split(zeros,96,nonce,tls)<0);
  memcpy(zeros,data,96);memcpy(zeros+32,zeros,32);
  assert(v12_n4a_entropy_split(zeros,96,nonce,tls)<0);
  memcpy(zeros,data,96);memcpy(zeros+64,zeros+32,32);
  assert(v12_n4a_entropy_split(zeros,96,nonce,tls)<0);
  v12_n4a_wipe(tls,64);
  for(unsigned i=0;i<64;i++) assert(tls[i]==0);
  puts("PASS: N4A 96B split, independent ranges, invalid lengths, no output on failure, wipe");
  return 0;
}

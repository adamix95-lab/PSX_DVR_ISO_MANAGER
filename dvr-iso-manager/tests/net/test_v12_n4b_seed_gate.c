#include "v12_n4b_seed_gate.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    v12_n4b_seed_gate g={0};
    uint8_t s[64],out[80],same[64]={0};
    for(unsigned i=0;i<64;i++)s[i]=(uint8_t)(i+1U);
    memset(out,0xa5,sizeof(out));
    assert(v12_n4b_seed_provision(NULL,s,64)<0);
    assert(v12_n4b_seed_provision(&g,s,63)<0 && g.state==0U);
    assert(v12_n4b_seed_provision(&g,s,65)<0 && g.state==0U);
    assert(v12_n4b_seed_provision(&g,same,64)<0 && g.state==0U);
    memcpy(same,s,64);memcpy(same+32,same,32);
    assert(v12_n4b_seed_provision(&g,same,64)<0);
    assert(v12_n4b_seed_take(&g,out,32)<0 && g.state==0U);
    assert(v12_n4b_seed_provision(&g,s,64)==0 && g.state==1U);
    assert(v12_n4b_seed_provision(&g,s,64)<0);
    assert(v12_n4b_seed_take(&g,out,65)<0 && g.state==2U);
    for(unsigned i=0;i<64;i++)assert(g.seed[i]==0);
    assert(v12_n4b_seed_take(&g,out,32)<0);
    v12_n4b_seed_clear(&g);
    assert(v12_n4b_seed_provision(&g,s,64)==0);
    assert(v12_n4b_seed_take(&g,out,64)==0);
    assert(memcmp(out,s,64)==0);
    assert(v12_n4b_seed_take(&g,out,64)<0);
    for(unsigned i=0;i<64;i++)assert(g.seed[i]==0);
    v12_n4b_seed_clear(&g);
    assert(v12_n4b_seed_provision(&g,s,64)==0);
    assert(v12_n4b_seed_take(&g,out,0)<0 && g.state==2U);
    for(unsigned i=0;i<64;i++)assert(g.seed[i]==0);
    puts("PASS: N4B one-shot seed, wipe, zero/oversize rejection, fail closed");
    return 0;
}

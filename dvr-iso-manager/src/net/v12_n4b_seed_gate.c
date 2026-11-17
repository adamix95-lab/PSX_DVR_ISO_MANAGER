#include "v12_n4b_seed_gate.h"
#include <string.h>

/* Volatile write to avoid ordinary dead-store removal. */
static void wipe(void *ptr,size_t n) {
    volatile uint8_t *p=(volatile uint8_t *)ptr;
    while(n--) *p++=0;
}
void v12_n4b_seed_clear(v12_n4b_seed_gate *g) {
    if(g) {wipe(g->seed,sizeof(g->seed));g->state=0U;}
}
int v12_n4b_seed_provision(v12_n4b_seed_gate *g,
                            const uint8_t *s,size_t n) {
    uint8_t orv=0;
    if(!g || !s || n!=V12_N4B_SEED_BYTES || g->state!=0U)
        return -1;
    for(size_t i=0;i<n;i++) orv|=s[i];
    if(!orv || memcmp(s,s+32,32)==0) return -1;
    memcpy(g->seed,s,n);
    g->state=1U;
    return 0;
}
int v12_n4b_seed_take(v12_n4b_seed_gate *g,uint8_t *dst,size_t n) {
    if(!g || !dst || g->state!=1U) return -1;
    /* Mark CONSUMED before returning any byte: no two requests can
     * obtain the same seed. No support for reseed (must supply a new
     * fresh source per request in future real integration).
     */
    g->state=2U;
    if(n==0U || n>V12_N4B_SEED_BYTES) {
        wipe(g->seed,sizeof(g->seed));
        return -2;
    }
    memcpy(dst,g->seed,n);
    wipe(g->seed,sizeof(g->seed));
    return 0;
}

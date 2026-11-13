/* N4B HOST-ONLY POSITIVE/NEGATIVE DRBG RUNTIME TEST.
 * Runs in CI Linux, never on PS2. The 64-byte input is read from
 * the runner's OS CSPRNG (/dev/urandom). Never printed or saved.
 * A successful native test does NOT validate PS2 hardware entropy,
 * TLS certificates, secure storage, or cross-reboot one-use policy.
 */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/ssl.h>
#include "v12_n4b_seed_gate.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#if !defined(WC_RNG_SEED_CB) || !defined(HAVE_HASHDRBG)
#error "Host runtime must use callback-enabled hash DRBG"
#endif
static v12_n4b_seed_gate gate={0};
static int callback(OS_Seed *unused,byte *dst,word32 count) {
    (void)unused;
    return v12_n4b_seed_take(&gate,dst,(size_t)count);
}
static void wipe(void *ptr,size_t n) {
    volatile unsigned char *p=(volatile unsigned char *)ptr;
    while(n--) *p++=0;
}
static int os_seed(uint8_t dst[64]) {
    int fd=open("/dev/urandom",O_RDONLY);
    size_t n=0;
    if(fd<0) return -1;
    while(n<64U) {
        ssize_t got=read(fd,dst+n,64U-n);
        if(got<=0) {close(fd);return -1;}
        n+=(size_t)got;
    }
    if(close(fd)!=0) return -1;
    return 0;
}
int main(void) {
    WC_RNG rng;
    uint8_t input[64]={0},output_a[32]={0},output_b[32]={0};
    int rc;
    if(os_seed(input)!=0) return 11;
    if(v12_n4b_seed_provision(&gate,input,64)!=0) return 12;
    wipe(input,sizeof(input));
    if(wolfSSL_Init()!=WOLFSSL_SUCCESS) return 13;
    if(wc_SetSeed_Cb(callback)!=0) return 14;
    rc=wc_InitRng(&rng);
    if(rc!=0) {fprintf(stderr,"callback-provisioned DRBG initialization failed: %d\n",rc);return 15;}
    if(gate.state!=2U) return 16;
    if(wc_RNG_GenerateBlock(&rng,output_a,sizeof(output_a))!=0 ||
       wc_RNG_GenerateBlock(&rng,output_b,sizeof(output_b))!=0)
       return 17;
    if(memcmp(output_a,output_b,sizeof(output_a))==0) return 18;
    if(wc_FreeRng(&rng)!=0) return 19;
    wipe(output_a,sizeof(output_a));
    wipe(output_b,sizeof(output_b));
    /* A second DRBG cannot obtain the already consumed source. It must fail
     * rather than silently fall back to a platform entropy source.
     */
    rc=wc_InitRng(&rng);
    if(rc==0) {
        wc_FreeRng(&rng);
        fprintf(stderr,"FAIL: missing fresh source did not stop DRBG init\n");
        return 20;
    }
    wc_SetSeed_Cb(NULL);
    wolfSSL_Cleanup();
    v12_n4b_seed_clear(&gate);
    puts("PASS: host wolfSSL hash DRBG seeded once from OS CSPRNG; repeat refused");
    puts("NOT a PS2 hardware test, NOT a TLS connection.");
    return 0;
}

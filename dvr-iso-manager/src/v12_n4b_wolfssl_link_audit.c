/* N4B OPTIONAL wolfSSL static link audit. NEVER distributed or booted.
 * Requires wolfSSL LIBRARY and HEADERS compiled with WC_RNG_SEED_CB.
 * This file is NOT a working TLS client or a production RNG.
 * A missing callback must be a build error. NO WEAK FALLBACK.
 */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/ssl.h>
#include "v12_n4b_seed_gate.h"
#if !defined(WC_RNG_SEED_CB)
#error "N4B BLOCKED: rebuild isolated wolfSSL with WC_RNG_SEED_CB"
#endif
#if !defined(HAVE_HASHDRBG)
#error "N4B BLOCKED: wolfSSL must retain a hash-based DRBG"
#endif
#if defined(WOLFSSL_GENSEED_FORTEST) || defined(WC_NO_HASHDRBG)
#error "N4B BLOCKED: test-only seed or no DRBG"
#endif

static v12_n4b_seed_gate gate={0};
static int host_seed_cb(OS_Seed *os,byte *output,word32 size) {
    (void)os;
    /* With no externally supplied entropy, MUST fail closed. No RTC,
     * no predictable fallback, no embedded private seed.
     */
    return v12_n4b_seed_take(&gate,output,(size_t)size);
}
int main(void) {
    WC_RNG rng;
    int rc;
    if(wolfSSL_Init()!=WOLFSSL_SUCCESS) return 11;
    if(wc_SetSeed_Cb(host_seed_cb)!=0) return 12;
    rc=wc_InitRng(&rng);
    if(rc==0) {
        wc_FreeRng(&rng);
        /* The unprovisioned callback MUST NOT succeed. */
        return 13;
    }
    wc_SetSeed_Cb(NULL);
    wolfSSL_Cleanup();
    return 0;
}

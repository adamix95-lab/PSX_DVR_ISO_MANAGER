#include "v12_roughtime_monocypher.h"
#include "monocypher-ed25519.h"
#include <stdint.h>

static int mono_ed25519(void *ctx, const uint8_t key[32],
                        const uint8_t sig[64],
                        const uint8_t *msg, size_t size) {
  (void)ctx;
  if(!key || !sig || !msg) return -1;
  return crypto_ed25519_check(sig,key,msg,size)==0 ? 0 : -1;
}

static int mono_sha512(void *ctx,const uint8_t *msg,size_t size,
                       uint8_t hash[64]) {
  (void)ctx;
  if(!msg || !hash) return -1;
  crypto_sha512(hash,msg,size);
  return 0;
}

v12_rt_crypto v12_rt_monocypher_backend(void) {
  v12_rt_crypto cb = {NULL,mono_ed25519,mono_sha512};
  return cb;
}

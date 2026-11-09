#include "v12_roughtime_wolfcrypt.h"
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <stdint.h>

#if !defined(HAVE_ED25519) || !defined(HAVE_ED25519_VERIFY) || \
    !defined(HAVE_ED25519_KEY_IMPORT) || !defined(WOLFSSL_SHA512)
#error "Roughtime needs an isolated wolfSSL build with Ed25519 verify + SHA512"
#endif

static int verify_ed(void *ctx,const uint8_t key[32],const uint8_t sig[64],
                     const uint8_t *msg,size_t size) {
  ed25519_key pub;
  int ok=0,rc;
  (void)ctx;
  if(!key || !sig || !msg || size>UINT32_MAX) return -1;
  rc=wc_ed25519_init(&pub);
  if(rc!=0) return -1;
  rc=wc_ed25519_import_public(key,32,&pub);
  if(rc==0) rc=wc_ed25519_verify_msg(sig,64,msg,(word32)size,&ok,&pub);
  wc_ed25519_free(&pub);
  return (rc==0 && ok==1)?0:-1;
}

static int hash512(void *ctx,const uint8_t *msg,size_t size,uint8_t digest[64]) {
  (void)ctx;
  if(!msg || !digest || size>UINT32_MAX) return -1;
  return wc_Sha512Hash(msg,(word32)size,digest)==0?0:-1;
}

v12_rt_crypto v12_rt_wolfcrypt_backend(void) {
  v12_rt_crypto backend={NULL,verify_ed,hash512};
  return backend;
}

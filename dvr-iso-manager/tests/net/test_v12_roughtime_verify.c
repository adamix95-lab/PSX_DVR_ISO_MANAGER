#include "v12_roughtime_verify.h"
#include "v12_rt_test_vector.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Host-only Ed25519/SHA512 callbacks. PS2 backend is wolfCrypt instead. */
static int ed_host(void *ctx, const uint8_t public_key[32],
                   const uint8_t signature[64],
                   const uint8_t *msg,size_t len) {
  EVP_PKEY *key;
  EVP_MD_CTX *md;
  int ok;
  (void)ctx;
  key=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,NULL,public_key,32);
  if(!key) return -1;
  md=EVP_MD_CTX_new();
  if(!md) {EVP_PKEY_free(key);return -1;}
  ok=EVP_DigestVerifyInit(md,NULL,NULL,NULL,key)==1 &&
     EVP_DigestVerify(md,signature,64,msg,len)==1;
  EVP_MD_CTX_free(md);EVP_PKEY_free(key);
  return ok?0:-1;
}
static int hash_host(void *ctx,const uint8_t *msg,size_t len,uint8_t hash[64]) {
  (void)ctx;
  return SHA512(msg,len,hash)?0:-1;
}
static int bad_hash(void *ctx,const uint8_t *msg,size_t len,uint8_t hash[64]) {
  (void)ctx;(void)msg;(void)len;(void)hash;
  return -1;
}
static void rejected(const uint8_t *wire,size_t len,const uint8_t *root,
                     const uint8_t *nonce,const v12_rt_crypto *crypto) {
  v12_rt_signed_time result={UINT64_C(999), 777U};
  assert(v12_rt_verify_draft08(wire,len,root,nonce,crypto,&result)<0);
  assert(result.midpoint_utc==UINT64_C(999) && result.radius_seconds==777U);
}
int main(void) {
  v12_rt_crypto cb={0,ed_host,hash_host};
  v12_rt_crypto broken={0,ed_host,bad_hash};
  v12_rt_signed_time result={0,0};
  uint8_t damaged[sizeof(rt_fixture_reply)];
  uint8_t wrong[32]={0};
  assert(v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply),
         rt_fixture_root,rt_fixture_nonce,&cb,&result)==0);
  assert(result.midpoint_utc==50 && result.radius_seconds==5);
  rejected(rt_fixture_reply,sizeof(rt_fixture_reply),wrong,rt_fixture_nonce,&cb);
  rejected(rt_fixture_reply,sizeof(rt_fixture_reply),rt_fixture_root,wrong,&cb);
  rejected(rt_fixture_reply,sizeof(rt_fixture_reply)-1,rt_fixture_root,
           rt_fixture_nonce,&cb);
  rejected(rt_fixture_reply,sizeof(rt_fixture_reply),rt_fixture_root,
           rt_fixture_nonce,&broken);
  for(size_t i=0;i<sizeof(rt_fixture_reply);i++) {
    memcpy(damaged,rt_fixture_reply,sizeof(damaged));
    damaged[i]^=1;
    /* Mutations in unused pad bytes can be semantically irrelevant;
     * signed/structured fields must always fail closed. */
    if(i<100 || (i>=120 && i<280)) {
      rejected(damaged,sizeof(damaged),rt_fixture_root,rt_fixture_nonce,&cb);
    }
  }
  puts("PASS: N3B host C Ed25519 delegation+SREP+Merkle; tampering rejected");
  return 0;
}

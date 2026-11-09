/* N3B Ed25519 + SHA512 link test for an EXPERIMENTAL wolfSSL rebuild.
 * Not included in any stable app or release. No network, RNG or RTC access.
 * Uses public RFC8032 test 1, no private keys.
 */
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/hash.h>
#include "v12_roughtime_verify.h"
#include "v12_roughtime_wolfcrypt.h"
#include "v12_rt_test_vector.h"
#include <string.h>

#if !defined(HAVE_ED25519) || !defined(HAVE_ED25519_VERIFY) || \
    !defined(HAVE_ED25519_KEY_IMPORT)
#error "This N3B audit requires a custom wolfSSL build with Ed25519 enabled"
#endif
#ifndef WOLFSSL_SHA512
#error "This N3B audit requires SHA512"
#endif

/* Public RFC 8032 test 1: Ed25519, empty message. This is a static
 * symbol-link audit; GitHub CI compiles/links but never executes it.
 * No RNG or trust bootstrap is required to verify a known signature.
 */
static int audit_ed25519_and_sha512(void) {
  static const byte pub[32] = {
    0xd7,0x5a,0x98,0x01,0x82,0xb1,0x0a,0xb7,
    0xd5,0x4b,0xfe,0xd3,0xc9,0x64,0x07,0x3a,
    0x0e,0xe1,0x72,0xf3,0xda,0xa6,0x23,0x25,
    0xaf,0x02,0x1a,0x68,0xf7,0x07,0x51,0x1a
  };
  static const byte sig[64] = {
    0xe5,0x56,0x43,0x00,0xc3,0x60,0xac,0x72,
    0x90,0x86,0xe2,0xcc,0x80,0x6e,0x82,0x8a,
    0x84,0x87,0x7f,0x1e,0xb8,0xe5,0xd9,0x74,
    0xd8,0x73,0xe0,0x65,0x22,0x49,0x01,0x55,
    0x5f,0xb8,0x82,0x15,0x90,0xa3,0x3b,0xac,
    0xc6,0x1e,0x39,0x70,0x1c,0xf9,0xb4,0x6b,
    0xd2,0x5b,0xf5,0xf0,0x59,0x5b,0xbe,0x24,
    0x65,0x51,0x41,0x43,0x8e,0x7a,0x10,0x0b
  };
  byte digest[64];
  ed25519_key k;
  int result=0,rc;
  rc=wc_Sha512Hash((const byte *)"",0,digest);
  if(rc!=0 || digest[0]!=0xcf || digest[1]!=0x83) return 21;
  rc=wc_ed25519_init(&k);
  if(rc!=0) return 22;
  rc=wc_ed25519_import_public(pub,sizeof(pub),&k);
  if(rc==0)
    rc=wc_ed25519_verify_msg(sig,sizeof(sig),
          (const byte *)"",0,&result,&k);
  wc_ed25519_free(&k);
  return (rc==0 && result==1) ? 0 : 23;
}


int main(void) {
  v12_rt_crypto cb;
  v12_rt_signed_time time={0,0};
  uint8_t corrupted[sizeof(rt_fixture_reply)];
  int rc=audit_ed25519_and_sha512();
  if(rc!=0) return rc;
  cb=v12_rt_wolfcrypt_backend();
  if(v12_rt_verify_draft08(rt_fixture_reply,sizeof(rt_fixture_reply),
        rt_fixture_root,rt_fixture_nonce,&cb,&time)!=0) return 24;
  if(time.midpoint_utc!=UINT64_C(50) || time.radius_seconds!=5U)
    return 25;
  memcpy(corrupted,rt_fixture_reply,sizeof(corrupted));
  corrupted[64]^=0x01; /* signed response or signature tamper */
  if(v12_rt_verify_draft08(corrupted,sizeof(corrupted),
       rt_fixture_root,rt_fixture_nonce,&cb,&time)==0) return 26;
  return 0;
}

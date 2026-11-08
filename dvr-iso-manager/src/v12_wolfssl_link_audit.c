/* v1.2 wolfSSL SDK link audit. No sockets, network requests, RNG override,
 * disk I/O or credential handling. Not bundled into stable builds or Manager.
 * No wolfSSL/NULD source is copied into this file.
 */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/hash.h>

#if !defined(HAVE_ED25519) || !defined(HAVE_ED25519_VERIFY) || \
    !defined(HAVE_ED25519_KEY_IMPORT)
#error "PS2SDK wolfSSL lacks Ed25519 public-key verification support"
#endif
#ifndef WOLFSSL_SHA512
#error "PS2SDK wolfSSL lacks SHA-512 for Roughtime Merkle hashes"
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

#include <time.h>

/* Deliberate failure clock; DO NOT use provisional NTP for certificate trust.
 * This is a LINK-ONLY API test. Nothing installs it in the real Manager. */
static time_t unavailable_trusted_time(time_t *out) {
  if (out) *out=0;
  return (time_t)0;
}

static int blocked_recv(WOLFSSL *ssl, char *buffer, int sz, void *ctx) {
  (void)ssl; (void)buffer; (void)sz; (void)ctx;
  return WOLFSSL_CBIO_ERR_GENERAL;
}
static int blocked_send(WOLFSSL *ssl, char *buffer, int sz, void *ctx) {
  (void)ssl; (void)buffer; (void)sz; (void)ctx;
  return WOLFSSL_CBIO_ERR_GENERAL;
}

int main(void) {
  WOLFSSL_CTX *ctx;
  WOLFSSL *ssl;
  int smoke;
  int ok=wolfSSL_Init();
  if(ok!=WOLFSSL_SUCCESS) return 10;
  smoke=audit_ed25519_and_sha512();
  if(smoke!=0) {wolfSSL_Cleanup();return smoke;}


  ctx=wolfSSL_CTX_new(wolfSSLv23_client_method());
  if (!ctx) {wolfSSL_Cleanup();return 11;}
  /* Never copy ORBIT/NULD's WOLFSSL_VERIFY_NONE shortcut. */
  /* Verify the custom clock ABI can be linked into the pinned wolfSSL port.
   * No authenticated time source is available yet; do not ship this image.
   * An epoch-zero return must fail certificate validity checks closed.
   */
  if (wc_SetTimeCb(unavailable_trusted_time)!=0) {
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();
    return 13;
  }
  wolfSSL_CTX_set_verify(ctx,WOLFSSL_VERIFY_PEER,NULL);
  wolfSSL_CTX_SetIORecv(ctx,blocked_recv);
  wolfSSL_CTX_SetIOSend(ctx,blocked_send);
  ssl=wolfSSL_new(ctx);
  if (ssl) {
    /* API link-time smoke test ONLY: no handshake or certificate acceptance. */
    if (wolfSSL_check_domain_name(ssl,"raw.githubusercontent.com")!=WOLFSSL_SUCCESS) {
      wolfSSL_free(ssl);wolfSSL_CTX_free(ctx);wolfSSL_Cleanup();return 12;
    }
    wolfSSL_free(ssl);
  }
  /* Restore wolfSSL's default callback after the ABI test. */
  wc_SetTimeCb(NULL);
  wolfSSL_CTX_free(ctx);
  wolfSSL_Cleanup();
  return 0;
}

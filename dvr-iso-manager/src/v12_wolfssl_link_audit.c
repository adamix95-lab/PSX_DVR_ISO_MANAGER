/* v1.2 wolfSSL SDK link audit. No sockets, network requests, RNG override,
 * disk I/O or credential handling. Not bundled into stable builds or Manager.
 * No wolfSSL/NULD source is copied into this file.
 */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/asn_public.h>
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
  int ok=wolfSSL_Init();
  if(ok!=WOLFSSL_SUCCESS) return 10;


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

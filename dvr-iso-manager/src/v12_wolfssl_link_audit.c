/* v1.2 wolfSSL SDK link audit. No sockets, network requests, RNG override,
 * disk I/O or credential handling. Not bundled into stable builds or Manager.
 * No wolfSSL/NULD source is copied into this file.
 */
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

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
  if (ok!=WOLFSSL_SUCCESS) return 10;
  ctx=wolfSSL_CTX_new(wolfSSLv23_client_method());
  if (!ctx) {wolfSSL_Cleanup();return 11;}
  /* Never copy ORBIT/NULD's WOLFSSL_VERIFY_NONE shortcut. */
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
  wolfSSL_CTX_free(ctx);
  wolfSSL_Cleanup();
  return 0;
}

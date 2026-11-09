#ifndef DESR_V12_ROUGHTIME_VERIFY_H
#define DESR_V12_ROUGHTIME_VERIFY_H
#include <stddef.h>
#include <stdint.h>

/* Experimental IETF Roughtime draft-08 message verifier.
 * The caller MUST provide a correctly pinned server root and a fresh,
 * cryptographically unpredictable nonce from its request. The existing
 * N3A xorshift nonce is NOT permitted. This standalone code has no socket,
 * disk, RTC, wolfSSL clock or application integration.
 *
 * A successful signature check on a known-answer fixture proves the code
 * path, NOT that live time is authenticated on DESR hardware.
 */
typedef struct {
  void *ctx;
  /* Return 0 ONLY when Ed25519 signature verifies against public_key. */
  int (*verify_ed25519)(void *ctx, const uint8_t public_key[32],
                        const uint8_t signature[64],
                        const uint8_t *message, size_t message_size);
  /* Return 0 ONLY if a complete SHA-512 digest was produced. */
  int (*sha512)(void *ctx, const uint8_t *message,
                size_t message_size, uint8_t digest[64]);
} v12_rt_crypto;

typedef struct {
  uint64_t midpoint_utc; /* IETF draft-08 seconds, NOT an RTC write */
  uint32_t radius_seconds;
} v12_rt_signed_time;

/* Returns 0 iff frame, delegated key, Ed25519 signatures, signed interval,
 * freshness *binding* to caller-supplied nonce and Merkle proof all pass.
 * Output remains unchanged on any error. No auth source is selected here.
 */
int v12_rt_verify_draft08(const uint8_t *reply, size_t reply_size,
                          const uint8_t pinned_root_public[32],
                          const uint8_t request_nonce[32],
                          const v12_rt_crypto *crypto,
                          v12_rt_signed_time *output);
#endif

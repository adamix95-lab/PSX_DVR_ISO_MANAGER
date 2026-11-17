#include "n6g_roughtime_retry.h"
#include "monocypher.h"
#include <string.h>

static int retryable_result(int rc) {
  /* Missing reply, malformed frame, invalid signature/nonce or signed time. */
  return rc == -506 || rc == -507 || rc == -508 || rc == -515;
}

int n6g_roughtime_retry(const uint8_t base_nonce[32],
                       n6g_roughtime_probe probe, void *ctx) {
  uint8_t nonce[32];
  int rc = -506;
  if (!base_nonce || !probe) return -1;

  for (unsigned attempt = 0; attempt < N6G_ROUGHTIME_MAX_ATTEMPTS; attempt++) {
    if (attempt == 0) {
      memcpy(nonce, base_nonce, sizeof(nonce));
    } else {
      /* Monocypher BLAKE2b keyed with the source-specific original nonce:
       * a distinct nonce for this request, without reusing an old reply.
       */
      const uint8_t domain[] = "PSX DVR Roughtime retry v1";
      uint8_t message[sizeof(domain)];
      memcpy(message, domain, sizeof(message));
      message[sizeof(message) - 1] = (uint8_t)attempt;
      crypto_blake2b_keyed(nonce, sizeof(nonce), base_nonce, 32,
                           message, sizeof(message));
      crypto_wipe(message, sizeof(message));
    }
    rc = probe(nonce, ctx);
    if (rc == 0 || !retryable_result(rc)) break;
  }

  crypto_wipe(nonce, sizeof(nonce));
  return rc;
}

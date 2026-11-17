#ifndef N6G_ROUGHTIME_RETRY_H
#define N6G_ROUGHTIME_RETRY_H

#include <stdint.h>

#define N6G_ROUGHTIME_MAX_ATTEMPTS 3
typedef int (*n6g_roughtime_probe)(const uint8_t nonce[32], void *ctx);

/* Strict bounded retry: attempt one uses the original nonce; the following
 * attempts derive distinct domain-separated nonces from the base value.
 * This adds no entropy to the experimental CPU-counter seed. No failed
 * signature, nonce or time check is ever accepted.
 */
int n6g_roughtime_retry(const uint8_t base_nonce[32],
                       n6g_roughtime_probe probe, void *ctx);

#endif

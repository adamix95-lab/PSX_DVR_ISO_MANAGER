#ifndef N6G_CPU_ENTROPY_H
#define N6G_CPU_ENTROPY_H

#include "n6g_batch_entropy.h"

#define N6G_CPU_SAMPLE_COUNT 96
#define N6G_CPU_RETRY_KEY_BYTES 64

#ifdef _EE
static inline uint32_t n6g_read_cpu_count(void) {
    uint32_t ticks;
    __asm__ volatile ("mfc0 %0, $9" : "=r" (ticks));
    return ticks;
}
#endif

int n6g_expand_cpu_entropy(n6g_batch_entropy entropy,
                          uint8_t retry_key[N6G_CPU_RETRY_KEY_BYTES], uint32_t launch_ticks,
                          const uint32_t samples[N6G_CPU_SAMPLE_COUNT]);
void n6g_derive_retry_nonce(uint8_t nonce[32], const uint8_t key[N6G_CPU_RETRY_KEY_BYTES],
                           const uint8_t first_nonce[32]);

#endif

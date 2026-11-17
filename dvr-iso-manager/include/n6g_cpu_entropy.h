#ifndef N6G_CPU_ENTROPY_H
#define N6G_CPU_ENTROPY_H

#include "n6g_batch_entropy.h"

#define N6G_CPU_SAMPLE_COUNT 96

#ifdef _EE
static inline uint32_t n6g_read_cpu_count(void) {
    uint32_t ticks;
    __asm__ volatile ("mfc0 %0, $9" : "=r" (ticks));
    return ticks;
}
#endif

int n6g_expand_cpu_entropy(n6g_batch_entropy entropy, uint32_t launch_ticks,
                          const uint32_t samples[N6G_CPU_SAMPLE_COUNT]);

#endif

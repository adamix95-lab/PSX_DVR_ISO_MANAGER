#include "n6g_cpu_entropy.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint32_t samples[N6G_CPU_SAMPLE_COUNT];
    n6g_batch_entropy first, second;

    for (size_t i = 0; i < N6G_CPU_SAMPLE_COUNT; ++i)
        samples[i] = 42;
    assert(n6g_expand_cpu_entropy(first, 100, samples) < 0);

    for (size_t i = 0; i < N6G_CPU_SAMPLE_COUNT; ++i)
        samples[i] += (uint32_t)i * 13;
    assert(n6g_expand_cpu_entropy(first, 100, samples) == 0);
    assert(n6g_batch_entropy_valid(first));
    assert(n6g_expand_cpu_entropy(second, 100, samples) == 0);
    assert(memcmp(first, second, sizeof(first)) == 0);

    assert(n6g_expand_cpu_entropy(second, 101, samples) == 0);
    assert(n6g_batch_entropy_valid(second));
    assert(memcmp(first, second, sizeof(first)) != 0);
    samples[N6G_CPU_SAMPLE_COUNT - 1] ^= 1;
    assert(n6g_expand_cpu_entropy(second, 100, samples) == 0);
    assert(memcmp(first, second, sizeof(first)) != 0);

    puts("CPU counter expansion: frozen counter rejected, 100 time/TLS slots valid");
    return 0;
}

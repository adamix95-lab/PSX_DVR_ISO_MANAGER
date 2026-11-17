#include "n6g_cpu_entropy.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint32_t samples[N6G_CPU_SAMPLE_COUNT];
    n6g_batch_entropy first, second;
    uint8_t first_key[N6G_CPU_RETRY_KEY_BYTES], second_key[N6G_CPU_RETRY_KEY_BYTES];
    uint8_t first_retry[32], second_retry[32];

    for (size_t i = 0; i < N6G_CPU_SAMPLE_COUNT; ++i)
        samples[i] = 42;
    assert(n6g_expand_cpu_entropy(first, first_key, 100, samples) < 0);

    for (size_t i = 0; i < N6G_CPU_SAMPLE_COUNT; ++i)
        samples[i] += (uint32_t)i * 13;
    assert(n6g_expand_cpu_entropy(first, first_key, 100, samples) == 0);
    assert(n6g_batch_entropy_valid(first));
    assert(n6g_expand_cpu_entropy(second, second_key, 100, samples) == 0);
    assert(memcmp(first, second, sizeof(first)) == 0);
    assert(memcmp(first_key, second_key, sizeof(first_key)) == 0);
    n6g_derive_retry_nonce(first_retry, first_key, first[0][0].nonce);
    n6g_derive_retry_nonce(second_retry, first_key, first[0][1].nonce);
    assert(memcmp(first_retry, first[0][0].nonce, sizeof(first_retry)) != 0);
    assert(memcmp(first_retry, second_retry, sizeof(first_retry)) != 0);

    assert(n6g_expand_cpu_entropy(second, second_key, 101, samples) == 0);
    assert(n6g_batch_entropy_valid(second));
    assert(memcmp(first, second, sizeof(first)) != 0);
    assert(memcmp(first_key, second_key, sizeof(first_key)) != 0);
    samples[N6G_CPU_SAMPLE_COUNT - 1] ^= 1;
    assert(n6g_expand_cpu_entropy(second, second_key, 100, samples) == 0);
    assert(memcmp(first, second, sizeof(first)) != 0);

    puts("CPU counter expansion: frozen counter rejected, 100 time/TLS slots and distinct retry nonces");
    return 0;
}

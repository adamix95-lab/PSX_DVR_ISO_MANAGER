#include "n6g_cpu_entropy.h"
#include "monocypher.h"

/* Experimental COP0 Count source, inspired by Daniel's NULD:
 * https://github.com/danielnuld/nuld/blob/2951dc9d22541aa6a2d6c2fcf7705187e97eae92/net.c
 * The keyed expansion supplies distinct inputs. It cannot add entropy.
 */
int n6g_expand_cpu_entropy(n6g_batch_entropy entropy,
                          uint8_t retry_key[N6G_CPU_RETRY_KEY_BYTES], uint32_t launch_ticks,
                          const uint32_t samples[N6G_CPU_SAMPLE_COUNT]) {
    static const uint8_t domain[] = "PSX DVR CPU entropy test v1";
    static const uint8_t retry_label[] = "PSX DVR Roughtime retry key";
    crypto_blake2b_ctx hash;
    uint8_t key[64];
    uint32_t movement = 0;

    for (size_t i = 1; i < N6G_CPU_SAMPLE_COUNT; ++i)
        movement |= samples[i] ^ samples[0];
    if (!movement) return -1;

    crypto_blake2b_init(&hash, sizeof(key));
    crypto_blake2b_update(&hash, domain, sizeof(domain) - 1);
    crypto_blake2b_update(&hash, (const uint8_t *)&launch_ticks, sizeof(launch_ticks));
    crypto_blake2b_update(&hash, (const uint8_t *)samples,
                          N6G_CPU_SAMPLE_COUNT * sizeof(samples[0]));
    crypto_blake2b_final(&hash, key);
    crypto_blake2b_keyed(retry_key, N6G_CPU_RETRY_KEY_BYTES, key, sizeof(key),
                         retry_label, sizeof(retry_label) - 1);

    for (size_t game = 0; game < N6G_BATCH_LIMIT; ++game) {
        for (size_t source = 0; source < 2; ++source) {
            uint8_t label[] = { (uint8_t)game, (uint8_t)source, 0 };
            n6g_source_entropy *slot = &entropy[game][source];
            crypto_blake2b_keyed(slot->nonce, sizeof(slot->nonce), key, sizeof(key),
                                 label, sizeof(label));
            label[2] = 1;
            crypto_blake2b_keyed(slot->low, sizeof(slot->low), key, sizeof(key),
                                 label, sizeof(label));
            label[2] = 2;
            crypto_blake2b_keyed(slot->high, sizeof(slot->high), key, sizeof(key),
                                 label, sizeof(label));
        }
    }
    crypto_wipe(key, sizeof(key));
    crypto_wipe(&hash, sizeof(hash));
    return 0;
}

void n6g_derive_retry_nonce(uint8_t nonce[32], const uint8_t key[N6G_CPU_RETRY_KEY_BYTES],
                           const uint8_t first_nonce[32]) {
    crypto_blake2b_keyed(nonce, 32, key, N6G_CPU_RETRY_KEY_BYTES, first_nonce, 32);
}

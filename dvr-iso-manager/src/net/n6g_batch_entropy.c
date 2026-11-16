#include "n6g_batch_entropy.h"
#include <string.h>

_Static_assert(sizeof(n6g_batch_entropy) == 16000, "N6G50.ENT format");

static int nonzero(const uint8_t *data, size_t size) {
    uint8_t bits = 0;
    for (size_t i = 0; i < size; ++i)
        bits |= data[i];
    return bits != 0;
}

int n6g_batch_entropy_valid(const n6g_batch_entropy entropy) {
    for (size_t i = 0; i < N6G_BATCH_LIMIT * 2U; ++i) {
        const n6g_source_entropy *current = &entropy[i / 2U][i % 2U];
        if (!nonzero(current->nonce, sizeof(current->nonce)) ||
            !nonzero(current->low, sizeof(current->low)) ||
            !nonzero(current->high, sizeof(current->high)) ||
            !memcmp(current->low, current->high, sizeof(current->low)))
            return 0;
        for (size_t j = 0; j < i; ++j) {
            const n6g_source_entropy *prior = &entropy[j / 2U][j % 2U];
            if (!memcmp(current->nonce, prior->nonce, sizeof(current->nonce)) ||
                !memcmp(current->low, prior->low, sizeof(current->low)) ||
                !memcmp(current->low, prior->high, sizeof(current->low)) ||
                !memcmp(current->high, prior->low, sizeof(current->high)) ||
                !memcmp(current->high, prior->high, sizeof(current->high)))
                return 0;
        }
    }
    return 1;
}

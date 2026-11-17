#ifndef N6G_BATCH_ENTROPY_H
#define N6G_BATCH_ENTROPY_H

#include "n6_usb_art.h"

typedef struct {
    uint8_t nonce[32];
    uint8_t low[64];
    uint8_t high[64];
} n6g_source_entropy;

/* Each missing game can try OPL and CosmicScale, with fresh time and TLS. */
typedef n6g_source_entropy n6g_batch_entropy[N6G_BATCH_LIMIT][2];

int n6g_batch_entropy_valid(const n6g_batch_entropy entropy);

#endif

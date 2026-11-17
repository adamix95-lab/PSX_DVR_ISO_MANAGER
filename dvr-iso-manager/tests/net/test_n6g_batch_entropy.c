#include "n6g_batch_entropy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    n6g_batch_entropy entropy;
    FILE *file;
    assert(argc == 2);
    file = fopen(argv[1], "rb");
    assert(file);
    assert(fread(entropy, 1, sizeof(entropy), file) == sizeof(entropy));
    assert(fgetc(file) == EOF);
    assert(fclose(file) == 0);
    assert(n6g_batch_entropy_valid(entropy));
    memcpy(entropy[49][1].nonce, entropy[0][0].nonce, sizeof(entropy[0][0].nonce));
    assert(!n6g_batch_entropy_valid(entropy));
    entropy[49][1].nonce[0] ^= 0xff;
    assert(n6g_batch_entropy_valid(entropy));
    memcpy(entropy[49][1].high, entropy[0][0].low, sizeof(entropy[0][0].low));
    assert(!n6g_batch_entropy_valid(entropy));
    memset(entropy[49][1].high, 0, sizeof(entropy[0][0].low));
    assert(!n6g_batch_entropy_valid(entropy));
    puts("PASS N6G50 separate nonces and DRBG seeds for 50 games and fallback");
    return 0;
}

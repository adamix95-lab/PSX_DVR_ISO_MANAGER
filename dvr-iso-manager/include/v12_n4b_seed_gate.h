#ifndef DESR_V12_N4B_SEED_GATE_H
#define DESR_V12_N4B_SEED_GATE_H
#include <stddef.h>
#include <stdint.h>
#define V12_N4B_SEED_BYTES 64U

/* Standalone, single-threaded, in-RAM N4B diagnostic adapter only.
 * Input MUST come from a trusted host OS CSPRNG and be UNIQUE per run.
 * Never use the public test vector as live entropy.
 *
 * This does NOT ensure anti-replay after a reboot, TLS clock trust, or
 * persistence. A requested seed beyond 64 B fails CLOSED. No fallback.
 */
typedef struct {
    uint8_t seed[V12_N4B_SEED_BYTES];
    unsigned state; /* 0 empty, 1 armed, 2 consumed */
} v12_n4b_seed_gate;

int v12_n4b_seed_provision(v12_n4b_seed_gate *gate,
                            const uint8_t *seed,size_t n);
int v12_n4b_seed_take(v12_n4b_seed_gate *gate,uint8_t *dst,size_t n);
void v12_n4b_seed_clear(v12_n4b_seed_gate *gate);
#endif

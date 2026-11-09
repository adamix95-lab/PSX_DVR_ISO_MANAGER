#ifndef DESR_V12_ROUGHTIME_NONCE_H
#define DESR_V12_ROUGHTIME_NONCE_H
#include <stddef.h>
#include <stdint.h>

/* DESIGN PROTOTYPE, NOT ENABLED IN THE DESR MANAGER.
 *
 * A secure deployment MUST:
 * 1) generate a unique 256-bit high-entropy secret via an OS CSPRNG,
 * 2) keep it private on the physical device,
 * 3) reserve/atomically persist a NEVER-REUSED 64-bit counter BEFORE
 *    any network request, including across resets, crashes and backups,
 * 4) pass it to HMAC-SHA256 below, rejecting counter reuse/rollback,
 * 5) never use the N3A xorshift counter as secret material.
 *
 * The HMAC callback is responsible for cryptographic correctness. The
 * function cannot validate secret provenance or counter durability.
 * Independent domains are needed for future wolfSSL DRBG seeding.
 */
typedef int (*v12_rt_hmac256)(void *ctx, const uint8_t secret[32],
                             const uint8_t *message, size_t message_size,
                             uint8_t digest[32]);

int v12_rt_nonce_derive(const uint8_t secret[32], uint64_t reserved_counter,
                        v12_rt_hmac256 hmac, void *ctx,
                        uint8_t nonce_out[32]);
#endif

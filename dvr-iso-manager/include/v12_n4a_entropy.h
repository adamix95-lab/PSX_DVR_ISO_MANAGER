#ifndef DESR_V12_N4A_ENTROPY_H
#define DESR_V12_N4A_ENTROPY_H
#include <stddef.h>
#include <stdint.h>
#define V12_N4A_ENT_BYTES 96U
/* Checks only shape/obvious failures, never claims proven randomness.
 * First 32 bytes: Roughtime nonce.
 * Remaining 64 bytes: reserved for future wolfSSL DRBG seed callback.
 * Never assume that loading this seed alone configures wolfSSL.
 * Copies to outputs only on success.
 */
int v12_n4a_entropy_split(const uint8_t *src,size_t count,
                          uint8_t nonce[32],uint8_t tls_seed[64]);
void v12_n4a_wipe(void *src,size_t count);
#endif

#ifndef DESR_V12_ROUGHTIME_TXRYAN_SIG_H
#define DESR_V12_ROUGHTIME_TXRYAN_SIG_H
#include <stddef.h>
#include <stdint.h>
/* Only a read-only hardware diagnostic. Its nonce is predictably generated.
 * No valid timestamp is returned or eligible for TLS certificate checks.
 * Strict draft-08 accepts only an echoed NONC, full signatures + Merkle.
 * 1 = forensic crypto verified but NONC absent (NOT valid IETF).
 * 0 = strict draft-08 crypto verified, still WEAK nonce (NOT trusted time).
 * Negative = reject.
 */
int v12_roughtime_txryan_sig_verify(const uint8_t *packet,size_t length,
                                    const uint8_t nonce[32]);
#endif

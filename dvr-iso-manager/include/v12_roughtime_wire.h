#ifndef DESR_V12_ROUGHTIME_WIRE_H
#define DESR_V12_ROUGHTIME_WIRE_H
#include <stddef.h>
#include <stdint.h>

/* Read-only probe of Roughtime IETF draft-08 wire framing.
 *
 * A PARSED frame is NOT a verified signature. Never feed its fields into
 * TLS trust decisions. The protocol is experimental and must be separately
 * cryptographically verified with Ed25519, delegated key and Merkle proof.
 */
#define V12_RT_REQUEST_BYTES 1024U
#define V12_RT_REPLY_MAX_BYTES 2048U

/* 0 on success; request has exactly 1024 bytes and a non-zero 32-byte nonce.
 * Nonce is supplied by caller. This layer supplies NO cryptographic RNG. */
int v12_rt_make_request(uint8_t *out, size_t capacity,
                       const uint8_t nonce[32]);

/* Checks frame bounds and required signature-related tags only; DOES NOT
 * verify SIG, CERT, SREP or time. 0 for well-framed message; -1 otherwise. */
int v12_rt_check_reply_frame(const uint8_t *packet, size_t size,
                            unsigned *tag_count);
#endif

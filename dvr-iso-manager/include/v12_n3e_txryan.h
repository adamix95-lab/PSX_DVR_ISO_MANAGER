#ifndef DESR_V12_N3E_TXRYAN_H
#define DESR_V12_N3E_TXRYAN_H
#include <stddef.h>
#include <stdint.h>
#include "v12_roughtime_verify.h"

/* N3E stand-alone only. Does NOT set a TLS clock or retain state.
 * On success returns strict signed timestamp and signed RADI in seconds.
 * No forensic NONC-absent fallback is allowed.
 */
int v12_n3e_txryan_strict(const uint8_t *packet,size_t length,
                           const uint8_t nonce[32],
                           v12_rt_signed_time *verified);
#endif

#ifndef DESR_V12_ROUGHTIME_MONOCYPHER_H
#define DESR_V12_ROUGHTIME_MONOCYPHER_H
#include "v12_roughtime_verify.h"

/* Experimental OPTIONAL BSD-2-Clause/CC0 crypto backend.
 * Uses Monocypher's RFC8032 Ed25519 + SHA-512, avoiding GPL-linked
 * wolfCrypt in the standalone diagnostic.
 * No entropy, transport, time source, secrets or disk writes.
 */
v12_rt_crypto v12_rt_monocypher_backend(void);

#endif

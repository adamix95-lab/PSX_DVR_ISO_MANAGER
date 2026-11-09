#ifndef DESR_V12_ROUGHTIME_WOLFCRYPT_H
#define DESR_V12_ROUGHTIME_WOLFCRYPT_H
#include "v12_roughtime_verify.h"

/* Only compiled in the separately isolated PS2 EE N3B crypto audit.
 * Does not ship inside Manager 1.2, and must not be used with N3A's
 * predictable nonce or without a pinned *production* Roughtime key.
 */
v12_rt_crypto v12_rt_wolfcrypt_backend(void);
#endif

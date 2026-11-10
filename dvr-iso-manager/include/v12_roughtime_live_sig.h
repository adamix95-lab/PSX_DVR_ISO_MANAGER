#ifndef DESR_V12_ROUGHTIME_LIVE_SIG_H
#define DESR_V12_ROUGHTIME_LIVE_SIG_H
#include <stddef.h>
#include <stdint.h>
/* READ-ONLY N3B signature diagnostic. Existing N3A request nonce is
 * predictable and replayable. A PASS is NEVER authenticated time and
 * MUST NOT be consumed by TLS certificate validation or set the RTC.
 */
int v12_roughtime_live_sig_verify(const uint8_t *reply,size_t reply_size,
                                  const uint8_t request_nonce[32]);
#endif

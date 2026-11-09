#ifndef DESR_V12_RT_JOURNAL_H
#define DESR_V12_RT_JOURNAL_H
#include <stddef.h>
#include <stdint.h>
#include "v12_rt_reservation.h"

/* Host-only N3C record format. NO PFS/APA/RTC/network I/O.
 * CRC32 protects against ACCIDENTAL corruption only; it does NOT
 * authenticate a counter or prevent rollback to a restored disk image.
 * A persistence adapter must perform crash-durable alternating-slot
 * updates and re-read verification before any nonce becomes available.
 */
#define V12_RT_RECORD_BYTES 64U
typedef struct {
  v12_rt_reservation reservation;
  uint64_t sequence;
} v12_rt_journal_state;

/* Encodes versioned, endian-stable 64-byte metadata (NO SECRET).
 * Returns -1 on invalid input and leaves output unchanged on failure.
 */
int v12_rt_journal_encode(const v12_rt_journal_state *state,
                          uint8_t record[V12_RT_RECORD_BYTES]);
int v12_rt_journal_decode(const uint8_t record[V12_RT_RECORD_BYTES],
                          v12_rt_journal_state *out);

/* Fail closed on any corrupted (non-blank) slot. The *initial* provisioning
 * state is one valid sequence=1 record and one all-zero slot. Later both
 * slots must be valid. No automatic recovery or rollback on corruption.
 * A tie is allowed ONLY when records are byte-identical.
 */
int v12_rt_journal_select(const uint8_t slots[2][V12_RT_RECORD_BYTES],
                          v12_rt_journal_state *out,
                          unsigned *active_slot);
#endif

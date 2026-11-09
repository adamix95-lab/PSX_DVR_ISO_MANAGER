#ifndef DESR_V12_RT_RESERVATION_H
#define DESR_V12_RT_RESERVATION_H
#include <stdint.h>
#include "v12_roughtime_nonce.h"

/*
 * HOST-TESTED N3C POLICY ONLY. Not linked into any DESR Manager executable.
 * Not a filesystem implementation, secret store, trusted clock or RNG.
 *
 * Prerequisites before the console may invoke this:
 * - one-time 32-byte secret generated with a trusted CSPRNG and kept private;
 * - unique 16-byte key identifier provisioned alongside that secret;
 * - persistent record for that identifier with last_reserved counter;
 * - reserve_and_sync MUST implement crash-durable compare-and-swap on
 *   exactly that key_id and counter, with power-loss-safe commit semantics;
 * - load MUST return the DURABLE state, not an unflushed cache snapshot;
 * - provision a NEW secret and key_id after restoring older HDD backups,
 *   rollbacks or cloning devices; do not reuse a previously reserved nonce.
 *
 * A callback implementation cannot be assumed durable solely because these
 * unit tests pass. The real PFS mount/rename/fsync behavior must be validated
 * separately on DESR hardware before enabling ANY network call.
 */
typedef struct {
  uint8_t key_id[16];
  uint64_t last_reserved;
} v12_rt_reservation;

typedef struct {
  void *ctx;
  int (*load)(void *ctx, v12_rt_reservation *current);
  int (*reserve_and_sync)(void *ctx,
                          const v12_rt_reservation *expected,
                          const v12_rt_reservation *next);
} v12_rt_reservation_store;

/* 0 only when reservation was acknowledged AND independently read back
 * before nonce derivation. No output is written on failure.
 * Counter values 0..UINT64_MAX-2 allowed in stored state, never wrapped.
 */
int v12_rt_reserve_nonce(const uint8_t secret[32],
                         const uint8_t expected_key_id[16],
                         const v12_rt_reservation_store *store,
                         v12_rt_hmac256 hmac, void *hmac_ctx,
                         uint8_t nonce[32]);

#endif

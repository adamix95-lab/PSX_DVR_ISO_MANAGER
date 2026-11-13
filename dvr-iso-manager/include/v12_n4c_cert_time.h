#ifndef DESR_V12_N4C_CERT_TIME_H
#define DESR_V12_N4C_CERT_TIME_H
#include <stdint.h>
#include "v12_roughtime_verify.h"

/* Fail-closed certificate-time interval evaluation. Pure C99, NO I/O.
 *
 * Inputs from CALLER:
 *  - signed_time: only from a successful STRICT pinned-root Roughtime check
 *    using a new cryptographically strong nonce (never forensic fallback).
 *  - monotonic timestamps: one nonwrapping clock, same timebase, ms.
 *
 * This helper DOES NOT authenticate nonce/clock by itself, does not install
 * wc_SetTimeCb, does not parse ASN.1, or check CA chain / hostname.
 * The full interval must fit inside a verified certificate validity interval.
 */
typedef struct {
    uint64_t lower_utc, upper_utc;  /* Unix seconds, inclusive conservative */
} v12_n4c_utc_window;

/* Return 0 only if bounds and freshness/time guards pass.
 * recv_ms - send_ms <= 6000; now_ms - recv_ms <= 30000;
 * signed radius 1..15s; signed midpoint between 2025 and 2037.
 * On failure leave output unchanged. No dependence on hardware RTC.
 */
int v12_n4c_time_window(const v12_rt_signed_time *signed_time,
                        uint64_t send_ms, uint64_t recv_ms, uint64_t now_ms,
                        v12_n4c_utc_window *output);

/* Caller MUST authenticate cert validity values with a CA-verified chain.
 * Both lower and upper UTC must be covered. Any boundary crossing rejects.
 */
int v12_n4c_cert_time_valid(const v12_n4c_utc_window *window,
                            uint64_t not_before_utc,
                            uint64_t not_after_utc);
#endif

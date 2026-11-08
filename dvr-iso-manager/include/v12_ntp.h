#ifndef DESR_V12_NTP_H
#define DESR_V12_NTP_H

#include <stddef.h>
#include <stdint.h>

/* NTPv4/SNTP bootstrap for a console with a dead RTC.
 * UNAUTHENTICATED network time: informational ONLY, NOT a trusted TLS clock.
 * No RTC updates, file writes, secure-time guarantees or NTS claims.
 */
#define V12_NTP_PACKET_SIZE 48U

typedef struct {
  uint32_t unix_utc;
  unsigned year, month, day, hour, minute, second;
  unsigned stratum;
} v12_ntp_time;

/* The token is for matching a reply, NOT a cryptographic secret. */
int v12_ntp_prepare(uint8_t out[V12_NTP_PACKET_SIZE], uint64_t token);

/* Validate one complete NTP reply, matched to request. Reject Kiss-o'-Death,
 * unsynchronised sources, invalid modes/strata, out-of-range years, and
 * packets with an unexpected originate timestamp. The result remains
 * UNTRUSTED even if this parser succeeds.
 */
int v12_ntp_parse(const uint8_t *reply, size_t size,
                  const uint8_t request[V12_NTP_PACKET_SIZE],
                  v12_ntp_time *out);
#endif

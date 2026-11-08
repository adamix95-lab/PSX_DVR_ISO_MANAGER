#ifndef DESR_V12_TLS_CLOCK_H
#define DESR_V12_TLS_CLOCK_H

/* Host-testable validation of the Sony PS2 sceCdCLOCK BCD fields.
 * A plausible RTC is NOT a trusted time source for HTTPS certificates.
 * No RTC writes or state changes are performed by this policy.
 */
typedef struct {
  unsigned year, month, day, hour, minute, second;
} v12_tls_clock;

int v12_tls_clock_decode(unsigned status, unsigned bcd_year,
                         unsigned bcd_month, unsigned bcd_day,
                         unsigned bcd_hour, unsigned bcd_minute,
                         unsigned bcd_second, v12_tls_clock *out);

#endif

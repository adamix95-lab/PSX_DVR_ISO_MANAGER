#ifndef DESR_V12_N3E_UTC_H
#define DESR_V12_N3E_UTC_H
#include <stdint.h>
#include "v12_roughtime_verify.h"

/* Pure host-testable decoder: Unix UTC seconds, IETF draft-08.
 * Never writes RTC, sets TLS time, or claims cross-reboot freshness.
 * All three output arrays must contain at least 20 characters.
 */
#define V12_N3E_UTC_BUF_SIZE 20U
int v12_n3e_utc_format(uint64_t seconds, char out[20]);
int v12_n3e_signed_interval(const v12_rt_signed_time *signed_time,
                            char mid[20], char low[20], char high[20]);
#endif

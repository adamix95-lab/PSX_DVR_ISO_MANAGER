#include "v12_n4c_cert_time.h"
#include <stddef.h>

#define MIN_UNIX_UTC UINT64_C(1735689600) /* 2025-01-01 */
#define MAX_UNIX_UTC UINT64_C(2145916800) /* 2038-01-01, exclusive */
#define MAX_RADIUS_SECONDS 15U
#define MAX_RTT_MS UINT64_C(6000)
#define MAX_AGE_MS UINT64_C(30000)

static uint64_t ceil_ms_to_s(uint64_t ms) {
    return ms/UINT64_C(1000) + (ms%UINT64_C(1000)!=0U);
}
int v12_n4c_time_window(const v12_rt_signed_time *t,
                        uint64_t sent_ms, uint64_t received_ms,
                        uint64_t now_ms, v12_n4c_utc_window *out) {
    uint64_t rtt, age, extra, lower, upper;
    v12_n4c_utc_window new_window;
    if(!t || !out || received_ms<sent_ms || now_ms<received_ms ||
       t->radius_seconds<1U || t->radius_seconds>MAX_RADIUS_SECONDS ||
       t->midpoint_utc<MIN_UNIX_UTC || t->midpoint_utc>=MAX_UNIX_UTC)
        return -1;
    rtt=received_ms-sent_ms;
    age=now_ms-received_ms;
    if(rtt>MAX_RTT_MS || age>MAX_AGE_MS) return -1;
    extra=ceil_ms_to_s(rtt)+ceil_ms_to_s(age);
    /* A signed server time lies inside MIDP +/- RADI. Due to bounded
     * transport and client elapsed time, upper is widened by the
     * roundtrip and age; keeping the original signed lower is conservative.
     * This is a *policy window*, not an authenticated synchronization of
     * clocks or a proof of the external monotonic timer's integrity.
     */
    lower=t->midpoint_utc-(uint64_t)t->radius_seconds;
    upper=t->midpoint_utc+(uint64_t)t->radius_seconds+extra;
    if(lower<MIN_UNIX_UTC || upper>=MAX_UNIX_UTC || lower>upper)
        return -1;
    new_window.lower_utc=lower;
    new_window.upper_utc=upper;
    *out=new_window;
    return 0;
}
int v12_n4c_cert_time_valid(const v12_n4c_utc_window *w,
                            uint64_t not_before_utc,
                            uint64_t not_after_utc) {
    if(!w || not_before_utc>=not_after_utc ||
       w->lower_utc>w->upper_utc ||
       w->lower_utc<MIN_UNIX_UTC || w->upper_utc>=MAX_UNIX_UTC)
        return -1;
    /* A single MIDP match isn't enough: reject cert if any possible
     * client time lies outside the certified validity window. */
    if(w->lower_utc<not_before_utc || w->upper_utc>not_after_utc)
        return -1;
    return 0;
}

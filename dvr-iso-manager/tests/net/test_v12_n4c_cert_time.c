#include "v12_n4c_cert_time.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    /* User-confirmed N3E MIDP: 2026-11-13 06:59:06 UTC. */
    const v12_rt_signed_time signed_t={UINT64_C(1794553146),3};
    const v12_n4c_utc_window untouched={777,888};
    v12_n4c_utc_window w=untouched;
    v12_rt_signed_time bad=signed_t;
    assert(v12_n4c_time_window(&signed_t,100000,100950,101025,&w)==0);
    assert(w.lower_utc==signed_t.midpoint_utc-3U);
    /* 950ms roundtrip => +1s, 75ms age => +1s */
    assert(w.upper_utc==signed_t.midpoint_utc+3U+2U);
    assert(v12_n4c_cert_time_valid(&w,w.lower_utc,w.upper_utc)==0);
    assert(v12_n4c_cert_time_valid(&w,w.lower_utc+1,w.upper_utc)<0);
    assert(v12_n4c_cert_time_valid(&w,w.lower_utc,w.upper_utc-1)<0);
    assert(v12_n4c_cert_time_valid(&w,w.upper_utc,w.lower_utc)<0);
    assert(v12_n4c_cert_time_valid(NULL,w.lower_utc,w.upper_utc)<0);
    assert(v12_n4c_time_window(NULL,0,0,0,&w)<0);
    assert(v12_n4c_time_window(&signed_t,0,0,0,NULL)<0);
    w=untouched;
    assert(v12_n4c_time_window(&signed_t,110,100,100,&w)<0);
    assert(v12_n4c_time_window(&signed_t,100,110,109,&w)<0);
    assert(v12_n4c_time_window(&signed_t,0,6001,6001,&w)<0);
    assert(v12_n4c_time_window(&signed_t,0,0,30001,&w)<0);
    assert(w.lower_utc==777 && w.upper_utc==888);
    bad.radius_seconds=0;
    assert(v12_n4c_time_window(&bad,1,1,1,&w)<0);
    bad.radius_seconds=16;
    assert(v12_n4c_time_window(&bad,1,1,1,&w)<0);
    bad=signed_t; bad.midpoint_utc=UINT64_MAX;
    assert(v12_n4c_time_window(&bad,1,1,1,&w)<0);
    bad.midpoint_utc=UINT64_C(2145916799);
    assert(v12_n4c_time_window(&bad,1,1,1,&w)<0);
    bad.midpoint_utc=UINT64_C(1735689600);
    assert(v12_n4c_time_window(&bad,1,1,1,&w)<0);
    bad.midpoint_utc=UINT64_C(0);
    assert(v12_n4c_time_window(&bad,1,1,1,&w)<0);
    /* Passing MIDP when certificate expires at signed MIDP must reject:
     * the true time might be in the signed upper range. */
    w.lower_utc=signed_t.midpoint_utc-3U;
    w.upper_utc=signed_t.midpoint_utc+3U;
    assert(v12_n4c_cert_time_valid(&w,signed_t.midpoint_utc-999,
                                    signed_t.midpoint_utc)<0);
    puts("PASS: N4C window, max age/rtt, overflow, stale and cert boundaries");
    return 0;
}

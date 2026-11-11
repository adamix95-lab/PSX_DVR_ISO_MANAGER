#include "v12_n3e_utc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static void check(uint64_t t,const char *expected) {
  char s[20];
  assert(v12_n3e_utc_format(t,s)==0);
  assert(!strcmp(s,expected));
}
int main(void) {
  char lo[20],mid[20],hi[20];
  v12_rt_signed_time signed_t={UINT64_C(1762819200),3U}; /* 2025-11-11 00:00 UTC */
  check(UINT64_C(1577836800),"2020-01-01 00:00:00");
  check(UINT64_C(1582934400),"2020-02-29 00:00:00");
  check(UINT64_C(1709164800),"2024-02-29 00:00:00");
  check(UINT64_C(1762819200),"2025-11-11 00:00:00");
  check(UINT64_C(4102444799),"2099-12-31 23:59:59");
  assert(v12_n3e_utc_format(UINT64_C(1577836799),mid)<0);
  assert(v12_n3e_utc_format(UINT64_C(4102444800),mid)<0);
  assert(v12_n3e_utc_format(UINT64_MAX,mid)<0);
  assert(v12_n3e_utc_format(UINT64_C(1762819200),NULL)<0);
  assert(v12_n3e_signed_interval(&signed_t,mid,lo,hi)==0);
  assert(!strcmp(lo,"2025-11-10 23:59:57"));
  assert(!strcmp(mid,"2025-11-11 00:00:00"));
  assert(!strcmp(hi,"2025-11-11 00:00:03"));
  signed_t.radius_seconds=0;assert(v12_n3e_signed_interval(&signed_t,mid,lo,hi)<0);
  signed_t.radius_seconds=301;assert(v12_n3e_signed_interval(&signed_t,mid,lo,hi)<0);
  signed_t.radius_seconds=3;
  signed_t.midpoint_utc=UINT64_C(1577836800);
  assert(v12_n3e_signed_interval(&signed_t,mid,lo,hi)<0);
  signed_t.midpoint_utc=UINT64_C(4102444799);
  assert(v12_n3e_signed_interval(&signed_t,mid,lo,hi)<0);
  assert(v12_n3e_signed_interval(NULL,mid,lo,hi)<0);
  puts("PASS: N3E UTC formatter, leap years, signed ranges, overflow and bounds");
  return 0;
}

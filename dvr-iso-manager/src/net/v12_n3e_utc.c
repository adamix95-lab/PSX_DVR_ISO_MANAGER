#include "v12_n3e_utc.h"
#include <stdio.h>
#include <string.h>

/* Deliberately bound output to 2020..2099 inclusive.
 * This rejects implausible signed time values, but is NOT an
 * anti-replay mechanism and provides NO TLS clock authority.
 */
#define MIN_UNIX_SECONDS UINT64_C(1577836800)
#define MAX_UNIX_SECONDS UINT64_C(4102444800)
#define MAX_DIAGNOSTIC_RADIUS 300U

static int leap(unsigned y) {
  return (y%4U==0U && (y%100U!=0U || y%400U==0U));
}
static unsigned mdays(unsigned y,unsigned m) {
  static const unsigned d[]={31,28,31,30,31,30,31,31,30,31,30,31};
  return d[m-1U]+(m==2U && leap(y)?1U:0U);
}
int v12_n3e_utc_format(uint64_t seconds,char out[20]) {
  unsigned y=1970,m=1,day,hour,minute,sec;
  uint64_t days;
  int len;
  if(!out || seconds<MIN_UNIX_SECONDS || seconds>=MAX_UNIX_SECONDS)
    return -1;
  days=seconds/UINT64_C(86400);
  sec=(unsigned)(seconds%UINT64_C(60));
  minute=(unsigned)((seconds/UINT64_C(60))%UINT64_C(60));
  hour=(unsigned)((seconds/UINT64_C(3600))%UINT64_C(24));
  while(y<2100U) {
    unsigned lenyear=leap(y)?366U:365U;
    if(days<lenyear) break;
    days-=lenyear;
    ++y;
  }
  if(y>=2100U) return -1;
  while(m<=12U) {
    unsigned count=mdays(y,m);
    if(days<count) break;
    days-=count;
    ++m;
  }
  if(m>12U) return -1;
  day=(unsigned)days+1U;
  len=snprintf(out,20,"%04u-%02u-%02u %02u:%02u:%02u",
               y,m,day,hour,minute,sec);
  return len==19?0:-1;
}
int v12_n3e_signed_interval(const v12_rt_signed_time *t,
                            char mid[20],char low[20],char high[20]) {
  uint64_t a,b;
  char a_buf[20],b_buf[20],c_buf[20];
  if(!t || !mid || !low || !high ||
     t->radius_seconds==0U || t->radius_seconds>MAX_DIAGNOSTIC_RADIUS ||
     t->midpoint_utc<MIN_UNIX_SECONDS || t->midpoint_utc>=MAX_UNIX_SECONDS)
    return -1;
  a=t->midpoint_utc-(uint64_t)t->radius_seconds;
  b=t->midpoint_utc+(uint64_t)t->radius_seconds;
  if(v12_n3e_utc_format(a,a_buf)!=0 ||
     v12_n3e_utc_format(t->midpoint_utc,b_buf)!=0 ||
     v12_n3e_utc_format(b,c_buf)!=0) return -1;
  memcpy(low,a_buf,20);
  memcpy(mid,b_buf,20);
  memcpy(high,c_buf,20);
  return 0;
}

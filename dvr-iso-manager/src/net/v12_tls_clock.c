#include "v12_tls_clock.h"

static int from_bcd(unsigned x) {
  unsigned hi=(x>>4)&15U,lo=x&15U;
  if (x>255 || hi>9 || lo>9) return -1;
  return (int)(hi*10U+lo);
}
static unsigned month_days(unsigned year,unsigned month) {
  static const unsigned days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  if (month<1 || month>12) return 0;
  if (month==2 && ((year%4U==0 && year%100U!=0) || year%400U==0))
    return 29;
  return days[month-1];
}

int v12_tls_clock_decode(unsigned status, unsigned by,unsigned bm,
                         unsigned bd,unsigned bh,unsigned bmin,
                         unsigned bs,v12_tls_clock *out) {
  v12_tls_clock parsed;
  int year,month,day,hour,minute,second;
  if(!out || status!=0) return -1;
  /* Some older CDVDMAN versions leave a flag in the high month bit. */
  year=from_bcd(by);
  month=from_bcd(bm&0x7fU);
  day=from_bcd(bd);
  hour=from_bcd(bh);
  minute=from_bcd(bmin);
  second=from_bcd(bs);
  if(year<0||month<0||day<0||hour<0||minute<0||second<0)
    return -1;
  parsed.year=2000U+(unsigned)year;
  parsed.month=(unsigned)month;
  parsed.day=(unsigned)day;
  parsed.hour=(unsigned)hour;
  parsed.minute=(unsigned)minute;
  parsed.second=(unsigned)second;
  /* EE/newlib 32-bit time_t is unreliable beyond early 2038. */
  if(parsed.year<2025U || parsed.year>2037U ||
     parsed.day<1U || parsed.day>month_days(parsed.year,parsed.month) ||
     parsed.hour>23U || parsed.minute>59U || parsed.second>59U)
    return -1;
  *out=parsed;
  return 0;
}

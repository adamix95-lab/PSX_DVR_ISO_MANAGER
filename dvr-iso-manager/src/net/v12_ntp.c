#include "v12_ntp.h"
#include <string.h>

#define NTP_UNIX_EPOCH_DELTA UINT32_C(2208988800)
#define NTP_EARLIEST UINT32_C(3944678400) /* 2025-01-01 UTC */
#define NTP_LAST UINT32_C(4291747199)     /* 2035-12-31 UTC, era 0 */

static uint32_t big32(const uint8_t *data) {
  return ((uint32_t)data[0]<<24) | ((uint32_t)data[1]<<16) |
         ((uint32_t)data[2]<<8) | (uint32_t)data[3];
}
static int leap(unsigned y) {
  return (y%4U==0 && (y%100U!=0 || y%400U==0));
}
static unsigned days_in_month(unsigned y,unsigned m) {
  static const unsigned d[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  if(m<1 || m>12) return 0;
  return (m==2 && leap(y))?29U:d[m-1];
}
static void decode_utc(uint32_t seconds,v12_ntp_time *out) {
  uint32_t days=seconds/86400U, tod=seconds%86400U;
  unsigned y=1970,m;
  while (y<=2035U) {
    unsigned n=leap(y)?366U:365U;
    if (days<n) break;
    days-=n;
    ++y;
  }
  for(m=1;m<=12U;++m) {
    unsigned n=days_in_month(y,m);
    if(days<n) break;
    days-=n;
  }
  out->year=y;out->month=m;out->day=days+1U;
  out->hour=tod/3600U;out->minute=(tod/60U)%60U;out->second=tod%60U;
}

int v12_ntp_prepare(uint8_t out[V12_NTP_PACKET_SIZE],uint64_t token) {
  unsigned i;
  if(!out || !token) return -1;
  memset(out,0,V12_NTP_PACKET_SIZE);
  out[0]=0x23; /* LI=0, version 4, client mode 3 */
  for(i=0;i<8;++i) out[40+i]=(uint8_t)(token>>(56U-8U*i));
  return 0;
}

int v12_ntp_parse(const uint8_t *reply,size_t size,
                  const uint8_t request[V12_NTP_PACKET_SIZE],
                  v12_ntp_time *out) {
  uint32_t ntp_seconds;
  unsigned li,mode,ver,stratum;
  v12_ntp_time parsed;
  if(!reply || !request || !out || size!=V12_NTP_PACKET_SIZE) return -1;
  li=(reply[0]>>6)&3U;ver=(reply[0]>>3)&7U;mode=reply[0]&7U;
  stratum=reply[1];
  if(li==3U || (ver!=3U && ver!=4U) || mode!=4U ||
     stratum==0U || stratum>15U) return -1;
  /* The server must echo the exact client transmit token at bytes 24..31.
   * This is a sanity/replay check only; unauthenticated NTP can be spoofed.
   */
  if(!memcmp(request+40,"\0\0\0\0\0\0\0\0",8) ||
     memcmp(reply+24,request+40,8)) return -1;
  ntp_seconds=big32(reply+40);
  if(ntp_seconds<NTP_EARLIEST || ntp_seconds>NTP_LAST) return -1;
  memset(&parsed,0,sizeof(parsed));
  parsed.unix_utc=ntp_seconds-NTP_UNIX_EPOCH_DELTA;
  parsed.stratum=stratum;
  decode_utc(parsed.unix_utc,&parsed);
  if(parsed.year<2025U || parsed.year>2035U ||
     parsed.month<1U || parsed.month>12U) return -1;
  *out=parsed;
  return 0;
}

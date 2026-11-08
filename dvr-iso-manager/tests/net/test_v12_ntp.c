#include "v12_ntp.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void valid_reply(uint8_t rep[48],const uint8_t req[48],uint32_t sec) {
  memset(rep,0,48);
  rep[0]=0x24; /* LI 0, NTPv4, server mode 4 */
  rep[1]=2;    /* Stratum 2 */
  memcpy(rep+24,req+40,8);
  rep[40]=(uint8_t)(sec>>24);
  rep[41]=(uint8_t)(sec>>16);
  rep[42]=(uint8_t)(sec>>8);
  rep[43]=(uint8_t)sec;
}
int main(void) {
  uint8_t request[48],reply[48];
  v12_ntp_time time={0};
  const uint32_t expected=UINT32_C(0xee9b73b6);
  assert(v12_ntp_prepare(request,UINT64_C(0x1122334455667788))==0);
  assert(request[0]==0x23);
  assert(request[40]==0x11 && request[47]==0x88);
  valid_reply(reply,request,expected);
  assert(v12_ntp_parse(reply,48,request,&time)==0);
  assert(time.year==2026 && time.month==11 && time.day==8);
  assert(time.hour==22 && time.minute==18 && time.second==30);
  assert(time.stratum==2 && time.unix_utc==1794176310U);
  assert(v12_ntp_parse(reply,47,request,&time)<0);
  assert(v12_ntp_parse(reply,49,request,&time)<0);
  reply[0]=0xE4;assert(v12_ntp_parse(reply,48,request,&time)<0); /* LI3 */
  reply[0]=0x23;assert(v12_ntp_parse(reply,48,request,&time)<0); /* client */
  reply[0]=0x24;
  reply[1]=0;assert(v12_ntp_parse(reply,48,request,&time)<0); /* KoD */
  reply[1]=16;assert(v12_ntp_parse(reply,48,request,&time)<0);
  reply[1]=2;
  reply[24]^=1;assert(v12_ntp_parse(reply,48,request,&time)<0); /* replay */
  reply[24]^=1;
  valid_reply(reply,request,UINT32_C(0));
  assert(v12_ntp_parse(reply,48,request,&time)<0); /* stale NTP */
  valid_reply(reply,request,UINT32_C(4291747199));
  assert(v12_ntp_parse(reply,48,request,&time)==0);
  assert(time.year==2035 && time.month==12 && time.day==31);
  valid_reply(reply,request,UINT32_C(3944678400));
  assert(v12_ntp_parse(reply,48,request,&time)==0);
  assert(time.year==2025 && time.month==1 && time.day==1);
  valid_reply(reply,request,UINT32_C(3944678399));
  assert(v12_ntp_parse(reply,48,request,&time)<0);
  valid_reply(reply,request,UINT32_C(4291747200));
  assert(v12_ntp_parse(reply,48,request,&time)<0);
  assert(v12_ntp_prepare(request,0)<0);
  puts("PASS: untrusted NTPv4 strict reply, originate echo and 2025-2035 UTC");
  return 0;
}

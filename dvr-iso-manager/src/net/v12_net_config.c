#include "v12_net_config.h"
#include <string.h>

static int ipv4(const char *str, uint8_t target[4]) {
  int i;
  if (!str || !*str) return -1;
  for (i=0; i<4; ++i) {
    unsigned x=0, digits=0;
    while (*str>='0' && *str<='9') {
      if (++digits>3) return -1;
      x=x*10U+(unsigned)(*str++-'0');
      if (x>255) return -1;
    }
    if (!digits) return -1;
    target[i]=(uint8_t)x;
    if (i<3 && *str++!='.') return -1;
  }
  return *str=='\0' ? 0 : -1;
}
static int valid_unicast(const uint8_t v[4]) {
  /* No unspecified, loopback, link-local, multicast, broadcast. */
  return v[0]>0 && v[0]!=127 && v[0]!=169 && v[0]<224 &&
         !(v[0]==255 && v[1]==255 && v[2]==255 && v[3]==255) &&
         !(v[0]==0 && v[1]==0 && v[2]==0 && v[3]==0);
}
int v12_parse_net_config(const char *data, size_t len,
                         v12_net_config *out) {
  v12_net_config c = {{0}, {0}, {0}, {0}};
  size_t at=0;
  unsigned seen=0;
  if (!data || !out || !len || len>V12_NET_CONFIG_MAX) return -1;
  while (at<len) {
    char line[64], *eq;
    size_t n=0;
    int key=-1;
    while (at<len && data[at]!='\n' && data[at]!='\r') {
      if (n+1>=sizeof(line)) return -1;
      line[n++]=data[at++];
    }
    while (at<len && (data[at]=='\n' || data[at]=='\r')) ++at;
    line[n]='\0';
    if (!n || line[0]=='#') continue;
    eq=strchr(line,'=');
    if (!eq) return -1;
    *eq++='\0';
    if (!strcmp(line,"IP")) key=0;
    else if (!strcmp(line,"NETMASK")) key=1;
    else if (!strcmp(line,"GATEWAY")) key=2;
    else if (!strcmp(line,"DNS")) key=3;
    else return -1;
    if (seen & (1U<<key)) return -1;
    if (ipv4(eq, key==0?c.ip:key==1?c.netmask:key==2?c.gateway:c.dns)<0)
      return -1;
    seen |= 1U<<key;
  }
  if (seen!=15U || !valid_unicast(c.ip) ||
      !valid_unicast(c.gateway) || !valid_unicast(c.dns) ||
      !(c.netmask[0] | c.netmask[1] | c.netmask[2] | c.netmask[3]))
    return -1;
  *out=c;
  return 0;
}

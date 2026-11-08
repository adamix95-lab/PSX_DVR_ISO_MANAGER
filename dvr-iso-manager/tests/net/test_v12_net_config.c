#include "v12_net_config.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static const char good[] =
  "# V12-only. No reference to the PC server.\r\n"
  "IP=192.168.1.90\r\n"
  "NETMASK=255.255.255.0\r\n"
  "GATEWAY=192.168.1.1\r\n"
  "DNS=1.1.1.1\r\n";

static void reject(const char *s) {
  v12_net_config cfg;
  memset(&cfg,0x5a,sizeof(cfg));
  assert(v12_parse_net_config(s,strlen(s),&cfg)<0);
  for(size_t i=0;i<sizeof(cfg);++i)
    assert(((const unsigned char *)&cfg)[i]==0x5a);
}
int main(void) {
  v12_net_config cfg={0};
  assert(v12_parse_net_config(good,strlen(good),&cfg)==0);
  assert(cfg.ip[0]==192 && cfg.ip[3]==90);
  assert(cfg.gateway[0]==192 && cfg.gateway[3]==1);
  assert(cfg.dns[0]==1 && cfg.dns[3]==1);
  assert(cfg.netmask[0]==255 && cfg.netmask[3]==0);
  reject("IP=192.168.1.50\nNETMASK=255.255.255.0\nGATEWAY=192.168.1.1\n");
  reject("IP=192.168.1.50\nIP=192.168.1.51\nNETMASK=255.255.255.0\nGATEWAY=192.168.1.1\nDNS=1.1.1.1\n");
  reject("IP=192.168.1.50\nNETMASK=255.255.255.0\nGATEWAY=192.168.1.1\nDNS=1.1.1.1\nSERVER_IP=1.2.3.4\n");
  reject("IP=192.168.1.50\nNETMASK=255.255.255.0\nGATEWAY=192.168.1.1\nDNS=1.1.1.999\n");
  reject("IP=192.168.1.50\nNETMASK=255.255.255.0\nGATEWAY=192.168.1.1\nDNS=1.1.1.1/anything\n");
  reject("IP=127.0.0.1\nNETMASK=255.0.0.0\nGATEWAY=192.168.1.1\nDNS=1.1.1.1\n");
  reject("IP=224.2.3.4\nNETMASK=255.0.0.0\nGATEWAY=192.168.1.1\nDNS=1.1.1.1\n");
  reject("IP=192.168.1.50\nNETMASK=0.0.0.0\nGATEWAY=192.168.1.1\nDNS=1.1.1.1\n");
  reject("IP=192.168.1.50\nNETMASK=255.255.255.0\nGATEWAY=192.168.1.1\nDNS=1.1.1.1\nIP=192.168.1.50\n");
  reject("IP=192.168.1.50\nNETMASK=255.255.255.0\nGATEWAY=192.168.1.1\nDNS=raw.githubusercontent.com\n");
  puts("PASS: V12 network config is isolated, strict and read-only");
  return 0;
}

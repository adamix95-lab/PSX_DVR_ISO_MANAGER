/* v0.7.3: optional Ethernet only AFTER explicit controller consent.
 * All USB/DVR base IRXs are loaded by the proven Manager init_drivers().
 * No IOP reset, no second DEV9 and no changes to Quickboot modules.
 */
#include "lan_art_sync.h"
#include "art_sync_runtime.h"
#include "probe_config.h"
#include <debug.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libcdvd.h>
#include <kernel.h>
#include <loadfile.h>
#include <netman.h>
#include <ps2ip.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define LAN_CFG "mass0:/DESR_DVR/NET_PROBE.CNF"
#define LAN_CFG_CAP 512

extern unsigned char netman_irx[] __attribute__((aligned(16)));
extern uint32_t size_netman_irx;
extern unsigned char smap_irx[] __attribute__((aligned(16)));
extern uint32_t size_smap_irx;

static char lan_config[LAN_CFG_CAP + 1];
static int network_started;

int desr_lan_art_missing(void) {
  return art_sync_prepare();
}
static int config_read(probe_config *out) {
  int fd, n, bytes=0, close_result;
  fd=fileXioOpen(LAN_CFG,FIO_O_RDONLY,0);
  if (fd<0) {scr_printf("NET_PROBE.CNF missing.\n");return -1;}
  while (bytes <= LAN_CFG_CAP) {
    n=fileXioRead(fd,lan_config+bytes,LAN_CFG_CAP+1-bytes);
    if (n<=0) break;
    bytes+=n;
  }
  close_result=fileXioClose(fd);
  if (n<0 || close_result<0 || bytes==0 || bytes>LAN_CFG_CAP) return -1;
  lan_config[bytes]='\0';
  if (probe_parse_config(lan_config,(size_t)bytes,out)<0) {
    scr_printf("Invalid NET_PROBE.CNF\n");
    return -1;
  }
  return 0;
}
static int load_network_irx(const char *name, unsigned char *buffer,
                            uint32_t bytes) {
  int rv=0,id=SifExecModuleBuffer(buffer,bytes,0,NULL,&rv);
  if (id<0 || rv<0) {
    scr_printf("%s IRX FAILED: id=%d rv=%d\n",name,id,rv);
    return -1;
  }
  return 0;
}
static int network_init(const probe_config *cfg) {
  struct ip4_addr ip,mask,gw;
  int link=0,i;
  if (network_started) return 0;
  scr_printf("Starting NETMAN + SMAP (no extra DEV9)\n");
  if (load_network_irx("netman",netman_irx,size_netman_irx)<0 ||
      load_network_irx("smap",smap_irx,size_smap_irx)<0) return -1;

  IP4_ADDR(&ip,cfg->ip[0],cfg->ip[1],cfg->ip[2],cfg->ip[3]);
  IP4_ADDR(&mask,cfg->netmask[0],cfg->netmask[1],
           cfg->netmask[2],cfg->netmask[3]);
  IP4_ADDR(&gw,cfg->gateway[0],cfg->gateway[1],
           cfg->gateway[2],cfg->gateway[3]);
  if (ps2ipInit(&ip,&mask,&gw)<0) {
    scr_printf("PS2IP init failed\n");
    return -1;
  }
  for (i=0;i<6;i++) {
    link=NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS,NULL,0,NULL,0);
    if (link==NETMAN_NETIF_ETH_LINK_STATE_UP) break;
    sleep(1);
  }
  if (link!=NETMAN_NETIF_ETH_LINK_STATE_UP) {
    scr_printf("Ethernet cable/link DOWN\n");
    return -1;
  }
  network_started=1;
  return 0;
}
int desr_lan_art_download(void) {
  probe_config cfg;
  if (config_read(&cfg)<0) return -1;
  if (network_init(&cfg)<0) return -1;
  return art_sync_run(&cfg);
}

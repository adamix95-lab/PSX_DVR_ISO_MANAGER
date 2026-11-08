/* Ethernet is initialized on demand after the user selects X on the
 * cover scan screen. The DVR DEV9 and USB stack are already initialized.
 */
#include "lan_art_sync.h"
#include "art_sync_runtime.h"
#include "probe_config.h"
#include <debug.h>
#include <errno.h>
#include <stdint.h>
#include "runtime_storage.h"
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
#ifdef DESR_HDD_V12
#include "v12_net_config.h"
#include "v12_https_policy.h"
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#endif

#define LAN_CFG DESR_APP_NET_CFG
#define LAN_CFG_CAP 512

extern unsigned char netman_irx[] __attribute__((aligned(16)));
extern uint32_t size_netman_irx;
extern unsigned char smap_irx[] __attribute__((aligned(16)));
extern uint32_t size_smap_irx;

static char lan_config[LAN_CFG_CAP + 1];
static int network_started;

int desr_lan_art_started(void) { return network_started; }

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
#ifdef DESR_HDD_NETFIX
  scr_printf("NETFIX: USB DEV9 / pinned SDK / isolated RX / TCP checksum\n");
#endif
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
  /* No network/module loading unless missing covers were just confirmed. */
  if (config_read(&cfg)<0) return -1;
  if (network_init(&cfg)<0) return -1;
  return art_sync_run(&cfg);
}

#ifdef DESR_HDD_ART_PROBE
int desr_lan_art_probe(const desr_cover_entry covers[], int count) {
  probe_config cfg;
  if (!covers || count<=0) return -1;
  if (config_read(&cfg)<0) return -1;
  if (network_init(&cfg)<0) return -1;
  return art_sync_probe_all(&cfg,covers,count);
}
#endif

#ifdef DESR_HDD_V12
/* READ-ONLY INTERNET PREFLIGHT: static IP + configurable DNS -> raw.github
 * DNS -> TCP 443. No HTTP bytes, no TLS handshake and no HDD writes.
 * Only the standalone 1.2 preview calls this function on TRIANGLE.
 */
static int v12_read_network_config(v12_net_config *out) {
  char text[V12_NET_CONFIG_MAX + 1U];
  int fd, total=0, n, close_rc;
  fd=fileXioOpen(DESR_APP_ROOT "V12_NET.CNF", FIO_O_RDONLY, 0);
  if (fd<0) {
    scr_printf("V12_NET.CNF missing from PP.DVRISO12 (rc=%d)\n",fd);
    return -301;
  }
  while (total<=(int)V12_NET_CONFIG_MAX) {
    n=fileXioRead(fd, text+total,
                  (int)(V12_NET_CONFIG_MAX+1U)-(int)total);
    if (n<=0) break;
    total+=n;
  }
  close_rc=fileXioClose(fd);
  if (n<0 || close_rc<0 || total==0 ||
      total>(int)V12_NET_CONFIG_MAX ||
      v12_parse_net_config(text,(size_t)total,out)<0) {
    scr_printf("Invalid V12_NET.CNF; use IP/NETMASK/GATEWAY/DNS only\n");
    return -302;
  }
  return 0;
}
int desr_v12_internet_probe(void) {
  v12_net_config net;
  probe_config old_cfg;
  struct addrinfo hints, *resolved=NULL, *cur;
  ip_addr_t dns;
  char host[96], path[192];
  int rc, sock=-1, success=0, last_errno=0;
  if (v12_read_network_config(&net)<0) return -301;
  memset(&old_cfg,0,sizeof(old_cfg));
  memcpy(old_cfg.ip,net.ip,4);
  memcpy(old_cfg.netmask,net.netmask,4);
  memcpy(old_cfg.gateway,net.gateway,4);
  scr_printf("IP %u.%u.%u.%u  gateway %u.%u.%u.%u\n",
             net.ip[0],net.ip[1],net.ip[2],net.ip[3],
             net.gateway[0],net.gateway[1],net.gateway[2],net.gateway[3]);
  if (network_init(&old_cfg)<0) return -303;
  IP4_ADDR(&dns,net.dns[0],net.dns[1],net.dns[2],net.dns[3]);
  dns_setserver(0,&dns);
  scr_printf("DNS server %u.%u.%u.%u\n",
             net.dns[0],net.dns[1],net.dns[2],net.dns[3]);
  if (v12_https_target(V12_ART_COSMICSCALE,"SLES_538.86",
                       host,sizeof(host),path,sizeof(path))<0)
    return -304;
  memset(&hints,0,sizeof(hints));
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_protocol=IPPROTO_TCP;
  scr_printf("Resolving %s ...\n",host);
  rc=getaddrinfo(host,"443",&hints,&resolved);
  if (rc!=0 || !resolved) {
    scr_printf("DNS FAILED: %d\n",rc);
    if (resolved) freeaddrinfo(resolved);
    return -305;
  }
  for (cur=resolved;cur;cur=cur->ai_next) {
    fd_set writes;
    struct timeval tv;
    socklen_t so_len;
    int so_error=0;
    const struct sockaddr_in *peer;
    const unsigned char *octets;
    if (cur->ai_family!=AF_INET || cur->ai_addrlen<sizeof(struct sockaddr_in))
      continue;
    peer=(const struct sockaddr_in *)cur->ai_addr;
    octets=(const unsigned char *)&peer->sin_addr.s_addr;
    scr_printf("DNS OK %u.%u.%u.%u\n",
               octets[0],octets[1],octets[2],octets[3]);
    sock=lwip_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sock<0) {last_errno=errno;continue;}
    if (fcntl(sock,F_SETFL,O_NONBLOCK)<0) {
      last_errno=errno;
      lwip_close(sock);sock=-1;continue;
    }
    rc=lwip_connect(sock,cur->ai_addr,cur->ai_addrlen);
    if (rc<0 && errno!=EINPROGRESS && errno!=EWOULDBLOCK &&
        errno!=EALREADY) {last_errno=errno;lwip_close(sock);sock=-1;continue;}
    if (rc<0) {
      FD_ZERO(&writes);FD_SET(sock,&writes);
      tv.tv_sec=8;tv.tv_usec=0;
      if (lwip_select(sock+1,NULL,&writes,NULL,&tv)<=0 ||
          !FD_ISSET(sock,&writes)) {
        last_errno=ETIMEDOUT;lwip_close(sock);sock=-1;continue;
      }
      so_len=sizeof(so_error);
      if (lwip_getsockopt(sock,SOL_SOCKET,SO_ERROR,&so_error,&so_len)<0 ||
          so_error!=0) {
        last_errno=so_error?so_error:errno;
        lwip_close(sock);sock=-1;continue;
      }
    }
    success=1;
    lwip_close(sock);
    sock=-1;
    break;
  }
  freeaddrinfo(resolved);
  if (!success) {
    scr_printf("TCP/443 FAILED; errno=%d\n",last_errno);
    return -306;
  }
  scr_printf("TCP/443 CONNECTED. NO HTTP / TLS / ART WRITES.\n");
  return 0;
}
#endif /* DESR_HDD_V12 */

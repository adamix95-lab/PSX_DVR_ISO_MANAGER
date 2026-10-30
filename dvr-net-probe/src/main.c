/*
 * PSX DESR-7000 local PNG download test — STANDALONE ELF.
 * Never writes to DVR, APA, ISO, ART, COVERS.IDX or BOOT.CNF.
 * Writes only new USB NET_DOWNLOAD_TEST.PNG after PNG validation.
 * The proven DVR ISO Manager, Quickboot and covers are not linked here.
 * No HDD/APA mounting or mutation; dvrfile is loaded for compatibility only.
 */
#include "probe_config.h"
#include "probe_png.h"
#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <libcdvd.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#include <netman.h>
#include <ps2ip.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CONFIG_PATH "mass0:/DESR_DVR/NET_PROBE.CNF"
#define CFG_MAX 512

#define IRX(name) \
  extern unsigned char name##_irx[] __attribute__((aligned(16))); \
  extern uint32_t size_##name##_irx
IRX(iomanX); IRX(fileXio); IRX(bdm); IRX(bdmfs_fatfs);
IRX(usbd_mini); IRX(usbmass_bd_mini);
IRX(dev9_ns); IRX(dvrdrv); IRX(dvrfile);
IRX(netman); IRX(smap);

static char config_buf[CFG_MAX + 1];

static int module(const char *name, void *irx, uint32_t size) {
  int ret = 0;
  int id = SifExecModuleBuffer(irx, size, 0, NULL, &ret);
  if (id < 0 || ret < 0)
    scr_printf("ERROR: %s id=%d result=%d\n", name, id, ret);
  return (id < 0 || ret < 0) ? -1 : 0;
}
#define LOAD(x) do { if (module(#x, x##_irx, size_##x##_irx) < 0) return -1; } while (0)

static int start_dvr_stack(void) {
  int fd;
  scr_printf("Init existing USB & DVR IRXs...\n");
  while (!SifIopReset("", 0)) {}
  while (!SifIopSync()) {}
  sceSifInitRpc(0);
  sbv_patch_enable_lmb();
  sbv_patch_disable_prefix_check();
  sbv_patch_fileio();

  LOAD(iomanX);
  LOAD(fileXio);
  fileXioInit();
  LOAD(bdm);
  LOAD(bdmfs_fatfs);
  LOAD(usbd_mini);
  LOAD(usbmass_bd_mini);
  LOAD(dev9_ns);
  LOAD(dvrdrv);
  LOAD(dvrfile);
#undef LOAD
  /* This call is part of the verified v0.5.3 driver initialization. */
  scr_printf("DVR notice: %d\n", sceCdNoticeGameStart(0, NULL));
  sleep(2);
  fd = fileXioDopen("dvr_pfs0:/ISO");
  if (fd < 0) {
    scr_printf("DVR ISO read-only open failed: %d\n", fd);
    return -1;
  }
  fileXioDclose(fd);
  scr_printf("DVR ISO directory: READ-ONLY OK\n");
  return 0;
}

static int read_config(probe_config *cfg) {
  int fd, size = 0, n;
  fd = fileXioOpen(CONFIG_PATH, FIO_O_RDONLY, 0);
  if (fd < 0) { scr_printf("NET_PROBE.CNF missing (%d)\n", fd); return -1; }
  while (size < CFG_MAX + 1) {
    n = fileXioRead(fd, config_buf + size, CFG_MAX + 1 - size);
    if (n < 0) { fileXioClose(fd); return -1; }
    if (!n) break;
    size += n;
  }
  fileXioClose(fd);
  if (!size || size > CFG_MAX) {
    scr_printf("NET_PROBE.CNF too large/empty\n");
    return -1;
  }
  config_buf[size] = '\0';
  if (probe_parse_config(config_buf, (size_t)size, cfg) < 0) {
    scr_printf("NET_PROBE.CNF invalid: check IPv4/port\n");
    return -1;
  }
  return 0;
}
static void print_ip(const uint8_t a[4]) {
  scr_printf("%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
}
static void linger_result(const char *reason) {
  scr_printf("\n%s\n", reason);
  scr_printf("Test-only USB file. Reboot to exit.\n");
  while (1) sleep(2);
}

/* One single LAN-only PNG transfer. No DNS, TLS, redirects or cloud APIs.
   A complete, CRC-checked PNG is held in EE memory before USB write.
   This is a throwaway test file outside ART; it is NEVER used by Manager. */
#define PNG_REQUEST_PATH "/NET_TEST_COV.png"
#define PNG_DEST_PATH "mass0:/DESR_DVR/NET_DOWNLOAD_TEST.PNG"
#define HTTP_BUFFER_CAP (PROBE_HTTP_HEADER_MAX + PROBE_PNG_MAX)
static uint8_t response[HTTP_BUFFER_CAP + 1U];

static int io_ready(int socket_fd, int write_ready) {
  fd_set fds;
  struct timeval tv;
  int rc;
  FD_ZERO(&fds);
  FD_SET(socket_fd,&fds);
  tv.tv_sec=5;
  tv.tv_usec=0;
  rc=lwip_select(socket_fd+1,write_ready?NULL:&fds,
                 write_ready?&fds:NULL,NULL,&tv);
  return rc>0 && FD_ISSET(socket_fd,&fds);
}
static int http_probe(const probe_config *cfg) {
  struct sockaddr_in addr;
  struct ip4_addr server_addr;
  char req[180];
  const uint8_t *png;
  size_t received=0, png_bytes=0;
  int sock=-1, fd, rc, soerr=0, n, written=0;
  socklen_t optlen=sizeof(soerr);
  unsigned offset=0;

  /* If a previous test was successful, fail closed rather than truncate it.
     Users can delete NET_DOWNLOAD_TEST.PNG on the PC to retest. */
  fd=fileXioOpen(PNG_DEST_PATH,FIO_O_RDONLY,0);
  if (fd>=0) {
    fileXioClose(fd);
    scr_printf("STOP: NET_DOWNLOAD_TEST.PNG already exists.\n");
    return -1;
  }

  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(cfg->port);
  IP4_ADDR(&server_addr,cfg->server[0],cfg->server[1],
           cfg->server[2],cfg->server[3]);
  memcpy(&addr.sin_addr,&server_addr,sizeof(addr.sin_addr));
  scr_printf("LAN HTTP -> ");
  print_ip(cfg->server);
  scr_printf(":%u\n",(unsigned)cfg->port);
  sock=lwip_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if (sock<0) { scr_printf("socket failed: %d\n",errno); return -1; }
  if (fcntl(sock,F_SETFL,O_NONBLOCK)<0) {
    scr_printf("nonblocking socket failed: %d\n",errno);
    goto fail;
  }
  rc=lwip_connect(sock,(struct sockaddr*)&addr,sizeof(addr));
  if (rc<0 && errno!=EINPROGRESS && errno!=EWOULDBLOCK &&
      errno!=EALREADY) {
    scr_printf("connect failed: %d\n",errno);
    goto fail;
  }
  if (rc<0) {
    if (!io_ready(sock,1)) {
      scr_printf("TCP connect timeout (5s)\n");
      goto fail;
    }
    if (lwip_getsockopt(sock,SOL_SOCKET,SO_ERROR,&soerr,&optlen)<0 ||
        soerr!=0) {
      scr_printf("connect SO_ERROR=%d\n",soerr);
      goto fail;
    }
  }
  scr_printf("TCP: CONNECTED\n");
  n=snprintf(req,sizeof(req),
      "GET " PNG_REQUEST_PATH " HTTP/1.0\r\n"
      "Host: %u.%u.%u.%u\r\nConnection: close\r\n\r\n",
      cfg->server[0],cfg->server[1],cfg->server[2],cfg->server[3]);
  if (n<=0 || n>=(int)sizeof(req)) goto fail;
  while (offset<(unsigned)n) {
    if (!io_ready(sock,1)) {
      scr_printf("HTTP send timeout\n");
      goto fail;
    }
    rc=lwip_send(sock,req+offset,(unsigned)n-offset,0);
    if (rc<=0) {
      if (rc<0 && (errno==EWOULDBLOCK||errno==EAGAIN)) continue;
      scr_printf("HTTP send failed: %d\n",errno);
      goto fail;
    }
    offset+=(unsigned)rc;
  }
  scr_printf("GET " PNG_REQUEST_PATH "\n");
  for (;;) {
    if (!io_ready(sock,0)) {
      scr_printf("HTTP read timeout (5s)\n");
      goto fail;
    }
    if (received>=HTTP_BUFFER_CAP) {
      scr_printf("HTTP response exceeds 258 KiB cap\n");
      goto fail;
    }
    n=lwip_recv(sock,response+received,HTTP_BUFFER_CAP-received,0);
    if (n==0) break;
    if (n<0) {
      if (errno==EWOULDBLOCK||errno==EAGAIN) continue;
      scr_printf("HTTP read error: %d\n",errno);
      goto fail;
    }
    received+=(size_t)n;
  }
  lwip_close(sock);
  sock=-1;
  if (probe_http_extract_png(response,received,&png,&png_bytes)<0) {
    scr_printf("REJECT: not complete valid HTTP 200 PNG\n");
    scr_printf("Received: %lu bytes\n",(unsigned long)received);
    return -1;
  }
  scr_printf("PNG: VALID CRC, %lu bytes\n",(unsigned long)png_bytes);

  /* The sole write anywhere in this diagnostic: a dedicated non-ART file.
     Never touch ART, COVERS.IDX, BOOT.CNF, DVR or game ISOs. */
  fd=fileXioOpen(PNG_DEST_PATH,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_TRUNC,0666);
  if (fd<0) {
    scr_printf("USB test output open failed: %d\n",fd);
    return -1;
  }
  while (written<(int)png_bytes) {
    int want=(int)png_bytes-written;
    if (want>4096) want=4096;
    rc=fileXioWrite(fd,png+written,want);
    if (rc<=0) {
      scr_printf("USB write failed; check partial test file\n");
      fileXioClose(fd);
      return -1;
    }
    written+=rc;
  }
  fileXioClose(fd);
  scr_printf("USB: NET_DOWNLOAD_TEST.PNG SAVED\n");
  return 0;
fail:
  if (sock>=0) lwip_close(sock);
  return -1;
}
int main(int argc, char **argv) {
  probe_config cfg;
  struct ip4_addr ip, mask, gateway;
  int link = 0, i, http_ok;
  (void)argc; (void)argv;
  init_scr();
  scr_setCursor(0);
  scr_setbgcolor(0x000000);
  scr_setfontcolor(0xFFFFFF);
  scr_printf("DESR LOCAL PNG FETCH v0.7.1 - STANDALONE\n");
  scr_printf("READ-ONLY DVR, SINGLE USB TEST FILE.\n\n");
  if (start_dvr_stack() < 0)
    linger_result("STOP: DVR/USB init failed. Network not started.");
  if (read_config(&cfg) < 0)
    linger_result("STOP: invalid/missing USB config. Network not started.");
  scr_printf("PSX IP: "); print_ip(cfg.ip);
  scr_printf("  Server: "); print_ip(cfg.server);
  scr_printf("\n\nLoad NETMAN + SMAP without second DEV9...\n");
  if (module("netman", netman_irx, size_netman_irx) < 0)
    linger_result("STOP: NETMAN incompatible with DVR stack.");
  if (module("smap", smap_irx, size_smap_irx) < 0)
    linger_result("STOP: SMAP incompatible with DVR dev9_ns.");

  IP4_ADDR(&ip, cfg.ip[0],cfg.ip[1],cfg.ip[2],cfg.ip[3]);
  IP4_ADDR(&mask, cfg.netmask[0],cfg.netmask[1],
           cfg.netmask[2],cfg.netmask[3]);
  IP4_ADDR(&gateway, cfg.gateway[0],cfg.gateway[1],
           cfg.gateway[2],cfg.gateway[3]);
  if (ps2ipInit(&ip, &mask, &gateway) < 0)
    linger_result("STOP: EE TCP/IP init failed.");
  scr_printf("PS2IP: STARTED\n");
  for (i = 0; i < 6; i++) {
    link = NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS,
                       NULL, 0, NULL, 0);
    if (link == NETMAN_NETIF_ETH_LINK_STATE_UP) break;
    sleep(1);
  }
  scr_printf("Ethernet link state: %d\n", link);
  if (link != NETMAN_NETIF_ETH_LINK_STATE_UP)
    linger_result("STOP: no Ethernet link (cable/driver).");
  http_ok = http_probe(&cfg);
  linger_result(http_ok == 0
    ? "PASS: DVR + LAN PNG + USB test output."
    : "FAIL: LAN PNG fetch or USB output failed.");
  return 0;
}

/*
 * PSX DESR-7000 network compatibility diagnostic — STANDALONE ELF.
 * Never writes to DVR, APA, ISO, USB or BOOT.CNF.
 * The proven DVR ISO Manager, Quickboot and covers are not linked here.
 * No HDD/APA mounting or mutation; dvrfile is loaded for compatibility only.
 */
#include "probe_config.h"
#include <debug.h>
#include <errno.h>
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
#define MAX_REPLY 95

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
  scr_printf("  %-15s id=%d result=%d\n", name, id, ret);
  return (id < 0 || ret < 0) ? -1 : 0;
}
#define LOAD(x) do { if (module(#x, x##_irx, size_##x##_irx) < 0) return -1; } while (0)

static int start_dvr_stack(void) {
  int fd;
  scr_printf("Reset IOP / load existing USB & DVR IRX...\n");
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
  scr_printf("No files written. Reboot to leave test.\n");
  while (1) sleep(2);
}

/* A single bounded HTTP/1.0 request to an IP-addressed local server only.
   Nonblocking connect + select timeout; at most 95 bytes of HTTP reply read. */
static int http_probe(const probe_config *cfg) {
  struct sockaddr_in addr;
  fd_set writefds, readfds;
  struct timeval tv;
  char req[160], reply[MAX_REPLY + 1];
  int s = -1, rc, soerr = 0, n;
  socklen_t optlen = sizeof(soerr);
  unsigned long nonblock = 1;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(cfg->port);
  IP4_ADDR((ip4_addr_t *)&addr.sin_addr, cfg->server[0],
           cfg->server[1], cfg->server[2], cfg->server[3]);
  scr_printf("TCP -> ");
  print_ip(cfg->server);
  scr_printf(":%u\n", (unsigned)cfg->port);
  s = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s < 0) { scr_printf("socket: %d\n", errno); return -1; }
  if (lwip_ioctl(s, FIONBIO, &nonblock) < 0) {
    scr_printf("nonblocking socket unsupported: %d\n", errno);
    lwip_close(s);
    return -1;
  }
  rc = lwip_connect(s, (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0 && errno != EINPROGRESS && errno != EWOULDBLOCK &&
      errno != EALREADY) {
    scr_printf("connect failed: %d\n", errno);
    lwip_close(s); return -1;
  }
  if (rc < 0) {
    FD_ZERO(&writefds);
    FD_SET(s, &writefds);
    tv.tv_sec = 5; tv.tv_usec = 0;
    rc = lwip_select(s + 1, NULL, &writefds, NULL, &tv);
    if (rc <= 0 || !FD_ISSET(s, &writefds)) {
      scr_printf("TCP timeout / error (5s): %d\n", rc);
      lwip_close(s); return -1;
    }
    if (lwip_getsockopt(s, SOL_SOCKET, SO_ERROR, &soerr, &optlen) < 0 ||
        soerr != 0) {
      scr_printf("connect SO_ERROR: %d\n", soerr);
      lwip_close(s); return -1;
    }
  }
  scr_printf("TCP: CONNECTED\n");
  /* HTTP GET / contains no user data; no storage is modified. */
  n = snprintf(req, sizeof(req),
       "GET / HTTP/1.0\r\nHost: %u.%u.%u.%u\r\nConnection: close\r\n\r\n",
       cfg->server[0],cfg->server[1],cfg->server[2],cfg->server[3]);
  if (n <= 0 || n >= (int)sizeof(req) || lwip_send(s, req, n, 0) != n) {
    scr_printf("HTTP request failed: %d\n", errno);
    lwip_close(s); return -1;
  }
  FD_ZERO(&readfds);
  FD_SET(s, &readfds);
  tv.tv_sec = 5; tv.tv_usec = 0;
  rc = lwip_select(s + 1, &readfds, NULL, NULL, &tv);
  if (rc <= 0 || !FD_ISSET(s, &readfds)) {
    scr_printf("HTTP timeout / error (5s): %d\n", rc);
    lwip_close(s); return -1;
  }
  n = lwip_recv(s, reply, MAX_REPLY, 0);
  lwip_close(s);
  if (n <= 0) { scr_printf("HTTP empty reply\n"); return -1; }
  reply[n] = '\0';
  for (rc = 0; rc < n; rc++) {
    if (reply[rc] == '\r' || reply[rc] == '\n') { reply[rc] = '\0'; break; }
    if ((unsigned char)reply[rc] < 32 ||
        (unsigned char)reply[rc] > 126) reply[rc] = '?';
  }
  scr_printf("HTTP: %.60s\n", reply);
  return !strncmp(reply, "HTTP/1.", 7) ? 0 : -1;
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
  scr_printf("DESR NETWORK PROBE v0.7 - STANDALONE\n");
  scr_printf("READ-ONLY DVR + LOCAL HTTP. NO QUICKBOOT.\n\n");
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
    ? "PASS: DVR + Ethernet + local HTTP response."
    : "PARTIAL: DVR + Ethernet OK; HTTP failed.");
  return 0;
}

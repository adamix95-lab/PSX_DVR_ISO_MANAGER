/*
 * DESR v1.2 N3B STANDALONE: isolate boot/IOP/NETFIX/Ed25519 stages.
 * This is NOT the Manager: no cover/ISO browser, no RTC updates, no
 * writable mounts, no app/Game ID changes, no HTTP/TLS or filesystem writes.
 * The diagnostic weak nonce is intentionally NOT a trusted time source.
 *
 * Based on the N3C hardware-proven standalone IOP / PFS init sequence and
 * N3A hardware-proven UDP/2003 network stack, but smaller than full Manager.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <tamtypes.h>
#include <debug.h>
#include <kernel.h>
#include <iopcontrol.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <libcdvd.h>
#include <netman.h>
#include <ps2ip.h>
#include <netdb.h>
#include <sys/select.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include "v12_net_config.h"
#include "v12_roughtime_wire.h"
#ifdef V12_N3B_TXRYAN
#include "v12_roughtime_txryan_sig.h"
#define VERIFY_RTPACKET v12_roughtime_txryan_sig_verify
#else
#include "v12_roughtime_live_sig.h"
#define VERIFY_RTPACKET v12_roughtime_live_sig_verify
#endif

#define PART "hdd0:PP.DVRISO12"
#define MOUNT "pfs1:"
#define NET_CNF "pfs1:/V12_NET.CNF"
#ifdef V12_N3B_TXRYAN
/* Server explicitly selected at build time, never from network input. */
#define NAME "time.txryan.com"
#define PORT "2002"
#else
#define NAME "roughtime.cloudflare.com"
#define PORT "2003"
#endif

#define IRX(name) \
  extern unsigned char name##_irx[] __attribute__((aligned(16))); \
  extern uint32_t size_##name##_irx
IRX(iomanX);
IRX(fileXio);
IRX(dev9_ns);
IRX(ps2atad);
IRX(ps2hdd);
IRX(ps2fs);
IRX(netman);
IRX(smap);

static const char hdd_args[]="-o\0" "4\0" "-n\0" "20";
static const char pfs_args[]="-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char test_hdd_args[(sizeof(hdd_args)==11)?1:-1];
typedef char test_pfs_args[(sizeof(pfs_args)==17)?1:-1];

static uint8_t request_packet[V12_RT_REQUEST_BYTES];
static uint8_t response_packet[V12_RT_REPLY_MAX_BYTES];
static int mounted=0;

static int load_irx(const char *name,void *ptr,unsigned length,
                    const char *args,int argslen) {
  int rv=0;
  int id=SifExecModuleBuffer(ptr,length,argslen,(void *)args,&rv);
  scr_printf("IRX %-8s id=%d rv=%d\n",name,id,rv);
  return id<0?id:(rv<0?rv:0);
}
#define LOAD(name) do { \
  rc=load_irx(#name,name##_irx,size_##name##_irx,NULL,0); \
  if(rc<0) return -502; \
} while(0)

static int init_io(void) {
  int rc;
  u32 notice_arg=0;
  scr_printf("STAGE 1: reset IOP\n");
  while(!SifIopReset("",0)) {}
  while(!SifIopSync()) {}
  sceSifInitRpc(0);
  sbv_patch_enable_lmb();
  sbv_patch_disable_prefix_check();
  sbv_patch_fileio();
  scr_printf("STAGE 2: HDD / PFS IRX\n");
  LOAD(iomanX);
  LOAD(fileXio);
  LOAD(dev9_ns);
  LOAD(ps2atad);
  rc=load_irx("ps2hdd",ps2hdd_irx,size_ps2hdd_irx,
              hdd_args,sizeof(hdd_args));
  if(rc<0) return -502;
  rc=load_irx("ps2fs",ps2fs_irx,size_ps2fs_irx,
              pfs_args,sizeof(pfs_args));
  if(rc<0) return -502;
  rc=fileXioInit();
  scr_printf("fileXioInit=%d\n",rc);
  if(rc<0) return -502;
  rc=sceCdInit(SCECdINoD);
  if(rc<=0) return -502;
  rc=sceCdNoticeGameStart(0,&notice_arg);
  if(rc<0) return -502;
  usleep(500000);
  return 0;
}
#undef LOAD

static int read_cfg(v12_net_config *cfg) {
  char text[V12_NET_CONFIG_MAX+1U];
  int fd,total=0,n,closed;
  scr_printf("STAGE 3: PFS read-only config\n");
  fd=fileXioDopen("pfs1:/");
  if(fd>=0) {fileXioDclose(fd);return -503;}
  fd=fileXioMount(MOUNT,PART,FIO_MT_RDONLY);
  scr_printf("RO Mount=%d\n",fd);
  if(fd<0) return -503;
  mounted=1;
  fd=fileXioOpen(NET_CNF,FIO_O_RDONLY,0);
  if(fd<0) return -503;
  while(total<(int)sizeof(text)) {
    n=fileXioRead(fd,text+total,(int)sizeof(text)-total);
    if(n<=0) break;
    total+=n;
  }
  closed=fileXioClose(fd);
  if(n<0 || closed<0 || total==0 || total>(int)V12_NET_CONFIG_MAX ||
     v12_parse_net_config(text,(size_t)total,cfg)!=0) return -503;
  scr_printf("V12_NET.CNF parsed\n");
  fd=fileXioUmount(MOUNT);
  if(fd<0) return -503;
  mounted=0;
  scr_printf("RO unmount=%d\n",fd);
  return 0;
}

static int init_net(const v12_net_config *cfg) {
  struct ip4_addr ip,mask,gw;
  ip_addr_t dns;
  int rc,link=0;
  scr_printf("STAGE 4: NETMAN / SMAP\n");
  rc=load_irx("netman",netman_irx,size_netman_irx,NULL,0);
  if(rc<0) return -504;
  rc=load_irx("smap",smap_irx,size_smap_irx,NULL,0);
  if(rc<0) return -504;
  IP4_ADDR(&ip,cfg->ip[0],cfg->ip[1],cfg->ip[2],cfg->ip[3]);
  IP4_ADDR(&mask,cfg->netmask[0],cfg->netmask[1],cfg->netmask[2],cfg->netmask[3]);
  IP4_ADDR(&gw,cfg->gateway[0],cfg->gateway[1],cfg->gateway[2],cfg->gateway[3]);
  rc=ps2ipInit(&ip,&mask,&gw);
  scr_printf("ps2ipInit=%d\n",rc);
  if(rc<0) return -504;
  for(int i=0;i<6;i++) {
    link=NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS,NULL,0,NULL,0);
    if(link==NETMAN_NETIF_ETH_LINK_STATE_UP) break;
    sleep(1);
  }
  if(link!=NETMAN_NETIF_ETH_LINK_STATE_UP) return -504;
  IP4_ADDR(&dns,cfg->dns[0],cfg->dns[1],cfg->dns[2],cfg->dns[3]);
  dns_setserver(0,&dns);
  return 0;
}

static int live_signed_probe(void) {
  struct addrinfo hints,*resolved=NULL,*cur;
  uint8_t nonce[32];
  static uint32_t sequence;
  unsigned tags=0;
  uint32_t x;
  int rc,fd=-1,received,found=0,verdict=-506;
  scr_printf("STAGE 5: UDP/%s (weak test nonce)\n",PORT);
  ++sequence;
  x=(uint32_t)clock() ^ ((uint32_t)(uintptr_t)&sequence) ^
    (UINT32_C(0x9e3779b9)*sequence);
  if(!x) x=1;
  for(unsigned i=0;i<sizeof(nonce);i++) {
    x^=x<<13;x^=x>>17;x^=x<<5;
    nonce[i]=(uint8_t)(x>>((i&3U)*8U));
  }
  if(v12_rt_make_request(request_packet,sizeof(request_packet),nonce)!=0)
    return -506;
  memset(&hints,0,sizeof(hints));
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_DGRAM;
  hints.ai_protocol=IPPROTO_UDP;
  rc=getaddrinfo(NAME,PORT,&hints,&resolved);
  scr_printf("DNS rc=%d\n",rc);
  if(rc!=0 || !resolved) {
    if(resolved) freeaddrinfo(resolved);
    return -505;
  }
  for(cur=resolved;cur;cur=cur->ai_next) {
    fd_set reads;
    struct timeval tv;
    if(cur->ai_family!=AF_INET ||
       cur->ai_addrlen<(socklen_t)sizeof(struct sockaddr_in)) continue;
    fd=lwip_socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(fd<0) continue;
    if(lwip_connect(fd,cur->ai_addr,cur->ai_addrlen)<0) {
      lwip_close(fd);fd=-1;continue;
    }
    if(lwip_send(fd,request_packet,sizeof(request_packet),0)!=
       (int)sizeof(request_packet)) {
      lwip_close(fd);fd=-1;continue;
    }
    FD_ZERO(&reads);FD_SET(fd,&reads);
    tv.tv_sec=6;tv.tv_usec=0;
    rc=lwip_select(fd+1,&reads,NULL,NULL,&tv);
    if(rc<=0 || !FD_ISSET(fd,&reads)) {
      lwip_close(fd);fd=-1;continue;
    }
    received=lwip_recv(fd,response_packet,sizeof(response_packet),0);
    lwip_close(fd);fd=-1;
    scr_printf("RESPONSE=%d bytes\n",received);
    if(received<=0) continue;
    found=1;
    if(v12_rt_check_reply_frame(response_packet,
           (size_t)received,&tags)<0) {
      verdict=-507;
      break;
    }
    scr_printf("FRAME tags=%u\n",tags);
    scr_printf("STAGE 6: Ed25519 / SHA512\n");
    rc=VERIFY_RTPACKET(response_packet,
                                    (size_t)received,nonce);
    scr_printf("CRYPTO diagnostic code=%d\n",rc);
    verdict=rc<0?-508:rc; /* 0 strict, 1 missing NONC forensic */
    break;
  }
  if(fd>=0) lwip_close(fd);
  freeaddrinfo(resolved);
  if(!found && verdict==-506) scr_printf("No UDP response\n");
  return verdict;
}

int main(void) {
  int rc;
  v12_net_config cfg;
  init_scr();
  scr_setCursor(0);
  scr_printf("N3B MINI BOOT - STAGE 0\n");
  scr_printf("TARGET: %s UDP/%s\n",NAME,PORT);
  scr_printf("READ-ONLY: no HTTP, RTC, ART or game boot.\n");
  sleep(1);
  rc=init_io();
  if(rc==0) rc=read_cfg(&cfg);
  if(rc==0) rc=init_net(&cfg);
  if(rc==0) rc=live_signed_probe();
  if(mounted) {
    int unmount_rc=fileXioUmount(MOUNT);
    if(unmount_rc<0 && rc>=0) rc=-509;
  }
  scr_clear();
  scr_printf("N3B MINI - FINAL RESULT\n");
  scr_printf("SERVER: %s UDP/%s\n",NAME,PORT);
  if(rc==1) {
    scr_printf("NONC MISSING: FORENSIC CRYPTO PASS\n");
    scr_printf("INVALID IETF REPLY: NOT A TRUSTED CLOCK\n");
  } else if(rc==0) {
    scr_printf("STRICT CRYPTO: PASS (WEAK NONCE)\n");
  } else {
    scr_printf("ERROR: %d (see stage in source)\n",rc);
  }
  scr_printf("NO TLS / COVER / PFS WRITES / RTC\n");
  scr_printf("RET=%d   Holds screen 120 sec\n",rc);
  sleep(120);
  return rc<0?1:0;
}

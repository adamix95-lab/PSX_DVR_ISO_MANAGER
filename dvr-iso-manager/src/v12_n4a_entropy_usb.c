/*
 * DESR v1.2 N4A: read-only dual-domain entropy + signed UTC probe.
 * This is NOT the Manager: no cover/ISO browser, no RTC updates, no
 * writable mounts, no app/Game ID changes, no HTTP/TLS or filesystem writes.
 * N4A reads a fresh 96-byte host-CSPRNG bundle from USB.
 * 32 bytes for Roughtime, 64 bytes RESERVED for future wolfSSL RNG.
 * The TLS bytes are NEVER used as a TLS seed in this diagnostic.
 * Reuse after reboot remains possible: no trusted TLS clock or HTTPS.
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
#include "v12_n3e_txryan.h"
#include "v12_n3e_utc.h"
#include "v12_n4a_entropy.h"

#define PART "hdd0:PP.DVRISO12"
#define MOUNT "pfs1:"
#define NET_CNF "pfs1:/V12_NET.CNF"
#define NAME "time.txryan.com"
#define PORT "2002"

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
IRX(bdm);
IRX(bdmfs_fatfs);
IRX(usbd_mini);
IRX(usbmass_bd_mini);

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


/* Two disjoint host-generated CSPRNG ranges. Checking byte patterns
 * cannot prove entropy quality; that guarantee belongs to PC source.
 * Never persist a consumed marker or write to USB/HDD in N4A.
 */
static int read_entropy_usb(uint8_t nonce[32],uint8_t tls_seed[64]) {
  uint8_t data[96]={0};
  int rc,fd=-1,n,got=0,extra,closed;
  uint8_t tail=0;
  scr_printf("STAGE 4: N4A USB entropy (READ ONLY)\n");
  rc=load_irx("bdm",bdm_irx,size_bdm_irx,NULL,0);
  if(rc<0) return -510;
  rc=load_irx("bdmfs_fatfs",bdmfs_fatfs_irx,size_bdmfs_fatfs_irx,NULL,0);
  if(rc<0) return -510;
  rc=load_irx("usbd_mini",usbd_mini_irx,size_usbd_mini_irx,NULL,0);
  if(rc<0) return -510;
  rc=load_irx("usbmass_bd_mini",usbmass_bd_mini_irx,size_usbmass_bd_mini_irx,NULL,0);
  if(rc<0) return -510;
  for(int attempt=0;attempt<16;attempt++) {
    fd=fileXioOpen("mass0:/N4A.ENT",FIO_O_RDONLY,0);
    if(fd<0) fd=fileXioOpen("mass:/N4A.ENT",FIO_O_RDONLY,0);
    if(fd>=0) break;
    usleep(500000);
  }
  if(fd<0) return -511;
  while(got<(int)sizeof(data)) {
    n=fileXioRead(fd,data+got,(int)sizeof(data)-got);
    if(n<=0) break;
    got+=n;
  }
  extra=fileXioRead(fd,&tail,1);
  closed=fileXioClose(fd);
  if(closed<0) {v12_n4a_wipe(data,sizeof(data));return -514;}
  if(got!=(int)sizeof(data) || extra!=0) {
    v12_n4a_wipe(data,sizeof(data));
    return -512;
  }
  rc=v12_n4a_entropy_split(data,sizeof(data),nonce,tls_seed);
  v12_n4a_wipe(data,sizeof(data));
  if(rc!=0) return -516;
  scr_printf("Roughtime NONCE 32B: INPUT ACCEPTED\n");
  scr_printf("TLS seed 64B: PRESENT, NOT USED\n");
  return 0;
}

static int init_net(const v12_net_config *cfg) {
  struct ip4_addr ip,mask,gw;
  ip_addr_t dns;
  int rc,link=0;
  scr_printf("STAGE 5: NETMAN / SMAP\n");
  rc=load_irx("netman",netman_irx,size_netman_irx,NULL,0);
  if(rc<0) return -520;
  rc=load_irx("smap",smap_irx,size_smap_irx,NULL,0);
  if(rc<0) return -521;
  IP4_ADDR(&ip,cfg->ip[0],cfg->ip[1],cfg->ip[2],cfg->ip[3]);
  IP4_ADDR(&mask,cfg->netmask[0],cfg->netmask[1],cfg->netmask[2],cfg->netmask[3]);
  IP4_ADDR(&gw,cfg->gateway[0],cfg->gateway[1],cfg->gateway[2],cfg->gateway[3]);
  rc=ps2ipInit(&ip,&mask,&gw);
  scr_printf("ps2ipInit=%d\n",rc);
  if(rc<0) return -522;
  for(int i=0;i<6;i++) {
    link=NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS,NULL,0,NULL,0);
    if(link==NETMAN_NETIF_ETH_LINK_STATE_UP) break;
    sleep(1);
  }
  if(link!=NETMAN_NETIF_ETH_LINK_STATE_UP) return -523;
  IP4_ADDR(&dns,cfg->dns[0],cfg->dns[1],cfg->dns[2],cfg->dns[3]);
  dns_setserver(0,&dns);
  return 0;
}

static int live_signed_probe(const uint8_t nonce[32],
                             v12_rt_signed_time *signed_time) {
  struct addrinfo hints,*resolved=NULL,*cur;
  unsigned tags=0;
  int rc,fd=-1,received,found=0,verdict=-506;
  scr_printf("STAGE 6: UDP/%s (USB random nonce)\n",PORT);
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
    scr_printf("STAGE 7: Ed25519 / SHA512\n");
    rc=v12_n3e_txryan_strict(response_packet,(size_t)received,
                              nonce,signed_time);
    scr_printf("CRYPTO/time policy code=%d\n",rc);
    verdict=rc==-5?-515:(rc<0?-508:0); /* No forensic fallback */
    break;
  }
  if(fd>=0) lwip_close(fd);
  freeaddrinfo(resolved);
  if(!found && verdict==-506) scr_printf("No UDP response\n");
  return verdict;
}

int main(void) {
  int rc;
  uint8_t nonce[32]={0};
  uint8_t tls_seed[64]={0};
  v12_rt_signed_time signed_time={0,0};
  char utc_mid[20],utc_low[20],utc_high[20];
  v12_net_config cfg;
  init_scr();
  scr_setCursor(0);
  scr_printf("N4A ENTROPY BOOT - STAGE 0\n");
  scr_printf("TARGET: %s UDP/%s\n",NAME,PORT);
  scr_printf("READ-ONLY: no HTTP, RTC, ART or game boot.\n");
  sleep(1);
  rc=init_io();
  if(rc==0) rc=read_cfg(&cfg);
  if(rc==0) rc=read_entropy_usb(nonce,tls_seed);
  /* This ELF intentionally has no TLS RNG adapter. Wipe unused seed. */
  v12_n4a_wipe(tls_seed,sizeof(tls_seed));
  if(rc==0) rc=init_net(&cfg);
  if(rc==0) rc=live_signed_probe(nonce,&signed_time);
  if(rc==0 && v12_n3e_signed_interval(&signed_time,
                utc_mid,utc_low,utc_high)!=0) rc=-515;
  if(mounted) {
    int unmount_rc=fileXioUmount(MOUNT);
    if(unmount_rc<0 && rc>=0) rc=-509;
  }
  v12_n4a_wipe(nonce,sizeof(nonce));
  scr_clear();
  scr_printf("N4A RNG INPUT - FINAL RESULT\n");
  scr_printf("SERVER: %s UDP/%s\n",NAME,PORT);
  if(rc==0) {
    scr_printf("STRICT SIGNED UTC: PASS (N4A RO)\n");
    scr_printf("UTC MIDP: %s\n",utc_mid);
    scr_printf("RADI: %u seconds\n",(unsigned)signed_time.radius_seconds);
    scr_printf("UTC LOW : %s\n",utc_low);
    scr_printf("UTC HIGH: %s\n",utc_high);
  } else {
    scr_printf("ERROR: %d (see stage in source)\n",rc);
  }
  scr_printf("TLS SEED NOT CONNECTED TO wolfSSL\n");
  scr_printf("NO TRUSTED TLS TIME OR ENTROPY HOOK\n");
  scr_printf("NEVER REUSE N4A.ENT\n");
  scr_printf("NO TLS / COVER / PFS WRITES / RTC\n");
  scr_printf("RET=%d   Holds screen 120 sec\n",rc);
  sleep(120);
  return rc<0?1:0;
}

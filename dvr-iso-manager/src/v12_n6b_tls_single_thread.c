/*
 * N6A STAGE 8 CONTROL/DVR comparison based on N6 (READ-ONLY).
 * NO HTTPS GET and NO USB/ART/PFS writes; strict verified TLS only.
 *
 * DESR v1.2 N5A: first bounded HTTPS GET to RAM, from N4H FastMath TLS.
 * Verified Roughtime + separate TLS sessions; HTTP GET ONLY after both
 * strict TLS peer/hostname/date checks have passed. No file writes.
 * Same strict Roughtime, CA, TLS peer/hostname/time, separate DRBG seeds,
 * read-only mounts, and signature-depth diagnostics as N4E SIGTRACE.
 * This is NOT production HTTPS: the time window and reboot nonce reuse
 * policy are diagnostic approximations, not a certified TLS trust policy.
 * Identical strict CA, hostname, date and RNG policy to N4E.
 * This is NOT the Manager: no cover/ISO browser, no RTC updates, no
 * writable mounts, no app/Game ID changes, no HTTP/TLS or filesystem writes.
 * N4A reads a fresh 96-byte host-CSPRNG bundle from USB.
 * 32-byte Roughtime nonce, 2 x 64-byte unique TLS session seeds.
 * USB file reuse after reboot is NOT prevented. NOT production HTTPS.
 * CA+hostname+cert-time are checked by wolfSSL at both time bounds.
 * Fixed GET to public 39-byte GitHub probe, validated entirely in RAM.
 * No HDD/ART/USB/RTC writes, no redirects, no arbitrary downloads.
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
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#if !defined(USE_FAST_MATH) || defined(WOLFSSL_SP_MATH_ALL) || !defined(TFM_NO_ASM)
#error "N4H requires portable FastMath with NO SP MATH and NO ASM"
#endif
#include <wolfssl/ssl.h>
#include <wolfssl/wolfio.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include "v12_net_config.h"
#include "v12_roughtime_wire.h"
#include "v12_n3e_txryan.h"
#include "v12_n3e_utc.h"
#include "v12_n4a_entropy.h"
#include "v12_n4b_seed_gate.h"
#include "n5a_http_ram.h"
#include "n6_usb_art.h"
#include "cover_index.h"
#include "probe_png.h"
#include "n6_normalize_png.h"

#define PART "hdd0:PP.DVRISO12"
#define MOUNT "pfs1:"
#define NET_CNF "pfs1:/V12_NET.CNF"
#define NAME "time.txryan.com"
#define PORT "2002"
#define TLS_HOST "raw.githubusercontent.com"
#define TLS_PORT "443"
#define CA_CAP 350000U
/* The public, immutable-by-accident marker is on our isolated test branch.
 * There are NO credentials, queries, redirects or dynamic URL inputs.
 */
/* Pinned, public OPL Manager GameArt archive (immutable commit).
 * The only variable is a STRICTLY validated 11-character PS2 Game ID.
 * No redirect or arbitrary remote URLs are accepted. */
#define N6_ARCHIVE "Luden02/psx-ps2-opl-art-database"
#define N6_PIN "e14b9abce3cc42a06bb4ad2e4056e289c8872a07"
#define N6_FALLBACK_ARCHIVE "CosmicScale/psbbn-art-database"
#define N6_FALLBACK_PIN "41daedfd81562cd21a255582358fb60f12899a0b"
/* Only the small candidate array survives for the optional DVR scan. */
static char n6_targets[N6_BATCH_MAX][12];

#if !defined(WC_RNG_SEED_CB) || !defined(HAVE_HASHDRBG) || !defined(HAVE_SNI)
#error "N4E BLOCKED: require custom wolfSSL DRBG seed callback and SNI"
#endif
#if defined(WOLFSSL_GENSEED_FORTEST) || defined(WC_NO_HASHDRBG)
#error "N4E forbids test/weak entropy"
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
IRX(bdm);
IRX(bdmfs_fatfs);
IRX(usbd_mini);
IRX(usbmass_bd_mini);
IRX(dvrdrv);
IRX(dvrfile);

static const char hdd_args[]="-o\0" "4\0" "-n\0" "20";
static const char pfs_args[]="-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char test_hdd_args[(sizeof(hdd_args)==11)?1:-1];
typedef char test_pfs_args[(sizeof(pfs_args)==17)?1:-1];

static uint8_t request_packet[V12_RT_REQUEST_BYTES];
static uint8_t response_packet[V12_RT_REPLY_MAX_BYTES];
static int mounted=0;
static int n5b_last_link=-999,n5b_link_polls=0;
static unsigned char ca_bundle[CA_CAP+1U];
static size_t ca_bytes=0;
static v12_n4b_seed_gate tls_gate={0};
static time_t signed_cert_clock=0;
/* Final screen survives scr_clear(). No private entropy or packet bytes. */
static int diag_bound=0; /* 1 LOW, 2 HIGH */
static int diag_ssl_connect_ret=0,diag_ssl_error=0;
static int diag_tcp_connected=0,diag_dns_rc=0;
static int diag_io_rx=0,diag_io_tx=0,diag_io_fail=0;
static int diag_gate_state=0;
/* Informational certificate verification callback; NEVER override failure. */
static int diag_cert_calls=0,diag_cert_failed=0;
static int diag_cert_last_error=0,diag_cert_last_depth=-1;
static int diag_cert_first_error=0,diag_cert_first_depth=-1;
static int diag_cert_total=0;
static int diag_cert_last_preverify=0;
/* Strict finite buffer. Never save response to USB/HDD/ART. */
static uint8_t n5a_http_ram[N5A_HTTP_INPUT_CAP];
static n5a_http_result n5a_http_info={0};
static int n5a_http_parse_error=0,n5a_http_tls_error=0;
static unsigned n5a_http_wire_bytes=0,n5a_http_wrote=0;
static int n5a_get_pass=0;
/* N5H: low-intrusion trace. wolfSSL hooks NEVER call scr_printf().
 * A separate EE thread renders at most eight volatile RAM trace entries.
 * Watchdog heartbeat distinguishes a blocked write from a frozen display.
 * The observer never accesses TLS plaintext, secrets or socket buffers.
 */
static volatile int n5c_http_active=0;
static volatile unsigned n5d_trace_last=0,n5d_trace_count=0;
static volatile unsigned n5h_trace_ring[8]={0};
static volatile int n5c_phase=0;
static int n5c_write_calls=0,n5c_read_calls=0;
static volatile int n5h_watch_active=0,n5h_watch_done=1;
static volatile unsigned n5h_heartbeat=0;
static volatile int n5h_hs_phase=-1,n5h_hs_result=-999;
static volatile int n5h_hs_state=-999,n5h_hs_connect=-999,n5h_hs_error=-999;
static u8 n5h_observer_stack[8192] __attribute__((aligned(16)));
extern void *_gp;

/* Trace hook must be minimal to avoid printf/GS and TLS reentrancy. */
void n5d_trace(unsigned int step) {
  unsigned seq;
  if(!n5c_http_active)return;
  seq=n5d_trace_count;
  n5h_trace_ring[seq&7U]=step;
  n5d_trace_last=step;
  n5d_trace_count=seq+1U;
}
void n5f_handshake_state(int phase,int result,int handshakeState,
                         int connectState,int sslError) {
  if(!n5c_http_active)return;
  n5h_hs_result=result;
  n5h_hs_state=handshakeState;
  n5h_hs_connect=connectState;
  n5h_hs_error=sslError;
  n5h_hs_phase=phase;
}
/* N6A traces happen in RAM only. The EE observer renders independent of
 * wolfSSL's calling stack, preserving the physical N5H success property.
 * Trace marks 801-830 identify exact TLS steps; no secrets or data printed. */
static void n5h_observer_thread(void *arg) {
  (void)arg;
  while(n5h_watch_active) {
    unsigned count;
    usleep(700000);
    if(!n5h_watch_active)break;
    n5h_heartbeat++;
    count=n5d_trace_count;
    scr_clear();
    scr_setCursor(0);
#ifdef N6A_CONTROL_NO_DVR
    scr_printf("N6A CONTROL - NO DVR MODULES - READ ONLY\n");
#else
    scr_printf("N6A DVR + NETFIX - READ ONLY\n");
#endif
    scr_printf("STAGE 8/9: verified TLS handshake\n");
    scr_printf("HEARTBEAT=%u (observer alive)\n",n5h_heartbeat);
    scr_printf("BOUND=%s  LAST STEP=%u  EVENTS=%u\n",
               diag_bound==1?"LOW":"HIGH",n5d_trace_last,count);
    scr_printf("TCP=%d DNS=%d NETIO RX=%d TX=%d\n",
               diag_tcp_connected,diag_dns_rc,diag_io_rx,diag_io_tx);
    scr_printf("IO FAILURE=%d TLS err=%d\n",
               diag_io_fail,diag_ssl_error);
    scr_printf("VERIFY calls=%d fail=%d\n",diag_cert_calls,diag_cert_failed);
    for(unsigned i=0;i<8U && i<count;i++)
      scr_printf("TRACE[%u]=%u\n",i,n5h_trace_ring[(count-1U-i)&7U]);
    scr_printf("No HDD/USB/ART/RTC writes; no GET\n");
  }
  n5h_watch_done=1;
  ExitThread();
}
static int n5h_start_observer(void) {
  ee_thread_t th;
  ee_thread_status_t current;
  int tid,ret,priority=0x30;
  memset(&th,0,sizeof(th));
  memset(&current,0,sizeof(current));
  ret=ReferThreadStatus(GetThreadId(),&current);
  if(ret>=0 && current.current_priority>2 &&
     current.current_priority<MAX_PRIORITY)
    priority=current.current_priority-1;
  th.func=n5h_observer_thread;
  th.stack=n5h_observer_stack;
  th.stack_size=sizeof(n5h_observer_stack);
  th.gp_reg=&_gp;
  th.initial_priority=priority;
  th.attr=0;
  th.option=0;
  tid=CreateThread(&th);
  if(tid<0)return tid;
  n5h_watch_done=0;
  n5h_watch_active=1;
  ret=StartThread(tid,NULL);
  if(ret<0) {
    n5h_watch_active=0;
    n5h_watch_done=1;
    DeleteThread(tid);
    return ret;
  }
  return tid;
}
static void n5h_stop_observer(int tid) {
  if(tid<0)return;
  n5h_watch_active=0;
  for(int i=0;i<250 && !n5h_watch_done;i++)
    usleep(10000);
  if(n5h_watch_done)
    DeleteThread(tid);
  else
    scr_printf("N5H warning: observer exit not confirmed\n");
}

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
  /* Difference isolated from CONTROL: DVR IRX + game metadata scan.
   * Like stable 1.0, the same dev9_ns is shared by HDD and Ethernet. */
#ifndef N6A_CONTROL_NO_DVR
  LOAD(dvrdrv);
  LOAD(dvrfile);
#endif
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


/* N4E one-shot private input: 32 RT nonce + 2 x 64 TLS seeds.
 * Host CSPRNG, raw bytes; no deterministic fallback or secret logging.
 */
/* One-time host CSPRNG input: 32-byte Roughtime nonce + sixteen DISTINCT
 * 64-byte seeds (LOW/HIGH per each of four possible TLS downloads).
 * Never cache, print, upload or regenerate on the console. */
static int read_entropy_usb(uint8_t nonce[32],
                            uint8_t seeds[N6_BATCH_MAX][4][64]) {
  uint8_t data[N6_ENT_BYTES]={0};
  int rc,fd=-1,n,got=0,extra,closed;
  uint8_t tail=0;
  scr_printf("N6 USB N6.ENT read-only: %u bytes\n",(unsigned)sizeof(data));
  rc=load_irx("bdm",bdm_irx,size_bdm_irx,NULL,0);
  if(rc<0)return -510;
  rc=load_irx("bdmfs_fatfs",bdmfs_fatfs_irx,size_bdmfs_fatfs_irx,NULL,0);
  if(rc<0)return -510;
  rc=load_irx("usbd_mini",usbd_mini_irx,size_usbd_mini_irx,NULL,0);
  if(rc<0)return -510;
  rc=load_irx("usbmass_bd_mini",usbmass_bd_mini_irx,size_usbmass_bd_mini_irx,NULL,0);
  if(rc<0)return -510;
  for(int i=0;i<16;i++) {
    fd=fileXioOpen("mass0:/N6.ENT",FIO_O_RDONLY,0);
    if(fd>=0)break;
    usleep(500000);
  }
  if(fd<0)return -511;
  while(got<(int)sizeof(data)) {
    n=fileXioRead(fd,data+got,(int)sizeof(data)-got);
    if(n<=0)break;
    got+=n;
  }
  extra=fileXioRead(fd,&tail,1);
  closed=fileXioClose(fd);
  if(closed<0||extra!=0||got!=(int)sizeof(data)) {
    v12_n4a_wipe(data,sizeof(data));return -512;
  }
  { uint8_t bits=0;
    for(int i=0;i<32;i++)bits|=data[i];
    if(!bits){v12_n4a_wipe(data,sizeof(data));return -516;}
    for(int i=0;i<N6_BATCH_MAX*4;i++) {
      bits=0;
      for(int j=0;j<64;j++)bits|=data[32+i*64+j];
      if(!bits){v12_n4a_wipe(data,sizeof(data));return -516;}
      for(int k=0;k<i;k++)
        if(memcmp(data+32+i*64,data+32+k*64,64)==0) {
          v12_n4a_wipe(data,sizeof(data));return -516;
        }
    }
  }
  memcpy(nonce,data,32);
  memcpy(seeds,data+32,sizeof(data)-32);
  v12_n4a_wipe(data,sizeof(data));
  scr_printf("N6: fresh nonce + 16 distinct TLS seeds loaded\n");
  return 0;
}

static int read_ca_bundle(void) {
  int fd=-1,n,closed,extra;
  uint8_t tail;
  size_t total=0;
  scr_printf("STAGE 4B: USB public N4E.CA (PEM)\n");
  fd=fileXioOpen("mass0:/N4E.CA",FIO_O_RDONLY,0);
  if(fd<0) fd=fileXioOpen("mass:/N4E.CA",FIO_O_RDONLY,0);
  if(fd<0) return -517;
  while(total<CA_CAP) {
    size_t count=CA_CAP-total;
    if(count>4096U) count=4096U;
    n=fileXioRead(fd,ca_bundle+total,(int)count);
    if(n<0){fileXioClose(fd);return -517;}
    if(n==0)break;
    total+=(size_t)n;
  }
  extra=fileXioRead(fd,&tail,1);
  closed=fileXioClose(fd);
  if(closed<0 || extra!=0 || total<300U || total>=CA_CAP) return -517;
  ca_bundle[total]=0;
  if(!strstr((char *)ca_bundle,"-----BEGIN CERTIFICATE-----"))
    return -517;
  ca_bytes=total;
  scr_printf("CA PEM loaded: %u bytes (public)\n",(unsigned)ca_bytes);
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
  scr_printf("N5B NETLINK: STATIC IP, DHCP OFF\n");
  scr_printf("IP %u.%u.%u.%u GW %u.%u.%u.%u\n",
    cfg->ip[0],cfg->ip[1],cfg->ip[2],cfg->ip[3],
    cfg->gateway[0],cfg->gateway[1],cfg->gateway[2],cfg->gateway[3]);
  scr_printf("MASK %u.%u.%u.%u DNS %u.%u.%u.%u\n",
    cfg->netmask[0],cfg->netmask[1],cfg->netmask[2],cfg->netmask[3],
    cfg->dns[0],cfg->dns[1],cfg->dns[2],cfg->dns[3]);
  for(int i=0;i<20;i++) {
    link=NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS,NULL,0,NULL,0);
    n5b_last_link=link;
    n5b_link_polls=i+1;
    if(i==0 || i%4==3 || link==NETMAN_NETIF_ETH_LINK_STATE_UP)
      scr_printf("N5B LINK poll=%d raw=%d UP=%d\n",
        i+1,link,NETMAN_NETIF_ETH_LINK_STATE_UP);
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


/* WOLFSSL_USER_IO uses lwIP callbacks; each receive has a finite timeout.
 * Connect itself is bounded by normal lwIP stack behavior and is not
 * a production-grade nonblocking connection timeout policy.
 */
/* N5C: trace calls without ever printing TLS buffers or secrets.
 * select() has a finite timeout; a blocking lwip_send/recv may still
 * stall inside the syscall. These ENTER markers distinguish that case.
 */
static int tls_recv(WOLFSSL *ssl,char *buffer,int sz,void *ctx) {
  int fd=ctx?*(int *)ctx:-1,rc;
  fd_set rfds;
  struct timeval tv;
  (void)ssl;
  if(fd<0 || !buffer || sz<=0)return WOLFSSL_CBIO_ERR_GENERAL;
  if(n5c_http_active && !n5h_watch_active) scr_printf("N5C RX SELECT enter cap=%d\n",sz);
  FD_ZERO(&rfds); FD_SET(fd,&rfds);
  tv.tv_sec=5;tv.tv_usec=0;
  rc=lwip_select(fd+1,&rfds,NULL,NULL,&tv);
  if(n5c_http_active && !n5h_watch_active) scr_printf("N5C RX SELECT ret=%d\n",rc);
  if(rc<=0 || !FD_ISSET(fd,&rfds)) {
    diag_io_fail=-701;return WOLFSSL_CBIO_ERR_GENERAL;
  }
  if(n5c_http_active && !n5h_watch_active) scr_printf("N5C RX RECV enter\n");
  rc=lwip_recv(fd,buffer,sz,0);
  if(n5c_http_active && !n5h_watch_active) scr_printf("N5C RX RECV ret=%d\n",rc);
  if(rc==0) {diag_io_fail=-702;return WOLFSSL_CBIO_ERR_CONN_CLOSE;}
  if(rc<0) {diag_io_fail=-703;return WOLFSSL_CBIO_ERR_GENERAL;}
  diag_io_rx+=1;
  return rc;
}
static int tls_send(WOLFSSL *ssl,char *buffer,int sz,void *ctx) {
  int fd=ctx?*(int *)ctx:-1,rc;
  fd_set wfds;
  struct timeval tv;
  (void)ssl;
  if(fd<0 || !buffer || sz<=0)return WOLFSSL_CBIO_ERR_GENERAL;
  if(n5c_http_active && !n5h_watch_active) scr_printf("N5C TX SELECT enter bytes=%d\n",sz);
  FD_ZERO(&wfds); FD_SET(fd,&wfds);
  tv.tv_sec=5;tv.tv_usec=0;
  rc=lwip_select(fd+1,NULL,&wfds,NULL,&tv);
  if(n5c_http_active && !n5h_watch_active) scr_printf("N5C TX SELECT ret=%d\n",rc);
  if(rc<=0 || !FD_ISSET(fd,&wfds)) {
    diag_io_fail=-704;return WOLFSSL_CBIO_ERR_GENERAL;
  }
  if(n5c_http_active && !n5h_watch_active) scr_printf("N5C TX SEND enter\n");
  rc=lwip_send(fd,buffer,sz,0);
  if(n5c_http_active && !n5h_watch_active) scr_printf("N5C TX SEND ret=%d\n",rc);
  if(rc<=0) {diag_io_fail=-705;return WOLFSSL_CBIO_ERR_GENERAL;}
  diag_io_tx+=1;
  return rc;
}
static int seed_cb(OS_Seed *os,byte *dst,word32 count) {
  (void)os;
  return v12_n4b_seed_take(&tls_gate,dst,(size_t)count);
}
static time_t signed_time_cb(time_t *t) {
  if(t) *t=signed_cert_clock;
  return signed_cert_clock;
}
static int connect_tls_endpoint(void) {
  struct addrinfo hints,*resolved=NULL,*cur;
  int fd=-1,rc;
  memset(&hints,0,sizeof(hints));
  hints.ai_family=AF_INET;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_protocol=IPPROTO_TCP;
  rc=getaddrinfo(TLS_HOST,TLS_PORT,&hints,&resolved);
  diag_dns_rc=rc;
  if(rc!=0 || !resolved) {
    if(resolved)freeaddrinfo(resolved);
    return -1;
  }
  for(cur=resolved;cur;cur=cur->ai_next) {
    if(cur->ai_family!=AF_INET)continue;
    fd=lwip_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(fd<0)continue;
    if(lwip_connect(fd,cur->ai_addr,cur->ai_addrlen)==0) {
      diag_tcp_connected=1;
      break;
    }
    lwip_close(fd);fd=-1;
  }
  freeaddrinfo(resolved);
  return fd;
}
/* HTTPS GET happens ONLY inside the high-bound TLS session, AFTER the
 * full wolfSSL peer certificate chain, hostname and clock are verified.
 * Finite 16KiB RAM response; accepts only status 200 and exact 39B body.
 * HTTP/1.1 redirects, wrong content, overflow and read failures hard-fail.
 */
/* Only validated, pinned archive paths. Strict TLS check done before GET.
 * All HTTP bytes stay in RAM until complete PNG CRC+framing checks pass.
 * The source file must be image/png with Content-Length (no chunked/redirects).
 */
/* N6B: intentionally NO HTTP GET body or ART network functionality. */
static int diag_verify_cb(int preverify,WOLFSSL_X509_STORE_CTX *store) {
  int code,depth;
  diag_cert_calls+=1;
  if(!store) {
    diag_cert_failed+=1;
    diag_cert_last_error=-1;
    return 0; /* missing verification context is a hard failure */
  }
  code=store->error;
  depth=store->error_depth;
  diag_cert_last_preverify=preverify;
  diag_cert_last_error=code;
  diag_cert_last_depth=depth;
  diag_cert_total=store->totalCerts;
  if(preverify!=1) {
    diag_cert_failed+=1;
    if(diag_cert_failed==1) {
      diag_cert_first_error=code;
      diag_cert_first_depth=depth;
    }
  }
  return preverify; /* CA, hostname and date verification NOT bypassed */
}

static int tls_one_bound(const uint8_t seed[64],time_t bound,
                         const char *label,int do_http_get) {
  WOLFSSL_CTX *ctx=NULL;
  WOLFSSL *ssl=NULL;
  int fd=-1,rc,ret=-645;
  (void)do_http_get;
  (void)label;
  /* Observer is started BEFORE all crypto/TCP operations, not only
   * before wolfSSL_write (which was too late for the N6 STAGE 8 hang). */
  /* N6B: no EE observer during TLS handshake. Unlike N6A,
   * this follows the already proven N5H threading model. */
#ifdef N6B_TRACE_ONLY
  n5c_http_active=1; /* RAM-only callbacks enabled; no console I/O inside. */
  scr_printf("N6B TRACE-ONLY / NO OBSERVER\n");
#else
  n5c_http_active=0; /* N5H-like handshake; diagnostics hooks inert. */
  scr_printf("N6B PLAIN TLS / NO OBSERVER\n");
#endif
  scr_printf("N6B STEP=801 START\n"); /* observer running */
  scr_printf("N6B STEP=802 SEED ENTER\n"); /* before fresh strict seed provision */
  if(v12_n4b_seed_provision(&tls_gate,seed,64)!=0){ret=-641;goto cleanup;}
  scr_printf("N6B STEP=803 SEED OK\n");
  signed_cert_clock=bound;
  scr_printf("N6B STEP=804 TIME ENTER\n"); /* set signed time */
  if(wc_SetTimeCb(signed_time_cb)!=0){ret=-642;goto cleanup;}
  scr_printf("N6B STEP=805 TIME OK\n");
  scr_printf("N6B STEP=806 CTX ENTER\n"); /* wolfSSL_CTX_new */
  ctx=wolfSSL_CTX_new(wolfTLSv1_2_client_method());
  if(!ctx){ret=-643;goto cleanup;}
  scr_printf("N6B STEP=807 CTX OK\n");
  wolfSSL_CTX_set_verify(ctx,WOLFSSL_VERIFY_PEER,diag_verify_cb);
  scr_printf("N6B STEP=808 CA ENTER\n"); /* CA import */
  if(wolfSSL_CTX_load_verify_buffer(ctx,ca_bundle,(long)ca_bytes,
          WOLFSSL_FILETYPE_PEM)!=WOLFSSL_SUCCESS){ret=-644;goto cleanup;}
  scr_printf("N6B STEP=809 CA OK\n");
  wolfSSL_CTX_SetIORecv(ctx,tls_recv);
  wolfSSL_CTX_SetIOSend(ctx,tls_send);
  scr_printf("N6B STEP=810 SSL ENTER\n"); /* wolfSSL_new */
  ssl=wolfSSL_new(ctx);
  if(!ssl){ret=-645;goto cleanup;}
  scr_printf("N6B STEP=811 SSL OK\n");
  scr_printf("N6B STEP=812 SNI ENTER\n"); /* SNI */
  if(wolfSSL_UseSNI(ssl,WOLFSSL_SNI_HOST_NAME,TLS_HOST,
          (unsigned short)strlen(TLS_HOST))!=WOLFSSL_SUCCESS){
    ret=-646;goto cleanup;
  }
  scr_printf("N6B STEP=813 SNI OK\n");
  scr_printf("N6B STEP=814 HOST ENTER\n"); /* hostname check setup */
  if(wolfSSL_check_domain_name(ssl,TLS_HOST)!=WOLFSSL_SUCCESS){
    ret=-647;goto cleanup;
  }
  scr_printf("N6B STEP=815 HOST OK\n");
  scr_printf("N6B STEP=816 TCP ENTER\n"); /* DNS + TCP connect */
  fd=connect_tls_endpoint();
  if(fd<0){ret=-648;goto cleanup;}
  scr_printf("N6B STEP=817 TCP OK\n");
  wolfSSL_SetIOReadCtx(ssl,&fd);
  wolfSSL_SetIOWriteCtx(ssl,&fd);
  scr_printf("N6B STEP=818 TLS CONNECT ENTER\n"); /* BEFORE wolfSSL_connect */
  rc=wolfSSL_connect(ssl);
  scr_printf("N6B STEP=819 TLS CONNECT RETURN\n"); /* AFTER wolfSSL_connect */
  diag_ssl_connect_ret=rc;
  diag_gate_state=(int)tls_gate.state;
  if(rc!=WOLFSSL_SUCCESS){
    diag_ssl_error=wolfSSL_get_error(ssl,rc);
    ret=-649;goto cleanup;
  }
  if(tls_gate.state!=2U){ret=-650;goto cleanup;}
  scr_printf("N6B STEP=820 TLS VERIFY PASS\n"); /* verified TLS complete */
  ret=0;
cleanup:
  scr_printf("N6B STEP=821 FREE SSL\n"); /* ssl free */
  if(ssl)wolfSSL_free(ssl);
  scr_printf("N6B STEP=822 SSL FREE\n");
  if(fd>=0)lwip_close(fd);
  scr_printf("N6B STEP=823 CLOSE TCP\n");
  if(ctx)wolfSSL_CTX_free(ctx);
  scr_printf("N6B STEP=824 CTX FREE\n");
  v12_n4b_seed_clear(&tls_gate);
  wc_SetTimeCb(NULL);
  signed_cert_clock=0;
  scr_printf("N6B STEP=825 CLEANUP\n"); /* cleanup done */
  /* no observer started in either N6B variant */
  n5c_http_active=0;
  return ret;
}
static int verify_two_bound_tls(const v12_rt_signed_time *signed_time,
                                const uint8_t first_seed[64],
                                const uint8_t second_seed[64]) {
  time_t lower,upper;
  int rc;
  if(!signed_time || signed_time->radius_seconds==0 ||
     signed_time->radius_seconds>15 ||
     signed_time->midpoint_utc<UINT64_C(1735689600) ||
     signed_time->midpoint_utc>UINT64_C(2140000000))
    return -640;
  /* Padding on both ends is conservative for this *one-run* test.
   * NOT a measured timing proof, not safe for production.
   * A 32-bit time_t supports this test until early 2038.
   */
  lower=(time_t)(signed_time->midpoint_utc-
       signed_time->radius_seconds-120U);
  upper=(time_t)(signed_time->midpoint_utc+
       signed_time->radius_seconds+120U);
  scr_printf("TLS peer: %s:443\n",TLS_HOST);
  scr_printf("SSL cert time checked at 2 UTC bounds\n");
  if(wolfSSL_Init()!=WOLFSSL_SUCCESS)return -651;
  if(wc_SetSeed_Cb(seed_cb)!=0) {
    wolfSSL_Cleanup();return -652;
  }
  scr_printf("STAGE 8: TLS lower bound\n");
  diag_bound=1;
  rc=tls_one_bound(first_seed,lower,"LOW",0);
  if(rc==0) {
    scr_printf("STAGE 9: TLS upper bound\n");
    diag_bound=2;
    diag_tcp_connected=0;
    diag_io_rx=0;diag_io_tx=0;diag_io_fail=0;
    diag_ssl_connect_ret=0;diag_ssl_error=0;diag_gate_state=0;
    diag_cert_calls=0;diag_cert_failed=0;
    diag_cert_last_error=0;diag_cert_last_depth=-1;
    diag_cert_first_error=0;diag_cert_first_depth=-1;
    diag_cert_total=0;diag_cert_last_preverify=0;
    rc=tls_one_bound(second_seed,upper,"HIGH",0);
  }
  wc_SetSeed_Cb(NULL);
  wc_SetTimeCb(NULL);
  wolfSSL_Cleanup();
  return rc;
}

int main(void) {
  int rc,rt_pass=0,targets=0,iso_count=0,ids_count=0,missing=0;
  uint8_t nonce[32]={0},seeds[N6_BATCH_MAX][4][64]={0};
  v12_rt_signed_time signed_time={0,0};
  v12_net_config cfg;
  init_scr();scr_setCursor(0);
#ifdef N6A_CONTROL_NO_DVR
  scr_printf("N6B CONTROL - NO DVR IRX / NO OBSERVER\n");
#else
  scr_printf("N6A DVR + NETFIX 1.0 COMPATIBILITY\n");
#endif
  scr_printf("READ-ONLY TLS TEST; NO ART/USB/HDD WRITES\n");
  sleep(1);
  rc=init_io();
  if(rc==0)rc=read_cfg(&cfg);
  if(rc==0)rc=read_entropy_usb(nonce,seeds);
  if(rc==0)rc=read_ca_bundle();
#ifndef N6A_CONTROL_NO_DVR
  if(rc==0) {
    scr_printf("N6A DVR ISO READ-ONLY SCAN...\n");
    targets=n6_scan_dvr(n6_targets,&iso_count,&ids_count,&missing);
    if(targets<0)rc=targets;
    else scr_printf("DVR ISO=%d ID=%d MISSING=%d\n",iso_count,ids_count,missing);
  }
#endif
  if(rc==0)rc=init_net(&cfg);
  if(rc==0)rc=live_signed_probe(nonce,&signed_time);
  if(rc==0) {
    rt_pass=1;
    scr_printf("ROUGHTIME SIGNED UTC: PASS\n");
  }
  if(rc==0) {
    scr_printf("N6B START TLS LOW (no observer)\n");
    rc=verify_two_bound_tls(&signed_time,seeds[0][0],seeds[0][1]);
  }
  v12_n4a_wipe(nonce,sizeof(nonce));
  v12_n4a_wipe(seeds,sizeof(seeds));
  if(mounted) {
    int u=fileXioUmount(MOUNT);
    if(u<0 && rc==0)rc=-509;
  }
  scr_clear();scr_setCursor(0);
#ifdef N6A_CONTROL_NO_DVR
  scr_printf("N6B CONTROL - FINAL RESULT\n");
#else
  scr_printf("N6B DVR / NETFIX - FINAL RESULT\n");
#endif
  scr_printf("ISO=%d ID=%d MISS=%d\n",iso_count,ids_count,missing);
  if(rt_pass)scr_printf("SIGNED UTC: PASS\n");
  if(rc==0)scr_printf("TLS LOW/HIGH: VERIFIED PASS\n");
  scr_printf("LAST STEP=%u COUNT=%u HEARTBEAT=%u\n",
             n5d_trace_last,n5d_trace_count,n5h_heartbeat);
  scr_printf("IO RX=%d TX=%d FAIL=%d DNS=%d TCP=%d\n",
             diag_io_rx,diag_io_tx,diag_io_fail,diag_dns_rc,diag_tcp_connected);
  scr_printf("TLS SSL CONNECT=%d ERROR=%d\n",diag_ssl_connect_ret,diag_ssl_error);
  scr_printf("CERT calls=%d failures=%d\n",diag_cert_calls,diag_cert_failed);
  scr_printf("TLS ONLY; NO HTTP GET / NO ART WRITES\n");
  scr_printf("HDD READONLY; USB READONLY; NO RTC WRITES\n");
  scr_printf("RET=%d (screen 120sec)\n",rc);
  sleep(120);
  return rc==0?0:1;
}

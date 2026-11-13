/*
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
static const char n5a_http_get_request[] =
  "GET /adamix95-lab/PSX_DVR_ISO_MANAGER/test/n5a-direct-download/diagnostics/n5a/PROBE.TXT HTTP/1.1\r\n"
  "Host: raw.githubusercontent.com\r\n"
  "User-Agent: PSX-DVR-ISO-MANAGER-N5A/1.2\r\n"
  "Accept: text/plain\r\n"
  "Accept-Encoding: identity\r\n"
  "Cache-Control: no-cache\r\n"
  "Connection: close\r\n\r\n";

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
    scr_printf("N5H ASYNC OBSERVER - READ ONLY\n");
    scr_printf("STAGE 10: HTTPS wolfSSL_write\n");
    scr_printf("HEARTBEAT=%u (alive every ~0.7s)\n",n5h_heartbeat);
    scr_printf("PHASE=%d TRACE COUNT=%u\n",n5c_phase,count);
    scr_printf("LAST step=%u\n",n5d_trace_last);
    scr_printf("HS phase=%d result=%d state=%d\n",
               n5h_hs_phase,n5h_hs_result,n5h_hs_state);
    scr_printf("HS connect=%d error=%d\n",n5h_hs_connect,n5h_hs_error);
    scr_printf("NET IO rx=%d tx=%d fail=%d\n",diag_io_rx,diag_io_tx,diag_io_fail);
    for(unsigned i=0;i<8U && i<count;i++) {
      unsigned step=n5h_trace_ring[(count-1U-i)&7U];
      scr_printf("RING[%u]=%u\n",i,step);
    }
    scr_printf("No entropy, certs or TLS payload logged\n");
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
static int read_entropy_usb(uint8_t nonce[32],uint8_t tls_lo[64],
                            uint8_t tls_hi[64]) {
  uint8_t data[160]={0};
  int rc,fd=-1,n,got=0,extra,closed;
  uint8_t tail=0,orv=0;
  scr_printf("STAGE 4: USB N4E.ENT (READ ONLY)\n");
  rc=load_irx("bdm",bdm_irx,size_bdm_irx,NULL,0);
  if(rc<0) return -510;
  rc=load_irx("bdmfs_fatfs",bdmfs_fatfs_irx,size_bdmfs_fatfs_irx,NULL,0);
  if(rc<0) return -510;
  rc=load_irx("usbd_mini",usbd_mini_irx,size_usbd_mini_irx,NULL,0);
  if(rc<0) return -510;
  rc=load_irx("usbmass_bd_mini",usbmass_bd_mini_irx,size_usbmass_bd_mini_irx,NULL,0);
  if(rc<0) return -510;
  for(int attempt=0;attempt<16;attempt++) {
    fd=fileXioOpen("mass0:/N4E.ENT",FIO_O_RDONLY,0);
    if(fd<0) fd=fileXioOpen("mass:/N4E.ENT",FIO_O_RDONLY,0);
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
  if(closed<0 || extra<0 || got!=(int)sizeof(data) || extra!=0) {
    v12_n4a_wipe(data,sizeof(data));
    return -512;
  }
  for(int i=0;i<32;i++) orv|=data[i];
  if(!orv || !memcmp(data+32,data+96,64)) {
    v12_n4a_wipe(data,sizeof(data));return -516;
  }
  orv=0;
  for(int i=32;i<160;i++) orv|=data[i];
  if(!orv || !memcmp(data,data+32,32)) {
    v12_n4a_wipe(data,sizeof(data));return -516;
  }
  memcpy(nonce,data,32);
  memcpy(tls_lo,data+32,64);
  memcpy(tls_hi,data+96,64);
  v12_n4a_wipe(data,sizeof(data));
  scr_printf("Roughtime NONCE + 2 TLS seeds loaded\n");
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
static int n5a_get_fixed_probe_ram(WOLFSSL *ssl) {
  size_t sent=0,total=0;
  int rc=0,read_count=0,recv_closed=0;
  n5a_http_result parsed={0};
  if(!ssl)return -710;
  /* Clear old STAGE 1-9 lines so GS debug-console wrapping cannot
   * masquerade as a new event or hide the last STAGE 10 marker. */
  scr_clear();
  scr_setCursor(0);
  n5c_http_active=1;
  scr_printf("N5G: Roughtime + 2 TLS bounds PASS\n");
  scr_printf("STAGE 10: N5H ASYNC RAM TRACE\n");
  {
    const char *cipher=wolfSSL_get_cipher(ssl);
    scr_printf("N5D CIPHER=%s\n",cipher?cipher:"unknown");
  }
  while(sent<sizeof(n5a_http_get_request)-1U) {
    n5c_phase=1;
    ++n5c_write_calls;
    if(n5c_write_calls>16)return -716;
    scr_printf("N5C WRITE enter #%d left=%u\n",
      n5c_write_calls,(unsigned)(sizeof(n5a_http_get_request)-1U-sent));
    {
      int watchid=n5h_start_observer();
      if(watchid<0) {
        scr_printf("N5H observer start failed=%d\n",watchid);
        return -717;
      }
      rc=wolfSSL_write(ssl,n5a_http_get_request+sent,
                       (int)(sizeof(n5a_http_get_request)-1U-sent));
      n5h_stop_observer(watchid);
    }
    scr_clear();
    scr_setCursor(0);
    scr_printf("N5H OBSERVER STOPPED, WRITE returned\n");
    scr_printf("LAST step=%u count=%u beats=%u\n",
                n5d_trace_last,n5d_trace_count,n5h_heartbeat);
    scr_printf("N5C WRITE return=%d\n",rc);
    if(rc<=0) {
      n5a_http_tls_error=wolfSSL_get_error(ssl,rc);
      scr_printf("N5C WRITE wolfERR=%d\n",n5a_http_tls_error);
      return -710;
    }
    sent+=(size_t)rc;
    n5a_http_wrote=(unsigned)sent;
  }
  scr_printf("N5C WRITE COMPLETE sent=%u\n",n5a_http_wrote);
  for(read_count=0;read_count<40;read_count++) {
    size_t remaining=N5A_HTTP_INPUT_CAP-total;
    int got;
    if(remaining==0U)return -713;
    if(remaining>1024U)remaining=1024U;
    n5c_phase=2;
    ++n5c_read_calls;
    scr_printf("N5C READ enter #%d cap=%u\n",n5c_read_calls,(unsigned)remaining);
    got=wolfSSL_read(ssl,n5a_http_ram+total,(int)remaining);
    scr_printf("N5C READ return=%d\n",got);
    if(got>0) {
      total+=(size_t)got;
      n5a_http_wire_bytes=(unsigned)total;
      scr_printf("N5C HTTP bytes=%u\n",n5a_http_wire_bytes);
      rc=n5a_http_check(n5a_http_ram,total,0,&parsed);
    } else {
      n5a_http_tls_error=wolfSSL_get_error(ssl,got);
      scr_printf("N5C READ wolfERR=%d\n",n5a_http_tls_error);
      if(got==0 || n5a_http_tls_error==WOLFSSL_ERROR_ZERO_RETURN) {
        recv_closed=1;
        rc=n5a_http_check(n5a_http_ram,total,1,&parsed);
      } else return -711;
    }
    if(rc==1) {
      n5a_http_info=parsed;
      n5a_get_pass=1;
      scr_printf("HTTPS STATUS: %d (VERIFIED TLS)\n",parsed.status);
      scr_printf("BODY: %u bytes MATCH (RAM)\n",parsed.body_bytes);
      n5c_http_active=0;
      return 0;
    }
    if(rc<0){n5a_http_parse_error=rc;return -712;}
    if(recv_closed)return -714;
  }
  return -715;
}

/* One distinct CSPRNG input for EACH TLS session.
 * Returns zero only after chain+hostname+date check in wolfSSL_connect.
 * No HTTP payload is sent even after success.
 */
/* Called by wolfSSL while verifying each certificate. Read-only trace.
 * Do not call through this callback for an acceptance override:
 * returning preverify unchanged is the security boundary.
 */
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
  if(v12_n4b_seed_provision(&tls_gate,seed,64)!=0)return -641;
  signed_cert_clock=bound;
  if(wc_SetTimeCb(signed_time_cb)!=0){ret=-642;goto cleanup;}
  ctx=wolfSSL_CTX_new(wolfTLSv1_2_client_method());
  if(!ctx){ret=-643;goto cleanup;}
  wolfSSL_CTX_set_verify(ctx,WOLFSSL_VERIFY_PEER,diag_verify_cb);
  if(wolfSSL_CTX_load_verify_buffer(ctx,ca_bundle,(long)ca_bytes,
          WOLFSSL_FILETYPE_PEM)!=WOLFSSL_SUCCESS){ret=-644;goto cleanup;}
  wolfSSL_CTX_SetIORecv(ctx,tls_recv);
  wolfSSL_CTX_SetIOSend(ctx,tls_send);
  ssl=wolfSSL_new(ctx);
  if(!ssl){ret=-645;goto cleanup;}
  if(wolfSSL_UseSNI(ssl,WOLFSSL_SNI_HOST_NAME,TLS_HOST,
          (unsigned short)strlen(TLS_HOST))!=WOLFSSL_SUCCESS) {
    ret=-646;goto cleanup;
  }
  if(wolfSSL_check_domain_name(ssl,TLS_HOST)!=WOLFSSL_SUCCESS) {
    ret=-647;goto cleanup;
  }
  fd=connect_tls_endpoint();
  if(fd<0){ret=-648;goto cleanup;}
  wolfSSL_SetIOReadCtx(ssl,&fd);
  wolfSSL_SetIOWriteCtx(ssl,&fd);
  rc=wolfSSL_connect(ssl);
  diag_ssl_connect_ret=rc;
  diag_gate_state=(int)tls_gate.state;
  if(rc!=WOLFSSL_SUCCESS) {
    diag_ssl_error=wolfSSL_get_error(ssl,rc);
    scr_printf("wolfSSL %s error=%d\n",label,diag_ssl_error);
    ret=-649;goto cleanup;
  }
  if(tls_gate.state!=2U){ret=-650;goto cleanup;}
  scr_printf("TLS %s: VERIFIED PEER + HOST PASS\n",label);
  if(do_http_get) {
    ret=n5a_get_fixed_probe_ram(ssl);
    if(ret!=0)goto cleanup;
  }
  ret=0;
cleanup:
  if(ssl)wolfSSL_free(ssl);
  if(fd>=0)lwip_close(fd);
  if(ctx)wolfSSL_CTX_free(ctx);
  v12_n4b_seed_clear(&tls_gate);
  wc_SetTimeCb(NULL);
  signed_cert_clock=0;
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
    rc=tls_one_bound(second_seed,upper,"HIGH",1);
  }
  wc_SetSeed_Cb(NULL);
  wc_SetTimeCb(NULL);
  wolfSSL_Cleanup();
  return rc;
}

int main(void) {
  int rc,rt_pass=0;
  uint8_t nonce[32]={0},tls_first[64]={0},tls_second[64]={0};
  v12_rt_signed_time signed_time={0,0};
  v12_net_config cfg;
  init_scr();
  scr_setCursor(0);
  scr_printf("N5H RAM TRACE OBSERVER - STAGE 0\n");
  scr_printf("MATH: FASTMATH PORTABLE C (NO SP/ASM)\n");
  scr_printf("FIXED GET INTO RAM; NO ART/HDD/RTC WRITES\n");
  sleep(1);
  rc=init_io();
  if(rc==0)rc=read_cfg(&cfg);
  if(rc==0)rc=read_entropy_usb(nonce,tls_first,tls_second);
  if(rc==0)rc=read_ca_bundle();
  if(rc==0)rc=init_net(&cfg);
  if(rc==0)rc=live_signed_probe(nonce,&signed_time);
  if(rc==0) {
    rt_pass=1;
    scr_printf("STRICT SIGNED UTC: PASS (diagnostic)\n");
  }
  v12_n4a_wipe(nonce,sizeof(nonce));
  if(rc==0)rc=verify_two_bound_tls(&signed_time,tls_first,tls_second);
  v12_n4a_wipe(tls_first,sizeof(tls_first));
  v12_n4a_wipe(tls_second,sizeof(tls_second));
  if(mounted) {
    int u=fileXioUmount(MOUNT);
    if(u<0 && rc==0)rc=-509;
  }
  scr_clear();
  scr_printf("N5H RAM TRACE - FINAL RESULT\n");
  if(rt_pass)scr_printf("ROUGHTIME SIGNED UTC: PASS\n");
  if(rc==0) {
    scr_printf("TLS1.2 CHAIN/HOST/DATE: PASS\n");
    scr_printf("TWO BOUND CHECKS: PASS\n");
    scr_printf("HTTPS GET 200 RAM: PASS\n");
    scr_printf("PAYLOAD %u BYTES: MATCH\n",n5a_http_info.body_bytes);
  } else {
    scr_printf("N5H ERROR: %d\n",rc);
    scr_printf("N5H LAST=%u EVENTS=%u BEATS=%u\n",n5d_trace_last,n5d_trace_count,n5h_heartbeat);
    scr_printf("IO PHASE=%d W=%d R=%d FAIL=%d\n",n5c_phase,n5c_write_calls,n5c_read_calls,diag_io_fail);
    if(rc==-523) {
      scr_printf("N5B LINK WAIT 20s FAILED\n");
      scr_printf("NETMAN raw=%d expected UP=%d\n",n5b_last_link,NETMAN_NETIF_ETH_LINK_STATE_UP);
      scr_printf("LINK polls=%d ps2ipInit=0\n",n5b_link_polls);
    }
    if(rc<=-710 && rc>=-715) {
      scr_printf("HTTP byte count=%u sent=%u\n",n5a_http_wire_bytes,n5a_http_wrote);
      scr_printf("HTTP parser=%d wolfSSL=%d\n",n5a_http_parse_error,n5a_http_tls_error);
    }
    if(rc==-649) {
      scr_printf("TLS BOUND: %s\n",diag_bound==1?"LOW":diag_bound==2?"HIGH":"UNKNOWN");
      scr_printf("SSL connect=%d err=%d\n",diag_ssl_connect_ret,diag_ssl_error);
      scr_printf("TCP connected=%d DNS rc=%d\n",diag_tcp_connected,diag_dns_rc);
      scr_printf("IO RX=%d TX=%d FAIL=%d\n",diag_io_rx,diag_io_tx,diag_io_fail);
      scr_printf("RNG consumed=%d\n",diag_gate_state==2);
      scr_printf("VERIFY calls=%d fail=%d\n",diag_cert_calls,diag_cert_failed);
      scr_printf("FIRST CERT err=%d depth=%d\n",diag_cert_first_error,diag_cert_first_depth);
      scr_printf("LAST CERT err=%d depth=%d\n",diag_cert_last_error,diag_cert_last_depth);
      scr_printf("CERT total=%d preverify=%d\n",diag_cert_total,diag_cert_last_preverify);
    }
  }
  scr_printf("NOT PRODUCTION HTTPS/CLOCK\n");
  scr_printf("NO HDD/USB/ART/RTC WRITES\n");
  scr_printf("NEVER REUSE N4E.ENT\n");
  scr_printf("RET=%d (screen 120 sec)\n",rc);
  sleep(120);
  return rc==0?0:1;
}

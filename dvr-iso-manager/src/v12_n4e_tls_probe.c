/*
 * DESR v1.2 N4E: isolated STRICT TLS 1.2 handshake diagnostic.
 * This is NOT the Manager: no cover/ISO browser, no RTC updates, no
 * writable mounts, no app/Game ID changes, no HTTP/TLS or filesystem writes.
 * N4A reads a fresh 96-byte host-CSPRNG bundle from USB.
 * 32-byte Roughtime nonce, 2 x 64-byte unique TLS session seeds.
 * USB file reuse after reboot is NOT prevented. NOT production HTTPS.
 * CA+hostname+cert-time are checked by wolfSSL at both time bounds.
 * No HTTP requests, ART writes, RTC writes, or game boot.
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

#define PART "hdd0:PP.DVRISO12"
#define MOUNT "pfs1:"
#define NET_CNF "pfs1:/V12_NET.CNF"
#define NAME "time.txryan.com"
#define PORT "2002"
#define TLS_HOST "raw.githubusercontent.com"
#define TLS_PORT "443"
#define CA_CAP 350000U

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
static unsigned char ca_bundle[CA_CAP+1U];
static size_t ca_bytes=0;
static v12_n4b_seed_gate tls_gate={0};
static time_t signed_cert_clock=0;

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


/* WOLFSSL_USER_IO uses lwIP callbacks; each receive has a finite timeout.
 * Connect itself is bounded by normal lwIP stack behavior and is not
 * a production-grade nonblocking connection timeout policy.
 */
static int tls_recv(WOLFSSL *ssl,char *buffer,int sz,void *ctx) {
  int fd=ctx?*(int *)ctx:-1,rc;
  fd_set rfds;
  struct timeval tv;
  (void)ssl;
  if(fd<0 || !buffer || sz<=0)return WOLFSSL_CBIO_ERR_GENERAL;
  FD_ZERO(&rfds); FD_SET(fd,&rfds);
  tv.tv_sec=8;tv.tv_usec=0;
  rc=lwip_select(fd+1,&rfds,NULL,NULL,&tv);
  if(rc<=0 || !FD_ISSET(fd,&rfds)) return WOLFSSL_CBIO_ERR_GENERAL;
  rc=lwip_recv(fd,buffer,sz,0);
  if(rc==0) return WOLFSSL_CBIO_ERR_CONN_CLOSE;
  return rc<0?WOLFSSL_CBIO_ERR_GENERAL:rc;
}
static int tls_send(WOLFSSL *ssl,char *buffer,int sz,void *ctx) {
  int fd=ctx?*(int *)ctx:-1,rc;
  fd_set wfds;
  struct timeval tv;
  (void)ssl;
  if(fd<0 || !buffer || sz<=0)return WOLFSSL_CBIO_ERR_GENERAL;
  FD_ZERO(&wfds); FD_SET(fd,&wfds);
  tv.tv_sec=8;tv.tv_usec=0;
  rc=lwip_select(fd+1,NULL,&wfds,NULL,&tv);
  if(rc<=0 || !FD_ISSET(fd,&wfds))return WOLFSSL_CBIO_ERR_GENERAL;
  rc=lwip_send(fd,buffer,sz,0);
  return rc<=0?WOLFSSL_CBIO_ERR_GENERAL:rc;
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
  if(rc!=0 || !resolved) {
    if(resolved)freeaddrinfo(resolved);
    return -1;
  }
  for(cur=resolved;cur;cur=cur->ai_next) {
    if(cur->ai_family!=AF_INET)continue;
    fd=lwip_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(fd<0)continue;
    if(lwip_connect(fd,cur->ai_addr,cur->ai_addrlen)==0)break;
    lwip_close(fd);fd=-1;
  }
  freeaddrinfo(resolved);
  return fd;
}
/* One distinct CSPRNG input for EACH TLS session.
 * Returns zero only after chain+hostname+date check in wolfSSL_connect.
 * No HTTP payload is sent even after success.
 */
static int tls_one_bound(const uint8_t seed[64],time_t bound,
                         const char *label) {
  WOLFSSL_CTX *ctx=NULL;
  WOLFSSL *ssl=NULL;
  int fd=-1,rc,ret=-645;
  if(v12_n4b_seed_provision(&tls_gate,seed,64)!=0)return -641;
  signed_cert_clock=bound;
  if(wc_SetTimeCb(signed_time_cb)!=0){ret=-642;goto cleanup;}
  ctx=wolfSSL_CTX_new(wolfTLSv1_2_client_method());
  if(!ctx){ret=-643;goto cleanup;}
  wolfSSL_CTX_set_verify(ctx,WOLFSSL_VERIFY_PEER,NULL);
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
  if(rc!=WOLFSSL_SUCCESS) {
    scr_printf("wolfSSL %s error=%d\n",label,wolfSSL_get_error(ssl,rc));
    ret=-649;goto cleanup;
  }
  if(tls_gate.state!=2U){ret=-650;goto cleanup;}
  scr_printf("TLS %s: VERIFIED PEER + HOST PASS\n",label);
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
  rc=tls_one_bound(first_seed,lower,"LOW");
  if(rc==0) {
    scr_printf("STAGE 9: TLS upper bound\n");
    rc=tls_one_bound(second_seed,upper,"HIGH");
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
  scr_printf("N4E TLS1.2 BOOT - STAGE 0\n");
  scr_printf("NO HTTP / ART / HDD WRITES / RTC\n");
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
  scr_printf("N4E TLS1.2 - FINAL RESULT\n");
  if(rt_pass)scr_printf("ROUGHTIME SIGNED UTC: PASS\n");
  if(rc==0) {
    scr_printf("TLS1.2 CHAIN/HOST/DATE: PASS\n");
    scr_printf("TWO BOUND CHECKS: PASS\n");
  } else scr_printf("N4E ERROR: %d\n",rc);
  scr_printf("NOT PRODUCTION HTTPS/CLOCK\n");
  scr_printf("NO HTTP/ART/HDD/RTC WRITES\n");
  scr_printf("NEVER REUSE N4E.ENT\n");
  scr_printf("RET=%d (screen 120 sec)\n",rc);
  sleep(120);
  return rc==0?0:1;
}

/*
 * N6F: N6E-PASS native HTTPS cover GET, strict one-cover USB ART save.
 * Full N6F signed TLS + HTTPS; USB-only staged .PART readback+rename. No HDD/RTC writes.
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
#include <stdlib.h>
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
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
#include "usb12_progress_ui.h"
#endif
#ifdef DESR_USB12_CPU_ENTROPY
#include "n6g_cpu_entropy.h"
#endif
#ifdef DESR_USB12_HTTPS
#include <elf-loader.h>
#include "usb12_covers.h"
#endif
#ifdef N6G_SCREEN_PROBE
#include "n6g_screen_probe.h"
#endif
#ifdef N6G_TLS_ARENA
#include "n6g_tls_arena.h"
static int n6g_use_arena=1;
#endif
#ifdef N6G_BATCH_50
#include "n6g_batch_entropy.h"
#include "n6g_roughtime_retry.h"
static int batch_trace_prep;
static int batch_trace_new;
static int batch_ca_first;
#endif

#ifdef N6G_PNG_WORKSPACE
#define USB_TEST_NAME "N6G"
#else
#define USB_TEST_NAME "N6F"
#endif

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
static char n6_request[448];
static char n6_current_id[12];
static char n6_targets[N6_BATCH_MAX][12];
static uint8_t n6_response[N6_HTTP_CAP+1U];
static const uint8_t *n6_png=0;
static size_t n6_png_size=0;
static size_t n6_wire_bytes=0;
static int n6_http_rc=0;
static int n6_source=0; /* 0: OPL Manager dump, 1: CosmicScale */

#ifdef N6G_PNG_WORKSPACE
static int normalize_download(const uint8_t **output,size_t *size,
                              int bad_image) {
  int rc=n6_normalize_png_into(n6_png,n6_png_size,n6_response,
                               sizeof(n6_response),size);
  if(rc==-3)return -721; /* local workspace / decoder allocation failure */
  if(rc!=0)return bad_image;
  *output=n6_response;
  return 0;
}
#endif

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
#ifdef DESR_USB12_HTTPS
static int diag_roughtime_policy=DESR_USB12_ROUGHTIME_NOT_CHECKED;
#endif
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

/* Consumer-only R4 UI: ONLY the main EE thread paints progress, and ONLY
 * before or after a completed cover attempt. This deliberately avoids
 * all GS/scr_printf activity in wolfSSL callbacks or the N5H observer.
 * Diagnostics still exist in the unchanged R4 commit. R4 HTTPS networking,
 * parser, TLS, entropy, normalization and USB writer stay untouched.
 *
 * ASCII transliteration is intentional: ps2sdk's debug font is not UTF-8.
 */
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
static usb12_progress_ui n6g_ui_progress;
static int n6g_ui_started;
static int n6g_ui_active;
static int n6g_ui_finished;

static void n6g_ui_redraw(void) {
  char line[40];
  int first;
  scr_setbgcolor(0x000000);
  scr_setfontcolor(0xFFFFFF);
  scr_clear();
  scr_setCursor(0);
  scr_printf("PSX DVR ISO MANAGER 1.2 USB\n");
  scr_printf("-----------------------------------------\n\n");
  if(n6g_ui_finished) {
    scr_printf(n6g_ui_progress.errors ?
               "DOWNLOAD INTERRUPTED\n\n" :
               "DOWNLOAD COMPLETE\n\n");
    if(n6g_ui_progress.total==0) {
      if(n6g_ui_progress.errors)
        scr_printf("DOWNLOAD PREPARATION FAILED\n");
      else
        scr_printf("NO MISSING COVERS\n");
    }else {
      scr_printf("DOWNLOADED: %d/%d\n",n6g_ui_progress.saved,
                 n6g_ui_progress.total);
      scr_printf("SKIPPED: %d\n",n6g_ui_progress.skipped);
      if(n6g_ui_progress.errors)
        scr_printf("ERROR DURING DOWNLOAD\n");
    }
    scr_printf("\nRETURNING TO MANAGER...\n");
    return;
  }
  if(!n6g_ui_started) {
    scr_printf("PREPARING COVER DOWNLOAD\n");
    scr_printf("PLEASE WAIT...\n");
    return;
  }
  scr_printf("DOWNLOADING MISSING COVERS\n");
  scr_printf("PLEASE WAIT...\n\n");
  first=n6g_ui_progress.completed-USB12_UI_VISIBLE_ROWS+1;
  if(first<1)first=1;
  for(int i=first;i<=n6g_ui_progress.completed;i++) {
    if(usb12_progress_line(&n6g_ui_progress,i,line,sizeof(line))==0)
      scr_printf("%s\n",line);
  }
  if(n6g_ui_active>n6g_ui_progress.completed &&
     n6g_ui_active<=n6g_ui_progress.total)
    scr_printf("%d/%d DOWNLOADING...\n",n6g_ui_active,
               n6g_ui_progress.total);
  scr_printf("\nDOWNLOADED: %d  SKIPPED: %d\n",
             n6g_ui_progress.saved,n6g_ui_progress.skipped);
}

/* Called around complete cover operations, never inside TCP/TLS/HTTP I/O. */
static void n6g_ui_prepare(void) {
  n6g_ui_started=0;
  n6g_ui_finished=0;
  n6g_ui_active=0;
  n6g_ui_redraw();
}
static void n6g_ui_begin(int targets) {
  if(usb12_progress_begin(&n6g_ui_progress,targets)!=0)
    usb12_progress_begin(&n6g_ui_progress,0);
  n6g_ui_started=1;
  n6g_ui_redraw();
}
static void n6g_ui_processing(int ordinal) {
  n6g_ui_active=ordinal;
  n6g_ui_redraw();
}
static void n6g_ui_done(int ordinal,enum usb12_ui_result result) {
  if(usb12_progress_record(&n6g_ui_progress,ordinal,result)==0) {
    n6g_ui_active=0;
    n6g_ui_redraw();
  }
}
static void n6g_ui_summary(int targets,int saved,int skipped,int rc) {
  if(!n6g_ui_started) {
    usb12_progress_begin(&n6g_ui_progress,targets>0?targets:0);
    n6g_ui_started=1;
  }
  /* Keep summary faithful to the downloader even on early failures.
   * All counts come from the trusted R4 batch loop, never screen parsing.
   */
  n6g_ui_progress.saved=saved;
  n6g_ui_progress.skipped=skipped;
  if(rc<0 && !(rc==-404 && skipped==targets && targets>0) &&
     n6g_ui_progress.errors==0)
    n6g_ui_progress.errors=1;
  n6g_ui_finished=1;
  n6g_ui_redraw();
}

/* Diagnostic output from the old R4 stays in the source, but is suppressed
 * in the UI-only test. In particular the N5H observer must NOT clear the
 * display on a secondary EE thread. It is still started/stopped normally.
 * Revert the compile definition to restore R4 debug output verbatim.
 */
#define scr_printf(...) ((void)0)
#define scr_clear() ((void)0)
#define scr_setCursor(...) ((void)0)
#endif


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
  /* Verified DESR driver order: CDVD notice BEFORE DVR IRXs. */
#ifndef N6D_NO_DVR
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


static int load_usb_modules(void) {
  int rc;
  rc=load_irx("bdm",bdm_irx,size_bdm_irx,NULL,0);
  if(rc<0)return -510;
  rc=load_irx("bdmfs_fatfs",bdmfs_fatfs_irx,size_bdmfs_fatfs_irx,NULL,0);
  if(rc<0)return -510;
  rc=load_irx("usbd_mini",usbd_mini_irx,size_usbd_mini_irx,NULL,0);
  if(rc<0)return -510;
  rc=load_irx("usbmass_bd_mini",usbmass_bd_mini_irx,size_usbmass_bd_mini_irx,NULL,0);
  if(rc<0)return -510;
  return 0;
}

/* Private entropy comes from the host CSPRNG and is read exactly once. */
#ifndef DESR_USB12_CPU_ENTROPY
static int read_entropy_file(const char *path,uint8_t *data,int bytes) {
  int rc,fd=-1,n,got=0,extra,closed;
  uint8_t tail=0;
#ifdef N6G_BATCH_50
  /* Bound each SIF read and keep DMA away from the private heap allocation. */
  uint8_t transfer[4096] __attribute__((aligned(64)));
#endif
  scr_printf("USB %s read-only: %d bytes\n",path,bytes);
  rc=load_usb_modules();
  if(rc<0)return rc;
  for(int i=0;i<16;i++) {
    fd=fileXioOpen(path,FIO_O_RDONLY,0);
    if(fd>=0)break;
    usleep(500000);
  }
  if(fd<0)return -511;
  while(got<bytes) {
    int chunk=bytes-got;
    uint8_t *destination=data+got;
#ifdef N6G_BATCH_50
    if(chunk>(int)sizeof(transfer))chunk=(int)sizeof(transfer);
    destination=transfer;
#endif
    n=fileXioRead(fd,destination,chunk);
    if(n<=0 || n>chunk)break;
#ifdef N6G_BATCH_50
    memcpy(data+got,transfer,(size_t)n);
#endif
    got+=n;
  }
  extra=fileXioRead(fd,&tail,1);
  closed=fileXioClose(fd);
  v12_n4a_wipe(&tail,sizeof(tail));
#ifdef N6G_BATCH_50
  v12_n4a_wipe(transfer,sizeof(transfer));
#endif
  if(closed<0||extra!=0||got!=bytes) {
    v12_n4a_wipe(data,(size_t)bytes);return -512;
  }
  return 0;
}
#endif
#ifndef N6G_BATCH_50
static int read_entropy_usb(uint8_t nonce[32],
                            uint8_t seeds[N6_BATCH_MAX][4][64]) {
  uint8_t data[N6_ENT_BYTES]={0};
  int rc=read_entropy_file("mass0:/N6C.ENT",data,(int)sizeof(data));
  if(rc!=0)return rc;
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
  scr_printf("N6C: fresh nonce + 4 distinct TLS seeds loaded\n");
  return 0;
}
#endif

static int read_ca_bundle(void) {
  int fd=-1,n,closed,extra;
  uint8_t tail;
  size_t total=0;
#ifdef DESR_USB12_CPU_ENTROPY
  const int attempts=16;
#else
  const int attempts=1; /* Reading .ENT already waited for USB. */
#endif
  scr_printf("STAGE 4B: USB DESR_DVR/N4E.CA (PEM)\n");
  for(int i=0;i<attempts;i++) {
#ifdef DESR_USB12_HTTPS
    /* One authoritative CA location for USB 1.2: DESR_DVR beside the ELF.
     * Do not silently fall back to an outdated CA at the USB root. */
    fd=fileXioOpen(DESR_USB12_CA_FILE,FIO_O_RDONLY,0);
#else
    /* Keep the legacy standalone diagnostic layout unchanged. */
    fd=fileXioOpen("mass0:/N4E.CA",FIO_O_RDONLY,0);
    if(fd<0)fd=fileXioOpen("mass:/N4E.CA",FIO_O_RDONLY,0);
#endif
    if(fd>=0)break;
    if(i+1<attempts)usleep(500000);
  }
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
#ifdef DESR_USB12_HTTPS
    diag_roughtime_policy=rc;
#endif
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
static int n6_get_cover_ram(WOLFSSL *ssl) {
  int len,rc,got,reads=0;
  size_t sent=0,total=0;
  const uint8_t *png=NULL;
  size_t png_size=0;
  if(!ssl || !desr_valid_game_id(n6_current_id))return -710;
  n6_png=NULL;n6_png_size=0;n6_wire_bytes=0;n6_http_rc=0;
  if(n6_source==0)
    len=snprintf(n6_request,sizeof(n6_request),
      "GET /" N6_ARCHIVE "/" N6_PIN "/PS2/%s/%s_COV.png HTTP/1.1\r\n"
      "Host: raw.githubusercontent.com\r\n"
      "User-Agent: PSX-DVR-ISO-MANAGER-N6/1.2\r\n"
      "Accept: image/png\r\nAccept-Encoding: identity\r\n"
      "Connection: close\r\n\r\n",n6_current_id,n6_current_id);
  else
    len=snprintf(n6_request,sizeof(n6_request),
      "GET /" N6_FALLBACK_ARCHIVE "/" N6_FALLBACK_PIN "/art/%s.png HTTP/1.1\r\n"
      "Host: raw.githubusercontent.com\r\n"
      "User-Agent: PSX-DVR-ISO-MANAGER-N6/1.2\r\n"
      "Accept: image/png\r\nAccept-Encoding: identity\r\n"
      "Connection: close\r\n\r\n",n6_current_id);
  if(len<=0 || len>=(int)sizeof(n6_request))return -710;
  scr_clear();scr_setCursor(0);
  scr_printf(USB_TEST_NAME " HTTPS GAME: %s source=%s\n",n6_current_id,
             n6_source==0?"OPL":"PSBBN");
  scr_printf("TRUST: SIGNED UTC, TLS PEER/HOST BOTH BOUNDS\n");
  scr_printf("STAGE 10: HTTPS GET COVER -> RAM\n");
  n5c_http_active=1;
  while(sent<(size_t)len) {
    int watchid=-1;
#ifdef N6G_SCREEN_PROBE
    if(!n6g_screen_muted())
#endif
    {
      watchid=n5h_start_observer();
      if(watchid<0){n5c_http_active=0;return -717;}
    }
    n5c_phase=1;
    rc=wolfSSL_write(ssl,n6_request+sent,len-(int)sent);
    n5h_stop_observer(watchid);
    if(rc<=0) {
      n5a_http_tls_error=wolfSSL_get_error(ssl,rc);
      n5c_http_active=0;return -710;
    }
    sent+=(size_t)rc;
  }
  n5c_http_active=0;
  scr_clear();scr_setCursor(0);
  scr_printf(USB_TEST_NAME " TLS WRITE DONE: %u bytes\n",(unsigned)sent);
  for(reads=0;reads<300;reads++) {
    size_t available=N6_HTTP_CAP-total;
    if(available<1U)return -713;
    if(available>2048U)available=2048U;
    n5c_phase=2;
    got=wolfSSL_read(ssl,n6_response+total,(int)available);
    if(got>0) {
      total+=(size_t)got;
      n6_wire_bytes=total;
      if(total>=13U) {
        n6_http_rc=probe_http_status(n6_response,total);
        if(n6_http_rc==404)return -404;
        if(n6_http_rc>=300 || (n6_http_rc>0 && n6_http_rc!=200))
          return -712;
      }
      if(probe_http_extract_png(n6_response,total,&png,&png_size)==0){
        n6_png=png;
        n6_png_size=png_size;
        scr_printf(USB_TEST_NAME " PNG verified CRC: %u bytes\n",(unsigned)png_size);
        return 0;
      }
    }else{
      n5a_http_tls_error=wolfSSL_get_error(ssl,got);
      if(got==0||n5a_http_tls_error==WOLFSSL_ERROR_ZERO_RETURN) {
        if(probe_http_extract_png(n6_response,total,&png,&png_size)==0){
          n6_png=png;n6_png_size=png_size;return 0;
        }
        return -714;
      }
      return -711;
    }
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

#ifdef N6G_BATCH_50
int __real_InitSSL(WOLFSSL *ssl,WOLFSSL_CTX *ctx,int write_dup);
int __real_wc_InitRng_ex(WC_RNG *rng,void *heap,int dev_id);

/* Trace only the first game's constructor. Never print from a handshake. */
int __wrap_InitSSL(WOLFSSL *ssl,WOLFSSL_CTX *ctx,int write_dup) {
  int rc;
  if(batch_trace_new)scr_printf("N6G50 SSL_NEW: INIT_SSL enter\n");
  rc=__real_InitSSL(ssl,ctx,write_dup);
  if(batch_trace_new)scr_printf("N6G50 SSL_NEW: INIT_SSL return=%d\n",rc);
  return rc;
}

int __wrap_wc_InitRng_ex(WC_RNG *rng,void *heap,int dev_id) {
  int rc;
  if(batch_trace_new)scr_printf("N6G50 SSL_NEW: DRBG enter\n");
  rc=__real_wc_InitRng_ex(rng,heap,dev_id);
  if(batch_trace_new)scr_printf("N6G50 SSL_NEW: DRBG return=%d\n",rc);
  return rc;
}
#endif

static WOLFSSL *tls_make_session(WOLFSSL_CTX *ctx,const char *label) {
#ifdef N6G_BATCH_50
  WOLFSSL *ssl;
  if(batch_trace_prep)scr_printf("N6G50 %s PREP: SSL_NEW\n",label);
  batch_trace_new=batch_trace_prep;
  ssl=wolfSSL_new(ctx);
  batch_trace_new=0;
  return ssl;
#else
  (void)label;
  return wolfSSL_new(ctx);
#endif
}

/* N6D: minimal single-thread status around verified TLS. No observer and
 * no scr_printf INSIDE wolfSSL or between I/O binding and wolfSSL_connect.
 * When the blocking TLS call returns, the main thread paints the result.
 * No hard timeout can be inferred from a frozen screen. */
static int tls_one_bound(const uint8_t seed[64],time_t bound,
                         const char *label,int do_http_get) {
  WOLFSSL_CTX *ctx=NULL;
  WOLFSSL *ssl=NULL;
  int fd=-1,rc,ret=-645;
  n5c_http_active=0;
  scr_printf(USB_TEST_NAME " %s: seed/time/context\n",label);
  if(v12_n4b_seed_provision(&tls_gate,seed,64)!=0)return -641;
  signed_cert_clock=bound;
  if(wc_SetTimeCb(signed_time_cb)!=0){ret=-642;goto cleanup;}
#ifdef N6G_BATCH_50
  if(batch_trace_prep)scr_printf("N6G50 %s PREP: CTX_NEW\n",label);
#endif
  ctx=wolfSSL_CTX_new(wolfTLSv1_2_client_method());
  if(!ctx){ret=-643;goto cleanup;}
  wolfSSL_CTX_set_verify(ctx,WOLFSSL_VERIFY_PEER,diag_verify_cb);
  wolfSSL_CTX_SetIORecv(ctx,tls_recv);
  wolfSSL_CTX_SetIOSend(ctx,tls_send);
#ifdef N6G_BATCH_50
  /* InitSSL retains its context. Loading CA into that context must succeed
   * before any TCP or TLS I/O in both allocation orders. */
  if(!batch_ca_first) {
    ssl=tls_make_session(ctx,label);
    if(!ssl){ret=-645;goto cleanup;}
  }
  if(batch_trace_prep)scr_printf("N6G50 %s PREP: LOAD_CA %u bytes\n",label,
                                 (unsigned)ca_bytes);
#endif
  if(wolfSSL_CTX_load_verify_buffer(ctx,ca_bundle,(long)ca_bytes,
       WOLFSSL_FILETYPE_PEM)!=WOLFSSL_SUCCESS){ret=-644;goto cleanup;}
  if(!ssl) {
    ssl=tls_make_session(ctx,label);
    if(!ssl){ret=-645;goto cleanup;}
  }
#ifdef N6G_BATCH_50
  if(batch_trace_prep)scr_printf("N6G50 %s PREP: SNI/HOST SETUP\n",label);
#endif
  if(wolfSSL_UseSNI(ssl,WOLFSSL_SNI_HOST_NAME,TLS_HOST,
         (unsigned short)strlen(TLS_HOST))!=WOLFSSL_SUCCESS){
    ret=-646;goto cleanup;
  }
  if(wolfSSL_check_domain_name(ssl,TLS_HOST)!=WOLFSSL_SUCCESS){
    ret=-647;goto cleanup;
  }
  scr_printf(USB_TEST_NAME " %s: CA/SNI/HOST OK, TCP next\n",label);
  fd=connect_tls_endpoint();
  if(fd<0){ret=-648;goto cleanup;}
  scr_printf(USB_TEST_NAME " %s: TCP CONNECTED, TLS START\n",label);
  /* No screen writes or active diagnostic callbacks until return. */
  wolfSSL_SetIOReadCtx(ssl,&fd);
  wolfSSL_SetIOWriteCtx(ssl,&fd);
  rc=wolfSSL_connect(ssl);
  diag_ssl_connect_ret=rc;
  diag_gate_state=(int)tls_gate.state;
  scr_printf(USB_TEST_NAME " %s: TLS RETURN=%d\n",label,rc);
  if(rc!=WOLFSSL_SUCCESS){
    diag_ssl_error=wolfSSL_get_error(ssl,rc);
    ret=-649;goto cleanup;
  }
  if(tls_gate.state!=2U){ret=-650;goto cleanup;}
  scr_printf(USB_TEST_NAME " %s: VERIFIED PEER/HOST PASS\n",label);
  /* Only after successful strict verification, execute HTTP GET to RAM.
   * N5H observer is started ONLY by n6_get_cover_ram() at wolfSSL_write,
   * not before TLS; never performs disk/ART writes. */
  if(do_http_get){
    scr_printf(USB_TEST_NAME " HTTP: GET ENTER (POST TLS)\n");
    ret=n6_get_cover_ram(ssl);
    scr_printf(USB_TEST_NAME " HTTP: GET RETURN=%d STATUS=%d BYTES=%u\n",
               ret,n6_http_rc,(unsigned)n6_wire_bytes);
    if(ret!=0)goto cleanup;
  }
  ret=0;
cleanup:
  n5c_http_active=0;
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

#ifdef N6G_TLS_ARENA
static int verify_cover_tls(const v12_rt_signed_time *signed_time,
                            const uint8_t low[64],const uint8_t high[64]) {
  n6g_tls_arena_stats stats;
  int rc,status;
  if(!n6g_use_arena)return verify_two_bound_tls(signed_time,low,high);
  if(n6g_tls_arena_begin()!=0)return -730;
  rc=verify_two_bound_tls(signed_time,low,high);
  status=n6g_tls_arena_end(&stats);
  scr_printf("ARENA bytes=%u live=%u oom=%u invalid=%u TLS=%d\n",
             (unsigned)stats.used,stats.live,stats.exhausted,stats.invalid,rc);
  return status==0?rc:-731;
}
#else
#define verify_cover_tls verify_two_bound_tls
#endif

#ifdef N6G_BATCH_50
static int retry_signed_probe(const uint8_t nonce[32],void *ctx) {
  return live_signed_probe(nonce,(v12_rt_signed_time *)ctx);
}

static int fetch_batch_cover(const n6g_source_entropy *entropy,int source,
                             const uint8_t **normalized,size_t *normalized_size) {
  v12_rt_signed_time signed_time={0,0};
  int rc;
  n6_source=source;
  n6_http_rc=0;
#ifdef DESR_USB12_HTTPS
  diag_ssl_error=0;
  diag_roughtime_policy=DESR_USB12_ROUGHTIME_NOT_CHECKED;
#endif
  n6_wire_bytes=0;
  n6_png=NULL;
  n6_png_size=0;
  *normalized=NULL;
  *normalized_size=0;
  /* Invalid Roughtime replies never become accepted. Each retry
   * carries a different nonce from this game's/source's seed. */
  rc=n6g_roughtime_retry(entropy->nonce,retry_signed_probe,&signed_time);
  if(rc==0)rc=verify_cover_tls(&signed_time,entropy->low,entropy->high);
  if(rc==0)rc=normalize_download(normalized,normalized_size,-565);
  return rc;
}

static int unavailable_cover(int rc) {
  return rc==-404 || rc==-565 || (rc==-714 && n6_http_rc==200);
}

static int save_batch(n6g_batch_entropy entropy,int targets,
                      int *attempted,int *saved,int *unavailable) {
  int first_cover_error=0;
  for(int game=0;game<targets;game++) {
    const uint8_t *normalized=NULL;
    size_t normalized_size=0;
    int rc;
    memcpy(n6_current_id,n6_targets[game],sizeof(n6_current_id));
    *attempted=game+1;
    batch_trace_prep=game==0;
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
    n6g_ui_processing(*attempted);
#endif
    scr_printf("N6G50 GAME %d/%d ID=%s SAVED=%d\n",
               *attempted,targets,n6_current_id,*saved);
    rc=fetch_batch_cover(&entropy[game][0],0,&normalized,&normalized_size);
    if(unavailable_cover(rc)) {
      scr_printf("N6G50 FALLBACK PSBBN: %s (%d)\n",n6_current_id,rc);
      rc=fetch_batch_cover(&entropy[game][1],1,&normalized,&normalized_size);
    }
    v12_n4a_wipe(entropy[game],sizeof(entropy[game]));
    if(unavailable_cover(rc)) {
      if(first_cover_error==0)first_cover_error=rc;
      (*unavailable)++;
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
      n6g_ui_done(*attempted,USB12_UI_SKIPPED);
#endif
      scr_printf("N6G50 COVER UNAVAILABLE: %s (%d)\n",n6_current_id,rc);
      continue;
    }
    if(rc!=0) {
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
      n6g_ui_done(*attempted,USB12_UI_ERROR);
#endif
      return rc;
    }
    if(!normalized || !normalized_size ||
       probe_png_check(normalized,normalized_size)!=0) {
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
      n6g_ui_done(*attempted,USB12_UI_ERROR);
#endif
      return -720;
    }
    rc=n6_write_usb_art(n6_current_id,normalized,normalized_size);
    if(rc!=0) {
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
      n6g_ui_done(*attempted,USB12_UI_ERROR);
#endif
      return rc;
    }
    (*saved)++;
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
    n6g_ui_done(*attempted,USB12_UI_OK);
#endif
    scr_printf("N6G50 SAVED %d: %s USB READBACK+RENAME PASS\n",
               *saved,n6_current_id);
  }
  if(first_cover_error!=0 && *saved==0)return first_cover_error;
  /* Completed batch with explicit unavailable count, or all selected saved. */
  return 0;
}

#ifdef DESR_USB12_HTTPS
#ifdef DESR_USB12_CPU_ENTROPY
static int fill_cpu_entropy(int argc, char **argv, n6g_batch_entropy entropy) {
  uint32_t samples[N6G_CPU_SAMPLE_COUNT];
  char *end;
  unsigned long launch_ticks;
  int rc;

  if(argc!=3 || strlen(argv[2])!=8)return -735;
  launch_ticks=strtoul(argv[2],&end,16);
  if(*end)return -735;

  scr_printf("ENTROPY SOURCE: CPU COUNTER (EXPERIMENTAL)\n");
  for(unsigned i=0;i<N6G_CPU_SAMPLE_COUNT;i+=2) {
    samples[i]=n6g_read_cpu_count();
    usleep(1000+(i%16)*125);
    samples[i+1]=n6g_read_cpu_count();
  }
  rc=n6g_expand_cpu_entropy(entropy,(uint32_t)launch_ticks,samples);
  v12_n4a_wipe(samples,sizeof(samples));
  return rc==0?0:-735;
}
#endif

static void return_to_manager(int saved, int result, int english, char *details) {
  char saved_arg[12], result_arg[12];
  char *args[]={DESR_USB12_ART_RESULT,saved_arg,result_arg,english?"1":"0",
                details,n6_current_id[0]?n6_current_id:"-"};

  snprintf(saved_arg,sizeof(saved_arg),"%d",saved);
  snprintf(result_arg,sizeof(result_arg),"%d",result);
  int loader_rc;
  scr_printf("Returning to Manager 1.2 in 8 seconds...\n");
  sleep(8);
  /* Do NOT place LoadELFFromFile inside a diagnostic printf: UI builds
   * intentionally suppress printf, but returning to Manager is essential.
   * Preserve the exact R4 loader argument contract and timing. */
  loader_rc=LoadELFFromFile(DESR_USB12_MANAGER_ELF,6,args);
  scr_printf("Manager loader returned %d. Reopen Manager 1.2.\n",loader_rc);
}

int main(int argc, char **argv) {
  int io_ready;
  int english=argc>=2 && strcmp(argv[1],"1")==0;
#else
int main(void) {
#endif
  int rc,targets=0,iso_count=0,ids_count=0,missing=0;
  int attempted=0,saved=0,unavailable=0;
  v12_net_config cfg;
  n6g_batch_entropy *entropy=NULL;
  init_scr();scr_setCursor(0);
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
  n6g_ui_prepare();
#endif
  scr_printf("N6G50 R3 BATCH HTTPS USB ART - MAX 50\n");
  scr_printf("DVR HDD / PP.DVRISO12: READ ONLY\n");
  sleep(1);
  rc=init_io();
#ifdef DESR_USB12_HTTPS
  io_ready=rc==0;
#endif
  if(rc==0)rc=read_cfg(&cfg);
  if(rc==0) {
    entropy=malloc(sizeof(*entropy));
    if(!entropy)rc=-734;
  }
#ifdef DESR_USB12_HTTPS
  /* Keep USB available for the return ELF if config or allocation failed. */
  if(io_ready && rc!=0)load_usb_modules();
#endif
#ifdef DESR_USB12_CPU_ENTROPY
  if(rc==0)rc=load_usb_modules();
  if(rc==0)rc=fill_cpu_entropy(argc,argv,*entropy);
#else
#ifdef DESR_USB12_HTTPS
  if(rc==0)rc=read_entropy_file(DESR_USB12_ENTROPY_FILE,
#else
  if(rc==0)rc=read_entropy_file("mass0:/N6G50.ENT",
#endif
                               (uint8_t *)entropy,(int)sizeof(*entropy));
#endif
  if(rc==0 && !n6g_batch_entropy_valid(*entropy))rc=-516;
  if(rc==0)rc=read_ca_bundle();
  if(rc==0) {
    scr_printf("N6G50 SCAN DVR ISO READ-ONLY\n");
    targets=n6_scan_dvr(n6_targets,&iso_count,&ids_count,&missing);
    if(targets<0)rc=targets;
    else if(ids_count==0)rc=-562;
    else if(targets==0 && missing>0)rc=-561;
  }
  if(rc==0 && targets>0) {
#ifndef DESR_USB12_HTTPS
    int marker=fileXioOpen("mass0:/N6G.CAFIRST",FIO_O_RDONLY,0);
    if(marker>=0)fileXioClose(marker);
    batch_ca_first=marker>=0;
#endif
    scr_printf("TLS INIT ORDER: %s\n",batch_ca_first?"CA FIRST":"SSL FIRST");
  }
  if(rc==0 && targets>0)rc=init_net(&cfg);
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
  if(rc==0) n6g_ui_begin(targets);
#endif
  if(rc==0 && targets>0)rc=save_batch(*entropy,targets,&attempted,&saved,&unavailable);
  if(entropy) {
    v12_n4a_wipe(entropy,sizeof(*entropy));
    free(entropy);
  }
  if(mounted){
    int unmount=fileXioUmount(MOUNT);
    if(unmount<0 && rc==0)rc=-509;
  }
#if defined(DESR_USB12_HTTPS) && defined(N6G_UI_SIMPLE)
  n6g_ui_summary(targets,saved,unavailable,rc);
#endif
  scr_clear();scr_setCursor(0);
  scr_printf("N6G50 HTTPS USB ART - FINAL RESULT\n");
  scr_printf("DVR ISO=%d ID=%d USB MISSING AT START=%d\n",
             iso_count,ids_count,missing);
  scr_printf("BATCH=%d ATTEMPTED=%d SAVED=%d UNAVAILABLE=%d\n",
             targets,attempted,saved,unavailable);
  scr_printf("LAST=%s HTTP=%d TLSERR=%d\n",n6_current_id,n6_http_rc,diag_ssl_error);
  scr_printf("RET=%d\n",rc);
#ifdef DESR_USB12_HTTPS
  if(io_ready) {
    char details[110];
    snprintf(details,sizeof(details),"%d,%d,%d,%d,%d,%d,%d,%d,%d",
             iso_count,ids_count,missing,targets,attempted,unavailable,
             n6_http_rc,diag_ssl_error,diag_roughtime_policy);
    return_to_manager(saved,rc,english,details);
  } else scr_printf("Reopen Manager 1.2 after restarting the console.\n");
#endif
  sleep(120);
  return rc==0?0:1;
}
#else
int main(void) {
  int rc=0,rt_pass=0,targets=0,iso_count=0,ids_count=0,missing=0;
  int verified=0,saved=0,not_found=0,source_ok=-1;
  uint8_t nonce[32]={0},seeds[N6_BATCH_MAX][4][64]={0};
  v12_rt_signed_time signed_time={0,0};
  const uint8_t *normalized=NULL;
  size_t normalized_size=0;
  v12_net_config cfg;
#if defined(N6G_SCREEN_PROBE) && !defined(N6G_HEADLESS)
  int quiet_run=0;
#endif
#ifdef N6G_HEADLESS
  n6g_mute_screen(1);
#else
  init_scr();scr_setCursor(0);
  scr_printf(USB_TEST_NAME " ONE COVER HTTPS -> USB ART TEST\n");
  scr_printf("SIGNED TLS+GET, USB .PART+READBACK+RENAME\n");
  scr_printf("DVR HDD / PP.DVRISO12: READ ONLY\n");
  scr_printf("ONLY WRITE mass0:/ART/ (ONE NEW COVER)\n");
#endif
#ifdef N6G_TLS_ARENA
  scr_printf("N6G ARENA - TLS HEAP COMPARISON\n");
#endif
  sleep(1);
  /* Keep N6E response-size memory pressure; normalizer adds static RAM. */
  ((volatile uint8_t*)n6_response)[0]=0;
  ((volatile uint8_t*)n6_response)[N6_HTTP_CAP]=0;
  rc=init_io();
  if(rc==0)rc=read_cfg(&cfg);
  if(rc==0)rc=read_entropy_usb(nonce,seeds);
  if(rc==0)rc=read_ca_bundle();
  if(rc==0) {
    scr_printf(USB_TEST_NAME " SCAN DVR ISO READ-ONLY\n");
    targets=n6_scan_dvr(n6_targets,&iso_count,&ids_count,&missing);
    if(targets<0)rc=targets;
    else {
      scr_printf("DVR ISO=%d ID=%d USB MISSING=%d SELECTED=%d\n",
                 iso_count,ids_count,missing,targets);
      if(targets>0) {
        memcpy(n6_current_id,n6_targets[0],sizeof(n6_current_id));
        scr_printf("MISSING GAME ID=%s\n",n6_current_id);
      }else if(missing>0) {
        /* A pre-existing .PART blocks the only missing ID; never erase. */
        rc=-561;
        scr_printf(USB_TEST_NAME " BLOCKED: MISSING BUT NO SAFE NEW TARGET\n");
      }else{
        scr_printf(USB_TEST_NAME " NO MISSING COVERS: NO NETWORK OR FILE WRITES\n");
      }
    }
  }
#if defined(N6G_SCREEN_PROBE) && !defined(N6G_HEADLESS)
  if(rc==0 && targets>0) {
    int marker=fileXioOpen("mass0:/N6G.SCREEN",FIO_O_RDONLY,0);
    if(marker>=0)fileXioClose(marker);
    quiet_run=marker<0;
    scr_printf("N6G SCREEN TEST: %s\n",quiet_run?"QUIET":"VERBOSE");
    n6g_mute_screen(quiet_run);
  }
#endif
#ifdef N6G_TLS_ARENA
  if(rc==0 && targets>0) {
    int marker=fileXioOpen("mass0:/N6G.NEWLIB",FIO_O_RDONLY,0);
    if(marker>=0)fileXioClose(marker);
    n6g_use_arena=marker<0;
    scr_printf("TLS HEAP: %s\n",n6g_use_arena?"ARENA":"NEWLIB");
  }
#endif
  if(rc==0 && targets>0)rc=init_net(&cfg);
  if(rc==0 && targets>0)rc=live_signed_probe(nonce,&signed_time);
  if(rc==0 && targets>0) {
    rt_pass=1;
    scr_printf(USB_TEST_NAME " ROUGHTIME SIGNED UTC: PASS\n");
  }
  if(rc==0 && targets>0) {
    n6_source=0; /* pinned OPL Manager mirror first */
    scr_printf(USB_TEST_NAME " SOURCE OPL START: %s\n",n6_current_id);
    rc=verify_cover_tls(&signed_time,seeds[0][0],seeds[0][1]);
#ifdef N6G_PNG_WORKSPACE
    if(rc==0)rc=normalize_download(&normalized,&normalized_size,-565);
#else
    if(rc==0 && (!n6_png || n6_png_size==0 ||
                 probe_png_check(n6_png,n6_png_size)!=0 ||
                 n6_normalize_png(n6_png,n6_png_size,
                                  &normalized,&normalized_size)!=0))
      rc=-565;
#endif
    if(rc==-404 || rc==-565 || (rc==-714 && n6_http_rc==200)) {
      /* Fallback ONLY for source-missing or invalid PNG.
       * TLS/cert/time/network failures never silently fall back. */
      scr_printf(USB_TEST_NAME " FALLBACK PSBBN: SOURCE ERROR=%d\n",rc);
      n6_source=1;
      n6_png=NULL;n6_png_size=0;
      normalized=NULL;normalized_size=0;
      rc=verify_cover_tls(&signed_time,seeds[0][2],seeds[0][3]);
#ifdef N6G_PNG_WORKSPACE
      if(rc==0)rc=normalize_download(&normalized,&normalized_size,-566);
#else
      if(rc==0 && (!n6_png || n6_png_size==0 ||
                   probe_png_check(n6_png,n6_png_size)!=0 ||
                   n6_normalize_png(n6_png,n6_png_size,
                                    &normalized,&normalized_size)!=0))
        rc=-566;
#endif
    }
    if(rc==0) {
      if(!normalized || !normalized_size ||
         probe_png_check(normalized,normalized_size)!=0)
        rc=-720;
      else {
        verified=1;
        scr_printf(USB_TEST_NAME " REAL PNG CRC: PASS %u bytes\n",
                   (unsigned)n6_png_size);
        scr_printf(USB_TEST_NAME " 140x200 RGB: PASS %u bytes\n",
                   (unsigned)normalized_size);
        scr_printf(USB_TEST_NAME " USB .PART + READBACK START: %s\n",n6_current_id);
        rc=n6_write_usb_art(n6_current_id,normalized,normalized_size);
        if(rc==0) {
          saved=1;
          source_ok=n6_source;
          scr_printf(USB_TEST_NAME " VERIFIED SAVE: mass0:/ART/%s_COV.png\n",
                     n6_current_id);
          scr_printf("SOURCE=%s STAGED READBACK+RENAME: PASS\n",
                     n6_source==0?"OPL":"PSBBN");
        }else{
          scr_printf(USB_TEST_NAME " USB ART SAVE FAILED: code %d\n",rc);
          scr_printf("Check .PART manually; NEVER auto-overwrite it\n");
        }
      }
    }else if(rc==-404){
      not_found=1;
      scr_printf(USB_TEST_NAME " NOT FOUND in OPL or PSBBN: %s\n",n6_current_id);
    }
  }
  v12_n4a_wipe(nonce,sizeof(nonce));
  v12_n4a_wipe(seeds,sizeof(seeds));
  if(mounted){
    int unmount=fileXioUmount(MOUNT);
    if(unmount<0 && rc==0)rc=-509;
  }
#ifdef N6G_SCREEN_PROBE
  n6g_mute_screen(0);
#endif
#ifdef N6G_HEADLESS
  init_scr();
#endif
  scr_clear();scr_setCursor(0);
  scr_printf(USB_TEST_NAME " HTTPS USB ART - FINAL RESULT\n");
#ifdef N6G_HEADLESS
  scr_printf("N6G HEADLESS - DISPLAY STARTED AT SUMMARY\n");
#elif defined(N6G_SCREEN_PROBE)
  if(targets>0)scr_printf("SCREEN TEST: %s\n",quiet_run?"QUIET":"VERBOSE");
#endif
  scr_printf("DVR ISO=%d ID=%d USB MISSING=%d\n",iso_count,ids_count,missing);
  scr_printf("BATCH=%d SAVED=%d NOT_FOUND=%d\n",targets,saved,not_found);
  scr_printf("GAME=%s SOURCE=%s\n",n6_current_id,
             source_ok==0?"OPL":source_ok==1?"PSBBN":"NONE");
  scr_printf("HTTP=%d RX=%u PNG_IN=%u PNG_OUT=%u\n",n6_http_rc,
             (unsigned)n6_wire_bytes,(unsigned)n6_png_size,
             (unsigned)normalized_size);
  if(rt_pass)scr_printf("ROUGHTIME SIGNED UTC: PASS\n");
  if(verified)scr_printf("HTTPS RAW PNG + RGB 140x200: PASS\n");
  if(saved)scr_printf("USB ART .PART READBACK+RENAME: PASS\n");
  if(not_found)scr_printf("BOTH SOURCES: 404 NOT FOUND\n");
  if(targets==0 && rc==0)scr_printf("NO MISSING TARGETS / NO NETWORK\n");
  scr_printf("TCP=%d RX_CALLS=%d TX_CALLS=%d FAIL=%d\n",
             diag_tcp_connected,diag_io_rx,diag_io_tx,diag_io_fail);
  scr_printf("CERT=%d FAIL=%d TLSERR=%d\n",diag_cert_calls,
             diag_cert_failed,diag_ssl_error);
  scr_printf("HDD/PFS READONLY; ONLY USB ART NEW FILE\n");
  scr_printf("RET=%d\n",rc);
  sleep(120);
  return rc==0?0:1;
}
#endif

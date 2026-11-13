/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Experimental N4D wolfCrypt DRBG on real DESR EE (CI-build-only).
 *
 * DO NOT DISTRIBUTE THE LINKED ELF before independent GPL/AFL review.
 * No TLS, HTTP, ART, XMB/game boot, RTC or HDD/USB writes. Only a
 * single private 64-byte host CSPRNG seed read from mass:/N4D.SEED.
 * Independent repetitions MUST use fresh input, not enforced on reboot.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <tamtypes.h>
#include <debug.h>
#include <kernel.h>
#include <iopcontrol.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <libcdvd.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/random.h>
#include "v12_n4b_seed_gate.h"

#if !defined(WC_RNG_SEED_CB) || !defined(HAVE_HASHDRBG)
#error "N4D requires a separately built wolfSSL with WC_RNG_SEED_CB and HASHDRBG"
#endif
#if defined(WOLFSSL_GENSEED_FORTEST) || defined(WC_NO_HASHDRBG)
#error "N4D rejects test RNG and no-DRBG modes"
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
IRX(bdm);
IRX(bdmfs_fatfs);
IRX(usbd_mini);
IRX(usbmass_bd_mini);

static const char hdd_args[]="-o\0" "4\0" "-n\0" "20";
static const char pfs_args[]="-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char check_hdd_args[(sizeof(hdd_args)==11)?1:-1];
typedef char check_pfs_args[(sizeof(pfs_args)==17)?1:-1];

static v12_n4b_seed_gate gate={0};

static int load_irx(const char *name,void *ptr,unsigned size,
                    const char *args,int arg_len) {
  int rv=0;
  int id=SifExecModuleBuffer(ptr,size,arg_len,(void *)args,&rv);
  scr_printf("IRX %-8s id=%d rv=%d\n",name,id,rv);
  return id<0?id:(rv<0?rv:0);
}
#define LOAD(name) do { \
  rc=load_irx(#name,name##_irx,size_##name##_irx,NULL,0); \
  if(rc<0) return -601; \
} while(0)

static int init_io(void) {
  int rc;
  u32 notice=0;
  scr_printf("STAGE 1: IOP reset\n");
  while(!SifIopReset("",0)) {}
  while(!SifIopSync()) {}
  sceSifInitRpc(0);
  sbv_patch_enable_lmb();
  sbv_patch_disable_prefix_check();
  sbv_patch_fileio();
  scr_printf("STAGE 2: known-good DESR IO modules\n");
  LOAD(iomanX);
  LOAD(fileXio);
  LOAD(dev9_ns);
  LOAD(ps2atad);
  rc=load_irx("ps2hdd",ps2hdd_irx,size_ps2hdd_irx,
              hdd_args,sizeof(hdd_args));
  if(rc<0) return -601;
  rc=load_irx("ps2fs",ps2fs_irx,size_ps2fs_irx,
              pfs_args,sizeof(pfs_args));
  if(rc<0) return -601;
  if(fileXioInit()<0) return -601;
  if(sceCdInit(SCECdINoD)<=0) return -601;
  if(sceCdNoticeGameStart(0,&notice)<0) return -601;
  usleep(500000);
  scr_printf("STAGE 3: USB FAT modules\n");
  LOAD(bdm);
  LOAD(bdmfs_fatfs);
  LOAD(usbd_mini);
  LOAD(usbmass_bd_mini);
  return 0;
}
#undef LOAD

/* Fail closed: exactly 64 binary bytes, never a text representation.
 * No explicit storage writes, and no crypto bytes printed on screen.
 */
static int load_private_seed(uint8_t output[64]) {
  int fd=-1,n,got=0,extra,closed;
  uint8_t more=0,orv=0;
  for(int i=0;i<16;i++) {
    fd=fileXioOpen("mass0:/N4D.SEED",FIO_O_RDONLY,0);
    if(fd<0) fd=fileXioOpen("mass:/N4D.SEED",FIO_O_RDONLY,0);
    if(fd>=0) break;
    usleep(500000);
  }
  if(fd<0) return -603;
  while(got<64) {
    n=fileXioRead(fd,output+got,64-got);
    if(n<=0) break;
    got+=n;
  }
  extra=fileXioRead(fd,&more,1);
  closed=fileXioClose(fd);
  if(closed<0 || extra<0) return -604;
  if(got!=64 || extra!=0) return -604;
  for(int i=0;i<64;i++) orv|=output[i];
  if(!orv || memcmp(output,output+32,32)==0) return -604;
  scr_printf("USB SEED: 64B INPUT ACCEPTED\n");
  return 0;
}
static int seed_cb(OS_Seed *unused,byte *dst,word32 count) {
  (void)unused;
  return v12_n4b_seed_take(&gate,dst,(size_t)count);
}
static int wolfcrypt_probe(void) {
  WC_RNG rng;
  byte first[32]={0},second[32]={0};
  int rc,ret=0;
  scr_printf("STAGE 5: wolfSSL Init + DRBG callback\n");
  rc=wolfSSL_Init();
  if(rc!=WOLFSSL_SUCCESS) return -606;
  if(wc_SetSeed_Cb(seed_cb)!=0) {ret=-607;goto finish;}
  rc=wc_InitRng(&rng);
  if(rc!=0) {ret=-608;goto finish;}
  if(gate.state!=2U) {ret=-608;goto free_first;}
  scr_printf("DRBG INIT: PASS (seed consumed)\n");
  if(wc_RNG_GenerateBlock(&rng,first,sizeof(first))!=0 ||
     wc_RNG_GenerateBlock(&rng,second,sizeof(second))!=0)
    ret=-609;
  else if(memcmp(first,second,sizeof(first))==0)
    ret=-611;
  else
    scr_printf("DRBG 2 BLOCKS: PASS (not printed)\n");
free_first:
  wc_FreeRng(&rng);
  /* Attempt another RNG from consumed seed; MUST fail. If it
   * succeeds, this diagnostic MUST NEVER report PASS.
   */
  if(ret==0) {
    rc=wc_InitRng(&rng);
    if(rc==0) {wc_FreeRng(&rng);ret=-610;}
    else scr_printf("REUSE BLOCKED: PASS\n");
  }
finish:
  v12_n4b_seed_clear(&gate);
  wc_SetSeed_Cb(NULL);
  wolfSSL_Cleanup();
  /* volatile wipe via gate's public utility not available for outputs:
   * avoid exposing the two generated blocks outside the function.
   */
  {
    volatile byte *a=first,*b=second;
    for(unsigned i=0;i<sizeof(first);i++) {a[i]=0;b[i]=0;}
  }
  return ret;
}
int main(void) {
  uint8_t seed[64]={0};
  int rc;
  init_scr();
  scr_setCursor(0);
  scr_printf("N4D WOLFCRYPT RNG BOOT - STAGE 0\n");
  scr_printf("READ ONLY: NO TLS / COVERS / HDD WRITES\n");
  sleep(1);
  rc=init_io();
  if(rc==0) {
    scr_printf("STAGE 4: USB N4D.SEED\n");
    rc=load_private_seed(seed);
  }
  if(rc==0 && v12_n4b_seed_provision(&gate,seed,sizeof(seed))!=0)
    rc=-605;
  {volatile uint8_t *p=seed;for(unsigned i=0;i<sizeof(seed);i++)p[i]=0;}
  if(rc==0) rc=wolfcrypt_probe();
  v12_n4b_seed_clear(&gate);
  scr_clear();
  scr_printf("N4D WOLFCRYPT RNG - FINAL RESULT\n");
  if(rc==0) {
    scr_printf("DRBG INITIALIZATION: PASS\n");
    scr_printf("TWO RANDOM BLOCKS: PASS\n");
    scr_printf("REUSED SEED: REJECTED\n");
  } else scr_printf("FAIL code=%d\n",rc);
  scr_printf("NO TLS / HTTPS / CERT / CLOCK\n");
  scr_printf("NO HDD / USB / RTC / PFS WRITES\n");
  scr_printf("NEVER REUSE N4D.SEED\n");
  scr_printf("RET=%d (screen 120s)\n",rc);
  sleep(120);
  return rc==0?0:1;
}

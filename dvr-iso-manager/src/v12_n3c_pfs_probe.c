/* PSX DVR ISO MANAGER 1.2 — N3C PFS dummy-record test.
 *
 * Two SEPARATE standalone modes:
 *  - V12_N3C_WRITE_ONCE: creates exactly two NEW 64-byte dummy slots on
 *    PP.DVRISO12 and performs PFS sync + unmount/remount RO + verification.
 *  - V12_N3C_VERIFY_RO: mounts PP.DVRISO12 READ-ONLY and verifies existing
 *    slots after a normal console restart. Does not write ANY disk data.
 *
 * NO secrets, network, ART, DVR ISO access, RTC updates, APA creation,
 * partition format, file deletion/rename or XMB/BOOT configuration.
 * Normal restarts test visibility after remount, NOT power-loss atomicity.
 */
#include "v12_rt_journal.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#if defined(V12_N3C_WRITE_ONCE) == defined(V12_N3C_VERIFY_RO)
#error "Select exactly one N3C PFS diagnostic mode"
#endif

#define N3C_PARTITION "hdd0:PP.DVRISO12"
#define N3C_MOUNT "pfs1:"
#define N3C_SLOT0 "pfs1:/PDIM_N3C_TEST_SLOT0.BIN"
#define N3C_SLOT1 "pfs1:/PDIM_N3C_TEST_SLOT1.BIN"

#define IRX(name) \
  extern unsigned char name##_irx[] __attribute__((aligned(16))); \
  extern uint32_t size_##name##_irx

IRX(iomanX);
IRX(fileXio);
IRX(dev9_ns);
IRX(ps2atad);
IRX(ps2hdd);
IRX(ps2fs);

static const char hdd_arguments[]="-o\0" "4\0" "-n\0" "20";
static const char pfs_arguments[]="-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char n3c_hdd_len_check[(sizeof(hdd_arguments)==11)?1:-1];
typedef char n3c_pfs_len_check[(sizeof(pfs_arguments)==17)?1:-1];

static int mounted;
static int load(const char *label,void *data,unsigned size,
                const char *arguments,int argc_bytes) {
  int rv=-999;
  int id=SifExecModuleBuffer(data,size,argc_bytes,(void *)arguments,&rv);
  scr_printf("IRX %-12s id=%d rv=%d\n",label,id,rv);
  return id<0?id:rv<0?rv:0;
}

static int drivers(void) {
  int r;
  u32 notice_arg=0;
  scr_printf("Reset IOP, initialize dedicated HDD/PFS modules\n");
  while(!SifIopReset("",0)) {}
  while(!SifIopSync()) {}
  sceSifInitRpc(0);
  sbv_patch_enable_lmb();
  sbv_patch_disable_prefix_check();
  sbv_patch_fileio();

#define LOAD(name) do { \
  r=load(#name,name##_irx,size_##name##_irx,NULL,0); \
  if(r<0) return r; \
} while(0)
  LOAD(iomanX);
  LOAD(fileXio);
  /* Tested DESR-7000 DEV9 provider — not the generic PS2DEV9 IRX. */
  LOAD(dev9_ns);
  LOAD(ps2atad);
  r=load("ps2hdd",ps2hdd_irx,size_ps2hdd_irx,
         hdd_arguments,sizeof(hdd_arguments));
  if(r<0) return r;
  r=load("ps2fs",ps2fs_irx,size_ps2fs_irx,
         pfs_arguments,sizeof(pfs_arguments));
  if(r<0) return r;
#undef LOAD
  r=fileXioInit();
  scr_printf("fileXioInit=%d\n",r);
  if(r<0) return r;
  r=sceCdInit(SCECdINoD);
  scr_printf("sceCdInit=%d\n",r);
  if(r<=0) return -401;
  r=sceCdNoticeGameStart(0,&notice_arg);
  scr_printf("sceCdNoticeGameStart=%d\n",r);
  if(r<0) return -402;
  usleep(500000);
  return 0;
}
static int mount_partition(int mode) {
  int fd,r;
  if(mounted) return -403;
  fd=fileXioDopen(N3C_MOUNT "/");
  if(fd>=0) {
    fileXioDclose(fd);
    scr_printf("PFS1 already occupied; refusing any mount\n");
    return -404;
  }
  r=fileXioMount(N3C_MOUNT,N3C_PARTITION,mode);
  scr_printf("Mount %s %s -> %d\n",N3C_PARTITION,
             mode==FIO_MT_RDONLY?"RO":"RW",r);
  if(r<0) return r;
  mounted=1;
  fd=fileXioDopen(N3C_MOUNT "/");
  if(fd<0) return fd;
  fileXioDclose(fd);
  return 0;
}
static int unmount_partition(void) {
  int r;
  if(!mounted) return 0;
  r=fileXioUmount(N3C_MOUNT);
  scr_printf("Unmount PFS1=%d\n",r);
  if(r>=0) mounted=0;
  return r;
}
static int read_slot(const char *path,uint8_t out[V12_RT_RECORD_BYTES]) {
  int fd,rc,size=0;
  uint8_t tailbyte;
  fd=fileXioOpen(path,FIO_O_RDONLY,0);
  if(fd<0) return fd;
  while(size<(int)V12_RT_RECORD_BYTES) {
    rc=fileXioRead(fd,out+size,(int)V12_RT_RECORD_BYTES-size);
    if(rc<=0) {fileXioClose(fd);return -405;}
    size+=rc;
  }
  rc=fileXioRead(fd,&tailbyte,1);
  if(fileXioClose(fd)<0 || rc!=0) return -406;
  return 0;
}

static int verify_slots(void) {
  static uint8_t slots[2][V12_RT_RECORD_BYTES];
  static const uint8_t dummy_id[16]={
    'P','D','I','M','N','3','C','0','N','L','Y','T','E','S','T','1'
  };
  v12_rt_journal_state selected;
  unsigned active=999;
  int a=read_slot(N3C_SLOT0,slots[0]);
  int c=read_slot(N3C_SLOT1,slots[1]);
  scr_printf("Read SLOT0=%d  SLOT1=%d\n",a,c);
  if(a<0 || c<0) {
    scr_printf("Expected two 64-byte records; no modifications\n");
    return -407;
  }
  if(v12_rt_journal_select((const uint8_t (*)[V12_RT_RECORD_BYTES])slots,
                           &selected,&active)<0) {
    scr_printf("Journal CRC / order / identity INVALID\n");
    return -408;
  }
  if(memcmp(selected.reservation.key_id,dummy_id,16) ||
     active!=1U || selected.sequence!=2U ||
     selected.reservation.last_reserved!=1001U) {
    scr_printf("Journal content NOT expected test generation\n");
    return -409;
  }
  scr_printf("JOURNAL VERIFIED: SLOT%u GEN=%lu COUNT=%lu\n",
             active,(unsigned long)selected.sequence,
             (unsigned long)selected.reservation.last_reserved);
  scr_printf("ONLY synthetic ID and non-secret counters.\n");
  return 0;
}

#ifdef V12_N3C_WRITE_ONCE
/* Create a new file with O_EXCL. No existing file may be overwritten.
 * Every error leaves the evidence on disk untouched for diagnosis.
 */
static int write_new_slot(const char *path,unsigned sequence,
                          unsigned counter) {
  static const uint8_t dummy_id[16]={
    'P','D','I','M','N','3','C','0','N','L','Y','T','E','S','T','1'
  };
  uint8_t record[V12_RT_RECORD_BYTES];
  v12_rt_journal_state state;
  int fd,rc,close_rc;
  memset(&state,0,sizeof(state));
  memcpy(state.reservation.key_id,dummy_id,16);
  state.sequence=sequence;
  state.reservation.last_reserved=counter;
  if(v12_rt_journal_encode(&state,record)<0) return -410;
  fd=fileXioOpen(path,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_EXCL,0666);
  scr_printf("Create %s fd=%d\n",sequence==1?"SLOT0":"SLOT1",fd);
  if(fd<0) return fd;
  rc=fileXioWrite(fd,record,(int)sizeof(record));
  close_rc=fileXioClose(fd);
  if(rc!=(int)sizeof(record) || close_rc<0) return -411;
  /* PFS source confirms this flushes dirty PFS cache; physical power-fail
   * durability is still NOT proven. No simulated fsync guarantee. */
  rc=fileXioSync(N3C_MOUNT,0);
  scr_printf("fileXioSync=%d\n",rc);
  return rc<0?rc:0;
}
static int write_once(void) {
  iox_stat_t st;
  int a,c,r;
  a=fileXioGetStat(N3C_SLOT0,&st);
  c=fileXioGetStat(N3C_SLOT1,&st);
  scr_printf("Preflight SLOT0 stat=%d SLOT1 stat=%d\n",a,c);
  /* Expected negative ENOENT for BOTH, no fallback if unexpected. */
  if(a!=-ENOENT || c!=-ENOENT) {
    scr_printf("Already initialized / unexpected stat: REFUSING WRITES\n");
    return -412;
  }
  r=unmount_partition();
  if(r<0) return r;
  r=mount_partition(FIO_MT_RDWR);
  if(r<0) return r;
  r=write_new_slot(N3C_SLOT0,1U,1000U);
  if(r<0) return r;
  r=write_new_slot(N3C_SLOT1,2U,1001U);
  if(r<0) return r;
  r=fileXioSync(N3C_MOUNT,0);
  scr_printf("Final PFS sync=%d\n",r);
  if(r<0) return r;
  r=unmount_partition();
  if(r<0) return r;
  r=mount_partition(FIO_MT_RDONLY);
  if(r<0) return r;
  return verify_slots();
}
#endif

int main(void) {
  int r,umount_result;
  init_scr();
#ifdef V12_N3C_WRITE_ONCE
  scr_printf("PSX DVR ISO MANAGER 1.2 - N3C PFS WRITE ONCE\n");
  scr_printf("ALLOWED: two new dummy 64-byte files on PP.DVRISO12\n");
#else
  scr_printf("PSX DVR ISO MANAGER 1.2 - N3C PFS VERIFY RO\n");
  scr_printf("READ ONLY; NO WRITES OR FILE CREATION\n");
#endif
  scr_printf("NO REAL SEED / NO NONCE / NO SSL / NO DVR ISO WRITES\n");
  scr_printf("Target partition: %s ONLY\n",N3C_PARTITION);
  scr_printf("---------------------------------------------\n");
  r=drivers();
  if(r==0) r=mount_partition(FIO_MT_RDONLY);
  if(r==0) {
#ifdef V12_N3C_WRITE_ONCE
    r=write_once();
#else
    r=verify_slots();
#endif
  }
  umount_result=unmount_partition();
  if(r==0 && umount_result<0) r=umount_result;
  scr_printf("---------------------------------------------\n");
  scr_printf("N3C PFS RESULT: %s (code %d)\n",r==0?"PASS":"FAIL",r);
  scr_printf("NO certified crash durability; no secure nonce yet.\n");
  scr_printf("Screen holds 120 sec. Photograph this result.\n");
  sleep(120);
  return r<0?1:0;
}

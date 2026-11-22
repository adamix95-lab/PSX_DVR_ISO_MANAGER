/* PDIM 1.2 HDD installer for Sony PSX DESR-7000 / PP.DVRISO.
 * NO writes at startup. Every disk write requires TWO confirmations.
 * Do not format or delete an existing partition; preserve ART and CNF.
 */
#include "sg_installer.h"
#include <libcdvd.h>
#include <debug.h>
#include <libpad.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <delaythread.h>
#include <kernel.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define MODULE(name) \
 extern unsigned char name##_irx[] __attribute__((aligned(16))); \
 extern unsigned int size_##name##_irx
MODULE(iomanX);
MODULE(fileXio);
MODULE(dev9_ns);
MODULE(ps2atad);
MODULE(ps2hdd_psx1);
MODULE(ps2fs);
MODULE(bdm);
MODULE(bdmfs_fatfs);
MODULE(usbd);
MODULE(usbmass_bd);
MODULE(sio2man);
MODULE(padman);
MODULE(dvrdrv);
MODULE(dvrfile);

static unsigned char pad_buffer[256] __attribute__((aligned(64)));
static unsigned char pad_init_stack[16384] __attribute__((aligned(64)));
static volatile int pad_init_done;
static volatile int pad_init_stage;
static volatile int pad_init_rc;
extern void *_gp;
static const char hdd_args[]="-o\0" "4\0" "-n\0" "20";
static const char pfs_args[]="-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char hdd_args_size[(sizeof(hdd_args)==11)?1:-1];
typedef char pfs_args_size[(sizeof(pfs_args)==17)?1:-1];

/* HDD 1.0 NETFIX's proven strategy: reset IOP before installing our
 * driver set. Reusing the launcher's IO/File_Manager module by name can
 * misidentify the stock IOMAN as the newer iomanX export library.
 * fileXio then fails with IOP dependency error -200. Never allow any
 * disk write unless this initialization completes successfully.
 */
static int reset_iop_bounded(void) {
    int started=0,synced=0;
    scr_printf("STAGE IOP RESET: CLEAN MODULE SET - NO HDD WRITE\n");
    for(int i=0;i<200;i++) {
        if(SifIopReset("",0)) {started=1;break;}
        DelayThread(50000);
    }
    if(!started) {
        scr_printf("IOP RESET REQUEST TIMEOUT\n");
        return -101;
    }
    for(int i=0;i<400;i++) {
        if(SifIopSync()) {synced=1;break;}
        DelayThread(50000);
    }
    if(!synced) {
        scr_printf("IOP SYNC TIMEOUT\n");
        return -102;
    }
    sceSifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();
    return 0;
}
static int load(const char *name,void *data,
                unsigned int size,const char *args,unsigned args_size) {
    int rv=0;
    int id=SifExecModuleBuffer(data,size,args_size,(void*)args,&rv);
    scr_printf("LOAD %-12s id=%d rv=%d\n",name,id,rv);
    /* Pass the real IOP error (e.g. dependency -200), no generic -100. */
    return id<0?id:(rv<0?rv:0);
}
#define LOAD(n) do { \
  int rc_load=load(#n,n##_irx,size_##n##_irx,NULL,0); \
  if(rc_load<0) return rc_load; \
} while(0)
static int setup_modules(void) {
    int rc=reset_iop_bounded();
    if(rc<0)return rc;
    /* Exactly like the working PDIM HDD 1.0 startup: install iomanX
     * exports before fileXio, then a single proven DEV9 provider. */
    LOAD(iomanX);
    LOAD(fileXio);
    LOAD(dev9_ns);
    LOAD(ps2atad);
    rc=load("ps2hdd",ps2hdd_psx1_irx,
            size_ps2hdd_psx1_irx,hdd_args,sizeof(hdd_args));
    if(rc<0)return rc;
    rc=load("ps2fs",ps2fs_irx,size_ps2fs_irx,
            pfs_args,sizeof(pfs_args));
    if(rc<0)return rc;
    rc=fileXioInit();
    scr_printf("fileXioInit=%d\n",rc);
    if(rc<0)return rc;
    LOAD(bdm);
    LOAD(bdmfs_fatfs);
    LOAD(usbd);
    LOAD(usbmass_bd);
    LOAD(sio2man);
    LOAD(padman);
    /* The same CD/DVD and DVR initialization as the tested HDD Manager. */
    if(sceCdInit(SCECdINoD)<=0) return -201;
    u32 notice=0;
    if(sceCdNoticeGameStart(0,&notice)<0) return -202;
    DelayThread(500000);
    LOAD(dvrdrv);
    LOAD(dvrfile);
    return 0;
}
#undef LOAD
/* On the DESR, PADMAN RPC can hang during startup from wLE-R3Z.
 * Keep it in a bounded EE worker. On timeout, NEVER enable HDD writes.
 */
static void pad_init_thread(void *unused) {
    (void)unused;
    pad_init_stage=1;
    int rc=padInit(0);
    pad_init_stage=2;
    if(rc>=0)rc=padPortOpen(0,0,pad_buffer)>0?0:-EIO;
    pad_init_rc=rc;
    pad_init_done=1;
    ExitDeleteThread();
}
static int pad_initialize_bounded(void) {
    ee_thread_t t;
    ee_thread_status_t parent;
    memset(&t,0,sizeof(t));
    pad_init_done=0;
    pad_init_stage=0;
    pad_init_rc=-EIO;
    if(ReferThreadStatus(GetThreadId(),&parent)<0 || parent.current_priority>=127)
        return -EIO;
    t.func=pad_init_thread;
    t.stack=pad_init_stack;
    t.stack_size=sizeof(pad_init_stack);
    t.gp_reg=&_gp;
    t.initial_priority=parent.current_priority+1;
    int id=CreateThread(&t);
    if(id<0)return id;
    if(StartThread(id,NULL)<0){DeleteThread(id);return -EIO;}
    for(int i=0;i<1000;i++){
        if(pad_init_done)return pad_init_rc;
        DelayThread(10000);
    }
    /* Do not terminate a thread which may own an active IOP RPC buffer. */
    return -ETIMEDOUT;
}
/* Fixed console coordinates avoid cumulative indents and line wraps. */

static int selected=0;
static int lang_en=0;
static void display(void) {
  scr_setbgcolor(0x000000);
  scr_setfontcolor(0xffffff);
  scr_clear();
  scr_setXY(2,2);
  scr_printf("PDIM XMB GAME INSTALLER 0.1 - HDD DVR\n");
  scr_setXY(2,4);
  scr_printf("Games: %d   Page %d/%d  Lang: %s\n",
    sg_count,sg_count?selected/12+1:0,sg_count?(sg_count+11)/12:0,
    lang_en?"ENGLISH":"SYSTEM");
  if(sg_count>0) {
    const sg_game *g=&sg_games[selected];
    scr_setXY(2,7);
    scr_printf("ISO: %.64s\n",g->filename);
    scr_setXY(2,9);
    scr_printf("GAME ID: %s\n",g->game_id[0]?g->game_id:"NOT IN INDEX");
    scr_setXY(2,10);
    scr_printf("TARGET: %s\n",g->partition[0]?g->partition:"NOT AVAILABLE");
    scr_setXY(2,11);
    scr_printf("COVER: %s\n",g->cover_available?"PDIM ART FOUND":"NO PNG / USE PDIM HTTPS FIRST");
  } else {
    scr_setXY(2,7);
    scr_printf("NO DVR ISO GAMES FOUND. CHECK DVR STORAGE.\n");
  }
  scr_setXY(2,14);
  scr_printf("UP / DOWN : SELECT GAME     LEFT / RIGHT : PAGE\n");
  scr_setXY(2,16);
  scr_printf("SELECT    : SYSTEM / ENGLISH\n");
  scr_setXY(2,18);
  scr_printf("SQUARE    : INSPECT TARGET (READ ONLY)\n");
  scr_setXY(2,19);
  scr_printf("X         : CREATE PARTITION + COPY FILES\n");
  scr_setXY(2,20);
  scr_printf("TRIANGLE  : REGISTER XMB (AFTER CORE PASS)\n");
  scr_setXY(2,22);
  scr_printf("CIRCLE    : EXIT\n");
  scr_setXY(2,25);
  scr_printf("WARNING: APA WRITES. VERIFY FULL SECTOR BACKUP.");
}
static unsigned buttons_prev=0xffff;
static unsigned poll_keys(void) {
  struct padButtonStatus pad;
  unsigned pressed=0;
  if(padRead(0,0,&pad)>0) {
    unsigned held=0xffffu ^ pad.btns;
    pressed=held & ~buttons_prev;
    buttons_prev=held;
    return pressed | (held<<16);
  }
  DelayThread(10000);
  return 0;
}
static void clear_keys(void) {
  unsigned held=0xffffu;
  for(int i=0;i<250;i++) {
    struct padButtonStatus p;
    if(padRead(0,0,&p)>0) {
      held=0xffffu ^ p.btns;
      buttons_prev=held;
      if(!held) return;
    }
    DelayThread(20000);
  }
}
static void pause_result(int rc) {
  scr_printf("\nSTATUS: %s (%d)\n",rc>=0?"PASS":"FAIL",rc);
  if(rc<0) scr_printf("REASON: %s\n",sg_error(rc));
  scr_printf("X / O: RETURN TO GAME LIST\n");
  clear_keys();
  for(;;) if(poll_keys()&(PAD_CROSS|PAD_CIRCLE)) break;
}
static int confirm_apa(const sg_game *g,int is_xmb) {
  scr_setbgcolor(0x000000);
  scr_setfontcolor(0xff4444);
  scr_clear();
  scr_setXY(2,3);
  scr_printf("WARNING: APA DISK WRITE! FULL SECTOR BACKUP FIRST\n");
  scr_setXY(2,6);
  scr_printf("Selected ISO: %.60s\n",g->filename);
  scr_setXY(2,8);
  scr_printf("Target partition: %s\n",g->partition);
  scr_setXY(2,10);
  scr_printf(is_xmb?"ACTION: WRITE XMB HEADER (TARGET ONLY)\n":
                     "ACTION: CREATE NEW 128 MiB PARTITION + PFS FILES\n");
  scr_setXY(2,13);
  scr_printf("MODIFYING APA MAY DAMAGE SONY DVR HDD STRUCTURE.\n");
  scr_setXY(2,15);
  scr_printf("X: CONTINUE     O: CANCEL\n");
  clear_keys();
  for(;;) {
    unsigned keys=poll_keys()&0xffffu;
    if(keys&PAD_CIRCLE) return 0;
    if(keys&PAD_CROSS) break;
  }
  scr_setXY(2,19);
  scr_printf("FINAL CONFIRM: HOLD L1 + R1 AND PRESS X\n");
  scr_setXY(2,21);
  scr_printf("O: ABORT WITHOUT ANY NEW WRITE\n");
  clear_keys();
  for(;;) {
    unsigned keys=poll_keys();
    if((keys&0xffffu)&PAD_CIRCLE) return 0;
    if(((keys&0xffffu)&PAD_CROSS) &&
       ((keys>>16)&(PAD_L1|PAD_R1))==(PAD_L1|PAD_R1)) return 1;
  }
}
int main(void) {
  init_scr();
  scr_setCursor(0);
  scr_printf("PDIM XMB GAME INSTALLER 0.1\n");
  scr_printf("Initializing PSX HDD, USB, DVR modules (NO WRITES)...\n");
  int rc=setup_modules();
  if(rc<0) {
    scr_printf("MODULE FAILURE %d. NO DISK WRITE. REBOOT.\n",rc);
    for(;;) DelayThread(1000000);
  }
  rc=pad_initialize_bounded();
  if(rc<0) {
    scr_printf("PAD ERROR %d. NO DISK WRITE. REBOOT.\n",rc);
    for(;;) DelayThread(1000000);
  }
  buttons_prev=0xffff;
  rc=sg_scan();
  if(rc<0) {
    scr_printf("DVR / ART SCAN FAILED: %d (%s)\n",rc,sg_error(rc));
    scr_printf("No writes performed. Reboot or exit with O.\n");
    clear_keys();
    for(;;) if(poll_keys()&PAD_CIRCLE) break;
    padPortClose(0,0);
    padEnd();
    return 2;
  }
  for(;;) {
    display();
    clear_keys();
    unsigned keys=0;
    do {keys=poll_keys()&0xffffu;} while(!keys);
    if(keys&PAD_CIRCLE) break;
    if(keys&PAD_SELECT){lang_en=!lang_en;continue;}
    if(keys&PAD_UP){if(sg_count)selected=(selected+sg_count-1)%sg_count;continue;}
    if(keys&PAD_DOWN){if(sg_count)selected=(selected+1)%sg_count;continue;}
    if(keys&PAD_LEFT){if(sg_count)selected=selected>=12?selected-12:0;continue;}
    if(keys&PAD_RIGHT){if(sg_count)selected=(selected+12)<sg_count?selected+12:sg_count-1;continue;}
    if(!sg_count)continue;
    const sg_game *g=&sg_games[selected];
    if(keys&PAD_SQUARE){
      scr_clear();
      scr_printf("READ-ONLY PREFLIGHT: %s\n",g->partition);
      pause_result(sg_preflight(g,lang_en));
    } else if(keys&PAD_CROSS){
      if(!g->cover_available||!g->game_id[0]) {pause_result(-30);continue;}
      if(!confirm_apa(g,0))continue;
      scr_clear();scr_printf("INSTALL CORE: %s\n",g->partition);
      pause_result(sg_install_core(g,lang_en));
    } else if(keys&PAD_TRIANGLE) {
      if(!confirm_apa(g,1))continue;
      scr_clear();scr_printf("XMB REGISTER: %s\n",g->partition);
      pause_result(sg_register_xmb(g,lang_en));
    }
  }
  padPortClose(0,0);
  padEnd();
  scr_clear();
  return 0;
}

/* PDIM 1.2 HDD installer for Sony PSX DESR-7000 / PP.DVRISO.
 * NO writes at startup. Every disk write requires TWO confirmations.
 * Do not format or delete an existing partition; preserve ART and CNF.
 */
#include "pdim12_installer.h"
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

static unsigned char pad_buffer[256] __attribute__((aligned(64)));
static unsigned char pad_init_stack[16384] __attribute__((aligned(64)));
static volatile int pad_init_done;
static volatile int pad_init_stage;
static volatile int pad_init_rc;
extern void *_gp;
static uint32_t buttons_prev;
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
static void screen(void){
    scr_setbgcolor(0x08080C);
    scr_setfontcolor(0xFFFFFF);
    scr_clear();
    scr_setXY(5,2);
    scr_printf("PSX DVR ISO MANAGER 1.2 HDD");
    scr_setXY(16,3);
    scr_printf("INSTALLER");
    scr_setXY(2,5);
    scr_printf("----------------------------------");
    scr_setXY(2,7);
    scr_printf("INSTALL TARGET: PP.DVRISO");
}
static void red_warning(int final_stage) {
    /* PS2SDK debug bgcolor stores red in the low 8-bit component. */
    scr_setbgcolor(0x000060);
    scr_setfontcolor(0xFFFFFF);
    scr_clear();
    scr_setXY(5,3);
    scr_printf("WARNING: HDD WRITE OPERATION");
    scr_setXY(3,6);
    scr_printf("PARTITION CHANGES MAY AFFECT");
    scr_setXY(3,7);
    scr_printf("THE ENTIRE PSX DESR HDD.");
    scr_setXY(3,10);
    scr_printf("A FULL, RESTORABLE SECTOR BACKUP");
    scr_setXY(3,11);
    scr_printf("OF THE HDD IS STRONGLY RECOMMENDED.");
    if(final_stage) {
        scr_setXY(3,16);
        scr_printf("FINAL CONFIRMATION:");
        scr_setXY(3,18);
        scr_printf("HOLD L1 + R1 AND PRESS X");
    } else {
        scr_setXY(3,18);
        scr_printf("X = CONTINUE");
    }
    scr_setXY(3,20);
    scr_printf("O = CANCEL");
}
static unsigned pad_pressed(unsigned *held_out) {
    struct padButtonStatus pad;
    int st=padGetState(0,0);
    if(st!=PAD_STATE_STABLE && st!=PAD_STATE_FINDCTP1) {
        if(held_out)*held_out=0;
        return 0;
    }
    if(padRead(0,0,&pad)==0) return 0;
    unsigned buttons=(~(unsigned)pad.btns)&0xffffu;
    unsigned pressed=buttons & ~buttons_prev;
    buttons_prev=buttons;
    if(held_out)*held_out=buttons;
    return pressed;
}
static unsigned read_action(void){
    for(;;){
        unsigned held=0, pressed=pad_pressed(&held);
        if(pressed){return (held<<16)|pressed;}
        DelayThread(30000);
    }
}
static void release_all_buttons(void){
    unsigned held;
    do { (void)pad_pressed(&held); DelayThread(30000); }while(held);
}
static int confirm_danger(void) {
    red_warning(0);
    release_all_buttons();
    for(;;){
        unsigned bits=read_action();
        if(bits&PAD_CIRCLE)return 0;
        if(bits&PAD_CROSS)break;
    }
    red_warning(1);
    release_all_buttons();
    for(;;){
        unsigned bits=read_action();
        unsigned held=bits>>16, pressed=bits&0xffffu;
        if(pressed&PAD_CIRCLE)return 0;
        if((pressed&PAD_CROSS) && (held&(PAD_L1|PAD_R1))==(PAD_L1|PAD_R1))
            return 1;
    }
}
static void pause_result(int rc, const pdim12_state *st) {
    scr_printf("\nSTATUS: %s (%d)\n",rc==0?"PASS":"FAIL",rc);
    if(rc<0)scr_printf("REASON: %s\n",pdim12_error(rc));
    if(st)scr_printf("FILES TOTAL %d COPIED %d KEPT %d VERIFIED %d\n",
                    st->files_total,st->files_copied,
                    st->files_existing,st->files_verified);
    if(st && st->net_config_status) {
        const char *net=st->net_config_status==PDIM12_NET_COPIED?"COPIED":
                        st->net_config_status==PDIM12_NET_KEPT?"KEPT":
                        "MISSING - HTTPS NEEDS CONFIG";
        scr_printf("V12_NET.CNF: %s\n",net);
    }
    scr_printf("\nX OR O: RETURN TO MENU\n");
    release_all_buttons();
    for(;;){unsigned bits=read_action();if(bits&(PAD_CROSS|PAD_CIRCLE))break;}
}
static void show_menu(const pdim12_state *st,int probe_rc) {
    screen();
    scr_setXY(2,9);
    if(probe_rc<0) {
        scr_printf("HDD PREFLIGHT FAIL: %d\n",probe_rc);
        scr_printf("%s\n",pdim12_error(probe_rc));
        scr_printf("NO HDD WRITES PERMITTED.");
    } else {
        scr_printf("PP.DVRISO: %s\n",st->exists?"EXISTS / PFS OK":"NOT FOUND");
        if(st->exists) {
            scr_setXY(2,10);
            scr_printf("START LBA %lu / %lu SECTORS",
                (unsigned long)st->start,(unsigned long)st->sectors);
        }
        scr_setXY(2,13);
        scr_printf("X: INSTALL CORE (79 FILES)");
        scr_setXY(2,15);
        scr_printf("TRIANGLE: INSTALL XMB (5 FILES)");
        scr_setXY(2,17);
        scr_printf("SQUARE: VERIFY CORE (READ ONLY)");
        scr_setXY(2,19);
        scr_printf("SELECT: INSPECT XMB (READ ONLY)");
    }
    scr_setXY(2,22);
    scr_printf("O: EXIT");
    scr_setXY(2,25);
    scr_printf("FULL HDD SECTOR BACKUP FIRST.");
}
int main(void) {
    init_scr();
    /* PDIM HDD 1.0 fix: disable the default PS2SDK block cursor.
     * Otherwise a white rectangle appears beside/after console text. */
    scr_setCursor(0);
    screen();
    scr_printf("LOADING MODULES - NO HDD WRITES\n");
    int rc=setup_modules();
    if(rc<0){
        scr_printf("\nSTOP: IOP MODULES ERROR %d\n",rc);
        scr_printf("REBOOT REQUIRED. NO DISK WRITES.\n");
        for(;;)DelayThread(1000000);
    }
    /* The controller must initialize within 10s, else no disk actions. */
    int pad_rc=pad_initialize_bounded();
    if(pad_rc<0) {
        scr_printf("\nSTOP: GAMEPAD FAIL %d STAGE %d. NO HDD WRITE.\n",
                   pad_rc,pad_init_stage);
        for(;;)DelayThread(1000000);
    }
    buttons_prev=0;
    for(;;) {
        pdim12_state st;
        int probe=pdim12_probe(&st);
        show_menu(&st,probe);
        release_all_buttons();
        unsigned pressed=read_action()&0xffffu;
        if(pressed&PAD_CIRCLE)break;
        if(probe<0)continue;
        if(pressed&PAD_SQUARE){
            screen();
            scr_printf("READ-ONLY FILE VERIFICATION\n\n");
            pause_result(pdim12_verify_core_files(&st),&st);
        }else if(pressed&PAD_SELECT) {
            screen();
            scr_printf("READ ONLY: XMB HEADER DIAGNOSTICS\n\n");
            pause_result(pdim12_inspect_xmb(&st),&st);
        }else if(pressed&PAD_CROSS) {
            if(!confirm_danger())continue;
            screen();scr_printf("WRITING ONLY PP.DVRISO\n");
            if(!st.exists) {
                rc=pdim12_prepare_partition(&st);
                if(rc<0){pause_result(rc,&st);continue;}
            }
            rc=pdim12_install_files(&st);
            pause_result(rc,&st);
        }else if(pressed&PAD_TRIANGLE){
            if(!confirm_danger())continue;
            screen();scr_printf("XMB FILES THEN PPAA: PP.DVRISO\n");
            rc=pdim12_install_xmb(&st);
            pause_result(rc,&st);
        }
    }
    padPortClose(0,0);
    padEnd();
    scr_clear();
    return 0;
}

/* Separate test installer for Sony PSX DESR-7000 / PP.DVRISO12.
 * NO writes at startup. Every disk write requires TWO confirmations.
 * DO NOT COPY AN UNPATCHED PP.DVRISO Manager/Neutrino ONTO PP.DVRISO12.
 */
#include "pdim12_installer.h"
#include <debug.h>
#include <libpad.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <smem.h>
#include <delaythread.h>
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
MODULE(ps2dev9);
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
static uint32_t buttons_prev;
static const char hdd_args[]="-o\0" "4\0" "-n\0" "20";
static const char pfs_args[]="-m\0" "4\0" "-o\0" "10\0" "-n\0" "40";
typedef char hdd_args_size[(sizeof(hdd_args)==11)?1:-1];
typedef char pfs_args_size[(sizeof(pfs_args)==17)?1:-1];

typedef struct module_prefix {
    uint32_t next,name;
    uint16_t version,flags,id,unused;
} module_prefix;
static int module_find(const char *name) {
    uint32_t address=0x800;
    for(int i=0;i<256 && address;i++) {
        module_prefix info;char label[64];
        uint32_t physical=address&0x1fffffffu;
        if(!physical || physical+sizeof(info)>0x200000 || (physical&3))
            return -1;
        if(smem_read((void*)(uintptr_t)physical,&info,sizeof(info))!=(int)sizeof(info))
            return -1;
        physical=info.name&0x1fffffffu;
        if(physical && physical+sizeof(label)<=0x200000 &&
           smem_read((void*)(uintptr_t)physical,label,sizeof(label))==(int)sizeof(label) &&
           memchr(label,'\0',sizeof(label)) && !strcmp(label,name))
            return info.id;
        address=info.next;
    }
    return -1;
}
static int load(const char *name,const char *iop_name,void *data,
                unsigned int size,const char *args,unsigned args_size) {
    if(module_find(iop_name)>=0) {
        scr_printf("USING LOADED %-12s\n",name);
        return 0;
    }
    int rv=0,id=SifExecModuleBuffer(data,size,args_size,(void*)args,&rv);
    scr_printf("LOAD %-12s id=%d rv=%d\n",name,id,rv);
    return id<0?id:(rv<0?rv:0);
}
#define LOAD(n,mod_name) do { \
  if(load(#n,mod_name,n##_irx,size_##n##_irx,0,0)<0) return -100; \
} while(0)
static int setup_modules(void){
    SifInitRpc(0);
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();
    sbv_patch_fileio();
    LOAD(iomanX,"IO/File_Manager");
    LOAD(fileXio,"IOX/File_Manager_Rpc");
    LOAD(ps2dev9,"ps2dev9");
    LOAD(ps2atad,"ps2atad");
    if(load("ps2hdd","ps2hdd",ps2hdd_psx1_irx,
            size_ps2hdd_psx1_irx,hdd_args,sizeof(hdd_args))<0)return -100;
    if(load("ps2fs","ps2fs",ps2fs_irx,size_ps2fs_irx,
            pfs_args,sizeof(pfs_args))<0)return -100;
    int rc=fileXioInit();
    if(rc<0)return -100;
    LOAD(bdm,"bdm");
    LOAD(bdmfs_fatfs,"bdmfs_fatfs");
    LOAD(usbd,"usbd");
    LOAD(usbmass_bd,"usbmass_bd");
    LOAD(sio2man,"sio2man");
    LOAD(padman,"padman");
    return 0;
}
static void screen(void){
    scr_setbgcolor(0x08080C);
    scr_setfontcolor(0xFFFFFF);
    scr_clear();
    scr_setXY(2,2);
    scr_printf("PSX DVR ISO MANAGER - HDD12 INSTALLER\n");
    scr_printf("TARGET: hdd0:PP.DVRISO12  (256 MiB NEW)\n");
    scr_printf("PDIM 1.0 HDD TEST FILES - NOT HDD HTTPS 1.2\n\n");
}
static void red_warning(int final_stage) {
    scr_setbgcolor(0x600000);
    scr_setfontcolor(0xFFFFFF);
    scr_clear();
    scr_setXY(2,3);
    scr_printf("========================================\n");
    scr_printf("         UWAGA! OSTRZEZENIE!\n");
    scr_printf("========================================\n\n");
    scr_printf("ZAPIS MODYFIKUJE STRUKTURE APA\n");
    scr_printf("CALEGO DYSKU PSX DESR!\n\n");
    scr_printf("AWARIA MOZE USZKODZIC INNE PARTYCJE,\n");
    scr_printf("NIE TYLKO PP.DVRISO12!\n\n");
    scr_printf("WYMAGANA JEST ODTWARZALNA KOPIA\n");
    scr_printf("SEKTOROWA CALEGO HDD!\n");
    scr_printf("KOPIA SPISU APA NIE WYSTARCZA!\n\n");
    scr_printf("PP.DVRISO 1.0 MA POZOSTAC NIETKNIETA.\n");
    if(final_stage) {
        scr_printf("\nOSTATECZNE POTWIERDZENIE:\n");
        scr_printf("TRZYMAJ L1 + R1 I NACISNIJ X\n");
    }else{
        scr_printf("\nX = PRZECZYTALEM, MAM KOPIE SEKTOROWA\n");
    }
    scr_printf("O = ANULUJ BEZ ZAPISU\n");
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
    scr_printf("\nX OR O: RETURN TO MENU\n");
    release_all_buttons();
    for(;;){unsigned bits=read_action();if(bits&(PAD_CROSS|PAD_CIRCLE))break;}
}
static void show_menu(const pdim12_state *st,int probe_rc) {
    screen();
    if(probe_rc<0) {
        scr_printf("HDD PREFLIGHT FAIL: %d %s\n",probe_rc,pdim12_error(probe_rc));
        scr_printf("NO PARTITION WRITES PERMITTED!\n\n");
    }else {
        scr_printf("PP.DVRISO12: %s\n",st->exists?"EXISTS: PFS RO CHECK PASS":"NOT FOUND");
        if(st->exists)scr_printf("LBA START %lu, SECTORS %lu\n",
           (unsigned long)st->start,(unsigned long)st->sectors);
        scr_printf("ALL OTHER PARTITIONS: NEVER TARGETED\n\n");
        scr_printf("X: CREATE IF ABSENT + INSTALL TEST FILES\n");
        scr_printf("TRIANGLE: REGISTER XMB ENTRY (OPTIONAL)\n");
        scr_printf("SQUARE: READ-ONLY VERIFY FILES\n\n");
    }
    scr_printf("O: EXIT\n\n");
    scr_printf("UWAGA! ZAPIS NA HDD JEST RYZYKOWNY.\n");
    scr_printf("KOPIA SEKTOROWA CALEGO HDD WYMAGANA!\n");
}
int main(void) {
    init_scr();
    screen();
    scr_printf("LOADING MODULES - NO HDD WRITES\n");
    int rc=setup_modules();
    if(rc<0){
        scr_printf("\nSTOP: IOP MODULES ERROR %d\n",rc);
        scr_printf("REBOOT REQUIRED. NO DISK WRITES.\n");
        for(;;)DelayThread(1000000);
    }
    /* Pad init after module startup, with a safe timeout on the IOP RPC
     * path enforced by separate hardware verification in the first test.
     * NEVER run disk writes without a functioning controller. */
    if(padInit(0)<0 || padPortOpen(0,0,pad_buffer)<=0) {
        scr_printf("\nSTOP: GAMEPAD INIT FAILED. NO HDD WRITE.\n");
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
            pause_result(pdim12_verify_files(&st),&st);
        }else if(pressed&PAD_CROSS) {
            if(!confirm_danger())continue;
            screen();scr_printf("WRITING ONLY PP.DVRISO12\n");
            if(!st.exists) {
                rc=pdim12_prepare_partition(&st);
                if(rc<0){pause_result(rc,&st);continue;}
            }
            rc=pdim12_install_files(&st);
            pause_result(rc,&st);
        }else if(pressed&PAD_TRIANGLE){
            if(!confirm_danger())continue;
            screen();scr_printf("REGISTERING XMB FOR PP.DVRISO12 ONLY\n");
            rc=pdim12_install_xmb(&st);
            pause_result(rc,&st);
        }
    }
    padPortClose(0,0);
    padEnd();
    scr_clear();
    return 0;
}

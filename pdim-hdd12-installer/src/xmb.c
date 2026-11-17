/* Optional Sony PSX XMB registration for PP.DVRISO12 only.
 * The PPAA partition-relative area starts at offset 0 for a RAW hdd0:PP.*
 * handle (sector 8). Mirrored from the hdl-dump/RepairBox working header
 * layout. We do not change the global APA linked list or any other target.
 * Existing, foreign or unrecognised headers are never overwritten.
 */
#include "pdim12_installer.h"
#include <stdio.h>
#include <string.h>
#include <debug.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define PPAA_SIZE (0x800+0x10000)
#define SYS_OFF 0x200
#define ICONSYS_OFF 0x400
#define ICON_OFF 0x800
#define ICON_LIMIT 0x10000
static unsigned char current[PPAA_SIZE] __attribute__((aligned(64)));
static unsigned char planned[PPAA_SIZE] __attribute__((aligned(64)));
static unsigned char readback[4096] __attribute__((aligned(64)));
static unsigned char sys[512] __attribute__((aligned(64)));
static unsigned char iconsys[1024] __attribute__((aligned(64)));
static unsigned char icon[ICON_LIMIT] __attribute__((aligned(64)));

static void set_le32(unsigned char *dst,unsigned value){
    for(int i=0;i<4;i++)dst[i]=(unsigned char)(value>>(8*i));
}
static int exact_asset(const char *path,void *buffer,unsigned limit) {
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);
    if(fd<0)return -1;
    int total=0,n=0;
    while(total<(int)limit){
        n=fileXioRead(fd,(unsigned char*)buffer+total,(int)limit-total);
        if(n<=0)break;
        total+=n;
    }
    unsigned char extra=0;
    int x=fileXioRead(fd,&extra,1),c=fileXioClose(fd);
    return n<0 || x!=0 || c<0 || total<=0 ? -1:total;
}
static int raw_read(int fd,unsigned char *dst,unsigned size) {
    if(fileXioLseek(fd,0,FIO_SEEK_SET)!=0)return -1;
    for(unsigned done=0;done<size;){
        int want=size-done>4096?4096:(int)(size-done);
        int n=fileXioRead(fd,dst+done,want);
        if(n!=want)return -1;
        done+=(unsigned)n;
    }
    return 0;
}
static int raw_write(int fd,const unsigned char *src,unsigned size){
    if(fileXioLseek(fd,0,FIO_SEEK_SET)!=0)return -1;
    for(unsigned done=0;done<size;){
        int want=size-done>4096?4096:(int)(size-done);
        int n=fileXioWrite(fd,(void*)(src+done),want);
        if(n!=want)return -1;
        done+=(unsigned)n;
    }
    return 0;
}
static int already_registered_or_foreign(const unsigned char *buf,unsigned len) {
    /* A registered partition must match EXACTLY; never replace a foreign
     * channel even if its partition happens to share our test name.
     */
    if(!memcmp(buf,"PS2ICON3D",9))
        return !memcmp(buf,planned,len)?1:-1;
    /* The newly created partition's PPAA header must be untouched/zero. */
    for(unsigned i=0;i<len;i++)if(buf[i])return -1;
    return 0;
}
int pdim12_install_xmb(pdim12_state *state) {
    if(!state || !state->exists || !state->pfs_ok ||
       !pdim12_partition_name_allowed(PDIM12_PARTITION))return -30;
    /* Only register a channel if ALL app and launcher files read back. */
    int rc=pdim12_verify_files(state);
    if(rc<0 || state->files_verified!=state->files_total)return -30;

    int sys_size=exact_asset(PDIM12_XMB_SOURCE "system.cnf",sys,sizeof(sys));
    int ini_size=exact_asset(PDIM12_XMB_SOURCE "icon.sys",iconsys,sizeof(iconsys));
    int png_size=exact_asset(PDIM12_XMB_SOURCE "jkt_002.png",icon,sizeof(icon));
    if(sys_size>0 && sys_size<(int)sizeof(sys))sys[sys_size]=0;
    if(ini_size>0 && ini_size<(int)sizeof(iconsys))iconsys[ini_size]=0;
    if(sys_size<=0 || sys_size>511 || ini_size<=0 || ini_size>1023 ||
       png_size<=0 || png_size>=ICON_LIMIT ||
       (sys_size<26 || strstr((const char *)sys,"BOOT2 = pfs:/EXECUTE.ELF")==NULL) ||
       memcmp(icon,"\x89PNG\r\n\x1a\n",8))return -30;
    unsigned padded=((unsigned)png_size+511u)&~511u;
    unsigned bytes=ICON_OFF+padded;
    if(bytes>sizeof(current))return -30;

    /* The XMB metadata and canonical icon bytes come from the existing
     * PDIM 1.0 XMB assets, with only the TEST12 titles adapted on PC.
     */
    int fd=fileXioOpen(PDIM12_DEVICE,FIO_O_RDWR,0);
    if(fd<0)return -31;
    if(raw_read(fd,current,bytes)<0){fileXioClose(fd);return -31;}
    memcpy(planned,current,bytes);
    memcpy(planned,"PS2ICON3D",9);
    set_le32(planned+0x10,SYS_OFF);
    set_le32(planned+0x14,(unsigned)sys_size);
    set_le32(planned+0x18,ICONSYS_OFF);
    set_le32(planned+0x1c,(unsigned)ini_size);
    set_le32(planned+0x20,ICON_OFF);
    set_le32(planned+0x24,(unsigned)png_size);
    set_le32(planned+0x28,ICON_OFF);
    set_le32(planned+0x2c,(unsigned)png_size);
    memcpy(planned+SYS_OFF,sys,(unsigned)sys_size);
    memset(planned+SYS_OFF+sys_size,0,ICONSYS_OFF-SYS_OFF-(unsigned)sys_size);
    memcpy(planned+ICONSYS_OFF,iconsys,(unsigned)ini_size);
    memset(planned+ICONSYS_OFF+ini_size,0,ICON_OFF-ICONSYS_OFF-(unsigned)ini_size);
    memcpy(planned+ICON_OFF,icon,(unsigned)png_size);
    memset(planned+ICON_OFF+png_size,0,padded-(unsigned)png_size);
    int header=already_registered_or_foreign(current,bytes);
    if(header<0){fileXioClose(fd);return -31;}
    if(header==0 && raw_write(fd,planned,bytes)<0){fileXioClose(fd);return -32;}
    if(raw_read(fd,current,bytes)<0){fileXioClose(fd);return -33;}
    int close=fileXioClose(fd);
    if(close<0||memcmp(current,planned,bytes))return -33;
    scr_printf("XMB header: PP.DVRISO12 VERIFIED %s\n",header?"EXISTING":"NEW");
    return 0;
}

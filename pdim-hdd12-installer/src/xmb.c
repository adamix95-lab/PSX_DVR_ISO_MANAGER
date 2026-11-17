/* PDIM HDD12 Installer R3.
 * XMB header injection uses the physical sector method from
 * NathanNeurotic/wLaunchELF_R3Z src/hdd_header_injector.c.
 * Only hdd0:PP.DVRISO12 sectors start+8, +9 and +10 can be written.
 * No PNG is ever injected into the PPAA list.ico descriptor.
 * res/info.sys + jkt_001.png + jkt_002.png are installed afterwards.
 */
#include "pdim12_installer.h"
#include "ppaa_plan.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <debug.h>
#include <hdd-ioctl.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define B PDIM12_PPAA_SECTOR_BYTES
static unsigned char request[sizeof(hddAtaTransfer_t)+B] __attribute__((aligned(64)));
static unsigned char original[3][B] __attribute__((aligned(64)));
static unsigned char proposed[3][B] __attribute__((aligned(64)));
static unsigned char again[B] __attribute__((aligned(64)));
static unsigned char sys[B+1] __attribute__((aligned(64)));
static unsigned char icon[B+1] __attribute__((aligned(64)));
static uint32_t sector_base;

static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static int read_sector(uint32_t lba, unsigned char dst[B]) {
    hddAtaTransfer_t *req=(hddAtaTransfer_t *)request;
    memset(request,0,sizeof(request));
    req->lba=lba; req->size=1;
    int rc=fileXioDevctl("hdd0:",HDIOC_READSECTOR,
                         request,sizeof(hddAtaTransfer_t),dst,B);
    return rc<0?rc:0;
}
static int write_sector(uint32_t lba, const unsigned char src[B]) {
    hddAtaTransfer_t *req=(hddAtaTransfer_t *)request;
    memset(request,0,sizeof(request));
    req->lba=lba; req->size=1;
    memcpy(req->data,src,B);
    int rc=fileXioDevctl("hdd0:",HDIOC_WRITESECTOR,
                         request,sizeof(hddAtaTransfer_t)+B,NULL,0);
    return rc<0?rc:0;
}
static int read_asset(const char *path,unsigned char *dst,int capacity) {
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);
    if(fd<0)return -1;
    int total=0,n=0;
    while(total<capacity) {
        n=fileXioRead(fd,dst+total,capacity-total);
        if(n<=0)break;
        total+=n;
    }
    unsigned char extra;
    int last=fileXioRead(fd,&extra,1);
    int end=fileXioClose(fd);
    if(n<0 || last!=0 || end<0 || total<=0)return -1;
    return total;
}
static int load_plan(pdim12_state *state, int *status) {
    iox_stat_t stat;
    if(!state || !state->exists || !state->pfs_ok ||
       !pdim12_partition_name_allowed(PDIM12_PARTITION))
        return -30;
    if(fileXioDevctl("hdd0:",HDIOC_STATUS,NULL,0,NULL,0)!=0)
        return -34;
    int rc=fileXioGetStat(PDIM12_DEVICE,&stat);
    if(rc<0 || (uint32_t)stat.private_5!=state->start ||
       (uint32_t)stat.size!=state->sectors ||
       !pdim12_address_safe(state->start,state->sectors) ||
       state->start>UINT32_MAX-10u)return -35;
    sector_base=state->start+8u;
    for(unsigned j=0;j<3;j++) {
        rc=read_sector(sector_base+j,original[j]);
        if(rc<0) {
            scr_printf("XMB sector read FAILED LBA=%lu code=%d\n",
                       (unsigned long)(sector_base+j),rc);
            return -34;
        }
    }
    int ns=read_asset(PDIM12_XMB_SOURCE "system.cnf",sys,B);
    int ni=read_asset(PDIM12_XMB_SOURCE "icon.sys",icon,B);
    if(ns<=0||ns>=B||ni<=0||ni>=B)return -30;
    sys[ns]=0;icon[ni]=0;
    if(!strstr((char*)sys,"BOOT2 = pfs:/EXECUTE.ELF") ||
       !strstr((char*)icon,"title1=HDD12 TEST") ||
       memcmp(icon,"PS2X",4))return -30;
    int plan=pdim12_ppaa_plan(original,sys,(size_t)ns,
                              icon,(size_t)ni,proposed);
    if(status)*status=plan;
    return plan==-2?-31:(plan==-1?-30:0);
}
static void describe_header(int status) {
    scr_printf("XMB PPAA: sector %lu (target only)\n",
               (unsigned long)sector_base);
    scr_printf("MAGIC: %.9s  first=%02X %02X %02X %02X\n",
               !memcmp(original[0],"PS2ICON3D",9)?"PS2ICON3D":"UNKNOWN",
               original[0][0],original[0][1],
               original[0][2],original[0][3]);
    scr_printf("system.cnf: offset=%08lX length=%lu\n",
               (unsigned long)le32(original[0]+0x10),
               (unsigned long)le32(original[0]+0x14));
    scr_printf("icon.sys: offset=%08lX length=%lu\n",
               (unsigned long)le32(original[0]+0x18),
               (unsigned long)le32(original[0]+0x1c));
    scr_printf("list.ico: offset=%08lX length=%lu\n",
               (unsigned long)le32(original[0]+0x20),
               (unsigned long)le32(original[0]+0x24));
    if(status==0)scr_printf("HEADER: NEW SLOT AVAILABLE\n");
    else if(status==1)scr_printf("HEADER: ALREADY INSTALLED\n");
    else scr_printf("HEADER: FOREIGN/PARTIAL - WRITES BLOCKED\n");
}
int pdim12_inspect_xmb(pdim12_state *state) {
    int status=-3,rc=load_plan(state,&status);
    if(rc==0||rc==-31)describe_header(status);
    scr_printf("READ-ONLY XMB INSPECTION (NO DISK WRITES)\n");
    return rc;
}
int pdim12_install_xmb(pdim12_state *state) {
    if(!state || !state->exists || !state->pfs_ok)return -30;
    /* Make sure the whole application is present, and the XMB jackets
     * can be copied after registration, before changing any PPAA bytes.
     */
    int rc=pdim12_verify_core_files(state);
    if(rc<0)return -30;
    rc=pdim12_preflight_xmb_assets(state);
    if(rc<0)return rc;
    int status=-3;
    rc=load_plan(state,&status);
    if(rc<0) {
        if(rc==-31)describe_header(status);
        return rc;
    }
    if(status==0) {
        /* Re-read the three sectors before modifying ANY of them. */
        iox_stat_t stat;
        if(fileXioGetStat(PDIM12_DEVICE,&stat)<0 ||
           (uint32_t)stat.private_5!=state->start ||
           (uint32_t)stat.size!=state->sectors)return -35;
        for(unsigned j=0;j<3;j++) {
            rc=read_sector(sector_base+j,again);
            if(rc<0||memcmp(again,original[j],B))return -35;
        }
        /* Text first; magic and descriptors last. No unrelated sectors. */
        for(unsigned j=1;j<3;j++) {
            if(!memcmp(original[j],proposed[j],B))continue;
            rc=write_sector(sector_base+j,proposed[j]);
            if(rc<0)return -32;
            rc=read_sector(sector_base+j,again);
            if(rc<0||memcmp(again,proposed[j],B))return -33;
        }
        rc=write_sector(sector_base,proposed[0]);
        if(rc<0)return -32;
        for(unsigned j=0;j<3;j++) {
            rc=read_sector(sector_base+j,again);
            if(rc<0||memcmp(again,proposed[j],B))return -33;
        }
        scr_printf("XMB PPAA header: VERIFIED NEW\n");
    } else if(status==1) {
        scr_printf("XMB PPAA header: ALREADY VERIFIED\n");
    } else return -31;
    /* The PNG jackets and info.sys are normal PFS files, not raw
     * APA sectors. Copy them NOW with CRC/readback, no manual wLE step.
     * Rerunning TRIANGLE after a power loss resumes this phase if
     * the previous PPAA header matches, rather than overwriting it.
     */
    scr_printf("Installing XMB res/ assets after header...\n");
    rc=pdim12_install_xmb_assets(state);
    if(rc<0) {
        scr_printf("XMB res/ installation FAILED (%d).\n",rc);
        scr_printf("Do NOT delete the partition; retry TRIANGLE\n");
        scr_printf("after checking the USB / full HDD backup.\n");
        return rc;
    }
    scr_printf("XMB: PP.DVRISO12 HEADER + ASSETS VERIFIED\n");
    return 0;
}

/* Exact 3-sector XMB PPAA registration, adapted from the hardware-proven
 * PDIM 1.2 HDD installer. Fail-closed for foreign/incomplete headers.
 * Only the selected partition's start+8, +9, +10 are ever writable.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <hdd-ioctl.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include "sg_installer.h"
#include "ppaa_plan.h"

#define B 512
static unsigned char transfer[sizeof(hddAtaTransfer_t)+B] __attribute__((aligned(64)));
static unsigned char before[3][B] __attribute__((aligned(64)));
static unsigned char after[3][B] __attribute__((aligned(64)));
static unsigned char readback[B] __attribute__((aligned(64)));

static int read_sector(uint32_t lba,unsigned char out[B]) {
  hddAtaTransfer_t *t=(hddAtaTransfer_t*)transfer;
  memset(transfer,0,sizeof(transfer));t->lba=lba;t->size=1;
  int rc=fileXioDevctl("hdd0:",HDIOC_READSECTOR,transfer,sizeof(hddAtaTransfer_t),out,B);
  return rc<0?-1:0;
}
static int write_sector(uint32_t lba,const unsigned char data[B]) {
  hddAtaTransfer_t *t=(hddAtaTransfer_t*)transfer;
  memset(transfer,0,sizeof(transfer));t->lba=lba;t->size=1;
  memcpy(t->data,data,B);
  int rc=fileXioDevctl("hdd0:",HDIOC_WRITESECTOR,transfer,sizeof(hddAtaTransfer_t)+B,NULL,0);
  return rc<0?-1:0;
}
int sg_register_xmb(const sg_game *g,int lang) {
  if(!g || !g->partition[0] || !g->game_id[0] || !g->cover_available)return -30;
  /* The original artwork index might refer to an ISO which has changed.
   * A detected *different* ID fails closed. An unidentifiable but readable
   * ISO falls back to the validated index and exact per-partition marker. */
  int live_iso=sg_check_iso_id(g);
  if(live_iso<0)return live_iso==SG_ISO_ID_MISMATCH?-63:-62;
  if(fileXioDevctl("hdd0:",HDIOC_STATUS,NULL,0,NULL,0)!=0)return -72;
  uint32_t start=0,sectors=0;
  int rc=sg_validate_payload(g,lang,&start,&sectors);
  if(rc<0)return rc;
  if(start==0||sectors!=SG_SECTORS||start>0x0FFFFFFF-11u)return -72;
  char dev[56];snprintf(dev,sizeof(dev),"hdd0:%s",g->partition);
  iox_stat_t st;
  if(fileXioGetStat(dev,&st)<0 ||
     (uint32_t)st.private_5!=start ||
     (uint32_t)st.size!=sectors)return -72;

  char system_cnf[B],icon_sys[B];
  sg_build_sys(system_cnf,sizeof(system_cnf));
  sg_build_icon(g,icon_sys,sizeof(icon_sys));
  size_t ns=strlen(system_cnf),ni=strlen(icon_sys);
  if(ns>=B||ni>=B||memcmp(icon_sys,"PS2X",4)||
     !strstr(system_cnf,"BOOT2 = pfs:/EXECUTE.ELF"))return -70;
  uint32_t sector=start+8u;
  for(int i=0;i<3;i++)if(read_sector(sector+i,before[i])<0)return -71;
  rc=pdim12_ppaa_plan((const uint8_t (*)[512])before,
                      (const uint8_t*)system_cnf,ns,
                      (const uint8_t*)icon_sys,ni,(uint8_t (*)[512])after);
  if(rc==1)return 0; /* already registered, no write */
  if(rc!=0)return -70; /* never apply legacy 1.0 upgrade to a game shortcut */

  /* Recheck both PFS marker and APA identity immediately before raw write. */
  uint32_t now_start=0,now_size=0;
  if(sg_validate_payload(g,lang,&now_start,&now_size)<0 ||
     start!=now_start || sectors!=now_size)return -72;
  if(fileXioGetStat(dev,&st)<0 ||
     (uint32_t)st.private_5!=start ||
     (uint32_t)st.size!=sectors ||
     fileXioDevctl("hdd0:",HDIOC_STATUS,NULL,0,NULL,0)!=0)return -72;
  for(int i=0;i<3;i++)
    if(read_sector(sector+i,readback)<0||memcmp(readback,before[i],B))return -72;

  /* Write the two text sectors first; descriptor sector LAST. */
  for(int i=1;i<=2;i++) {
    if(!memcmp(before[i],after[i],B))continue;
    if(write_sector(sector+i,after[i])<0 ||
       read_sector(sector+i,readback)<0 ||
       memcmp(readback,after[i],B))return -71;
  }
  if(memcmp(before[0],after[0],B)) {
    if(write_sector(sector,after[0])<0)return -71;
  }
  for(int i=0;i<3;i++)
    if(read_sector(sector+i,readback)<0||memcmp(readback,after[i],B))return -71;
  return 0;
}

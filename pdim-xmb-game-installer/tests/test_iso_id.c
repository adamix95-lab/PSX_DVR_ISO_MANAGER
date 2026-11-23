/* PSX DVR ISO multi-method Game ID scanner host regression tests.
 * Simulated Sony DVR fileXio driver with ISO2048/raw2352 images, unseekable
 * files and partial reads. Zero writes; Killzone/BLACK both tested.
 */
#include "sg_installer.h"
#include "fileXio_rpc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define IMAGE_BYTES (1024*1024)
static unsigned char image[IMAGE_BYTES];
static unsigned position;
static int is_open;
static int opens,closes,seek_attempts,reads;
static char active_id[12],active_file[128];
enum {
  CASE_CNF_MATCH, CASE_CNF_DIFFERENT, CASE_EXE_MATCH, CASE_EXE_DIFFERENT,
  CASE_NO_ID, CASE_NO_PVD, CASE_BAD_ROOT, CASE_OPEN_FAIL,
  CASE_PVD_READ_FAIL, CASE_CNF_READ_FAIL, CASE_BROKEN_SEEK,
  CASE_SEEK_LIES, CASE_PARTIAL_READ, CASE_RAW_MODE1, CASE_RAW_MODE2,
  CASE_BLACK_NO_ID, CASE_BLACK_EXE_MATCH
};
static int scenario;
static unsigned fail_at;

static void put32(unsigned char *p,uint32_t n){
  p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);
  p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);
}
static void build_record(unsigned char *block,const char *name,int sector,
                         uint32_t size) {
  size_t n=strlen(name);
  assert(n<100);
  unsigned len=(unsigned)(33+n);
  if(len&1)len++;
  block[0]=(unsigned char)len;
  block[32]=(unsigned char)n;
  memcpy(block+33,name,n);
  put32(block+2,(unsigned)sector);
  put32(block+10,size);
}
static int uses_exe(void) {
  return scenario==CASE_EXE_MATCH||scenario==CASE_EXE_DIFFERENT||
         scenario==CASE_BLACK_EXE_MATCH;
}
static int is_raw(void) {
  return scenario==CASE_RAW_MODE1 || scenario==CASE_RAW_MODE2;
}
static void set_case(int which,const char *filename,const char *expected_id) {
  scenario=which;position=0;opens=0;closes=0;is_open=0;
  seek_attempts=0;reads=0;fail_at=0;
  snprintf(active_file,sizeof(active_file),"dvr_pfs0:/ISO/%s",filename);
  strcpy(active_id,expected_id);
  memset(image,0,sizeof(image));
  if(which==CASE_NO_PVD||which==CASE_OPEN_FAIL||
     which==CASE_PVD_READ_FAIL||which==CASE_BLACK_NO_ID)
    if(which==CASE_NO_PVD||which==CASE_OPEN_FAIL) return;
  uint32_t stride=is_raw()?2352u:2048u;
  uint32_t pad=which==CASE_RAW_MODE1?16u:
               which==CASE_RAW_MODE2?24u:0u;
  unsigned char *pvd=image+16u*stride+pad;
  unsigned char *root=image+32u*stride+pad;
  unsigned char *cnf=image+33u*stride+pad;
  pvd[0]=1;memcpy(pvd+1,"CD001",5);pvd[6]=1;
  pvd[156]=34;pvd[156+32]=1;
  put32(pvd+156+2,32);
  put32(pvd+156+10,2048);
  if(which==CASE_BAD_ROOT){root[0]=2;return;}
  if(which==CASE_NO_ID||which==CASE_BLACK_NO_ID) {
    build_record(root,"README.TXT;1",33,100);
  } else if(uses_exe()) {
    build_record(root,(which==CASE_EXE_DIFFERENT)?"SLUS_210.47;1":
                      which==CASE_BLACK_EXE_MATCH?"SLUS_213.76;1":
                      "SCUS_974.02;1",33,100);
  } else {
    build_record(root,"SYSTEM.CNF;1",33,100);
    const char *value=which==CASE_CNF_DIFFERENT?
      "BOOT2 = cdrom0:\\SLUS_210.47;1\r\n":
      "BOOT2 = cdrom0:\\SCUS_974.02;1\r\n";
    memcpy(cnf,value,strlen(value));
  }
  if(which==CASE_PVD_READ_FAIL)fail_at=16u*2048u;
  if(which==CASE_CNF_READ_FAIL)fail_at=33u*2048u;
}
int fileXioOpen(const char *path,int flags,int mode) {
  (void)mode;(void)flags;
  assert(!strcmp(path,active_file));
  if(scenario==CASE_OPEN_FAIL)return -31;
  assert(!is_open);
  is_open=1;position=0;opens++;
  return 7;
}
int fileXioClose(int fd) {
  assert(fd==7 && is_open);is_open=0;closes++;return 0;
}
int fileXioLseek(int fd,int pos,int origin) {
  assert(fd==7 && is_open && origin==SEEK_SET);
  seek_attempts++;
  if(scenario==CASE_BROKEN_SEEK)return -40;
  position=(unsigned)pos;
  return pos;
}
int fileXioRead(int fd,void *dst,int requested) {
  assert(fd==7 && is_open && requested>0);
  reads++;
  if(position==fail_at && fail_at && (scenario==CASE_PVD_READ_FAIL||
                                     scenario==CASE_CNF_READ_FAIL))
    return -41;
  if(scenario==CASE_SEEK_LIES && seek_attempts>0 && opens==1)
    return -41; /* seek reported success but read fails; reopen must fix it */
  if(position>=IMAGE_BYTES)return 0;
  unsigned n=(unsigned)requested;
  if(n>IMAGE_BYTES-position)n=IMAGE_BYTES-position;
  if(scenario==CASE_PARTIAL_READ && n>127)n=127;
  memcpy(dst,image+position,n);position+=n;
  return (int)n;
}
static void check(int which,const char *name,const char *id,int expected){
  sg_game g={0};
  strcpy(g.filename,name);
  strcpy(g.game_id,id);
  set_case(which,name,id);
  int result=sg_check_iso_id(&g);
  if(result!=expected)fprintf(stderr,
    "case %d file %s expected %d got %d, opens %d, seeks %d, reads %d\n",
    which,name,expected,result,opens,seek_attempts,reads);
  assert(result==expected);
  assert(is_open==0 && opens==closes);
  if(which==CASE_BROKEN_SEEK || which==CASE_SEEK_LIES)
    assert(opens>=2); /* must retry using sequential reads */
}
int main(void) {
  check(CASE_CNF_MATCH,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MATCH);
  check(CASE_CNF_DIFFERENT,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MISMATCH);
  check(CASE_EXE_MATCH,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MATCH);
  check(CASE_EXE_DIFFERENT,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MISMATCH);
  check(CASE_NO_ID,"Killzone.iso","SCUS_974.02",SG_ISO_ID_INDEX_ONLY);
  check(CASE_NO_PVD,"Killzone.iso","SCUS_974.02",SG_ISO_ID_INDEX_ONLY);
  check(CASE_BAD_ROOT,"Killzone.iso","SCUS_974.02",SG_ISO_ID_INDEX_ONLY);
  check(CASE_OPEN_FAIL,"Killzone.iso","SCUS_974.02",SG_ISO_ID_IO_ERROR);
  check(CASE_PVD_READ_FAIL,"Killzone.iso","SCUS_974.02",SG_ISO_ID_INDEX_READER);
  check(CASE_CNF_READ_FAIL,"Killzone.iso","SCUS_974.02",SG_ISO_ID_INDEX_READER);
  check(CASE_BROKEN_SEEK,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MATCH);
  check(CASE_SEEK_LIES,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MATCH);
  check(CASE_PARTIAL_READ,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MATCH);
  check(CASE_RAW_MODE1,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MATCH);
  check(CASE_RAW_MODE2,"Killzone.iso","SCUS_974.02",SG_ISO_ID_MATCH);
  check(CASE_BLACK_NO_ID,"BLACK.iso","SLUS_213.76",SG_ISO_ID_INDEX_ONLY);
  check(CASE_BLACK_EXE_MATCH,"BLACK.iso","SLUS_213.76",SG_ISO_ID_MATCH);
  puts("PASS: multi-method PSX DVR ISO scanner, indexed fallback, raw 2352, nonseekable and partial reads.");
  return 0;
}

/* Standalone host regression of the DVR ISO Game ID probe.
 * Tests the Killzone regression: an accessible ISO without a detectable ID
 * must return INDEX_ONLY instead of the fatal -62.
 * No console, APA or filesystem writes.
 */
#include "sg_installer.h"
#include "fileXio_rpc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

enum {
  CASE_CNF_MATCH, CASE_CNF_DIFFERENT, CASE_EXE_MATCH, CASE_EXE_DIFFERENT,
  CASE_NO_ID, CASE_NO_PVD, CASE_BAD_ROOT, CASE_OPEN_FAIL,
  CASE_PVD_READ_FAIL, CASE_CNF_READ_FAIL
};
static int scenario, at, nclose;

static void put32(unsigned char *p,unsigned n) {
  p[0]=(unsigned char)n; p[1]=(unsigned char)(n>>8);
  p[2]=(unsigned char)(n>>16); p[3]=(unsigned char)(n>>24);
}
int fileXioOpen(const char *path,int flags,int mode) {
  (void)flags;(void)mode;
  assert(!strcmp(path,"dvr_pfs0:/ISO/Killzone.iso"));
  return scenario==CASE_OPEN_FAIL?-1:7;
}
int fileXioLseek(int fd,int offset,int whence) {
  assert(fd==7 && whence==SEEK_SET);
  at=offset; return offset;
}
int fileXioClose(int fd) {assert(fd==7);nclose++;return 0;}
int fileXioRead(int fd,void *dst,int size) {
  assert(fd==7 && size>0);
  unsigned char *p=dst;memset(p,0,(size_t)size);
  if(scenario==CASE_PVD_READ_FAIL && at==16*2048) return -1;
  if(scenario==CASE_CNF_READ_FAIL && at==33*2048)return -1;
  if(at==16*2048) {
    if(scenario==CASE_NO_PVD)return size;
    assert(size==2048);
    p[0]=1;memcpy(p+1,"CD001",5);p[6]=1;
    p[156]=34;p[156+32]=1;
    put32(p+156+2,32);put32(p+156+10,2048);
  } else if(at==32*2048) {
    assert(size==2048);
    if(scenario==CASE_BAD_ROOT){p[0]=2;return size;}
    const char *name=(scenario==CASE_EXE_MATCH)?"SCUS_974.02;1":
                     (scenario==CASE_EXE_DIFFERENT)?"SLUS_210.47;1":
                     (scenario==CASE_NO_ID)?"README.TXT;1":
                     "SYSTEM.CNF;1";
    int len=(int)strlen(name);
    int rec=33+len;
    if(rec&1)rec++;
    p[0]=(unsigned char)rec;p[32]=(unsigned char)len;
    memcpy(p+33,name,(size_t)len);
    put32(p+2,33);put32(p+10,96);
  } else if(at==33*2048) {
    const char *cnf=scenario==CASE_CNF_DIFFERENT?
             "BOOT2 = cdrom0:\\SLUS_210.47;1\r\n":
             "BOOT2 = cdrom0:\\SCUS_974.02;1\r\n";
    memcpy(p,cnf,strlen(cnf));
  } else {
    assert(at%2048==0); /* Remaining PVD sectors */
  }
  at+=size;return size;
}
static void check(int c,int expected) {
  sg_game g={0};
  strcpy(g.filename,"Killzone.iso");
  strcpy(g.game_id,"SCUS_974.02");
  scenario=c;at=0;nclose=0;
  int got=sg_check_iso_id(&g);
  if(got!=expected)
    fprintf(stderr,"scenario %d expected %d got %d\n",c,expected,got);
  assert(got==expected);
  assert(nclose==(c==CASE_OPEN_FAIL?0:1));
}
int main(void) {
  check(CASE_CNF_MATCH,SG_ISO_ID_MATCH);
  check(CASE_CNF_DIFFERENT,SG_ISO_ID_MISMATCH);
  check(CASE_EXE_MATCH,SG_ISO_ID_MATCH);
  check(CASE_EXE_DIFFERENT,SG_ISO_ID_MISMATCH);
  check(CASE_NO_ID,SG_ISO_ID_INDEX_ONLY);
  check(CASE_NO_PVD,SG_ISO_ID_INDEX_ONLY);
  check(CASE_BAD_ROOT,SG_ISO_ID_INDEX_ONLY);
  check(CASE_OPEN_FAIL,SG_ISO_ID_IO_ERROR);
  check(CASE_PVD_READ_FAIL,SG_ISO_ID_IO_ERROR);
  check(CASE_CNF_READ_FAIL,SG_ISO_ID_IO_ERROR);
  puts("PASS: live ISO Match/Mismatch/Unknown and hard IO error are different.");
  return 0;
}

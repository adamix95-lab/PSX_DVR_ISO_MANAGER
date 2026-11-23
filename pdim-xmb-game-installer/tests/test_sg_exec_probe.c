/* Host tests for EXECUTE.ELF diagnostics against a mocked read-only PFS.
 * All seven outcomes are checked independently of hardware.
 */
#include "sg_exec_probe.h"
#include "fileXio_rpc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum {
  FIX_MATCH, FIX_CRC, FIX_SIZE, FIX_MISSING, FIX_DIR_ERROR,
  FIX_OPEN_ERROR, FIX_READ_ERROR, FIX_SMALL_CHUNKS
};
static int which, listed, dir_open, file_open, pos, close_calls, open_flags;
static unsigned char original[80],modified[82];
static unsigned actual_length;
static void setup(int kind) {
  which=kind; listed=0;dir_open=0;file_open=0;pos=0;close_calls=0;
  open_flags=0;actual_length=sizeof(original);
  for(unsigned i=0;i<sizeof(original);i++)
    original[i]=(unsigned char)(i^0x5a);
  memcpy(modified,original,sizeof(original));
  if(kind==FIX_CRC)modified[12]^=0xff;
  if(kind==FIX_SIZE)actual_length++;
  modified[80]=0x44;
}
int fileXioDopen(const char *path) {
  assert(!strcmp(path,"pfs0:/"));
  if(which==FIX_DIR_ERROR)return -9;
  assert(!dir_open);dir_open=1;listed=0;return 3;
}
int fileXioDread(int fd,iox_dirent_t *ent) {
  assert(fd==3 && dir_open);
  if(which==FIX_DIR_ERROR)return -9;
  if(which==FIX_MISSING)return 0;
  if(listed++)return 0;
  memset(ent,0,sizeof(*ent));
  strcpy(ent->name,"EXECUTE.ELF");return 1;
}
int fileXioDclose(int fd) {assert(fd==3 && dir_open);dir_open=0;return 0;}
int fileXioOpen(const char *path,int flags,int mode) {
  (void)mode;assert(!strcmp(path,"pfs0:/EXECUTE.ELF"));
  assert(flags==FIO_O_RDONLY);
  open_flags=flags;
  if(which==FIX_OPEN_ERROR)return -7;
  assert(!file_open);file_open=1;return 9;
}
int fileXioRead(int fd,void *dest,int requested) {
  assert(fd==9 && file_open && requested>0);
  if(which==FIX_READ_ERROR && pos>=16)return -8;
  if((unsigned)pos>=actual_length)return 0;
  int n=requested;
  if(which==FIX_READ_ERROR && n>16)n=16;
  if(which==FIX_SMALL_CHUNKS && n>7)n=7;
  if(n>(int)(actual_length-(unsigned)pos))
    n=(int)(actual_length-(unsigned)pos);
  memcpy(dest,modified+pos,(size_t)n);pos+=n;return n;
}
int fileXioClose(int fd){assert(fd==9 && file_open);file_open=0;close_calls++;return 0;}
static void check(int fixture,enum sg_exec_status expected) {
  setup(fixture);
  sg_exec_probe result;
  assert(sg_probe_execute(original,sizeof(original),&result)==0);
  if(result.status!=expected)
    fprintf(stderr,"case %d got %d expected %d\n",fixture,result.status,expected);
  assert(result.status==expected);
  assert(result.expected_size==sizeof(original));
  if(expected==SG_EXEC_IDENTICAL) {
    assert(result.actual_complete && result.actual_size==sizeof(original));
    assert(result.actual_crc==result.expected_crc);
  }
  if(expected==SG_EXEC_CRC_DIFF) {
    assert(result.actual_complete && result.actual_size==sizeof(original));
    assert(result.actual_crc!=result.expected_crc);
  }
  if(expected==SG_EXEC_SIZE_DIFF) {
    assert(result.actual_complete && result.actual_size==sizeof(original)+1);
  }
  if(expected==SG_EXEC_READ_ERROR)
    assert(!result.actual_complete && result.actual_size>0);
  if(fixture!=FIX_OPEN_ERROR && fixture!=FIX_MISSING && fixture!=FIX_DIR_ERROR)
    assert(open_flags==FIO_O_RDONLY && close_calls==1);
  assert(!dir_open && !file_open);
}
int main(void) {
  check(FIX_MATCH,SG_EXEC_IDENTICAL);
  check(FIX_CRC,SG_EXEC_CRC_DIFF);
  check(FIX_SIZE,SG_EXEC_SIZE_DIFF);
  check(FIX_MISSING,SG_EXEC_MISSING);
  check(FIX_DIR_ERROR,SG_EXEC_DIR_ERROR);
  check(FIX_OPEN_ERROR,SG_EXEC_OPEN_ERROR);
  check(FIX_READ_ERROR,SG_EXEC_READ_ERROR);
  check(FIX_SMALL_CHUNKS,SG_EXEC_IDENTICAL);
  puts("PASS: EXECUTE.ELF read-only diagnostics all statuses, CRC/size, partial reads");
  return 0;
}

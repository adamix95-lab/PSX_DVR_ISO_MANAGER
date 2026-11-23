/* Sony PSX KELF read-only investigation, v0.3.3.
 * The first complete read is compared byte-by-byte with a pinned original
 * EXECUTE.ELF. A second FIO_O_RDONLY open/read checks repeatability.
 * This does not validate cryptographic signatures or change the strict
 * sg_verify_mounted() used before raw APA writes.
 */
#include "sg_exec_probe.h"
#include <stdint.h>
#include <string.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define SG_EXEC_MAX_BYTES (16u*1024u*1024u)
#define SG_EXEC_NAME "EXECUTE.ELF"
#define SG_EXEC_PATH "pfs0:/" SG_EXEC_NAME
static unsigned char buffer[4096] __attribute__((aligned(64)));

typedef struct {
  uint32_t size, crc, first_difference, different_bytes;
  int first_difference_valid;
} read_result;

static uint32_t crc_update(uint32_t c,const unsigned char *p,uint32_t n) {
  for(uint32_t i=0;i<n;i++){
    c^=p[i];
    for(unsigned b=0;b<8;b++)
      c=(c>>1)^((c&1u)?0xedb88320u:0u);
  }
  return c;
}
static enum sg_exec_status locate(void) {
  int fd=fileXioDopen("pfs0:/");
  if(fd<0)return SG_EXEC_DIR_ERROR;
  iox_dirent_t item __attribute__((aligned(64)));
  int result,found=0;
  while((result=fileXioDread(fd,&item))>0){
    item.name[sizeof(item.name)-1]=0;
    if(!strcmp(item.name,SG_EXEC_NAME))found=1;
  }
  int closed=fileXioDclose(fd);
  if(result<0 || closed<0)return SG_EXEC_DIR_ERROR;
  return found?SG_EXEC_IDENTICAL:SG_EXEC_MISSING;
}
/* 0 complete, -1 open failure, -2 read/close failure. A fresh file
 * descriptor guarantees the repeated scan begins from byte zero. */
static int read_once(const unsigned char *expected,uint32_t expected_size,
                     read_result *result,int compare_bytes) {
  int fd=fileXioOpen(SG_EXEC_PATH,FIO_O_RDONLY,0);
  if(fd<0)return -1;
  uint32_t crc=0xffffffffu;
  uint32_t got=0;
  int n=0,failed=0;
  memset(result,0,sizeof(*result));
  while((n=fileXioRead(fd,buffer,sizeof(buffer)))>0) {
    if(n>(int)sizeof(buffer) || got>SG_EXEC_MAX_BYTES-(uint32_t)n) {
      failed=1;break;
    }
    if(compare_bytes) {
      for(int i=0;i<n;i++) {
        uint32_t offset=got+(uint32_t)i;
        if(offset>=expected_size || buffer[i]!=expected[offset]) {
          if(!result->first_difference_valid) {
            result->first_difference=offset;
            result->first_difference_valid=1;
          }
          result->different_bytes++;
        }
      }
    }
    got+=(uint32_t)n;
    crc=crc_update(crc,buffer,(uint32_t)n);
  }
  int closed=fileXioClose(fd);
  result->size=got;
  if(failed || n<0 || closed<0)return -2;
  if(compare_bytes && got<expected_size) {
    if(!result->first_difference_valid) {
      result->first_difference=got;
      result->first_difference_valid=1;
    }
    result->different_bytes+=expected_size-got;
  }
  result->crc=crc^0xffffffffu;
  return 0;
}
int sg_probe_execute(const unsigned char *expected,uint32_t size,
                     sg_exec_probe *out) {
  if(!out)return -1;
  memset(out,0,sizeof(*out));
  out->expected_size=size;
  out->status=SG_EXEC_READ_ERROR;
  if(!expected || !size || size>SG_EXEC_MAX_BYTES)return 0;
  out->expected_crc=crc_update(0xffffffffu,expected,size)^0xffffffffu;

  enum sg_exec_status located=locate();
  if(located!=SG_EXEC_IDENTICAL){out->status=located;return 0;}

  read_result first={0},second={0};
  int rc=read_once(expected,size,&first,1);
  out->actual_size=first.size;
  if(rc!=0) {
    out->status=rc==-1?SG_EXEC_OPEN_ERROR:SG_EXEC_READ_ERROR;
    return 0;
  }
  out->actual_complete=1;
  out->actual_crc=first.crc;
  out->first_difference=first.first_difference;
  out->different_bytes=first.different_bytes;
  out->first_difference_valid=first.first_difference_valid;
  if(first.size!=size)out->status=SG_EXEC_SIZE_DIFF;
  else if(first.crc!=out->expected_crc)out->status=SG_EXEC_CRC_DIFF;
  else out->status=SG_EXEC_IDENTICAL;

  /* Independent second read is purely diagnostic. Its failure does NOT
   * turn a different/working KELF into an accepted installer payload. */
  rc=read_once(expected,size,&second,0);
  if(rc==0) {
    out->repeat_complete=1;
    out->repeat_size=second.size;
    out->repeat_crc=second.crc;
    out->repeat_stable=(second.size==first.size && second.crc==first.crc);
  } else out->repeat_error=1;
  return 0;
}

/* XMB EXECUTE.ELF read-only diagnostic.
 * This compares two byte strings without claiming to validate the signature
 * of a foreign KELF. An otherwise working XMB shortcut can contain another
 * valid KELF revision; strict installation verification remains unchanged.
 */
#include "sg_exec_probe.h"
#include <stdint.h>
#include <string.h>
#include <limits.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define SG_EXEC_MAX_BYTES (16u*1024u*1024u)
#define SG_EXEC_NAME "EXECUTE.ELF"
#define SG_EXEC_PATH "pfs0:/" SG_EXEC_NAME
static unsigned char buffer[4096] __attribute__((aligned(64)));

static uint32_t crc_update(uint32_t c,const unsigned char *p,uint32_t n) {
  for(uint32_t i=0;i<n;i++){
    c^=p[i];
    for(unsigned b=0;b<8;b++)
      c=(c>>1)^((c&1u)?0xedb88320u:0u);
  }
  return c;
}
/* Only reliable, completed directory enumeration can prove "MISSING".
 * Anything else is an I/O failure and must not be called a missing file. */
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
  int fd=fileXioOpen(SG_EXEC_PATH,FIO_O_RDONLY,0);
  if(fd<0){out->status=SG_EXEC_OPEN_ERROR;return 0;}

  uint32_t crc=0xffffffffu,size_read=0;
  int n,failed=0;
  while((n=fileXioRead(fd,buffer,sizeof(buffer)))>0) {
    if(n>(int)sizeof(buffer) || size_read>SG_EXEC_MAX_BYTES-(uint32_t)n) {
      failed=1;break;
    }
    size_read+=(uint32_t)n;
    crc=crc_update(crc,buffer,(uint32_t)n);
  }
  int closed=fileXioClose(fd);
  out->actual_size=size_read;
  if(failed || n<0 || closed<0){
    out->status=SG_EXEC_READ_ERROR;
    return 0;
  }
  out->actual_complete=1;
  out->actual_crc=crc^0xffffffffu;
  if(size_read!=out->expected_size)out->status=SG_EXEC_SIZE_DIFF;
  else if(out->actual_crc!=out->expected_crc)out->status=SG_EXEC_CRC_DIFF;
  else out->status=SG_EXEC_IDENTICAL;
  return 0;
}

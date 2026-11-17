#include "fileXio_rpc.h"
#include "n6_usb_art.h"
#include "runtime_storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int index_in_dir, enumerating, resumed, active_iso, lba;
static void put32(unsigned char *p,unsigned n) {
  for(int i=0;i<4;i++)p[i]=(unsigned char)(n>>(i*8));
}
int fileXioDopen(const char *path) {
  assert(!strcmp(path,"dvr_pfs0:/ISO"));
  assert(!enumerating);enumerating=1;index_in_dir=0;return 2;
}
int fileXioDread(int fd,iox_dirent_t *entry) {
  assert(fd==2 && enumerating);
  if(index_in_dir==60)return 0;
  snprintf(entry->name,sizeof(entry->name),"G%02d.iso",index_in_dir++);
  return 1;
}
int fileXioDclose(int fd) {assert(fd==2 && enumerating);enumerating=0;return 0;}
int fileXioOpen(const char *path,int flags,int mode) {
  int id;
  (void)mode;
  assert(flags==FIO_O_RDONLY && !enumerating);
  if(sscanf(path,"dvr_pfs0:/ISO/G%d.iso",&id)==1) {active_iso=id;return 3;}
  assert(sscanf(path,DESR_APP_ART_ROOT "SLUS_500.%d",&id)==1);
  if(!resumed)return -1;
  const char *ext=strstr(path,"_COV.");
  assert(ext);
  if(!strcmp(ext,"_COV.png") && id<50)return 4;
  if(!strcmp(ext,"_COV.jpg") && id==50)return 4;
  if(!strcmp(ext,"_COV.png.PART") && id==51)return 4;
  return -1;
}
int fileXioClose(int fd) {assert(fd==3 || fd==4);return 0;}
int fileXioLseek(int fd,int offset,int whence) {
  assert(fd==3 && whence==SEEK_SET && offset%2048==0);lba=offset/2048;return offset;
}
int fileXioRead(int fd,void *data,int size) {
  unsigned char *p=data;
  assert(fd==3 && size==2048);
  memset(p,0,2048);
  if(lba==16) {
    p[0]=1;memcpy(p+1,"CD001",5);p[6]=1;p[156]=34;p[188]=1;
    put32(p+158,32);put32(p+166,2048);
  }else{
    char id[16];
    assert(lba==32);
    int n=snprintf(id,sizeof(id),"SLUS_500.%02d;1",active_iso);
    assert(n==13);
    p[0]=46;p[32]=13;memcpy(p+33,id,13);put32(p+2,33);put32(p+10,2048);
  }
  return size;
}
int main(void) {
  char ids[N6_BATCH_MAX][12];
  int iso,unique,missing;
  assert(n6_scan_dvr(ids,&iso,&unique,&missing)==50);
  assert(iso==60 && unique==60 && missing==60);
  assert(!strcmp(ids[0],"SLUS_500.00") && !strcmp(ids[49],"SLUS_500.49"));
  resumed=1;
  assert(n6_scan_dvr(ids,&iso,&unique,&missing)==8);
  assert(iso==60 && unique==60 && missing==9);
  assert(!strcmp(ids[0],"SLUS_500.52") && !strcmp(ids[7],"SLUS_500.59"));
  puts("PASS scanner: 50 of 60 selected, repeat skips existing PNG/JPG/PART");
}

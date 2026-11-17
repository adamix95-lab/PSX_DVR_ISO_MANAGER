#include "fileXio_rpc.h"
#include "n6_usb_art.h"
#include "runtime_storage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const unsigned char png[] = {
  137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,
  31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,16,84,50,254,15,0,2,20,1,
  102,4,224,222,127,0,0,0,0,73,69,78,68,174,66,96,130
};
enum {PNG,JPG,PART};
enum {NORMAL,CORRUPT,EXTRA,RACE_PART,RACE_FINAL,IO_ERR};
static const char id[] = "SLUS_203.12";
static int stat_status[3], mode, creates, writes, renames, at, size;
static unsigned char on_disk[sizeof(png)+1];

static int path_type(const char *path) {
  static const char cover[] = DESR_APP_ART_ROOT "SLUS_203.12_COV";
  assert(strncmp(path, cover, sizeof(cover)-1)==0);
  if(strstr(path,".PART"))return PART;
  return strstr(path,".jpg")?JPG:PNG;
}
static void reset(int scenario) {
  for(int i=0;i<3;i++)stat_status[i]=-4;
  mode=scenario; creates=writes=renames=at=size=0;
}
int fileXioGetStat(const char *path,iox_stat_t *s) {
  (void)s;
  return stat_status[path_type(path)];
}
int fileXioOpen(const char *path,int flags,int m) {
  (void)m;
  assert(path_type(path)==PART);
  if(flags==FIO_O_RDONLY){at=0;return stat_status[PART]==0?4:-4;}
  assert(flags==(FIO_O_WRONLY|FIO_O_CREAT|FIO_O_EXCL));
  creates++;
  if(mode==RACE_PART)stat_status[PART]=0;
  if(stat_status[PART]==0)return -8;
  stat_status[PART]=0;
  return 3;
}
int fileXioClose(int fd){assert(fd==3||fd==4);return 0;}
int fileXioWrite(int fd,const void *data,int n){
  assert(fd==3);
  if(n>31)n=31;
  memcpy(on_disk+size,data,n);
  size+=n;writes++;
  return mode==IO_ERR?-1:n;
}
int fileXioRead(int fd,void *data,int n){
  assert(fd==4);
  if(at==size&&mode==EXTRA)return 1;
  if(n>13)n=13;
  if(n>size-at)n=size-at;
  memcpy(data,on_disk+at,n);
  if(mode==CORRUPT&&n)((unsigned char*)data)[0]^=1;
  at+=n;
  if(at==size&&mode==RACE_FINAL)stat_status[PNG]=0;
  return n;
}
int fileXioDopen(const char *p){assert(!strcmp(p,DESR_APP_ROOT "ART"));return 2;}
int fileXioDclose(int fd){assert(fd==2);return 0;}
int fileXioMkdir(const char *p,int mode){(void)p;(void)mode;assert(0);return -1;}
int fileXioDread(int fd,iox_dirent_t *e){(void)fd;(void)e;assert(0);return -1;}
int fileXioLseek(int fd,int offset,int whence){(void)fd;(void)offset;(void)whence;assert(0);return -1;}
int fileXioRename(const char *src,const char *dst){
  assert(path_type(src)==PART&&path_type(dst)==PNG);
  assert(stat_status[PNG]!=0&&stat_status[PART]==0);
  assert(at==sizeof(png)&&!memcmp(on_disk,png,sizeof(png)));
  stat_status[PNG]=0;stat_status[PART]=-4;renames++;
  return 0;
}
int main(void) {
  for(int i=0;i<3;i++){
    reset(NORMAL);stat_status[i]=0;
    assert(n6_write_usb_art(id,png,sizeof(png))==-552);
    assert(!creates&&!writes&&!renames);
  }
  reset(NORMAL);stat_status[PART]=-1;
  assert(n6_write_usb_art(id,png,sizeof(png))==-552&&!creates);
  reset(RACE_PART);
  assert(n6_write_usb_art(id,png,sizeof(png))==-554&&!writes&&!renames);
  reset(CORRUPT);
  assert(n6_write_usb_art(id,png,sizeof(png))==-558&&!renames);
  reset(EXTRA);
  assert(n6_write_usb_art(id,png,sizeof(png))==-559&&!renames);
  reset(RACE_FINAL);
  assert(n6_write_usb_art(id,png,sizeof(png))==-552&&!renames&&stat_status[PART]==0);
  reset(IO_ERR);
  assert(n6_write_usb_art(id,png,sizeof(png))==-555&&!renames&&stat_status[PART]==0);
  reset(NORMAL);
  assert(!n6_write_usb_art(id,png,sizeof(png)));
  assert(creates==1&&writes>1&&renames==1&&stat_status[PNG]==0&&stat_status[PART]==-4);
  puts("PASS N6G writer: existing PNG/JPG/PART, stat failure, creation/publish races, write/readback errors, verified publish");
}

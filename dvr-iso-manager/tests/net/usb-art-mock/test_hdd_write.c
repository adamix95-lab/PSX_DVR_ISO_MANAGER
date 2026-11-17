/* HDD12 PFS1 ART writer host model.
 * Tests no-overwrite guarantees, .PART readback, PFS sync and final stat.
 * Does not pretend to validate PSX hardware PFS/IOP behavior.
 */
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
enum {PNG=0,JPG=1,PART=2};
enum {NORMAL=0,CORRUPT,EXTRA,RACE_PART,RACE_FINAL,IO_ERR,SYNC_ERR,DIR_ERR};
static const char id[]="SLUS_203.12";
static const char *file_names[]={
 "SLUS_203.12_COV.png","SLUS_203.12_COV.jpg","SLUS_203.12_COV.png.PART"
};
static int present[3],scenario,created,writes,renamed,synced,dir_index,at,file_size;
static unsigned char bytes[sizeof(png)+1];

static int which(const char *path) {
  assert(strncmp(path,DESR_APP_ART_ROOT,strlen(DESR_APP_ART_ROOT))==0);
  const char *name=path+strlen(DESR_APP_ART_ROOT);
  for(int i=0;i<3;i++)if(!strcmp(name,file_names[i]))return i;
  assert(0);return -1;
}
static void reset(int sc) {
  memset(present,0,sizeof(present));
  scenario=sc;created=writes=renamed=synced=dir_index=at=file_size=0;
  memset(bytes,0,sizeof(bytes));
}
int fileXioDopen(const char *path) {
  assert(!strcmp(path,DESR_APP_ROOT "ART"));
  if(scenario==DIR_ERR)return -3;
  dir_index=0;return 2;
}
int fileXioDread(int fd,iox_dirent_t *ent) {
  assert(fd==2);
  while(dir_index<3) {
    int j=dir_index++;
    if(!present[j])continue;
    snprintf(ent->name,sizeof(ent->name),"%s",file_names[j]);return 1;
  }
  return 0;
}
int fileXioDclose(int fd) {assert(fd==2);return 0;}
int fileXioMkdir(const char *path,int mode) {
  (void)mode;assert(!strcmp(path,DESR_APP_ROOT "ART"));
  return -3;
}
int fileXioOpen(const char *path,int flags,int mode) {
  (void)mode; assert(which(path)==PART);
  if(flags==FIO_O_RDONLY){at=0;return present[PART]?4:-2;}
  assert(flags==(FIO_O_WRONLY|FIO_O_CREAT|FIO_O_EXCL));
  created++;
  if(scenario==RACE_PART)present[PART]=1;
  if(present[PART])return -8;
  present[PART]=1;return 3;
}
int fileXioClose(int fd) {assert(fd==3||fd==4);return 0;}
int fileXioWrite(int fd,const void *p,int n) {
  assert(fd==3);if(n>31)n=31;
  if(scenario==IO_ERR)return -1;
  assert(file_size+n <= (int)sizeof(bytes));
  memcpy(bytes+file_size,p,(size_t)n);file_size+=n;writes++;
  return n;
}
int fileXioRead(int fd,void *p,int n) {
  assert(fd==4);
  if(at==file_size && scenario==EXTRA)return 1;
  if(n>13)n=13;
  if(n>file_size-at)n=file_size-at;
  assert(n>=0);
  memcpy(p,bytes+at,(size_t)n);
  if(scenario==CORRUPT && n)((unsigned char *)p)[0]^=0x20;
  at+=n;
  if(at==file_size && scenario==RACE_FINAL)present[PNG]=1;
  return n;
}
int fileXioRename(const char *src,const char *dst) {
  assert(which(src)==PART&&which(dst)==PNG);
  assert(present[PART] && !present[PNG]);
  assert(file_size==(int)sizeof(png));
  assert(!memcmp(bytes,png,sizeof(png)));
  present[PART]=0;present[PNG]=1;renamed++;
  return 0;
}
int fileXioSync(const char *path,int flags) {
  assert(!strcmp(path,"pfs1:") && flags==FXIO_WAIT);
  synced++;return scenario==SYNC_ERR?-1:0;
}
int fileXioGetStat(const char *path,iox_stat_t *st) {
  assert(which(path)==PNG);
  if(!present[PNG])return -2;
  st->size=(unsigned)file_size;
  return 0;
}
int fileXioLseek(int fd,int off,int whence) {
  (void)fd;(void)off;(void)whence;assert(0);return -1;
}
int main(void) {
  for(int i=0;i<3;i++) {
    reset(NORMAL);present[i]=1;
    assert(n6_write_usb_art(id,png,sizeof(png))==-552);
    assert(!created && !renamed);
  }
  reset(RACE_PART);
  assert(n6_write_usb_art(id,png,sizeof(png))==-554);
  assert(created==1 && !renamed);
  reset(CORRUPT);
  assert(n6_write_usb_art(id,png,sizeof(png))==-558);
  assert(synced==1 && !renamed);
  reset(EXTRA);
  assert(n6_write_usb_art(id,png,sizeof(png))==-559);
  assert(!renamed);
  reset(RACE_FINAL);
  assert(n6_write_usb_art(id,png,sizeof(png))==-552);
  assert(!renamed && present[PART] && present[PNG]);
  reset(IO_ERR);
  assert(n6_write_usb_art(id,png,sizeof(png))==-555);
  assert(!renamed);
  reset(SYNC_ERR);
  assert(n6_write_usb_art(id,png,sizeof(png))==-571);
  assert(!renamed);
  reset(DIR_ERR);
  assert(n6_write_usb_art(id,png,sizeof(png))==-553);
  assert(!created && !renamed);
  reset(NORMAL);
  assert(n6_write_usb_art(id,png,sizeof(png))==0);
  assert(created==1 && writes>1 && renamed==1 && synced==2);
  assert(present[PNG] && !present[PART]);
  puts("PASS HDD12 ART: PFS1 paths, no overwrite, full readback, exclusive PART, sync, rename, final stat, fail-closed");
  return 0;
}

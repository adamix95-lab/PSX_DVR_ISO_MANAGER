/* N6: verified, bounded DVR ISO metadata read and isolated USB ART output.
 * NEVER writes to DVR, APA or PFS, and NEVER guesses Game IDs from titles.
 * Reuses the Manager 1.0 ISO9660/BOOT2 validators (cover_index.c).
 * No network code here. No existing art or PART is ever overwritten.
 */
#include "n6_usb_art.h"
#include "cover_index.h"
#include "iso_utils.h"
#include "probe_png.h"
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#define DVR_DIR "dvr_pfs0:/ISO"
#define USB_DIR "mass0:/ART"
#define SECTOR 2048U
#define GAME_LIMIT DESR_GAME_CAP

static unsigned char sector[SECTOR];
static char cnf[8192];
static char seen[GAME_LIMIT][DESR_COVER_ID_CAP];
static char scanned_iso[GAME_LIMIT][DESR_FILENAME_CAP];
static unsigned char verify_chunk[1024];

static int read_at(int fd,uint32_t offset,void *dst,int bytes) {
  int done=0,n;
  if(offset>INT_MAX || bytes<0 || bytes>INT_MAX-(int)offset)return -1;
  if(fileXioLseek(fd,(int)offset,SEEK_SET)!=(int)offset)return -1;
  while(done<bytes) {
    n=fileXioRead(fd,(uint8_t*)dst+done,bytes-done);
    if(n<=0)return -1;
    done+=n;
  }
  return 0;
}
static int sector_at(int fd,uint32_t lba) {
  if(lba>(uint32_t)(INT_MAX/(int)SECTOR))return -1;
  return read_at(fd,lba*SECTOR,sector,(int)SECTOR);
}
/* 1=identified, 0=unidentifiable, -1=physical/driver read error.
 * Strict BOOT2 priority; root executable fallback only if unambiguous. */
static int id_from_iso(const char *filename,char id[12]) {
  char path[256],candidate[12]={0};
  uint32_t root_lba=0,root_size=0,cnf_lba=0,cnf_size=0;
  int fd,found_pvd=0,found_cnf=0,got_candidate=0,ambiguous=0,rc=0;
  id[0]=0;
  if(desr_make_iso_path(filename,path,sizeof(path))<0)return 0;
  fd=fileXioOpen(path,FIO_O_RDONLY,0);
  if(fd<0)return -1;
  for(int i=16;i<32;i++) {
    if(sector_at(fd,(uint32_t)i)<0){rc=-1;goto done;}
    if(memcmp(sector+1,"CD001",5))continue;
    if(sector[0]==255)break;
    if(sector[0]==1) {
      if(desr_iso_root(sector,SECTOR,&root_lba,&root_size)<0)goto done;
      found_pvd=1;break;
    }
  }
  if(!found_pvd || !root_size || root_size>262144U)goto done;
  for(uint32_t i=0;i<(root_size+SECTOR-1U)/SECTOR;i++) {
    int a,b;
    char local_id[12];
    if(root_lba>(uint32_t)(INT_MAX/(int)SECTOR)-i)goto done;
    if(sector_at(fd,root_lba+i)<0){rc=-1;goto done;}
    a=desr_iso_system_record(sector,SECTOR,&cnf_lba,&cnf_size);
    b=desr_iso_root_boot_candidate(sector,SECTOR,local_id);
    if(a<0||b<0)goto done;
    if(a==1)found_cnf=1;
    if(b==2)ambiguous=1;
    if(b==1) {
      if(got_candidate && strcmp(candidate,local_id))ambiguous=1;
      else {memcpy(candidate,local_id,sizeof(candidate));got_candidate=1;}
    }
  }
  if(found_cnf && cnf_size && cnf_size<=sizeof(cnf) &&
     cnf_lba<=(uint32_t)(INT_MAX/(int)SECTOR)) {
    if(read_at(fd,cnf_lba*SECTOR,cnf,(int)cnf_size)<0){
      rc=-1;goto done;
    }
    if(desr_cnf_game_id(cnf,cnf_size,id)) {rc=1;goto done;}
  }
  if(!ambiguous && got_candidate) {
    memcpy(id,candidate,12);rc=1;
  }
done:
  fileXioClose(fd);
  return rc;
}
static int exists(const char *path) {
  int fd=fileXioOpen(path,FIO_O_RDONLY,0);
  if(fd<0)return 0;
  fileXioClose(fd);
  return 1;
}
static int paths(const char *id,char dst[72],char jpg[72],char part[80]){
  int n;
  if(!desr_valid_game_id(id))return -1;
  n=snprintf(dst,72,USB_DIR "/%s_COV.png",id);
  if(n<0||n>=72)return -1;
  n=snprintf(jpg,72,USB_DIR "/%s_COV.jpg",id);
  if(n<0||n>=72)return -1;
  n=snprintf(part,80,USB_DIR "/%s_COV.png.PART",id);
  if(n<0||n>=80)return -1;
  return 0;
}
int n6_usb_cover_present(const char id[12]) {
  char png[72],jpg[72],tmp[80];
  if(paths(id,png,jpg,tmp)<0)return -1;
  return exists(png)||exists(jpg);
}
/* Match stable Manager v1.0 behavior: finish DVR directory enumeration
 * FIRST, then open individual ISOs sequentially, never while dir is open.
 * This avoids relying on DVR module support for overlapping dir/file FDs.
 */
int n6_scan_dvr(char ids[N6_BATCH_MAX][12],
                int *iso_count,int *ids_count,int *missing_count){
  int dir,n,closed,seen_count=0,chosen=0;
  iox_dirent_t ent;
  if(!ids||!iso_count||!ids_count||!missing_count)return -1;
  *iso_count=*ids_count=*missing_count=0;
  memset(seen,0,sizeof(seen));
  dir=fileXioDopen(DVR_DIR);
  if(dir<0)return -530;
  while((n=fileXioDread(dir,&ent))>0) {
    size_t size;
    ent.name[sizeof(ent.name)-1]=0;
    if(!desr_filename_is_safe(ent.name))continue;
    if(*iso_count>=GAME_LIMIT){fileXioDclose(dir);return -531;}
    size=strlen(ent.name);
    if(size>=DESR_FILENAME_CAP)continue;
    memcpy(scanned_iso[*iso_count],ent.name,size+1U);
    (*iso_count)++;
  }
  closed=fileXioDclose(dir);
  if(closed<0||n<0)return -535;
  for(int index=0;index<*iso_count;index++) {
    char id[12],png[72],jpg[72],part[80];
    int rc,duplicate=0,present;
    rc=id_from_iso(scanned_iso[index],id);
    if(rc<0)return -532;
    if(rc==0)continue;
    (*ids_count)++;
    for(int i=0;i<seen_count;i++)
      if(!strcmp(id,seen[i])){duplicate=1;break;}
    if(duplicate)continue;
    memcpy(seen[seen_count++],id,12);
    present=n6_usb_cover_present(id);
    if(present<0)return -533;
    if(present)continue;
    (*missing_count)++;
    if(chosen<N6_BATCH_MAX) {
      if(paths(id,png,jpg,part)<0)return -534;
      if(!exists(part))
        memcpy(ids[chosen++],id,12);
    }
  }
  return chosen;
}
/* Final USB-only publish. Incomplete/old PART is left untouched for
 * manual inspection. Always compare staged data with verified RAM bytes.
 * Existing .png/.jpg are never replaced, even on failed prior downloads.
 */
#ifndef N6A_READONLY
int n6_write_usb_art(const char id[12],const uint8_t *png,size_t n) {
  char dst[72],jpg[72],part[80];
  int dir,fd=-1,w=0,ret;
  if(!png||!n||n>N6_COVER_CAP||probe_png_check(png,n)!=0)return -550;
  if(paths(id,dst,jpg,part)<0)return -551;
  if(exists(dst)||exists(jpg)||exists(part))return -552;
  dir=fileXioDopen(USB_DIR);
  if(dir>=0)fileXioDclose(dir);
  else {
    ret=fileXioMkdir(USB_DIR,0777);
    if(ret<0)return -553;
    dir=fileXioDopen(USB_DIR);
    if(dir<0)return -553;
    fileXioDclose(dir);
  }
  fd=fileXioOpen(part,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_TRUNC,0666);
  if(fd<0)return -554;
  while((size_t)w<n) {
    int chunk=(int)(n-(size_t)w);
    if(chunk>4096)chunk=4096;
    ret=fileXioWrite(fd,png+w,chunk);
    if(ret<=0){fileXioClose(fd);return -555;}
    w+=ret;
  }
  if(fileXioClose(fd)<0)return -556;
  fd=fileXioOpen(part,FIO_O_RDONLY,0);
  if(fd<0)return -557;
  w=0;
  while((size_t)w<n) {
    int chunk=(int)(n-(size_t)w);
    if(chunk>(int)sizeof(verify_chunk))chunk=sizeof(verify_chunk);
    ret=fileXioRead(fd,verify_chunk,chunk);
    if(ret<=0||memcmp(verify_chunk,png+w,(size_t)ret)){
      fileXioClose(fd);return -558;
    }
    w+=ret;
  }
  ret=fileXioRead(fd,verify_chunk,1);
  if(fileXioClose(fd)<0||ret!=0)return -559;
  if(exists(dst)||exists(jpg))return -552;
  if(fileXioRename(part,dst)<0)return -560;
  return 0;
}

#endif /* N6A_READONLY: read-only scan helper only */

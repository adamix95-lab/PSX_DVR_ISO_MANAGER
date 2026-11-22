/* Read the live DVR ISO boot ID without trusting COVERS.IDX.
 * Algorithm derived from PDIM 1.2 cover_runtime's proven ISO9660 scanner.
 * No file writes or mount changes; reused exactly in SQUARE, X and TRIANGLE.
 */
#include "sg_installer.h"
#include "sg_policy.h"
#include "iso_utils.h"
#include "cover_index.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define S 2048u
static unsigned char block[S];
static char system_cnf[8192];
static int read_bytes(int fd,uint32_t pos,void *dst,int n) {
  if(pos>INT_MAX || n<0 || (uint32_t)n>(uint32_t)INT_MAX-pos)return -1;
  int seek=fileXioLseek(fd,(int)pos,SEEK_SET);
  if(seek!=(int)pos)return -1;
  int got=0;
  while(got<n) {
    int r=fileXioRead(fd,(char*)dst+got,n-got);
    if(r<=0||r>n-got)return -1;
    got+=r;
  }
  return 0;
}
static int sector_read(int fd,uint32_t sector) {
  if(sector>INT_MAX/(int)S)return -1;
  return read_bytes(fd,sector*S,block,S);
}
int sg_check_iso_id(const sg_game *g) {
  if(!g || !sg_iso_filename_allowed(g->filename) ||
     !desr_valid_game_id(g->game_id))return -1;
  char path[256];
  if(desr_make_iso_path(g->filename,path,sizeof(path))<0)return -1;
  int fd=fileXioOpen(path,FIO_O_RDONLY,0);
  if(fd<0)return -1;
  int result=-1;
  uint32_t root_lba=0,root_bytes=0,cnf_lba=0,cnf_bytes=0;
  char fallback[DESR_COVER_ID_CAP]={0};
  int found_pvd=0,found_cnf=0,found_fallback=0,ambiguous=0;

  for(uint32_t i=16;i<=31;i++){
    if(sector_read(fd,i)<0)goto done;
    if(memcmp(block+1,"CD001",5))continue;
    if(block[0]==255)break;
    if(block[0]==1){
      if(desr_iso_root(block,S,&root_lba,&root_bytes)<0)goto done;
      found_pvd=1;break;
    }
  }
  if(!found_pvd||!root_lba||root_bytes>262144u)goto done;
  int sectors=(int)((root_bytes+S-1u)/S);
  for(int i=0;i<sectors;i++){
    uint32_t got_lba=root_lba+(uint32_t)i;
    if(got_lba<root_lba||sector_read(fd,got_lba)<0)goto done;
    uint32_t cur_lba=0,cur_size=0;
    char name[DESR_COVER_ID_CAP]={0};
    int sy=desr_iso_system_record(block,S,&cur_lba,&cur_size);
    int candidate=desr_iso_root_boot_candidate(block,S,name);
    if(sy<0||candidate<0)goto done;
    if(sy==1){
      if(found_cnf && (cnf_lba!=cur_lba||cnf_bytes!=cur_size))goto done;
      found_cnf=1;cnf_lba=cur_lba;cnf_bytes=cur_size;
    }
    if(candidate==2)ambiguous=1;
    else if(candidate==1){
      if(found_fallback && strcmp(fallback,name))ambiguous=1;
      else{memcpy(fallback,name,sizeof(fallback));found_fallback=1;}
    }
  }
  char actual[DESR_COVER_ID_CAP]={0};
  if(found_cnf&&cnf_bytes>0&&cnf_bytes<=sizeof(system_cnf) &&
     cnf_lba<=INT_MAX/(int)S &&
     read_bytes(fd,cnf_lba*S,system_cnf,(int)cnf_bytes)==0 &&
     desr_cnf_game_id(system_cnf,cnf_bytes,actual)){
       result=strcmp(actual,g->game_id)?-2:1;
  }else if(!ambiguous && found_fallback) {
    result=strcmp(fallback,g->game_id)?-2:1;
  }
done:
  fileXioClose(fd);
  return result;
}

/* PDIM XMB installer 0.3: bounded, read-only multi-method ISO Game ID probe.
 * The original DVR store is dvr_pfs0:/ISO, NOT the PP.DVRISO pfs1 mount.
 *
 * Methods: ISO9660 2048-byte sectors; raw 2352-byte Mode1 (+16) / Mode2
 * (+24); BOOT2 in SYSTEM.CNF; unique root executable; COVERS.IDX fallback
 * through the caller. All actual IDs are validated with PDIM cover_index.c.
 *
 * Sony DVR drivers can refuse fileXioLseek on otherwise playable ISO files.
 * Retry via a fresh descriptor and bounded sequential fileXioRead. If reads
 * still fail but open succeeded, do not invent an ID or return fatal -62:
 * the caller uses its exact verified COVERS.IDX entry and displays warning.
 * A genuinely missing/unopenable ISO or a positively detected mismatched
 * ID still blocks writes.
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

#define ISO_BLOCK 2048u
#define ISO_MAX_DIR 262144u
#define ISO_MAX_CNF 8192u
#define ISO_STREAM_LIMIT (8u*1024u*1024u)

typedef struct {
  char path[256];
  int fd;
  int sequential;
  uint32_t position;
} iso_io;
static unsigned char sector_buf[ISO_BLOCK];
static unsigned char skip_buf[ISO_BLOCK];
static char system_cnf[ISO_MAX_CNF];

static int open_image(iso_io *io, const char *path) {
  if (!io || !path)return -1;
  memset(io,0,sizeof(*io));
  if(snprintf(io->path,sizeof(io->path),"%s",path)>=(int)sizeof(io->path))
    return -1;
  io->fd=fileXioOpen(io->path,FIO_O_RDONLY,0);
  return io->fd<0?-1:0;
}
static int reopen_sequential(iso_io *io) {
  if(io->fd>=0)fileXioClose(io->fd);
  io->fd=fileXioOpen(io->path,FIO_O_RDONLY,0);
  if(io->fd<0)return -1;
  io->sequential=1;
  io->position=0;
  return 0;
}
static int exact_read(iso_io *io,void *out,uint32_t length) {
  uint8_t *p=out;
  while(length) {
    int amount=length>ISO_BLOCK?(int)ISO_BLOCK:(int)length;
    int got=fileXioRead(io->fd,p,amount);
    if(got<=0||got>amount)return -1;
    io->position+=(uint32_t)got;p+=got;length-=(uint32_t)got;
  }
  return 0;
}
static int sequential_read_at(iso_io *io,uint32_t offset,void *dst,uint32_t count) {
  if(offset+count>ISO_STREAM_LIMIT || offset+count<offset)return -1;
  if(offset<io->position && reopen_sequential(io)<0)return -1;
  while(io->position<offset) {
    uint32_t remaining=offset-io->position;
    uint32_t chunk=remaining>ISO_BLOCK?ISO_BLOCK:remaining;
    if(exact_read(io,skip_buf,chunk)<0)return -1;
  }
  return exact_read(io,dst,count);
}
static int read_at(iso_io *io,uint32_t offset,void *dst,uint32_t count) {
  if(!io||io->fd<0||!dst||!count || offset>INT_MAX-count)return -1;
  if(io->sequential)return sequential_read_at(io,offset,dst,count);
  int at=fileXioLseek(io->fd,(int)offset,SEEK_SET);
  if(at!=(int)offset) {
    if(reopen_sequential(io)<0)return -1;
    return sequential_read_at(io,offset,dst,count);
  }
  io->position=offset;
  if(exact_read(io,dst,count)==0)return 0;
  /* Some DVR implementations claim seek success but reject the next read. */
  if(reopen_sequential(io)<0)return -1;
  return sequential_read_at(io,offset,dst,count);
}
static void close_image(iso_io *io) {
  if(io->fd>=0){fileXioClose(io->fd);io->fd=-1;}
}
static int read_user_block(iso_io *io,uint32_t block,uint32_t stride,
                           uint32_t data_offset,void *dest) {
  uint64_t pos=(uint64_t)block*stride+data_offset;
  if(pos>INT_MAX-ISO_BLOCK)return -1;
  return read_at(io,(uint32_t)pos,dest,ISO_BLOCK);
}
static int scan_layout(const char *path,uint32_t stride,uint32_t data_offset,
                       char id[DESR_COVER_ID_CAP]) {
  iso_io io;
  uint32_t root_lba=0,root_bytes=0,cnf_lba=0,cnf_bytes=0;
  int found_pvd=0,found_cnf=0,have_candidate=0,ambiguous=0;
  int result=0;
  char candidate[DESR_COVER_ID_CAP]={0};
  if(open_image(&io,path)<0)return -1;
  for(uint32_t i=16;i<=31;i++) {
    if(read_user_block(&io,i,stride,data_offset,sector_buf)<0) {
      result=2;goto done;
    }
    if(memcmp(sector_buf+1,"CD001",5))continue;
    if(sector_buf[0]==255)break;
    if(sector_buf[0]==1) {
      if(desr_iso_root(sector_buf,ISO_BLOCK,&root_lba,&root_bytes)<0)
        goto done;
      found_pvd=1;
      break;
    }
  }
  if(!found_pvd || !root_lba || !root_bytes || root_bytes>ISO_MAX_DIR)
    goto done;

  for(uint32_t i=0;i<(root_bytes+ISO_BLOCK-1u)/ISO_BLOCK;i++) {
    uint32_t lba=root_lba+i,entry_lba=0,entry_bytes=0;
    if(lba<root_lba)goto done;
    if(read_user_block(&io,lba,stride,data_offset,sector_buf)<0) {
      result=2;goto done;
    }
    char boot_name[DESR_COVER_ID_CAP]={0};
    int sys=desr_iso_system_record(sector_buf,ISO_BLOCK,
                                   &entry_lba,&entry_bytes);
    int boot=desr_iso_root_boot_candidate(sector_buf,ISO_BLOCK,boot_name);
    if(sys<0 || boot<0)goto done;
    if(sys==1) {
      if(found_cnf && (cnf_lba!=entry_lba||cnf_bytes!=entry_bytes))
        goto done;
      found_cnf=1;cnf_lba=entry_lba;cnf_bytes=entry_bytes;
    }
    if(boot==2)ambiguous=1;
    else if(boot==1) {
      if(have_candidate && strcmp(candidate,boot_name))ambiguous=1;
      else {memcpy(candidate,boot_name,sizeof(candidate));have_candidate=1;}
    }
  }
  /* SYSTEM.CNF has priority over the unique root executable. */
  if(found_cnf && cnf_bytes>0 && cnf_bytes<=ISO_MAX_CNF) {
    uint64_t start=(uint64_t)cnf_lba*stride+data_offset;
    if(start>INT_MAX-cnf_bytes)goto done;
    if(read_at(&io,(uint32_t)start,system_cnf,cnf_bytes)<0) {
      result=2;goto done;
    }
    if(desr_cnf_game_id(system_cnf,cnf_bytes,id)) {
      result=1;goto done;
    }
  }
  if(!ambiguous && have_candidate) {
    memcpy(id,candidate,DESR_COVER_ID_CAP);
    result=1;
  }
done:
  close_image(&io);
  return result;
}

/* 1: actual valid Game ID recovered; 0: not found; 2: access works but
 * random seeks/sector reads unsupported; -1: ISO does not open.
 * This function intentionally does not use the cover index. */
int sg_probe_iso_id(const char *filename,char out[SG_ID]) {
  static const uint32_t layout[][2] = {
    {2048u,0u}, {2352u,16u}, {2352u,24u}
  };
  char path[256];
  int reader_failed=0;
  if(!out)return -1;
  out[0]=0;
  if(!sg_iso_filename_allowed(filename) ||
     desr_make_iso_path(filename,path,sizeof(path))<0)return -1;
  /* Separate open probe: a nonexistent game must never use cached metadata. */
  int fd=fileXioOpen(path,FIO_O_RDONLY,0);
  if(fd<0)return -1;
  fileXioClose(fd);
  for(unsigned i=0;i<sizeof(layout)/sizeof(layout[0]);i++) {
    char found[SG_ID]={0};
    int r=scan_layout(path,layout[i][0],layout[i][1],found);
    if(r==1) {
      if(!desr_valid_game_id(found))continue;
      memcpy(out,found,SG_ID);
      return 1;
    }
    if(r==2 || r<0)reader_failed=1;
  }
  return reader_failed?2:0;
}
int sg_check_iso_id(const sg_game *g) {
  char actual[SG_ID]={0};
  if(!g || !desr_valid_game_id(g->game_id))return SG_ISO_ID_IO_ERROR;
  int rc=sg_probe_iso_id(g->filename,actual);
  if(rc<0)return SG_ISO_ID_IO_ERROR;
  if(rc==2)return SG_ISO_ID_INDEX_READER;
  if(rc==0)return SG_ISO_ID_INDEX_ONLY;
  return strcmp(actual,g->game_id) ?
    SG_ISO_ID_MISMATCH : SG_ISO_ID_MATCH;
}

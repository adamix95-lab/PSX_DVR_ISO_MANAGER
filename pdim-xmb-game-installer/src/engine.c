/* Standalone per-game XMB installer.
 * All target writes are restricted to one NEW PP.<title>-<GAMEID> PFS.
 * The existing PP.DVRISO and DVR ISO filesystem are ALWAYS read-only.
 */
#include "sg_installer.h"
#include "iso_utils.h"
#include "cover_index.h"
#include "sg_policy.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <hdd-ioctl.h>
#include <debug.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

extern unsigned char execute_kelf[];
extern unsigned int size_execute_kelf;
extern unsigned char osd_launcher[];
extern unsigned int size_osd_launcher;

sg_game sg_games[SG_COUNT];
int sg_count;
static char idx[24576] __attribute__((aligned(64)));
static unsigned char io_buf[4096] __attribute__((aligned(64)));
static int src_mounted, dst_mounted;

static uint32_t crc_update(uint32_t crc,const unsigned char *p,int n) {
  for(int i=0;i<n;i++) {
    crc^=p[i];
    for(int b=0;b<8;b++) crc=(crc>>1) ^ ((crc&1)?0xedb88320u:0u);
  }
  return crc;
}
uint32_t sg_crc_bytes(const void *src,int n) {
  return crc_update(0xffffffffu,(const unsigned char*)src,n)^0xffffffffu;
}
int sg_crc_file(const char *p,uint32_t *sz,uint32_t *hash) {
  int fd=fileXioOpen(p,FIO_O_RDONLY,0);
  if(fd<0) return -1;
  uint32_t got=0,c=0xffffffffu;
  int n;
  while((n=fileXioRead(fd,io_buf,sizeof(io_buf)))>0) {
    if(got>16*1024*1024u-(uint32_t)n){fileXioClose(fd);return -1;}
    got+=(uint32_t)n;
    c=crc_update(c,io_buf,n);
  }
  int closed=fileXioClose(fd);
  if(n<0||closed<0)return -1;
  *sz=got;*hash=c^0xffffffffu;
  return 0;
}
static int match_file(const char *file,const void *expected,size_t len) {
  char full[160];
  if(snprintf(full,sizeof(full),"pfs0:/%s",file)>=(int)sizeof(full))return -1;
  uint32_t got=0,crc=0;
  if(sg_crc_file(full,&got,&crc)<0||got!=len)return -1;
  return crc==sg_crc_bytes(expected,(int)len)?0:-1;
}
static int copy_new(const char *filename,const unsigned char *data,uint32_t bytes) {
  char path[160];
  if(snprintf(path,sizeof(path),"pfs0:/%s",filename)>=(int)sizeof(path))return -1;
  if(bytes==0||bytes>4*1024*1024u)return -1;
  int fd=fileXioOpen(path,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_EXCL,0666);
  if(fd<0)return -1;
  uint32_t pos=0;int result=0;
  while(pos<bytes) {
    int n=bytes-pos>4096?4096:(int)(bytes-pos);
    int w=fileXioWrite(fd,data+pos,n);
    if(w!=n){result=-1;break;}
    pos+=(uint32_t)n;
  }
  if(fileXioClose(fd)<0)result=-1;
  if(result||fileXioSync("pfs0:",FXIO_WAIT)<0)return -1;
  return match_file(filename,data,bytes);
}
static int mount_source(void) {
  if(src_mounted)return -1;
  int fd=fileXioDopen("pfs1:/");
  if(fd>=0){fileXioDclose(fd);return -1;} /* never hijack a mount */
  int rc=fileXioMount("pfs1:",SG_SOURCE_PART,FIO_MT_RDONLY);
  if(rc<0)return -1;
  src_mounted=1;
  return 0;
}
static void unmount_source(void) {
  if(src_mounted){fileXioUmount("pfs1:");src_mounted=0;}
}
static int mount_target(const char *part,int rw) {
  char device[56];
  if(dst_mounted||!part||strncmp(part,"PP.",3))return -1;
  int fd=fileXioDopen("pfs0:/");
  if(fd>=0){fileXioDclose(fd);return -1;}
  if(snprintf(device,sizeof(device),"hdd0:%s",part)>=(int)sizeof(device))return -1;
  int rc=fileXioMount("pfs0:",device,rw?FIO_MT_RDWR:FIO_MT_RDONLY);
  if(rc<0)return -1;
  dst_mounted=1;
  return 0;
}
static void unmount_target(void) {
  if(dst_mounted) {
    fileXioSync("pfs0:",FXIO_WAIT);
    fileXioUmount("pfs0:");
    dst_mounted=0;
  }
}
int sg_probe(const sg_game *g,uint32_t *start,uint32_t *sectors) {
  if(!g||!g->partition[0]||!strcmp(g->partition,"PP.DVRISO"))return -1;
  iox_dirent_t item __attribute__((aligned(64)));
  int dd=fileXioDopen("hdd0:");
  if(dd<0)return -1;
  int found=0,got;
  while((got=fileXioDread(dd,&item))>0) {
    item.name[sizeof(item.name)-1]=0;
    if(!strcmp(item.name,g->partition)) {
      found++;
      if(start)*start=(uint32_t)item.stat.private_5;
      if(sectors)*sectors=(uint32_t)item.stat.size;
    }
  }
  int close=fileXioDclose(dd);
  if(got<0||close<0||found>1)return -1;
  return found;
}
static int compare_name(const void *pa,const void *pb) {
  const sg_game *a=pa,*b=pb;
  const unsigned char *s=(const unsigned char*)a->filename,
                      *t=(const unsigned char*)b->filename;
  while(*s&&*t){int d=toupper(*s)-toupper(*t);if(d)return d;s++;t++;}
  return (int)*s-(int)*t;
}
static void make_partition(sg_game *g) {
  if(!g || !sg_target_partition(g->filename,g->game_id,
                                g->partition,sizeof(g->partition)))
    if(g)g->partition[0]=0;
}
static int is_png(const char *id) {
  char p[96];
  if(!desr_valid_game_id(id))return 0;
  snprintf(p,sizeof(p),"pfs1:/ART/%s_COV.png",id);
  int fd=fileXioOpen(p,FIO_O_RDONLY,0);
  if(fd<0)return 0;
  unsigned char sig[8];
  int n=fileXioRead(fd,sig,8);
  fileXioClose(fd);
  static const unsigned char magic[8]={137,80,78,71,13,10,26,10};
  return n==8 && !memcmp(sig,magic,8);
}
/* Only trust exact ISO-filename matches; never infer IDs from names.
 * Same PSXCOV1 and PSXCOVSCAN1 formats used by the hardware-tested PDIM.
 * A cover may exist even if the last scan report said NO_ART. */
static int read_metadata_file(const char *path, char *out,size_t cap) {
  int fd=fileXioOpen(path,FIO_O_RDONLY,0);
  if(fd<0)return -1;
  size_t used=0;
  while(used<cap-1) {
    int n=fileXioRead(fd,out+used,(int)(cap-1-used));
    if(n<0){fileXioClose(fd);return -1;}
    if(n==0)break;
    used+=(size_t)n;
  }
  unsigned char extra;
  int overflow=(used==cap-1)?fileXioRead(fd,&extra,1):0;
  int closed=fileXioClose(fd);
  if(closed<0 || overflow!=0 || used>=cap-1)return -1;
  out[used]=0;
  return (int)used;
}
static void set_game_id(const char *filename,const char *id,int source) {
  if(!desr_valid_game_id(id))return;
  for(int i=0;i<sg_count;i++) {
    sg_game *g=&sg_games[i];
    if(g->game_id[0]||strcmp(g->filename,filename))continue;
    memcpy(g->game_id,id,SG_ID);
    g->id_source=source;
    g->cover_available=is_png(id);
    if(g->cover_available)make_partition(g);
    break;
  }
}
static void parse_cover_index(const char *name,int source) {
  int size=read_metadata_file(name,idx,sizeof(idx));
  if(size<8 || memcmp(idx,"PSXCOV1\n",8))return;
  char *p=idx+8,*end=idx+size;
  while(p<end) {
    char *nl=memchr(p,'\n',(size_t)(end-p));
    if(!nl)break;
    *nl=0;
    char filename[DESR_FILENAME_CAP];
    desr_cover_entry entry;
    if(desr_cover_parse(p,filename,&entry) && !strcmp(entry.ext,"png"))
      set_game_id(filename,entry.game_id,source);
    p=nl+1;
  }
}
static void parse_scan_report(void) {
  int size=read_metadata_file("pfs1:/COVER_SCAN.LOG",idx,sizeof(idx));
  static const char header[]="PSXCOVSCAN1\n";
  if(size<(int)sizeof(header)-1 ||
     memcmp(idx,header,sizeof(header)-1))return;
  char *p=idx+sizeof(header)-1,*end=idx+size;
  while(p<end) {
    char *nl=memchr(p,'\n',(size_t)(end-p));
    if(!nl)break;
    *nl=0;
    char *status=strchr(p,'\t');
    if(!status){p=nl+1;continue;}
    *status++=0;
    char *id=strchr(status,'\t');
    if(!id){p=nl+1;continue;}
    *id++=0;
    if(!strcmp(status,"NO_ART") || !strcmp(status,"NO_ART_EXE") ||
       !strcmp(status,"FOUND_PNG") || !strcmp(status,"FOUND_PNG_EXE") ||
       !strcmp(status,"FOUND_JPG") || !strcmp(status,"FOUND_JPG_EXE"))
      set_game_id(p,id,SG_ID_SOURCE_SCAN_LOG);
    p=nl+1;
  }
}
int sg_resolve_selected(sg_game *g) {
  if(!g)return -30;
  if(g->game_id[0])return 0;
  char id[SG_ID]={0};
  int rc=sg_probe_iso_id(g->filename,id);
  if(rc<0)return -62;
  if(rc==2)return -66;
  if(rc!=1||!desr_valid_game_id(id))return -67;
  if(mount_source()<0)return -11;
  int cover=is_png(id);
  unmount_source();
  if(!cover)return -30; /* ART may be fetched using PDIM HTTPS first. */
  memcpy(g->game_id,id,SG_ID);
  g->id_source=SG_ID_SOURCE_ISO;
  g->cover_available=1;
  make_partition(g);
  if(!g->partition[0])return -65;
  for(int i=0;i<sg_count;i++) {
    if(&sg_games[i]!=g && sg_games[i].partition[0] &&
       !strcmp(g->partition,sg_games[i].partition)) {
      g->partition[0]=0;
      return -65;
    }
  }
  return 0;
}
int sg_scan(void) {
  iox_dirent_t ent __attribute__((aligned(64)));
  int dd=fileXioDopen("dvr_pfs0:/ISO/");
  if(dd<0)return -10;
  memset(sg_games,0,sizeof(sg_games));sg_count=0;
  int n;
  while((n=fileXioDread(dd,&ent))>0) {
    ent.name[sizeof(ent.name)-1]=0;
    if(!desr_filename_is_safe(ent.name)||sg_count>=SG_COUNT)continue;
    strncpy(sg_games[sg_count].filename,ent.name,SG_NAME-1);
    sg_count++;
  }
  int closed=fileXioDclose(dd);
  if(n<0||closed<0)return -10;
  qsort(sg_games,sg_count,sizeof(sg_games[0]),compare_name);
  if(mount_source()<0)return -11;
  parse_cover_index("pfs1:/COVERS.IDX",SG_ID_SOURCE_INDEX);
  parse_cover_index("pfs1:/COVERS.BAK",SG_ID_SOURCE_BACKUP);
  parse_scan_report();
  unmount_source();
  /* The normalized name uses only 10 title letters and a Game ID.
   * Two ISO filenames can still collide. Reject ALL involved entries:
   * no ambiguous game may be installed to a shared partition name. */
  unsigned char conflict[SG_COUNT]={0};
  for(int i=0;i<sg_count;i++)if(sg_games[i].partition[0]) {
    for(int j=i+1;j<sg_count;j++)if(sg_games[j].partition[0] &&
         !strcmp(sg_games[i].partition,sg_games[j].partition))
      conflict[i]=conflict[j]=1;
  }
  for(int i=0;i<sg_count;i++)if(conflict[i])sg_games[i].partition[0]=0;
  return sg_count;
}
static int match_embedded(const char *name,const unsigned char *buf,uint32_t sz) {
  return match_file(name,buf,sz);
}
void sg_build_boot(const sg_game *g,int lang,char *out,size_t cap) {
  snprintf(out,cap,"path=hdd0:PP.DVRISO:pfs:/%s\narg=-bsd=dvr\narg=-dvd=dvr_pfs0:/ISO/%s\n%sarg=-dev9=NICHDD\n#PAD\n",
     lang?"neutrino_hdd_lang.elf":"neutrino_hdd.elf",
     g->filename,lang?"arg=-cfg=osdlang\narg=-osdlang=en\n":"");
}
void sg_build_execute(const sg_game *g,char *out,size_t cap) {
  snprintf(out,cap,"path=hdd0:%s:pfs:/BOOT.ELF\narg=hdd0:%s/BOOT.CNF\narg=-dev9=NICHDD\n#PAD\n",g->partition,g->partition);
}
void sg_build_info(const sg_game *g,char *out,size_t cap) {
  char title[112]={0};strncpy(title,g->filename,sizeof(title)-1);
  size_t l=strlen(title);
  if(l>=4 && !strcasecmp(title+l-4,".iso"))title[l-4]=0;
  snprintf(out,cap,"title = %s\ntitle_id = %s\ntitle_sub_id = 0\nrelease_date = 0\n"
    "developer_id = Unknown\npublisher_id = Unknown\nnote = DVR ISO game shortcut\n"
    "image_topviewflag = 0\nimage_type = 0\nimage_count = 1\n"
    "image_viewsec = 600\ncopyright_viewflag = 0\ncopyright_imgcount = 0\n"
    "genre = Unknown\nparental_lock = 1\neffective_date = 0\nexpire_date = 0\n"
    "area = 0\nviolence_flag = 0\ncontent_type = 255\ncontent_subtype = 0\n",title,g->game_id);
}
void sg_build_sys(char *out,size_t cap) {
  snprintf(out,cap,"BOOT2 = pfs:/EXECUTE.ELF\nVER = 1.00\nVMODE = NTSC\nHDDUNITPOWER = NICHDD\n");
}
void sg_build_icon(const sg_game *g,char *out,size_t cap) {
  char title[12]={0};
  int n=0;for(const unsigned char *p=(const unsigned char*)g->filename;*p && n<11;p++){
    if(*p=='.')break;
    if(*p=='\n'||*p=='\r'||*p=='=')break;
    title[n++]=(char)*p;
  }
  snprintf(out,cap,"PS2X\ntitle0=%s\ntitle1=PS2\nbgcola=0\nbgcol0=0,0,0\n"
    "bgcol1=0,0,0\nbgcol2=0,0,0\nbgcol3=0,0,0\nlightdir0=1.0,-1.0,1.0\n"
    "lightdir1=-1.0,1.0,-1.0\nlightdir2=0.0,0.0,0.0\n"
    "lightcolamb=64,64,64\nlightcol0=64,64,64\nlightcol1=16,16,16\n"
    "lightcol2=0,0,0\nuninstallmes0=Delete shortcut?\n"
    "uninstallmes1=ISO remains on DVR.\nuninstallmes2=\n",title);
}
static int file_matches(const char *name,const char *text) {
  return match_file(name,text,strlen(text));
}
int sg_verify_mounted(const sg_game *g,int lang) {
  char boot[512],exe[256],info[1024],mark[512],parsed[512];
  sg_build_boot(g,lang,boot,sizeof(boot));
  sg_build_execute(g,exe,sizeof(exe));
  sg_build_info(g,info,sizeof(info));
  if(match_embedded("BOOT.ELF",osd_launcher,size_osd_launcher)<0 ||
     match_embedded("EXECUTE.ELF",execute_kelf,size_execute_kelf)<0 ||
     file_matches("BOOT.CNF",boot)<0 ||
     file_matches("EXECUTE.CNF",exe)<0 ||
     file_matches("res/info.sys",info)<0) return -61;
  int fd=fileXioOpen("pfs0:/SHORTCUT.ID",FIO_O_RDONLY,0);
  if(fd<0)return -61;
  int n=fileXioRead(fd,mark,sizeof(mark)-1),close=fileXioClose(fd);
  if(n<=0||n>=(int)sizeof(mark)-1||close<0)return -61;
  mark[n]=0;
  char part[SG_PART],iso[SG_NAME],id[SG_ID];
  unsigned lang_found=3,crc1=0,crc2=0,bytes1=0,bytes2=0;
  if(sscanf(mark,"PDIMXMB1\nPART=%31[^\n]\nISO=%127[^\n]\nID=%11[^\n]\nLANG=%u\nPNG1=%x,%u\nPNG2=%x,%u\nEND\n",
     part,iso,id,&lang_found,&crc1,&bytes1,&crc2,&bytes2)!=8)return -61;
  if(strcmp(part,g->partition)||strcmp(iso,g->filename)||strcmp(id,g->game_id)||lang_found!=(unsigned)lang)return -61;
  snprintf(parsed,sizeof(parsed),"PDIMXMB1\nPART=%s\nISO=%s\nID=%s\nLANG=%u\nPNG1=%08X,%u\nPNG2=%08X,%u\nEND\n",
       part,iso,id,lang_found,crc1,bytes1,crc2,bytes2);
  if(strcmp(mark,parsed))return -61;
  uint32_t bytes=0,crc=0;
  if(sg_crc_file("pfs0:/res/jkt_001.png",&bytes,&crc)<0||bytes!=bytes1||crc!=crc1)return -61;
  if(sg_crc_file("pfs0:/res/jkt_002.png",&bytes,&crc)<0||bytes!=bytes2||crc!=crc2)return -61;
  return 0;
}
int sg_validate_payload(const sg_game *g,int lang,uint32_t *start,uint32_t *sectors) {
  int exist=sg_probe(g,start,sectors);
  if(exist!=1)return exist==0?-51:-50;
  if(!*start||*sectors!=SG_SECTORS||*start>0x0FFFFFFF-11u)return -50;
  if(mount_target(g->partition,0)<0)return -52;
  int rc=sg_verify_mounted(g,lang);
  unmount_target();
  return rc;
}
int sg_preflight(const sg_game *g,int lang) {
  if(!g||!g->partition[0]||!g->cover_available||!g->game_id[0] ||
     !sg_iso_filename_allowed(g->filename))return -30;
  /* COVERS.IDX may be stale after someone replaces an ISO with a file
   * of the same name. Compare the LIVE ISO boot ID read-only. */
  int iso=sg_check_iso_id(g);
  if(iso<0)return iso==SG_ISO_ID_MISMATCH?-63:-62;
  if(iso!=SG_ISO_ID_MATCH && g->id_source==SG_ID_SOURCE_ISO)
    return -62; /* a newly discovered ID requires a repeatable ISO probe */
  int rc=sg_probe(g,NULL,NULL);
  if(rc<0)return -50;
  if(rc==0) {
    /* Preflight must reject a damaged/unexpected source cover BEFORE APA
     * creation. The normalized PDIM HTTPS output is exactly 140x200. */
    if(mount_source()<0)return -11;
    char art[96];
    snprintf(art,sizeof(art),"pfs1:/ART/%s_COV.png",g->game_id);
    int png=sg_png_validate_source(art);
    unmount_source();
    return png<0?-64:(iso==SG_ISO_ID_MATCH?0:
                        iso==SG_ISO_ID_INDEX_READER?5:3);
  }
  uint32_t start=0,sectors=0;
  rc=sg_validate_payload(g,lang,&start,&sectors);
  if(rc<0)return rc;
  return iso==SG_ISO_ID_MATCH?1:
         iso==SG_ISO_ID_INDEX_READER?4:2;
}
int sg_png_make_pair(const char *src,uint32_t *crc1,uint32_t *len1,uint32_t *crc2,uint32_t *len2);
int sg_install_core(const sg_game *g,int lang) {
  if(!g||!g->cover_available||!desr_valid_game_id(g->game_id)||!g->partition[0] ||
     !sg_iso_filename_allowed(g->filename))return -30;
  /* Both checks happen before the first APA write. */
  int iso=sg_check_iso_id(g);
  if(iso<0)return iso==SG_ISO_ID_MISMATCH?-63:-62;
  if(iso!=SG_ISO_ID_MATCH && g->id_source==SG_ID_SOURCE_ISO)
    return -62;
  if(sg_probe(g,NULL,NULL)!=0)return -50; /* existing partition never modified by CORE */
  /* Require an intact source ART PNG and Neutrino before any APA change. */
  if(mount_source()<0)return -11;
  char src[100];snprintf(src,sizeof(src),"pfs1:/ART/%s_COV.png",g->game_id);
  uint32_t source_size=0,source_crc=0;
  if(sg_crc_file(src,&source_size,&source_crc)<0||source_size<64||source_size>2000000u) {
    unmount_source();return -30;
  }
  /* Only create the new APA partition if the image fully decodes.
   * Otherwise damaged artwork would leave an unnecessary partial PFS. */
  if(sg_png_validate_source(src)<0) {
    unmount_source();return -64;
  }
  int fd=fileXioOpen("pfs1:/neutrino_hdd.elf",FIO_O_RDONLY,0);
  if(fd<0){unmount_source();return -30;}fileXioClose(fd);
  fd=fileXioOpen("pfs1:/neutrino_hdd_lang.elf",FIO_O_RDONLY,0);
  if(fd<0){unmount_source();return -30;}fileXioClose(fd);
  char dev[80],spec[100];
  snprintf(dev,sizeof(dev),"hdd0:%s",g->partition);
  snprintf(spec,sizeof(spec),"hdd0:%s,,,128M,PFS",g->partition);
  /* Conservative 128 GiB capacity check (does not guarantee free space). */
  int dd=fileXioDopen("hdd0:");
  if(dd<0){unmount_source();return -50;}
  iox_dirent_t item __attribute__((aligned(64)));
  uint64_t max_end=0;int n;
  while((n=fileXioDread(dd,&item))>0) {
    uint64_t end=(uint64_t)(uint32_t)item.stat.private_5+(uint32_t)item.stat.size;
    if(end>max_end)max_end=end;
  }
  int close=fileXioDclose(dd);
  if(n<0||close<0||!max_end||max_end+SG_SECTORS>SG_LBA28_LIMIT) {
    unmount_source();return -50;
  }
  /* This is the FIRST write. From here a failure leaves a partial target. */
  fd=fileXioOpen(spec,FIO_O_RDWR|FIO_O_CREAT,0);
  if(fd<0){unmount_source();return -53;}
  if(fileXioClose(fd)<0){unmount_source();return -53;}
  uint32_t start=0,sectors=0;
  if(sg_probe(g,&start,&sectors)!=1||!start||sectors!=SG_SECTORS||start>0x0FFFFFFF-11u){
    unmount_source();return -50;
  }
  int zone=8192;
  if(fileXioFormat("pfs:",dev,(const char*)&zone,sizeof(zone))<0){unmount_source();return -54;}
  if(mount_target(g->partition,1)<0){unmount_source();return -52;}
  char boot[512],exe[256],info[1024];
  sg_build_boot(g,lang,boot,sizeof(boot));
  sg_build_execute(g,exe,sizeof(exe));
  sg_build_info(g,info,sizeof(info));
  int rc=0;
  if(copy_new("EXECUTE.ELF",execute_kelf,size_execute_kelf)<0||
     copy_new("BOOT.ELF",osd_launcher,size_osd_launcher)<0||
     copy_new("EXECUTE.CNF",(const unsigned char*)exe,strlen(exe))<0||
     copy_new("BOOT.CNF",(const unsigned char*)boot,strlen(boot))<0)rc=-55;
  if(!rc&&fileXioMkdir("pfs0:/res",0777)<0)rc=-55;
  if(!rc&&copy_new("res/info.sys",(const unsigned char*)info,strlen(info))<0)rc=-55;
  uint32_t a=0,b=0,c=0,d=0;
  if(!rc&&sg_png_make_pair(src,&a,&b,&c,&d)<0)rc=-56;
  if(!rc) {
    char marker[512];
    snprintf(marker,sizeof(marker),"PDIMXMB1\nPART=%s\nISO=%s\nID=%s\nLANG=%d\nPNG1=%08X,%u\nPNG2=%08X,%u\nEND\n",
         g->partition,g->filename,g->game_id,lang,(unsigned)a,(unsigned)b,(unsigned)c,(unsigned)d);
    if(copy_new("SHORTCUT.ID",(const unsigned char*)marker,strlen(marker))<0)rc=-55;
  }
  if(!rc)rc=sg_verify_mounted(g,lang);
  unmount_target();
  unmount_source();
  return rc;
}
const char *sg_error(int rc) {
  switch(rc){
    case 0:return "OK: new safe target or installed and verified";
    case 1:return "EXISTING SHORTCUT FILES VERIFIED";
    case -10:return "DVR ISO DIRECTORY CANNOT BE READ";
    case -11:return "PP.DVRISO READ-ONLY MOUNT ERROR";
    case -30:return "MISSING GAME ID OR PNG; DOWNLOAD IN PDIM HTTPS";
    case -50:return "TARGET PARTITION EXISTS/UNSAFE - NO OVERWRITE";
    case -51:return "TARGET PARTITION DOES NOT EXIST";
    case -52:return "PFS MOUNT ERROR";
    case -53:return "CREATE PARTITION FAILED - CHECK APA MANUALLY";
    case -54:return "NEW PFS FORMAT FAILED - DO NOT DELETE BLINDLY";
    case -55:return "COPY/VERIFY FAILED - LEAVE PARTITION FOR INSPECTION";
    case -56:return "PNG8 CONVERSION FAILED - PARTIAL PARTITION";
    case -61:return "SHORTCUT.ID OR PAYLOAD CRC MISMATCH";
    case -62:return "ISO READ ERROR - CANNOT VERIFY LIVE GAME";
    case -63:return "ISO ID DIFFERS FROM COVERS.IDX - ABORT";
    case -64:return "ART PNG INVALID - NO PARTITION CREATED";
    case -65:return "DUPLICATE OR UNSAFE XMB PARTITION NAME";
    case -66:return "ISO ID SCAN UNSUPPORTED; NEED COVERS.IDX";
    case -67:return "ISO GAME ID NOT FOUND; NEED PDIM ART INDEX";
    case -70:return "XMB HEADER IS FOREIGN / INVALID";
    case -71:return "XMB RAW SECTOR WRITE/VERIFY ERROR";
    case -72:return "XMB TARGET IDENTITY CHANGED";
    default:return "UNKNOWN ERROR; NEVER FORMAT SONY SYSTEM HDD";
  }
}

/* PDIM HDD12 TEST installer. Never formats an existing APA partition.
 * Installs files copied from the official HDD 1.0 package, but the Manager
 * and both FIX95 Neutrino binaries are REBUILT for PP.DVRISO12. The source
 * package is immutable; destination files are staged and verified.
 */
#include "pdim12_installer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <unistd.h>

#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define LINE_CAP 192
#define MANIFEST_CAP 30000
#define IO_CHUNK 4096
#define PFS_FORMAT_ZONE 8192

typedef struct {
    char relative[PDIM12_REL_LIMIT];
    uint32_t size;
    uint32_t crc;
} manifest_entry;
static manifest_entry manifest[PDIM12_MAX_FILES];
static int manifest_count=0;
static char raw_manifest[MANIFEST_CAP+1];
static unsigned char transfer[IO_CHUNK] __attribute__((aligned(64)));
static int mount_owned=0;
static const char *format_str="hdd0:PP.DVRISO12,,,256M,PFS";
static const char *source_prefix=PDIM12_APP_SOURCE;

const char *pdim12_error(int rc) {
    switch(rc) {
    case -10:return "HDD PARTITION LIST FAILED";
    case -11:return "PARTITION INVALID / UNSAFE SIZE OR LBA";
    case -12:return "PFS MOUNT FAILED - EXISTING PARTITION NOT FORMATTED";
    case -13:return "PARTITION CREATE FAILED";
    case -14:return "NEW PARTITION FORMAT FAILED - DO NOT AUTO DELETE";
    case -15:return "NEW PFS VERIFY FAILED";
    case -20:return "MANIFEST MISSING OR INVALID";
    case -21:return "UNSAFE FILE OR INVALID PACKAGE";
    case -22:return "DESTINATION FILE DIFFERS - PRESERVED";
    case -23:return "NEW FILE WRITE FAILED - .NEW PRESERVED";
    case -24:return "FILE READBACK/CRC FAILED - .NEW PRESERVED";
    case -25:return "DIRECTORY CREATE FAILED";
    case -26:return "FILE RENAME/SYNC FAILED - CHECK .NEW";
    case -30:return "REQUIRED XMB FILES NOT VERIFIED";
    case -31:return "XMB HEADER NOT EMPTY / FOREIGN";
    case -32:return "XMB HEADER WRITE FAILED";
    case -33:return "XMB HEADER VERIFY FAILED";
    default:return "UNEXPECTED ERROR; NO AUTOMATIC ROLLBACK";
    }
}
static int scan_target(pdim12_state *state) {
    iox_dirent_t item __attribute__((aligned(64)));
    int dd=fileXioDopen("hdd0:");
    if(dd<0)return -10;
    int n,found=0;
    uint32_t max_end=0;
    while((n=fileXioDread(dd,&item))>0) {
        if (item.stat.mode==0) continue;
        uint64_t end=(uint64_t)item.stat.private_5 + (uint64_t)item.stat.size;
        if (end>max_end && end<=0xffffffffULL)max_end=(uint32_t)end;
        if (!strcmp(item.name,PDIM12_PARTITION)) {
            if(found++) {fileXioDclose(dd);return -11;}
            state->exists=1;
            state->start=(uint32_t)item.stat.private_5;
            state->sectors=(uint32_t)item.stat.size;
        }
    }
    int closed=fileXioDclose(dd);
    if(n<0 || closed<0)return -10;
    state->last_end=max_end;
    if(state->exists && !pdim12_address_safe(state->start,state->sectors))
        return -11;
    return 0;
}
int pdim12_probe(pdim12_state *state) {
    int rc;
    memset(state,0,sizeof(*state));
    rc=scan_target(state);
    if(rc<0)return rc;
    if (state->exists) {
        rc=pdim12_mount(1);
        if(rc<0)return rc;
        state->pfs_ok=1;
        pdim12_unmount();
    }
    return 0;
}
int pdim12_mount(int readonly) {
    if (mount_owned)return -12;
    int probe=fileXioDopen(PDIM12_MOUNT "/");
    if(probe>=0) {fileXioDclose(probe);return -12;}
    int r=fileXioMount(PDIM12_MOUNT,PDIM12_DEVICE,readonly?FIO_MT_RDONLY:FIO_MT_RDWR);
    if(r<0)return -12;
    mount_owned=1;
    probe=fileXioDopen(PDIM12_MOUNT "/");
    if(probe<0) {pdim12_unmount();return -12;}
    fileXioDclose(probe);
    return 0;
}
void pdim12_unmount(void) {
    if (mount_owned) {
        fileXioSync(PDIM12_MOUNT,FXIO_WAIT);
        fileXioUmount(PDIM12_MOUNT);
        mount_owned=0;
    }
}
int pdim12_prepare_partition(pdim12_state *state) {
    if (!state || state->exists)return -11;
    /* Refuse if the existing APA data already reaches/passes the 128GiB
     * boundary; this is a conservative heuristic, not a placement promise.
     * Recheck the new target's actual start BEFORE formatting it.
     */
    if (!state->last_end || (uint64_t)state->last_end + PDIM12_SECTORS > PDIM12_LBA28_LIMIT)
        return -11;
    int fd=fileXioOpen(format_str,FIO_O_RDWR|FIO_O_CREAT,0);
    if(fd<0)return -13;
    if(fileXioClose(fd)<0)return -13;
    pdim12_state check={0};
    if(scan_target(&check)<0 || !check.exists ||
       !pdim12_address_safe(check.start,check.sectors))return -11;
    /* Only format the exact partition JUST CREATED by this wizard.
     * On any failure, leave it for deliberate manual inspection.
     */
    int zone=PFS_FORMAT_ZONE;
    int rc=fileXioFormat("pfs:",PDIM12_DEVICE,(const char *)&zone,sizeof(zone));
    if(rc<0)return -14;
    rc=pdim12_mount(1);
    if(rc<0)return -15;
    pdim12_unmount();
    *state=check;
    state->exists=1;state->pfs_ok=1;state->created_this_run=1;
    return 0;
}
static int parse_manifest(void) {
    int fd=fileXioOpen(PDIM12_MANIFEST,FIO_O_RDONLY,0);
    if(fd<0)return -20;
    int n=fileXioRead(fd,raw_manifest,MANIFEST_CAP);
    unsigned char extra;
    int over=fileXioRead(fd,&extra,1);
    int closed=fileXioClose(fd);
    if(n<=0 || n>=MANIFEST_CAP || over!=0 || closed<0)return -20;
    raw_manifest[n]=0;
    char *line=raw_manifest;
    int count=0;
    while(line && *line) {
        char *next=strchr(line,'\n');
        if(next)*next++='\0';
        size_t length=strlen(line);
        if(length && line[length-1]=='\r')line[--length]='\0';
        if(!length) {line=next;continue;}
        char *first=strchr(line,'|');
        if(!first)return -20;
        *first++='\0';
        char *second=strchr(first,'|');
        if(!second)return -20;
        *second++='\0';
        if(strchr(second,'|') || count>=PDIM12_MAX_FILES ||
           !pdim12_path_allowed(line))return -21;
        char *end_size,*end_crc;
        unsigned long size=strtoul(first,&end_size,10);
        unsigned long crc=strtoul(second,&end_crc,16);
        if(end_size==first || *end_size || end_crc==second || *end_crc ||
           strlen(second)!=8 || size==0 || size>PDIM12_MAX_FILE)
            return -21;
        for (int i=0;i<count;i++) {
            if (!strcmp(line,manifest[i].relative))return -21;
        }
        strcpy(manifest[count].relative,line);
        manifest[count].size=(uint32_t)size;
        manifest[count].crc=(uint32_t)crc;
        count++;
        line=next;
    }
    if(count<8)return -21;
    manifest_count=count;
    return 0;
}
static int file_crc(const char *path,uint32_t *size,uint32_t *hash) {
    int fd=fileXioOpen(path,FIO_O_RDONLY,0);
    if(fd<0)return -1;
    uint32_t crc=0xffffffffu,received=0;
    int n;
    while((n=fileXioRead(fd,transfer,sizeof(transfer)))>0){
        received+=(uint32_t)n;
        if(received>PDIM12_MAX_FILE) {fileXioClose(fd);return -1;}
        crc=pdim12_crc32_update(crc,transfer,(size_t)n);
    }
    int close=fileXioClose(fd);
    if(n<0||close<0)return -1;
    *size=received;*hash=pdim12_crc32_end(crc);
    return 0;
}
static int path_for(char *out,size_t cap,const char *prefix,const char *relative,const char *suffix) {
    if(!pdim12_path_allowed(relative))return -21;
    int n=snprintf(out,cap,"%s%s%s",prefix,relative,suffix?suffix:"");
    return n>0 && (size_t)n<cap?0:-21;
}
static int make_parents(const char *relative) {
    char path[256];
    int n=snprintf(path,sizeof(path),"%s",PDIM12_MOUNT "/");
    if(n<0 || (size_t)n>=sizeof(path))return -25;
    size_t prefix=(size_t)n;
    size_t len=strlen(relative);
    for(size_t i=0;i<len;i++) {
        if(relative[i]=='/') {
            path[prefix+i]='\0';
            int rc=fileXioMkdir(path,0777);
            if(rc<0) {
                int dd=fileXioDopen(path);
                if(dd<0)return -25;
                fileXioDclose(dd);
            }
            path[prefix+i]='/';
        } else path[prefix+i]=relative[i];
    }
    return 0;
}
static int verify_one(const manifest_entry *m) {
    char path[256];uint32_t got,crc;
    if(path_for(path,sizeof(path),PDIM12_MOUNT "/",m->relative,NULL)<0)return -21;
    if(file_crc(path,&got,&crc)<0 || got!=m->size || crc!=m->crc)return -24;
    return 0;
}
static int copy_one(const manifest_entry *m,int *already) {
    char src[256],dst[256],tmp[256];
    uint32_t size,crc;
    if(path_for(src,sizeof(src),source_prefix,m->relative,NULL)<0||
       path_for(dst,sizeof(dst),PDIM12_MOUNT "/",m->relative,NULL)<0||
       path_for(tmp,sizeof(tmp),PDIM12_MOUNT "/",m->relative,".PDIMNEW")<0)
        return -21;
    if(file_crc(src,&size,&crc)<0 || size!=m->size || crc!=m->crc)return -21;
    if(file_crc(dst,&size,&crc)==0) {
        if(size!=m->size || crc!=m->crc)return -22;
        *already=1;return 0;
    }
    /* An orphan temporary file is a failed prior install: never overwrite. */
    int old=fileXioOpen(tmp,FIO_O_RDONLY,0);
    if(old>=0) {fileXioClose(old);return -23;}
    if(make_parents(m->relative)<0)return -25;
    int in=fileXioOpen(src,FIO_O_RDONLY,0);
    if(in<0)return -21;
    int out=fileXioOpen(tmp,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_EXCL,0666);
    if(out<0){fileXioClose(in);return -23;}
    uint32_t copied=0,hash=0xffffffffu;
    int rc=0;
    while(copied<m->size) {
        int want=(m->size-copied)>IO_CHUNK?IO_CHUNK:(int)(m->size-copied);
        int n=fileXioRead(in,transfer,want);
        if(n<=0||n>want){rc=-23;break;}
        hash=pdim12_crc32_update(hash,transfer,(size_t)n);
        int written=0;
        while(written<n){
            int w=fileXioWrite(out,transfer+written,n-written);
            if(w<=0 || w>n-written){rc=-23;break;}
            written+=w;
        }
        if(rc)break;
        copied+=(uint32_t)n;
    }
    int out_closed=fileXioClose(out);
    int in_closed=fileXioClose(in);
    if(out_closed<0 || in_closed<0)rc=-23;
    if(rc)return rc;
    if(copied!=m->size || pdim12_crc32_end(hash)!=m->crc)return -23;
    if(fileXioSync(PDIM12_MOUNT,FXIO_WAIT)<0)return -26;
    if(file_crc(tmp,&size,&crc)<0 || size!=m->size || crc!=m->crc)return -24;
    if(fileXioRename(tmp,dst)<0)return -26;
    if(fileXioSync(PDIM12_MOUNT,FXIO_WAIT)<0)return -26;
    return verify_one(m);
}
int pdim12_verify_files(pdim12_state *state) {
    int rc=parse_manifest();
    if(rc<0)return rc;
    rc=pdim12_mount(1);
    if(rc<0)return rc;
    int count=0;
    for(int i=0;i<manifest_count;i++) {
        rc=verify_one(&manifest[i]);
        if(rc<0){
            scr_printf("MISMATCH: %s  error %d\n",manifest[i].relative,rc);
            break;
        }
        count++;
    }
    pdim12_unmount();
    if(state) {state->files_total=manifest_count;state->files_verified=count;}
    return rc;
}
int pdim12_install_files(pdim12_state *state) {
    if(!state || !state->exists || !state->pfs_ok)return -12;
    int rc=parse_manifest();
    if(rc<0)return rc;
    rc=pdim12_mount(0);
    if(rc<0)return rc;
    state->files_total=manifest_count;
    state->files_copied=state->files_existing=state->files_verified=0;
    for(int i=0;i<manifest_count;i++){
        int existed=0;
        scr_printf("[%d/%d] %s\n",i+1,manifest_count,manifest[i].relative);
        rc=copy_one(&manifest[i],&existed);
        if(rc<0)break;
        if(existed)state->files_existing++;
        else state->files_copied++;
        state->files_verified++;
    }
    pdim12_unmount();
    return rc;
}

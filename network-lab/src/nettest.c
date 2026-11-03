/* Sony PSX DESR LAN A/B/C test suite.
 * Separate, reproducible ELF variants; no game launching, no writes to DVR.
 * C and CLEAN exclusively mount hdd0:PP.NETTEST. A and B are RAM-only.
 * All three use the same HTTP GET /<GameID>_COV.png and strict PNG CRC.
 */
#include "settings.h"
#include "probe_png.h"
#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <libcdvd.h>
#include <libpad.h>
#include <loadfile.h>
#include <netman.h>
#include <ps2ip.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>

#define IRX(name) \
 extern unsigned char name##_irx[] __attribute__((aligned(16))); \
 extern uint32_t size_##name##_irx
IRX(iomanX); IRX(fileXio);
IRX(sio2man); IRX(mcman); IRX(mcserv); IRX(freepad);
IRX(netman); IRX(smap);
#if defined(LAB_MODE_A) || defined(LAB_MODE_B)
/* Match the proven USB 0.8 module stack in both A and B;
   the only change between them is dev9_ns versus ps2dev9. */
IRX(bdm); IRX(bdmfs_fatfs);
IRX(usbd_mini); IRX(usbmass_bd_mini);
#endif
#ifdef LAB_MODE_A
IRX(dev9_ns);
#else
IRX(ps2dev9);
#endif
#if defined(LAB_MODE_C) || defined(LAB_MODE_CLEAN)
IRX(ps2atad); IRX(ps2hdd); IRX(ps2fs);
IRX(dvrdrv); IRX(dvrfile);
static const char hdd_args[]="-o\0""4\0""-n\0""20";
static const char pfs_args[]="-m\0""4\0""-o\0""10\0""-n\0""40";
typedef char check_hdd_args[(sizeof(hdd_args)==11)?1:-1];
typedef char check_pfs_args[(sizeof(pfs_args)==17)?1:-1];
static int own_mount;
#endif

#if defined(LAB_MODE_A)
#define MODE_LABEL "A dev9_ns (NO ATA/PFS)"
#elif defined(LAB_MODE_B)
#define MODE_LABEL "B ps2dev9 (NO ATA/PFS)"
#elif defined(LAB_MODE_C)
#define MODE_LABEL "C ps2dev9 + ATA/PFS/DVR"
#else
#define MODE_LABEL "CLEAN PP.NETTEST ART (ONLY)"
#endif

#define BUF_CAP (PROBE_PNG_MAX+PROBE_HTTP_HEADER_MAX)
#define PAGE_SIZE 11
static uint8_t net_response[BUF_CAP+1];
#if defined(LAB_MODE_C)
static uint8_t verify_buffer[PROBE_PNG_MAX];
#endif
static unsigned char padbuf[256] __attribute__((aligned(64)));
static int summary_ok,summary_bad,summary_link,summary_http,summary_miss,summary_pfs;
enum {ST_NONE=0,ST_OK,ST_BAD,ST_LINK,ST_HTTP,ST_MISS,ST_PFS};
typedef struct {int status, err; unsigned long bytes;} result_t;
static result_t results[LAB_GAME_COUNT];

static int load_buf(const char *name, void *bytes, uint32_t size,
                    const char *args, int arg_size) {
  int rv=0,id=SifExecModuleBuffer(bytes,size,arg_size,(void*)args,&rv);
  scr_printf("IRX %-12s id=%d rv=%d\n",name,id,rv);
  if (id<0 || rv<0) return id<0?id:rv;
  return 0;
}
#define LOAD(name) do {int e=load_buf(#name,name##_irx,size_##name##_irx,NULL,0);if(e<0)return e;} while(0)

static int init_drivers(void) {
  int r;
  scr_printf("RESET IOP; LAN TEST %s\n",MODE_LABEL);
  while (!SifIopReset("",0)) { }
  while (!SifIopSync()) { }
  sceSifInitRpc(0);
  sbv_patch_enable_lmb();
  sbv_patch_disable_prefix_check();
  sbv_patch_fileio();
  LOAD(iomanX);
  LOAD(fileXio);
#if defined(LAB_MODE_A) || defined(LAB_MODE_B)
  fileXioInit();
#else
  LOAD(ps2dev9);
  LOAD(ps2atad);
  r=load_buf("ps2hdd",ps2hdd_irx,size_ps2hdd_irx,hdd_args,sizeof(hdd_args));
  if (r<0) return r;
  r=load_buf("ps2fs",ps2fs_irx,size_ps2fs_irx,pfs_args,sizeof(pfs_args));
  if (r<0) return r;
  r=fileXioInit();
  if (r<0) return r;
#endif
  LOAD(sio2man);
  LOAD(mcman);
  LOAD(mcserv);
  LOAD(freepad);
#if defined(LAB_MODE_A) || defined(LAB_MODE_B)
  LOAD(bdm);
  LOAD(bdmfs_fatfs);
  LOAD(usbd_mini);
  LOAD(usbmass_bd_mini);
#if defined(LAB_MODE_A)
  LOAD(dev9_ns);
#else
  LOAD(ps2dev9);
#endif
#endif
#if defined(LAB_MODE_C) || defined(LAB_MODE_CLEAN)
  {
    u32 notice_arg=0;
    r=sceCdInit(SCECdINoD);
    scr_printf("CDVD init=%d\n",r);
    if (r<=0) return -201;
    r=sceCdNoticeGameStart(0,&notice_arg);
    scr_printf("DVR notice=%d\n",r);
    if (r<0) return -202;
    usleep(500000);
  }
  LOAD(dvrdrv);
  LOAD(dvrfile);
#endif
  return 0;
}
#undef LOAD

#if defined(LAB_MODE_C) || defined(LAB_MODE_CLEAN)
static int mount_sandbox(void) {
  int d,r;
  /* Explicit whitelist; this test never constructs a partition name
     dynamically and NEVER opens the user's PP.DVRISO partition. */
  d=fileXioDopen(LAB_MOUNT "/");
  if (d>=0) {fileXioDclose(d);return -310;}
  r=fileXioMount(LAB_MOUNT,LAB_PARTITION,FIO_MT_RDWR);
  if (r<0) return r;
  own_mount=1;
  d=fileXioDopen(LAB_MOUNT "/");
  if (d<0) {
    fileXioUmount(LAB_MOUNT);
    own_mount=0;
    return d;
  }
  fileXioDclose(d);
  return 0;
}
static void unmount_sandbox(void) {
  if(own_mount) {
    fileXioUmount(LAB_MOUNT);
    own_mount=0;
  }
}
static int ensure_art(void) {
  int d=fileXioDopen(LAB_ART);
  if (d>=0) {fileXioDclose(d);return 0;}
  /* Creates only the directory inside the already-mounted PP.NETTEST,
     never creates a partition or formats a filesystem. */
  if (fileXioMkdir(LAB_ART,0777)<0) return -1;
  d=fileXioDopen(LAB_ART);
  if (d<0) return d;
  fileXioDclose(d);
  return 0;
}
#endif

static int poll_press(void) {
  static int previous;
  struct padButtonStatus p;
  int bits=0,edge;
  if (padRead(0,0,&p)>0) bits=0xffff ^ p.btns;
  edge=bits & ~previous;
  previous=bits;
  return edge;
}
static int wait_command(int allow_x,int allow_pages) {
  for (;;) {
    int press=poll_press();
    if (press & PAD_START) return -1;
    if (allow_x && (press & PAD_CROSS)) return 1;
    if (allow_pages && (press & PAD_RIGHT)) return 2;
    if (allow_pages && (press & PAD_LEFT)) return 3;
    usleep(30000);
  }
}
static void show_error(const char *message,int code) {
  scr_clear();
  scr_printf("PSX DESR LAN %s\n",MODE_LABEL);
  scr_printf("FAILED: %s = %d\n",message,code);
  scr_printf("NO GAME/ISO/DVR PARTITIONS WERE MODIFIED\n");
  scr_printf("START: exit\n");
  while(wait_command(0,0)!=-1) {}
}
static const char *status_text(int code) {
  switch(code) {
  case ST_OK: return "OK";
  case ST_BAD:return "BAD_PNG";
  case ST_LINK:return "LINK";
  case ST_HTTP:return "HTTP";
  case ST_MISS:return "MISS404";
  case ST_PFS:return "PFS_WRITE";
  default:return "NOT_RUN";
  }
}
static void show_report(int page) {
  int i,from=page*PAGE_SIZE,to=from+PAGE_SIZE;
  if (to>LAB_GAME_COUNT) to=LAB_GAME_COUNT;
  scr_clear();
  scr_printf("PSX DVR NETTEST %s\n",MODE_LABEL);
  scr_printf("OK=%d BAD=%d LINK=%d HTTP=%d MISS=%d PFS=%d\n",
     summary_ok,summary_bad,summary_link,summary_http,summary_miss,summary_pfs);
  scr_printf("REPORT %d/%d (Game IDs 1..44)\n\n",
     page+1,(LAB_GAME_COUNT+PAGE_SIZE-1)/PAGE_SIZE);
  for (i=from;i<to;i++) {
    scr_printf("%02d %-11s %-9s %6luB err=%d\n",
      i+1,lab_ids[i],status_text(results[i].status),
      results[i].bytes,results[i].err);
  }
#if defined(LAB_MODE_C)
  scr_printf("\nPP.NETTEST only: verified PNG -> ART + REPORT_C.TXT\n");
#else
  scr_printf("\nRAM ONLY: no HDD/ATA/PFS operations during transfer\n");
#endif
  scr_printf("LEFT/RIGHT: page | START: exit\n");
}

static int net_init(void) {
  struct ip4_addr ip,mask,gw;
  int link,i,r;
  r=load_buf("netman",netman_irx,size_netman_irx,NULL,0);
  if(r<0)return r;
  r=load_buf("smap",smap_irx,size_smap_irx,NULL,0);
  if(r<0)return r;
  IP4_ADDR(&ip,LAB_IP_A,LAB_IP_B,LAB_IP_C,LAB_IP_D);
  IP4_ADDR(&mask,LAB_MASK_A,LAB_MASK_B,LAB_MASK_C,LAB_MASK_D);
  IP4_ADDR(&gw,LAB_GW_A,LAB_GW_B,LAB_GW_C,LAB_GW_D);
  r=ps2ipInit(&ip,&mask,&gw);
  if(r<0)return -330;
  for(i=0;i<6;i++) {
    link=NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS,NULL,0,NULL,0);
    if(link==NETMAN_NETIF_ETH_LINK_STATE_UP)return 0;
    sleep(1);
  }
  return -331;
}
static int socket_ready(int sock,int writeable) {
  fd_set fds;
  struct timeval tv;
  int rc;
  FD_ZERO(&fds);
  FD_SET(sock,&fds);
  tv.tv_sec=writeable?5:35;
  tv.tv_usec=0;
  rc=lwip_select(sock+1,writeable?NULL:&fds,writeable?&fds:NULL,NULL,&tv);
  return rc>0 && FD_ISSET(sock,&fds);
}
static int download_png(const char *id,const uint8_t **png,size_t *png_bytes,
                        unsigned long *read_bytes,int *stage_code) {
  struct sockaddr_in addr;
  struct ip4_addr server;
  char request[170];
  int sock=-1,rc,n,sent=0,soerr=0,stage=0;
  size_t recv_len=0;
  socklen_t optlen;
  *png=NULL;
  *png_bytes=0;
  *read_bytes=0;
  *stage_code=0;
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(LAB_PC_PORT);
  IP4_ADDR(&server,LAB_PC_A,LAB_PC_B,LAB_PC_C,LAB_PC_D);
  memcpy(&addr.sin_addr,&server,sizeof(addr.sin_addr));
  stage=1;
  sock=lwip_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if(sock<0) goto error;
  stage=2;
  if(fcntl(sock,F_SETFL,O_NONBLOCK)<0)goto error;
  stage=3;
  rc=lwip_connect(sock,(struct sockaddr*)&addr,sizeof(addr));
  if(rc<0 && errno!=EINPROGRESS && errno!=EWOULDBLOCK &&
     errno!=EALREADY)goto error;
  if(rc<0) {
    if(!socket_ready(sock,1))goto error;
    optlen=sizeof(soerr);
    if(lwip_getsockopt(sock,SOL_SOCKET,SO_ERROR,&soerr,&optlen)<0 ||
       soerr!=0)goto error;
  }
  n=snprintf(request,sizeof(request),
    "GET /%s_COV.png HTTP/1.0\r\n"
    "Host: %d.%d.%d.%d\r\nConnection: close\r\n\r\n",
    id,LAB_PC_A,LAB_PC_B,LAB_PC_C,LAB_PC_D);
  if(n<=0 || n>=(int)sizeof(request))goto error;
  stage=4;
  while(sent<n) {
    if(!socket_ready(sock,1))goto error;
    rc=lwip_send(sock,request+sent,n-sent,0);
    if(rc<0 && (errno==EWOULDBLOCK || errno==EAGAIN))continue;
    if(rc<=0)goto error;
    sent+=rc;
  }
  stage=5;
  for(;;) {
    if(!socket_ready(sock,0)) {
      /* FIN is not required if the exact Content-Length and PNG CRC already
         match. This check is identical in A/B/C, so comparison is fair. */
      if(recv_len>0 &&
         probe_http_extract_png(net_response,recv_len,png,png_bytes)==0)
        break;
      goto error;
    }
    if(recv_len>=BUF_CAP)goto error;
    {
      size_t remain=BUF_CAP-recv_len;
      if(remain>LAB_RECV_LIMIT)remain=LAB_RECV_LIMIT;
      rc=lwip_recv(sock,net_response+recv_len,(int)remain,0);
    }
    if(rc<0 && (errno==EWOULDBLOCK || errno==EAGAIN))continue;
    if(rc<0)goto error;
    if(rc==0)break;
    recv_len+=(size_t)rc;
  }
  *read_bytes=(unsigned long)recv_len;
  lwip_close(sock);
  rc=probe_http_status(net_response,recv_len);
  if(rc==404)return ST_MISS;
  if(rc!=200){*stage_code=rc;return ST_HTTP;}
  if(probe_http_extract_png(net_response,recv_len,png,png_bytes)<0)
    return ST_BAD;
  return ST_OK;
error:
  *stage_code=stage*1000+errno;
  *read_bytes=(unsigned long)recv_len;
  if(sock>=0)lwip_close(sock);
  return ST_LINK;
}

#if defined(LAB_MODE_C)
static int save_png(const char *id,const uint8_t *png,size_t size) {
  char path[96],partial[96];
  int fd,rc,n,got,verifyfd;
  size_t read_at=0;
  n=snprintf(path,sizeof(path),LAB_ART "%s_COV.png",id);
  if(n<=0 || (size_t)n>=sizeof(path))return -410;
  n=snprintf(partial,sizeof(partial),LAB_ART "%s_COV.png.PART",id);
  if(n<=0 || (size_t)n>=sizeof(partial))return -411;
  fd=fileXioOpen(partial,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_TRUNC,0666);
  if(fd<0)return fd;
  rc=fileXioWrite(fd,png,(int)size);
  got=fileXioClose(fd);
  if(rc!=(int)size || got<0)return -412;
  verifyfd=fileXioOpen(partial,FIO_O_RDONLY,0);
  if(verifyfd<0)return verifyfd;
  while(read_at<size) {
    got=fileXioRead(verifyfd,verify_buffer+read_at,
                    (int)(size-read_at));
    if(got<=0){fileXioClose(verifyfd);return -413;}
    read_at+=(size_t)got;
  }
  got=fileXioClose(verifyfd);
  if(got<0 || memcmp(verify_buffer,png,size)!=0 ||
     probe_png_check(verify_buffer,size)<0)return -414;
  /* Remove only the *same* whitelisted cover file if a previous test
     left it behind; CLEAN should normally be used before C. */
  fileXioRemove(path);
  rc=fileXioRename(partial,path);
  return rc<0 ? rc:0;
}
static int save_report(void) {
  int fd,i,n,done;
  char line[132];
  fd=fileXioOpen(LAB_REPORT,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_TRUNC,0666);
  if(fd<0)return fd;
  n=snprintf(line,sizeof(line),"PSX DVR NETTEST C / PP.NETTEST only\r\n"
     "OK=%d BAD=%d LINK=%d HTTP=%d MISS=%d PFS=%d\r\n",
     summary_ok,summary_bad,summary_link,summary_http,summary_miss,summary_pfs);
  done=fileXioWrite(fd,line,n);
  if(done!=n){fileXioClose(fd);return -420;}
  for(i=0;i<LAB_GAME_COUNT;i++) {
    n=snprintf(line,sizeof(line),"%02d %s %s bytes=%lu err=%d\r\n",
      i+1,lab_ids[i],status_text(results[i].status),
      results[i].bytes,results[i].err);
    done=fileXioWrite(fd,line,n);
    if(done!=n){fileXioClose(fd);return -421;}
  }
  return fileXioClose(fd);
}
#endif

static void run_network_test(void) {
  int i,rc;
  const uint8_t *png;
  size_t png_size;
  unsigned long bytes;
  int diag;
  memset(results,0,sizeof(results));
  summary_ok=summary_bad=summary_link=summary_http=summary_miss=summary_pfs=0;
  scr_clear();
  scr_printf("START TEST %s, %d GAME IDs\n",MODE_LABEL,LAB_GAME_COUNT);
  rc=net_init();
  if(rc<0) {
    scr_printf("NETMAN/SMAP/PS2IP failed %d\n",rc);
    show_error("NETWORK INIT",rc);
    return;
  }
  for(i=0;i<LAB_GAME_COUNT;i++) {
    int status;
    scr_printf("[%02d/44] %s ",i+1,lab_ids[i]);
    status=download_png(lab_ids[i],&png,&png_size,&bytes,&diag);
#if defined(LAB_MODE_C)
    if(status==ST_OK) {
      rc=save_png(lab_ids[i],png,png_size);
      if(rc<0) {diag=rc;status=ST_PFS;}
    }
#endif
    results[i].status=status;
    results[i].err=diag;
    results[i].bytes=bytes;
    switch(status) {
      case ST_OK:summary_ok++;break;
      case ST_BAD:summary_bad++;break;
      case ST_LINK:summary_link++;break;
      case ST_HTTP:summary_http++;break;
      case ST_MISS:summary_miss++;break;
      case ST_PFS:summary_pfs++;break;
    }
    scr_printf("%s bytes=%lu err=%d\n",status_text(status),bytes,diag);
    usleep(200000);
  }
#if defined(LAB_MODE_C)
  rc=save_report();
  scr_printf("REPORT_C.TXT write %d\n",rc);
#endif
  show_report(0);
  {
    int page=0;
    for(;;) {
      int command=wait_command(0,1);
      if(command==-1)break;
      if(command==2 && page<3)page++;
      else if(command==3 && page>0)page--;
      show_report(page);
    }
  }
}

#if defined(LAB_MODE_CLEAN)
static int cleanup_art(void) {
  int i,removed=0,failed=0;
  if(ensure_art()<0)return -500;
  for(i=0;i<LAB_GAME_COUNT;i++) {
    char file[96],part[96];
    int fd;
    snprintf(file,sizeof(file),LAB_ART "%s_COV.png",lab_ids[i]);
    snprintf(part,sizeof(part),LAB_ART "%s_COV.png.PART",lab_ids[i]);
    fd=fileXioOpen(file,FIO_O_RDONLY,0);
    if(fd>=0) {
      fileXioClose(fd);
      if(fileXioRemove(file)<0)failed++;
      else removed++;
    }
    fd=fileXioOpen(part,FIO_O_RDONLY,0);
    if(fd>=0) {
      fileXioClose(fd);
      if(fileXioRemove(part)<0)failed++;
      else removed++;
    }
  }
  scr_printf("CLEAN: deleted %d generated PNG/PART, errors %d\n",
             removed,failed);
  scr_printf("ONLY PP.NETTEST/ART; other files untouched\n");
  return failed==0?0:-501;
}
#endif

int main(int argc,char *argv[]) {
  int r;
  (void)argc;(void)argv;
  init_scr();
  scr_setCursor(0);
  scr_setbgcolor(0);
  scr_setfontcolor(0xFFFFFF);
  scr_printf("PSX DESR NETWORK LAB %s\n",MODE_LABEL);
  scr_printf("Target PC: %d.%d.%d.%d:%d; 44 common IDs\n",
             LAB_PC_A,LAB_PC_B,LAB_PC_C,LAB_PC_D,LAB_PC_PORT);
  r=init_drivers();
  if(r<0){show_error("DRIVER LOAD",r);return 1;}
#if defined(LAB_MODE_C) || defined(LAB_MODE_CLEAN)
  r=mount_sandbox();
  if(r<0){
    scr_printf("NO PP.NETTEST / NOT PFS / MOUNT BUSY: %d\n",r);
    show_error("PP.NETTEST mount",r);
    return 2;
  }
#if defined(LAB_MODE_C)
  r=ensure_art();
  if(r<0){show_error("CREATE/OPEN sandbox ART",r);unmount_sandbox();return 3;}
#endif
#endif
  if (!padInit(0) || !padPortOpen(0,0,padbuf)) {
    show_error("PAD INIT",-1);
#if defined(LAB_MODE_C) || defined(LAB_MODE_CLEAN)
    unmount_sandbox();
#endif
    return 4;
  }
  scr_printf("Mode: %s\n",MODE_LABEL);
#if defined(LAB_MODE_CLEAN)
  scr_printf("CLEAN: press X to delete test-only 44 PNG/PART names\n");
  scr_printf("ONLY PARTITION: %s | START: exit\n",LAB_PARTITION);
  r=wait_command(1,0);
  if(r>0)cleanup_art();
  scr_printf("START: exit\n");
  while(wait_command(0,0)!=-1) {}
#else
  scr_printf("Press X to RUN 44 downloads; START to exit\n");
  scr_printf("C uses only PP.NETTEST; A/B RAM only\n");
  r=wait_command(1,0);
  if(r>0)run_network_test();
#endif
  padPortClose(0,0);
  padEnd();
#if defined(LAB_MODE_C) || defined(LAB_MODE_CLEAN)
  unmount_sandbox();
#endif
  return 0;
}

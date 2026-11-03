/* LAN artwork fetcher integrated with PSX DVR ISO Manager.
 * Reads missing Game IDs from a USB scan report. Downloads validated PNGs
 * into new USB ART files via .PART staging; index updates are caller-owned.
 * DVR/ISO and existing USB art are never modified.
 */
#include "art_sync_runtime.h"
#include "art_sync.h"
#include "probe_png.h"
#include <errno.h>
#include <fcntl.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <io_common.h>
#include <ps2ip.h>
#include <debug.h>
#include "runtime_storage.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define SCAN_LOG DESR_APP_COVER_SCAN_LOG
#define ART_PATH DESR_APP_ART_ROOT
#define BUF_CAP (PROBE_PNG_MAX + PROBE_HTTP_HEADER_MAX)

static char report[ART_SYNC_REPORT_MAX + 1U];
static art_sync_item games[ART_SYNC_GAMES_MAX];
static int game_count;
static int last_attempted, last_http_failed, last_write_failed;
void art_sync_last_stats(int *candidates, int *attempted,
                         int *http_failed, int *write_failed) {
  if (candidates) *candidates=game_count;
  if (attempted) *attempted=last_attempted;
  if (http_failed) *http_failed=last_http_failed;
  if (write_failed) *write_failed=last_write_failed;
}
static uint8_t response[BUF_CAP + 1U];
static uint8_t verification[1024];

static int present(const char *path) {
  int fd=fileXioOpen(path,FIO_O_RDONLY,0);
  if (fd<0) return 0;
  fileXioClose(fd);
  return 1;
}
static int paths_for(const char *id, char *dst, size_t dstcap,
                     char *staging, size_t stagecap,
                     char *jpg, size_t jpgcap) {
  int n;
  if (!art_sync_valid_id(id)) return -1;
  n=snprintf(dst,dstcap,ART_PATH "%s_COV.png",id);
  if (n<0 || (size_t)n>=dstcap) return -1;
  n=snprintf(staging,stagecap,ART_PATH "%s_COV.png.PART",id);
  if (n<0 || (size_t)n>=stagecap) return -1;
  n=snprintf(jpg,jpgcap,ART_PATH "%s_COV.jpg",id);
  return n<0 || (size_t)n>=jpgcap ? -1 : 0;
}
int art_sync_prepare(void) {
  int fd,n,bytes=0,i,remain=0;
  fd=fileXioOpen(SCAN_LOG,FIO_O_RDONLY,0);
  if (fd<0) {
    scr_printf("ERROR: COVER_SCAN.LOG missing.\n");
    scr_printf("Run TRIANGLE in working Manager first.\n");
    return -1;
  }
  for (;;) {
    if (bytes >= (int)ART_SYNC_REPORT_MAX) {
      fileXioClose(fd);
      scr_printf("ERROR: COVER_SCAN.LOG too large.\n");
      return -1;
    }
    n=fileXioRead(fd,report+bytes,(int)ART_SYNC_REPORT_MAX-bytes);
    if (n<0) {fileXioClose(fd); return -1;}
    if (!n) break;
    bytes+=n;
  }
  if (fileXioClose(fd)<0 || !bytes) return -1;
  report[bytes]='\0';
  n=art_sync_collect(report,(size_t)bytes,games,ART_SYNC_GAMES_MAX);
  if (n<0) {
    scr_printf("ERROR: COVER_SCAN.LOG corrupt/invalid.\n");
    return -1;
  }
  game_count=n;
  scr_printf("Missing entries in last scan: %d\n",game_count);
  for (i=0;i<game_count;i++) {
    char dst[96],tmp[96],jpg[96];
    if (paths_for(games[i].id,dst,sizeof(dst),
                  tmp,sizeof(tmp),jpg,sizeof(jpg))<0) return -1;
    if (!present(dst) && !present(jpg)) remain++;
  }
  scr_printf("Unfilled ART files: %d\n",remain);
  if (remain>ART_SYNC_BATCH_MAX)
    scr_printf("Safety: at most %d downloads per run.\n",ART_SYNC_BATCH_MAX);
  return remain;
}
static int ready(int sock, int writable) {
  fd_set fds;
  struct timeval tv;
  int rc;
  FD_ZERO(&fds);
  FD_SET(sock,&fds);
  tv.tv_sec=writable ? 5 : 35; /* Allow PC HTTPS cache miss. */
  tv.tv_usec=0;
  rc=lwip_select(sock+1,writable?NULL:&fds,
                 writable?&fds:NULL,NULL,&tv);
  return rc>0 && FD_ISSET(sock,&fds);
}
static int download(const probe_config *cfg, const char *id,
                    const uint8_t **png, size_t *png_size) {
  struct sockaddr_in addr;
  struct ip4_addr server_addr;
  char req[180],url[40];
  size_t received=0;
  socklen_t optlen;
  int sock=-1,n,rc,soerr=0,sent=0,stage=0;
  if (art_sync_request_path(url,sizeof(url),id)<0) return -1;
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_port=htons(cfg->port);
  IP4_ADDR(&server_addr,cfg->server[0],cfg->server[1],
           cfg->server[2],cfg->server[3]);
  memcpy(&addr.sin_addr,&server_addr,sizeof(addr.sin_addr));
  stage=1;
  sock=lwip_socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if (sock<0) goto fail;
  stage=2;
  if (fcntl(sock,F_SETFL,O_NONBLOCK)<0) goto fail;
  stage=3;
  rc=lwip_connect(sock,(struct sockaddr *)&addr,sizeof(addr));
  if (rc<0 && errno!=EINPROGRESS && errno!=EWOULDBLOCK &&
      errno!=EALREADY) goto fail;
  if (rc<0) {
    if (!ready(sock,1)) goto fail;
    optlen=sizeof(soerr);
    if (lwip_getsockopt(sock,SOL_SOCKET,SO_ERROR,&soerr,&optlen)<0
        ||soerr!=0) goto fail;
  }
  n=snprintf(req,sizeof(req),
    "GET %s HTTP/1.0\r\nHost: %u.%u.%u.%u\r\nConnection: close\r\n\r\n",
    url,cfg->server[0],cfg->server[1],cfg->server[2],cfg->server[3]);
  if (n<0 || n>=(int)sizeof(req)) goto fail;
  stage=4;
  while (sent<n) {
    if (!ready(sock,1)) goto fail;
    rc=lwip_send(sock,req+sent,n-sent,0);
    if (rc<0 && (errno==EWOULDBLOCK || errno==EAGAIN)) continue;
    if (rc<=0) goto fail;
    sent+=rc;
  }
  scr_printf("GET %s\n",url);
  stage=5;
  for (;;) {
    if (!ready(sock,0)) goto fail;
    if (received>=BUF_CAP) goto fail;
    rc=lwip_recv(sock,response+received,BUF_CAP-received,0);
    if (rc<0 && (errno==EWOULDBLOCK||errno==EAGAIN)) continue;
    if (rc<0) goto fail;
    if (rc==0) break;
    received+=(size_t)rc;
  }
  lwip_close(sock);
  if (probe_http_extract_png(response,received,png,png_size)<0) {
    /* Diagnose incomplete TCP body vs unavailable image vs corrupt PNG.
       Preserve the strict PNG CRC and Content-Length checks: NEVER write
       a file merely because HTTP had status 200. */
    size_t h=0,content_at=0,expected=0,body_len=0,i;
    char firstline[28];
    int known_length=0,valid_png=-1;
    for (i=0;i<sizeof(firstline)-1 && i<received;i++) {
      unsigned char c=response[i];
      if (c=='\r' || c=='\n') break;
      firstline[i]=(c>=32 && c<=126)?(char)c:'?';
    }
    firstline[i]='\0';
    for (h=0;h+3<received && h<PROBE_HTTP_HEADER_MAX;h++)
      if (!memcmp(response+h,"\r\n\r\n",4)) {
        content_at=h+4;
        break;
      }
    if (content_at>0) {
      for (i=0;i+16<h;i++) {
        static const char length_tag[]="Content-Length: ";
        if ((i==0 || (i>=2 && response[i-2]=='\r' &&
                       response[i-1]=='\n')) &&
            !memcmp(response+i,length_tag,sizeof(length_tag)-1)) {
          size_t j=i+sizeof(length_tag)-1;
          int digits=0;
          for (;j<h && response[j]>='0' && response[j]<='9';j++) {
            expected=expected*10U+(unsigned)(response[j]-'0');
            digits++;
            if (expected>PROBE_PNG_MAX) break;
          }
          known_length=digits>0;
          break;
        }
      }
      body_len=received-content_at;
      valid_png=probe_png_check(response+content_at,body_len);
    }
    scr_printf("HTTP BAD: %.26s total=%lu\n",
               firstline,(unsigned long)received);
    scr_printf("HTTP BODY: %lu expected=%lu has_len=%d PNG=%d\n",
               (unsigned long)body_len,(unsigned long)expected,
               known_length,valid_png);
    return -6;
  }
  return 0;
fail:
  /* Stage 1=socket, 2=nonblocking, 3=connect, 4=send, 5=receive.
     errno and stage give a useful diagnosis if the second cover fails. */
  rc=errno;
  if (sock>=0) lwip_close(sock);
  scr_printf("HTTP FAILURE stage=%d errno=%d got=%lu\n",
             stage,rc,(unsigned long)received);
  return -stage;
}
static int write_new_art(const char *id, const uint8_t *bytes, size_t count) {
  char dst[96],tmp[96],jpg[96];
  int fd,n,pos=0;
  if (!bytes || !count || count>PROBE_PNG_MAX ||
      paths_for(id,dst,sizeof(dst),tmp,sizeof(tmp),jpg,sizeof(jpg))<0)
    return -1;
  /* No replacement, even of existing .jpg. Pre-existing PART is
     considered a possible interrupted transfer and left untouched. */
  if (present(dst) || present(jpg) || present(tmp)) {
    scr_printf("SKIP: destination or .PART already exists\n");
    return -1;
  }
  fd=fileXioOpen(tmp,FIO_O_WRONLY|FIO_O_CREAT|FIO_O_TRUNC,0666);
  if (fd<0) {scr_printf("ERROR: cannot create ART .PART\n");return -1;}
  while (pos<(int)count) {
    int want=(int)count-pos;
    if (want>4096) want=4096;
    n=fileXioWrite(fd,bytes+pos,want);
    if (n<=0) {fileXioClose(fd);return -1;}
    pos+=n;
  }
  if (fileXioClose(fd)<0) return -1;
  /* Read back the complete staging file, byte-for-byte. */
  fd=fileXioOpen(tmp,FIO_O_RDONLY,0);
  if (fd<0) return -1;
  pos=0;
  while (pos<(int)count) {
    int want=(int)count-pos;
    if (want>(int)sizeof(verification)) want=sizeof(verification);
    n=fileXioRead(fd,verification,want);
    if (n<=0 || memcmp(bytes+pos,verification,(size_t)n)) {
      fileXioClose(fd);return -1;
    }
    pos+=n;
  }
  n=fileXioRead(fd,verification,1);
  if (fileXioClose(fd)<0 || n!=0) return -1;
  if (present(dst) || present(jpg)) return -1;
  if (fileXioRename(tmp,dst)<0) {
    scr_printf("ERROR: ART rename failed; .PART retained\n");
    return -1;
  }
  return 0;
}
int art_sync_run(const probe_config *cfg) {
  int i,attempted=0,saved=0,fail_streak=0,http_failed=0,write_failed=0,stale=0;
  last_attempted=last_http_failed=last_write_failed=0;
  if (!cfg || game_count<1) return -1;
  for (i=0;i<game_count && attempted<ART_SYNC_BATCH_MAX;i++) {
    char dst[96],tmp[96],jpg[96];
    const uint8_t *png=NULL;
    size_t bytes=0;
    if (paths_for(games[i].id,dst,sizeof(dst),tmp,sizeof(tmp),
                  jpg,sizeof(jpg))<0) continue;
    if (present(dst)||present(jpg)) continue;
    if (present(tmp)) {
      stale++;
      scr_printf("SKIP %s: stale .PART\n",games[i].id);
      continue;
    }
    attempted++;
    scr_printf("ART %d/%d: %s\n",attempted,game_count,games[i].id);
    {
      int retry, got=-1;
      /* Two connections at most, each closes cleanly before next request. */
      for (retry=0;retry<2;retry++) {
        got=download(cfg,games[i].id,&png,&bytes);
        if (got==0) break;
        scr_printf("HTTP retry %d/2: code %d\n",retry+1,got);
        if (retry==0) usleep(300000);
      }
      if (got<0) {
        http_failed++;
        scr_printf("ART HTTP FAILED: %s\n",games[i].id);
        if (++fail_streak>=4) {
          scr_printf("STOP: 4 consecutive HTTP failures; check PC server.\n");
          break;
        }
        continue;
      }
    }
    fail_streak=0;
    scr_printf("PNG: CRC OK, %lu bytes\n",(unsigned long)bytes);
    if (write_new_art(games[i].id,png,bytes)<0) {
      write_failed++;
      scr_printf("FAIL: ART staging/verify/rename\n");
      continue;
    }
    scr_printf("SAVED: %s_COV.png\n",games[i].id);
    saved++;
    /* Let the console TCP stack release the last closed socket before
       opening the next one; never halt after the first successful cover. */
    usleep(200000);
  }
  last_attempted=attempted;
  last_http_failed=http_failed;
  last_write_failed=write_failed;
  scr_printf("ART DONE: candidates=%d tried=%d saved=%d\n",
             game_count,attempted,saved);
  scr_printf("ART ERRORS: HTTP=%d PFS_WRITE=%d STALE_PART=%d\n",
             http_failed,write_failed,stale);
  if (saved>0)
    scr_printf("New PNG covers saved in app ART.\n");
  return saved>0 ? saved : -1;
}

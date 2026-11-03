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
static int last_attempted, last_not_found, last_http_failed, last_write_failed;
static int last_bad_payload, last_transport_fail, last_http_status_fail;
/* HDD-only diagnostic snapshot, no extra HDD writes. A BAD reason is:
 * 1 short HTTP body; 2 longer-than-declared body; 3 complete but invalid PNG;
 * 4 header/length could not be parsed. */
static char last_bad_id[ART_SYNC_ID_CAP];
static unsigned long last_bad_body, last_bad_expected;
static int last_bad_reason;
void art_sync_last_bad_detail(char *id, size_t cap, unsigned long *body,
                              unsigned long *expected, int *reason) {
  if (id && cap) {
    snprintf(id,cap,"%s",last_bad_id);
  }
  if (body) *body=last_bad_body;
  if (expected) *expected=last_bad_expected;
  if (reason) *reason=last_bad_reason;
}

void art_sync_last_net_details(int *bad_payload, int *transport_fail,
                               int *http_status_fail) {
  if (bad_payload) *bad_payload=last_bad_payload;
  if (transport_fail) *transport_fail=last_transport_fail;
  if (http_status_fail) *http_status_fail=last_http_status_fail;
}
void art_sync_last_stats(int *candidates, int *attempted, int *not_found,
                         int *http_failed, int *write_failed) {
  if (candidates) *candidates=game_count;
  if (attempted) *attempted=last_attempted;
  if (not_found) *not_found=last_not_found;
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
    if (!ready(sock,0)) {
#ifdef DESR_HDD_TEST
      /* Some LAN peers delay FIN after fully sending Content-Length.
         Accept only a byte-complete and CRC-validated PNG; a short or
         malformed response is still an error, never written to HDD. */
      if (received>0 &&
          probe_http_extract_png(response,received,png,png_size)==0) {
        scr_printf("HTTP full PNG received before FIN\n");
        lwip_close(sock);
        return 0;
      }
#endif
      goto fail;
    }
    if (received>=BUF_CAP) goto fail;
    {
      size_t remaining=BUF_CAP-received;
#ifdef DESR_HDD_ART_PROBE
      /* PS2SDK ps2ips RPC uses an IOP staging buffer BUFF_SIZE=1024
         with only 32 bytes slack, although the DMA base can be offset
         by up to 64. Request <=512 per recv as a controlled probe for
         the known large-recv buffer-overrun/corrupted-PNG symptom.
         DO NOT apply to the proven USB/HDD game launcher builds yet. */
      if (remaining>512U) remaining=512U;
#endif
      rc=lwip_recv(sock,response+received,(int)remaining,0);
    }
    if (rc<0 && (errno==EWOULDBLOCK||errno==EAGAIN)) continue;
    if (rc<0) goto fail;
    if (rc==0) break;
    received+=(size_t)rc;
  }
  lwip_close(sock);
#ifdef DESR_HDD_TEST
  {
    int http_status = probe_http_status(response,received);
    /* PC returns 404 when neither OPL nor PSBBN has this Game ID.
       Missing artwork is NOT a transient socket failure: skip this
       game, avoid retrying the same permanent 404, continue the queue. */
    if (http_status==404) {
      scr_printf("ART ABSENT: %s (PC HTTP 404)\n", id);
      return -404;
    }
    if (http_status==403) {
      scr_printf("ART BLOCKED: PC HTTP 403; verify console IP\n");
      return -403;
    }
    if (http_status!=200) {
      scr_printf("ART HTTP STATUS=%d for %s\n",http_status,id);
      return -7;
    }
  }
#endif
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
#ifdef DESR_HDD_TEST
    snprintf(last_bad_id,sizeof last_bad_id,"%s",id);
    last_bad_body=(unsigned long)body_len;
    last_bad_expected=(unsigned long)expected;
    last_bad_reason=(!content_at || !known_length) ? 4
                    : body_len<expected ? 1
                    : body_len>expected ? 2 : 3;
    scr_printf("HTTP BAD GAME %s reason=%d\n",id,last_bad_reason);
#endif
    scr_printf("HTTP BAD: %.26s total=%lu\n",
               firstline,(unsigned long)received);
    scr_printf("HTTP BODY: %lu expected=%lu has_len=%d PNG=%d\n",
               (unsigned long)body_len,(unsigned long)expected,
               known_length,valid_png);
#ifdef DESR_HDD_ART_PROBE
    if (content_at>0 && body_len==expected && known_length) {
      char kind[5];
      size_t at=0;
      uint32_t want=0,got=0;
      int checked=probe_png_crc_detail(response+content_at,body_len,
                                       kind,&at,&want,&got);
      scr_printf("PNG CRC DETAIL: rc=%d chunk=%s offset=%lu\n",
                 checked,kind,(unsigned long)at);
      if (checked==1)
        scr_printf("PNG CRC: EXPECT=%08lx ACTUAL=%08lx\n",
                   (unsigned long)want,(unsigned long)got);
    }
#endif
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
  int i,attempted=0,saved=0,fail_streak=0,not_found=0,http_failed=0,write_failed=0,stale=0;
  last_attempted=last_not_found=last_http_failed=last_write_failed=0;
  last_bad_payload=last_transport_fail=last_http_status_fail=0;
  last_bad_id[0]='\0';
  last_bad_body=last_bad_expected=0;
  last_bad_reason=0;
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
        if (got==0 || got==-404 || got==-403) break;
        scr_printf("HTTP retry %d/2: code %d\n",retry+1,got);
        if (retry==0) usleep(300000);
      }
      if (got==-404) {
        not_found++;
        fail_streak=0;
        scr_printf("SKIP NOT FOUND: %s; next game\n",games[i].id);
        continue;
      }
      if (got==-403) {
        http_failed++;
        last_http_status_fail++;
        scr_printf("STOP HTTP 403: PC rejects console IP; check server settings\n");
        break;
      }
      if (got<0) {
        http_failed++;
#ifdef DESR_HDD_TEST
        /* A 200 response whose PNG/body is invalid is a per-game failure,
           NOT a global LAN outage. Continue trying the next game.
           Keep the 4-failure safety stop only for repeated transport or
           actual HTTP server errors (e.g. 503). */
        if (got==-6) {
          last_bad_payload++;
          fail_streak=0;
          scr_printf("SKIP BAD HTTP/PNG for %s; next game\n", games[i].id);
          continue;
        }
        if (got==-7) last_http_status_fail++;
        else last_transport_fail++;
#endif
        scr_printf("ART HTTP FAILED: %s code=%d\n",games[i].id,got);
        if (++fail_streak>=4) {
          scr_printf("STOP: 4 consecutive network/server failures.\n");
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
  last_not_found=not_found;
  last_http_failed=http_failed;
  last_write_failed=write_failed;
  scr_printf("ART DONE: candidates=%d tried=%d saved=%d\n",
             game_count,attempted,saved);
  scr_printf("ART RESULTS: NOT_FOUND=%d HTTP_FAIL=%d PFS_WRITE=%d STALE_PART=%d\n",
             not_found,http_failed,write_failed,stale);
#ifdef DESR_HDD_TEST
  scr_printf("HTTP DETAIL: BAD_PNG=%d LINK=%d STATUS=%d\n",
             last_bad_payload,last_transport_fail,last_http_status_fail);
#endif
  if (saved>0)
    scr_printf("New PNG covers saved in app ART.\n");
  return saved>0 ? saved : -1;
}

#ifdef DESR_HDD_ART_PROBE
static int probe_ok,probe_total,probe_bad,probe_link,probe_missing,probe_status,probe_skipped;
void art_sync_probe_stats(int *ok,int *total,int *bad,int *link,int *http,int *missing) {
  if (ok) *ok=probe_ok;
  if (total) *total=probe_total;
  if (bad) *bad=probe_bad;
  if (link) *link=probe_link;
  if (http) *http=probe_status;
  if (missing) *missing=probe_missing;
}
/* Strict read-only benchmark of the very same download()/PNG checker.
 * Pull ALL Game IDs from the already-loaded COVERS.IDX, including artwork
 * that exists, to avoid any filesystem writes and eliminate PFS write
 * contention from the experiment.
 */
int art_sync_probe_all(const probe_config *cfg,
                       const desr_cover_entry covers[], int total) {
  int i, ok=0, bad=0, link=0, missing=0, status=0, skipped=0;
  if (!cfg || !covers || total<=0 || total>DESR_GAME_CAP) return -1;
  for (i=0;i<total;i++) {
    const uint8_t *img=NULL;
    size_t len=0;
    int rc;
    if (!art_sync_valid_id(covers[i].game_id)) {
      skipped++;
      continue;
    }
    scr_printf("RAM PROBE %d/%d: %s\n",i+1,total,covers[i].game_id);
    rc=download(cfg,covers[i].game_id,&img,&len);
    if (rc==0) {
      ok++;
      scr_printf("RAM OK %s: %lu bytes, valid CRC\n",
                 covers[i].game_id,(unsigned long)len);
    } else if (rc==-6) {
      bad++;
    } else if (rc==-404) {
      missing++;
    } else if (rc==-7 || rc==-403) {
      status++;
    } else {
      link++;
    }
    /* Deliberate spacing to avoid mistaking a back-to-back socket reuse
       race for invalid PNG bytes; no fileXio writes anywhere in this loop. */
    usleep(200000);
  }
  probe_ok=ok;
  probe_total=total;
  probe_bad=bad;
  probe_link=link;
  probe_status=status;
  probe_missing=missing;
  probe_skipped=skipped;
  scr_printf("RAM DONE %d/%d VALID, BAD%d LINK%d H%d MISS%d SKIP%d\n",
             ok,total,bad,link,status,missing,skipped);
  scr_printf("NO FILES WRITTEN. PFS1 LEFT READ-ONLY.\n");
  return ok;
}
#endif

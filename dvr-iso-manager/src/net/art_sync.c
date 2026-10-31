#include "art_sync.h"
#include <stdio.h>
#include <string.h>

#define LOG_HEADER "PSXCOVSCAN1\n"

int art_sync_valid_id(const char *id) {
  int i;
  if (!id || strlen(id)!=11 || id[4]!='_' || id[8]!='.') return 0;
  for (i=0;i<4;i++) if (id[i]<'A'||id[i]>'Z') return 0;
  for (i=5;i<11;i++)
    if (i!=8 && (id[i]<'0'||id[i]>'9')) return 0;
  return 1;
}
static int valid_iso_name(const char *s) {
  size_t i,n;
  if (!s) return 0;
  n=strlen(s);
  if (n<5 || n>=ART_SYNC_ISO_CAP) return 0;
  for (i=0;i<n;i++) {
    unsigned char c=(unsigned char)s[i];
    if (c<32 || c>126 || c=='/' || c=='\\' || c==':' ||
        c=='*' || c=='?' || c=='"' || c=='<' || c=='>' || c=='|')
      return 0;
  }
  return s[n-4]=='.' &&
         (s[n-3]=='i'||s[n-3]=='I') &&
         (s[n-2]=='s'||s[n-2]=='S') &&
         (s[n-1]=='o'||s[n-1]=='O');
}
int art_sync_request_path(char *out, size_t cap, const char *id) {
  int n;
  if (!out || !cap || !art_sync_valid_id(id)) return -1;
  n=snprintf(out,cap,"/%s_COV.png",id);
  return n>0 && (size_t)n<cap ? 0 : -1;
}
int art_sync_collect(char *log, size_t length,
                     art_sync_item *items, size_t capacity) {
  size_t at=sizeof(LOG_HEADER)-1;
  int found=0;
  if (!log || !items || capacity<1 || capacity>ART_SYNC_GAMES_MAX ||
      length<=at || length>ART_SYNC_REPORT_MAX ||
      memcmp(log,LOG_HEADER,at) || log[length-1]!='\n') return -1;

  while (at<length) {
    char *line, *t1, *t2, *nl;
    size_t len;
    int match, i;
    nl=(char *)memchr(log+at,'\n',length-at);
    if (!nl) return -1;
    len=(size_t)(nl-(log+at));
    if (!len || len>220) return -1;
    line=log+at;
    *nl='\0';
    t1=strchr(line,'\t');
    if (!t1) return -1;
    *t1++='\0';
    t2=strchr(t1,'\t');
    if (!t2) return -1;
    *t2++='\0';
    if (strchr(t2,'\t') || !valid_iso_name(line)) return -1;
    match=(!strcmp(t1,"NO_ART") || !strcmp(t1,"NO_ART_EXE"));
    if (match) {
      if (!art_sync_valid_id(t2)) return -1;
      for (i=0;i<found;i++) {
        if (!strcmp(items[i].id,t2)) break;
      }
      if (i==found) {
        if ((size_t)found>=capacity) return -1;
        memcpy(items[found].id,t2,ART_SYNC_ID_CAP);
        memcpy(items[found].filename,line,strlen(line)+1);
        found++;
      }
    } else {
      /* All other report statuses are skipped, but require a plausible
         ID or '-' so an overwritten/corrupted log cannot be trusted. */
      if (!art_sync_valid_id(t2) && strcmp(t2,"-")) return -1;
    }
    at=(size_t)(nl-log)+1;
  }
  return found;
}

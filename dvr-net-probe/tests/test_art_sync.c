#include "art_sync.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char example[]=
  "PSXCOVSCAN1\n"
  "Age of Empires II The Age of Kings.iso\tFOUND_PNG\tSLES_502.82\n"
  "Black.iso\tNO_ART_EXE\tSLES_538.86\n"
  "Burnout Dominator.iso\tNO_ART\tSLES_546.27\n"
  "Killzone.ISO\tNO_ART_EXE\tSCUS_974.02\n"
  "Mafia.iso\tFOUND_PNG_EXE\tSLES_522.78\n";
int main(void) {
  char buf[2048],url[40],bad[2048];
  art_sync_item items[ART_SYNC_GAMES_MAX];
  int n;
  strcpy(buf,example);
  n=art_sync_collect(buf,strlen(buf),items,ART_SYNC_GAMES_MAX);
  assert(n==3);
  assert(!strcmp(items[0].id,"SLES_538.86"));
  assert(!strcmp(items[0].filename,"Black.iso"));
  assert(!strcmp(items[1].id,"SLES_546.27"));
  assert(!strcmp(items[2].id,"SCUS_974.02"));
  assert(art_sync_request_path(url,sizeof(url),items[0].id)==0);
  assert(!strcmp(url,"/SLES_538.86_COV.png"));
  assert(art_sync_request_path(url,4,items[0].id)<0);
  assert(art_sync_request_path(url,sizeof(url),"../../bad")<0);
  assert(!art_sync_valid_id("SLES_538.8"));
  assert(art_sync_valid_id("SCUS_974.02"));
  strcpy(buf,example);
  assert(art_sync_collect(buf,strlen(buf),items,2)<0);
  strcpy(bad,example);
  bad[strlen(bad)-1]='x';
  assert(art_sync_collect(bad,strlen(bad),items,ART_SYNC_GAMES_MAX)<0);
  strcpy(bad,example);
  memcpy(bad,"PSXEVIL",7);
  assert(art_sync_collect(bad,strlen(bad),items,ART_SYNC_GAMES_MAX)<0);
  strcpy(bad,example);
  {
    char *p=strstr(bad,"Black.iso");
    assert(p); p[0]='/'; /* no directory traversal */
  }
  assert(art_sync_collect(bad,strlen(bad),items,ART_SYNC_GAMES_MAX)<0);
  strcpy(bad,example);
  {
    char *p=strstr(bad,"SLES_538.86");
    assert(p); p[0]='.'; /* fail closed on invalid IDs */
  }
  assert(art_sync_collect(bad,strlen(bad),items,ART_SYNC_GAMES_MAX)<0);
  /* Empty valid scan: all covers already indexed. */
  strcpy(buf,"PSXCOVSCAN1\n"
             "Black.iso\tFOUND_PNG_EXE\tSLES_538.86\n");
  assert(art_sync_collect(buf,strlen(buf),items,ART_SYNC_GAMES_MAX)==0);
  puts("OK: only missing art, safe ISO names, valid IDs, bounded candidates");
  return 0;
}

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sg_policy.h"
int main(void){
  char p[32];
  assert(sg_iso_filename_allowed("ICO.iso"));
  assert(sg_iso_filename_allowed("Killzone.ISO"));
  assert(sg_target_partition("ICO.iso","SCES_507.60",p,sizeof(p)));
  assert(!strcmp(p,"PP.ICO-SCES50760"));
  assert(sg_target_partition("Killzone.ISO","SCUS_974.02",p,sizeof(p)));
  assert(!strcmp(p,"PP.KILLZONE-SCUS97402"));
  assert(!sg_iso_filename_allowed("broken..iso"));
  assert(!sg_iso_filename_allowed("malicious=name.iso"));
  assert(!sg_iso_filename_allowed("dir/game.iso"));
  assert(!sg_iso_filename_allowed("game\\data.iso"));
  assert(!sg_iso_filename_allowed("bad\nname.iso"));
  assert(!sg_iso_filename_allowed("game.jpg"));
  assert(!sg_iso_filename_allowed("bad.iso/"));
  assert(!sg_iso_filename_allowed("game\303\263.iso"));
  char big[102];memset(big,'x',96);memcpy(big+96,".iso",5);
  assert(!sg_iso_filename_allowed(big));
  assert(!sg_target_partition("ICO.iso","NOT_AN_ID",p,sizeof(p)));
  assert(!sg_target_partition("ICO.iso","SCES_507.60",p,12));
  assert(!sg_target_partition("bad.iso/","SCES_507.60",p,sizeof(p)));
  assert(sg_target_partition("Metal Slug Anthology.iso","SLUS_214.50",p,sizeof(p)));
  assert(!strcmp(p,"PP.METALSLUGA-SLUS21450"));
  puts("PASS: FIX95-safe filenames, deterministic APA names, buffer bounds.");
  return 0;
}

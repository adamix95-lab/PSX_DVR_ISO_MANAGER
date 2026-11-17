#include "pdim12_policy.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void) {
  assert(pdim12_partition_name_allowed("PP.DVRISO12"));
  assert(!pdim12_partition_name_allowed("PP.DVRISO"));
  assert(!pdim12_partition_name_allowed("PP.DVRISO123"));
  assert(!pdim12_partition_name_allowed("__system"));
  assert(pdim12_path_allowed("DVR_ISO_MANAGER_HDD.ELF"));
  assert(pdim12_path_allowed("modules/ee_core_lang.elf"));
  assert(pdim12_path_allowed("res/jkt_002.png"));
  for (unsigned i=0;i<sizeof((const char*[]){"", "../bad", "x/../y","ART/game.png","BOOT.CNF","V12_NET.CNF","/absolute","x//y","x\\evil","hdd0:/evil"})/sizeof(char*);i++) {
    const char *bad[]={"", "../bad", "x/../y","ART/game.png","BOOT.CNF","V12_NET.CNF","/absolute","x//y","x\\evil","hdd0:/evil"};
    assert(!pdim12_path_allowed(bad[i]));
  }
  assert(!pdim12_address_safe(0,PDIM12_SECTORS));
  assert(!pdim12_address_safe(0x0FFF0000U,PDIM12_SECTORS));
  assert(pdim12_address_safe(0x0FE00000U,PDIM12_SECTORS));
  const char *data="123456789";
  assert(pdim12_crc32_end(pdim12_crc32_update(0xffffffffu,data,strlen(data)))==0xCBF43926u);
  puts("PASS: strictly limited HDD12 destination, no traversal, no mutable user files, LBA28 and CRC32");
  return 0;
}

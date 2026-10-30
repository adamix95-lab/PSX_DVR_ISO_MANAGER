#include "cover_report.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  char buf[512];
  size_t used = 0;
  assert(desr_cover_report_init(buf, sizeof(buf), &used) == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Black.iso", DESR_SCAN_NO_ID, NULL) == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Burnout Dominator.iso", DESR_SCAN_NO_ART, "SLES_123.45") == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "God of War.iso", DESR_SCAN_FOUND_PNG, "SCUS_973.99") == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Mafia.iso", DESR_SCAN_READ_ERROR, NULL) == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Killzone.ISO", DESR_SCAN_NO_PVD, NULL) == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Shadow of the Colossus.iso", DESR_SCAN_NO_SYSTEM, NULL) == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Wipeout Fusion.iso", DESR_SCAN_BAD_BOOT2, NULL) == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Black.iso", DESR_SCAN_AMBIGUOUS_EXE, NULL) == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Killzone.ISO", DESR_SCAN_FOUND_PNG_EXE, "SCES_520.04") == 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "Mafia.iso", DESR_SCAN_NO_ART_EXE, "SLES_533.19") == 0);
  assert(strcmp(buf, "PSXCOVSCAN1\n"
    "Black.iso\tNO_ID\t-\n"
    "Burnout Dominator.iso\tNO_ART\tSLES_123.45\n"
    "God of War.iso\tFOUND_PNG\tSCUS_973.99\n"
    "Mafia.iso\tREAD_ERROR\t-\n"
    "Killzone.ISO\tNO_PVD\t-\n"
    "Shadow of the Colossus.iso\tNO_SYSTEM\t-\n"
    "Wipeout Fusion.iso\tBAD_BOOT2\t-\n"
    "Black.iso\tAMBIGUOUS_EXE\t-\n"
    "Killzone.ISO\tFOUND_PNG_EXE\tSCES_520.04\n"
    "Mafia.iso\tNO_ART_EXE\tSLES_533.19\n") == 0);

  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "../x.iso", DESR_SCAN_NO_ID, NULL) < 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "X.iso", DESR_SCAN_NO_ART, "INVALID") < 0);
  assert(desr_cover_report_add(buf, sizeof(buf), &used,
         "X.iso", (enum desr_cover_scan_status)99, NULL) < 0);
  assert(desr_cover_report_init(buf, 2, &used) < 0);
  assert(desr_cover_report_init(buf, sizeof(buf), &used) == 0);
  assert(desr_cover_report_add(buf, 15, &used,
         "Name of a large ISO.iso", DESR_SCAN_NO_ID, NULL) < 0);
  puts("OK: bounded per-title cover scan diagnostic with safe filenames/IDs");
  return 0;
}

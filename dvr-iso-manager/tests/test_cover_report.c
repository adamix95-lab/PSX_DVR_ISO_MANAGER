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
  assert(strcmp(buf, "PSXCOVSCAN1\n"
    "Black.iso\tNO_ID\t-\n"
    "Burnout Dominator.iso\tNO_ART\tSLES_123.45\n"
    "God of War.iso\tFOUND_PNG\tSCUS_973.99\n"
    "Mafia.iso\tREAD_ERROR\t-\n") == 0);

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

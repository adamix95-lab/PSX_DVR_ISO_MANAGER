#include "cover_report.h"
#include "cover_index.h"
#include <stdio.h>
#include <string.h>

#define REPORT_HEADER "PSXCOVSCAN1\n"

int desr_cover_report_init(char *out, size_t cap, size_t *used) {
  const size_t len = sizeof(REPORT_HEADER) - 1;
  if (!out || !used || cap <= len) return -1;
  memcpy(out, REPORT_HEADER, len);
  out[len] = '\0';
  *used = len;
  return 0;
}

int desr_cover_report_add(char *out, size_t cap, size_t *used,
                          const char *iso_filename,
                          enum desr_cover_scan_status status,
                          const char *game_id) {
  const char *label;
  int n;
  if (!out || !used || !desr_filename_is_safe(iso_filename) ||
      *used >= cap || out[*used] != '\0') return -1;
  if (status == DESR_SCAN_NO_ID) label = "NO_ID";
  else if (status == DESR_SCAN_NO_ART) label = "NO_ART";
  else if (status == DESR_SCAN_FOUND_PNG) label = "FOUND_PNG";
  else if (status == DESR_SCAN_FOUND_JPG) label = "FOUND_JPG";
  else if (status == DESR_SCAN_READ_ERROR) label = "READ_ERROR";
  else return -1;

  if (status == DESR_SCAN_NO_ART || status == DESR_SCAN_FOUND_PNG ||
      status == DESR_SCAN_FOUND_JPG) {
    if (!desr_valid_game_id(game_id)) return -1;
  } else game_id = "-";

  n = snprintf(out + *used, cap - *used, "%s\t%s\t%s\n",
               iso_filename, label, game_id);
  if (n < 0 || (size_t)n >= cap - *used) return -1;
  *used += (size_t)n;
  return 0;
}

#ifndef DESR_COVER_REPORT_H
#define DESR_COVER_REPORT_H

#include <stddef.h>

/* Optional USB-only diagnostic produced on explicit TRIANGLE rescan.
   COVERS.IDX format stays untouched. This helper is also host-testable. */
enum desr_cover_scan_status {
  DESR_SCAN_NO_ID = 0,
  DESR_SCAN_NO_ART,
  DESR_SCAN_FOUND_PNG,
  DESR_SCAN_FOUND_JPG,
  DESR_SCAN_READ_ERROR
};

/* Return 0 on success, -1 when buffer/input is invalid or full. */
int desr_cover_report_init(char *out, size_t cap, size_t *used);
int desr_cover_report_add(char *out, size_t cap, size_t *used,
                          const char *iso_filename,
                          enum desr_cover_scan_status status,
                          const char *game_id);
#endif

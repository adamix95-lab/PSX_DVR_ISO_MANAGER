#ifndef DESR_USB12_PROGRESS_UI_H
#define DESR_USB12_PROGRESS_UI_H
#include <stddef.h>
#include <stdint.h>

/* UI-only counter; NO networking, storage, heap, or PS2 SDK dependency.
 * The 1-based ordinal is the attempt number, NOT the number of saves.
 */
#define USB12_UI_MAX_COVERS 50
#define USB12_UI_VISIBLE_ROWS 12

enum usb12_ui_result {
  USB12_UI_NONE = 0,
  USB12_UI_OK = 1,
  USB12_UI_SKIPPED = 2,
  USB12_UI_ERROR = 3
};

typedef struct {
  int total;
  int completed;
  int saved;
  int skipped;
  int errors;
  uint8_t results[USB12_UI_MAX_COVERS];
} usb12_progress_ui;

/* Call once after the read-only DVR scan. Reject invalid totals. */
int usb12_progress_begin(usb12_progress_ui *ui, int total);
/* Record only the next attempt, exactly once. 1-based index. */
int usb12_progress_record(usb12_progress_ui *ui, int ordinal,
                         enum usb12_ui_result result);
/* Render a completed attempt as e.g. "1/45 OK" or "2/45 POMINIETO". */
int usb12_progress_line(const usb12_progress_ui *ui, int ordinal,
                        char *dst, size_t capacity);
#endif

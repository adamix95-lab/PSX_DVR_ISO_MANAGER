/* Simple Polish USB 1.2 HTTPS progress messages, ASCII-only for PS2 debug font.
 * Pure formatting only. Rendering happens on the EE main thread between
 * network/TLS operations. This module MUST NOT touch IOP, sockets or GS.
 */
#include "usb12_progress_ui.h"
#include <stdio.h>
#include <string.h>

int usb12_progress_begin(usb12_progress_ui *ui, int total) {
  if (!ui || total < 0 || total > USB12_UI_MAX_COVERS) return -1;
  memset(ui, 0, sizeof(*ui));
  ui->total = total;
  return 0;
}

int usb12_progress_record(usb12_progress_ui *ui, int ordinal,
                         enum usb12_ui_result result) {
  if (!ui || ordinal != ui->completed + 1 ||
      ordinal < 1 || ordinal > ui->total ||
      (result != USB12_UI_OK && result != USB12_UI_SKIPPED &&
       result != USB12_UI_ERROR))
    return -1;
  ui->results[ordinal - 1] = (uint8_t)result;
  ui->completed = ordinal;
  if (result == USB12_UI_OK) ++ui->saved;
  if (result == USB12_UI_SKIPPED) ++ui->skipped;
  if (result == USB12_UI_ERROR) ++ui->errors;
  return 0;
}

int usb12_progress_line(const usb12_progress_ui *ui, int ordinal,
                        char *dst, size_t capacity) {
  const char *label = NULL;
  int n;
  if (!ui || !dst || capacity == 0 ||
      ordinal < 1 || ordinal > ui->completed) return -1;
  switch (ui->results[ordinal - 1]) {
    case USB12_UI_OK:      label = "OK"; break;
    case USB12_UI_SKIPPED: label = "POMINIETO"; break;
    case USB12_UI_ERROR:   label = "BLAD"; break;
    default: return -1;
  }
  n = snprintf(dst, capacity, "%d/%d %s", ordinal, ui->total, label);
  return n >= 0 && (size_t)n < capacity ? 0 : -1;
}

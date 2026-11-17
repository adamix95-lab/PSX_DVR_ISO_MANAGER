#include "usb12_progress_ui.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main(void) {
  usb12_progress_ui ui;
  char text[40], small[5];
  assert(usb12_progress_begin(NULL, 45) != 0);
  assert(usb12_progress_begin(&ui, 51) != 0);
  assert(usb12_progress_begin(&ui, 45) == 0);
  assert(ui.total == 45 && ui.saved == 0 && ui.completed == 0);
  assert(usb12_progress_line(&ui, 1, text, sizeof text) != 0);
  assert(usb12_progress_record(&ui, 2, USB12_UI_OK) != 0);
  assert(usb12_progress_record(&ui, 1, USB12_UI_OK) == 0);
  assert(usb12_progress_record(&ui, 1, USB12_UI_OK) != 0);
  assert(usb12_progress_line(&ui, 1, text, sizeof text) == 0);
  assert(strcmp(text, "1/45 OK") == 0);
  assert(usb12_progress_record(&ui, 2, USB12_UI_SKIPPED) == 0);
  assert(usb12_progress_line(&ui, 2, text, sizeof text) == 0);
  assert(strcmp(text, "2/45 SKIPPED") == 0);
  assert(usb12_progress_record(&ui, 3, USB12_UI_ERROR) == 0);
  assert(usb12_progress_line(&ui, 3, text, sizeof text) == 0);
  assert(strcmp(text, "3/45 ERROR") == 0);
  assert(usb12_progress_line(&ui, 3, small, sizeof small) != 0);
  assert(ui.saved == 1 && ui.skipped == 1 && ui.errors == 1 &&
         ui.completed == 3);
  assert(usb12_progress_record(&ui, 4, USB12_UI_NONE) != 0);
  assert(usb12_progress_begin(&ui, 50) == 0);
  for (int i = 1; i <= 50; ++i)
    assert(usb12_progress_record(&ui, i, USB12_UI_OK) == 0);
  assert(ui.completed == 50 && ui.saved == 50);
  assert(usb12_progress_line(&ui, 50, text, sizeof text) == 0);
  assert(strcmp(text, "50/50 OK") == 0);
  assert(usb12_progress_record(&ui, 51, USB12_UI_OK) != 0);
  assert(usb12_progress_begin(&ui, 0) == 0);
  assert(ui.total == 0 && ui.completed == 0 && ui.saved == 0);
  puts("PASS: USB12 progress UI bounds, labels, scrolling source state");
  return 0;
}

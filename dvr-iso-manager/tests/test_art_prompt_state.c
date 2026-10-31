#include "art_prompt_state.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
  desr_art_prompt_state s;
  desr_art_prompt_reset(&s);
  assert(!s.active && !s.missing && !s.report_error);

  /* Real user regression: TRIANGLE -> cancel O -> TRIANGLE again -> X. */
  assert(desr_art_prompt_after_scan(&s, 40, 37, 3) == 3);
  assert(s.active && s.missing == 3);
  desr_art_prompt_cancel(&s);
  assert(!s.active && s.missing == 3);
  assert(desr_art_prompt_after_scan(&s, 40, 37, 3) == 3);
  assert(s.active && s.missing == 3);
  /* Successful sync, another scan now correctly reports no missing art. */
  desr_art_prompt_cancel(&s);
  assert(desr_art_prompt_after_scan(&s, 40, 40, 0) == 0);
  assert(!s.active && !s.missing && !s.report_error);

  /* A bad/missing/stale report must NEVER be displayed as 0 missing. */
  assert(desr_art_prompt_after_scan(&s, 40, 39, -1) < 0);
  assert(!s.active && s.report_error && s.missing == 1);
  assert(desr_art_prompt_after_scan(&s, 40, 39, 0) < 0);
  assert(!s.active && s.report_error && s.missing == 1);
  assert(desr_art_prompt_after_scan(&s, 40, 39, 2) < 0);
  assert(!s.active && s.report_error);
  /* After a USB/report error, a clean rescan can offer again. */
  assert(desr_art_prompt_after_scan(&s, 40, 39, 1) == 1);
  assert(s.active && s.missing == 1);
  desr_art_prompt_cancel(&s);
  assert(desr_art_prompt_after_scan(&s, 40, 39, 1) == 1);
  assert(s.active);

  /* Two distinct ISOs may map to one Game ID: dedup report is valid. */
  assert(desr_art_prompt_after_scan(&s, 40, 37, 2) == 2);
  assert(s.active && s.missing == 2);

  assert(desr_art_prompt_after_scan(&s, -1, -1, -1) < 0);
  assert(!s.active);
  puts("OK: scan screen state, O-cancel repeat scan and report mismatch");
  return 0;
}

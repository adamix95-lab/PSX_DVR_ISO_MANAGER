#include "art_prompt_state.h"

void desr_art_prompt_reset(desr_art_prompt_state *state) {
  if (!state) return;
  state->active = 0;
  state->missing = 0;
  state->report_error = 0;
}

void desr_art_prompt_cancel(desr_art_prompt_state *state) {
  if (!state) return;
  /* Dismissing a question is NOT accepting/rejecting the underlying
     missing-art scan. A later TRIANGLE scan can offer it again. */
  state->active = 0;
}

int desr_art_prompt_after_scan(desr_art_prompt_state *state,
                               int identified, int matched, int report_missing) {
  int missing;
  if (!state) return -1;
  state->active = 0;
  state->report_error = 0;
  state->missing = 0;
  if (identified < 0 || matched < 0 || matched > identified) {
    state->report_error = 1;
    return -1;
  }
  missing = identified - matched;
  /* A corrupted/failed COVER_SCAN.LOG must not masquerade as 0 missing.
     There may be more games than distinct IDs, hence report_missing may
     be less than missing. It must not exceed missing. */
  if (report_missing < 0 || report_missing > missing ||
      (missing > 0 && report_missing == 0)) {
    state->missing = missing;
    state->report_error = 1;
    return -1;
  }
  state->missing = report_missing;
  state->active = report_missing > 0;
  return report_missing;
}

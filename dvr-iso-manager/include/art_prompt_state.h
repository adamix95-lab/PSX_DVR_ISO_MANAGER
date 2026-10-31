#ifndef DESR_ART_PROMPT_STATE_H
#define DESR_ART_PROMPT_STATE_H

/* No PS2SDK dependencies. Input decisions and scan results remain testable
   on the host independently of the hardware gamepad and USB driver. */
typedef struct {
  int active;         /* X downloads, O cancels while active */
  int missing;        /* last validated number of distinct missing IDs */
  int report_error;   /* known missing art but USB report invalid/inconsistent */
} desr_art_prompt_state;

void desr_art_prompt_reset(desr_art_prompt_state *state);
void desr_art_prompt_cancel(desr_art_prompt_state *state);

/* Returns >=0 number of missing IDs to offer for LAN download.
   Negative: scan index error, missing/corrupt report, or contradiction.
   Exactly zero: no missing art. Handles repeated scans after O cancel. */
int desr_art_prompt_after_scan(desr_art_prompt_state *state,
                               int identified, int matched, int report_missing);

#endif

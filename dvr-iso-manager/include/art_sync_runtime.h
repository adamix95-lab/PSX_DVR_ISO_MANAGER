#ifndef DESR_ART_SYNC_RUNTIME_H
#define DESR_ART_SYNC_RUNTIME_H

#include "probe_config.h"
#include <stddef.h>

/* Read the Manager's USB scan report and select only missing covers.
   Returns -1 on invalid/missing log, 0 if nothing is missing, or a
   positive count of missing Game IDs. */
int art_sync_prepare(void);

/* Requires NETMAN/SMAP/PS2IP initialized by Manager.
   Downloads up to ART_SYNC_BATCH_MAX missing covers from local LAN.
   Returns count saved, or -1 if none was saved. */
int art_sync_run(const probe_config *cfg);

/* Last batch counters for the menu status, not only transient console text. */
void art_sync_last_stats(int *candidates, int *attempted, int *not_found,
                         int *http_failed, int *write_failed);

/* HDD test: errors split into invalid HTTP body/PNG, TCP/socket and
   non-200/non-404 HTTP statuses. Read-only status reporting. */
void art_sync_last_net_details(int *bad_payload, int *transport_fail,
                               int *http_status_fail);

/* HDD-only diagnostic: last rejected HTTP 200 payload.
   reason 1=SHORT, 2=EXTRA, 3=INVALID PNG with exact length,
          4=unparsed headers/content length. Never writes to disk. */
void art_sync_last_bad_detail(char *id, size_t cap, unsigned long *body,
                              unsigned long *expected, int *reason);

#endif

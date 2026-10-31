#ifndef DESR_ART_SYNC_RUNTIME_H
#define DESR_ART_SYNC_RUNTIME_H

#include "probe_config.h"

/* Load the USB diagnostic report and select only missing covers.
   Returns -1 on invalid/missing log, 0 if nothing is missing, positive
   total number of candidates (batch currently limited to 3). */
int art_sync_prepare(void);

/* Requires NETMAN/SMAP/PS2IP initialized as by v0.7/v0.7.1.
   Tries at most three unique missing covers from local LAN server.
   Returns count saved, or -1 if none was saved. */
int art_sync_run(const probe_config *cfg);

#endif

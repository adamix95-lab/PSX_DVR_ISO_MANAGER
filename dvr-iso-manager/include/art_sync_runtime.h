#ifndef DESR_ART_SYNC_RUNTIME_H
#define DESR_ART_SYNC_RUNTIME_H

#include "probe_config.h"

/* Read the Manager's USB scan report and select only missing covers.
   Returns -1 on invalid/missing log, 0 if nothing is missing, or a
   positive count of missing Game IDs. */
int art_sync_prepare(void);

/* Requires NETMAN/SMAP/PS2IP initialized by Manager.
   Downloads up to ART_SYNC_BATCH_MAX missing covers from local LAN.
   Returns count saved, or -1 if none was saved. */
int art_sync_run(const probe_config *cfg);

#endif

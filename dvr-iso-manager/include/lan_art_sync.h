#ifndef DESR_LAN_ART_SYNC_H
#define DESR_LAN_ART_SYNC_H

/* Count only NO_ART/NO_ART_EXE entries in the newly generated USB report.
   No network activity or USB writes. Returns -1 on missing/bad report. */
int desr_lan_art_missing(void);

/* Explicitly invoked ONLY by X while the missing-art prompt is visible.
   Lazy-loads NETMAN/SMAP, then uses the hardware-tested v0.7.2 ART_SYNC
   code. Returns >0 newly saved PNGs, -1 if none or network error.
   Never touches DVR partitions, BOOT.CNF or existing art. */
int desr_lan_art_download(void);

#ifdef DESR_HDD_ART_PROBE
#include "cover_index.h"
/* Never opens any PFS file for writing; validates images in EE RAM. */
int desr_lan_art_probe(const desr_cover_entry covers[], int count);
#endif

/* True after NETMAN/SMAP + PS2IP start, for read-only HDD diagnostics. */
int desr_lan_art_started(void);

#endif

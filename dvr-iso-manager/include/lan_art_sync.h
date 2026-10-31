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

#endif

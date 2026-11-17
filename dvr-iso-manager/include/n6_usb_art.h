#ifndef N6_USB_ART_H
#define N6_USB_ART_H
#include <stddef.h>
#include <stdint.h>

/* First physical N6 experiment: four covers max per power-on.
 * Each request uses TWO distinct 64-byte TLS seeds and verified time bounds.
 * All game discovery happens on dvr_pfs0:/ISO, which is read-only.
 * USB root ART is the ONLY writable destination, no auto-overwrite.
 */
#define N6G_BATCH_LIMIT 50
#if defined(N6G_BATCH_50)
#define N6_BATCH_MAX N6G_BATCH_LIMIT
#elif defined(N6C_SINGLE_COVER)
#define N6_BATCH_MAX 1
#else
#define N6_BATCH_MAX 4
#endif
#define N6_ENT_BYTES (32U + 4U * N6_BATCH_MAX * 64U)
#define N6_COVER_CAP (256U * 1024U)
#define N6_HTTP_CAP (N6_COVER_CAP + 8192U)

int n6_scan_dvr(char ids[N6_BATCH_MAX][12],
                int *iso_count,int *ids_count,int *missing_count);
int n6_usb_cover_present(const char id[12]);
int n6_write_usb_art(const char id[12],const uint8_t *png,size_t n);

#endif

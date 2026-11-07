#ifndef DESR_HDD_BOOT_CONFIG_H
#define DESR_HDD_BOOT_CONFIG_H

#include <stddef.h>

/* User-owned application partition. NEVER mount or modify Sony partitions. */
#define DESR_HDD_APP_PARTITION "hdd0:PP.DVRISO"
#define DESR_HDD_APP_MOUNT "pfs0:"
#define DESR_HDD_APP_ROOT "pfs0:/"

/* The existing DVR ISO store remains dvr_pfs0:/ISO/, unchanged. */

/* Construct OSDMenu Quickboot CNF for the future HDD Manager 1.0.
 * ISO name must pass desr_filename_is_safe(); ENGLISH uses the existing
 * neutrino-v02 OSD hook and separate EE_CORE.
 *
 * The final ignored #PAD line was hardware-proven to protect the
 * final -dev9=NICHDD argument from anomalous final-byte reads.
 * Returns exact byte count on success or -1 on invalid/unrepresentable data.
 * No filesystem operation occurs in this function.
 */
int desr_make_hdd_boot_cnf(const char *iso_filename,
                           int force_english, char *out, size_t out_cap);

#endif

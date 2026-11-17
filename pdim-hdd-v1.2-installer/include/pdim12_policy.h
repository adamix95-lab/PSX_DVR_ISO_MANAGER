#ifndef PDIM_HDD12_POLICY_H
#define PDIM_HDD12_POLICY_H

#include <stddef.h>
#include <stdint.h>

#define PDIM12_PARTITION "PP.DVRISO"
#define PDIM12_DEVICE "hdd0:PP.DVRISO"
#define PDIM12_MOUNT "pfs1:"
#define PDIM12_SOURCE "mass0:/PDIM_HDD_V12_INSTALLER/"
#define PDIM12_APP_SOURCE PDIM12_SOURCE "FILES/"
#define PDIM12_MANIFEST PDIM12_SOURCE "MANIFEST.TXT"
#define PDIM12_XMB_SOURCE PDIM12_SOURCE "XMB/"
#define PDIM12_MAX_FILES 160
#define PDIM12_REL_LIMIT 112
#define PDIM12_LBA28_LIMIT 0x10000000ULL
#define PDIM12_SECTORS 0x00080000U /* 256 MiB */
#define PDIM12_MAX_FILE (16U*1024U*1024U)

int pdim12_path_allowed(const char *path);
int pdim12_is_mutable_user_data(const char *path);
int pdim12_partition_name_allowed(const char *name);
int pdim12_address_safe(uint32_t start_sector, uint32_t sectors);
uint32_t pdim12_crc32_update(uint32_t crc, const void *data, size_t size);
static inline uint32_t pdim12_crc32_end(uint32_t crc) { return crc ^ UINT32_MAX; }

#endif

#include "pdim12_policy.h"
#include <string.h>

int pdim12_partition_name_allowed(const char *name) {
    return name && strcmp(name, PDIM12_PARTITION) == 0;
}
int pdim12_address_safe(uint32_t start, uint32_t sectors) {
    /* Reuse an already existing 128 MiB..2 GiB PFS without changing its
     * size. The creator still allocates exactly 256 MiB if absent. */
    return sectors >= 0x00040000u && sectors <= 0x00400000u &&
      (sectors & (sectors-1u))==0 && start != 0 &&
      ((uint64_t)start + (uint64_t)sectors) <= PDIM12_LBA28_LIMIT;
}
int pdim12_is_mutable_user_data(const char *path) {
    if (!path) return 1;
    static const char *const DENY[] = {
        "BOOT.CNF", "BOOT.NEXT.CNF", "BOOT.PREV.CNF",
        "BOOT_RECOVERY.CNF", "COVERS.IDX", "COVERS.NEW",
        "COVERS.BAK", "COVER_SCAN.LOG", "DVR_TEST.CNF",
        "V12_NET.CNF", "PDIM12.INSTALLED",
    };
    for (unsigned i=0; i<sizeof(DENY)/sizeof(DENY[0]); ++i)
        if (strcmp(path, DENY[i]) == 0) return 1;
    if (strncmp(path,"ART/",4)==0 || strcmp(path,"ART")==0) return 1;
    return 0;
}
int pdim12_path_allowed(const char *path) {
    if (!path) return 0;
    size_t len = strlen(path);
    if (!len || len >= PDIM12_REL_LIMIT || path[0]=='/' ||
        path[0]=='.' || path[len-1]=='/' || strstr(path, "..") ||
        pdim12_is_mutable_user_data(path)) return 0;
    unsigned component = 0;
    for (size_t i=0; i<len; ++i) {
        unsigned char c=(unsigned char)path[i];
        if (c=='/') {
            if (!component) return 0;
            component=0;
        } else if ((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
                   (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.') {
            component++;
        } else return 0;
    }
    return component>0;
}
uint32_t pdim12_crc32_update(uint32_t crc, const void *data, size_t size) {
    const unsigned char *bytes=(const unsigned char *)data;
    for (size_t i=0;i<size;i++){
        crc ^= bytes[i];
        for (int bit=0;bit<8;bit++)
            crc=(crc>>1) ^ ((crc&1u)?0xEDB88320u:0u);
    }
    return crc;
}

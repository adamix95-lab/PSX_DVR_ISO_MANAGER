#include "ppaa_plan.h"
#include <string.h>

#define MAGIC "PS2ICON3D"
#define MAGIC_LEN 9u
#define SYSTEM_DESC 0x10u
#define ICON_DESC 0x18u
#define KELF_DESC 0x30u
#define SYSTEM_OFFSET 0x200u
#define ICON_OFFSET 0x400u

static uint32_t le32(const uint8_t *p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void store32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static int is_zero(const uint8_t *p, size_t n) {
    for(size_t i=0;i<n;i++) if(p[i]) return 0;
    return 1;
}
int pdim12_ppaa_plan(const uint8_t current[3][PDIM12_PPAA_SECTOR_BYTES],
                    const uint8_t *sys, size_t sys_len,
                    const uint8_t *icon, size_t icon_len,
                    uint8_t proposed[3][PDIM12_PPAA_SECTOR_BYTES]) {
    if(!current || !sys || !icon || !proposed ||
       !sys_len || !icon_len ||
       sys_len>PDIM12_PPAA_SECTOR_BYTES ||
       icon_len>PDIM12_PPAA_SECTOR_BYTES)
        return -1;
    /* Recognize only an exact, previously installed channel.
     * If a magic exists but files/descriptors differ, do not hijack it.
     */
    if(!memcmp(current[0],MAGIC,MAGIC_LEN)) {
        if(le32(current[0]+SYSTEM_DESC)!=SYSTEM_OFFSET ||
           le32(current[0]+SYSTEM_DESC+4)!=sys_len ||
           le32(current[0]+ICON_DESC)!=ICON_OFFSET ||
           le32(current[0]+ICON_DESC+4)!=icon_len ||
           !is_zero(current[0]+0x20u,0x20u) ||
           memcmp(current[1],sys,sys_len))
            return -2;
        memcpy(proposed,current,3u*PDIM12_PPAA_SECTOR_BYTES);
        if(!memcmp(current[2],icon,icon_len))
            return 1; /* already installed 1.2 */
        /* Allow precisely the known 1.0 -> 1.2 version-label change,
         * NOT a generic rewrite of any foreign PS2ICON3D header. The
         * title1 suffix has an identical length, so descriptors remain
         * valid and unrelated bytes are retained. */
        static const char new_title[]="title1=HDD 1.2";
        static const char old_title[]="title1=HDD 1.0";
        uint8_t legacy[PDIM12_PPAA_SECTOR_BYTES];
        memcpy(legacy,icon,icon_len);
        int found=0;
        for(size_t i=0;i+sizeof(new_title)-1u<=icon_len;i++) {
            if(!memcmp(legacy+i,new_title,sizeof(new_title)-1u)) {
                memcpy(legacy+i,old_title,sizeof(old_title)-1u);
                found++;
            }
        }
        if(found==1 && !memcmp(current[2],legacy,icon_len)) {
            memset(proposed[2],0,PDIM12_PPAA_SECTOR_BYTES);
            memcpy(proposed[2],icon,icon_len);
            return 2; /* safe, exact legacy-header upgrade */
        }
        return -2;
    }
    /* wLE-style read-modify-write: preserve unrelated sector bytes.
     * But never overwrite an unknown magic or populated PPAA descriptor.
     */
    if(!is_zero(current[0],MAGIC_LEN) ||
       !is_zero(current[0]+SYSTEM_DESC,KELF_DESC+0x10u-SYSTEM_DESC))
        return -2;
    memcpy(proposed,current,3u*PDIM12_PPAA_SECTOR_BYTES);
    memcpy(proposed[0],MAGIC,MAGIC_LEN);
    store32(proposed[0]+SYSTEM_DESC,SYSTEM_OFFSET);
    store32(proposed[0]+SYSTEM_DESC+4,(uint32_t)sys_len);
    store32(proposed[0]+ICON_DESC,ICON_OFFSET);
    store32(proposed[0]+ICON_DESC+4,(uint32_t)icon_len);
    /* Only the CNF and icon.sys text sectors are replaced. */
    memset(proposed[1],0,PDIM12_PPAA_SECTOR_BYTES);
    memcpy(proposed[1],sys,sys_len);
    memset(proposed[2],0,PDIM12_PPAA_SECTOR_BYTES);
    memcpy(proposed[2],icon,icon_len);
    return 0;
}

#ifndef SG_INSTALLER_H
#define SG_INSTALLER_H
#include <stddef.h>
#include <stdint.h>
#define SG_COUNT 128
#define SG_NAME 128
#define SG_PART 32
#define SG_ID 12
#define SG_SOURCE_PART "hdd0:PP.DVRISO"
#define SG_SOURCE_MOUNT "pfs1:"
#define SG_DEST_MOUNT "pfs0:"
#define SG_SECTORS 0x00040000U
#define SG_LBA28_LIMIT 0x10000000ULL
typedef struct {
  char filename[SG_NAME];
  char game_id[SG_ID];
  char partition[SG_PART];
  int cover_available;
} sg_game;
extern sg_game sg_games[SG_COUNT];
extern int sg_count;
int sg_scan(void);
/* Best-effort live ISO Game ID probe.
   1: verified against COVERS.IDX
   0: ISO accessible but Game ID not detectable (trusted-index fallback)
  -1: I/O failure / unreadable image (block writes)
  -2: detected DIFFERENT Game ID (block writes) */
enum {
  SG_ISO_ID_MISMATCH = -2,
  SG_ISO_ID_IO_ERROR = -1,
  SG_ISO_ID_INDEX_ONLY = 0,
  SG_ISO_ID_MATCH = 1
};
int sg_check_iso_id(const sg_game *g);
int sg_png_validate_source(const char *source);
int sg_preflight(const sg_game *g,int lang);
int sg_install_core(const sg_game *g,int lang);
int sg_register_xmb(const sg_game *g,int lang);
int sg_probe(const sg_game *g,uint32_t *start,uint32_t *sectors);
int sg_validate_payload(const sg_game *g,int lang,uint32_t *start,uint32_t *sectors);
void sg_build_boot(const sg_game *g,int lang,char *out,size_t cap);
void sg_build_execute(const sg_game *g,char *out,size_t cap);
void sg_build_info(const sg_game *g,char *out,size_t cap);
void sg_build_sys(char *out,size_t cap);
void sg_build_icon(const sg_game *g,char *out,size_t cap);
uint32_t sg_crc_bytes(const void *src,int n);
int sg_crc_file(const char *p,uint32_t *sz,uint32_t *hash);
const char *sg_error(int code);
#endif

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
int sg_preflight(const sg_game *g,int lang);
int sg_install_core(const sg_game *g,int lang);
int sg_register_xmb(const sg_game *g,int lang);
const char *sg_error(int code);
#endif

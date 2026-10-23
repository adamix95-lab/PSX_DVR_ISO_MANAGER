#ifndef DESR_ISO_UTILS_H
#define DESR_ISO_UTILS_H
#include <stddef.h>

#define DESR_GAME_CAP 128
#define DESR_PAGE_SIZE 12
#define DESR_FILENAME_CAP 128
#define DESR_ISO_ROOT "dvr_pfs0:/ISO/"

/* Portable helpers: no PS2 hardware or SDK dependency. */
/* Move one 12-game page; keep the row when possible. No wrapping. */
int desr_page_move(int selected, int count, int direction);
int desr_is_iso_filename(const char *name);
int desr_filename_is_safe(const char *name);
int desr_make_iso_path(const char *filename, char *out, size_t out_size);
int desr_make_boot_cnf(const char *filename, char *out, size_t out_size);
/* force_english=1: use separate neutrino_lang.elf + osdlang config/core. */
int desr_make_boot_cnf_lang(const char *filename, int force_english,
                            char *out, size_t out_size);
#endif

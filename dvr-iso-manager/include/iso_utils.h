#ifndef DESR_ISO_UTILS_H
#define DESR_ISO_UTILS_H
#include <stddef.h>

#define DESR_GAME_CAP 128
#define DESR_FILENAME_CAP 128
#define DESR_ISO_ROOT "dvr_pfs0:/ISO/"

/* Portable helpers: no PS2 hardware or SDK dependency. */
int desr_is_iso_filename(const char *name);
int desr_filename_is_safe(const char *name);
int desr_make_iso_path(const char *filename, char *out, size_t out_size);
int desr_make_boot_cnf(const char *filename, char *out, size_t out_size);
/* force_english=1: use separate neutrino_lang.elf + osdlang config/core. */
int desr_make_boot_cnf_lang(const char *filename, int force_english,
                            char *out, size_t out_size);
#endif

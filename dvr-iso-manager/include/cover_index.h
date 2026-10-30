#ifndef DESR_COVER_INDEX_H
#define DESR_COVER_INDEX_H

#include <stddef.h>
#include <stdint.h>
#include "iso_utils.h"

#define DESR_COVER_ID_CAP 12
#define DESR_COVER_EXT_CAP 4
#define DESR_COVER_INDEX_CAP 24576
#define DESR_COVER_INDEX_HEADER "PSXCOV1\n"

/* One entry per ISO; present is 0 when no matching cover is found. */
typedef struct {
  char game_id[DESR_COVER_ID_CAP];
  char ext[DESR_COVER_EXT_CAP];
  int present;
} desr_cover_entry;

/* Portable, bounds-checked ISO9660 helpers. No hardware or disk writes. */
int desr_iso_root(const unsigned char *pvd, size_t n,
                  uint32_t *extent, uint32_t *bytes);
int desr_iso_system_record(const unsigned char *block, size_t n,
                           uint32_t *extent, uint32_t *bytes);
int desr_cnf_game_id(const char *cnf, size_t n,
                     char out[DESR_COVER_ID_CAP]);
int desr_valid_game_id(const char *id);
int desr_cover_format(char *out, size_t cap, const char *filename,
                      const desr_cover_entry *cover);
int desr_cover_parse(char *line, char filename[DESR_FILENAME_CAP],
                     desr_cover_entry *cover);

#endif

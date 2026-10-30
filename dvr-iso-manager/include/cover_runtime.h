#ifndef DESR_COVER_RUNTIME_H
#define DESR_COVER_RUNTIME_H

#include "cover_index.h"

/* Indexed covers are optional: failure never blocks selecting/launching ISO.
   All index and cover files live on mass0:/DESR_DVR, never on DVR. */
void desr_covers_clear(desr_cover_entry covers[DESR_GAME_CAP]);
int desr_covers_load(char games[][DESR_FILENAME_CAP], int count,
                     desr_cover_entry covers[DESR_GAME_CAP]);
/* Returns found count; -1 on ISO read failure or unsafe index update.
   Existing index remains available if an update cannot be committed. */
int desr_covers_refresh(char games[][DESR_FILENAME_CAP], int count,
                        desr_cover_entry covers[DESR_GAME_CAP],
                        int *identified);
#endif

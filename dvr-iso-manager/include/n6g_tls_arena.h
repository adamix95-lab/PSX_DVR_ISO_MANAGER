#ifndef N6G_TLS_ARENA_H
#define N6G_TLS_ARENA_H

#include <stddef.h>

typedef struct {
    size_t used;
    unsigned live;
    unsigned exhausted;
    unsigned invalid;
} n6g_tls_arena_stats;

int n6g_tls_arena_begin(void);
int n6g_tls_arena_end(n6g_tls_arena_stats *stats);

#endif

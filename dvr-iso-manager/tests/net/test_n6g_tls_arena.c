#include "n6g_tls_arena.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/memory.h>

static wolfSSL_Malloc_cb allocate;
static wolfSSL_Free_cb release;
static wolfSSL_Realloc_cb resize;

int wolfSSL_SetAllocators(wolfSSL_Malloc_cb m, wolfSSL_Free_cb f,
                         wolfSSL_Realloc_cb r) {
    allocate = m;
    release = f;
    resize = r;
    return 0;
}

int main(void) {
    n6g_tls_arena_stats stats;
    uint8_t *old, *moved;
    assert(n6g_tls_arena_begin() == 0);
    assert(n6g_tls_arena_begin() != 0);
    old = allocate(96);
    assert(old && (uintptr_t)old % 64U == 0);
    memset(old, 0xa5, 96);
    release(old + 64);
    assert(old[64] == 0xa5);
    assert(resize(old, SIZE_MAX) == NULL && old[0] == 0xa5);
    moved = resize(old, 100);
    assert(moved && moved != old && moved[95] == 0xa5 && old[95] == 0);
    release(old);
    assert(resize(moved, 32) == moved && moved[31] == 0xa5 && moved[32] == 0);
    assert(resize(moved, 0) == NULL && moved[31] == 0);
    release(NULL);
    assert(n6g_tls_arena_end(&stats) != 0);
    assert(stats.live == 0 && stats.exhausted == 1 && stats.invalid == 2);
    assert(!allocate && !release && !resize);

    assert(n6g_tls_arena_begin() == 0);
    old = resize(NULL, 0);
    assert(old);
    assert(n6g_tls_arena_end(&stats) != 0 && stats.live == 1);
    release(old);
    assert(n6g_tls_arena_end(&stats) == 0 && stats.live == 0);
    assert(!allocate && !release && !resize);
    puts("PASS N6G TLS arena alignment, realloc, quarantine, exhaustion and invalid free");
    return 0;
}

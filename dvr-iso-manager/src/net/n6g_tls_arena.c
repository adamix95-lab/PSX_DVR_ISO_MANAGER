#include "n6g_tls_arena.h"
#include "v12_n4a_entropy.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/memory.h>

#define ARENA_CAPACITY (8U * 1024U * 1024U)
#define ARENA_ALIGNMENT 64U
#define ARENA_MAP_BYTES (ARENA_CAPACITY / ARENA_ALIGNMENT / 8U)

typedef struct {
    size_t size;
    uintptr_t cookie;
} arena_block;

_Static_assert(sizeof(arena_block) <= ARENA_ALIGNMENT, "arena header fits");

static void *raw_arena;
static uint8_t *arena;
static uint8_t *allocated_starts;
static n6g_tls_arena_stats counts;

static uintptr_t block_cookie(const arena_block *block) {
    return (uintptr_t)block ^ UINT32_C(0x2c197ab5);
}

static arena_block *find_block(void *pointer) {
    uintptr_t base = (uintptr_t)arena;
    uintptr_t address = (uintptr_t)pointer;
    size_t offset;
    if (arena && address >= base) {
        offset = address - base;
        if (offset >= ARENA_ALIGNMENT && offset < counts.used &&
            offset % ARENA_ALIGNMENT == 0) {
            size_t slot = offset / ARENA_ALIGNMENT;
            if (allocated_starts[slot / 8U] & (1U << (slot % 8U))) {
                arena_block *block = (arena_block *)(arena + offset - ARENA_ALIGNMENT);
                if (block->cookie == block_cookie(block) &&
                    block->size <= counts.used - offset)
                    return block;
            }
        }
    }
    counts.invalid++;
    return NULL;
}

/* TLS calls are serialized on one EE thread. Freed slots are wiped and
 * quarantined until both sessions and wolfSSL_Cleanup have finished. */
static void *arena_alloc(size_t size) {
    size_t space = ARENA_CAPACITY - counts.used;
    size_t aligned, slot;
    arena_block *block;
    if (size == 0)
        size = 1;
    if (!arena || space < ARENA_ALIGNMENT || size > space - ARENA_ALIGNMENT) {
        counts.exhausted++;
        return NULL;
    }
    aligned = (size + ARENA_ALIGNMENT - 1U) & ~(size_t)(ARENA_ALIGNMENT - 1U);
    block = (arena_block *)(arena + counts.used);
    block->size = size;
    block->cookie = block_cookie(block);
    slot = counts.used / ARENA_ALIGNMENT + 1U;
    allocated_starts[slot / 8U] |= 1U << (slot % 8U);
    counts.used += ARENA_ALIGNMENT + aligned;
    counts.live++;
    return (uint8_t *)block + ARENA_ALIGNMENT;
}

static void arena_free(void *pointer) {
    if (pointer) {
        arena_block *block = find_block(pointer);
        if (block) {
            size_t slot = ((uint8_t *)pointer - arena) / ARENA_ALIGNMENT;
            v12_n4a_wipe(pointer, block->size);
            block->cookie = 0;
            allocated_starts[slot / 8U] &= ~(1U << (slot % 8U));
            counts.live--;
        }
    }
}

static void *arena_resize(void *pointer, size_t size) {
    arena_block *block;
    void *next;
    if (!pointer)
        return arena_alloc(size);
    if (size == 0) {
        arena_free(pointer);
        return NULL;
    }
    block = find_block(pointer);
    if (!block)
        return NULL;
    if (size <= block->size) {
        v12_n4a_wipe((uint8_t *)pointer + size, block->size - size);
        block->size = size;
        return pointer;
    }
    next = arena_alloc(size);
    if (!next)
        return NULL;
    memcpy(next, pointer, block->size);
    arena_free(pointer);
    return next;
}

int n6g_tls_arena_begin(void) {
    if (raw_arena)
        return -1;
    raw_arena = malloc(ARENA_CAPACITY + ARENA_MAP_BYTES + ARENA_ALIGNMENT - 1U);
    if (!raw_arena)
        return -1;
    arena = (uint8_t *)(((uintptr_t)raw_arena + ARENA_ALIGNMENT - 1U) &
                       ~(uintptr_t)(ARENA_ALIGNMENT - 1U));
    allocated_starts = arena + ARENA_CAPACITY;
    memset(allocated_starts, 0, ARENA_MAP_BYTES);
    memset(&counts, 0, sizeof(counts));
    if (wolfSSL_SetAllocators(arena_alloc, arena_free, arena_resize) != 0) {
        free(raw_arena);
        raw_arena = NULL;
        arena = NULL;
        return -1;
    }
    return 0;
}

int n6g_tls_arena_end(n6g_tls_arena_stats *stats) {
    *stats = counts;
    /* Keep storage alive if wolfSSL still owns blocks. The caller stops the
     * experiment in this case and performs no further TLS or USB write. */
    if (counts.live)
        return -1;
    if (wolfSSL_SetAllocators(NULL, NULL, NULL) != 0)
        return -1;
    v12_n4a_wipe(arena, counts.used);
    free(raw_arena);
    raw_arena = NULL;
    arena = NULL;
    return counts.exhausted || counts.invalid ? -1 : 0;
}

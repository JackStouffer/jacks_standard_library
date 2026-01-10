#pragma once

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"

// Stored immediately before every allocation so realloc can recover the length.
struct JSL__InfiniteArenaAllocationHeader
{
    int64_t length;
};

// TODO: docs
struct JSL__InfiniteArenaChunk
{
    struct JSL__InfiniteArenaChunk* next;
    struct JSL__InfiniteArenaChunk* prev;
    // the pointer generated from virtualalloc and mmap
    uint8_t* start;
    uint8_t* current;
    uint8_t* end;
};

/**
 * TODO: docs
 */
typedef struct JSLInfiniteArena
{
    struct JSL__InfiniteArenaChunk* head;
    struct JSL__InfiniteArenaChunk* tail;
    struct JSL__InfiniteArenaChunk* free_list;
} JSLInfiniteArena;

/**
 * TODO: docs
 */
JSL_DEF void jsl_infinite_arena_init(JSLInfiniteArena* arena);

/**
 * TODO: docs
 */
JSL_DEF JSLAllocatorInterface jsl_infinite_arena_get_allocator_interface(
    JSLInfiniteArena* arena
);

/**
 * TODO: docs
 */
JSL_DEF void* jsl_infinite_arena_allocate(
    JSLInfiniteArena* arena,
    int64_t bytes,
    bool zeroed
);

/**
 * TODO: docs
 */
JSL_DEF void* jsl_infinite_arena_allocate_aligned(
    JSLInfiniteArena* arena,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
);

/**
 * TODO: docs
 */
JSL_DEF void* jsl_infinite_arena_reallocate(
    JSLInfiniteArena* arena,
    void* original_allocation,
    int64_t new_num_bytes
);

/**
 * TODO: docs
 */
JSL_DEF void* jsl_infinite_arena_reallocate_aligned(
    JSLInfiniteArena* arena,
    void* original_allocation,
    int64_t new_num_bytes,
    int32_t align
);

/**
 * TODO: docs
 */
JSL_DEF void jsl_infinite_arena_reset(JSLInfiniteArena* arena);

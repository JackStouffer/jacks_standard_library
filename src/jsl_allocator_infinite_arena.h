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
 * A bump allocator with a (conceptually) infinite amount of memory. Memory is pulled
 * from the OS using `VirualAlloc`/`mmap` when ever it's needed with no limits.
 * 
 * This allocator is useful for two main situations. One programs that really don't
 * need to care about memory at all, like batch scripts and tooling. It's perfectly
 * legitimate to ask for a new piece of memory everytime you need something and never
 * free. You're not going to exhaust the memory one your machine when writing tooling
 * to process a small text file, for example. Two, this allocator 
 *
 * See the DESIGN.md file for detailed notes on arena implementation, their uses,
 * and when they shouldn't be used.
 *
 * Functions and Macros:
 *
 * * jsl_arena_init
 * * jsl_arena_init2
 * * jsl_arena_allocate
 * * jsl_arena_allocate_aligned
 * * jsl_arena_reallocate
 * * jsl_arena_reallocate_aligned
 * * jsl_arena_reset
 * * jsl_arena_save_restore_point
 * * jsl_arena_load_restore_point
 * * JSL_ARENA_TYPED_ALLOCATE
 *
 * @note The arena API is not thread safe. Arena memory is assumed to live in a
 * single thread. If you want to share an arena between threads you need to lock.
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

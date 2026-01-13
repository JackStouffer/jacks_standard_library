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

// A single chunk of memory in the doubly linked list of arena chunks.
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
 * from the OS using `VirualAlloc`/`mmap` whenever it's needed with no limits.
 *
 * This allocator is useful for simple programs that can one, be a little sloppy with
 * memory and two, have a single memory lifetime for the whole program. A couple examples
 * of such programs would be batch scripts, developer tooling, and daemons. For these
 * types of programs it's perfectly legitimate to ask for a new piece of memory every time
 * you need something and never free until the program exits or the process starts over.
 * You're not going to exhaust the memory one your dev machine when writing tooling to
 * process a 30kb text file, for example.
 * 
 * This infinite arena is more useful than a conventional arena in these situations because
 * you don't want the program to fail if you suddenly need way more memory than you
 * anticipated. In contrast, a desktop GUI program needs to be way more careful about
 * how much memory is used per lifetime and the reset points of those lifetimes. For
 * such a program, it would be a bad idea to use an infinite arena since you want to
 * have constraints as soon as possible in the development cycle to make sure that your
 * program can run performantly on the minimum tech specs you plan on supporting. 
 *
 * Functions and Macros:
 *
 * * jsl_infinite_arena_init
 * * jsl_infinite_arena_allocate
 * * jsl_infinite_arena_allocate_aligned
 * * jsl_infinite_arena_reallocate
 * * jsl_infinite_arena_reallocate_aligned
 * * jsl_infinite_arena_reset
 * * JSL_INFINITE_ARENA_TYPED_ALLOCATE
 *
 * @note This API is not thread safe. Arena memory is assumed to live in a
 * single thread. If you want to share an arena between threads you need to lock.
 */
typedef struct JSLInfiniteArena
{
    struct JSL__InfiniteArenaChunk* head;
    // the tail is the active chunk
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

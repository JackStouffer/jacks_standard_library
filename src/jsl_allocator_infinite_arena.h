#pragma once

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"

#if !JSL_IS_WINDOWS && !JSL_IS_POSIX

    #error "jsl_allocator_infinite_arena.h: Unsupported OS detected. The infinite arena is for POSIX and Windows systems only."

#endif


// Stored immediately before every allocation so realloc can recover the length.
struct JSL__InfiniteArenaAllocationHeader
{
    int64_t length;
};

struct JSL__InfiniteArena
{
    uint64_t sentinel;

    uint8_t* start;
    uint8_t* current;
    uint8_t* end;

    #if JSL_IS_WINDOWS
        int64_t committed_bytes;
    #endif
};

/**
 * A bump allocator with a (conceptually) infinite amount of memory. Memory is pulled
 * from the OS using `VirtualAlloc`/`mmap` with no limits.
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
 * program can run performantly on the minimum tech specs you plan on supporting. You
 * should develop such a program with mechanisms to break work up so any problem size
 * fits in the memory limits so set.
 * 
 * ## Functions and Macros
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
typedef struct JSL__InfiniteArena JSLInfiniteArena;

/**
 * Initialize an infinite arena to an empty state. This function
 * does not allocate, as allocations are on demand, so this function
 * cannot fail.
 *
 * @param arena The arena to initialize.
 */
JSL_DEF bool jsl_infinite_arena_init(JSLInfiniteArena* arena);

/**
 * Create a `JSLAllocatorInterface` that routes allocations to the arena.
 *
 * The returned interface is valid as long as `arena` remains alive.
 *
 * @param arena The arena used for all allocator callbacks.
 * @return Allocator interface that uses the infinite arena for allocate/reallocate/free.
 */
JSL_DEF void jsl_infinite_arena_get_allocator_interface(JSLAllocatorInterface* allocator, JSLInfiniteArena* arena);

/**
 * Allocate a block of memory from the arena using the default alignment.
 *
 * NULL is returned if `VirualAlloc`/`mmap` fail. When
 * `zeroed` is true, the allocated bytes are zero-initialized.
 *
 * @param arena Arena to allocate from; must not be null.
 * @param bytes Number of bytes to reserve.
 * @param zeroed When true, zero-initialize the allocation.
 * @return Fat pointer describing the allocation or `{0}` on failure.
 */
JSL_DEF void* jsl_infinite_arena_allocate(
    JSLInfiniteArena* arena,
    int64_t bytes,
    bool zeroed
);

/**
 * Allocate a block of memory from the arena with the provided alignment.
 *
 * `NULL` is returned if `VirualAlloc`/`mmap` fail. When
 * `zeroed` is true, the allocated bytes are zero-initialized.
 *
 * @param arena Arena to allocate from; must not be null.
 * @param bytes Number of bytes to reserve.
 * @param alignment Desired alignment in bytes; must be a positive power of two.
 * @param zeroed When true, zero-initialize the allocation.
 * @return Fat pointer describing the allocation or `{0}` on failure.
 */
JSL_DEF void* jsl_infinite_arena_allocate_aligned(
    JSLInfiniteArena* arena,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
);

/**
 * Macro to make it easier to allocate an instance of `T` within an arena.
 *
 * @param T Type to allocate.
 * @param arena Arena to allocate from; must be initialized.
 * @return Pointer to the allocated object or `NULL` on failure.
 *
 * ```
 * struct MyStruct { uint64_t the_data; };
 * struct MyStruct* thing = JSL_INFINITE_ARENA_TYPED_ALLOCATE(struct MyStruct, arena);
 * ```
 */
#define JSL_INFINITE_ARENA_TYPED_ALLOCATE(T, arena) (T*) jsl_infinite_arena_allocate_aligned(arena, sizeof(T), _Alignof(T), false)

/**
 * Resize the current allocation if 
 * 
 * 1. It was the last allocation
 * 2. The new size fits in the currently used range of reserved address space
 * 3. `original_allocation` has the default alignment 
 * 
 * Otherwise, allocate a new chunk of memory and copy the old allocation's contents.
 * 
 * If `original_allocation` is null then this is treated as a call to
 * `jsl_infinite_arena_allocate`.
 * 
 * If `new_num_bytes` is less than the size of the original allocation this is a
 * no-op. 
 */
JSL_DEF void* jsl_infinite_arena_reallocate(
    JSLInfiniteArena* arena,
    void* original_allocation,
    int64_t new_num_bytes
);

/**
 * Resize the current allocation if 
 * 
 * 1. It was the last allocation
 * 2. The new size fits in the currently used range of reserved address space
 * 3. `original_allocation` has `align` alignment 
 * 
 * Otherwise, allocate a new chunk of memory and copy the old allocation's contents.
 * 
 * If `original_allocation` is null then this is treated as a call to
 * `jsl_infinite_arena_allocate`.
 * 
 * If `new_num_bytes` is less than the size of the original allocation this is a
 * no-op. 
 */
JSL_DEF void* jsl_infinite_arena_reallocate_aligned(
    JSLInfiniteArena* arena,
    void* original_allocation,
    int64_t new_num_bytes,
    int32_t align
);

/**
 * Set the arena to have zero active memory regions. This does not return
 * the reserved virtual address ranges back to the OS. All memory is kept
 * in a free list for future use. If you wish to return the memory to the
 * OS you'll need to use `jsl_infinite_arena_release`.
 *
 * @param arena The arena to reset
 */
JSL_DEF void jsl_infinite_arena_reset(JSLInfiniteArena* arena);

/**
 * Release all of the virtual memory back to the OS. This invalidates
 * the infinite arena and it can not be reused in future operations
 * until init is called on it again.
 * 
 * Note that it is not necessary to call this function before your program
 * exits. All virtual memory is automatically freed by the OS when the process
 * ends. The OS actually does this much faster than you can, all the freeing
 * can be done in kernal space while this function has to be run in user space.
 * Manually freeing is a waste of your user's time.
 *
 * @param arena The arena to release the memory from
 */
JSL_DEF void jsl_infinite_arena_release(JSLInfiniteArena* arena);

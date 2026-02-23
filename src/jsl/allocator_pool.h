#pragma once

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "core.h"
#include "allocator.h"

// Stored immediately before every allocation so realloc can recover the length.
struct JSL__PoolAllocatorHeader
{
    uint64_t sentinel;
    struct JSL__PoolAllocatorHeader* next;
    struct JSL__PoolAllocatorHeader** prev_next;
    void* allocation;
};

struct JSL__PoolAllocator
{
    // putting the sentinel first means it's much more likely to get
    // corrupted from accidental overwrites, therefore making it
    // more likely that memory bugs are caught.
    uint64_t sentinel;

    // we need to keep track of the in use stuff so we can do "free all"
    struct JSL__PoolAllocatorHeader* checked_out;
    struct JSL__PoolAllocatorHeader* free_list;
    uintptr_t memory_start;
    uintptr_t memory_end;
    int64_t allocation_size;
    int64_t chunk_count;
};

/**
 * A pool allocator is a specialized allocator for allocating lots of things of the
 * same size (or with a well defined maximum). Since every allocation returned is
 * the same size, allocating and freeing are very fast. The entire allocator is just
 * one stack of used allocations and another stack of unused allocations.
 *
 * A pool allocator should not be confused with a connection pool. These are very
 * different tools. In fact, this allocator should not even be used for the backing
 * memory for such a connection pool, as the vast majority of connection pools can
 * be allocated statically.
 *
 * This allocator is best used in situations where you'll have thousands (or more)
 * of tiny objects or hundreds of large objects that are all the same type and can
 * have a relatively short lifetime. In these sorts of situations using a general
 * purpose allocator can result in heap fragmentation.
 *
 * Examples of situations where this allocator shines:
 * 
 *  * Games with hundreds of short lived entities
 *  * Very large, changing tree structures where each node carries some state
 *  * Many input buffers that have a max size, like in an HTTP server
 *    when you need request body buffers for each request in flight
 *  * Event queues with thousands of events in flight
 *
 * You should not use this allocator if
 * 
 *  * If you aren't both allocating and freeing within the same lifetime.
 *    i.e. if you're just using the free all function all the time then this
 *    allocator isn't giving you anything.
 *  * You cannot define a maximum for your allocation size
 *  * The sum of bytes of valid allocated objects at any given time is low 
 * 
 * Since this allocator is so specialized, this allocator does not provide
 * the standardized allocator interface in `jsl_allocator.h`. The main reason
 * being that the concept of a "realloc" from a pool is nonsensical.
 * 
 * Functions and Macros:
 *
 * * jsl_pool_init
 * * jsl_pool_init2
 * * jsl_pool_allocate
 * * jsl_pool_free
 * * jsl_pool_free_all
 * * jsl_pool_free_allocation_count
 * * jsl_pool_total_allocation_count
 *
 * @note The pool API is not thread safe. pool memory is assumed to live in a
 * single thread. If you want to share an pool between threads you need to lock
 * when calling these functions.
 */
typedef struct JSL__PoolAllocator JSLPoolAllocator;

/**
 * Initialize an pool with the supplied buffer.
 * 
 * The number of available allocations is not equal to
 * `memory length / allocation size`. This allocator needs more space for
 * bookkeeping, and each of the given allocation pointers given the user
 * is aligned depending on the size of the allocation chunk for performance
 * reasons. Larger allocation sizes are aligned to a 4kb page, for example. 
 *
 * @param pool pool instance to initialize; must not be null.
 * @param memory Pointer to the beginning of the backing storage.
 * @param length Size of the backing storage in bytes.
 */
JSL_DEF void jsl_pool_init(
    JSLPoolAllocator* pool,
    void* memory,
    int64_t length,
    int64_t allocation_size
);

/**
 * Initialize an pool with the supplied buffer.
 * 
 * The number of available chunks is not equal to `memory length / allocation_size`.
 * This allocator needs more space for bookkeeping and each of the given
 * allocation pointers given the user is aligned depending on the size of
 * the allocation chunk for performance reasons. Larger chunk sizes
 * are aligned to a 4kb page, for example. 
 *
 * @param pool pool instance to initialize; must not be null.
 * @param memory Pointer to the beginning of the backing storage.
 * @param length Size of the backing storage in bytes.
 */
JSL_DEF void jsl_pool_init2(
    JSLPoolAllocator* pool,
    JSLImmutableMemory memory,
    int64_t allocation_size
);

/**
 * Get the number of available allocations that the pool has to give out
 * in calls to `jsl_pool_allocate`.
 *
 * @return Allocation count
 */
int64_t jsl_pool_free_allocation_count(
    JSLPoolAllocator* pool
);

/**
 * Get the total number of possible allocations that the pool can give out
 * in calls to `jsl_pool_allocate` regardless of how many allocations are
 * currently outstanding.
 *
 * @return Allocation count
 */
int64_t jsl_pool_total_allocation_count(
    JSLPoolAllocator* pool
);

/**
 * Grab an allocation from the pool.
 *
 * `NULL` is returned if the pool does not have any available allocations.
 * When `zeroed` is true, the allocated bytes are zero-initialized.
 *
 * @param pool pool to allocate from; must not be null.
 * @param zeroed When true, zero-initialize the allocation.
 * @return pointer to the allocation or `NULL` on failure.
 */
JSL_DEF void* jsl_pool_allocate(JSLPoolAllocator* pool, bool zeroed);

/**
 * Return an allocation to the pool. When the `JSL_DEBUG` preprocessor
 * macro is set then the pool will overwrite the memory with `0xfeefee`.
 *
 * @return Returns true if this allocation is owned by the pool and
 * was outstanding. Returns false if this allocation is not owned by the
 * pool or if the allocation was already freed.
 */
JSL_DEF bool jsl_pool_free(JSLPoolAllocator* pool, void* allocation);

/**
 * Set all outstanding allocations. When the `JSL_DEBUG` preprocessor
 * macro is set then the pool will overwrite memory with `0xfeefee`.
 */
JSL_DEF void jsl_pool_free_all(JSLPoolAllocator* pool);

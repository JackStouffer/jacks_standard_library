/**
 * A general-purpose allocator that wraps the C standard library's
 * malloc/realloc/free. This allocator tracks all outstanding allocations
 * in a doubly-linked list so that it can support `free_all` and
 * child allocators.
 *
 * This is useful when you need a general-purpose allocator that still
 * fits into the `JSLAllocatorInterface` abstraction. Unlike arenas,
 * individual allocations can be freed at any time.
 *
 * ## License
 *
 * Copyright (c) 2026 Jack Stouffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "core.h"
#include "allocator.h"

// Stored immediately before every allocation so that the allocator can
// track, realloc, and free individual allocations.
struct JSL__LibcAllocationHeader
{
    struct JSL__LibcAllocationHeader* prev;
    struct JSL__LibcAllocationHeader* next;
    void* malloc_ptr;
    int64_t length;
};

struct JSL__LibcAllocator
{
    uint64_t sentinel;
    struct JSL__LibcAllocationHeader* head;
};

/**
 * A general-purpose allocator backed by the C standard library's
 * malloc/realloc/free. All outstanding allocations are tracked so
 * that `free_all` and child allocators work correctly.
 *
 * ## Functions and Macros
 *
 * * jsl_libc_allocator_init
 * * jsl_libc_allocator_get_allocator_interface
 * * jsl_libc_allocator_allocate
 * * jsl_libc_allocator_allocate_aligned
 * * jsl_libc_allocator_reallocate
 * * jsl_libc_allocator_reallocate_aligned
 * * jsl_libc_allocator_free
 * * jsl_libc_allocator_free_all
 * * JSL_LIBC_ALLOCATOR_TYPED_ALLOCATE
 *
 * @note This API is not thread safe. If you want to share a libc
 * allocator between threads you need to lock.
 */
typedef struct JSL__LibcAllocator JSLLibcAllocator;

/**
 * Initialize a libc allocator to an empty state.
 *
 * @param allocator The allocator to initialize.
 */
JSL_DEF void jsl_libc_allocator_init(JSLLibcAllocator* allocator);

/**
 * Create a `JSLAllocatorInterface` that routes allocations through the
 * libc allocator.
 *
 * The returned interface is valid as long as `allocator` remains alive.
 *
 * @param interface Output allocator interface.
 * @param allocator The libc allocator used for all callbacks.
 */
JSL_DEF void jsl_libc_allocator_get_allocator_interface(
    JSLAllocatorInterface* interface,
    JSLLibcAllocator* allocator
);

/**
 * Allocate a block of memory using the default alignment.
 *
 * Returns `NULL` if malloc fails. When `zeroed` is true, the
 * allocated bytes are zero-initialized.
 *
 * @param allocator Allocator to allocate from; must be initialized.
 * @param bytes Number of bytes to allocate.
 * @param zeroed When true, zero-initialize the allocation.
 * @return Pointer to the allocated memory or `NULL` on failure.
 */
JSL_DEF void* jsl_libc_allocator_allocate(
    JSLLibcAllocator* allocator,
    int64_t bytes,
    bool zeroed
);

/**
 * Allocate a block of memory with the provided alignment.
 *
 * Returns `NULL` if malloc fails. When `zeroed` is true, the
 * allocated bytes are zero-initialized.
 *
 * @param allocator Allocator to allocate from; must be initialized.
 * @param bytes Number of bytes to allocate.
 * @param alignment Desired alignment in bytes; must be a positive power of two.
 * @param zeroed When true, zero-initialize the allocation.
 * @return Pointer to the allocated memory or `NULL` on failure.
 */
JSL_DEF void* jsl_libc_allocator_allocate_aligned(
    JSLLibcAllocator* allocator,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
);

/**
 * Macro to make it easier to allocate an instance of `T`.
 *
 * @param T Type to allocate.
 * @param allocator Libc allocator to allocate from; must be initialized.
 * @return Pointer to the allocated object or `NULL` on failure.
 *
 * ```
 * struct MyStruct { uint64_t the_data; };
 * struct MyStruct* thing = JSL_LIBC_ALLOCATOR_TYPED_ALLOCATE(struct MyStruct, allocator);
 * ```
 */
#define JSL_LIBC_ALLOCATOR_TYPED_ALLOCATE(T, allocator) \
    (T*) jsl_libc_allocator_allocate_aligned(allocator, sizeof(T), _Alignof(T), false)

/**
 * Reallocate a block with the default alignment.
 *
 * If `original_allocation` is `NULL`, this behaves like
 * `jsl_libc_allocator_allocate`.
 *
 * @param allocator Allocator that owns the allocation.
 * @param original_allocation Pointer previously returned by this allocator, or `NULL`.
 * @param new_bytes New size in bytes.
 * @return Pointer to the reallocated memory or `NULL` on failure.
 */
JSL_DEF void* jsl_libc_allocator_reallocate(
    JSLLibcAllocator* allocator,
    void* original_allocation,
    int64_t new_bytes
);

/**
 * Reallocate a block with the provided alignment.
 *
 * If `original_allocation` is `NULL`, this behaves like
 * `jsl_libc_allocator_allocate_aligned`.
 *
 * @param allocator Allocator that owns the allocation.
 * @param original_allocation Pointer previously returned by this allocator, or `NULL`.
 * @param new_bytes New size in bytes.
 * @param alignment Desired alignment in bytes; must be a positive power of two.
 * @return Pointer to the reallocated memory or `NULL` on failure.
 */
JSL_DEF void* jsl_libc_allocator_reallocate_aligned(
    JSLLibcAllocator* allocator,
    void* original_allocation,
    int64_t new_bytes,
    int32_t alignment
);

/**
 * Free a single allocation.
 *
 * @param allocator Allocator that owns the allocation.
 * @param allocation Pointer previously returned by this allocator.
 * @return `true` on success, `false` if the allocation was not found.
 */
JSL_DEF bool jsl_libc_allocator_free(JSLLibcAllocator* allocator, const void* allocation);

/**
 * Free every outstanding allocation made through this allocator.
 *
 * After this call the allocator is empty but still usable.
 *
 * @param allocator The allocator to drain.
 * @return `true` on success.
 */
JSL_DEF bool jsl_libc_allocator_free_all(JSLLibcAllocator* allocator);

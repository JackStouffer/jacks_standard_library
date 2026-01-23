#pragma once

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"

/**
 * TODO: docs
 * 
 * Assumptions:
 * - The returned memory is at least bytes long, can be more
 * - if size request by the user isn't available this returns null
 * - if the allocator is not ready/initialized this returns null
 */
typedef void* (*JSLAllocateFP)(void* ctx, int64_t bytes, int32_t alignment, bool zeroed);
/**
 * TODO: docs
 */
typedef void* (*JSLReallocateFP)(void* ctx, void* allocation, int64_t new_bytes, int32_t alignment);
/**
 * TODO: docs
 */
typedef bool (*JSLFreeFP)(void* ctx, void* allocation);
/**
 * TODO: docs
 */
typedef bool (*JSLFreeAllFP)(void* ctx);

/**
 * TODO: docs
 */
typedef struct JSLAllocatorInterface
{
    uint64_t sentinel;
    JSLAllocateFP allocate;
    JSLReallocateFP reallocate;
    JSLFreeFP free;
    JSLFreeAllFP free_all;
    void* context;
} JSLAllocatorInterface;

/**
 * TODO: docs
 */
void* jsl_align_ptr_upwards(void* ptr, int32_t align);

/**
 * TODO: docs
 */
uintptr_t jsl_align_ptr_upwards_uintptr(uintptr_t ptr, int32_t align);

/**
 * TODO: docs
 */
void jsl_allocator_interface_init(
    JSLAllocatorInterface* allocator,
    JSLAllocateFP allocate_fp,
    JSLReallocateFP reallocate_fp,
    JSLFreeFP free_fp,
    JSLFreeAllFP free_all_fp,
    void* context
);

/**
 * TODO: docs
 */
void* jsl_allocator_interface_alloc(
    JSLAllocatorInterface* allocator,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
);

/**
 * TODO: docs
 */
void* jsl_allocator_interface_realloc(
    JSLAllocatorInterface* allocator,
    void* allocation,
    int64_t new_bytes,
    int32_t alignment
);

/**
 * TODO: docs
 */
bool jsl_allocator_interface_free(
    JSLAllocatorInterface* allocator,
    void* allocation
);

/**
 * TODO: docs
 */
bool jsl_allocator_interface_free_all(
    JSLAllocatorInterface* allocator
);

/**
 * Macro to make it easier to allocate an instance of `T`.
 *
 * @param T Type to allocate.
 * @param allocator allocator.
 * @return Pointer to the allocated object or `NULL` on failure.
 *
 * ```
 * struct MyStruct { uint64_t the_data; };
 * struct MyStruct* thing = JSL_TYPED_ALLOCATE(struct MyStruct, arena);
 * ```
 */
#define JSL_TYPED_ALLOCATE(T, allocator) (T*) jsl_allocator_interface_alloc(allocator, sizeof(T), _Alignof(T), false)

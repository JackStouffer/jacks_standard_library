#pragma once

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"

/**
 * TODO: docs
 */
typedef JSLFatPtr (*JSLAllocateFP)(void* ctx, int64_t bytes, int32_t alignment, bool zeroed);
/**
 * TODO: docs
 */
typedef JSLFatPtr (*JSLReallocateFP)(void* ctx, JSLFatPtr allocation, int64_t new_bytes, int32_t alignment);
/**
 * TODO: docs
 */
typedef bool (*JSLFreeFP)(void* ctx, JSLFatPtr allocation);
/**
 * TODO: docs
 */
typedef bool (*JSLFreeAllFP)(void* ctx);

/**
 * TODO: docs
 */
typedef struct JSLAllocatorInterface
{
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
JSLFatPtr jsl_allocator_interface_alloc(
    JSLAllocatorInterface* allocator,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
);

/**
 * TODO: docs
 */
JSLFatPtr jsl_allocator_interface_realloc(
    JSLAllocatorInterface* allocator,
    JSLFatPtr allocation,
    int64_t new_bytes,
    int32_t alignment
);

/**
 * TODO: docs
 */
bool jsl_allocator_interface_free(
    JSLAllocatorInterface* allocator,
    JSLFatPtr allocation
);

/**
 * TODO: docs
 */
bool jsl_allocator_interface_free_all(
    JSLAllocatorInterface* allocator,
    JSLFatPtr allocation
);

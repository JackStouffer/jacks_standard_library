#pragma once

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"

#define JSL__ALLOCATOR_PRIVATE_SENTINEL 2954080723981509744U

void jsl_allocator_interface_init(
    JSLAllocatorInterface* allocator,
    JSLAllocateFP allocate_fp,
    JSLReallocateFP reallocate_fp,
    JSLFreeFP free_fp,
    JSLFreeAllFP free_all_fp,
    void* context
)
{
    if (allocator == NULL)
        return;

    allocator->sentinel = JSL__ALLOCATOR_PRIVATE_SENTINEL;
    allocator->allocate = allocate_fp;
    allocator->reallocate = reallocate_fp;
    allocator->free = free_fp;
    allocator->free_all = free_all_fp;
    allocator->context = context;
}

void* jsl_allocator_interface_alloc(
    JSLAllocatorInterface* allocator,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
)
{
    if (
        allocator == NULL
        || allocator->sentinel != JSL__ALLOCATOR_PRIVATE_SENTINEL
        || bytes < 1
        || alignment < 1
    )
        return NULL;

    return allocator->allocate(allocator->context, bytes, alignment, zeroed);
}

void* jsl_allocator_interface_realloc(
    JSLAllocatorInterface* allocator,
    void* allocation,
    int64_t new_bytes,
    int32_t alignment
)
{
    if (
        allocator == NULL
        || allocator->sentinel != JSL__ALLOCATOR_PRIVATE_SENTINEL
        || allocation == NULL
        || new_bytes < 1
        || alignment < 1
    )
        return NULL;

    return allocator->reallocate(allocator->context, allocation, new_bytes, alignment);
}

bool jsl_allocator_interface_free(
    JSLAllocatorInterface* allocator,
    void* allocation
)
{
    if (allocator == NULL || allocator->sentinel != JSL__ALLOCATOR_PRIVATE_SENTINEL || allocation == NULL)
        return NULL;

    return allocator->allocate(allocator->context, allocation);
}

bool jsl_allocator_interface_free_all(
    JSLAllocatorInterface* allocator
)
{
    if (allocator == NULL || allocator->sentinel != JSL__ALLOCATOR_PRIVATE_SENTINEL)
        return NULL;

    return allocator->allocate(allocator->context);
}

void* jsl_align_ptr_upwards(void* ptr, int32_t alignment)
{
    if (ptr == NULL || alignment < 1)
        return NULL;

    uintptr_t addr   = (uintptr_t) ptr;
    uintptr_t ualign = (uintptr_t) alignment;

    uintptr_t mask = ualign - 1;
    addr = (addr + mask) & ~mask;

    return (uint8_t*) addr;
}

#undef JSL__ALLOCATOR_PRIVATE_SENTINEL

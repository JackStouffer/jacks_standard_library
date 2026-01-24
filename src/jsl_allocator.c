/**
 * Copyright (c) 2026 Jack Stouffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
    if (allocator == NULL|| allocator->sentinel != JSL__ALLOCATOR_PRIVATE_SENTINEL)
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
    if (allocator == NULL|| allocator->sentinel != JSL__ALLOCATOR_PRIVATE_SENTINEL)
        return NULL;

    return allocator->reallocate(allocator->context, allocation, new_bytes, alignment);
}

bool jsl_allocator_interface_free(
    JSLAllocatorInterface* allocator,
    void* allocation
)
{
    if (allocator == NULL || allocator->sentinel != JSL__ALLOCATOR_PRIVATE_SENTINEL)
        return false;

    return allocator->free(allocator->context, allocation);
}

bool jsl_allocator_interface_free_all(
    JSLAllocatorInterface* allocator
)
{
    if (allocator == NULL || allocator->sentinel != JSL__ALLOCATOR_PRIVATE_SENTINEL)
        return false;

    return allocator->free_all(allocator->context);
}

void* jsl_align_ptr_upwards(void* ptr, int32_t alignment)
{
    if (ptr == NULL || alignment < 1)
        return NULL;

    uintptr_t addr   = (uintptr_t) ptr;
    const uintptr_t ualign = (uintptr_t) alignment;

    const uintptr_t mask = ualign - 1;
    addr = (addr + mask) & ~mask;

    return (void*) addr;
}

uintptr_t jsl_align_ptr_upwards_uintptr(uintptr_t ptr, int32_t alignment)
{
    if (ptr == 0 || alignment < 1)
        return 0;

    const uintptr_t ualign = (uintptr_t) alignment;
    const uintptr_t mask = ualign - 1;

    ptr = (ptr + mask) & ~mask;

    return ptr;
}

#undef JSL__ALLOCATOR_PRIVATE_SENTINEL

/**
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

#include "core.h"
#include "allocator.h"
#include "allocator_libc.h"

#include <stdlib.h>
#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#define JSL__LIBC_ALLOCATOR_PRIVATE_SENTINEL 4738291056482917365U

static JSL__FORCE_INLINE int32_t jsl__libc_effective_alignment(int32_t requested_alignment)
{
    int32_t header_alignment = (int32_t) _Alignof(struct JSL__LibcAllocationHeader);
    return requested_alignment > header_alignment ? requested_alignment : header_alignment;
}

#ifdef JSL_DEBUG

    static JSL__FORCE_INLINE void jsl__libc_debug_memset_old_memory(
        void* allocation,
        int64_t num_bytes
    )
    {
        int32_t* fake_array = (int32_t*) allocation;
        int64_t fake_array_len = num_bytes / (int64_t) sizeof(int32_t);
        for (int64_t i = 0; i < fake_array_len; ++i)
        {
            fake_array[i] = 0xfeefee;
        }

        int64_t trailing_bytes = num_bytes - (fake_array_len * (int64_t) sizeof(int32_t));
        if (trailing_bytes > 0)
        {
            const uint32_t pattern = 0x00feefee;
            const uint8_t* pattern_bytes = (const uint8_t*) &pattern;
            uint8_t* trailing = (uint8_t*) (fake_array + fake_array_len);
            for (int64_t i = 0; i < trailing_bytes; ++i)
            {
                trailing[i] = pattern_bytes[i];
            }
        }
    }

#endif

static void jsl__libc_link_header(JSLLibcAllocator* allocator, struct JSL__LibcAllocationHeader* header)
{
    header->next = NULL;
    header->prev = allocator->head;

    if (allocator->head != NULL)
        allocator->head->next = header;

    allocator->head = header;
}

static void jsl__libc_unlink_header(JSLLibcAllocator* allocator, struct JSL__LibcAllocationHeader* header)
{
    if (header->next != NULL)
        header->next->prev = header->prev;

    if (header->prev != NULL)
        header->prev->next = header->next;

    if (allocator->head == header)
        allocator->head = header->prev;

    header->prev = NULL;
    header->next = NULL;
}

void jsl_libc_allocator_init(JSLLibcAllocator* allocator)
{
    if (allocator == NULL)
        return;

    allocator->sentinel = JSL__LIBC_ALLOCATOR_PRIVATE_SENTINEL;
    allocator->head = NULL;
}

static void* jsl__libc_alloc_interface_alloc(void* ctx, int64_t bytes, int32_t align, bool zeroed)
{
    JSLLibcAllocator* allocator = (JSLLibcAllocator*) ctx;
    return jsl_libc_allocator_allocate_aligned(allocator, bytes, align, zeroed);
}

static void* jsl__libc_alloc_interface_realloc(
    void* ctx,
    void* allocation,
    int64_t new_bytes,
    int32_t alignment
)
{
    JSLLibcAllocator* allocator = (JSLLibcAllocator*) ctx;
    return jsl_libc_allocator_reallocate_aligned(allocator, allocation, new_bytes, alignment);
}

static bool jsl__libc_alloc_interface_free(void* ctx, const void* allocation)
{
    JSLLibcAllocator* allocator = (JSLLibcAllocator*) ctx;
    return jsl_libc_allocator_free(allocator, allocation);
}

static bool jsl__libc_alloc_interface_free_all(void* ctx)
{
    JSLLibcAllocator* allocator = (JSLLibcAllocator*) ctx;
    return jsl_libc_allocator_free_all(allocator);
}

static bool jsl__libc_create_child(void* ctx, JSLAllocatorInterface* child)
{
    JSLLibcAllocator* parent = (JSLLibcAllocator*) ctx;
    bool success = false;

    JSLLibcAllocator* child_state = (JSLLibcAllocator*)
        jsl_libc_allocator_allocate_aligned(
            parent,
            (int64_t) sizeof(JSLLibcAllocator),
            (int32_t) _Alignof(JSLLibcAllocator),
            false
        );

    if (child_state != NULL)
    {
        jsl_libc_allocator_init(child_state);

        jsl_allocator_interface_init(
            child,
            jsl__libc_alloc_interface_alloc,
            jsl__libc_alloc_interface_realloc,
            jsl__libc_alloc_interface_free,
            jsl__libc_alloc_interface_free_all,
            jsl__libc_create_child,
            child_state
        );

        success = true;
    }

    return success;
}

void jsl_libc_allocator_get_allocator_interface(
    JSLAllocatorInterface* allocator,
    JSLLibcAllocator* libc_alloc
)
{
    jsl_allocator_interface_init(
        allocator,
        jsl__libc_alloc_interface_alloc,
        jsl__libc_alloc_interface_realloc,
        jsl__libc_alloc_interface_free,
        jsl__libc_alloc_interface_free_all,
        jsl__libc_create_child,
        libc_alloc
    );
}

void* jsl_libc_allocator_allocate(JSLLibcAllocator* allocator, int64_t bytes, bool zeroed)
{
    return jsl_libc_allocator_allocate_aligned(
        allocator,
        bytes,
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        zeroed
    );
}

void* jsl_libc_allocator_allocate_aligned(
    JSLLibcAllocator* allocator,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
)
{
    JSL_ASSERT(
        alignment > 0
        && jsl_is_power_of_two_i32(alignment)
    );

    #ifdef NDEBUG
        if (alignment < 1 || !jsl_is_power_of_two_i32(alignment))
            return NULL;
    #endif

    if (bytes < 1)
        return NULL;

    const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__LibcAllocationHeader);
    const int32_t effective_alignment = jsl__libc_effective_alignment(
        JSL_MAX(alignment, 8)
    );

    // Over-allocate to guarantee we can align the user pointer after the header.
    // Layout: [malloc'd region ... padding ... header | aligned user data]
    uintptr_t total = header_size + (uintptr_t) effective_alignment - 1 + (uintptr_t) bytes;
    void* raw = malloc((size_t) total);
    if (raw == NULL)
        return NULL;

    uintptr_t raw_addr = (uintptr_t) raw;
    uintptr_t base_after_header = raw_addr + header_size;
    uintptr_t aligned_addr = jsl_align_ptr_upwards_uintptr(base_after_header, effective_alignment);

    struct JSL__LibcAllocationHeader* header =
        (struct JSL__LibcAllocationHeader*) (aligned_addr - header_size);

    header->malloc_ptr = raw;
    header->length = bytes;

    jsl__libc_link_header(allocator, header);

    if (zeroed)
        JSL_MEMSET((void*) aligned_addr, 0, (size_t) bytes);

    return (void*) aligned_addr;
}

void* jsl_libc_allocator_reallocate(
    JSLLibcAllocator* allocator,
    void* original_allocation,
    int64_t new_bytes
)
{
    return jsl_libc_allocator_reallocate_aligned(
        allocator, original_allocation, new_bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT
    );
}

void* jsl_libc_allocator_reallocate_aligned(
    JSLLibcAllocator* allocator,
    void* original_allocation,
    int64_t new_bytes,
    int32_t alignment
)
{
    JSL_ASSERT(alignment > 0 && jsl_is_power_of_two_i32(alignment));

    #ifdef NDEBUG
        if (new_bytes < 1 || alignment < 1 || !jsl_is_power_of_two_i32(alignment))
            return NULL;
    #else
        if (new_bytes < 1)
            return NULL;
    #endif

    if (original_allocation == NULL)
        return jsl_libc_allocator_allocate_aligned(allocator, new_bytes, alignment, false);

    const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__LibcAllocationHeader);
    struct JSL__LibcAllocationHeader* old_header =
        (struct JSL__LibcAllocationHeader*) ((uint8_t*) original_allocation - header_size);
    int64_t old_length = old_header->length;

    void* new_ptr = jsl_libc_allocator_allocate_aligned(allocator, new_bytes, alignment, false);
    if (new_ptr == NULL)
        return NULL;

    size_t bytes_to_copy = (size_t) (new_bytes < old_length ? new_bytes : old_length);
    JSL_MEMCPY(new_ptr, original_allocation, bytes_to_copy);

    #ifdef JSL_DEBUG
        jsl__libc_debug_memset_old_memory(original_allocation, old_length);
    #endif

    jsl__libc_unlink_header(allocator, old_header);
    free(old_header->malloc_ptr);

    return new_ptr;
}

bool jsl_libc_allocator_free(JSLLibcAllocator* allocator, const void* allocation)
{
    if (allocation == NULL)
        return false;

    const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__LibcAllocationHeader);
    struct JSL__LibcAllocationHeader* header =
        (struct JSL__LibcAllocationHeader*) ((uint8_t*) allocation - header_size);

    #ifdef JSL_DEBUG
        jsl__libc_debug_memset_old_memory((void*) allocation, header->length);
    #endif

    jsl__libc_unlink_header(allocator, header);
    free(header->malloc_ptr);

    return true;
}

bool jsl_libc_allocator_free_all(JSLLibcAllocator* allocator)
{
    if (allocator == NULL || allocator->sentinel != JSL__LIBC_ALLOCATOR_PRIVATE_SENTINEL)
        return false;

    struct JSL__LibcAllocationHeader* current = allocator->head;
    while (current != NULL)
    {
        struct JSL__LibcAllocationHeader* to_free = current;
        current = current->prev;

        #ifdef JSL_DEBUG
            jsl__libc_debug_memset_old_memory(
                (uint8_t*) to_free + sizeof(struct JSL__LibcAllocationHeader),
                to_free->length
            );
        #endif

        free(to_free->malloc_ptr);
    }

    allocator->head = NULL;

    return true;
}

#undef JSL__LIBC_ALLOCATOR_PRIVATE_SENTINEL

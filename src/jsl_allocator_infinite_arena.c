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

#if JSL_IS_WINDOWS
    #include <memoryapi.h>
#elif JSL_IS_POSIX
    #include <sys/mman.h>
#else
    #error "jsl_allocator_infinite_arena.c: Only windows and posix systems are supported"
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"
#include "jsl_allocator_infinite_arena.h"

static JSL__FORCE_INLINE int32_t jsl__infinite_arena_effective_alignment(int32_t requested_alignment)
{
    int32_t header_alignment = (int32_t) _Alignof(struct JSL__InfiniteArenaAllocationHeader);
    return requested_alignment > header_alignment ? requested_alignment : header_alignment;
}

static JSL__FORCE_INLINE void jsl__infinite_arena_debug_memset_old_memory(void* allocation, int64_t num_bytes)
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

void jsl_infinite_arena_init(JSLInfiniteArena* arena)
{
    
}

static void* jsl__infinite_arena_alloc_interface_alloc(void* ctx, int64_t bytes, int32_t align, bool zeroed)
{
    JSLInfiniteArena* arena = (JSLInfiniteArena*) ctx;
    return jsl_infinite_arena_allocate_aligned(arena, bytes, align, zeroed);
}

static void* jsl__infinite_arena_alloc_interface_realloc(void* ctx, void* allocation, int64_t new_bytes, int32_t alignment)
{
    JSLInfiniteArena* arena = (JSLInfiniteArena*) ctx;
    return jsl_infinite_arena_reallocate_aligned(arena, allocation, new_bytes, alignment);
}

static bool jsl__infinite_arena_alloc_interface_free(void* ctx, void* allocation)
{
    (void) ctx;

    #ifdef JSL_DEBUG

        const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__InfiniteArenaAllocationHeader);
        struct JSL__InfiniteArenaAllocationHeader* header = (struct JSL__InfiniteArenaAllocationHeader*) (
            (uint8_t*) allocation - header_size
        );

        jsl__infinite_arena_debug_memset_old_memory(allocation, header->length);
        return true;

    #else

        (void) allocation;
        return true;

    #endif

}

static bool jsl__infinite_arena_alloc_interface_free_all(void* ctx)
{
    JSLInfiniteArena* arena = (JSLInfiniteArena*) ctx;
    jsl_infinite_arena_reset(arena);
    return true;
}

JSLAllocatorInterface jsl_infinite_arena_get_allocator_interface(JSLInfiniteArena* arena)
{
    JSLAllocatorInterface i;
    jsl_allocator_interface_init(
        &i,
        jsl__infinite_arena_alloc_interface_alloc,
        jsl__infinite_arena_alloc_interface_realloc,
        jsl__infinite_arena_alloc_interface_free,
        jsl__infinite_arena_alloc_interface_free_all,
        arena
    );
    return i;
}

void* jsl_infinite_arena_allocate(JSLInfiniteArena* arena, int64_t bytes, bool zeroed)
{
    return jsl_infinite_arena_allocate_aligned(
        arena,
        bytes,
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        zeroed
    );
}

void* jsl_infinite_arena_allocate_aligned(JSLInfiniteArena* arena, int64_t bytes, int32_t alignment, bool zeroed)
{
    JSL_ASSERT(
        alignment > 0
        && jsl_is_power_of_two(alignment)
    );

    #ifdef NDEBUG
        if (alignment < 1 || !jsl_is_power_of_two(alignment))
            return NULL;
    #endif

    
}

void* jsl_infinite_arena_reallocate(JSLInfiniteArena* arena, void* original_allocation, int64_t new_num_bytes)
{
    return jsl_infinite_arena_reallocate_aligned(
        arena, original_allocation, new_num_bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT
    );
}

void* jsl_infinite_arena_reallocate_aligned(
    JSLInfiniteArena* arena,
    void* original_allocation,
    int64_t new_num_bytes,
    int32_t align
)
{
    JSL_ASSERT(align > 0 && jsl_is_power_of_two(align));

    #ifdef NDEBUG
        if (align < 1 || !jsl_is_power_of_two(align))
            return NULL;
    #endif

    
}

void jsl_infinite_arena_reset(JSLInfiniteArena* arena)
{
    
}

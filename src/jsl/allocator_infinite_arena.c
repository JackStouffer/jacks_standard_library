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

#include "core.h"
#include "allocator.h"
#include "allocator_infinite_arena.h"

#include <errno.h>
#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#if JSL_IS_WINDOWS
    #include <windows.h>
#elif JSL_IS_POSIX
    #include <sys/mman.h>
#else
    #error "allocator_infinite_arena.c: Only windows and posix systems are supported"
#endif

#define JSL__INFINITE_ARENA_PRIVATE_SENTINEL 8926154793150255142U
#define JSL__INFINITE_ARENA_CHUNK_BYTES JSL_MEGABYTES(2)

static JSL__FORCE_INLINE int32_t jsl__infinite_arena_effective_alignment(int32_t requested_alignment)
{
    int32_t header_alignment = (int32_t) _Alignof(struct JSL__InfiniteArenaAllocationHeader);
    return requested_alignment > header_alignment ? requested_alignment : header_alignment;
}

#ifdef JSL_DEBUG

    static JSL__FORCE_INLINE void jsl__infinite_arena_debug_memset_old_memory(
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

bool jsl_infinite_arena_init(JSLInfiniteArena* arena)
{
    bool success = false;

    #if JSL_IS_WINDOWS

        arena->start = VirtualAlloc(
            NULL,
            (size_t) JSL_TERABYTES(8),
            MEM_RESERVE,
            PAGE_READWRITE
        );

        if (arena->start != NULL)
        {
            arena->current = arena->start;
            arena->end = arena->start + JSL_TERABYTES(8);
            arena->committed_bytes = 0;
            arena->sentinel = JSL__INFINITE_ARENA_PRIVATE_SENTINEL;
            success = true;
        }

    #elif JSL_IS_POSIX

        arena->start = mmap(
            NULL,
            (size_t) JSL_TERABYTES(8L),
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
        );

        if (arena->start != MAP_FAILED)
        {
            arena->current = arena->start;
            arena->end = arena->start + JSL_TERABYTES(8);
            arena->sentinel = JSL__INFINITE_ARENA_PRIVATE_SENTINEL;
            success = true;
        }

    #endif

    return success;
}

static void* jsl__infinite_arena_alloc_interface_alloc(
    void* ctx,
    int64_t bytes,
    int32_t align,
    bool zeroed
)
{
    JSLInfiniteArena* arena = (JSLInfiniteArena*) ctx;
    return jsl_infinite_arena_allocate_aligned(arena, bytes, align, zeroed);
}

static void* jsl__infinite_arena_alloc_interface_realloc(
    void* ctx,
    void* allocation,
    int64_t new_bytes,
    int32_t alignment
)
{
    JSLInfiniteArena* arena = (JSLInfiniteArena*) ctx;
    return jsl_infinite_arena_reallocate_aligned(arena, allocation, new_bytes, alignment);
}

static bool jsl__infinite_arena_alloc_interface_free(void* ctx, const void* allocation)
{
    (void) ctx;

    #ifdef JSL_DEBUG

        const uintptr_t header_size =
            (uintptr_t) sizeof(struct JSL__InfiniteArenaAllocationHeader);
        struct JSL__InfiniteArenaAllocationHeader* header =
        (struct JSL__InfiniteArenaAllocationHeader*) (
            (uint8_t*) allocation - header_size
        );

        jsl__infinite_arena_debug_memset_old_memory((void*) allocation, header->length);
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

void jsl_infinite_arena_get_allocator_interface(JSLAllocatorInterface* allocator, JSLInfiniteArena* arena)
{
    jsl_allocator_interface_init(
        allocator,
        jsl__infinite_arena_alloc_interface_alloc,
        jsl__infinite_arena_alloc_interface_realloc,
        jsl__infinite_arena_alloc_interface_free,
        jsl__infinite_arena_alloc_interface_free_all,
        arena
    );
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

void* jsl_infinite_arena_allocate_aligned(
    JSLInfiniteArena* arena,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
)
{
    JSL_ASSERT(alignment > 0 && jsl_is_power_of_two(alignment));

    #ifdef NDEBUG
        bool params_ok = (bytes > 0 && alignment > 0 && jsl_is_power_of_two(alignment));
    #else
        bool params_ok = (bytes > 0);
    #endif

    uintptr_t result_addr = 0;
    uintptr_t header_addr = 0;
    uintptr_t allocation_end = 0;
    uintptr_t next_current_addr = 0;
    
    const uintptr_t arena_end = (uintptr_t) arena->end;
    const uintptr_t arena_current = (uintptr_t) arena->current;
    const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__InfiniteArenaAllocationHeader);
    
    #if JSL__HAS_ASAN
        uintptr_t poison_gap_size = 0;
        const uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        const uintptr_t guard_size = 0;
    #endif

    // Calculates potential addresses. If inputs are invalid, 'result_addr' remains 0.
    if (params_ok)
    {
        const int32_t effective_alignment = jsl__infinite_arena_effective_alignment(
            JSL_MAX(alignment, 8)
        );

        const uintptr_t base_after_header = arena_current + header_size;
        
        // Calculate the aligned address
        result_addr = (uintptr_t) jsl_align_ptr_upwards(
            (void*) base_after_header, 
            effective_alignment
        );

        // Derive related addresses based on the calculated result
        allocation_end = result_addr + (uintptr_t) bytes;
        next_current_addr = allocation_end + guard_size;
        header_addr = result_addr - header_size;

        #if JSL__HAS_ASAN
        // Calculate gap size for ASAN (avoiding nested if later)
        poison_gap_size = (header_addr > arena_current) ? (header_addr - arena_current) : 0;
        #endif
    }

    // Verifies that the calculated addresses fit within the arena's reserved space.
    // If checks fail, 'result_addr' is reset to 0 to skip future phases.
    if (result_addr != 0)
    {
        const bool overflows_end = (next_current_addr > arena_end);
        const bool underflows_header = (result_addr < (arena_current + header_size));

        // Using a ternary to reset result_addr if bounds are violated
        result_addr = (overflows_end || underflows_header) ? 0 : result_addr;
    }

    // Checks if the allocation crosses the committed boundary and commits more memory if needed.
    // If VirtualAlloc fails, 'result_addr' is reset to 0.

    #if JSL_IS_WINDOWS

        size_t amount_to_commit = 0;
        void* committed_memory = NULL;
        const uintptr_t current_committed_end = result_addr != 0 ?
            (uintptr_t) arena->start + (uintptr_t) arena->committed_bytes
            : UINTPTR_MAX;

        if (allocation_end >= current_committed_end)
        {
            amount_to_commit = (size_t) jsl_round_up_pow2_u64(
                allocation_end - current_committed_end,
                (uint64_t) JSL_MEGABYTES(8)
            );

            committed_memory = VirtualAlloc(
                (void*) current_committed_end,
                amount_to_commit,
                MEM_COMMIT,
                PAGE_READWRITE
            );

            // If commit failed, invalidate the result
            result_addr = (committed_memory == NULL) ? 0 : result_addr;
        }

        // Update committed bytes only if successful
        if (committed_memory != NULL)
        {
            arena->committed_bytes += amount_to_commit;
            ASAN_POISON_MEMORY_REGION(
                (void*) current_committed_end,
                amount_to_commit
            );
        }

    #endif

    if (result_addr != 0)
    {
        struct JSL__InfiniteArenaAllocationHeader* header =
            (struct JSL__InfiniteArenaAllocationHeader*) header_addr;

        // Poison the gap between previous allocation and this header
        ASAN_POISON_MEMORY_REGION((void*) arena_current, poison_gap_size);

        // Unpoison the header and the body
        ASAN_UNPOISON_MEMORY_REGION(header, header_size + (size_t) bytes);

        header->length = bytes;
        arena->current = (uint8_t*) next_current_addr;

        // Poison the guard zone after the allocation
        ASAN_POISON_MEMORY_REGION((void*) allocation_end, guard_size);
    }

    if (result_addr != 0 && zeroed)
    {
        JSL_MEMSET((void*) result_addr, 0, (size_t) bytes);
    }

    return (void*) result_addr;
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
        if (new_num_bytes < 1 || align < 1 || !jsl_is_power_of_two(align))
            return NULL;
    #else
        if (new_num_bytes < 1)
            return NULL;
    #endif

    if (original_allocation == NULL)
        return jsl_infinite_arena_allocate_aligned(arena, new_num_bytes, align, false);

    const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__InfiniteArenaAllocationHeader);
    const int32_t effective_alignment = jsl__infinite_arena_effective_alignment(
        JSL_MAX(align, 8)
    );
    const uintptr_t arena_start = (uintptr_t) arena->start;
    const uintptr_t arena_end = (uintptr_t) arena->end;
    const uintptr_t arena_current_addr = (uintptr_t) arena->current;

    #if JSL__HAS_ASAN
        const uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        const uintptr_t guard_size = 0;
    #endif

    uintptr_t allocation_addr = (uintptr_t) original_allocation;
    uintptr_t header_addr = allocation_addr - header_size;

    bool header_in_range = header_addr >= arena_start && allocation_addr <= arena_end;
    if (!header_in_range)
        return NULL;

    struct JSL__InfiniteArenaAllocationHeader* header = (struct JSL__InfiniteArenaAllocationHeader*) header_addr;
    int64_t original_length = header->length;
    uintptr_t original_end_addr = allocation_addr + (uintptr_t) original_length;

    if (original_length < 0 || original_end_addr > arena_end)
        return NULL;

    if (new_num_bytes < original_length)
        return original_allocation;

    uintptr_t guarded_end = original_end_addr + guard_size;
    bool matches_no_guard = arena_current_addr == original_end_addr;
    bool matches_guard = arena_current_addr == guarded_end;
    bool has_required_alignment = (allocation_addr % (uintptr_t) effective_alignment) == 0;
    bool can_resize_in_place = (matches_no_guard || matches_guard) && has_required_alignment;

    uintptr_t potential_end = allocation_addr + (uintptr_t) new_num_bytes;

    bool guard_overflow_new = guard_size > UINTPTR_MAX - potential_end;
    uintptr_t next_current_addr = potential_end;
    if (!guard_overflow_new)
        next_current_addr = potential_end + guard_size;
    else
        can_resize_in_place = false;

    if (next_current_addr > arena_end)
        can_resize_in_place = false;

    if (can_resize_in_place)
    {
        header->length = new_num_bytes;
        arena->current = (uint8_t*) next_current_addr;

        #if JSL_IS_WINDOWS

            const uintptr_t current_committed_end = (uintptr_t) arena->start
                + (uintptr_t) arena->committed_bytes;

            if (potential_end >= current_committed_end)
            {
                size_t amount_to_commit = jsl_round_up_pow2_u64(
                    potential_end - current_committed_end,
                    JSL_MEGABYTES(8)
                );
                void* committed_memory = VirtualAlloc(
                    (void*) current_committed_end,
                    amount_to_commit,
                    MEM_COMMIT,
                    PAGE_READWRITE
                );
                if (committed_memory == NULL)
                    return NULL;
                arena->committed_bytes += amount_to_commit;

                #if JSL__HAS_ASAN
                    ASAN_POISON_MEMORY_REGION(
                        (void*) current_committed_end,
                        amount_to_commit
                    );
                #endif
            }

        #endif

        ASAN_UNPOISON_MEMORY_REGION(header, header_size + (size_t) new_num_bytes);
        ASAN_POISON_MEMORY_REGION((void*) potential_end, guard_size);

        if (new_num_bytes < original_length)
        {
            ASAN_POISON_MEMORY_REGION(
                (uint8_t*) original_allocation + new_num_bytes,
                (size_t) (original_length - new_num_bytes)
            );
        }

        return original_allocation;
    }

    void* res = jsl_infinite_arena_allocate_aligned(arena, new_num_bytes, align, false);
    if (res == NULL)
        return NULL;

    size_t bytes_to_copy = (size_t) (
        new_num_bytes < original_length ? new_num_bytes : original_length
    );
    JSL_MEMCPY(res, original_allocation, bytes_to_copy);

    #ifdef JSL_DEBUG
        jsl__infinite_arena_debug_memset_old_memory(original_allocation, original_length);
    #endif

    ASAN_POISON_MEMORY_REGION(
        (uint8_t*) original_allocation - header_size,
        header_size + (size_t) original_length
    );

    return res;
}

void jsl_infinite_arena_reset(JSLInfiniteArena* arena)
{
    if (arena != NULL && arena->sentinel == JSL__INFINITE_ARENA_PRIVATE_SENTINEL)
    {
        arena->current = arena->start;
    }
}

JSL_DEF void jsl_infinite_arena_release(JSLInfiniteArena* arena)
{
    if (arena != NULL && arena->sentinel == JSL__INFINITE_ARENA_PRIVATE_SENTINEL)
    {
        #if JSL_IS_WINDOWS
            VirtualFree(arena->start, 0, MEM_RELEASE);
        #elif JSL_IS_POSIX
            munmap(arena->start, (size_t) (arena->end - arena->current));
        #endif

        arena->sentinel = 0;
    }
}

#undef JSL__INFINITE_ARENA_PRIVATE_SENTINEL
#undef JSL__INFINITE_ARENA_CHUNK_BYTES

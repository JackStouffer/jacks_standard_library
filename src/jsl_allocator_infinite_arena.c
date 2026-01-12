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
    #include <windows.h>
#elif JSL_IS_POSIX
    #include <sys/mman.h>
#else
    #error "jsl_allocator_infinite_arena.c: Only windows and posix systems are supported"
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"
#include "jsl_allocator_infinite_arena.h"

static const int64_t JSL__INFINITE_ARENA_CHUNK_BYTES = JSL_MEGABYTES(2);

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

static struct JSL__InfiniteArenaChunk* jsl__infinite_arena_new_chunk(
    JSLInfiniteArena* arena,
    int64_t payload_bytes
)
{
    struct JSL__InfiniteArenaChunk* chunk = NULL;
    bool size_ok = true;
    uint64_t payload_u = 0;
    uint64_t total_u = 0;

    if (payload_bytes < 0)
        size_ok = false;
    else
        payload_u = (uint64_t) payload_bytes;

    if (size_ok && payload_u > UINT64_MAX - (uint64_t) sizeof(struct JSL__InfiniteArenaChunk))
        size_ok = false;
    else if (size_ok)
        total_u = payload_u + (uint64_t) sizeof(struct JSL__InfiniteArenaChunk);

    if (size_ok)
    {
        if (total_u > (uint64_t) SIZE_MAX || total_u > (uint64_t) UINTPTR_MAX)
            size_ok = false;
    }

    void* block = NULL;

    if (size_ok)
    {
        #if JSL_IS_WINDOWS
            block = VirtualAlloc(
                NULL,
                (SIZE_T) total_u,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE
            );
        #elif JSL_IS_POSIX
            void* mapped = mmap(
                NULL,
                (size_t) total_u,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1,
                0
            );
            if (mapped != MAP_FAILED)
                block = mapped;
        #endif
    }

    if (block != NULL)
    {
        uint8_t* chunk_start = (uint8_t*) block + sizeof(struct JSL__InfiniteArenaChunk);
        chunk = (struct JSL__InfiniteArenaChunk*) block;

        chunk->next = NULL;
        chunk->prev = arena->tail;
        chunk->start = chunk_start;
        chunk->current = chunk_start;
        chunk->end = chunk_start + (uintptr_t) payload_bytes;

        if (arena->tail != NULL)
            arena->tail->next = chunk;
        else
            arena->head = chunk;

        arena->tail = chunk;

        ASAN_POISON_MEMORY_REGION(
            chunk->current,
            (size_t) (chunk->end - chunk->current)
        );
    }

    return chunk;
}

static void* jsl__infinite_arena_try_alloc_from_chunk(
    struct JSL__InfiniteArenaChunk* chunk,
    int64_t bytes,
    int32_t effective_alignment,
    uintptr_t header_size,
    uintptr_t guard_size,
    bool zeroed
)
{
    void* allocation = NULL;
    uintptr_t chunk_end = 0;
    uintptr_t chunk_current = UINTPTR_MAX;
    uintptr_t aligned_addr = UINTPTR_MAX;
    uintptr_t next_current = UINTPTR_MAX;
    uintptr_t allocation_end = 0;

    if (chunk != NULL)
    {
        chunk_end = (uintptr_t) chunk->end;
        chunk_current = (uintptr_t) chunk->current;

        uintptr_t base_after_header = chunk_current + header_size;
        aligned_addr = (uintptr_t) jsl_align_ptr_upwards(
            (void*) base_after_header,
            effective_alignment
        );
    }

    if (aligned_addr <= chunk_end)
    {
        allocation_end = aligned_addr + (uintptr_t) bytes;
        next_current = allocation_end + guard_size;
    }

    if (next_current <= chunk_end)
    {
        struct JSL__InfiniteArenaAllocationHeader* header =
            (struct JSL__InfiniteArenaAllocationHeader*) (aligned_addr - header_size);

        ASAN_UNPOISON_MEMORY_REGION(header, header_size + (size_t) bytes);

        header->length = bytes;
        chunk->current = (uint8_t*) next_current;

        ASAN_POISON_MEMORY_REGION((void*) allocation_end, guard_size);

        if (zeroed)
            JSL_MEMSET((void*) aligned_addr, 0, (size_t) bytes);

        allocation = (void*) aligned_addr;
        success = true;
    }

    if (success && out_allocation != NULL)
        *out_allocation = allocation;

    return success;
}

void jsl_infinite_arena_init(JSLInfiniteArena* arena)
{
    arena->head = NULL;
    arena->tail = NULL;
    arena->free_list = NULL;
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

void* jsl_infinite_arena_allocate_aligned(
    JSLInfiniteArena* arena,
    int64_t bytes,
    int32_t alignment,
    bool zeroed
)
{
    void* result = NULL;
    bool params_ok = true;

    JSL_ASSERT(alignment > 0 && jsl_is_power_of_two(alignment));

    #ifdef NDEBUG
        if (alignment < 1 || !jsl_is_power_of_two(alignment))
            params_ok = false;
    #else
        
    #endif

    if (bytes < 1)
        params_ok = false;

    const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__InfiniteArenaAllocationHeader);
    const int32_t effective_alignment = jsl__infinite_arena_effective_alignment(alignment);

    #if JSL__HAS_ASAN
        const uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        const uintptr_t guard_size = 0;
    #endif

    struct JSL__InfiniteArenaChunk* chunk = NULL;
    if (params_ok)
    {
        chunk = arena->tail;
    }

    bool allocated_from_current = false;
    if (params_ok && chunk != NULL)
    {
        allocated_from_current = jsl__infinite_arena_try_alloc_from_chunk(
            chunk,
            bytes,
            effective_alignment,
            header_size,
            guard_size,
            zeroed,
            &result
        );
    }

    struct JSL__InfiniteArenaChunk* new_chunk = NULL;
    if (params_ok && !allocated_from_current)
    {
        int64_t slop = effective_alignment - 1;
        int64_t payload_needed = header_size + slop + bytes + guard_size;

        int64_t chunk_payload = payload_needed <= chunk_payload ?
            JSL__INFINITE_ARENA_CHUNK_BYTES
            : jsl_round_up_pow2_i64(
                payload_needed,
                JSL__INFINITE_ARENA_CHUNK_BYTES
            );

        new_chunk = jsl__infinite_arena_new_chunk(arena, chunk_payload);
    }

    if (new_chunk != NULL)
    {
        jsl__infinite_arena_try_alloc_from_chunk(
            new_chunk,
            bytes,
            effective_alignment,
            header_size,
            guard_size,
            zeroed,
            &result
        );
    }

    return result;
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

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

void* jsl_infinite_arena_allocate_aligned(JSLInfiniteArena* arena, int64_t bytes, int32_t alignment, bool zeroed)
{
    void* result = NULL;
    bool ok = true;

    JSL_ASSERT(alignment > 0 && jsl_is_power_of_two(alignment));

    #ifdef NDEBUG
        if (alignment < 1 || !jsl_is_power_of_two(alignment))
            ok = false;
    #endif

    if (bytes < 1)
        ok = false;

    if (ok)
    {
        const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__InfiniteArenaAllocationHeader);
        const int32_t effective_alignment = jsl__infinite_arena_effective_alignment(alignment);
        #if JSL__HAS_ASAN
            const uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
        #else
            const uintptr_t guard_size = 0;
        #endif

        struct JSL__InfiniteArenaChunk* chunk = arena->tail;
        bool allocation_success = false;

        if (chunk != NULL)
        {
            uintptr_t chunk_end = (uintptr_t) chunk->end;
            uintptr_t chunk_current = (uintptr_t) chunk->current;

            if (header_size <= UINTPTR_MAX - chunk_current)
            {
                uintptr_t base_after_header = chunk_current + header_size;
                uintptr_t aligned_addr = (uintptr_t) jsl_align_ptr_upwards(
                    (void*) base_after_header,
                    effective_alignment
                );

                if (aligned_addr >= base_after_header && aligned_addr <= chunk_end)
                {
                    if ((uint64_t) bytes <= UINTPTR_MAX - aligned_addr)
                    {
                        uintptr_t allocation_end = aligned_addr + (uintptr_t) bytes;

                        if (guard_size <= UINTPTR_MAX - allocation_end)
                        {
                            uintptr_t next_current = allocation_end + guard_size;

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

                                result = (void*) aligned_addr;
                                allocation_success = true;
                            }
                        }
                    }
                }
            }
        }

        if (!allocation_success)
        {
            bool size_ok = true;
            uint64_t payload_needed = 0;
            uint64_t header_u = (uint64_t) header_size;
            uint64_t slop_u = (uint64_t) (effective_alignment - 1);
            uint64_t bytes_u = (uint64_t) bytes;
            uint64_t guard_u = (uint64_t) guard_size;

            if (header_u > UINT64_MAX - slop_u)
                size_ok = false;
            else
                payload_needed = header_u + slop_u;

            if (size_ok && payload_needed > UINT64_MAX - bytes_u)
                size_ok = false;
            else if (size_ok)
                payload_needed += bytes_u;

            if (size_ok && payload_needed > UINT64_MAX - guard_u)
                size_ok = false;
            else if (size_ok)
                payload_needed += guard_u;

            uint64_t chunk_payload_u = 0;
            if (size_ok)
            {
                chunk_payload_u = (uint64_t) JSL__INFINITE_ARENA_CHUNK_BYTES;
                if (payload_needed > chunk_payload_u)
                {
                    if (payload_needed > (uint64_t) INT64_MAX)
                        size_ok = false;
                    else
                        chunk_payload_u = (uint64_t) jsl_round_up_pow2_i64(
                            (int64_t) payload_needed,
                            JSL__INFINITE_ARENA_CHUNK_BYTES
                        );
                }
            }

            if (size_ok)
            {
                if (chunk_payload_u > (uint64_t) SIZE_MAX || chunk_payload_u > (uint64_t) UINTPTR_MAX)
                    size_ok = false;
            }

            if (size_ok)
            {
                struct JSL__InfiniteArenaChunk* new_chunk = NULL;
                uint8_t* chunk_start = NULL;

                #if JSL_IS_WINDOWS
                    new_chunk = (struct JSL__InfiniteArenaChunk*) VirtualAlloc(
                        NULL,
                        (SIZE_T) sizeof(struct JSL__InfiniteArenaChunk),
                        MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE
                    );

                    if (new_chunk != NULL)
                    {
                        chunk_start = (uint8_t*) VirtualAlloc(
                            NULL,
                            (SIZE_T) chunk_payload_u,
                            MEM_COMMIT | MEM_RESERVE,
                            PAGE_READWRITE
                        );

                        if (chunk_start == NULL)
                        {
                            VirtualFree(new_chunk, 0, MEM_RELEASE);
                            new_chunk = NULL;
                        }
                    }
                #elif JSL_IS_POSIX
                    void* chunk_meta = mmap(
                        NULL,
                        (size_t) sizeof(struct JSL__InfiniteArenaChunk),
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1,
                        0
                    );

                    if (chunk_meta != MAP_FAILED)
                    {
                        new_chunk = (struct JSL__InfiniteArenaChunk*) chunk_meta;

                        void* chunk_mem = mmap(
                            NULL,
                            (size_t) chunk_payload_u,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1,
                            0
                        );

                        if (chunk_mem != MAP_FAILED)
                        {
                            chunk_start = (uint8_t*) chunk_mem;
                        }
                        else
                        {
                            munmap(chunk_meta, sizeof(struct JSL__InfiniteArenaChunk));
                            new_chunk = NULL;
                        }
                    }
                #endif

                if (new_chunk != NULL && chunk_start != NULL)
                {
                    new_chunk->next = NULL;
                    new_chunk->prev = arena->tail;
                    new_chunk->start = chunk_start;
                    new_chunk->current = chunk_start;
                    new_chunk->end = chunk_start + (uintptr_t) chunk_payload_u;

                    if (arena->tail != NULL)
                        arena->tail->next = new_chunk;
                    else
                        arena->head = new_chunk;

                    arena->tail = new_chunk;

                    ASAN_POISON_MEMORY_REGION(
                        new_chunk->current,
                        (size_t) (new_chunk->end - new_chunk->current)
                    );

                    chunk = new_chunk;

                    uintptr_t chunk_end = (uintptr_t) chunk->end;
                    uintptr_t chunk_current = (uintptr_t) chunk->current;

                    if (header_size <= UINTPTR_MAX - chunk_current)
                    {
                        uintptr_t base_after_header = chunk_current + header_size;
                        uintptr_t aligned_addr = (uintptr_t) jsl_align_ptr_upwards(
                            (void*) base_after_header,
                            effective_alignment
                        );

                        if (aligned_addr >= base_after_header && aligned_addr <= chunk_end)
                        {
                            if ((uint64_t) bytes <= UINTPTR_MAX - aligned_addr)
                            {
                                uintptr_t allocation_end = aligned_addr + (uintptr_t) bytes;

                                if (guard_size <= UINTPTR_MAX - allocation_end)
                                {
                                    uintptr_t next_current = allocation_end + guard_size;

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

                                        result = (void*) aligned_addr;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
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

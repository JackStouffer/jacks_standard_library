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
#include "jsl_allocator_arena.h"

static JSL__FORCE_INLINE int32_t jsl__arena_effective_alignment(int32_t requested_alignment)
{
    int32_t header_alignment = (int32_t) _Alignof(struct JSL__ArenaAllocationHeader);
    return requested_alignment > header_alignment ? requested_alignment : header_alignment;
}

void jsl_arena_init(JSLArena* arena, void* memory, int64_t length)
{
    arena->start = memory;
    arena->current = memory;
    arena->end = (uint8_t*) memory + length;

    ASAN_POISON_MEMORY_REGION(memory, length);
}

void jsl_arena_init2(JSLArena* arena, JSLFatPtr memory)
{
    arena->start = memory.data;
    arena->current = memory.data;
    arena->end = memory.data + memory.length;

    ASAN_POISON_MEMORY_REGION(memory.data, memory.length);
}

static void* alloc_interface_alloc(void* ctx, int64_t bytes, int32_t align, bool zeroed)
{
    JSLArena* arena = (JSLArena*) ctx;
    return jsl_arena_allocate_aligned(arena, bytes, align, zeroed);
}

static void* alloc_interface_realloc(void* ctx, void* allocation, int64_t new_bytes, int32_t alignment)
{
    JSLArena* arena = (JSLArena*) ctx;
    return jsl_arena_reallocate_aligned(arena, allocation, new_bytes, alignment);
}

static bool alloc_interface_free(void* ctx, void* allocation)
{
    (void) ctx;
    (void) allocation;

    // TODO:
    // #ifdef JSL_DEBUG
    //     JSL_MEMSET();
    // #endif

    return true;
}

static bool alloc_interface_free_all(void* ctx)
{
    JSLArena* arena = (JSLArena*) ctx;
    jsl_arena_reset(arena);
    return true;
}

JSLAllocatorInterface jsl_arena_get_allocator_interface(JSLArena* arena)
{
    JSLAllocatorInterface i;
    jsl_allocator_interface_init(
        &i,
        alloc_interface_alloc,
        alloc_interface_realloc,
        alloc_interface_free,
        alloc_interface_free_all,
        arena
    );
    return i;
}

void* jsl_arena_allocate(JSLArena* arena, int64_t bytes, bool zeroed)
{
    return jsl_arena_allocate_aligned(
        arena,
        bytes,
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        zeroed
    );
}

void* jsl_arena_allocate_aligned(JSLArena* arena, int64_t bytes, int32_t alignment, bool zeroed)
{
    JSL_ASSERT(
        alignment > 0
        && jsl_is_power_of_two(alignment)
    );

    #ifdef NDEBUG
        if (alignment < 1 || !jsl_is_power_of_two(alignment))
            return NULL;
    #endif

    if (bytes < 1)
        return NULL;

    const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__ArenaAllocationHeader);
    const int32_t effective_alignment = jsl__arena_effective_alignment(alignment);
    uintptr_t arena_end = (uintptr_t) arena->end;
    uintptr_t arena_current_addr = (uintptr_t) arena->current;

    // When ASAN is enabled, we leave poisoned guard
    // zones between allocations to catch buffer overflows
    #if JSL__HAS_ASAN
        uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        uintptr_t guard_size = 0;
    #endif

    if (header_size > UINTPTR_MAX - arena_current_addr)
        return NULL;

    uintptr_t base_after_header = arena_current_addr + header_size;
    uintptr_t aligned_allocation_addr = (uintptr_t) jsl_align_ptr_upwards(
        (void*) base_after_header,
        effective_alignment
    );

    if (aligned_allocation_addr < base_after_header || aligned_allocation_addr > arena_end)
        return NULL;

    if ((uint64_t) bytes > UINTPTR_MAX - aligned_allocation_addr)
        return NULL;

    uintptr_t allocation_end = aligned_allocation_addr + (uintptr_t) bytes;

    if (guard_size > UINTPTR_MAX - allocation_end)
        return NULL;

    uintptr_t next_current_addr = allocation_end + guard_size;

    if (next_current_addr > arena_end)
        return NULL;

    struct JSL__ArenaAllocationHeader* header = (struct JSL__ArenaAllocationHeader*) (aligned_allocation_addr - header_size);
    header->length = bytes;

    arena->current = (uint8_t*) next_current_addr;

    ASAN_UNPOISON_MEMORY_REGION(header, header_size + (size_t) bytes);
    ASAN_POISON_MEMORY_REGION((void*) allocation_end, guard_size);

    if (zeroed)
        JSL_MEMSET((void*) aligned_allocation_addr, 0, (size_t) bytes);

    return (void*) aligned_allocation_addr;
}

void* jsl_arena_reallocate(JSLArena* arena, void* original_allocation, int64_t new_num_bytes)
{
    return jsl_arena_reallocate_aligned(
        arena, original_allocation, new_num_bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT
    );
}

void* jsl_arena_reallocate_aligned(
    JSLArena* arena,
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

    if (new_num_bytes < 1)
        return NULL;
    if (original_allocation == NULL)
        return jsl_arena_allocate_aligned(arena, new_num_bytes, align, false);

    const uintptr_t header_size = (uintptr_t) sizeof(struct JSL__ArenaAllocationHeader);
    const int32_t effective_alignment = jsl__arena_effective_alignment(align);
    const uintptr_t arena_start = (uintptr_t) arena->start;
    const uintptr_t arena_end = (uintptr_t) arena->end;
    const uintptr_t arena_current_addr = (uintptr_t) arena->current;

    #if JSL__HAS_ASAN
        const uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        const uintptr_t guard_size = 0;
    #endif

    uintptr_t allocation_addr = (uintptr_t) original_allocation;

    if (allocation_addr < header_size)
        return NULL;

    uintptr_t header_addr = allocation_addr - header_size;

    bool header_in_range = header_addr >= arena_start && allocation_addr <= arena_end;
    if (!header_in_range)
        return NULL;

    if ((allocation_addr % (uintptr_t) effective_alignment) != 0)
        return NULL;

    struct JSL__ArenaAllocationHeader* header = (struct JSL__ArenaAllocationHeader*) header_addr;
    int64_t original_length = header->length;

    if (original_length < 0)
        return NULL;

    if ((uint64_t) original_length > UINTPTR_MAX - allocation_addr)
        return NULL;

    uintptr_t original_end_addr = allocation_addr + (uintptr_t) original_length;
    if (original_end_addr > arena_end)
        return NULL;

    uintptr_t guarded_end = original_end_addr;
    bool guard_overflow = guard_size > UINTPTR_MAX - original_end_addr;
    if (!guard_overflow)
        guarded_end = original_end_addr + guard_size;

    bool matches_no_guard = arena_current_addr == original_end_addr;
    bool matches_guard = !guard_overflow && arena_current_addr == guarded_end;
    bool can_resize_in_place = matches_no_guard || matches_guard;

    bool new_size_overflow = (uint64_t) new_num_bytes > UINTPTR_MAX - allocation_addr;
    if (new_size_overflow)
        can_resize_in_place = false;

    uintptr_t potential_end = allocation_addr;
    if (!new_size_overflow)
        potential_end = allocation_addr + (uintptr_t) new_num_bytes;

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

    void* res = jsl_arena_allocate_aligned(arena, new_num_bytes, align, false);
    if (res == NULL)
        return NULL;

    size_t bytes_to_copy = (size_t) (
        new_num_bytes < original_length ? new_num_bytes : original_length
    );
    JSL_MEMCPY(res, original_allocation, bytes_to_copy);

    #ifdef JSL_DEBUG
        int64_t* fake_array = (int64_t*) original_allocation;
        int64_t fake_array_len = original_length / (int64_t) sizeof(int64_t);
        for (int64_t i = 0; i < fake_array_len; ++i)
        {
            fake_array[i] = 0xfeeefeee;
        }
    #endif

    ASAN_POISON_MEMORY_REGION(
        (uint8_t*) original_allocation - header_size,
        header_size + (size_t) original_length
    );

    return res;
}

void jsl_arena_reset(JSLArena* arena)
{
    ASAN_UNPOISON_MEMORY_REGION(arena->start, arena->end - arena->start);

    #ifdef JSL_DEBUG
        JSL_MEMSET(
            (void*) arena->start,
            (int32_t) 0xfeeefeee,
            (size_t) (arena->current - arena->start)
        );
    #endif

    arena->current = arena->start;

    ASAN_POISON_MEMORY_REGION(arena->start, arena->end - arena->start);
}

uint8_t* jsl_arena_save_restore_point(JSLArena* arena)
{
    return arena->current;
}

void jsl_arena_load_restore_point(JSLArena* arena, uint8_t* restore_point)
{
    const uintptr_t restore_addr = (uintptr_t) restore_point;
    const uintptr_t start_addr = (uintptr_t) arena->start;
    const uintptr_t end_addr = (uintptr_t) arena->end;
    const uintptr_t current_addr = (uintptr_t) arena->current;

    const bool in_bounds = restore_addr >= start_addr && restore_addr <= end_addr;
    const bool before_current = restore_addr <= current_addr;

    JSL_ASSERT(in_bounds && before_current);
    #ifdef NDEBUG
        if (!in_bounds || !before_current)
            return;
    #endif

    ASAN_UNPOISON_MEMORY_REGION(
        restore_point,
        (size_t) (current_addr - restore_addr)
    );

    #ifdef JSL_DEBUG
        JSL_MEMSET(
            (void*) restore_point,
            (int32_t) 0xfeeefeee,
            (size_t) (current_addr - restore_addr)
        );
    #endif

    ASAN_POISON_MEMORY_REGION(
        restore_point,
        (size_t) (current_addr - restore_addr)
    );

    arena->current = restore_point;
}

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"
#include "jsl_allocator_arena.h"

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

static JSLFatPtr alloc_interface_alloc(void* ctx, int64_t bytes, int32_t align, bool zeroed)
{
    JSLArena* arena = (JSLArena*) ctx;
    return jsl_arena_allocate_aligned(arena, bytes, align, zeroed);
}

static JSLFatPtr alloc_interface_realloc(void* ctx, JSLFatPtr allocation, int64_t new_bytes, int32_t alignment)
{
    JSLArena* arena = (JSLArena*) ctx;
    return jsl_arena_reallocate_aligned(arena, allocation, new_bytes, alignment);
}

static bool alloc_interface_free(void* ctx, JSLFatPtr allocation)
{
    // TODO:
    // #ifdef JSL_DEBUG
    //     JSL_MEMSET();
    // #endif

    return true;
}

static bool alloc_interface_free_all(void* ctx, JSLFatPtr allocation)
{
    JSLArena* arena = (JSLArena*) ctx;
    jsl_arena_reset(arena);
    return true;
}

JSLAllocatorInterface jsl_arena_get_allocator_interface(JSLArena* arena)
{
    JSLAllocatorInterface i;
    jsl_allocator_init(
        &i,
        alloc_interface_alloc,
        alloc_interface_realloc,
        alloc_interface_free,
        alloc_interface_free_all,
        arena
    );
    return i;
}

JSLFatPtr jsl_arena_allocate(JSLArena* arena, int64_t bytes, bool zeroed)
{
    return jsl_arena_allocate_aligned(arena, bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT, zeroed);
}

JSLFatPtr jsl_arena_allocate_aligned(JSLArena* arena, int64_t bytes, int32_t alignment, bool zeroed)
{
    JSL_ASSERT(
        alignment > 0
        && jsl_is_power_of_two(alignment)
    );

    JSLFatPtr res = {0};

    #ifdef NDEBUG
        if (alignment < 1 || !jsl_is_power_of_two(alignment))
        {
            return res;
        }
    #endif

    if (bytes < 0)
        return res;

    uintptr_t arena_end = (uintptr_t) arena->end;
    uintptr_t aligned_current_addr = (uintptr_t) jsl_align_ptr_upwards(arena->current, alignment);

    if (aligned_current_addr > arena_end)
        return res;

    // When ASAN is enabled, we leave poisoned guard
    // zones between allocations to catch buffer overflows
    #if JSL__HAS_ASAN
        uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        uintptr_t guard_size = 0;
    #endif

    if ((uint64_t) bytes > UINTPTR_MAX - aligned_current_addr)
        return res;

    uintptr_t potential_end = aligned_current_addr + (uintptr_t) bytes;
    uintptr_t next_current_addr = potential_end + guard_size;

    if (next_current_addr <= arena_end)
    {
        uint8_t* aligned_current = (uint8_t*) aligned_current_addr;

        res.data = aligned_current;
        res.length = bytes;

        #if JSL__HAS_ASAN
            arena->current = (uint8_t*) next_current_addr;
            ASAN_UNPOISON_MEMORY_REGION(res.data, res.length);
        #else
            arena->current = (uint8_t*) next_current_addr;
        #endif

        if (zeroed)
            JSL_MEMSET((void*) res.data, 0, (size_t) res.length);
    }

    return res;
}

JSLFatPtr jsl_arena_reallocate(JSLArena* arena, JSLFatPtr original_allocation, int64_t new_num_bytes)
{
    return jsl_arena_reallocate_aligned(
        arena, original_allocation, new_num_bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT
    );
}

JSLFatPtr jsl_arena_reallocate_aligned(
    JSLArena* arena,
    JSLFatPtr original_allocation,
    int64_t new_num_bytes,
    int32_t align
)
{
    JSL_ASSERT(align > 0 && jsl_is_power_of_two(align));

    JSLFatPtr res = {0};
    bool should_continue = true;

    #ifdef NDEBUG
        bool runtime_align_valid = (align > 0 && jsl_is_power_of_two(align));
        if (!runtime_align_valid)
        {
            should_continue = false;
        }
    #endif

    bool sizes_non_negative = (new_num_bytes >= 0 && original_allocation.length >= 0);
    if (!sizes_non_negative)
    {
        should_continue = false;
    }

    const uintptr_t arena_start = (uintptr_t) arena->start;
    const uintptr_t arena_end = (uintptr_t) arena->end;

    #if JSL__HAS_ASAN
        const uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        const uintptr_t guard_size = 0;
    #endif

    // Only resize if this given allocation was the last thing alloc-ed
    uintptr_t original_data_addr = (uintptr_t) original_allocation.data;
    uintptr_t aligned_original_addr = 0;
    uintptr_t original_end_addr = 0;
    uintptr_t arena_current_addr = (uintptr_t) arena->current;

    bool original_length_fits = (uint64_t) original_allocation.length <= UINTPTR_MAX - original_data_addr;
    bool original_addr_in_range = (
        original_data_addr >= arena_start
        && original_data_addr <= arena_end
    );
    bool pointer_checks_passed = should_continue && original_addr_in_range && original_length_fits;

    if (pointer_checks_passed)
    {
        original_end_addr = original_data_addr + (uintptr_t) original_allocation.length;
    }

    bool matches_no_guard = pointer_checks_passed && arena_current_addr == original_end_addr;

    bool guard_room_available = pointer_checks_passed && guard_size <= UINTPTR_MAX - original_end_addr;
    uintptr_t guarded_end = original_end_addr;
    if (guard_room_available)
    {
        guarded_end = original_end_addr + guard_size;
    }

    bool matches_guard = guard_room_available && arena_current_addr == guarded_end;
    bool same_pointer = pointer_checks_passed && (matches_no_guard || matches_guard);

    if (same_pointer)
    {
        aligned_original_addr = (uintptr_t) jsl_align_ptr_upwards(
            (uint8_t*) original_data_addr,
            align
        );
    }

    bool can_hold_bytes = same_pointer && ((uint64_t) new_num_bytes <= UINTPTR_MAX - aligned_original_addr);
    uintptr_t potential_end = aligned_original_addr;
    if (can_hold_bytes)
    {
        potential_end = aligned_original_addr + (uintptr_t) new_num_bytes;
    }

    bool guard_overflow = can_hold_bytes && guard_size > UINTPTR_MAX - potential_end;
    bool can_advance_current = can_hold_bytes && !guard_overflow;

    uintptr_t next_current_addr = potential_end;
    if (can_advance_current)
    {
        next_current_addr = potential_end + guard_size;
    }

    bool is_space_left = can_advance_current && next_current_addr <= arena_end;
    bool perform_in_place = is_space_left;

    if (perform_in_place)
    {
        res.data = (uint8_t*) original_data_addr;
        res.length = new_num_bytes;
        arena->current = (uint8_t*) next_current_addr;

        ASAN_UNPOISON_MEMORY_REGION(res.data, res.length);
    }

    bool shrink_allocation = perform_in_place && new_num_bytes < original_allocation.length;
    if (shrink_allocation)
    {
        ASAN_POISON_MEMORY_REGION(
            res.data + new_num_bytes,
            original_allocation.length - new_num_bytes
        );
    }

    bool should_allocate_new = should_continue && !perform_in_place;
    if (should_allocate_new)
    {
        res = jsl_arena_allocate_aligned(arena, new_num_bytes, align, false);
    }

    bool allocation_successful = should_allocate_new && res.data != NULL;
    if (allocation_successful)
    {
        JSL_MEMCPY(
            res.data,
            original_allocation.data,
            (size_t) original_allocation.length
        );

        #ifdef JSL_DEBUG
            int64_t* fake_array = (int64_t*) original_allocation.data;
            int64_t fake_array_len = original_allocation.length / (int64_t) sizeof(int64_t);
            for (int64_t i = 0; i < fake_array_len; ++i)
            {
                fake_array[i] = 0xfeeefeee;
            }
        #endif

        ASAN_POISON_MEMORY_REGION(original_allocation.data, original_allocation.length);
    }

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

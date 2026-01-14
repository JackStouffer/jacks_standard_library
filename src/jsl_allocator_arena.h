#pragma once

#include <stdint.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"

// Stored immediately before every allocation so realloc can recover the length.
struct JSL__ArenaAllocationHeader
{
    int64_t length;
};

/**
 * A bump allocator. Designed for situations in your program when you know a
 * definite lifetime and a good upper bound on how much memory that lifetime will
 * need.
 *
 * See the DESIGN.md file for detailed notes on arena implementation, their uses,
 * and when they shouldn't be used.
 *
 * Functions and Macros:
 *
 * * jsl_arena_init
 * * jsl_arena_init2
 * * jsl_arena_allocate
 * * jsl_arena_allocate_aligned
 * * jsl_arena_reallocate
 * * jsl_arena_reallocate_aligned
 * * jsl_arena_reset
 * * jsl_arena_save_restore_point
 * * jsl_arena_load_restore_point
 * * JSL_ARENA_TYPED_ALLOCATE
 * * JSL_ARENA_FROM_STACK
 *
 * @note The arena API is not thread safe. Arena memory is assumed to live in a
 * single thread. If you want to share an arena between threads you need to lock.
 */
typedef struct JSLArena
{
    uint8_t* start;
    uint8_t* current;
    uint8_t* end;
} JSLArena;

static inline JSLArena jsl__arena_from_stack_internal(void* buf, size_t len)
{
    JSLArena arena = {
        (uint8_t*) buf,
        (uint8_t*) buf,
        ((uint8_t*) buf) + len
    };

    ASAN_POISON_MEMORY_REGION(buf, len);

    return arena;
}

/**
 * Creates an arena from stack memory.
 *
 * Example
 *
 * ```
 * uint8_t buffer[2048];
 * JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 * ```
 *
 * This incredibly useful for getting a dynamic allocator for things which will only
 * last the lifetime of the current function. For example, if the current function
 * needs a hash map, you can use this macro and then there's no cleanup at the end
 * because the stack pointer will be reset.
 *
 * ```
 * void some_func(void)
 * {
 *      uint8_t buffer[JSL_KILOBYTES(16)];
 *      JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 *
 *      // example hash map, not real
 *      IntToStrMap map = int_to_str_ctor(&arena);
 *      int_to_str_add(&map, 64, JSL_FATPTR_INITIALIZER("This is my string data!"));
 *
 *      my_hash_map_calculations(&map);
 *
 *      // All hash map memory goes out of scope automatically,
 *      // no need for any destructor
 * }
 * ```
 *
 * Fast, cheap, easy automatic memory management!
 */
#define JSL_ARENA_FROM_STACK(buf) jsl__arena_from_stack_internal((buf), sizeof(buf))

/**
 * Initialize an arena with the supplied buffer.
 *
 * @param arena Arena instance to initialize; must not be null.
 * @param memory Pointer to the beginning of the backing storage.
 * @param length Size of the backing storage in bytes.
 */
JSL_DEF void jsl_arena_init(JSLArena* arena, void* memory, int64_t length);

/**
 * Initialize an arena using a fat pointer as the backing buffer.
 *
 * This is a convenience overload for cases where the backing memory and its
 * length are already packaged in a `JSLFatPtr`.
 *
 * @param arena Arena to initialize; must not be null.
 * @param memory Backing storage for the arena; `memory.data` must not be null.
 */
JSL_DEF void jsl_arena_init2(JSLArena* arena, JSLFatPtr memory);

/**
 * TODO: docs
 */
JSL_DEF JSLAllocatorInterface jsl_arena_get_allocator_interface(JSLArena* arena);

/**
 * Allocate a block of memory from the arena using the default alignment.
 *
 * NULL is returned if the arena does not have enough capacity. When
 * `zeroed` is true, the allocated bytes are zero-initialized.
 *
 * @param arena Arena to allocate from; must not be null.
 * @param bytes Number of bytes to reserve.
 * @param zeroed When true, zero-initialize the allocation.
 * @return Fat pointer describing the allocation or `{0}` on failure.
 */
JSL_DEF void* jsl_arena_allocate(JSLArena* arena, int64_t bytes, bool zeroed);

/**
 * Allocate a block of memory from the arena with the provided alignment.
 *
 * NULL is returned if the arena does not have enough capacity. When
 * `zeroed` is true, the allocated bytes are zero-initialized.
 *
 * @param arena Arena to allocate from; must not be null.
 * @param bytes Number of bytes to reserve.
 * @param alignment Desired alignment in bytes; must be a positive power of two.
 * @param zeroed When true, zero-initialize the allocation.
 * @return Fat pointer describing the allocation or `{0}` on failure.
 */
JSL_DEF void* jsl_arena_allocate_aligned(JSLArena* arena, int64_t bytes, int32_t alignment, bool zeroed);

/**
 * Macro to make it easier to allocate an instance of `T` within an arena.
 *
 * @param T Type to allocate.
 * @param arena Arena to allocate from; must be initialized.
 * @return Pointer to the allocated object or `NULL` on failure.
 *
 * ```
 * struct MyStruct { uint64_t the_data; };
 * struct MyStruct* thing = JSL_ARENA_TYPED_ALLOCATE(struct MyStruct, arena);
 * ```
 */
#define JSL_ARENA_TYPED_ALLOCATE(T, arena) (T*) jsl_arena_allocate_aligned(arena, sizeof(T), _Alignof(T), false)

/**
 * Macro to make it easier to allocate a zero filled array of `T` within an arena.
 *
 * @param T Type to allocate.
 * @param arena Arena to allocate from; must be initialized.
 * @return Pointer to the allocated object or `NULL` on failure.
 *
 * @code
 * struct MyStruct { uint64_t the_data; };
 * struct MyStruct* thing_array = JSL_ARENA_TYPED_ALLOCATE(struct MyStruct, arena, 42);
 * @endcode
 */
#define JSL_ARENA_TYPED_ARRAY_ALLOCATE(T, arena, length) (T*) jsl_arena_allocate_aligned(arena, (int64_t) sizeof(T) * length, _Alignof(T), true)

/**
 * Resize the allocation if it was the last allocation, otherwise, allocate a new
 * chunk of memory and copy the old allocation's contents.
 */
JSL_DEF void* jsl_arena_reallocate(
    JSLArena* arena,
    void* original_allocation,
    int64_t new_num_bytes
);

/**
 * Resize the allocation if it was the last allocation, otherwise, allocate a new
 * chunk of memory and copy the old allocation's contents.
 */
JSL_DEF void* jsl_arena_reallocate_aligned(
    JSLArena* arena,
    void* original_allocation,
    int64_t new_num_bytes,
    int32_t align
);

/**
 * Set the current pointer back to the start of the arena.
 *
 * In debug mode, this function will set all of the memory that was
 * allocated to `0xfeefee` to help detect use after free bugs.
 */
JSL_DEF void jsl_arena_reset(JSLArena* arena);

/**
 * The functions jsl_arena_save_restore_point and jsl_arena_load_restore_point
 * help you make temporary allocations inside an existing arena. You can think of
 * it as an "arena inside an arena". Basically the save function marks the current
 * state of the arena and the load function sets the saved state to the given arena,
 * wiping out any allocations which happened in the interim.
 *
 * This is very useful when you need memory from the arena but only for a specific
 * function.
 *
 * For example, say you have an existing one megabyte arena that has used 128 kilobytes
 * of space. You then call a function with this arena which needs a string to make an
 * operating system call, but that string is no longer needed after the function returns.
 * You can "save" and "load" a restore point at the start and end of the function
 * (respectively) and when the function returns, the arena will still only have 128
 * kilobytes used.
 *
 * In debug mode, jsl_arena_load_restore_point function will set all of the memory
 * that was allocated to `0xfeefee` to help detect use after free bugs.
 */
JSL_DEF uint8_t* jsl_arena_save_restore_point(JSLArena* arena);

/**
 * The functions jsl_arena_save_restore_point and jsl_arena_load_restore_point
 * help you make temporary allocations inside an existing arena. You can think of
 * it as an "arena inside an arena". Basically the save function marks the current
 * state of the arena and the load function sets the saved state to the given arena,
 * wiping out any allocations which happened in the interim.
 *
 * This is very useful when you need memory from the arena but only for a specific
 * function.
 *
 * For example, say you have an existing one megabyte arena that has used 128 kilobytes
 * of space. You then call a function with this arena which needs a string to make an
 * operating system call, but that string is no longer needed after the function returns.
 * You can "save" and "load" a restore point at the start and end of the function
 * (respectively) and when the function returns, the arena will still only have 128
 * kilobytes used.
 *
 * In debug mode, jsl_arena_load_restore_point function will set all of the memory
 * that was allocated to `0xfeefee` to help detect use after free bugs.
 * 
 * This function will assert if you attempt to use a different restore point from an
 * arena different that the one passed in as the parameter.
 */
JSL_DEF void jsl_arena_load_restore_point(JSLArena* arena, uint8_t* restore_point);

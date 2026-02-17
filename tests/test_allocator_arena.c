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

#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/jsl_core.h"
#include "../src/jsl_allocator.h"
#include "../src/jsl_allocator_arena.h"
#include "../src/jsl_allocator_infinite_arena.h"

#include "minctest.h"

typedef struct TestStruct
{
    uint32_t a;
    uint32_t b;
} TestStruct;

typedef struct TestAlign16
{
    _Alignas(16) uint8_t data[16];
} TestAlign16;

static void test_arena_init_sets_pointers(void)
{
    uint8_t buffer[128];
    JSLArena arena = {0};

    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    TEST_POINTERS_EQUAL(arena.start, buffer);
    TEST_POINTERS_EQUAL(arena.current, buffer);
    TEST_POINTERS_EQUAL(arena.end, buffer + sizeof(buffer));
}

static void test_arena_init2_sets_pointers(void)
{
    uint8_t buffer[96];
    JSLMutableMemory memory = JSL_MEMORY_FROM_STACK(buffer);

    JSLArena arena = {0};
    jsl_arena_init2(&arena, memory);

    TEST_POINTERS_EQUAL(arena.start, buffer);
    TEST_POINTERS_EQUAL(arena.current, buffer);
    TEST_POINTERS_EQUAL(arena.end, buffer + sizeof(buffer));
}

static void test_arena_allocate_zeroed_and_alignment(void)
{
    uint8_t buffer[4096];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    uint8_t* allocation = (uint8_t*) jsl_arena_allocate(&arena, 32, true);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL(((uintptr_t) allocation % JSL_DEFAULT_ALLOCATION_ALIGNMENT) == 0);
    for (int64_t i = 0; i < 32; ++i)
    {
        TEST_BOOL(allocation[i] == 0);
    }

    void* aligned = jsl_arena_allocate_aligned(&arena, 16, 64, false);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    TEST_BOOL(((uintptr_t) aligned % 64) == 0);

    aligned = jsl_arena_allocate_aligned(&arena, 8, 256, false);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    TEST_BOOL(((uintptr_t) aligned % 256) == 0);
}

static void test_arena_allocate_invalid_sizes_return_null(void)
{
    uint8_t buffer[128];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    TEST_POINTERS_EQUAL(jsl_arena_allocate(&arena, 0, false), NULL);
    TEST_POINTERS_EQUAL(jsl_arena_allocate(&arena, -5, false), NULL);
    TEST_POINTERS_EQUAL(jsl_arena_allocate_aligned(&arena, 0, 8, false), NULL);
}

static void test_arena_allocate_out_of_memory_returns_null(void)
{
    uint8_t buffer[64];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    TEST_POINTERS_EQUAL(jsl_arena_allocate(&arena, 1024, false), NULL);
}

static void test_arena_reallocate_null_behaves_like_allocate(void)
{
    uint8_t buffer[256];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    void* allocation = jsl_arena_reallocate(&arena, NULL, 24);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL(((uintptr_t) allocation % JSL_DEFAULT_ALLOCATION_ALIGNMENT) == 0);
}

static void test_arena_reallocate_in_place_when_last(void)
{
    uint8_t buffer[512];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    uint8_t* allocation = (uint8_t*) jsl_arena_allocate(&arena, 16, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    for (uint8_t i = 0; i < 16; ++i)
    {
        allocation[i] = (uint8_t) (i + 1);
    }

    void* grown = jsl_arena_reallocate(&arena, allocation, 32);
    TEST_POINTERS_EQUAL(grown, allocation);
    for (uint8_t i = 0; i < 16; ++i)
    {
        TEST_BOOL(allocation[i] == (uint8_t) (i + 1));
    }

    void* shrunk = jsl_arena_reallocate(&arena, allocation, 8);
    TEST_POINTERS_EQUAL(shrunk, allocation);
    for (uint8_t i = 0; i < 8; ++i)
    {
        TEST_BOOL(allocation[i] == (uint8_t) (i + 1));
    }
}

static void test_arena_reallocate_not_last_allocates_new(void)
{
    uint8_t buffer[512];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    uint8_t expected[16];
    for (uint8_t i = 0; i < 16; ++i)
    {
        expected[i] = (uint8_t) (200 + i);
    }

    uint8_t* first = (uint8_t*) jsl_arena_allocate(&arena, 16, false);
    TEST_BOOL(first != NULL);
    if (!first) return;

    memcpy(first, expected, sizeof(expected));

    void* second = jsl_arena_allocate(&arena, 16, false);
    TEST_BOOL(second != NULL);
    if (!second) return;

    void* moved = jsl_arena_reallocate(&arena, first, 32);
    TEST_BOOL(moved != NULL);
    if (!moved) return;

    TEST_BOOL(moved != first);
    TEST_BOOL(memcmp(moved, expected, sizeof(expected)) == 0);
}

static void test_arena_reallocate_invalid_pointer_returns_null(void)
{
    uint8_t buffer[128];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    uint8_t dummy[32];
    TEST_POINTERS_EQUAL(jsl_arena_reallocate(&arena, dummy, 8), NULL);
}

static void test_arena_reset_reuses_memory(void)
{
    uint8_t buffer[256];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    void* first = jsl_arena_allocate(&arena, 24, false);
    TEST_BOOL(first != NULL);
    if (!first) return;

    jsl_arena_reset(&arena);

    void* second = jsl_arena_allocate(&arena, 24, false);
    TEST_POINTERS_EQUAL(first, second);
}

static void test_arena_save_restore_point_rewinds(void)
{
    uint8_t buffer[256];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    void* first = jsl_arena_allocate(&arena, 16, false);
    TEST_BOOL(first != NULL);
    if (!first) return;

    uint8_t* restore = jsl_arena_save_restore_point(&arena);

    void* second = jsl_arena_allocate(&arena, 32, false);
    TEST_BOOL(second != NULL);
    if (!second) return;

    jsl_arena_load_restore_point(&arena, restore);

    void* third = jsl_arena_allocate(&arena, 32, false);
    TEST_POINTERS_EQUAL(third, second);
}

static void test_arena_allocator_interface_basic(void)
{
    uint8_t buffer[256];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    JSLAllocatorInterface allocator;

    jsl_arena_get_allocator_interface(&allocator, &arena);

    uint8_t* allocation = (uint8_t*) jsl_allocator_interface_alloc(&allocator, 32, 8, true);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    for (int64_t i = 0; i < 32; ++i)
    {
        TEST_BOOL(allocation[i] == 0);
    }

    TEST_BOOL(jsl_allocator_interface_free(&allocator, allocation));
    TEST_BOOL(jsl_allocator_interface_free_all(&allocator));

    void* second = jsl_allocator_interface_alloc(&allocator, 32, 8, false);
    TEST_POINTERS_EQUAL(second, allocation);
}

static void test_arena_typed_macros(void)
{
    uint8_t buffer[256];
    JSLArena arena = {0};
    jsl_arena_init(&arena, buffer, (int64_t) sizeof(buffer));

    TestStruct* value = JSL_ARENA_TYPED_ALLOCATE(TestStruct, &arena);
    TEST_BOOL(value != NULL);
    if (!value) return;

    TEST_BOOL(((uintptr_t) value % _Alignof(TestStruct)) == 0);

    TestStruct* array = JSL_ARENA_TYPED_ARRAY_ALLOCATE(TestStruct, &arena, 4);
    TEST_BOOL(array != NULL);
    if (!array) return;

    for (int32_t i = 0; i < 4; ++i)
    {
        TEST_UINT32_EQUAL(array[i].a, 0u);
        TEST_UINT32_EQUAL(array[i].b, 0u);
    }

    TestAlign16* aligned = JSL_ARENA_TYPED_ALLOCATE(TestAlign16, &arena);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    TEST_BOOL(((uintptr_t) aligned % _Alignof(TestAlign16)) == 0);
}

static void test_arena_from_stack_macro(void)
{
    uint8_t buffer[128];
    JSLArena arena = JSL_ARENA_FROM_STACK(buffer);

    TEST_POINTERS_EQUAL(arena.start, buffer);
    TEST_POINTERS_EQUAL(arena.current, buffer);
    TEST_POINTERS_EQUAL(arena.end, buffer + sizeof(buffer));

    void* allocation = jsl_arena_allocate(&arena, 16, true);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL((uint8_t*) allocation >= buffer);
    TEST_BOOL((uint8_t*) allocation < buffer + sizeof(buffer));

    ASAN_UNPOISON_MEMORY_REGION(buffer, sizeof(buffer));
}

static void test_infinite_arena_init(void)
{
    JSLInfiniteArena arena;
    arena.start = (void*) 1;
    arena.current = (void*) 2;
    arena.end = (void*) 3;

    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    TEST_BOOL(arena.start != (void*) 1);
    TEST_BOOL(arena.current != (void*) 2);
    TEST_BOOL(arena.end != (void*) 3);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_allocate_zeroed_and_alignment(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    uint8_t* allocation = (uint8_t*) jsl_infinite_arena_allocate(&arena, 32, true);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL(((uintptr_t) allocation % JSL_DEFAULT_ALLOCATION_ALIGNMENT) == 0);
    for (int64_t i = 0; i < 32; ++i)
    {
        TEST_BOOL(allocation[i] == 0);
    }

    void* aligned = jsl_infinite_arena_allocate_aligned(&arena, 16, 64, false);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    TEST_BOOL(((uintptr_t) aligned % 64) == 0);

    aligned = jsl_infinite_arena_allocate_aligned(&arena, 8, 256, false);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    TEST_BOOL(((uintptr_t) aligned % 256) == 0);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_allocate_invalid_sizes_return_null(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    TEST_POINTERS_EQUAL(jsl_infinite_arena_allocate(&arena, 0, false), NULL);
    TEST_POINTERS_EQUAL(jsl_infinite_arena_allocate(&arena, -5, false), NULL);
    TEST_POINTERS_EQUAL(jsl_infinite_arena_allocate_aligned(&arena, 0, 8, false), NULL);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_allocate_multiple_are_distinct(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    void* first = jsl_infinite_arena_allocate(&arena, 24, false);
    TEST_BOOL(first != NULL);
    if (!first) return;

    void* second = jsl_infinite_arena_allocate(&arena, 24, false);
    TEST_BOOL(second != NULL);
    if (!second) return;

    TEST_BOOL(first != second);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_reallocate_null_behaves_like_allocate(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    void* allocation = jsl_infinite_arena_reallocate(&arena, NULL, 24);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL(((uintptr_t) allocation % JSL_DEFAULT_ALLOCATION_ALIGNMENT) == 0);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_reallocate_aligned_null_behaves_like_allocate(void)
{
    JSLInfiniteArena arena = {0};
    jsl_infinite_arena_init(&arena);

    void* allocation = jsl_infinite_arena_reallocate_aligned(&arena, NULL, 24, 64);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL(((uintptr_t) allocation % 64) == 0);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_reallocate_in_place_when_last(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    uint8_t* allocation = (uint8_t*) jsl_infinite_arena_allocate(&arena, 16, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    for (uint8_t i = 0; i < 16; ++i)
    {
        allocation[i] = (uint8_t) (i + 1);
    }

    void* grown = jsl_infinite_arena_reallocate(&arena, allocation, 32);
    TEST_POINTERS_EQUAL(grown, allocation);
    for (uint8_t i = 0; i < 16; ++i)
    {
        TEST_BOOL(allocation[i] == (uint8_t) (i + 1));
    }

    void* shrunk = jsl_infinite_arena_reallocate(&arena, allocation, 8);
    TEST_POINTERS_EQUAL(shrunk, allocation);
    for (uint8_t i = 0; i < 8; ++i)
    {
        TEST_BOOL(allocation[i] == (uint8_t) (i + 1));
    }

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_reallocate_not_last_allocates_new(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    uint8_t expected[16];
    for (uint8_t i = 0; i < 16; ++i)
    {
        expected[i] = (uint8_t) (200 + i);
    }

    uint8_t* first = (uint8_t*) jsl_infinite_arena_allocate(&arena, 16, false);
    TEST_BOOL(first != NULL);
    if (!first) return;

    memcpy(first, expected, sizeof(expected));

    void* second = jsl_infinite_arena_allocate(&arena, 16, false);
    TEST_BOOL(second != NULL);
    if (!second) return;

    void* moved = jsl_infinite_arena_reallocate(&arena, first, 32);
    TEST_BOOL(moved != NULL);
    if (!moved) return;

    TEST_BOOL(moved != first);
    TEST_BOOL(memcmp(moved, expected, sizeof(expected)) == 0);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_reallocate_aligned_in_place_when_last_and_fits(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    uint8_t* allocation = (uint8_t*) jsl_infinite_arena_allocate_aligned(&arena, 16, 64, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL(((uintptr_t) allocation % 64) == 0);
    for (uint8_t i = 0; i < 16; ++i)
    {
        allocation[i] = (uint8_t) (50 + i);
    }

    uint8_t* grown = (uint8_t*) jsl_infinite_arena_reallocate_aligned(&arena, allocation, 32, 64);
    TEST_POINTERS_EQUAL(grown, allocation);
    TEST_BOOL(((uintptr_t) grown % 64) == 0);
    for (uint8_t i = 0; i < 16; ++i)
    {
        TEST_BOOL(grown[i] == (uint8_t) (50 + i));
    }

    void* shrunk = jsl_infinite_arena_reallocate_aligned(&arena, allocation, 8, 64);
    TEST_POINTERS_EQUAL(shrunk, allocation);
    for (uint8_t i = 0; i < 8; ++i)
    {
        TEST_BOOL(allocation[i] == (uint8_t) (50 + i));
    }

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_reallocate_aligned_not_last_allocates_new(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    uint8_t expected[16];
    for (uint8_t i = 0; i < 16; ++i)
    {
        expected[i] = (uint8_t) (120 + i);
    }

    uint8_t* first = (uint8_t*) jsl_infinite_arena_allocate_aligned(&arena, 16, 64, false);
    TEST_BOOL(first != NULL);
    if (!first) return;

    memcpy(first, expected, sizeof(expected));

    void* second = jsl_infinite_arena_allocate_aligned(&arena, 16, 64, false);
    TEST_BOOL(second != NULL);
    if (!second) return;

    void* moved = jsl_infinite_arena_reallocate_aligned(&arena, first, 32, 64);
    TEST_BOOL(moved != NULL);
    if (!moved) return;

    TEST_BOOL(moved != first);
    TEST_BOOL(((uintptr_t) moved % 64) == 0);
    TEST_BOOL(memcmp(moved, expected, sizeof(expected)) == 0);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_reallocate_aligned_alignment_mismatch_allocates_new(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    uint8_t expected[16];
    for (uint8_t i = 0; i < 16; ++i)
    {
        expected[i] = (uint8_t) (90 + i);
    }

    uint8_t* allocation = (uint8_t*) jsl_infinite_arena_allocate_aligned(&arena, 16, 16, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL(((uintptr_t) allocation % 64) != 0);
    memcpy(allocation, expected, sizeof(expected));

    void* moved = jsl_infinite_arena_reallocate_aligned(&arena, allocation, 32, 64);
    TEST_BOOL(moved != NULL);
    if (!moved) return;

    TEST_BOOL(moved != allocation);
    TEST_BOOL(((uintptr_t) moved % 64) == 0);
    TEST_BOOL(memcmp(moved, expected, sizeof(expected)) == 0);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_reset_reuses_memory(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    void* first = jsl_infinite_arena_allocate(&arena, 24, false);
    TEST_BOOL(first != NULL);
    if (!first) return;

    jsl_infinite_arena_reset(&arena);

    void* second = jsl_infinite_arena_allocate(&arena, 24, false);
    TEST_POINTERS_EQUAL(first, second);

    jsl_infinite_arena_release(&arena);
}

static void test_infinite_arena_allocator_interface_basic(void)
{
    JSLInfiniteArena arena = {0};
    bool init = jsl_infinite_arena_init(&arena);
    TEST_BOOL(init == true);
    if (!init) return;

    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &arena);

    uint8_t* allocation = (uint8_t*) jsl_allocator_interface_alloc(&allocator, 32, 8, true);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    for (int64_t i = 0; i < 32; ++i)
    {
        TEST_BOOL(allocation[i] == 0);
    }

    TEST_BOOL(jsl_allocator_interface_free(&allocator, allocation));
    TEST_BOOL(jsl_allocator_interface_free_all(&allocator));

    void* second = jsl_allocator_interface_alloc(&allocator, 32, 8, false);
    TEST_BOOL(second != NULL);

    jsl_infinite_arena_release(&arena);
}

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    RUN_TEST_FUNCTION("Test arena init sets pointers", test_arena_init_sets_pointers);
    RUN_TEST_FUNCTION("Test arena init2 sets pointers", test_arena_init2_sets_pointers);
    RUN_TEST_FUNCTION("Test arena allocate zeroed and alignment", test_arena_allocate_zeroed_and_alignment);
    RUN_TEST_FUNCTION("Test arena allocate invalid sizes", test_arena_allocate_invalid_sizes_return_null);
    RUN_TEST_FUNCTION("Test arena allocate out of memory", test_arena_allocate_out_of_memory_returns_null);
    RUN_TEST_FUNCTION("Test arena realloc null behaves like alloc", test_arena_reallocate_null_behaves_like_allocate);
    RUN_TEST_FUNCTION("Test arena realloc in place", test_arena_reallocate_in_place_when_last);
    RUN_TEST_FUNCTION("Test arena realloc not last", test_arena_reallocate_not_last_allocates_new);
    RUN_TEST_FUNCTION("Test arena realloc invalid pointer", test_arena_reallocate_invalid_pointer_returns_null);
    RUN_TEST_FUNCTION("Test arena reset reuses memory", test_arena_reset_reuses_memory);
    RUN_TEST_FUNCTION("Test arena save/restore point", test_arena_save_restore_point_rewinds);
    RUN_TEST_FUNCTION("Test arena allocator interface", test_arena_allocator_interface_basic);
    RUN_TEST_FUNCTION("Test arena typed macros", test_arena_typed_macros);
    RUN_TEST_FUNCTION("Test arena from stack macro", test_arena_from_stack_macro);

    RUN_TEST_FUNCTION("Test infinite arena init sets pointers", test_infinite_arena_init);
    RUN_TEST_FUNCTION("Test infinite arena allocate zeroed and alignment", test_infinite_arena_allocate_zeroed_and_alignment);
    RUN_TEST_FUNCTION("Test infinite arena allocate invalid sizes", test_infinite_arena_allocate_invalid_sizes_return_null);
    RUN_TEST_FUNCTION("Test infinite arena allocate distinct blocks", test_infinite_arena_allocate_multiple_are_distinct);
    RUN_TEST_FUNCTION("Test infinite arena realloc null behaves like alloc", test_infinite_arena_reallocate_null_behaves_like_allocate);
    RUN_TEST_FUNCTION("Test infinite arena realloc aligned null behaves like alloc", test_infinite_arena_reallocate_aligned_null_behaves_like_allocate);
    RUN_TEST_FUNCTION("Test infinite arena realloc in place", test_infinite_arena_reallocate_in_place_when_last);
    RUN_TEST_FUNCTION("Test infinite arena realloc not last", test_infinite_arena_reallocate_not_last_allocates_new);
    RUN_TEST_FUNCTION("Test infinite arena realloc aligned in place", test_infinite_arena_reallocate_aligned_in_place_when_last_and_fits);
    RUN_TEST_FUNCTION("Test infinite arena realloc aligned not last", test_infinite_arena_reallocate_aligned_not_last_allocates_new);
    RUN_TEST_FUNCTION("Test infinite arena realloc aligned mismatch", test_infinite_arena_reallocate_aligned_alignment_mismatch_allocates_new);
    RUN_TEST_FUNCTION("Test infinite arena reset reuses memory", test_infinite_arena_reset_reuses_memory);
    RUN_TEST_FUNCTION("Test infinite arena allocator interface", test_infinite_arena_allocator_interface_basic);

    TEST_RESULTS();
    return lfails != 0;
}

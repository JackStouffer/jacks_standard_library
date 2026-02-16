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

#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/jsl_core.h"
#include "../src/jsl_allocator.h"
#include "../src/jsl_allocator_pool.h"

#include "minctest.h"

static void test_pool_init_sets_counts_and_lists(void)
{
    uint8_t buffer[512];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 32);

    int64_t total = jsl_pool_total_allocation_count(&pool);
    int64_t free_count = jsl_pool_free_allocation_count(&pool);

    TEST_BOOL(total > 0);
    TEST_INT64_EQUAL(free_count, total);
    TEST_INT64_EQUAL(pool.chunk_count, total);
    TEST_POINTERS_EQUAL(pool.checked_out, NULL);
    if (total > 0)
    {
        TEST_BOOL(pool.free_list != NULL);
    }
}

static void test_pool_init2_sets_counts(void)
{
    uint8_t buffer[256];
    JSLPoolAllocator pool = {0};
    JSLImmutableMemory memory = JSL_MEMORY_FROM_STACK(buffer);

    jsl_pool_init2(&pool, memory, 24);

    int64_t total = jsl_pool_total_allocation_count(&pool);
    int64_t free_count = jsl_pool_free_allocation_count(&pool);

    TEST_BOOL(total > 0);
    TEST_INT64_EQUAL(free_count, total);
    TEST_INT64_EQUAL(pool.chunk_count, total);
}

static void test_pool_init_too_small_has_no_allocations(void)
{
    uint8_t buffer[48];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 64);

    TEST_INT64_EQUAL(jsl_pool_total_allocation_count(&pool), 0);
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), 0);
    TEST_POINTERS_EQUAL(jsl_pool_allocate(&pool, false), NULL);
}

static void test_pool_allocate_zeroed_and_alignment_small(void)
{
    uint8_t buffer[512];
    JSLPoolAllocator pool = {0};
    const int64_t allocation_size = 32;

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), allocation_size);

    uint8_t* allocation = (uint8_t*) jsl_pool_allocate(&pool, true);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    TEST_BOOL(((uintptr_t) allocation % 8) == 0);
    for (int64_t i = 0; i < allocation_size; ++i)
    {
        TEST_BOOL(allocation[i] == 0);
    }

    TEST_BOOL(jsl_pool_free(&pool, allocation));
}

static void test_pool_allocate_exhaustion_updates_counts(void)
{
    uint8_t buffer[1024];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 32);

    int64_t total = jsl_pool_total_allocation_count(&pool);
    TEST_BOOL(total > 0);
    if (total <= 0) return;

    void** allocations = (void**) malloc((size_t) total * sizeof(void*));
    TEST_BOOL(allocations != NULL);
    if (!allocations) return;

    for (int64_t i = 0; i < total; ++i)
    {
        allocations[i] = jsl_pool_allocate(&pool, false);
        TEST_BOOL(allocations[i] != NULL);
        if (!allocations[i]) break;

        for (int64_t j = 0; j < i; ++j)
        {
            TEST_BOOL(allocations[i] != allocations[j]);
        }

        TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), total - i - 1);
    }

    TEST_POINTERS_EQUAL(jsl_pool_allocate(&pool, false), NULL);
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), 0);

    jsl_pool_free_all(&pool);
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), total);

    free(allocations);
}

static void test_pool_free_invalid_and_double_free(void)
{
    uint8_t buffer[512];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 32);

    int64_t total = jsl_pool_total_allocation_count(&pool);
    TEST_BOOL(total > 0);
    if (total <= 0) return;

    void* allocation = jsl_pool_allocate(&pool, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    int64_t free_after_alloc = jsl_pool_free_allocation_count(&pool);
    uint8_t dummy = 0;

    TEST_BOOL(!jsl_pool_free(&pool, &dummy));
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), free_after_alloc);

    TEST_BOOL(jsl_pool_free(&pool, allocation));
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), total);

    TEST_BOOL(!jsl_pool_free(&pool, allocation));
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), total);
}

static void test_pool_alignment_medium_alloc(void)
{
    uint8_t buffer[2048];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 128);

    void* allocation = jsl_pool_allocate(&pool, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    #if JSL_IS_WEB_ASSEMBLY
        TEST_BOOL(((uintptr_t) allocation % 8) == 0);
    #else
        TEST_BOOL(((uintptr_t) allocation % 64) == 0);
    #endif
}

static void test_pool_alignment_large_alloc(void)
{
    uint8_t raw_buffer[32768];
    const int32_t page_alignment = (int32_t) JSL_KILOBYTES(4);
    uint8_t* aligned = (uint8_t*) jsl_align_ptr_upwards(raw_buffer, page_alignment);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    int64_t aligned_length = (int64_t) (raw_buffer + sizeof(raw_buffer) - aligned);
    JSLImmutableMemory memory = {aligned, aligned_length};
    JSLPoolAllocator pool = {0};

    jsl_pool_init2(&pool, memory, JSL_KILOBYTES(2));

    int64_t total = jsl_pool_total_allocation_count(&pool);
    TEST_BOOL(total > 0);
    if (total <= 0) return;

    void* allocation = jsl_pool_allocate(&pool, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    #if JSL_IS_WEB_ASSEMBLY
        TEST_BOOL(((uintptr_t) allocation % 8) == 0);
    #else
        TEST_BOOL(((uintptr_t) allocation % (uintptr_t) page_alignment) == 0);
    #endif
}

static void test_pool_free_middle_node(void)
{
    uint8_t buffer[512];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 32);

    void* a = jsl_pool_allocate(&pool, false);
    void* b = jsl_pool_allocate(&pool, false);
    void* c = jsl_pool_allocate(&pool, false);
    TEST_BOOL(a != NULL);
    TEST_BOOL(b != NULL);
    TEST_BOOL(c != NULL);
    if (!a || !b || !c) return;

    int64_t total = jsl_pool_total_allocation_count(&pool);
    TEST_BOOL(jsl_pool_free(&pool, b));
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), total - 2);

    TEST_BOOL(jsl_pool_free(&pool, a));
    TEST_BOOL(jsl_pool_free(&pool, c));
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), total);
}

static void test_pool_free_interior_pointer(void)
{
    uint8_t buffer[512];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 32);

    uint8_t* allocation = (uint8_t*) jsl_pool_allocate(&pool, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    int64_t free_after_alloc = jsl_pool_free_allocation_count(&pool);
    TEST_BOOL(!jsl_pool_free(&pool, allocation + 1));
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), free_after_alloc);

    TEST_BOOL(jsl_pool_free(&pool, allocation));
}

static void test_pool_free_wrong_pool(void)
{
    uint8_t buffer_a[512];
    uint8_t buffer_b[512];
    JSLPoolAllocator pool_a = {0};
    JSLPoolAllocator pool_b = {0};

    jsl_pool_init(&pool_a, buffer_a, (int64_t) sizeof(buffer_a), 32);
    jsl_pool_init(&pool_b, buffer_b, (int64_t) sizeof(buffer_b), 32);

    void* allocation = jsl_pool_allocate(&pool_a, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    int64_t free_before = jsl_pool_free_allocation_count(&pool_b);
    TEST_BOOL(!jsl_pool_free(&pool_b, allocation));
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool_b), free_before);

    TEST_BOOL(jsl_pool_free(&pool_a, allocation));
}

static void test_pool_free_after_free_all(void)
{
    uint8_t buffer[512];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 32);

    void* allocation = jsl_pool_allocate(&pool, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    int64_t total = jsl_pool_total_allocation_count(&pool);
    jsl_pool_free_all(&pool);
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), total);
    TEST_BOOL(!jsl_pool_free(&pool, allocation));
}

static void test_pool_free_sentinel_corruption(void)
{
    uint8_t buffer[512];
    JSLPoolAllocator pool = {0};

    jsl_pool_init(&pool, buffer, (int64_t) sizeof(buffer), 32);

    void* allocation = jsl_pool_allocate(&pool, false);
    TEST_BOOL(allocation != NULL);
    if (!allocation) return;

    int64_t free_after_alloc = jsl_pool_free_allocation_count(&pool);
    struct JSL__PoolAllocatorHeader* header = (struct JSL__PoolAllocatorHeader*) (
        (uint8_t*) allocation - sizeof(struct JSL__PoolAllocatorHeader)
    );
    uint64_t old_sentinel = header->sentinel;
    header->sentinel = 0;
    TEST_BOOL(!jsl_pool_free(&pool, allocation));
    TEST_INT64_EQUAL(jsl_pool_free_allocation_count(&pool), free_after_alloc);

    header->sentinel = old_sentinel;
    TEST_BOOL(jsl_pool_free(&pool, allocation));
}

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    RUN_TEST_FUNCTION("Test pool init sets counts", test_pool_init_sets_counts_and_lists);
    RUN_TEST_FUNCTION("Test pool init2 sets counts", test_pool_init2_sets_counts);
    RUN_TEST_FUNCTION("Test pool init too small", test_pool_init_too_small_has_no_allocations);
    RUN_TEST_FUNCTION("Test pool allocate zeroed", test_pool_allocate_zeroed_and_alignment_small);
    RUN_TEST_FUNCTION("Test pool allocate exhaustion", test_pool_allocate_exhaustion_updates_counts);
    RUN_TEST_FUNCTION("Test pool free invalid/double", test_pool_free_invalid_and_double_free);
    RUN_TEST_FUNCTION("Test pool medium alignment", test_pool_alignment_medium_alloc);
    RUN_TEST_FUNCTION("Test pool large alignment", test_pool_alignment_large_alloc);
    RUN_TEST_FUNCTION("Test pool free middle node", test_pool_free_middle_node);
    RUN_TEST_FUNCTION("Test pool free interior pointer", test_pool_free_interior_pointer);
    RUN_TEST_FUNCTION("Test pool free wrong pool", test_pool_free_wrong_pool);
    RUN_TEST_FUNCTION("Test pool free after free all", test_pool_free_after_free_all);
    RUN_TEST_FUNCTION("Test pool free sentinel corruption", test_pool_free_sentinel_corruption);

    TEST_RESULTS();
    return lfails != 0;
}

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

#include "jsl/core.h"
#include "jsl/allocator.h"
#include "jsl/allocator_libc.h"

#include "minctest.h"
#include "test_allocator_libc.h"

typedef struct TestStruct
{
    uint32_t a;
    uint32_t b;
} TestStruct;

typedef struct TestAlign16
{
    _Alignas(16) uint8_t data[16];
} TestAlign16;

void test_libc_init(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    TEST_POINTERS_EQUAL(allocator.head, NULL);
}

void test_libc_init_null(void)
{
    jsl_libc_allocator_init(NULL);
}

void test_libc_allocate_basic(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    void* ptr = jsl_libc_allocator_allocate(&allocator, 64, false);
    TEST_BOOL(ptr != NULL);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_allocate_zeroed(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    uint8_t* ptr = (uint8_t*) jsl_libc_allocator_allocate(&allocator, 32, true);
    TEST_BOOL(ptr != NULL);
    if (!ptr) return;

    for (int64_t i = 0; i < 32; ++i)
    {
        TEST_BOOL(ptr[i] == 0);
    }

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_allocate_alignment(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    void* ptr = jsl_libc_allocator_allocate(&allocator, 32, false);
    TEST_BOOL(ptr != NULL);
    if (!ptr) return;

    TEST_BOOL(((uintptr_t) ptr % JSL_DEFAULT_ALLOCATION_ALIGNMENT) == 0);

    void* aligned = jsl_libc_allocator_allocate_aligned(&allocator, 16, 64, false);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    TEST_BOOL(((uintptr_t) aligned % 64) == 0);

    aligned = jsl_libc_allocator_allocate_aligned(&allocator, 8, 256, false);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    TEST_BOOL(((uintptr_t) aligned % 256) == 0);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_allocate_invalid_sizes_return_null(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    TEST_POINTERS_EQUAL(jsl_libc_allocator_allocate(&allocator, 0, false), NULL);
    TEST_POINTERS_EQUAL(jsl_libc_allocator_allocate(&allocator, -5, false), NULL);
    TEST_POINTERS_EQUAL(jsl_libc_allocator_allocate_aligned(&allocator, 0, 8, false), NULL);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_allocate_multiple_are_distinct(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    void* first = jsl_libc_allocator_allocate(&allocator, 24, false);
    TEST_BOOL(first != NULL);
    if (!first) return;

    void* second = jsl_libc_allocator_allocate(&allocator, 24, false);
    TEST_BOOL(second != NULL);
    if (!second) return;

    TEST_BOOL(first != second);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_free_single(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    void* first = jsl_libc_allocator_allocate(&allocator, 32, false);
    TEST_BOOL(first != NULL);

    void* second = jsl_libc_allocator_allocate(&allocator, 32, false);
    TEST_BOOL(second != NULL);

    TEST_BOOL(jsl_libc_allocator_free(&allocator, first));
    TEST_BOOL(jsl_libc_allocator_free(&allocator, second));
}

void test_libc_free_null_returns_false(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    TEST_BOOL(jsl_libc_allocator_free(&allocator, NULL) == false);
}

void test_libc_free_all(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    jsl_libc_allocator_allocate(&allocator, 32, false);
    jsl_libc_allocator_allocate(&allocator, 64, false);
    jsl_libc_allocator_allocate(&allocator, 128, false);

    TEST_BOOL(jsl_libc_allocator_free_all(&allocator));
    TEST_POINTERS_EQUAL(allocator.head, NULL);
}

void test_libc_free_all_null_returns_false(void)
{
    TEST_BOOL(jsl_libc_allocator_free_all(NULL) == false);
}

void test_libc_free_all_uninitialized_returns_false(void)
{
    JSLLibcAllocator allocator = {0};
    TEST_BOOL(jsl_libc_allocator_free_all(&allocator) == false);
}

void test_libc_reallocate_null_behaves_like_allocate(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    void* ptr = jsl_libc_allocator_reallocate(&allocator, NULL, 24);
    TEST_BOOL(ptr != NULL);
    if (!ptr) return;

    TEST_BOOL(((uintptr_t) ptr % JSL_DEFAULT_ALLOCATION_ALIGNMENT) == 0);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_reallocate_preserves_data(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    uint8_t expected[16];
    for (uint8_t i = 0; i < 16; ++i)
    {
        expected[i] = (uint8_t) (200 + i);
    }

    uint8_t* ptr = (uint8_t*) jsl_libc_allocator_allocate(&allocator, 16, false);
    TEST_BOOL(ptr != NULL);
    if (!ptr) return;

    memcpy(ptr, expected, sizeof(expected));

    uint8_t* grown = (uint8_t*) jsl_libc_allocator_reallocate(&allocator, ptr, 64);
    TEST_BOOL(grown != NULL);
    if (!grown) return;

    TEST_BOOL(memcmp(grown, expected, sizeof(expected)) == 0);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_reallocate_shrink_preserves_data(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    uint8_t expected[8];
    for (uint8_t i = 0; i < 8; ++i)
    {
        expected[i] = (uint8_t) (100 + i);
    }

    uint8_t* ptr = (uint8_t*) jsl_libc_allocator_allocate(&allocator, 32, false);
    TEST_BOOL(ptr != NULL);
    if (!ptr) return;

    memcpy(ptr, expected, sizeof(expected));

    uint8_t* shrunk = (uint8_t*) jsl_libc_allocator_reallocate(&allocator, ptr, 8);
    TEST_BOOL(shrunk != NULL);
    if (!shrunk) return;

    TEST_BOOL(memcmp(shrunk, expected, sizeof(expected)) == 0);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_reallocate_aligned_preserves_alignment(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    uint8_t* ptr = (uint8_t*) jsl_libc_allocator_allocate_aligned(&allocator, 16, 64, false);
    TEST_BOOL(ptr != NULL);
    if (!ptr) return;

    TEST_BOOL(((uintptr_t) ptr % 64) == 0);

    for (uint8_t i = 0; i < 16; ++i)
    {
        ptr[i] = (uint8_t) (50 + i);
    }

    uint8_t* grown = (uint8_t*) jsl_libc_allocator_reallocate_aligned(&allocator, ptr, 64, 64);
    TEST_BOOL(grown != NULL);
    if (!grown) return;

    TEST_BOOL(((uintptr_t) grown % 64) == 0);
    for (uint8_t i = 0; i < 16; ++i)
    {
        TEST_BOOL(grown[i] == (uint8_t) (50 + i));
    }

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_reallocate_invalid_size_returns_null(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    void* ptr = jsl_libc_allocator_allocate(&allocator, 32, false);
    TEST_BOOL(ptr != NULL);

    TEST_POINTERS_EQUAL(jsl_libc_allocator_reallocate(&allocator, ptr, 0), NULL);
    TEST_POINTERS_EQUAL(jsl_libc_allocator_reallocate(&allocator, ptr, -1), NULL);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_typed_allocate_macro(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    TestStruct* value = JSL_LIBC_ALLOCATOR_TYPED_ALLOCATE(TestStruct, &allocator);
    TEST_BOOL(value != NULL);
    if (!value) return;

    TEST_BOOL(((uintptr_t) value % _Alignof(TestStruct)) == 0);

    value->a = 42;
    value->b = 99;
    TEST_UINT32_EQUAL(value->a, 42u);
    TEST_UINT32_EQUAL(value->b, 99u);

    TestAlign16* aligned = JSL_LIBC_ALLOCATOR_TYPED_ALLOCATE(TestAlign16, &allocator);
    TEST_BOOL(aligned != NULL);
    if (!aligned) return;

    TEST_BOOL(((uintptr_t) aligned % _Alignof(TestAlign16)) == 0);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_allocator_interface_basic(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    JSLAllocatorInterface interface;
    jsl_libc_allocator_get_allocator_interface(&interface, &allocator);

    uint8_t* ptr = (uint8_t*) jsl_allocator_interface_alloc(interface, 32, 8, true);
    TEST_BOOL(ptr != NULL);
    if (!ptr) return;

    for (int64_t i = 0; i < 32; ++i)
    {
        TEST_BOOL(ptr[i] == 0);
    }

    TEST_BOOL(jsl_allocator_interface_free(interface, ptr));
    TEST_BOOL(jsl_allocator_interface_free_all(interface));
}

void test_libc_allocator_interface_realloc(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    JSLAllocatorInterface interface;
    jsl_libc_allocator_get_allocator_interface(&interface, &allocator);

    uint8_t* ptr = (uint8_t*) jsl_allocator_interface_alloc(interface, 16, 8, false);
    TEST_BOOL(ptr != NULL);
    if (!ptr) return;

    for (uint8_t i = 0; i < 16; ++i)
    {
        ptr[i] = (uint8_t) (i + 1);
    }

    uint8_t* grown = (uint8_t*) jsl_allocator_interface_realloc(interface, ptr, 64, 8);
    TEST_BOOL(grown != NULL);
    if (!grown) return;

    for (uint8_t i = 0; i < 16; ++i)
    {
        TEST_BOOL(grown[i] == (uint8_t) (i + 1));
    }

    jsl_allocator_interface_free_all(interface);
}

void test_libc_create_child_basic(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    JSLAllocatorInterface interface;
    jsl_libc_allocator_get_allocator_interface(&interface, &allocator);

    uint8_t* parent_alloc = (uint8_t*) jsl_allocator_interface_alloc(interface, 32, 8, true);
    TEST_BOOL(parent_alloc != NULL);
    if (!parent_alloc) return;

    for (uint8_t i = 0; i < 32; ++i)
    {
        parent_alloc[i] = (uint8_t) (i + 1);
    }

    JSLAllocatorInterface scratch;
    TEST_BOOL(jsl_allocator_interface_create_child(interface, &scratch));

    void* scratch1 = jsl_allocator_interface_alloc(scratch, 64, 8, false);
    TEST_BOOL(scratch1 != NULL);

    void* scratch2 = jsl_allocator_interface_alloc(scratch, 64, 8, false);
    TEST_BOOL(scratch2 != NULL);

    jsl_allocator_interface_free_all(scratch);

    for (uint8_t i = 0; i < 32; ++i)
    {
        TEST_BOOL(parent_alloc[i] == (uint8_t) (i + 1));
    }

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_create_child_parent_survives_realloc(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    JSLAllocatorInterface interface;
    jsl_libc_allocator_get_allocator_interface(&interface, &allocator);

    uint8_t* parent_buf = (uint8_t*) jsl_allocator_interface_alloc(interface, 16, 8, false);
    TEST_BOOL(parent_buf != NULL);
    if (!parent_buf) return;

    for (uint8_t i = 0; i < 16; ++i)
    {
        parent_buf[i] = (uint8_t) (100 + i);
    }

    JSLAllocatorInterface scratch;
    TEST_BOOL(jsl_allocator_interface_create_child(interface, &scratch));

    void* scratch_alloc = jsl_allocator_interface_alloc(scratch, 64, 8, false);
    TEST_BOOL(scratch_alloc != NULL);

    uint8_t* grown = (uint8_t*) jsl_allocator_interface_realloc(interface, parent_buf, 64, 8);
    TEST_BOOL(grown != NULL);
    if (!grown) return;

    for (uint8_t i = 0; i < 16; ++i)
    {
        TEST_BOOL(grown[i] == (uint8_t) (100 + i));
    }

    jsl_allocator_interface_free_all(scratch);

    for (uint8_t i = 0; i < 16; ++i)
    {
        TEST_BOOL(grown[i] == (uint8_t) (100 + i));
    }

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_create_child_nested(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    JSLAllocatorInterface interface;
    jsl_libc_allocator_get_allocator_interface(&interface, &allocator);

    JSLAllocatorInterface child1;
    TEST_BOOL(jsl_allocator_interface_create_child(interface, &child1));

    void* a = jsl_allocator_interface_alloc(child1, 32, 8, false);
    TEST_BOOL(a != NULL);

    JSLAllocatorInterface child2;
    TEST_BOOL(jsl_allocator_interface_create_child(child1, &child2));

    void* b = jsl_allocator_interface_alloc(child2, 32, 8, false);
    TEST_BOOL(b != NULL);

    jsl_allocator_interface_free_all(child2);
    jsl_allocator_interface_free_all(child1);

    jsl_libc_allocator_free_all(&allocator);
}

void test_libc_free_middle_of_list(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    void* first = jsl_libc_allocator_allocate(&allocator, 32, false);
    TEST_BOOL(first != NULL);

    void* second = jsl_libc_allocator_allocate(&allocator, 32, false);
    TEST_BOOL(second != NULL);

    void* third = jsl_libc_allocator_allocate(&allocator, 32, false);
    TEST_BOOL(third != NULL);

    TEST_BOOL(jsl_libc_allocator_free(&allocator, second));
    TEST_BOOL(jsl_libc_allocator_free(&allocator, first));
    TEST_BOOL(jsl_libc_allocator_free(&allocator, third));

    TEST_POINTERS_EQUAL(allocator.head, NULL);
}

void test_libc_free_all_then_reuse(void)
{
    JSLLibcAllocator allocator;
    jsl_libc_allocator_init(&allocator);

    jsl_libc_allocator_allocate(&allocator, 32, false);
    jsl_libc_allocator_allocate(&allocator, 64, false);

    TEST_BOOL(jsl_libc_allocator_free_all(&allocator));
    TEST_POINTERS_EQUAL(allocator.head, NULL);

    void* after = jsl_libc_allocator_allocate(&allocator, 16, false);
    TEST_BOOL(after != NULL);

    jsl_libc_allocator_free_all(&allocator);
}


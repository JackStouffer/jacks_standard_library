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
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/jsl_core.h"
#include "../src/jsl_allocator.h"
#include "../src/jsl_allocator_arena.h"
#include "../src/jsl_str_to_str_map.h"

#include "minctest.h"
#include "test_hash_map_types.h"
#include "arrays/dynamic_int32_array.h"
#include "arrays/dynamic_comp1_array.h"
#include "arrays/dynamic_comp2_array.h"
#include "arrays/dynamic_comp3_array.h"

const int64_t arena_size = JSL_MEGABYTES(32);
JSLArena global_arena;

static CompositeType1 make_comp1(int32_t a, int32_t b)
{
    CompositeType1 value = {0};
    value.a = a;
    value.b = b;
    return value;
}

static CompositeType2 make_comp2(int32_t a, int32_t b, bool c)
{
    CompositeType2 value = {0};
    value.a = a;
    value.b = b;
    value.c = c;
    return value;
}

static CompositeType3 make_comp3(int64_t a, int64_t b, int64_t c, int64_t d, int64_t e, int64_t f, int64_t g)
{
    CompositeType3 value = {0};
    value.a = a;
    value.b = b;
    value.c = c;
    value.d = d;
    value.e = e;
    value.f = f;
    value.g = g;
    return value;
}

static bool comp1_equal(const CompositeType1* lhs, const CompositeType1* rhs)
{
    return memcmp(lhs, rhs, sizeof(CompositeType1)) == 0;
}

static bool comp2_equal(const CompositeType2* lhs, const CompositeType2* rhs)
{
    return memcmp(lhs, rhs, sizeof(CompositeType2)) == 0;
}

static bool comp3_equal(const CompositeType3* lhs, const CompositeType3* rhs)
{
    return memcmp(lhs, rhs, sizeof(CompositeType3)) == 0;
}

static void test_dynamic_array_init_success(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicInt32Array array = {0};
        const int64_t initial_capacity = 10;
        bool ok = dynamic_int32_array_init(&array, &allocator, initial_capacity);

        TEST_BOOL(ok);
        if (!ok) return;

        TEST_POINTERS_EQUAL(array.allocator, &allocator);
        TEST_UINT64_EQUAL(array.sentinel, PRIVATE_SENTINEL_DynamicInt32Array);
        TEST_INT64_EQUAL(array.length, (int64_t) 0);
        TEST_BOOL(array.data != NULL);
        TEST_INT64_EQUAL(array.capacity, jsl_next_power_of_two_i64(JSL_MAX(32L, initial_capacity)));
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType1Map array = {0};
        const int64_t initial_capacity = 64;
        bool ok = dynamic_comp1_array_init(&array, &allocator, initial_capacity);

        TEST_BOOL(ok);
        if (!ok) return;

        TEST_POINTERS_EQUAL(array.allocator, &allocator);
        TEST_UINT64_EQUAL(array.sentinel, PRIVATE_SENTINEL_DynamicCompositeType1Map);
        TEST_INT64_EQUAL(array.length, (int64_t) 0);
        TEST_BOOL(array.data != NULL);
        TEST_INT64_EQUAL(array.capacity, jsl_next_power_of_two_i64(JSL_MAX(32L, initial_capacity)));
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType2ToIntMap array = {0};
        const int64_t initial_capacity = 1;
        bool ok = dynamic_comp2_array_init(&array, &allocator, initial_capacity);

        TEST_BOOL(ok);
        if (!ok) return;

        TEST_POINTERS_EQUAL(array.allocator, &allocator);
        TEST_UINT64_EQUAL(array.sentinel, PRIVATE_SENTINEL_DynamicCompositeType2ToIntMap);
        TEST_INT64_EQUAL(array.length, (int64_t) 0);
        TEST_BOOL(array.data != NULL);
        TEST_INT64_EQUAL(array.capacity, jsl_next_power_of_two_i64(JSL_MAX(32L, initial_capacity)));
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType3ToCompositeType2Map array = {0};
        const int64_t initial_capacity = 128;
        bool ok = dynamic_comp3_array_init(&array, &allocator, initial_capacity);

        TEST_BOOL(ok);
        if (!ok) return;

        TEST_POINTERS_EQUAL(array.allocator, &allocator);
        TEST_UINT64_EQUAL(array.sentinel, PRIVATE_SENTINEL_DynamicCompositeType3ToCompositeType2Map);
        TEST_INT64_EQUAL(array.length, (int64_t) 0);
        TEST_BOOL(array.data != NULL);
        TEST_INT64_EQUAL(array.capacity, jsl_next_power_of_two_i64(JSL_MAX(32L, initial_capacity)));
    }

    jsl_allocator_interface_free_all(&allocator);
}

static void test_dynamic_array_init_invalid_args(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicInt32Array array = {0};
        TEST_BOOL(!dynamic_int32_array_init(NULL, &allocator, 8));
        TEST_BOOL(!dynamic_int32_array_init(&array, NULL, 8));
        TEST_BOOL(!dynamic_int32_array_init(&array, &allocator, -1));
    }

    {
        DynamicCompositeType1Map array = {0};
        TEST_BOOL(!dynamic_comp1_array_init(NULL, &allocator, 8));
        TEST_BOOL(!dynamic_comp1_array_init(&array, NULL, 8));
        TEST_BOOL(!dynamic_comp1_array_init(&array, &allocator, -1));
    }

    {
        DynamicCompositeType2ToIntMap array = {0};
        TEST_BOOL(!dynamic_comp2_array_init(NULL, &allocator, 8));
        TEST_BOOL(!dynamic_comp2_array_init(&array, NULL, 8));
        TEST_BOOL(!dynamic_comp2_array_init(&array, &allocator, -1));
    }

    {
        DynamicCompositeType3ToCompositeType2Map array = {0};
        TEST_BOOL(!dynamic_comp3_array_init(NULL, &allocator, 8));
        TEST_BOOL(!dynamic_comp3_array_init(&array, NULL, 8));
        TEST_BOOL(!dynamic_comp3_array_init(&array, &allocator, -1));
    }
}

static void test_dynamic_array_insert_appends_and_grows(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicInt32Array array = {0};
        bool ok = dynamic_int32_array_init(&array, &allocator, 1);
        TEST_BOOL(ok);
        if (!ok) return;

        for (int32_t i = 0; i < 50; ++i)
        {
            TEST_BOOL(dynamic_int32_array_insert(&array, i));
        }

        TEST_INT64_EQUAL(array.length, (int64_t) 50);
        TEST_BOOL(array.capacity >= array.length);
        for (int32_t i = 0; i < 50; ++i)
        {
            TEST_INT32_EQUAL(array.data[i], i);
        }
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType1Map array = {0};
        bool ok = dynamic_comp1_array_init(&array, &allocator, 2);
        TEST_BOOL(ok);
        if (!ok) return;

        for (int32_t i = 0; i < 40; ++i)
        {
            CompositeType1 value = make_comp1(i, i * 10);
            TEST_BOOL(dynamic_comp1_array_insert(&array, value));
        }

        TEST_INT64_EQUAL(array.length, (int64_t) 40);
        TEST_BOOL(array.capacity >= array.length);
        for (int32_t i = 0; i < 40; ++i)
        {
            CompositeType1 expected = make_comp1(i, i * 10);
            TEST_BOOL(comp1_equal(&array.data[i], &expected));
        }
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType2ToIntMap array = {0};
        bool ok = dynamic_comp2_array_init(&array, &allocator, 4);
        TEST_BOOL(ok);
        if (!ok) return;

        for (int32_t i = 0; i < 35; ++i)
        {
            CompositeType2 value = make_comp2(i, i + 1, (i % 2) == 0);
            TEST_BOOL(dynamic_comp2_array_insert(&array, value));
        }

        TEST_INT64_EQUAL(array.length, (int64_t) 35);
        TEST_BOOL(array.capacity >= array.length);
        for (int32_t i = 0; i < 35; ++i)
        {
            CompositeType2 expected = make_comp2(i, i + 1, (i % 2) == 0);
            TEST_BOOL(comp2_equal(&array.data[i], &expected));
        }
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType3ToCompositeType2Map array = {0};
        bool ok = dynamic_comp3_array_init(&array, &allocator, 2);
        TEST_BOOL(ok);
        if (!ok) return;

        for (int32_t i = 0; i < 30; ++i)
        {
            CompositeType3 value = make_comp3(i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6);
            TEST_BOOL(dynamic_comp3_array_insert(&array, value));
        }

        TEST_INT64_EQUAL(array.length, (int64_t) 30);
        TEST_BOOL(array.capacity >= array.length);
        for (int32_t i = 0; i < 30; ++i)
        {
            CompositeType3 expected = make_comp3(i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6);
            TEST_BOOL(comp3_equal(&array.data[i], &expected));
        }
    }

    jsl_allocator_interface_free_all(&allocator);
}

static void test_dynamic_array_insert_at_inserts_and_shifts(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicInt32Array array = {0};
        bool ok = dynamic_int32_array_init(&array, &allocator, 4);
        TEST_BOOL(ok);
        if (!ok) return;

        TEST_BOOL(dynamic_int32_array_insert(&array, 1));
        TEST_BOOL(dynamic_int32_array_insert(&array, 3));
        TEST_BOOL(dynamic_int32_array_insert(&array, 4));

        TEST_BOOL(dynamic_int32_array_insert_at(&array, 2, 1));
        TEST_INT64_EQUAL(array.length, (int64_t) 4);

        TEST_BOOL(dynamic_int32_array_insert_at(&array, 0, 0));
        TEST_BOOL(dynamic_int32_array_insert_at(&array, 5, array.length));
        TEST_INT64_EQUAL(array.length, (int64_t) 6);

        int32_t expected[] = {0, 1, 2, 3, 4, 5};
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
        {
            TEST_INT32_EQUAL(array.data[i], expected[i]);
        }

        TEST_BOOL(!dynamic_int32_array_insert_at(&array, 6, array.length + 1));
        TEST_INT64_EQUAL(array.length, (int64_t) 6);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType1Map array = {0};
        bool ok = dynamic_comp1_array_init(&array, &allocator, 2);
        TEST_BOOL(ok);
        if (!ok) return;

        CompositeType1 v1 = make_comp1(1, 10);
        CompositeType1 v3 = make_comp1(3, 30);
        CompositeType1 v4 = make_comp1(4, 40);

        TEST_BOOL(dynamic_comp1_array_insert(&array, v1));
        TEST_BOOL(dynamic_comp1_array_insert(&array, v3));
        TEST_BOOL(dynamic_comp1_array_insert(&array, v4));

        CompositeType1 v2 = make_comp1(2, 20);
        TEST_BOOL(dynamic_comp1_array_insert_at(&array, v2, 1));
        TEST_INT64_EQUAL(array.length, (int64_t) 4);

        CompositeType1 v0 = make_comp1(0, 0);
        TEST_BOOL(dynamic_comp1_array_insert_at(&array, v0, 0));
        CompositeType1 v5 = make_comp1(5, 50);
        TEST_BOOL(dynamic_comp1_array_insert_at(&array, v5, array.length));
        TEST_INT64_EQUAL(array.length, (int64_t) 6);

        CompositeType1 expected[] = {v0, v1, v2, v3, v4, v5};
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
        {
            TEST_BOOL(comp1_equal(&array.data[i], &expected[i]));
        }

        TEST_BOOL(!dynamic_comp1_array_insert_at(&array, make_comp1(6, 60), (int64_t) array.length + 5));
        TEST_INT64_EQUAL(array.length, (int64_t) 6);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType2ToIntMap array = {0};
        bool ok = dynamic_comp2_array_init(&array, &allocator, 3);
        TEST_BOOL(ok);
        if (!ok) return;

        CompositeType2 v1 = make_comp2(1, 10, false);
        CompositeType2 v3 = make_comp2(3, 30, true);
        CompositeType2 v4 = make_comp2(4, 40, false);

        TEST_BOOL(dynamic_comp2_array_insert(&array, v1));
        TEST_BOOL(dynamic_comp2_array_insert(&array, v3));
        TEST_BOOL(dynamic_comp2_array_insert(&array, v4));

        CompositeType2 v2 = make_comp2(2, 20, true);
        TEST_BOOL(dynamic_comp2_array_insert_at(&array, v2, 1));
        TEST_INT64_EQUAL(array.length, (int64_t) 4);

        CompositeType2 v0 = make_comp2(0, 0, true);
        TEST_BOOL(dynamic_comp2_array_insert_at(&array, v0, 0));
        CompositeType2 v5 = make_comp2(5, 50, false);
        TEST_BOOL(dynamic_comp2_array_insert_at(&array, v5, array.length));
        TEST_INT64_EQUAL(array.length, (int64_t) 6);

        CompositeType2 expected[] = {v0, v1, v2, v3, v4, v5};
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
        {
            TEST_BOOL(comp2_equal(&array.data[i], &expected[i]));
        }

        TEST_BOOL(!dynamic_comp2_array_insert_at(&array, make_comp2(6, 60, true), (int64_t) array.length + 2));
        TEST_INT64_EQUAL(array.length, (int64_t) 6);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType3ToCompositeType2Map array = {0};
        bool ok = dynamic_comp3_array_init(&array, &allocator, 1);
        TEST_BOOL(ok);
        if (!ok) return;

        CompositeType3 v1 = make_comp3(1, 2, 3, 4, 5, 6, 7);
        CompositeType3 v3 = make_comp3(8, 9, 10, 11, 12, 13, 14);
        CompositeType3 v4 = make_comp3(15, 16, 17, 18, 19, 20, 21);

        TEST_BOOL(dynamic_comp3_array_insert(&array, v1));
        TEST_BOOL(dynamic_comp3_array_insert(&array, v3));
        TEST_BOOL(dynamic_comp3_array_insert(&array, v4));

        CompositeType3 v2 = make_comp3(22, 23, 24, 25, 26, 27, 28);
        TEST_BOOL(dynamic_comp3_array_insert_at(&array, v2, 1));
        TEST_INT64_EQUAL(array.length, (int64_t) 4);

        CompositeType3 v0 = make_comp3(29, 30, 31, 32, 33, 34, 35);
        TEST_BOOL(dynamic_comp3_array_insert_at(&array, v0, 0));
        CompositeType3 v5 = make_comp3(36, 37, 38, 39, 40, 41, 42);
        TEST_BOOL(dynamic_comp3_array_insert_at(&array, v5, array.length));
        TEST_INT64_EQUAL(array.length, (int64_t) 6);

        CompositeType3 expected[] = {v0, v1, v2, v3, v4, v5};
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
        {
            TEST_BOOL(comp3_equal(&array.data[i], &expected[i]));
        }

        TEST_BOOL(!dynamic_comp3_array_insert_at(&array, make_comp3(43, 44, 45, 46, 47, 48, 49), (int64_t) array.length + 3));
        TEST_INT64_EQUAL(array.length, (int64_t) 6);
    }

    jsl_allocator_interface_free_all(&allocator);
}

static void test_dynamic_array_delete_at_removes_and_shifts(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicInt32Array array = {0};
        bool ok = dynamic_int32_array_init(&array, &allocator, 8);
        TEST_BOOL(ok);
        if (!ok) return;

        int32_t values[] = {10, 20, 30, 40};
        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
        {
            TEST_BOOL(dynamic_int32_array_insert(&array, values[i]));
        }

        TEST_BOOL(!dynamic_int32_array_delete_at(&array, -1));
        TEST_BOOL(!dynamic_int32_array_delete_at(&array, 10));
        TEST_INT64_EQUAL(array.length, (int64_t) 4);

        TEST_BOOL(dynamic_int32_array_delete_at(&array, 1));
        TEST_INT64_EQUAL(array.length, (int64_t) 3);
        TEST_INT32_EQUAL(array.data[0], 10);
        TEST_INT32_EQUAL(array.data[1], 30);
        TEST_INT32_EQUAL(array.data[2], 40);

        TEST_BOOL(dynamic_int32_array_delete_at(&array, 2));
        TEST_INT64_EQUAL(array.length, (int64_t) 2);
        TEST_INT32_EQUAL(array.data[0], 10);
        TEST_INT32_EQUAL(array.data[1], 30);

        TEST_BOOL(dynamic_int32_array_delete_at(&array, 0));
        TEST_INT64_EQUAL(array.length, (int64_t) 1);
        TEST_INT32_EQUAL(array.data[0], 30);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType1Map array = {0};
        bool ok = dynamic_comp1_array_init(&array, &allocator, 4);
        TEST_BOOL(ok);
        if (!ok) return;

        CompositeType1 values[] = {
            make_comp1(1, 10),
            make_comp1(2, 20),
            make_comp1(3, 30),
            make_comp1(4, 40)
        };

        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
        {
            TEST_BOOL(dynamic_comp1_array_insert(&array, values[i]));
        }

        TEST_BOOL(!dynamic_comp1_array_delete_at(&array, -1));
        TEST_BOOL(!dynamic_comp1_array_delete_at(&array, 6));
        TEST_INT64_EQUAL(array.length, (int64_t) 4);

        TEST_BOOL(dynamic_comp1_array_delete_at(&array, 2));
        TEST_INT64_EQUAL(array.length, (int64_t) 3);
        TEST_BOOL(comp1_equal(&array.data[0], &values[0]));
        TEST_BOOL(comp1_equal(&array.data[1], &values[1]));
        TEST_BOOL(comp1_equal(&array.data[2], &values[3]));

        TEST_BOOL(dynamic_comp1_array_delete_at(&array, 2));
        TEST_INT64_EQUAL(array.length, (int64_t) 2);
        TEST_BOOL(comp1_equal(&array.data[0], &values[0]));
        TEST_BOOL(comp1_equal(&array.data[1], &values[1]));

        TEST_BOOL(dynamic_comp1_array_delete_at(&array, 0));
        TEST_INT64_EQUAL(array.length, (int64_t) 1);
        TEST_BOOL(comp1_equal(&array.data[0], &values[1]));
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType2ToIntMap array = {0};
        bool ok = dynamic_comp2_array_init(&array, &allocator, 4);
        TEST_BOOL(ok);
        if (!ok) return;

        CompositeType2 values[] = {
            make_comp2(1, 10, true),
            make_comp2(2, 20, false),
            make_comp2(3, 30, true),
            make_comp2(4, 40, false)
        };

        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
        {
            TEST_BOOL(dynamic_comp2_array_insert(&array, values[i]));
        }

        TEST_BOOL(!dynamic_comp2_array_delete_at(&array, -5));
        TEST_BOOL(!dynamic_comp2_array_delete_at(&array, 9));
        TEST_INT64_EQUAL(array.length, (int64_t) 4);

        TEST_BOOL(dynamic_comp2_array_delete_at(&array, 1));
        TEST_INT64_EQUAL(array.length, (int64_t) 3);
        TEST_BOOL(comp2_equal(&array.data[0], &values[0]));
        TEST_BOOL(comp2_equal(&array.data[1], &values[2]));
        TEST_BOOL(comp2_equal(&array.data[2], &values[3]));

        TEST_BOOL(dynamic_comp2_array_delete_at(&array, 2));
        TEST_INT64_EQUAL(array.length, (int64_t) 2);
        TEST_BOOL(comp2_equal(&array.data[0], &values[0]));
        TEST_BOOL(comp2_equal(&array.data[1], &values[2]));

        TEST_BOOL(dynamic_comp2_array_delete_at(&array, 0));
        TEST_INT64_EQUAL(array.length, (int64_t) 1);
        TEST_BOOL(comp2_equal(&array.data[0], &values[2]));
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType3ToCompositeType2Map array = {0};
        bool ok = dynamic_comp3_array_init(&array, &allocator, 4);
        TEST_BOOL(ok);
        if (!ok) return;

        CompositeType3 values[] = {
            make_comp3(1, 2, 3, 4, 5, 6, 7),
            make_comp3(8, 9, 10, 11, 12, 13, 14),
            make_comp3(15, 16, 17, 18, 19, 20, 21),
            make_comp3(22, 23, 24, 25, 26, 27, 28)
        };

        for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
        {
            TEST_BOOL(dynamic_comp3_array_insert(&array, values[i]));
        }

        TEST_BOOL(!dynamic_comp3_array_delete_at(&array, -2));
        TEST_BOOL(!dynamic_comp3_array_delete_at(&array, 12));
        TEST_INT64_EQUAL(array.length, (int64_t) 4);

        TEST_BOOL(dynamic_comp3_array_delete_at(&array, 2));
        TEST_INT64_EQUAL(array.length, (int64_t) 3);
        TEST_BOOL(comp3_equal(&array.data[0], &values[0]));
        TEST_BOOL(comp3_equal(&array.data[1], &values[1]));
        TEST_BOOL(comp3_equal(&array.data[2], &values[3]));

        TEST_BOOL(dynamic_comp3_array_delete_at(&array, 2));
        TEST_INT64_EQUAL(array.length, (int64_t) 2);
        TEST_BOOL(comp3_equal(&array.data[0], &values[0]));
        TEST_BOOL(comp3_equal(&array.data[1], &values[1]));

        TEST_BOOL(dynamic_comp3_array_delete_at(&array, 0));
        TEST_INT64_EQUAL(array.length, (int64_t) 1);
        TEST_BOOL(comp3_equal(&array.data[0], &values[1]));
    }

    jsl_allocator_interface_free_all(&allocator);
}

static void test_dynamic_array_clear_resets_length(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicInt32Array array = {0};
        bool ok = dynamic_int32_array_init(&array, &allocator, 2);
        TEST_BOOL(ok);
        if (!ok) return;

        TEST_BOOL(dynamic_int32_array_insert(&array, 1));
        TEST_BOOL(dynamic_int32_array_insert(&array, 2));

        int64_t initial_capacity = array.capacity;
        int32_t* data_ptr = array.data;

        dynamic_int32_array_clear(&array);

        TEST_INT64_EQUAL(array.length, (int64_t) 0);
        TEST_INT64_EQUAL(array.capacity, initial_capacity);
        TEST_POINTERS_EQUAL(array.data, data_ptr);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType1Map array = {0};
        bool ok = dynamic_comp1_array_init(&array, &allocator, 2);
        TEST_BOOL(ok);
        if (!ok) return;

        TEST_BOOL(dynamic_comp1_array_insert(&array, make_comp1(1, 2)));
        TEST_BOOL(dynamic_comp1_array_insert(&array, make_comp1(3, 4)));

        int64_t initial_capacity = array.capacity;
        CompositeType1* data_ptr = array.data;

        dynamic_comp1_array_clear(&array);

        TEST_INT64_EQUAL(array.length, (int64_t) 0);
        TEST_INT64_EQUAL(array.capacity, initial_capacity);
        TEST_POINTERS_EQUAL(array.data, data_ptr);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType2ToIntMap array = {0};
        bool ok = dynamic_comp2_array_init(&array, &allocator, 2);
        TEST_BOOL(ok);
        if (!ok) return;

        TEST_BOOL(dynamic_comp2_array_insert(&array, make_comp2(1, 2, true)));
        TEST_BOOL(dynamic_comp2_array_insert(&array, make_comp2(3, 4, false)));

        int64_t initial_capacity = array.capacity;
        CompositeType2* data_ptr = array.data;

        dynamic_comp2_array_clear(&array);

        TEST_INT64_EQUAL(array.length, (int64_t) 0);
        TEST_INT64_EQUAL(array.capacity, initial_capacity);
        TEST_POINTERS_EQUAL(array.data, data_ptr);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        DynamicCompositeType3ToCompositeType2Map array = {0};
        bool ok = dynamic_comp3_array_init(&array, &allocator, 2);
        TEST_BOOL(ok);
        if (!ok) return;

        TEST_BOOL(dynamic_comp3_array_insert(&array, make_comp3(1, 2, 3, 4, 5, 6, 7)));
        TEST_BOOL(dynamic_comp3_array_insert(&array, make_comp3(8, 9, 10, 11, 12, 13, 14)));

        int64_t initial_capacity = array.capacity;
        CompositeType3* data_ptr = array.data;

        dynamic_comp3_array_clear(&array);

        TEST_INT64_EQUAL(array.length, (int64_t) 0);
        TEST_INT64_EQUAL(array.capacity, initial_capacity);
        TEST_POINTERS_EQUAL(array.data, data_ptr);
    }

    jsl_allocator_interface_free_all(&allocator);
}

static void test_dynamic_array_checks_sentinel(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    DynamicInt32Array array = {0};
    bool ok = dynamic_int32_array_init(&array, &allocator, 2);
    TEST_BOOL(ok);
    if (!ok) return;

    array.sentinel = 0;

    TEST_BOOL(!dynamic_int32_array_insert(&array, 1));
    TEST_BOOL(!dynamic_int32_array_insert_at(&array, 2, 0));
    TEST_BOOL(!dynamic_int32_array_delete_at(&array, 0));

    array.length = 5;
    dynamic_int32_array_clear(&array);
    TEST_INT64_EQUAL(array.length, (int64_t) 5);

    jsl_arena_reset(&global_arena);
}

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    jsl_arena_init(&global_arena, malloc(arena_size), arena_size);

    RUN_TEST_FUNCTION("Test dynamic array init success", test_dynamic_array_init_success);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test dynamic array init invalid args", test_dynamic_array_init_invalid_args);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test dynamic array insert", test_dynamic_array_insert_appends_and_grows);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test dynamic array insert at", test_dynamic_array_insert_at_inserts_and_shifts);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test dynamic array delete at", test_dynamic_array_delete_at_removes_and_shifts);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test dynamic array clear", test_dynamic_array_clear_resets_length);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test dynamic array sentinel checks", test_dynamic_array_checks_sentinel);
    jsl_arena_reset(&global_arena);

    TEST_RESULTS();
    return lfails != 0;
}

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
#include "hash_maps/fixed_comp2_to_int_map.h"
#include "hash_maps/fixed_comp3_to_comp2_map.h"
#include "hash_maps/fixed_int32_to_comp1_map.h"
#include "hash_maps/fixed_int32_to_int32_map.h"

const int64_t arena_size = JSL_MEGABYTES(32);
JSLArena global_arena;

static void test_fixed_insert(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        FixedIntToIntMap hashmap;
        fixed_int32_to_int32_map_init(&hashmap, &allocator, 256, 0);

        int32_t insert_res = fixed_int32_to_int32_map_insert(&hashmap, 42, 999);

        TEST_BOOL(insert_res);
        TEST_INT64_EQUAL(hashmap.item_count, (int64_t) 1);

        // test max item count behavior
        for (int32_t i = 0; i < 300; i++)
        {
            fixed_int32_to_int32_map_insert(&hashmap, i, 999);
        }

        TEST_INT64_EQUAL(hashmap.item_count, (int64_t) 256);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedIntToCompositeType1Map hashmap;
        fixed_int32_to_comp1_map_init(&hashmap, &allocator, 256, 0);

        CompositeType1 value;
        value.a = 887;
        value.b = 56784587;
        int32_t insert_res = fixed_int32_to_comp1_map_insert(&hashmap, 4875847, value);

        TEST_BOOL(insert_res);
        TEST_INT64_EQUAL(hashmap.item_count, (int64_t) 1);

        // test max item count behavior
        for (int32_t i = 0; i < 300; i++)
        {
            fixed_int32_to_comp1_map_insert(&hashmap, i, value);
        }

        TEST_INT64_EQUAL(hashmap.item_count, (int64_t) 256);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedCompositeType2ToIntMap hashmap;
        fixed_comp2_to_int_map_init(&hashmap, &allocator, 256, 0);

        CompositeType2 key;
        key.a = 5497684;
        key.b = 84656;
        key.c = true;
        int32_t insert_res = fixed_comp2_to_int_map_insert(&hashmap, key, 849594759);

        TEST_BOOL(insert_res);
        TEST_INT64_EQUAL(hashmap.item_count, (int64_t) 1);

        // test max item count behavior
        for (int32_t i = 0; i < 300; i++)
        {
            CompositeType2 key1;
            key1.a = i;
            key1.b = 84656;
            key1.c = true;
            fixed_comp2_to_int_map_insert(&hashmap, key1, 849594759);
        }

        TEST_INT64_EQUAL(hashmap.item_count, (int64_t) 256);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedCompositeType3ToCompositeType2Map hashmap;
        fixed_comp3_to_comp2_map_init(&hashmap, &allocator, 256, 0);

        CompositeType3 key = {
            82154,
            50546,
            167199,
            144665,
            109103,
            79725,
            192849
        };
        CompositeType2 value;
        value.a = 5497684;
        value.b = 84656;
        value.c = true;
        int32_t insert_res = fixed_comp3_to_comp2_map_insert(&hashmap, key, value);

        TEST_BOOL(insert_res);
        TEST_INT64_EQUAL(hashmap.item_count, (int64_t) 1);

        // test max item count behavior
        for (int32_t i = 0; i < 300; i++)
        {
            CompositeType3 key1 = {
                i,
                50546,
                167199,
                144665,
                109103,
                79725,
                192849
            };
            fixed_comp3_to_comp2_map_insert(&hashmap, key1, value);
        }

        TEST_INT64_EQUAL(hashmap.item_count, (int64_t) 256);
    }

    jsl_allocator_interface_free_all(&allocator);
}

static void test_fixed_get(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        FixedIntToIntMap hashmap;
        fixed_int32_to_int32_map_init(&hashmap, &allocator, 256, 0);

        int32_t insert_res = fixed_int32_to_int32_map_insert(&hashmap, 8976, 1111);
        TEST_BOOL(insert_res == true);

        int32_t* get_res = fixed_int32_to_int32_map_get(&hashmap, 1112);
        TEST_POINTERS_EQUAL(get_res, NULL);

        get_res = fixed_int32_to_int32_map_get(&hashmap, 8976);
        TEST_BOOL(*get_res == 1111);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedIntToCompositeType1Map hashmap;
        fixed_int32_to_comp1_map_init(&hashmap, &allocator, 256, 0);

        CompositeType1 value = {0};
        value.a = 887;
        value.b = 56784587;

        int32_t insert_res = fixed_int32_to_comp1_map_insert(&hashmap, 585678435, value);
        TEST_BOOL(insert_res == true);

        CompositeType1* get_res = fixed_int32_to_comp1_map_get(&hashmap, 809367483);
        TEST_POINTERS_EQUAL(get_res, NULL);

        get_res = fixed_int32_to_comp1_map_get(&hashmap, 585678435);
        TEST_BOOL(memcmp(get_res, &value, sizeof(CompositeType1)) == 0);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedCompositeType2ToIntMap hashmap;
        fixed_comp2_to_int_map_init(&hashmap, &allocator, 256, 0);

        CompositeType2 key = {0};
        key.a = 36463453;
        key.b = true;

        int32_t insert_res = fixed_comp2_to_int_map_insert(&hashmap, key, 777777);
        TEST_BOOL(insert_res == true);

        CompositeType2 bad_key = {0};
        bad_key.a = 36463453;
        bad_key.b = false;
        int32_t* get_res = fixed_comp2_to_int_map_get(&hashmap, bad_key);
        TEST_POINTERS_EQUAL(get_res, NULL);

        get_res = fixed_comp2_to_int_map_get(&hashmap, key);
        TEST_BOOL(*get_res == 777777);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedCompositeType3ToCompositeType2Map hashmap;
        fixed_comp3_to_comp2_map_init(&hashmap, &allocator, 256, 0);

        CompositeType3 key = {
            82154,
            50546,
            167199,
            144665,
            109103,
            79725,
            192849
        };

        CompositeType2 value = {0};
        value.a = 887;
        value.b = 56784587;
        value.c = false;

        int32_t insert_res = fixed_comp3_to_comp2_map_insert(&hashmap, key, value);
        TEST_BOOL(insert_res == true);

        CompositeType3 bad_key = key;
        bad_key.a = 36463453;
        CompositeType2* get_res = fixed_comp3_to_comp2_map_get(&hashmap, bad_key);
        TEST_POINTERS_EQUAL(get_res, NULL);

        get_res = fixed_comp3_to_comp2_map_get(&hashmap, key);
        TEST_BOOL(memcmp(get_res, &value, sizeof(CompositeType2)) == 0);
    }

    jsl_allocator_interface_free_all(&allocator);
}

static void test_fixed_delete(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        FixedIntToIntMap hashmap;
        fixed_int32_to_int32_map_init(&hashmap, &allocator, 256, 0);

        bool insert_res = fixed_int32_to_int32_map_insert(&hashmap, 567687, 3546757);
        TEST_BOOL(insert_res == true);

        insert_res = fixed_int32_to_int32_map_insert(&hashmap, 23940, 3546757);
        TEST_BOOL(insert_res == true);

        insert_res = fixed_int32_to_int32_map_insert(&hashmap, 48686, 3546757);
        TEST_BOOL(insert_res == true);

        TEST_BOOL(hashmap.item_count == 3);

        bool delete_res = fixed_int32_to_int32_map_delete(&hashmap, 9999999);
        TEST_BOOL(delete_res == false);
        TEST_BOOL(hashmap.item_count == 3);

        delete_res = fixed_int32_to_int32_map_delete(&hashmap, 23940);
        TEST_BOOL(delete_res == true);
        TEST_BOOL(hashmap.item_count == 2);

        int64_t count = 0;
        FixedIntToIntMapIterator iter;
        int32_t iter_key;
        int32_t iter_value;
        bool iter_ok = fixed_int32_to_int32_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok == true);

        while (fixed_int32_to_int32_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            TEST_BOOL(iter_key != 23940);
            ++count;
        }

        TEST_INT64_EQUAL(count, (int64_t) 2);
        TEST_INT64_EQUAL(count, hashmap.item_count);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedIntToCompositeType1Map hashmap;
        fixed_int32_to_comp1_map_init(&hashmap, &allocator, 256, 0);

        CompositeType1 value;
        value.a = 887;
        value.b = 56784587;
        bool insert_res = fixed_int32_to_comp1_map_insert(&hashmap, 567687, value);
        TEST_BOOL(insert_res == true);

        insert_res = fixed_int32_to_comp1_map_insert(&hashmap, 23940, value);
        TEST_BOOL(insert_res == true);

        insert_res = fixed_int32_to_comp1_map_insert(&hashmap, 48686, value);
        TEST_BOOL(insert_res == true);

        TEST_BOOL(hashmap.item_count == 3);

        bool delete_res = fixed_int32_to_comp1_map_delete(&hashmap, 9999999);
        TEST_BOOL(delete_res == false);
        TEST_BOOL(hashmap.item_count == 3);

        delete_res = fixed_int32_to_comp1_map_delete(&hashmap, 23940);
        TEST_BOOL(delete_res == true);
        TEST_BOOL(hashmap.item_count == 2);

        int64_t count = 0;

        FixedIntToCompositeType1MapIterator iter;
        int32_t iter_key;
        CompositeType1 iter_value;

        bool iter_ok = fixed_int32_to_comp1_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok == true);

        TEST_BOOL(iter_ok == true);

        while (fixed_int32_to_comp1_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            TEST_BOOL(iter_key != 23940);
            ++count;
        }

        TEST_INT64_EQUAL(count, (int64_t) 2);
        TEST_INT64_EQUAL(count, hashmap.item_count);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedCompositeType2ToIntMap hashmap;
        fixed_comp2_to_int_map_init(&hashmap, &allocator, 256, 0);

        CompositeType2 key1 = { .a = 67, .b = false };
        CompositeType2 key2 = { .a = 67, .b = true };
        CompositeType2 key3 = { .a = 1434, .b = true };
        CompositeType2 key4 = { .a = 0, .b = false };

        bool insert_res = fixed_comp2_to_int_map_insert(&hashmap, key1, 58678568);
        TEST_BOOL(insert_res == true);

        insert_res = fixed_comp2_to_int_map_insert(&hashmap, key2, 58678568);
        TEST_BOOL(insert_res == true);

        insert_res = fixed_comp2_to_int_map_insert(&hashmap, key3, 58678568);
        TEST_BOOL(insert_res == true);

        TEST_BOOL(hashmap.item_count == 3);

        bool delete_res = fixed_comp2_to_int_map_delete(&hashmap, key4);
        TEST_BOOL(delete_res == false);
        TEST_BOOL(hashmap.item_count == 3);

        delete_res = fixed_comp2_to_int_map_delete(&hashmap, key2);
        TEST_BOOL(delete_res == true);
        TEST_BOOL(hashmap.item_count == 2);

        int64_t count = 0;
        FixedCompositeType2ToIntMapIterator iter;
        CompositeType2 iter_key;
        int32_t iter_value;

        bool iter_ok = fixed_comp2_to_int_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok == true);

        while (fixed_comp2_to_int_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            TEST_BOOL(memcmp(&iter_key, &key2, sizeof(CompositeType2)) != 0);
            ++count;
        }

        TEST_INT64_EQUAL(count, (int64_t) 2);
        TEST_INT64_EQUAL(count, hashmap.item_count);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedCompositeType3ToCompositeType2Map hashmap;
        fixed_comp3_to_comp2_map_init(&hashmap, &allocator, 256, 0);

        CompositeType3 key1 = {
            82154,
            50546,
            167199,
            144665,
            109103,
            79725,
            192849
        };
        CompositeType3 key2 = {
            286444,
            361030,
            167199,
            144665,
            109103,
            79725,
            192849
        };
        CompositeType3 key3 = {
            82154,
            50546,
            167199,
            2170383,
            109103,
            79725,
            192849
        };
        CompositeType3 key4 = {
            82154,
            50546,
            167199,
            144665,
            109103,
            1444863,
            6646077
        };

        CompositeType2 value;
        value.a = 887;
        value.b = 56784587;
        value.c = false;
        bool insert_res = fixed_comp3_to_comp2_map_insert(&hashmap, key1, value);
        TEST_BOOL(insert_res == true);

        insert_res = fixed_comp3_to_comp2_map_insert(&hashmap, key2, value);
        TEST_BOOL(insert_res == true);

        insert_res = fixed_comp3_to_comp2_map_insert(&hashmap, key3, value);
        TEST_BOOL(insert_res == true);

        TEST_BOOL(hashmap.item_count == 3);

        bool delete_res = fixed_comp3_to_comp2_map_delete(&hashmap, key4);
        TEST_BOOL(delete_res == false);
        TEST_BOOL(hashmap.item_count == 3);

        delete_res = fixed_comp3_to_comp2_map_delete(&hashmap, key2);
        TEST_BOOL(delete_res == true);
        TEST_BOOL(hashmap.item_count == 2);

        int64_t count = 0;
        FixedCompositeType3ToCompositeType2MapIterator iter;
        CompositeType3 iter_key;
        CompositeType2 iter_value;

        bool iter_ok = fixed_comp3_to_comp2_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok == true);

        while (fixed_comp3_to_comp2_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            TEST_BOOL(memcmp(&iter_key, &key2, sizeof(CompositeType3)) != 0);
            ++count;
        }

        TEST_INT64_EQUAL(count, (int64_t) 2);
        TEST_INT64_EQUAL(count, hashmap.item_count);
    }

    jsl_allocator_interface_free_all(&allocator);
}

static void test_fixed_iterator(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    {
        FixedIntToIntMap hashmap;
        fixed_int32_to_int32_map_init(&hashmap, &allocator, 500, 0);

        for (int32_t i = 0; i < 300; ++i)
        {
            int32_t res = fixed_int32_to_int32_map_insert(&hashmap, i, i);
            TEST_BOOL(res == true);
        }

        int32_t count = 0;

        FixedIntToIntMapIterator iter;
        int32_t iter_key;
        int32_t iter_value;
        bool iter_ok = fixed_int32_to_int32_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok == true);

        while (fixed_int32_to_int32_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        TEST_INT32_EQUAL(count, 300);

        fixed_int32_to_int32_map_delete(&hashmap, 100);

        count = 0;
        iter_ok = fixed_int32_to_int32_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok == true);

        while (fixed_int32_to_int32_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        TEST_INT32_EQUAL(count, 299);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedIntToCompositeType1Map hashmap;
        fixed_int32_to_comp1_map_init(&hashmap, &allocator, 500, 0);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompositeType1 value;
            value.a = 887;
            value.b = 56784587;
            int32_t res = fixed_int32_to_comp1_map_insert(&hashmap, i, value);
            TEST_BOOL(res == true);
        }

        int32_t count = 0;
        FixedIntToCompositeType1MapIterator iter;
        bool iter_ok = fixed_int32_to_comp1_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok);

        int32_t iter_key;
        CompositeType1 iter_value;
        while (fixed_int32_to_comp1_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        TEST_INT32_EQUAL(count, 300);

        fixed_int32_to_comp1_map_delete(&hashmap, 100);

        count = 0;
        iter_ok = fixed_int32_to_comp1_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok);

        while (fixed_int32_to_comp1_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        TEST_INT32_EQUAL(count, 299);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedCompositeType2ToIntMap hashmap;
        fixed_comp2_to_int_map_init(&hashmap, &allocator, 500, 0);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompositeType2 key = {0};
            key.a = i;
            key.b = 10;
            key.c = true;
            int32_t res = fixed_comp2_to_int_map_insert(&hashmap, key, i);
            TEST_BOOL(res == true);
        }

        int32_t count = 0;
        FixedCompositeType2ToIntMapIterator iter;
        bool iter_ok = fixed_comp2_to_int_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok);

        CompositeType2 iter_key;
        int32_t iter_value;
        while (fixed_comp2_to_int_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        TEST_INT32_EQUAL(count, 300);

        CompositeType2 delete_key = {0};
        delete_key.a = 100;
        delete_key.b = 10;
        delete_key.c = true;
        bool del_ok = fixed_comp2_to_int_map_delete(&hashmap, delete_key);
        TEST_BOOL(del_ok);

        count = 0;
        iter_ok = fixed_comp2_to_int_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok);

        while (fixed_comp2_to_int_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        TEST_INT32_EQUAL(count, 299);
    }

    jsl_allocator_interface_free_all(&allocator);

    {
        FixedCompositeType3ToCompositeType2Map hashmap;
        fixed_comp3_to_comp2_map_init(&hashmap, &allocator, 500, 0);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompositeType3 key = {0};
            key.a = i;

            CompositeType2 value;
            value.a = 887;
            value.b = i;

            int32_t res = fixed_comp3_to_comp2_map_insert(&hashmap, key, value);
            TEST_BOOL(res == true);
        }

        int32_t count = 0;
        FixedCompositeType3ToCompositeType2MapIterator iter;
        bool iter_ok = fixed_comp3_to_comp2_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok);

        CompositeType3 iter_key;
        CompositeType2 iter_value;
        while (fixed_comp3_to_comp2_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        TEST_INT32_EQUAL(count, 300);

        CompositeType3 delete_key = {0};
        delete_key.a = 100;
        bool del_ok = fixed_comp3_to_comp2_map_delete(&hashmap, delete_key);
        TEST_BOOL(del_ok);

        count = 0;
        iter_ok = fixed_comp3_to_comp2_map_iterator_start(&hashmap, &iter);
        TEST_BOOL(iter_ok);
        while (fixed_comp3_to_comp2_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        TEST_INT32_EQUAL(count, 299);
    }

    jsl_allocator_interface_free_all(&allocator);
}

typedef struct ExpectedPair {
    JSLFatPtr key;
    JSLFatPtr value;
    bool seen;
} ExpectedPair;

static void test_jsl_str_to_str_map_init_success(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 0xBEEF, 64, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    TEST_POINTERS_EQUAL(map.allocator, &allocator);
    TEST_BOOL(map.entry_lookup_table != NULL);
    int64_t expected_length = JSL_MAX(32L, jsl_next_power_of_two_i64(65));
    TEST_INT64_EQUAL(map.entry_lookup_table_length, expected_length);
    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 0);
    TEST_BOOL(map.load_factor == 0.5f);
}

static void test_jsl_str_to_str_map_init_invalid_arguments(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};

    TEST_BOOL(!jsl_str_to_str_map_init2(NULL, &allocator, 0, 8, 0.5f));
    TEST_BOOL(!jsl_str_to_str_map_init2(&map, NULL, 0, 8, 0.5f));
    TEST_BOOL(!jsl_str_to_str_map_init2(&map, &allocator, 0, 0, 0.5f));
    TEST_BOOL(!jsl_str_to_str_map_init2(&map, &allocator, 0, -4, 0.5f));
    TEST_BOOL(!jsl_str_to_str_map_init2(&map, &allocator, 0, 4, 0.0f));
    TEST_BOOL(!jsl_str_to_str_map_init2(&map, &allocator, 0, 4, 1.0f));
    TEST_BOOL(!jsl_str_to_str_map_init2(&map, &allocator, 0, 4, 1.5f));
}

static void test_jsl_str_to_str_map_item_count_and_has_key(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init(&map, &allocator, 1234);
    TEST_BOOL(ok);
    if (!ok) return;

    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 0);

    JSLFatPtr key1 = JSL_FATPTR_INITIALIZER("alpha");
    JSLFatPtr key2 = JSL_FATPTR_INITIALIZER("beta");
    JSLFatPtr missing = JSL_FATPTR_INITIALIZER("missing");

    TEST_BOOL(jsl_str_to_str_map_insert(&map, key1, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("one"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_map_insert(&map, key2, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("two"), JSL_STRING_LIFETIME_STATIC));

    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 2);
    TEST_BOOL(jsl_str_to_str_map_has_key(&map, key1));
    TEST_BOOL(jsl_str_to_str_map_has_key(&map, key2));
    TEST_BOOL(!jsl_str_to_str_map_has_key(&map, missing));

    JSLStrToStrMap uninitialized = {0};
    TEST_BOOL(!jsl_str_to_str_map_has_key(&uninitialized, key1));
    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&uninitialized), (int64_t) -1);
}

static void test_jsl_str_to_str_map_get(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 2468, 8, 0.6f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("lookup-key");
    JSLFatPtr value = JSL_FATPTR_INITIALIZER("lookup-value");

    JSLFatPtr out_value = JSL_FATPTR_EXPRESSION("should-reset");
    TEST_BOOL(!jsl_str_to_str_map_get(&map, key, &out_value));
    TEST_INT64_EQUAL(out_value.length, (int64_t) 0);

    TEST_BOOL(jsl_str_to_str_map_insert(
        &map,
        key, JSL_STRING_LIFETIME_STATIC,
        value, JSL_STRING_LIFETIME_STATIC
    ));

    TEST_BOOL(jsl_str_to_str_map_get(&map, key, &out_value));
    TEST_BOOL(jsl_fatptr_memory_compare(out_value, value));

    JSLFatPtr missing = JSL_FATPTR_INITIALIZER("missing");
    out_value = JSL_FATPTR_EXPRESSION("still-reset");
    TEST_BOOL(!jsl_str_to_str_map_get(&map, missing, &out_value));
    TEST_INT64_EQUAL(out_value.length, (int64_t) 0);

    TEST_BOOL(!jsl_str_to_str_map_get(&map, key, NULL));
}

static void test_jsl_str_to_str_map_insert_overwrites_value(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 42, 8, 0.7f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("dup-key");

    TEST_BOOL(jsl_str_to_str_map_insert(&map, key, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("first"), JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 1);

    TEST_BOOL(jsl_str_to_str_map_insert(&map, key, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("second"), JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 1);

    JSLStrToStrMapKeyValueIter iter;
    jsl_str_to_str_map_key_value_iterator_init(&map, &iter);

    JSLFatPtr out_key = (JSLFatPtr) {0};
    JSLFatPtr out_value = (JSLFatPtr) {0};
    bool found = false;
    while (jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value))
    {
        if (jsl_fatptr_memory_compare(out_key, key))
        {
            found = true;
            TEST_BOOL(jsl_fatptr_memory_compare(out_value, JSL_FATPTR_EXPRESSION("second")));
            break;
        }
    }

    TEST_BOOL(found);
}

static void test_jsl_str_to_str_map_transient_lifetime_copies_data(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 555, 8, 0.6f);
    TEST_BOOL(ok);
    if (!ok) return;

    char key_small_buf[] = "short";
    char val_small_buf[] = "miniVal";
    char key_long_buf[] = "a-longer-key";
    char val_long_buf[] = "a-longer-value";

    JSLFatPtr key_small = jsl_fatptr_from_cstr(key_small_buf);
    JSLFatPtr val_small = jsl_fatptr_from_cstr(val_small_buf);
    JSLFatPtr key_long = jsl_fatptr_from_cstr(key_long_buf);
    JSLFatPtr val_long = jsl_fatptr_from_cstr(val_long_buf);

    TEST_BOOL(jsl_str_to_str_map_insert(&map, key_small, JSL_STRING_LIFETIME_TRANSIENT, val_small, JSL_STRING_LIFETIME_TRANSIENT));
    TEST_BOOL(jsl_str_to_str_map_insert(&map, key_long, JSL_STRING_LIFETIME_TRANSIENT, val_long, JSL_STRING_LIFETIME_TRANSIENT));

    key_small_buf[0] = 'X';
    val_small_buf[0] = 'Z';
    key_long_buf[0] = 'Y';
    val_long_buf[0] = 'Q';

    const JSLFatPtr expected_small_key = JSL_FATPTR_EXPRESSION("short");
    const JSLFatPtr expected_small_value = JSL_FATPTR_EXPRESSION("miniVal");
    const JSLFatPtr expected_long_key = JSL_FATPTR_EXPRESSION("a-longer-key");
    const JSLFatPtr expected_long_value = JSL_FATPTR_EXPRESSION("a-longer-value");

    JSLStrToStrMapKeyValueIter iter;
    jsl_str_to_str_map_key_value_iterator_init(&map, &iter);

    bool saw_small = false;
    bool saw_long = false;
    JSLFatPtr out_key = (JSLFatPtr) {0};
    JSLFatPtr out_value = (JSLFatPtr) {0};
    while (jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value))
    {
        if (jsl_fatptr_memory_compare(out_key, expected_small_key))
        {
            saw_small = true;
            TEST_BOOL(jsl_fatptr_memory_compare(out_value, expected_small_value));
            TEST_BOOL(out_key.data != (uint8_t*) key_small_buf);
            TEST_BOOL(out_value.data != (uint8_t*) val_small_buf);
        }
        else if (jsl_fatptr_memory_compare(out_key, expected_long_key))
        {
            saw_long = true;
            TEST_BOOL(jsl_fatptr_memory_compare(out_value, expected_long_value));
            TEST_BOOL(out_key.data != (uint8_t*) key_long_buf);
            TEST_BOOL(out_value.data != (uint8_t*) val_long_buf);
        }
    }

    TEST_BOOL(saw_small);
    TEST_BOOL(saw_long);
}

static void test_jsl_str_to_str_map_fixed_lifetime_uses_original_pointers(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 777, 8, 0.65f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("fixed-key");
    JSLFatPtr value = JSL_FATPTR_INITIALIZER("fixed-value");

    const uint8_t* expected_key_ptr = key.data;
    const uint8_t* expected_val_ptr = value.data;

    TEST_BOOL(jsl_str_to_str_map_insert(&map, key, JSL_STRING_LIFETIME_STATIC, value, JSL_STRING_LIFETIME_STATIC));

    JSLStrToStrMapKeyValueIter iter;
    jsl_str_to_str_map_key_value_iterator_init(&map, &iter);

    JSLFatPtr out_key = (JSLFatPtr) {0};
    JSLFatPtr out_value = (JSLFatPtr) {0};
    bool found = jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value);
    TEST_BOOL(found);
    if (!found) return;

    TEST_POINTERS_EQUAL(out_key.data, expected_key_ptr);
    TEST_POINTERS_EQUAL(out_value.data, expected_val_ptr);
    TEST_BOOL(jsl_fatptr_memory_compare(out_key, key));
    TEST_BOOL(jsl_fatptr_memory_compare(out_value, value));
}

static void test_jsl_str_to_str_map_handles_empty_and_binary_strings(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 888, 8, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr empty_key = JSL_FATPTR_INITIALIZER("");
    JSLFatPtr binary_key = JSL_FATPTR_INITIALIZER("bin");
    JSLFatPtr empty_value_key = JSL_FATPTR_INITIALIZER("empty-value-key");

    uint8_t binary_value_buf[] = { 'A', 0x00, 'B', 0x7F };
    JSLFatPtr binary_value = jsl_fatptr_init(binary_value_buf, 4);
    JSLFatPtr empty_value = JSL_FATPTR_INITIALIZER("");

    TEST_BOOL(jsl_str_to_str_map_insert(&map, empty_key, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("empty-key-value"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_map_insert(&map, binary_key, JSL_STRING_LIFETIME_STATIC, binary_value, JSL_STRING_LIFETIME_TRANSIENT));
    TEST_BOOL(jsl_str_to_str_map_insert(&map, empty_value_key, JSL_STRING_LIFETIME_STATIC, empty_value, JSL_STRING_LIFETIME_STATIC));

    TEST_BOOL(jsl_str_to_str_map_has_key(&map, empty_key));
    TEST_BOOL(jsl_str_to_str_map_has_key(&map, binary_key));
    TEST_BOOL(jsl_str_to_str_map_has_key(&map, empty_value_key));

    ExpectedPair expected[] = {
        { empty_key, JSL_FATPTR_EXPRESSION("empty-key-value"), false },
        { binary_key, binary_value, false },
        { empty_value_key, empty_value, false }
    };

    JSLStrToStrMapKeyValueIter iter;
    jsl_str_to_str_map_key_value_iterator_init(&map, &iter);

    JSLFatPtr out_key = (JSLFatPtr) {0};
    JSLFatPtr out_value = (JSLFatPtr) {0};
    while (jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value))
    {
        bool matched = false;
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
        {
            if (!expected[i].seen && jsl_fatptr_memory_compare(out_key, expected[i].key))
            {
                if (expected[i].value.length == out_value.length)
                {
                    if (out_value.length == 0)
                    {
                        matched = true;
                    }
                    else if (memcmp(out_value.data, expected[i].value.data, (size_t) out_value.length) == 0)
                    {
                        matched = true;
                    }
                }

                expected[i].seen = matched;
                break;
            }
        }
        TEST_BOOL(matched);
    }

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        TEST_BOOL(expected[i].seen);
    }
}

static void test_jsl_str_to_str_map_iterator_covers_all_pairs(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 999, 10, 0.6f);
    TEST_BOOL(ok);
    if (!ok) return;

    ExpectedPair expected[] = {
        { JSL_FATPTR_INITIALIZER("a"), JSL_FATPTR_INITIALIZER("1"), false },
        { JSL_FATPTR_INITIALIZER("b"), JSL_FATPTR_INITIALIZER("2"), false },
        { JSL_FATPTR_INITIALIZER("c"), JSL_FATPTR_INITIALIZER("3"), false },
        { JSL_FATPTR_INITIALIZER("d"), JSL_FATPTR_INITIALIZER("4"), false }
    };

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        TEST_BOOL(jsl_str_to_str_map_insert(
            &map,
            expected[i].key,
            JSL_STRING_LIFETIME_STATIC,
            expected[i].value,
            JSL_STRING_LIFETIME_STATIC
        ));
    }

    JSLStrToStrMapKeyValueIter iter;
    jsl_str_to_str_map_key_value_iterator_init(&map, &iter);

    size_t seen = 0;
    JSLFatPtr out_key = (JSLFatPtr) {0};
    JSLFatPtr out_value = (JSLFatPtr) {0};
    while (jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value))
    {
        bool matched = false;
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
        {
            if (!expected[i].seen && jsl_fatptr_memory_compare(out_key, expected[i].key) && jsl_fatptr_memory_compare(out_value, expected[i].value))
            {
                expected[i].seen = true;
                matched = true;
                seen++;
                break;
            }
        }
        TEST_BOOL(matched);
    }

    TEST_INT64_EQUAL((int64_t) seen, (int64_t)(sizeof(expected) / sizeof(expected[0])));
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        TEST_BOOL(expected[i].seen);
    }

    TEST_BOOL(!jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value));
}

static void test_jsl_str_to_str_map_iterator_invalidated_on_mutation(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init(&map, &allocator, 1111);
    TEST_BOOL(ok);
    if (!ok) return;

    TEST_BOOL(jsl_str_to_str_map_insert(&map, JSL_FATPTR_EXPRESSION("key1"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("value1"), JSL_STRING_LIFETIME_STATIC));

    JSLStrToStrMapKeyValueIter iter;
    jsl_str_to_str_map_key_value_iterator_init(&map, &iter);

    TEST_BOOL(jsl_str_to_str_map_insert(&map, JSL_FATPTR_EXPRESSION("key2"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("value2"), JSL_STRING_LIFETIME_STATIC));

    JSLFatPtr out_key = (JSLFatPtr) {0};
    JSLFatPtr out_value = (JSLFatPtr) {0};
    bool has_data = jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value);
    TEST_BOOL(has_data == false);
}

static void test_jsl_str_to_str_map_delete(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 2222, 12, 0.7f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr keep = JSL_FATPTR_INITIALIZER("keep");
    JSLFatPtr drop = JSL_FATPTR_INITIALIZER("drop");
    JSLFatPtr other = JSL_FATPTR_INITIALIZER("other");

    TEST_BOOL(jsl_str_to_str_map_insert(&map, keep, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("1"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_map_insert(&map, drop, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("2"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_map_insert(&map, other, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("3"), JSL_STRING_LIFETIME_STATIC));

    TEST_BOOL(!jsl_str_to_str_map_delete(&map, JSL_FATPTR_EXPRESSION("missing")));

    TEST_BOOL(jsl_str_to_str_map_delete(&map, drop));
    TEST_BOOL(!jsl_str_to_str_map_has_key(&map, drop));
    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 2);

    TEST_BOOL(!jsl_str_to_str_map_delete(&map, drop));

    TEST_BOOL(jsl_str_to_str_map_has_key(&map, keep));
    TEST_BOOL(jsl_str_to_str_map_has_key(&map, other));
}

static void test_jsl_str_to_str_map_clear(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init(&map, &allocator, 3333);
    TEST_BOOL(ok);
    if (!ok) return;

    TEST_BOOL(jsl_str_to_str_map_insert(&map, JSL_FATPTR_EXPRESSION("a"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("1"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_map_insert(&map, JSL_FATPTR_EXPRESSION("b"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("2"), JSL_STRING_LIFETIME_STATIC));

    jsl_str_to_str_map_clear(&map);

    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 0);
    TEST_BOOL(!jsl_str_to_str_map_has_key(&map, JSL_FATPTR_EXPRESSION("a")));
    TEST_BOOL(!jsl_str_to_str_map_has_key(&map, JSL_FATPTR_EXPRESSION("b")));

    JSLStrToStrMapKeyValueIter iter;
    jsl_str_to_str_map_key_value_iterator_init(&map, &iter);
    JSLFatPtr out_key = (JSLFatPtr) {0};
    JSLFatPtr out_value = (JSLFatPtr) {0};
    TEST_BOOL(!jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value));

    TEST_BOOL(jsl_str_to_str_map_insert(&map, JSL_FATPTR_EXPRESSION("c"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("3"), JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 1);
    TEST_BOOL(jsl_str_to_str_map_has_key(&map, JSL_FATPTR_EXPRESSION("c")));
}

static void test_jsl_str_to_str_map_rehash(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init2(&map, &allocator, 4444, 4, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    int64_t initial_capacity = map.entry_lookup_table_length;

    #define key_count 34
    JSLFatPtr keys[key_count] = {0};
    JSLFatPtr values[key_count] = {0};

    for (int i = 0; i < key_count; ++i)
    {
        keys[i] = jsl_format(&allocator, JSL_FATPTR_EXPRESSION("key-%d"), i);
        values[i] = jsl_format(&allocator, JSL_FATPTR_EXPRESSION("val-%d"), i);

        bool insert_res = jsl_str_to_str_map_insert(
            &map,
            keys[i], JSL_STRING_LIFETIME_STATIC,
            values[i], JSL_STRING_LIFETIME_STATIC
        );
        TEST_BOOL(insert_res);
    }

    TEST_BOOL(map.entry_lookup_table_length > initial_capacity);
    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) key_count);

    for (int i = 0; i < key_count; ++i)
    {
        TEST_BOOL(jsl_str_to_str_map_has_key(&map, keys[i]));
    }

    JSLStrToStrMapKeyValueIter iter;
    jsl_str_to_str_map_key_value_iterator_init(&map, &iter);

    int64_t seen = 0;
    JSLFatPtr out_key = (JSLFatPtr) {0};
    JSLFatPtr out_value = (JSLFatPtr) {0};
    while (jsl_str_to_str_map_key_value_iterator_next(&iter, &out_key, &out_value))
    {
        bool matched = false;
        for (int i = 0; i < key_count; ++i)
        {
            if (jsl_fatptr_memory_compare(out_key, keys[i]) && jsl_fatptr_memory_compare(out_value, values[i]))
            {
                matched = true;
                break;
            }
        }
        TEST_BOOL(matched);
        seen++;
    }

    TEST_INT64_EQUAL(seen, (int64_t) key_count);

    #undef key_count
}

static void test_jsl_str_to_str_map_invalid_inserts(void)
{
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    jsl_allocator_interface_free_all(&allocator);

    JSLStrToStrMap map = {0};
    bool ok = jsl_str_to_str_map_init(&map, &allocator, 5555);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("key");
    JSLFatPtr value = JSL_FATPTR_INITIALIZER("value");

    TEST_BOOL(!jsl_str_to_str_map_insert(NULL, key, JSL_STRING_LIFETIME_STATIC, value, JSL_STRING_LIFETIME_STATIC));

    JSLFatPtr null_key = {0};
    TEST_BOOL(!jsl_str_to_str_map_insert(&map, null_key, JSL_STRING_LIFETIME_STATIC, value, JSL_STRING_LIFETIME_STATIC));

    JSLFatPtr null_value = {0};
    TEST_BOOL(!jsl_str_to_str_map_insert(&map, key, JSL_STRING_LIFETIME_STATIC, null_value, JSL_STRING_LIFETIME_STATIC));

    JSLFatPtr negative_len = jsl_fatptr_init((uint8_t*)"neg", -1);
    TEST_BOOL(!jsl_str_to_str_map_insert(&map, negative_len, JSL_STRING_LIFETIME_STATIC, value, JSL_STRING_LIFETIME_STATIC));

    TEST_INT64_EQUAL(jsl_str_to_str_map_item_count(&map), (int64_t) 0);
}

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    jsl_arena_init(&global_arena, malloc(arena_size), arena_size);

    RUN_TEST_FUNCTION("Test fixed hashmap insert", test_fixed_insert);
    RUN_TEST_FUNCTION("Test fixed hashmap get", test_fixed_get);
    RUN_TEST_FUNCTION("Test fixed hashmap iterator", test_fixed_iterator);
    RUN_TEST_FUNCTION("Test fixed hashmap delete", test_fixed_delete);

    RUN_TEST_FUNCTION("Test str to str map init success", test_jsl_str_to_str_map_init_success);
    RUN_TEST_FUNCTION("Test str to str map init invalid args", test_jsl_str_to_str_map_init_invalid_arguments);
    RUN_TEST_FUNCTION("Test str to str map item count and has key", test_jsl_str_to_str_map_item_count_and_has_key);
    RUN_TEST_FUNCTION("Test str to str map get", test_jsl_str_to_str_map_get);
    RUN_TEST_FUNCTION("Test str to str map insert", test_jsl_str_to_str_map_insert_overwrites_value);
    RUN_TEST_FUNCTION("Test str to str map transient lifetime copies", test_jsl_str_to_str_map_transient_lifetime_copies_data);
    RUN_TEST_FUNCTION("Test str to str map static lifetime keeps pointer", test_jsl_str_to_str_map_fixed_lifetime_uses_original_pointers);
    RUN_TEST_FUNCTION("Test str to str map empty and binary strings", test_jsl_str_to_str_map_handles_empty_and_binary_strings);
    RUN_TEST_FUNCTION("Test str to str map iterator", test_jsl_str_to_str_map_iterator_covers_all_pairs);
    RUN_TEST_FUNCTION("Test str to str map iterator invalid after mutation", test_jsl_str_to_str_map_iterator_invalidated_on_mutation);
    RUN_TEST_FUNCTION("Test str to str map delete", test_jsl_str_to_str_map_delete);
    RUN_TEST_FUNCTION("Test str to str map clear", test_jsl_str_to_str_map_clear);
    RUN_TEST_FUNCTION("Test str to str map rehash", test_jsl_str_to_str_map_rehash);
    RUN_TEST_FUNCTION("Test str to str map invalid inserts", test_jsl_str_to_str_map_invalid_inserts);

    TEST_RESULTS();
    return lfails != 0;
}

/**
 * Copyright (c) 2025 Jack Stouffer
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

#define JSL_IMPLEMENTATION
#include "../src/jacks_standard_library.h"

#include "minctest.h"
#include "test_hash_map_types.h"
#include "tests/hash_maps/comp2_to_int_map.h"
#include "tests/hash_maps/comp3_to_comp2_map.h"
#include "tests/hash_maps/float_to_float_map.h"
#include "tests/hash_maps/int32_to_comp1_map.h"
#include "tests/hash_maps/int32_to_int32_map.h"

const int64_t arena_size = 2 * 1024 * 1024;
JSLArena arena;

void test_insert(void)
{
    jss_arena_reset(&arena);

    {
        MyHashMap hashmap;
        my_map_ctor(&hashmap, &arena);

        int32_t insert_res = my_map_insert(&hashmap, 42, 999);

        lok(insert_res == true);
        lok(hashmap.item_count == 1);
    }

    jss_arena_reset(&arena);

    {
        CompMap hashmap;
        comp_map_ctor(&hashmap, &arena);

        CompValue value;
        value.a = 7.57847887;
        value.b = 58677586784587;
        int32_t insert_res = comp_map_insert(&hashmap, 4875847, value);

        lok(insert_res == true);
        lok(hashmap.item_count == 1);
    }

    jss_arena_reset(&arena);

    {
        CompMap2 hashmap;
        comp_map2_ctor(&hashmap, &arena);

        CompKey key;
        key.a = 5497684;
        key.b = true;
        int32_t insert_res = comp_map2_insert(&hashmap, key, 849594759);

        lok(insert_res == true);
        lok(hashmap.item_count == 1);
    }

    jss_arena_reset(&arena);

    {
        CompMap3 hashmap;
        comp_map3_ctor(&hashmap, &arena);

        CompKey key;
        CompValue value;
        key.a = 5497684;
        key.b = true;
        value.a = 0.98767;
        value.b = 54;
        int32_t insert_res = comp_map3_insert(&hashmap, key, value);

        lok(insert_res == true);
        lok(hashmap.item_count == 1);
    }

    jss_arena_reset(&arena);
}

void test_get(void)
{
    jss_arena_reset(&arena);

    {
        MyHashMap hashmap;
        my_map_ctor(&hashmap, &arena);

        int32_t insert_res = my_map_insert(&hashmap, 8976, 1111);
        lok(insert_res == true);

        int32_t* get_res = my_map_get(&hashmap, 1112);
        lok(get_res == NULL);

        get_res = my_map_get(&hashmap, 8976);
        lok(*get_res == 1111);
    }

    jss_arena_reset(&arena);

    {
        CompMap hashmap;
        comp_map_ctor(&hashmap, &arena);

        CompValue value = {0};
        value.a = 2.89;
        value.b = 1222121;

        int32_t insert_res = comp_map_insert(&hashmap, 585678435, value);
        lok(insert_res == true);

        CompValue* get_res = comp_map_get(&hashmap, 809367483);
        lok(get_res == NULL);

        get_res = comp_map_get(&hashmap, 585678435);
        lok(memcmp(get_res, &value, sizeof(CompValue)) == 0);
    }

    jss_arena_reset(&arena);

    {
        CompMap2 hashmap;
        comp_map2_ctor(&hashmap, &arena);

        CompKey key = {0};
        key.a = 36463453;
        key.b = true;

        int32_t insert_res = comp_map2_insert(&hashmap, key, 777777);
        lok(insert_res == true);

        CompKey bad_key = {0};
        bad_key.a = 36463453;
        bad_key.b = false;
        int32_t* get_res = comp_map2_get(&hashmap, bad_key);
        lok(get_res == NULL);

        get_res = comp_map2_get(&hashmap, key);
        lok(*get_res == 777777);
    }

    jss_arena_reset(&arena);

    {
        CompMap3 hashmap;
        comp_map3_ctor(&hashmap, &arena);

        CompKey key = {0};
        key.a = 36463453;
        key.b = true;

        CompValue value = {0};
        value.a = 2.89;
        value.b = 1222121;

        int32_t insert_res = comp_map3_insert(&hashmap, key, value);
        lok(insert_res == true);

        CompKey bad_key = {0};
        bad_key.a = 36463453;
        bad_key.b = false;
        CompValue* get_res = comp_map3_get(&hashmap, bad_key);
        lok(get_res == NULL);

        get_res = comp_map3_get(&hashmap, key);
        lok(memcmp(get_res, &value, sizeof(CompValue)) == 0);
    }

    jss_arena_reset(&arena);
}

void test_delete(void)
{
    jss_arena_reset(&arena);

    {
        MyHashMap hashmap;
        my_map_ctor(&hashmap, &arena);

        bool insert_res = my_map_insert(&hashmap, 567687, 3546757);
        lok(insert_res == true);

        insert_res = my_map_insert(&hashmap, 23940, 3546757);
        lok(insert_res == true);

        insert_res = my_map_insert(&hashmap, 48686, 3546757);
        lok(insert_res == true);

        lok(hashmap.item_count == 3);

        bool delete_res = my_map_delete(&hashmap, 9999999);
        lok(delete_res == false);
        lok(hashmap.item_count == 3);

        delete_res = my_map_delete(&hashmap, 23940);
        lok(delete_res == true);
        lok(hashmap.item_count == 2);

        int32_t count = 0;
        MyHashMapIterator iter = my_map_iterator_start(&hashmap);
        MyHashMapItem* next_item;
        while ((next_item = my_map_iterator_next(&iter)) != NULL)
        {
            lok(next_item->key != 23940);
            ++count;
        }

        lok(count == 2);
        lok(count == hashmap.item_count);
    }

    jss_arena_reset(&arena);

    {
        CompMap hashmap;
        comp_map_ctor(&hashmap, &arena);

        CompValue value;
        value.a = 67.7677;
        value.b = 47621743;
        bool insert_res = comp_map_insert(&hashmap, 567687, value);
        lok(insert_res == true);

        insert_res = comp_map_insert(&hashmap, 23940, value);
        lok(insert_res == true);

        insert_res = comp_map_insert(&hashmap, 48686, value);
        lok(insert_res == true);

        lok(hashmap.item_count == 3);

        bool delete_res = comp_map_delete(&hashmap, 9999999);
        lok(delete_res == false);
        lok(hashmap.item_count == 3);

        delete_res = comp_map_delete(&hashmap, 23940);
        lok(delete_res == true);
        lok(hashmap.item_count == 2);

        int32_t count = 0;
        CompMapIterator iter = comp_map_iterator_start(&hashmap);
        CompMapItem* next_item;
        while ((next_item = comp_map_iterator_next(&iter)) != NULL)
        {
            lok(next_item->key != 23940);
            ++count;
        }

        lok(count == 2);
        lok(count == hashmap.item_count);
    }

    jss_arena_reset(&arena);

    {
        CompMap2 hashmap;
        comp_map2_ctor(&hashmap, &arena);

        CompKey key1 = { .a = 67, .b = false };
        CompKey key2 = { .a = 67, .b = true };
        CompKey key3 = { .a = 1434, .b = true };
        CompKey key4 = { .a = 0, .b = false };

        bool insert_res = comp_map2_insert(&hashmap, key1, 58678568);
        lok(insert_res == true);

        insert_res = comp_map2_insert(&hashmap, key2, 58678568);
        lok(insert_res == true);

        insert_res = comp_map2_insert(&hashmap, key3, 58678568);
        lok(insert_res == true);

        lok(hashmap.item_count == 3);

        bool delete_res = comp_map2_delete(&hashmap, key4);
        lok(delete_res == false);
        lok(hashmap.item_count == 3);

        delete_res = comp_map2_delete(&hashmap, key2);
        lok(delete_res == true);
        lok(hashmap.item_count == 2);

        int32_t count = 0;
        CompMap2Iterator iter = comp_map2_iterator_start(&hashmap);
        CompMap2Item* next_item;
        while ((next_item = comp_map2_iterator_next(&iter)) != NULL)
        {
            lok(memcmp(&next_item->key, &key2, sizeof(CompKey)) != 0);
            ++count;
        }

        lok(count == 2);
        lok(count == hashmap.item_count);
    }

    jss_arena_reset(&arena);

    {
        CompMap3 hashmap;
        comp_map3_ctor(&hashmap, &arena);

        CompKey key1 = { .a = 67, .b = false };
        CompKey key2 = { .a = 67, .b = true };
        CompKey key3 = { .a = 1434, .b = true };
        CompKey key4 = { .a = 0, .b = false };

        CompValue value;
        value.a = 898788789.01;
        value.b = 45627;
        bool insert_res = comp_map3_insert(&hashmap, key1, value);
        lok(insert_res == true);

        insert_res = comp_map3_insert(&hashmap, key2, value);
        lok(insert_res == true);

        insert_res = comp_map3_insert(&hashmap, key3, value);
        lok(insert_res == true);

        lok(hashmap.item_count == 3);

        bool delete_res = comp_map3_delete(&hashmap, key4);
        lok(delete_res == false);
        lok(hashmap.item_count == 3);

        delete_res = comp_map3_delete(&hashmap, key2);
        lok(delete_res == true);
        lok(hashmap.item_count == 2);

        int32_t count = 0;
        CompMap3Iterator iter = comp_map3_iterator_start(&hashmap);
        CompMap3Item* next_item;
        while ((next_item = comp_map3_iterator_next(&iter)) != NULL)
        {
            lok(memcmp(&next_item->key, &key2, sizeof(CompKey)) != 0);
            ++count;
        }

        lok(count == 2);
        lok(count == hashmap.item_count);
    }

    jss_arena_reset(&arena);
}

void test_iterator(void)
{
    jss_arena_reset(&arena);

    {
        MyHashMap hashmap;
        my_map_ctor(&hashmap, &arena);

        for (int32_t i = 0; i < 300; ++i)
        {
            int32_t res = my_map_insert(&hashmap, i, i);
            lok(res == true);
        }

        int32_t count = 0;
        MyHashMapIterator iter = my_map_iterator_start(&hashmap);
        MyHashMapItem* next_item = NULL;
        while ((next_item = my_map_iterator_next(&iter)) != NULL)
        {
            ++count;
        }

        lok(count == 300);

        my_map_delete(&hashmap, 100);

        count = 0;
        iter = my_map_iterator_start(&hashmap);
        while ((next_item = my_map_iterator_next(&iter)) != NULL)
        {
            ++count;
        }

        lok(count == 299);
    }

    jss_arena_reset(&arena);

    {
        CompMap hashmap;
        comp_map_ctor(&hashmap, &arena);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompValue value;
            value.a = 2.89;
            value.b = 1222121;
            int32_t res = comp_map_insert(&hashmap, i, value);
            lok(res == true);
        }

        int32_t count = 0;
        CompMapIterator iter = comp_map_iterator_start(&hashmap);
        CompMapItem* next_item;
        while ((next_item = comp_map_iterator_next(&iter)) != NULL)
        {
            ++count;
        }

        lok(count == 300);

        comp_map_delete(&hashmap, 100);

        count = 0;
        iter = comp_map_iterator_start(&hashmap);
        while ((next_item = comp_map_iterator_next(&iter)) != NULL)
        {
            ++count;
        }

        lok(count == 299);
    }

    jss_arena_reset(&arena);

    {
        CompMap2 hashmap;
        comp_map2_ctor(&hashmap, &arena);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompKey key = {0};
            key.a = i;
            key.b = true;
            int32_t res = comp_map2_insert(&hashmap, key, i);
            lok(res == true);
        }

        int32_t count = 0;
        CompMap2Iterator iter = comp_map2_iterator_start(&hashmap);
        CompMap2Item* next_item;
        while ((next_item = comp_map2_iterator_next(&iter)) != NULL)
        {
            ++count;
        }

        lok(count == 300);

        CompKey delete_key = {0};
        delete_key.a = 100;
        delete_key.b = true;
        comp_map2_delete(&hashmap, delete_key);

        count = 0;
        iter = comp_map2_iterator_start(&hashmap);
        while ((next_item = comp_map2_iterator_next(&iter)) != NULL)
        {
            ++count;
        }

        lok(count == 299);
    }

    jss_arena_reset(&arena);

    {
        CompMap3 hashmap;
        comp_map3_ctor(&hashmap, &arena);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompKey key = {0};
            key.a = i;
            key.b = true;

            CompValue value;
            value.a = 2.89;
            value.b = i;

            int32_t res = comp_map3_insert(&hashmap, key, value);
            lok(res == true);
        }

        int32_t count = 0;
        CompMap3Iterator iter = comp_map3_iterator_start(&hashmap);
        CompMap3Item* next_item;
        while ((next_item = comp_map3_iterator_next(&iter)) != NULL)
        {
            ++count;
        }

        lok(count == 300);

        CompKey delete_key = {0};
        delete_key.a = 100;
        delete_key.b = true;
        comp_map3_delete(&hashmap, delete_key);

        count = 0;
        iter = comp_map3_iterator_start(&hashmap);
        while ((next_item = comp_map3_iterator_next(&iter)) != NULL)
        {
            ++count;
        }

        lok(count == 299);
    }

    jss_arena_reset(&arena);
}

void test_string_insert(void)
{
    jss_arena_reset(&arena);

    StrMap hashmap;
    str_map_ctor(&hashmap, &arena);

    JSLFatPtr key = jss_fatptr_from_cstr("string_key");
    CompValue value;
    value.a = 73465.464567;
    value.b = 51324896;
    int32_t insert_res = str_map_insert(&hashmap, key, value);

    lok(insert_res == true);
    lok(hashmap.item_count == 1);

    jss_arena_reset(&arena);
}

void test_string_get(void)
{
    jss_arena_reset(&arena);

    StrMap hashmap;
    str_map_ctor(&hashmap, &arena);

    JSLFatPtr key1 = jss_fatptr_from_cstr("minister");
    JSLFatPtr key2 = jss_fatptr_from_cstr("agile");
    JSLFatPtr key3 = jss_fatptr_from_cstr("disagreement");
    JSLFatPtr key4 = jss_fatptr_from_cstr("invisible");
    JSLFatPtr key5 = {0};

    CompValue value;
    value.a = 73465.464567;
    value.b = 51324896;

    str_map_insert(&hashmap, key1, value);
    str_map_insert(&hashmap, key2, value);
    str_map_insert(&hashmap, key3, value);

    CompValue* get_res = str_map_get(&hashmap, key1);
    lok(get_res != NULL);
    lok(get_res->b == 51324896);

    get_res = str_map_get(&hashmap, key2);
    lok(get_res != NULL);
    lok(get_res->b == 51324896);

    get_res = str_map_get(&hashmap, key3);
    lok(get_res != NULL);
    lok(get_res->b == 51324896);

    get_res = str_map_get(&hashmap, key4);
    lok(get_res == NULL);

    value.a = 44.333333;
    value.b = 47689;
    str_map_insert(&hashmap, key5, value);
    get_res = str_map_get(&hashmap, key5);
    lok(get_res != NULL);
    lok(get_res->b == 47689);

    jss_arena_reset(&arena);
}

static void rand_str(uint8_t* dest, int32_t length)
{
    uint8_t charset[63] = "0123456789"
                          "abcdefghijklmnopqrstuvwxyz"
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int32_t i = 0; i < length; i++)
    {
        int32_t index = rand() % 62;
        dest[i] = charset[index];
    }
}

void test_string_iterator(void)
{
    jss_arena_reset(&arena);

    uint8_t key_data[32];

    StrMap hashmap;
    str_map_ctor(&hashmap, &arena);
    CompValue value;
    value.a = 73465.464567;
    value.b = 51324896;

    for (int32_t i = 0; i < 300; ++i)
    {
        rand_str(key_data, 32);
        int32_t res = str_map_insert(&hashmap, jss_fatptr_ctor(key_data, 32), value);
        lok(res == true);
    }

    JSLFatPtr null_key = {0};
    str_map_insert(&hashmap, null_key, value);

    int32_t count = 0;
    StrMapIterator iter = str_map_iterator_start(&hashmap);
    StrMapIteratorReturn next_item;
    while ((next_item = str_map_iterator_next(&iter)).value != NULL)
    {
        ++count;
    }

    lok(count == 301);

    jss_arena_reset(&arena);
}

void test_string_delete(void)
{
    jss_arena_reset(&arena);

    StrMap hashmap;
    str_map_ctor(&hashmap, &arena);

    CompValue value;
    value.a = 73465.464567;
    value.b = 51324896;

    JSLFatPtr key1 = jss_fatptr_from_cstr("minister");
    JSLFatPtr key2 = jss_fatptr_from_cstr("agile");
    JSLFatPtr key3 = jss_fatptr_from_cstr("disagreement");
    JSLFatPtr key4 = jss_fatptr_from_cstr("invisible");
    JSLFatPtr key5 = {0};

    bool insert_res = str_map_insert(&hashmap, key1, value);
    lok(insert_res == true);

    insert_res = str_map_insert(&hashmap, key2, value);
    lok(insert_res == true);

    insert_res = str_map_insert(&hashmap, key3, value);
    lok(insert_res == true);

    insert_res = str_map_insert(&hashmap, key5, value);
    lok(insert_res == true);

    lok(hashmap.item_count == 4);

    bool delete_res = str_map_delete(&hashmap, key4);
    lok(delete_res == false);
    lok(hashmap.item_count == 4);

    delete_res = str_map_delete(&hashmap, key2);
    lok(delete_res == true);
    lok(hashmap.item_count == 3);

    int32_t count = 0;
    StrMapIterator iter = str_map_iterator_start(&hashmap);
    StrMapIteratorReturn next_item;
    while ((next_item = str_map_iterator_next(&iter)).value != NULL)
    {
        lok(jss_fatptr_memory_compare(next_item.key, key2) == false);
        ++count;
    }

    lok(count == 3);
    lok(count == hashmap.item_count);
}

void test_string_to_string_insert(void)
{
    jss_arena_reset(&arena);

    JSLStrToStrMap hashmap;
    jss_str_to_str_map_ctor(&hashmap, &arena);

    JSLFatPtr key = jss_fatptr_from_cstr("string_key");
    JSLFatPtr value = jss_fatptr_from_cstr("string_value");
    int32_t insert_res = jss_str_to_str_map_insert(&hashmap, key, value);

    lok(insert_res == true);
    lok(hashmap.item_count == 1);

    jss_arena_reset(&arena);
}

void test_string_to_string_get(void)
{
    jss_arena_reset(&arena);

    JSLStrToStrMap hashmap;
    jss_str_to_str_map_ctor(&hashmap, &arena);

    JSLFatPtr key1 = jss_fatptr_from_cstr("minister");
    JSLFatPtr key2 = jss_fatptr_from_cstr("agile");
    JSLFatPtr key3 = jss_fatptr_from_cstr("disagreement");
    JSLFatPtr key4 = jss_fatptr_from_cstr("invisible");
    JSLFatPtr key5 = {0};
    JSLFatPtr key6 = jss_fatptr_from_cstr("headquarters");

    JSLFatPtr value1 = jss_fatptr_from_cstr("conductor");
    JSLFatPtr value2 = jss_fatptr_from_cstr("participate");
    JSLFatPtr value3 = jss_fatptr_from_cstr("situation");
    JSLFatPtr value4 = jss_fatptr_from_cstr("advocate");
    JSLFatPtr value5 = {0};

    bool insert_res;
    insert_res = jss_str_to_str_map_insert(&hashmap, key1, value1);
    lok(insert_res == true);
    insert_res = jss_str_to_str_map_insert(&hashmap, key2, value2);
    lok(insert_res == true);
    insert_res = jss_str_to_str_map_insert(&hashmap, key3, value3);
    lok(insert_res == true);
    insert_res = jss_str_to_str_map_insert(&hashmap, key5, value4);
    lok(insert_res == true);
    insert_res = jss_str_to_str_map_insert(&hashmap, key6, value5);
    lok(insert_res == true);

    JSLFatPtr get_value;
    bool get_res = jss_str_to_str_map_get(&hashmap, key1, &get_value);
    lok(get_res == true);
    lok(jss_fatptr_memory_compare(get_value, value1));

    get_res = jss_str_to_str_map_get(&hashmap, key2, &get_value);
    lok(get_res == true);
    lok(jss_fatptr_memory_compare(get_value, value2));

    get_res = jss_str_to_str_map_get(&hashmap, key3, &get_value);
    lok(get_res == true);
    lok(jss_fatptr_memory_compare(get_value, value3));

    get_res = jss_str_to_str_map_get(&hashmap, key4, &get_value);
    lok(get_res == false);

    get_res = jss_str_to_str_map_get(&hashmap, key5, &get_value);
    lok(get_res == true);
    lok(jss_fatptr_memory_compare(get_value, value4));

    get_res = jss_str_to_str_map_get(&hashmap, key6, &get_value);
    lok(get_res == true);
    lok(get_value.data == NULL);

    jss_arena_reset(&arena);
}

void test_string_to_string_iterator(void)
{
    jss_arena_reset(&arena);

    uint8_t key_data[32];
    JSLFatPtr value = jss_fatptr_from_cstr("conductor");

    JSLStrToStrMap hashmap;
    jss_str_to_str_map_ctor(&hashmap, &arena);

    for (int32_t i = 0; i < 300; ++i)
    {
        rand_str(key_data, 32);
        int32_t res = jss_str_to_str_map_insert(&hashmap, jss_fatptr_ctor(key_data, 32), value);
        lok(res == true);
    }

    JSLFatPtr null_key = {0};
    jss_str_to_str_map_insert(&hashmap, null_key, value);

    int32_t count = 0;
    JSLStrToStrMapIterator iter = jss_str_to_str_map_iterator_start(&hashmap);
    JSLStrToStrMapIteratorReturn next_item;
    while ((next_item = jss_str_to_str_map_iterator_next(&iter)).value != NULL)
    {
        ++count;
    }

    lok(count == 301);

    jss_arena_reset(&arena);
}

void test_string_to_string_delete(void)
{
    jss_arena_reset(&arena);

    JSLStrToStrMap hashmap;
    jss_str_to_str_map_ctor(&hashmap, &arena);

    JSLFatPtr value = jss_fatptr_from_cstr("conductor");

    JSLFatPtr key1 = jss_fatptr_from_cstr("minister");
    JSLFatPtr key2 = jss_fatptr_from_cstr("agile");
    JSLFatPtr key3 = jss_fatptr_from_cstr("disagreement");
    JSLFatPtr key4 = jss_fatptr_from_cstr("invisible");
    JSLFatPtr key5 = {0};

    bool insert_res = jss_str_to_str_map_insert(&hashmap, key1, value);
    lok(insert_res == true);

    insert_res = jss_str_to_str_map_insert(&hashmap, key2, value);
    lok(insert_res == true);

    insert_res = jss_str_to_str_map_insert(&hashmap, key3, value);
    lok(insert_res == true);

    insert_res = jss_str_to_str_map_insert(&hashmap, key5, value);
    lok(insert_res == true);

    lok(hashmap.item_count == 4);

    bool delete_res = jss_str_to_str_map_delete(&hashmap, key4);
    lok(delete_res == false);
    lok(hashmap.item_count == 4);

    delete_res = jss_str_to_str_map_delete(&hashmap, key2);
    lok(delete_res == true);
    lok(hashmap.item_count == 3);

    int32_t count = 0;
    JSLStrToStrMapIterator iter = jss_str_to_str_map_iterator_start(&hashmap);
    JSLStrToStrMapIteratorReturn next_item;
    while ((next_item = jss_str_to_str_map_iterator_next(&iter)).value != NULL)
    {
        lok(jss_fatptr_memory_compare(next_item.key, key2) == false);
        ++count;
    }

    lok(count == 3);
    lok(count == hashmap.item_count);
}

void test_string_to_string_clear(void)
{
    jss_arena_reset(&arena);

    JSLStrToStrMap hashmap;
    jss_str_to_str_map_ctor(&hashmap, &arena);

    JSLFatPtr value = jss_fatptr_from_cstr("conductor");

    JSLFatPtr key1 = jss_fatptr_from_cstr("minister");
    JSLFatPtr key2 = jss_fatptr_from_cstr("agile");
    JSLFatPtr key3 = {0};

    jss_str_to_str_map_insert(&hashmap, key1, value);
    jss_str_to_str_map_insert(&hashmap, key2, value);
    jss_str_to_str_map_insert(&hashmap, key3, value);

    lok(hashmap.item_count == 3);
    jss_str_to_str_map_clear(&hashmap);
    lok(hashmap.item_count == 0);

    int32_t count = 0;
    JSLStrToStrMapIterator iter = jss_str_to_str_map_iterator_start(&hashmap);
    JSLStrToStrMapIteratorReturn next_item;
    while ((next_item = jss_str_to_str_map_iterator_next(&iter)).value != NULL)
    {
        lok(jss_fatptr_memory_compare(next_item.key, key2) == false);
        ++count;
    }

    lok(count == 0);
}

int main(void)
{
    arena = jss_arena_ctor(malloc(arena_size), arena_size);

    lrun("Test hashmap insert", test_insert);
    lrun("Test hashmap get", test_get);
    lrun("Test hashmap iterator", test_iterator);
    lrun("Test hashmap delete", test_delete);

    lrun("Test string hashmap insert", test_string_insert);
    lrun("Test string hashmap get", test_string_get);
    lrun("Test string hashmap iterator", test_string_iterator);
    lrun("Test string hashmap delete", test_string_delete);

    lrun("Test string to string hashmap insert", test_string_to_string_insert);
    lrun("Test string to string hashmap get", test_string_to_string_get);
    lrun("Test string to string hashmap iterator", test_string_to_string_iterator);
    lrun("Test string to string hashmap delete", test_string_to_string_delete);
    lrun("Test string to string hashmap clear", test_string_to_string_clear);

    lresults();
    return lfails != 0;
}

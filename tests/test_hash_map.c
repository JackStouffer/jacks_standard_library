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
#include "hash_maps/comp2_to_int_map.h"
#include "hash_maps/comp3_to_comp2_map.h"
#include "hash_maps/int32_to_comp1_map.h"
#include "hash_maps/int32_to_int32_map.h"

const int64_t arena_size = 2 * 1024 * 1024;
JSLArena arena;

void test_insert(void)
{
    jsl_arena_reset(&arena);

    {
        IntToIntMap hashmap;
        int32_to_int32_map_init(&hashmap, &arena, 256, 0);

        int32_t insert_res = int32_to_int32_map_insert(&hashmap, 42, 999);

        lok(insert_res == true);
        lok(hashmap.item_count == 1);
    }

    jsl_arena_reset(&arena);

    {
        IntToCompositeType1Map hashmap;
        int32_to_comp1_map_init(&hashmap, &arena, 256, 0);

        CompositeType1 value;
        value.a = 887;
        value.b = 56784587;
        int32_t insert_res = int32_to_comp1_map_insert(&hashmap, 4875847, value);

        lok(insert_res == true);
        lok(hashmap.item_count == 1);
    }

    jsl_arena_reset(&arena);

    {
        CompositeType2ToIntMap hashmap;
        comp2_to_int_map_init(&hashmap, &arena, 256, 0);

        CompositeType2 key;
        key.a = 5497684;
        key.b = 84656;
        key.c = true;
        int32_t insert_res = comp2_to_int_map_insert(&hashmap, key, 849594759);

        lok(insert_res == true);
        lok(hashmap.item_count == 1);
    }

    jsl_arena_reset(&arena);

    {
        CompositeType3ToCompositeType2Map hashmap;
        comp3_to_comp2_map_init(&hashmap, &arena, 256, 0);

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
        int32_t insert_res = comp3_to_comp2_map_insert(&hashmap, key, value);

        lok(insert_res == true);
        lok(hashmap.item_count == 1);
    }

    jsl_arena_reset(&arena);
}

void test_get(void)
{
    jsl_arena_reset(&arena);

    {
        IntToIntMap hashmap;
        int32_to_int32_map_init(&hashmap, &arena, 256, 0);

        int32_t insert_res = int32_to_int32_map_insert(&hashmap, 8976, 1111);
        lok(insert_res == true);

        int32_t* get_res = int32_to_int32_map_get(&hashmap, 1112);
        lok(get_res == NULL);

        get_res = int32_to_int32_map_get(&hashmap, 8976);
        lok(*get_res == 1111);
    }

    jsl_arena_reset(&arena);

    {
        IntToCompositeType1Map hashmap;
        int32_to_comp1_map_init(&hashmap, &arena, 256, 0);

        CompositeType1 value = {0};
        value.a = 887;
        value.b = 56784587;

        int32_t insert_res = int32_to_comp1_map_insert(&hashmap, 585678435, value);
        lok(insert_res == true);

        CompositeType1* get_res = int32_to_comp1_map_get(&hashmap, 809367483);
        lok(get_res == NULL);

        get_res = int32_to_comp1_map_get(&hashmap, 585678435);
        lok(memcmp(get_res, &value, sizeof(CompositeType1)) == 0);
    }

    jsl_arena_reset(&arena);

    {
        CompositeType2ToIntMap hashmap;
        comp2_to_int_map_init(&hashmap, &arena, 256, 0);

        CompositeType2 key = {0};
        key.a = 36463453;
        key.b = true;

        int32_t insert_res = comp2_to_int_map_insert(&hashmap, key, 777777);
        lok(insert_res == true);

        CompositeType2 bad_key = {0};
        bad_key.a = 36463453;
        bad_key.b = false;
        int32_t* get_res = comp2_to_int_map_get(&hashmap, bad_key);
        lok(get_res == NULL);

        get_res = comp2_to_int_map_get(&hashmap, key);
        lok(*get_res == 777777);
    }

    jsl_arena_reset(&arena);

    {
        CompositeType3ToCompositeType2Map hashmap;
        comp3_to_comp2_map_init(&hashmap, &arena, 256, 0);

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

        int32_t insert_res = comp3_to_comp2_map_insert(&hashmap, key, value);
        lok(insert_res == true);

        CompositeType3 bad_key = key;
        bad_key.a = 36463453;
        CompositeType2* get_res = comp3_to_comp2_map_get(&hashmap, bad_key);
        lok(get_res == NULL);

        get_res = comp3_to_comp2_map_get(&hashmap, key);
        lok(memcmp(get_res, &value, sizeof(CompositeType2)) == 0);
    }

    jsl_arena_reset(&arena);
}

void test_delete(void)
{
    jsl_arena_reset(&arena);

    {
        IntToIntMap hashmap;
        int32_to_int32_map_init(&hashmap, &arena, 256, 0);

        bool insert_res = int32_to_int32_map_insert(&hashmap, 567687, 3546757);
        lok(insert_res == true);

        insert_res = int32_to_int32_map_insert(&hashmap, 23940, 3546757);
        lok(insert_res == true);

        insert_res = int32_to_int32_map_insert(&hashmap, 48686, 3546757);
        lok(insert_res == true);

        lok(hashmap.item_count == 3);

        bool delete_res = int32_to_int32_map_delete(&hashmap, 9999999);
        lok(delete_res == false);
        lok(hashmap.item_count == 3);

        delete_res = int32_to_int32_map_delete(&hashmap, 23940);
        lok(delete_res == true);
        lok(hashmap.item_count == 2);

        int64_t count = 0;
        IntToIntMapIterator iter;
        int32_t iter_key;
        int32_t iter_value;
        bool iter_ok = int32_to_int32_map_iterator_start(&hashmap, &iter);
        lok(iter_ok == true);

        while (int32_to_int32_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            lok(iter_key != 23940);
            ++count;
        }

        TEST_INT64_EQUAL(count, (int64_t) 2);
        TEST_INT64_EQUAL(count, hashmap.item_count);
    }

    jsl_arena_reset(&arena);

    {
        IntToCompositeType1Map hashmap;
        int32_to_comp1_map_init(&hashmap, &arena, 256, 0);

        CompositeType1 value;
        value.a = 887;
        value.b = 56784587;
        bool insert_res = int32_to_comp1_map_insert(&hashmap, 567687, value);
        lok(insert_res == true);

        insert_res = int32_to_comp1_map_insert(&hashmap, 23940, value);
        lok(insert_res == true);

        insert_res = int32_to_comp1_map_insert(&hashmap, 48686, value);
        lok(insert_res == true);

        lok(hashmap.item_count == 3);

        bool delete_res = int32_to_comp1_map_delete(&hashmap, 9999999);
        lok(delete_res == false);
        lok(hashmap.item_count == 3);

        delete_res = int32_to_comp1_map_delete(&hashmap, 23940);
        lok(delete_res == true);
        lok(hashmap.item_count == 2);

        int64_t count = 0;

        IntToCompositeType1MapIterator iter;
        int32_t iter_key;
        CompositeType1 iter_value;

        bool iter_ok = int32_to_comp1_map_iterator_start(&hashmap, &iter);
        lok(iter_ok == true);

        lok(iter_ok == true);

        while (int32_to_comp1_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            lok(iter_key != 23940);
            ++count;
        }

        TEST_INT64_EQUAL(count, (int64_t) 2);
        TEST_INT64_EQUAL(count, hashmap.item_count);
    }

    jsl_arena_reset(&arena);

    {
        CompositeType2ToIntMap hashmap;
        comp2_to_int_map_init(&hashmap, &arena, 256, 0);

        CompositeType2 key1 = { .a = 67, .b = false };
        CompositeType2 key2 = { .a = 67, .b = true };
        CompositeType2 key3 = { .a = 1434, .b = true };
        CompositeType2 key4 = { .a = 0, .b = false };

        bool insert_res = comp2_to_int_map_insert(&hashmap, key1, 58678568);
        lok(insert_res == true);

        insert_res = comp2_to_int_map_insert(&hashmap, key2, 58678568);
        lok(insert_res == true);

        insert_res = comp2_to_int_map_insert(&hashmap, key3, 58678568);
        lok(insert_res == true);

        lok(hashmap.item_count == 3);

        bool delete_res = comp2_to_int_map_delete(&hashmap, key4);
        lok(delete_res == false);
        lok(hashmap.item_count == 3);

        delete_res = comp2_to_int_map_delete(&hashmap, key2);
        lok(delete_res == true);
        lok(hashmap.item_count == 2);

        int64_t count = 0;
        CompositeType2ToIntMapIterator iter;
        CompositeType2 iter_key;
        int32_t iter_value;

        bool iter_ok = comp2_to_int_map_iterator_start(&hashmap, &iter);
        lok(iter_ok == true);

        while (comp2_to_int_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            lok(memcmp(&iter_key, &key2, sizeof(CompositeType2)) != 0);
            ++count;
        }

        TEST_INT64_EQUAL(count, (int64_t) 2);
        TEST_INT64_EQUAL(count, hashmap.item_count);
    }

    jsl_arena_reset(&arena);

    {
        CompositeType3ToCompositeType2Map hashmap;
        comp3_to_comp2_map_init(&hashmap, &arena, 256, 0);

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
        bool insert_res = comp3_to_comp2_map_insert(&hashmap, key1, value);
        lok(insert_res == true);

        insert_res = comp3_to_comp2_map_insert(&hashmap, key2, value);
        lok(insert_res == true);

        insert_res = comp3_to_comp2_map_insert(&hashmap, key3, value);
        lok(insert_res == true);

        lok(hashmap.item_count == 3);

        bool delete_res = comp3_to_comp2_map_delete(&hashmap, key4);
        lok(delete_res == false);
        lok(hashmap.item_count == 3);

        delete_res = comp3_to_comp2_map_delete(&hashmap, key2);
        lok(delete_res == true);
        lok(hashmap.item_count == 2);

        int64_t count = 0;
        CompositeType3ToCompositeType2MapIterator iter;
        CompositeType3 iter_key;
        CompositeType2 iter_value;
    
        bool iter_ok = comp3_to_comp2_map_iterator_start(&hashmap, &iter);
        lok(iter_ok == true);

        while (comp3_to_comp2_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            lok(memcmp(&iter_key, &key2, sizeof(CompositeType3)) != 0);
            ++count;
        }

        TEST_INT64_EQUAL(count, (int64_t) 2);
        TEST_INT64_EQUAL(count, hashmap.item_count);
    }

    jsl_arena_reset(&arena);
}

void test_iterator(void)
{
    jsl_arena_reset(&arena);

    {
        IntToIntMap hashmap;
        int32_to_int32_map_init(&hashmap, &arena, 500, 0);

        for (int32_t i = 0; i < 300; ++i)
        {
            int32_t res = int32_to_int32_map_insert(&hashmap, i, i);
            lok(res == true);
        }

        int32_t count = 0;

        IntToIntMapIterator iter;
        int32_t iter_key;
        int32_t iter_value;
        bool iter_ok = int32_to_int32_map_iterator_start(&hashmap, &iter);
        lok(iter_ok == true);

        while (int32_to_int32_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        lequal(count, 300);

        int32_to_int32_map_delete(&hashmap, 100);

        count = 0;
        iter_ok = int32_to_int32_map_iterator_start(&hashmap, &iter);
        lok(iter_ok == true);

        while (int32_to_int32_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        lequal(count, 299);
    }

    jsl_arena_reset(&arena);

    {
        IntToCompositeType1Map hashmap;
        int32_to_comp1_map_init(&hashmap, &arena, 500, 0);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompositeType1 value;
            value.a = 887;
            value.b = 56784587;
            int32_t res = int32_to_comp1_map_insert(&hashmap, i, value);
            lok(res == true);
        }

        int32_t count = 0;
        IntToCompositeType1MapIterator iter;
        bool iter_ok = int32_to_comp1_map_iterator_start(&hashmap, &iter);
        lok(iter_ok);

        int32_t iter_key;
        CompositeType1 iter_value;
        while (int32_to_comp1_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        lequal(count, 300);

        int32_to_comp1_map_delete(&hashmap, 100);

        count = 0;
        iter_ok = int32_to_comp1_map_iterator_start(&hashmap, &iter);
        lok(iter_ok);

        while (int32_to_comp1_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        lequal(count, 299);
    }

    jsl_arena_reset(&arena);

    {
        CompositeType2ToIntMap hashmap;
        comp2_to_int_map_init(&hashmap, &arena, 500, 0);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompositeType2 key = {0};
            key.a = i;
            key.b = 10;
            key.c = true;
            int32_t res = comp2_to_int_map_insert(&hashmap, key, i);
            lok(res == true);
        }

        int32_t count = 0;
        CompositeType2ToIntMapIterator iter;
        bool iter_ok = comp2_to_int_map_iterator_start(&hashmap, &iter);
        lok(iter_ok);

        CompositeType2 iter_key;
        int32_t iter_value;
        while (comp2_to_int_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        lequal(count, 300);

        CompositeType2 delete_key = {0};
        delete_key.a = 100;
        delete_key.b = 10;
        delete_key.c = true;
        bool del_ok = comp2_to_int_map_delete(&hashmap, delete_key);
        lok(del_ok);

        count = 0;
        iter_ok = comp2_to_int_map_iterator_start(&hashmap, &iter);
        lok(iter_ok);

        while (comp2_to_int_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        lequal(count, 299);
    }

    jsl_arena_reset(&arena);

    {
        CompositeType3ToCompositeType2Map hashmap;
        comp3_to_comp2_map_init(&hashmap, &arena, 500, 0);

        for (int32_t i = 0; i < 300; ++i)
        {
            CompositeType3 key = {0};
            key.a = i;

            CompositeType2 value;
            value.a = 887;
            value.b = i;

            int32_t res = comp3_to_comp2_map_insert(&hashmap, key, value);
            lok(res == true);
        }

        int32_t count = 0;
        CompositeType3ToCompositeType2MapIterator iter;
        bool iter_ok = comp3_to_comp2_map_iterator_start(&hashmap, &iter);
        lok(iter_ok);

        CompositeType3 iter_key;
        CompositeType2 iter_value;
        while (comp3_to_comp2_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        lequal(count, 300);

        CompositeType3 delete_key = {0};
        delete_key.a = 100;
        bool del_ok = comp3_to_comp2_map_delete(&hashmap, delete_key);
        lok(del_ok);

        count = 0;
        iter_ok = comp3_to_comp2_map_iterator_start(&hashmap, &iter);
        lok(iter_ok);
        while (comp3_to_comp2_map_iterator_next(&iter, &iter_key, &iter_value))
        {
            ++count;
        }

        lequal(count, 299);
    }

    jsl_arena_reset(&arena);
}

// void test_string_insert(void)
// {
//     jsl_arena_reset(&arena);

//     StrMap hashmap;
//     str_map_ctor(&hashmap, &arena);

//     JSLFatPtr key = jsl_fatptr_from_cstr("string_key");
//     CompositeType1 value;
//     value.a = 887;
//     value.b = 56784587;
//     int32_t insert_res = str_map_insert(&hashmap, key, value);

//     lok(insert_res == true);
//     lok(hashmap.item_count == 1);

//     jsl_arena_reset(&arena);
// }

// void test_string_get(void)
// {
//     jsl_arena_reset(&arena);

//     StrMap hashmap;
//     str_map_ctor(&hashmap, &arena);

//     JSLFatPtr key1 = jsl_fatptr_from_cstr("minister");
//     JSLFatPtr key2 = jsl_fatptr_from_cstr("agile");
//     JSLFatPtr key3 = jsl_fatptr_from_cstr("disagreement");
//     JSLFatPtr key4 = jsl_fatptr_from_cstr("invisible");
//     JSLFatPtr key5 = {0};

//     CompositeType1 value;
//     value.a = 887;
//     value.b = 56784587;

//     str_map_insert(&hashmap, key1, value);
//     str_map_insert(&hashmap, key2, value);
//     str_map_insert(&hashmap, key3, value);

//     CompositeType1* get_res = str_map_get(&hashmap, key1);
//     lok(get_res != NULL);
//     lok(get_res->b == 51324896);

//     get_res = str_map_get(&hashmap, key2);
//     lok(get_res != NULL);
//     lok(get_res->b == 51324896);

//     get_res = str_map_get(&hashmap, key3);
//     lok(get_res != NULL);
//     lok(get_res->b == 51324896);

//     get_res = str_map_get(&hashmap, key4);
//     lok(get_res == NULL);

//     value.a = 44.333333;
//     value.b = 47689;
//     str_map_insert(&hashmap, key5, value);
//     get_res = str_map_get(&hashmap, key5);
//     lok(get_res != NULL);
//     lok(get_res->b == 47689);

//     jsl_arena_reset(&arena);
// }

// static void rand_str(uint8_t* dest, int32_t length)
// {
//     uint8_t charset[63] = "0123456789"
//                           "abcdefghijklmnopqrstuvwxyz"
//                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

//     for (int32_t i = 0; i < length; i++)
//     {
//         int32_t index = rand() % 62;
//         dest[i] = charset[index];
//     }
// }

// void test_string_iterator(void)
// {
//     jsl_arena_reset(&arena);

//     uint8_t key_data[32];

//     StrMap hashmap;
//     str_map_ctor(&hashmap, &arena);
//     CompositeType1 value;
//     value.a = 887;
//     value.b = 56784587;

//     for (int32_t i = 0; i < 300; ++i)
//     {
//         rand_str(key_data, 32);
//         int32_t res = str_map_insert(&hashmap, jsl_fatptr_ctor(key_data, 32), value);
//         lok(res == true);
//     }

//     JSLFatPtr null_key = {0};
//     str_map_insert(&hashmap, null_key, value);

//     int32_t count = 0;
//     StrMapIterator iter = str_map_iterator_start(&hashmap);
//     StrMapIteratorReturn next_item;
//     while ((next_item = str_map_iterator_next(&iter)).value != NULL)
//     {
//         ++count;
//     }

//     lok(count == 301);

//     jsl_arena_reset(&arena);
// }

// void test_string_delete(void)
// {
//     jsl_arena_reset(&arena);

//     StrMap hashmap;
//     str_map_ctor(&hashmap, &arena);

//     CompositeType1 value;
//     value.a = 887;
//     value.b = 56784587;

//     JSLFatPtr key1 = jsl_fatptr_from_cstr("minister");
//     JSLFatPtr key2 = jsl_fatptr_from_cstr("agile");
//     JSLFatPtr key3 = jsl_fatptr_from_cstr("disagreement");
//     JSLFatPtr key4 = jsl_fatptr_from_cstr("invisible");
//     JSLFatPtr key5 = {0};

//     bool insert_res = str_map_insert(&hashmap, key1, value);
//     lok(insert_res == true);

//     insert_res = str_map_insert(&hashmap, key2, value);
//     lok(insert_res == true);

//     insert_res = str_map_insert(&hashmap, key3, value);
//     lok(insert_res == true);

//     insert_res = str_map_insert(&hashmap, key5, value);
//     lok(insert_res == true);

//     lok(hashmap.item_count == 4);

//     bool delete_res = str_map_delete(&hashmap, key4);
//     lok(delete_res == false);
//     lok(hashmap.item_count == 4);

//     delete_res = str_map_delete(&hashmap, key2);
//     lok(delete_res == true);
//     lok(hashmap.item_count == 3);

//     int32_t count = 0;
//     StrMapIterator iter = str_map_iterator_start(&hashmap);
//     StrMapIteratorReturn next_item;
//     while ((next_item = str_map_iterator_next(&iter)).value != NULL)
//     {
//         lok(jsl_fatptr_memory_compare(next_item.key, key2) == false);
//         ++count;
//     }

//     lok(count == 3);
//     lok(count == hashmap.item_count);
// }

// void test_string_to_string_insert(void)
// {
//     jsl_arena_reset(&arena);

//     JSLStrToStrMap hashmap;
//     jsl_str_to_str_map_ctor(&hashmap, &arena);

//     JSLFatPtr key = jsl_fatptr_from_cstr("string_key");
//     JSLFatPtr value = jsl_fatptr_from_cstr("string_value");
//     int32_t insert_res = jsl_str_to_str_map_insert(&hashmap, key, value);

//     lok(insert_res == true);
//     lok(hashmap.item_count == 1);

//     jsl_arena_reset(&arena);
// }

// void test_string_to_string_get(void)
// {
//     jsl_arena_reset(&arena);

//     JSLStrToStrMap hashmap;
//     jsl_str_to_str_map_ctor(&hashmap, &arena);

//     JSLFatPtr key1 = jsl_fatptr_from_cstr("minister");
//     JSLFatPtr key2 = jsl_fatptr_from_cstr("agile");
//     JSLFatPtr key3 = jsl_fatptr_from_cstr("disagreement");
//     JSLFatPtr key4 = jsl_fatptr_from_cstr("invisible");
//     JSLFatPtr key5 = {0};
//     JSLFatPtr key6 = jsl_fatptr_from_cstr("headquarters");

//     JSLFatPtr value1 = jsl_fatptr_from_cstr("conductor");
//     JSLFatPtr value2 = jsl_fatptr_from_cstr("participate");
//     JSLFatPtr value3 = jsl_fatptr_from_cstr("situation");
//     JSLFatPtr value4 = jsl_fatptr_from_cstr("advocate");
//     JSLFatPtr value5 = {0};

//     bool insert_res;
//     insert_res = jsl_str_to_str_map_insert(&hashmap, key1, value1);
//     lok(insert_res == true);
//     insert_res = jsl_str_to_str_map_insert(&hashmap, key2, value2);
//     lok(insert_res == true);
//     insert_res = jsl_str_to_str_map_insert(&hashmap, key3, value3);
//     lok(insert_res == true);
//     insert_res = jsl_str_to_str_map_insert(&hashmap, key5, value4);
//     lok(insert_res == true);
//     insert_res = jsl_str_to_str_map_insert(&hashmap, key6, value5);
//     lok(insert_res == true);

//     JSLFatPtr get_value;
//     bool get_res = jsl_str_to_str_map_get(&hashmap, key1, &get_value);
//     lok(get_res == true);
//     lok(jsl_fatptr_memory_compare(get_value, value1));

//     get_res = jsl_str_to_str_map_get(&hashmap, key2, &get_value);
//     lok(get_res == true);
//     lok(jsl_fatptr_memory_compare(get_value, value2));

//     get_res = jsl_str_to_str_map_get(&hashmap, key3, &get_value);
//     lok(get_res == true);
//     lok(jsl_fatptr_memory_compare(get_value, value3));

//     get_res = jsl_str_to_str_map_get(&hashmap, key4, &get_value);
//     lok(get_res == false);

//     get_res = jsl_str_to_str_map_get(&hashmap, key5, &get_value);
//     lok(get_res == true);
//     lok(jsl_fatptr_memory_compare(get_value, value4));

//     get_res = jsl_str_to_str_map_get(&hashmap, key6, &get_value);
//     lok(get_res == true);
//     lok(get_value.data == NULL);

//     jsl_arena_reset(&arena);
// }

// void test_string_to_string_iterator(void)
// {
//     jsl_arena_reset(&arena);

//     uint8_t key_data[32];
//     JSLFatPtr value = jsl_fatptr_from_cstr("conductor");

//     JSLStrToStrMap hashmap;
//     jsl_str_to_str_map_ctor(&hashmap, &arena);

//     for (int32_t i = 0; i < 300; ++i)
//     {
//         rand_str(key_data, 32);
//         int32_t res = jsl_str_to_str_map_insert(&hashmap, jsl_fatptr_ctor(key_data, 32), value);
//         lok(res == true);
//     }

//     JSLFatPtr null_key = {0};
//     jsl_str_to_str_map_insert(&hashmap, null_key, value);

//     int32_t count = 0;
//     JSLStrToStrMapIterator iter = jsl_str_to_str_map_iterator_start(&hashmap);
//     JSLStrToStrMapIteratorReturn next_item;
//     while ((next_item = jsl_str_to_str_map_iterator_next(&iter)).value != NULL)
//     {
//         ++count;
//     }

//     lok(count == 301);

//     jsl_arena_reset(&arena);
// }

// void test_string_to_string_delete(void)
// {
//     jsl_arena_reset(&arena);

//     JSLStrToStrMap hashmap;
//     jsl_str_to_str_map_ctor(&hashmap, &arena);

//     JSLFatPtr value = jsl_fatptr_from_cstr("conductor");

//     JSLFatPtr key1 = jsl_fatptr_from_cstr("minister");
//     JSLFatPtr key2 = jsl_fatptr_from_cstr("agile");
//     JSLFatPtr key3 = jsl_fatptr_from_cstr("disagreement");
//     JSLFatPtr key4 = jsl_fatptr_from_cstr("invisible");
//     JSLFatPtr key5 = {0};

//     bool insert_res = jsl_str_to_str_map_insert(&hashmap, key1, value);
//     lok(insert_res == true);

//     insert_res = jsl_str_to_str_map_insert(&hashmap, key2, value);
//     lok(insert_res == true);

//     insert_res = jsl_str_to_str_map_insert(&hashmap, key3, value);
//     lok(insert_res == true);

//     insert_res = jsl_str_to_str_map_insert(&hashmap, key5, value);
//     lok(insert_res == true);

//     lok(hashmap.item_count == 4);

//     bool delete_res = jsl_str_to_str_map_delete(&hashmap, key4);
//     lok(delete_res == false);
//     lok(hashmap.item_count == 4);

//     delete_res = jsl_str_to_str_map_delete(&hashmap, key2);
//     lok(delete_res == true);
//     lok(hashmap.item_count == 3);

//     int32_t count = 0;
//     JSLStrToStrMapIterator iter = jsl_str_to_str_map_iterator_start(&hashmap);
//     JSLStrToStrMapIteratorReturn next_item;
//     while ((next_item = jsl_str_to_str_map_iterator_next(&iter)).value != NULL)
//     {
//         lok(jsl_fatptr_memory_compare(next_item.key, key2) == false);
//         ++count;
//     }

//     lok(count == 3);
//     lok(count == hashmap.item_count);
// }

// void test_string_to_string_clear(void)
// {
//     jsl_arena_reset(&arena);

//     JSLStrToStrMap hashmap;
//     jsl_str_to_str_map_ctor(&hashmap, &arena);

//     JSLFatPtr value = jsl_fatptr_from_cstr("conductor");

//     JSLFatPtr key1 = jsl_fatptr_from_cstr("minister");
//     JSLFatPtr key2 = jsl_fatptr_from_cstr("agile");
//     JSLFatPtr key3 = {0};

//     jsl_str_to_str_map_insert(&hashmap, key1, value);
//     jsl_str_to_str_map_insert(&hashmap, key2, value);
//     jsl_str_to_str_map_insert(&hashmap, key3, value);

//     lok(hashmap.item_count == 3);
//     jsl_str_to_str_map_clear(&hashmap);
//     lok(hashmap.item_count == 0);

//     int32_t count = 0;
//     JSLStrToStrMapIterator iter = jsl_str_to_str_map_iterator_start(&hashmap);
//     JSLStrToStrMapIteratorReturn next_item;
//     while ((next_item = jsl_str_to_str_map_iterator_next(&iter)).value != NULL)
//     {
//         lok(jsl_fatptr_memory_compare(next_item.key, key2) == false);
//         ++count;
//     }

//     lok(count == 0);
// }

int main(void)
{
    jsl_arena_init(&arena, malloc(arena_size), arena_size);

    lrun("Test hashmap insert", test_insert);
    lrun("Test hashmap get", test_get);
    lrun("Test hashmap iterator", test_iterator);
    lrun("Test hashmap delete", test_delete);

    // lrun("Test string hashmap insert", test_string_insert);
    // lrun("Test string hashmap get", test_string_get);
    // lrun("Test string hashmap iterator", test_string_iterator);
    // lrun("Test string hashmap delete", test_string_delete);

    // lrun("Test string to string hashmap insert", test_string_to_string_insert);
    // lrun("Test string to string hashmap get", test_string_to_string_get);
    // lrun("Test string to string hashmap iterator", test_string_to_string_iterator);
    // lrun("Test string to string hashmap delete", test_string_to_string_delete);
    // lrun("Test string to string hashmap clear", test_string_to_string_clear);

    lresults();
    return lfails != 0;
}

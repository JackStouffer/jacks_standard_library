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
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "../src/jsl_core.h"
#include "../src/jsl_allocator.h"
#include "../src/jsl_allocator_arena.h"
#include "../src/jsl_str_to_str_multimap.h"

#include "minctest.h"

JSLArena global_arena;

typedef struct ExpectedPair {
    JSLFatPtr key;
    JSLFatPtr value;
    bool seen;
} ExpectedPair;

static JSLFatPtr random_string(int64_t len)
{
    static const uint8_t charset[] =
        u8"abcdefghijklmnopqrstuvwxyz"
        u8"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        u8"0123456789";

    void* buf = malloc((size_t) len);
    JSLFatPtr res = jsl_fatptr_init(buf, len);
    int64_t charset_size = (int64_t) sizeof(charset) - 1;
    for (int64_t i = 0; i < len; i++) {
        res.data[i] = charset[rand() % charset_size];
    }
    return res;
}

static void test_jsl_str_to_str_multimap_init_success(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 0xDEADBEEFUL, 8, 0.5f);
    TEST_BOOL(ok);
    TEST_POINTERS_EQUAL(map.allocator, &allocator);
}

static void test_jsl_str_to_str_multimap_init_invalid_arguments(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    TEST_BOOL(!jsl_str_to_str_multimap_init2(NULL, &allocator, 0, 4, 0.5f));
    TEST_BOOL(!jsl_str_to_str_multimap_init2(&map, NULL, 0, 4, 0.5f));
    TEST_BOOL(!jsl_str_to_str_multimap_init2(&map, &allocator, 0, 0, 0.5f));
    TEST_BOOL(!jsl_str_to_str_multimap_init2(&map, &allocator, 0, -4, 0.5f));
    TEST_BOOL(!jsl_str_to_str_multimap_init2(&map, &allocator, 0, 4, 0.0f));
    TEST_BOOL(!jsl_str_to_str_multimap_init2(&map, &allocator, 0, 4, 1.1f));
    TEST_BOOL(!jsl_str_to_str_multimap_init2(&map, &allocator, 0, 4, -0.5f));
}

static void test_jsl_str_to_str_multimap_insert_and_get_value_count(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 42, 8, 0.65f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key1 = JSL_FATPTR_INITIALIZER("alpha");
    JSLFatPtr key2 = JSL_FATPTR_INITIALIZER("beta");
    JSLFatPtr missing = JSL_FATPTR_INITIALIZER("missing");

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key1, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("one"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key1, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("two"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key2, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("three"), JSL_STRING_LIFETIME_STATIC));

    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, key1), (int64_t) 2);
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, key2), (int64_t) 1);
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, missing), (int64_t) 0);

    JSLStrToStrMultimap uninitialized = {0};
    TEST_BOOL(!jsl_str_to_str_multimap_insert(&uninitialized, key1, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("value"), JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&uninitialized, key1), (int64_t) -1);
}

static void test_jsl_str_to_str_multimap_duplicate_values_allowed(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 7, 4, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("dup-key");
    JSLFatPtr value = JSL_FATPTR_INITIALIZER("repeat");

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, value, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, value, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("unique"), JSL_STRING_LIFETIME_STATIC));

    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, key), (int64_t) 3);

    JSLStrToStrMultimapValueIter iter;
    jsl_str_to_str_multimap_get_values_for_key_iterator_init(&map, &iter, key);

    int32_t repeat_seen = 0;
    int32_t unique_seen = 0;
    JSLFatPtr val;
    while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &val))
    {
        if (jsl_fatptr_memory_compare(val, value))
            repeat_seen++;
        else if (jsl_fatptr_memory_compare(val, JSL_FATPTR_EXPRESSION("unique")))
            unique_seen++;
    }

    TEST_INT32_EQUAL(repeat_seen, 2);
    TEST_INT32_EQUAL(unique_seen, 1);
}

static void test_jsl_str_to_str_multimap_transient_lifetime_copies(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 123, 4, 0.75f);
    TEST_BOOL(ok);
    if (!ok) return;

    char key_buffer[] = "transient-key";
    char value_buffer[] = "transient-value";
    JSLFatPtr key = jsl_fatptr_from_cstr(key_buffer);
    JSLFatPtr value = jsl_fatptr_from_cstr(value_buffer);

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_TRANSIENT, value, JSL_STRING_LIFETIME_TRANSIENT));

    key_buffer[0] = 'X';
    value_buffer[0] = 'Z';

    JSLStrToStrMultimapKeyValueIter iter;
    jsl_str_to_str_multimap_key_value_iterator_init(&map, &iter);

    bool found = false;
    JSLFatPtr out_key = {0};
    JSLFatPtr out_value = {0};
    while (jsl_str_to_str_multimap_key_value_iterator_next(&iter, &out_key, &out_value))
    {
        if (jsl_fatptr_memory_compare(out_key, JSL_FATPTR_EXPRESSION("transient-key")))
        {
            found = true;
            TEST_BOOL(jsl_fatptr_memory_compare(out_value, JSL_FATPTR_EXPRESSION("transient-value")));
            TEST_BOOL(out_key.data != (uint8_t*) key_buffer);
            TEST_BOOL(out_value.data != (uint8_t*) value_buffer);
            break;
        }
    }

    TEST_BOOL(found);
}

static void test_jsl_str_to_str_multimap_static_lifetime_no_copy(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 789, 4, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("static-key");
    JSLFatPtr value = JSL_FATPTR_INITIALIZER("static-value");

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, value, JSL_STRING_LIFETIME_STATIC));

    JSLStrToStrMultimapKeyValueIter iter;
    jsl_str_to_str_multimap_key_value_iterator_init(&map, &iter);

    JSLFatPtr out_key = {0};
    JSLFatPtr out_value = {0};
    bool found = jsl_str_to_str_multimap_key_value_iterator_next(&iter, &out_key, &out_value);
    TEST_BOOL(found);
    if (!found) return;

    TEST_POINTERS_EQUAL(out_key.data, key.data);
    TEST_POINTERS_EQUAL(out_value.data, value.data);
    TEST_BOOL(jsl_fatptr_memory_compare(out_key, key));
    TEST_BOOL(jsl_fatptr_memory_compare(out_value, value));
}

static void test_jsl_str_to_str_multimap_key_value_iterator_covers_all_pairs(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 555, 8, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    ExpectedPair expected[] = {
        {JSL_FATPTR_INITIALIZER("a"), JSL_FATPTR_INITIALIZER("1"), false},
        {JSL_FATPTR_INITIALIZER("b"), JSL_FATPTR_INITIALIZER("2"), false},
        {JSL_FATPTR_INITIALIZER("a"), JSL_FATPTR_INITIALIZER("3"), false},
        {JSL_FATPTR_INITIALIZER("c"), JSL_FATPTR_INITIALIZER("4"), false}
    };

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        TEST_BOOL(jsl_str_to_str_multimap_insert(
            &map,
            expected[i].key,
            JSL_STRING_LIFETIME_STATIC,
            expected[i].value,
            JSL_STRING_LIFETIME_STATIC
        ));
    }

    JSLStrToStrMultimapKeyValueIter iter;
    jsl_str_to_str_multimap_key_value_iterator_init(&map, &iter);

    size_t seen = 0;
    JSLFatPtr out_key = {0};
    JSLFatPtr out_value = {0};
    while (jsl_str_to_str_multimap_key_value_iterator_next(&iter, &out_key, &out_value))
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

    TEST_BOOL(!jsl_str_to_str_multimap_key_value_iterator_next(&iter, &out_key, &out_value));
}

static void test_jsl_str_to_str_multimap_get_key_iterator_filters_by_key(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 1010, 6, 0.6f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("key");
    JSLFatPtr other_key = JSL_FATPTR_INITIALIZER("other");

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("A"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("B"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, other_key, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("C"), JSL_STRING_LIFETIME_STATIC));

    JSLStrToStrMultimapValueIter iter;
    jsl_str_to_str_multimap_get_values_for_key_iterator_init(&map, &iter, key);

    bool saw_a = false;
    bool saw_b = false;
    JSLFatPtr val;
    while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &val))
    {
        if (jsl_fatptr_memory_compare(val, JSL_FATPTR_EXPRESSION("A")))
            saw_a = true;
        else if (jsl_fatptr_memory_compare(val, JSL_FATPTR_EXPRESSION("B")))
            saw_b = true;
        else
            TEST_BOOL(false);
    }

    TEST_BOOL(saw_a);
    TEST_BOOL(saw_b);

    jsl_str_to_str_multimap_get_values_for_key_iterator_init(&map, &iter, JSL_FATPTR_EXPRESSION("does-not-exist"));
    bool has_data = jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &val);
    TEST_BOOL(has_data == false);
}

static void test_jsl_str_to_str_multimap_handles_empty_and_binary_values(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 2020, 4, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr empty_key = JSL_FATPTR_INITIALIZER("");
    JSLFatPtr empty_value = JSL_FATPTR_INITIALIZER("");
    JSLFatPtr bin_key = JSL_FATPTR_INITIALIZER("bin");
    uint8_t binary_value_buf[] = { 'A', 0x00, 'B', 0x7F };
    JSLFatPtr binary_value = jsl_fatptr_init(binary_value_buf, 4);

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, empty_key, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("empty-key"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, bin_key, JSL_STRING_LIFETIME_STATIC, binary_value, JSL_STRING_LIFETIME_TRANSIENT));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, bin_key, JSL_STRING_LIFETIME_STATIC, empty_value, JSL_STRING_LIFETIME_STATIC));

    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, empty_key), (int64_t) 1);
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, bin_key), (int64_t) 2);

    JSLStrToStrMultimapValueIter iter;
    jsl_str_to_str_multimap_get_values_for_key_iterator_init(&map, &iter, empty_key);
    JSLFatPtr val;
    bool has_data = jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &val);
    TEST_BOOL(has_data);
    TEST_BOOL(jsl_fatptr_memory_compare(val, JSL_FATPTR_EXPRESSION("empty-key")));

    TEST_BOOL(jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &val) == false);

    jsl_str_to_str_multimap_get_values_for_key_iterator_init(&map, &iter, bin_key);
    bool saw_binary = false;
    bool saw_empty = false;

    while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &val))
    {
        if (val.length == 4 && memcmp(val.data, binary_value_buf, 4) == 0)
            saw_binary = true;
        else if (val.length == 0)
            saw_empty = true;
    }

    TEST_BOOL(saw_binary);
    TEST_BOOL(saw_empty);
}

static void test_jsl_str_to_str_multimap_delete_value(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 3030, 8, 0.55f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("numbers");
    JSLFatPtr one = JSL_FATPTR_INITIALIZER("one");
    JSLFatPtr two = JSL_FATPTR_INITIALIZER("two");

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, one, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, two, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, two, JSL_STRING_LIFETIME_STATIC));

    TEST_BOOL(!jsl_str_to_str_multimap_delete_value(&map, JSL_FATPTR_EXPRESSION("missing"), one));
    TEST_BOOL(!jsl_str_to_str_multimap_delete_value(&map, key, JSL_FATPTR_EXPRESSION("missing")));

    TEST_BOOL(jsl_str_to_str_multimap_delete_value(&map, key, two));
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, key), (int64_t) 2);

    TEST_BOOL(jsl_str_to_str_multimap_delete_value(&map, key, two));
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, key), (int64_t) 1);

    TEST_BOOL(!jsl_str_to_str_multimap_delete_value(&map, key, two));
    TEST_BOOL(jsl_str_to_str_multimap_delete_value(&map, key, one));
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, key), (int64_t) 0);
    TEST_BOOL(!jsl_str_to_str_multimap_delete_value(&map, key, one));
}

static void test_jsl_str_to_str_multimap_delete_value_removes_empty_key(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 6060, 6, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr key = JSL_FATPTR_INITIALIZER("letters");
    JSLFatPtr a = JSL_FATPTR_INITIALIZER("a");
    JSLFatPtr b = JSL_FATPTR_INITIALIZER("b");
    JSLFatPtr other_key = JSL_FATPTR_INITIALIZER("other");
    JSLFatPtr other_value = JSL_FATPTR_INITIALIZER("value");

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, a, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, key, JSL_STRING_LIFETIME_STATIC, b, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, other_key, JSL_STRING_LIFETIME_STATIC, other_value, JSL_STRING_LIFETIME_STATIC));

    TEST_BOOL(jsl_str_to_str_multimap_has_key(&map, key));

    TEST_BOOL(jsl_str_to_str_multimap_delete_value(&map, key, a));
    TEST_BOOL(jsl_str_to_str_multimap_has_key(&map, key));
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, key), (int64_t) 1);

    TEST_BOOL(jsl_str_to_str_multimap_delete_value(&map, key, b));
    TEST_BOOL(!jsl_str_to_str_multimap_has_key(&map, key));
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, key), (int64_t) 0);

    JSLStrToStrMultimapKeyValueIter iter;
    jsl_str_to_str_multimap_key_value_iterator_init(&map, &iter);
    JSLFatPtr out_key = {0};
    JSLFatPtr out_val = {0};
    bool saw_other = false;
    while (jsl_str_to_str_multimap_key_value_iterator_next(&iter, &out_key, &out_val))
    {
        TEST_BOOL(jsl_fatptr_memory_compare(out_key, other_key));
        TEST_BOOL(jsl_fatptr_memory_compare(out_val, other_value));
        saw_other = true;
    }
    TEST_BOOL(saw_other);
}

static void test_jsl_str_to_str_multimap_delete_key(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 4040, 6, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr keep = JSL_FATPTR_INITIALIZER("keep");
    JSLFatPtr drop = JSL_FATPTR_INITIALIZER("drop");

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, keep, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("1"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, drop, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("a"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, drop, JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("b"), JSL_STRING_LIFETIME_STATIC));

    TEST_BOOL(jsl_str_to_str_multimap_delete_key(&map, drop));
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, drop), (int64_t) 0);
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, keep), (int64_t) 1);

    JSLStrToStrMultimapKeyValueIter iter;
    jsl_str_to_str_multimap_key_value_iterator_init(&map, &iter);
    JSLFatPtr out_key = {0};
    JSLFatPtr out_val = {0};
    int seen_keep = 0;
    while (jsl_str_to_str_multimap_key_value_iterator_next(&iter, &out_key, &out_val))
    {
        TEST_BOOL(jsl_fatptr_memory_compare(out_key, keep));
        seen_keep++;
    }
    TEST_INT64_EQUAL((int64_t) seen_keep, (int64_t) 1);

    TEST_BOOL(!jsl_str_to_str_multimap_delete_key(&map, drop));
    TEST_BOOL(!jsl_str_to_str_multimap_delete_key(&map, JSL_FATPTR_EXPRESSION("none")));
}

static void test_jsl_str_to_str_multimap_clear(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init2(&map, &allocator, 5050, 10, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, JSL_FATPTR_EXPRESSION("x"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("1"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, JSL_FATPTR_EXPRESSION("y"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("2"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, JSL_FATPTR_EXPRESSION("y"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("3"), JSL_STRING_LIFETIME_STATIC));

    jsl_str_to_str_multimap_clear(&map);

    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, JSL_FATPTR_EXPRESSION("x")), (int64_t) 0);
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, JSL_FATPTR_EXPRESSION("y")), (int64_t) 0);

    JSLStrToStrMultimapKeyValueIter iter;
    jsl_str_to_str_multimap_key_value_iterator_init(&map, &iter);
    JSLFatPtr out_key = {0};
    JSLFatPtr out_value = {0};
    TEST_BOOL(!jsl_str_to_str_multimap_key_value_iterator_next(&iter, &out_key, &out_value));

    TEST_BOOL(jsl_str_to_str_multimap_insert(&map, JSL_FATPTR_EXPRESSION("z"), JSL_STRING_LIFETIME_STATIC, JSL_FATPTR_EXPRESSION("4"), JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_to_str_multimap_get_value_count_for_key(&map, JSL_FATPTR_EXPRESSION("z")), (int64_t) 1);
}

static void test_stress_test(void)
{
    JSLStrToStrMultimap map = {0};
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_str_to_str_multimap_init(&map, &allocator, 0);
    TEST_BOOL(ok);
    if (!ok) return;

    int64_t key_count = 10000;
    int64_t value_per_key = 30;

    for (int32_t i = 0; i < key_count; ++i)
    {
        JSLFatPtr key = jsl_format(&allocator, JSL_FATPTR_EXPRESSION("%d"), i);
        
        for (int64_t j = 0; j < value_per_key; ++j)
        {
            int value_len = rand() % 64;
            JSLFatPtr value = random_string(JSL_MAX(1, value_len));

            bool insert_res = jsl_str_to_str_multimap_insert(
                &map,
                key, JSL_STRING_LIFETIME_STATIC,
                value, JSL_STRING_LIFETIME_STATIC
            );
            TEST_BOOL(insert_res);
        }

        TEST_INT64_EQUAL(
            jsl_str_to_str_multimap_get_value_count_for_key(&map, key),
            value_per_key
        );
    }

    TEST_INT64_EQUAL(
        jsl_str_to_str_multimap_get_key_count(&map),
        key_count
    );
    TEST_INT64_EQUAL(
        jsl_str_to_str_multimap_get_value_count(&map),
        key_count * value_per_key
    );

    int64_t seen_value_count = 0;

    JSLStrToStrMultimapKeyValueIter iter;
    jsl_str_to_str_multimap_key_value_iterator_init(&map, &iter);
    JSLFatPtr out_key = {0};
    JSLFatPtr out_val = {0};
    while (jsl_str_to_str_multimap_key_value_iterator_next(&iter, &out_key, &out_val))
    {
        seen_value_count++;
    }

    TEST_INT64_EQUAL(seen_value_count, key_count * value_per_key);
}

int main(void)
{
    srand((unsigned) time(NULL));

    jsl_arena_init(&global_arena, malloc(JSL_MEGABYTES(32)), JSL_MEGABYTES(32));

    RUN_TEST_FUNCTION("init success", test_jsl_str_to_str_multimap_init_success);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("init invalid args", test_jsl_str_to_str_multimap_init_invalid_arguments);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("insert and value count", test_jsl_str_to_str_multimap_insert_and_get_value_count);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("duplicate values", test_jsl_str_to_str_multimap_duplicate_values_allowed);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("transient lifetime copies data", test_jsl_str_to_str_multimap_transient_lifetime_copies);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("static lifetime uses original pointers", test_jsl_str_to_str_multimap_static_lifetime_no_copy);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("key/value iterator covers all", test_jsl_str_to_str_multimap_key_value_iterator_covers_all_pairs);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("get-key iterator filters", test_jsl_str_to_str_multimap_get_key_iterator_filters_by_key);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("empty strings and binary values", test_jsl_str_to_str_multimap_handles_empty_and_binary_values);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("delete value behavior", test_jsl_str_to_str_multimap_delete_value);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("delete value removes empty key", test_jsl_str_to_str_multimap_delete_value_removes_empty_key);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("delete key behavior", test_jsl_str_to_str_multimap_delete_key);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("clear and reuse", test_jsl_str_to_str_multimap_clear);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("stress test", test_stress_test);
    jsl_arena_reset(&global_arena);

    TEST_RESULTS();
    return lfails != 0;
}

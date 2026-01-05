/**
 * Copyright (c) 2026 Jack Stouffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
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
#include <stdio.h>

#include "../src/jsl_core.h"
#include "../src/jsl_str_set.h"

#include "minctest.h"

const int64_t arena_size = JSL_MEGABYTES(32);
JSLArena global_arena;

typedef struct ExpectedValue {
    JSLFatPtr value;
    bool seen;
} ExpectedValue;

static bool insert_values(JSLStrSet* set, const char** values, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        JSLFatPtr value = jsl_fatptr_from_cstr(values[i]);
        if (!jsl_str_set_insert(set, value, JSL_STRING_LIFETIME_STATIC))
        {
            return false;
        }
    }

    return true;
}

static void test_jsl_str_set_init_success(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init2(&set, &global_arena, 0xABCDULL, 10, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    TEST_POINTERS_EQUAL(set.arena, &global_arena);
    TEST_UINT64_EQUAL(set.hash_seed, 0xABCDULL);
    TEST_F32_EQUAL(set.load_factor, 0.5f);
    TEST_BOOL(set.entry_lookup_table != NULL);
    TEST_INT64_EQUAL(set.entry_lookup_table_length, jsl_next_power_of_two_i64(33));
    TEST_INT64_EQUAL(set.item_count, (int64_t) 0);
    TEST_INT64_EQUAL(set.tombstone_count, (int64_t) 0);
}

static void test_jsl_str_set_init_invalid_arguments(void)
{
    JSLStrSet set = {0};

    TEST_BOOL(!jsl_str_set_init2(NULL, &global_arena, 0, 4, 0.5f));
    TEST_BOOL(!jsl_str_set_init2(&set, NULL, 0, 4, 0.5f));
    TEST_BOOL(!jsl_str_set_init2(&set, &global_arena, 0, 0, 0.5f));
    TEST_BOOL(!jsl_str_set_init2(&set, &global_arena, 0, -1, 0.5f));
    TEST_BOOL(!jsl_str_set_init2(&set, &global_arena, 0, 4, 0.0f));
    TEST_BOOL(!jsl_str_set_init2(&set, &global_arena, 0, 4, 1.0f));
    TEST_BOOL(!jsl_str_set_init2(&set, &global_arena, 0, 4, -0.25f));
}

static void test_jsl_str_set_insert_and_has(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init2(&set, &global_arena, 42, 8, 0.75f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr alpha = JSL_FATPTR_INITIALIZER("alpha");
    JSLFatPtr beta = JSL_FATPTR_INITIALIZER("beta");
    JSLFatPtr missing = JSL_FATPTR_INITIALIZER("missing");

    TEST_BOOL(!jsl_str_set_has(&set, alpha));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) 0);

    TEST_BOOL(jsl_str_set_insert(&set, alpha, JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) 1);
    TEST_BOOL(jsl_str_set_has(&set, alpha));

    TEST_BOOL(jsl_str_set_insert(&set, beta, JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) 2);
    TEST_BOOL(jsl_str_set_has(&set, beta));

    TEST_BOOL(jsl_str_set_insert(&set, alpha, JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) 2);

    TEST_BOOL(!jsl_str_set_has(&set, missing));

    JSLStrSet uninitialized = {0};
    TEST_BOOL(!jsl_str_set_has(&uninitialized, alpha));
    TEST_INT64_EQUAL(jsl_str_set_item_count(NULL), (int64_t) -1);
}

static void test_jsl_str_set_respects_lifetime_rules(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init2(&set, &global_arena, 7, 4, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    char small_buffer[] = "short-string";
    char long_buffer[] = "this string is definitely longer than sixteen chars";
    JSLFatPtr small_value = jsl_fatptr_from_cstr(small_buffer);
    JSLFatPtr long_value = jsl_fatptr_from_cstr(long_buffer);
    JSLFatPtr literal_value = JSL_FATPTR_INITIALIZER("literal-static");

    TEST_BOOL(jsl_str_set_insert(&set, small_value, JSL_STRING_LIFETIME_TRANSIENT));
    TEST_BOOL(jsl_str_set_insert(&set, long_value, JSL_STRING_LIFETIME_TRANSIENT));
    TEST_BOOL(jsl_str_set_insert(&set, literal_value, JSL_STRING_LIFETIME_STATIC));

    small_buffer[0] = 'Z';
    long_buffer[0] = 'Y';

    JSLStrSetKeyValueIter iter;
    TEST_BOOL(jsl_str_set_iterator_init(&set, &iter));

    bool saw_small = false;
    bool saw_long = false;
    bool saw_literal = false;
    JSLFatPtr out_value = {0};
    while (jsl_str_set_iterator_next(&iter, &out_value))
    {
        if (jsl_fatptr_memory_compare(out_value, JSL_FATPTR_EXPRESSION("short-string")))
        {
            saw_small = true;
            TEST_BOOL(out_value.data != (uint8_t*) small_buffer);
        }
        else if (jsl_fatptr_memory_compare(out_value, JSL_FATPTR_EXPRESSION("this string is definitely longer than sixteen chars")))
        {
            saw_long = true;
            TEST_BOOL(out_value.data != (uint8_t*) long_buffer);
        }
        else if (jsl_fatptr_memory_compare(out_value, literal_value))
        {
            saw_literal = true;
            TEST_POINTERS_EQUAL(out_value.data, literal_value.data);
        }
    }

    TEST_BOOL(saw_small);
    TEST_BOOL(saw_long);
    TEST_BOOL(saw_literal);
}

static void test_jsl_str_set_iterator_covers_all_values(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init2(&set, &global_arena, 99, 6, 0.6f);
    TEST_BOOL(ok);
    if (!ok) return;

    ExpectedValue expected[] = {
        { JSL_FATPTR_INITIALIZER("a"), false },
        { JSL_FATPTR_INITIALIZER("b"), false },
        { JSL_FATPTR_INITIALIZER("c"), false },
        { JSL_FATPTR_INITIALIZER("d"), false }
    };

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
    {
        TEST_BOOL(jsl_str_set_insert(&set, expected[i].value, JSL_STRING_LIFETIME_STATIC));
    }

    JSLStrSetKeyValueIter iter;
    TEST_BOOL(jsl_str_set_iterator_init(&set, &iter));

    size_t seen = 0;
    JSLFatPtr out_value = {0};
    while (jsl_str_set_iterator_next(&iter, &out_value))
    {
        bool matched = false;
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
        {
            if (!expected[i].seen && jsl_fatptr_memory_compare(out_value, expected[i].value))
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

    TEST_BOOL(!jsl_str_set_iterator_next(&iter, &out_value));
}

static void test_jsl_str_set_iterator_invalidated_on_mutation(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init(&set, &global_arena, 1111);
    TEST_BOOL(ok);
    if (!ok) return;

    TEST_BOOL(jsl_str_set_insert(&set, JSL_FATPTR_EXPRESSION("first"), JSL_STRING_LIFETIME_STATIC));

    JSLStrSetKeyValueIter iter;
    TEST_BOOL(jsl_str_set_iterator_init(&set, &iter));

    TEST_BOOL(jsl_str_set_insert(&set, JSL_FATPTR_EXPRESSION("second"), JSL_STRING_LIFETIME_STATIC));

    JSLFatPtr out_value = {0};
    TEST_BOOL(!jsl_str_set_iterator_next(&iter, &out_value));
}

static void test_jsl_str_set_delete(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init2(&set, &global_arena, 2020, 12, 0.7f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr keep = JSL_FATPTR_INITIALIZER("keep");
    JSLFatPtr drop = JSL_FATPTR_INITIALIZER("drop");
    JSLFatPtr other = JSL_FATPTR_INITIALIZER("other");

    TEST_BOOL(jsl_str_set_insert(&set, keep, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_set_insert(&set, drop, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_set_insert(&set, other, JSL_STRING_LIFETIME_STATIC));

    TEST_BOOL(!jsl_str_set_delete(&set, JSL_FATPTR_EXPRESSION("missing")));

    TEST_BOOL(jsl_str_set_delete(&set, drop));
    TEST_BOOL(!jsl_str_set_has(&set, drop));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) 2);

    TEST_BOOL(jsl_str_set_insert(&set, JSL_FATPTR_EXPRESSION("new"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_set_has(&set, JSL_FATPTR_EXPRESSION("new")));
}

static void test_jsl_str_set_clear(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init2(&set, &global_arena, 3030, 10, 0.6f);
    TEST_BOOL(ok);
    if (!ok) return;

    TEST_BOOL(jsl_str_set_insert(&set, JSL_FATPTR_EXPRESSION("x"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_set_insert(&set, JSL_FATPTR_EXPRESSION("y"), JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_set_insert(&set, JSL_FATPTR_EXPRESSION("z"), JSL_STRING_LIFETIME_STATIC));

    jsl_str_set_clear(&set);

    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) 0);
    TEST_BOOL(!jsl_str_set_has(&set, JSL_FATPTR_EXPRESSION("x")));
    TEST_BOOL(!jsl_str_set_has(&set, JSL_FATPTR_EXPRESSION("y")));
    TEST_BOOL(!jsl_str_set_has(&set, JSL_FATPTR_EXPRESSION("z")));
    TEST_INT64_EQUAL(set.tombstone_count, (int64_t) 0);

    JSLStrSetKeyValueIter iter;
    TEST_BOOL(jsl_str_set_iterator_init(&set, &iter));
    JSLFatPtr out_value = {0};
    TEST_BOOL(!jsl_str_set_iterator_next(&iter, &out_value));

    TEST_BOOL(jsl_str_set_insert(&set, JSL_FATPTR_EXPRESSION("reused"), JSL_STRING_LIFETIME_STATIC));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) 1);
    TEST_BOOL(jsl_str_set_has(&set, JSL_FATPTR_EXPRESSION("reused")));
}

static void test_jsl_str_set_handles_empty_and_binary_values(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init2(&set, &global_arena, 5050, 8, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr empty_value = JSL_FATPTR_INITIALIZER("");
    uint8_t binary_buf[] = { 'A', 0x00, 'B', 0x7F };
    JSLFatPtr binary_value = jsl_fatptr_init(binary_buf, 4);

    TEST_BOOL(jsl_str_set_insert(&set, empty_value, JSL_STRING_LIFETIME_STATIC));
    TEST_BOOL(jsl_str_set_insert(&set, binary_value, JSL_STRING_LIFETIME_TRANSIENT));

    TEST_BOOL(jsl_str_set_has(&set, empty_value));
    TEST_BOOL(jsl_str_set_has(&set, binary_value));

    ExpectedValue expected[] = {
        { empty_value, false },
        { binary_value, false }
    };

    JSLStrSetKeyValueIter iter;
    TEST_BOOL(jsl_str_set_iterator_init(&set, &iter));

    JSLFatPtr out_value = {0};
    while (jsl_str_set_iterator_next(&iter, &out_value))
    {
        bool matched = false;
        for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
        {
            if (!expected[i].seen && jsl_fatptr_memory_compare(out_value, expected[i].value))
            {
                expected[i].seen = true;
                matched = true;
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

static void test_jsl_str_set_intersection_basic(void)
{
    JSLStrSet a = {0};
    JSLStrSet b = {0};
    JSLStrSet out = {0};
    bool ok = (
        jsl_str_set_init2(&a, &global_arena, 101, 8, 0.75f)
        && jsl_str_set_init2(&b, &global_arena, 202, 8, 0.75f)
        && jsl_str_set_init2(&out, &global_arena, 303, 4, 0.75f)
    );
    TEST_BOOL(ok);
    if (!ok) return;

    const char* a_values[] = {"alpha", "beta", "common-one", "common-two"};
    const char* b_values[] = {"common-two", "gamma", "common-one"};

    TEST_BOOL(insert_values(&a, a_values, sizeof(a_values) / sizeof(a_values[0])));
    TEST_BOOL(insert_values(&b, b_values, sizeof(b_values) / sizeof(b_values[0])));

    TEST_BOOL(jsl_str_set_intersection(&a, &b, &out));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out), (int64_t) 2);
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("common-one")));
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("common-two")));
    TEST_BOOL(!jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("alpha")));
    TEST_BOOL(!jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("beta")));
    TEST_BOOL(!jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("gamma")));
}

static void test_jsl_str_set_intersection_with_empty_sets(void)
{
    JSLStrSet filled = {0};
    JSLStrSet empty = {0};
    JSLStrSet out_one = {0};
    JSLStrSet out_two = {0};
    bool ok = (
        jsl_str_set_init(&filled, &global_arena, 404)
        && jsl_str_set_init(&empty, &global_arena, 505)
        && jsl_str_set_init(&out_one, &global_arena, 606)
        && jsl_str_set_init(&out_two, &global_arena, 707)
    );
    TEST_BOOL(ok);
    if (!ok) return;

    const char* values[] = {"lonely", "spare"};
    TEST_BOOL(insert_values(&filled, values, sizeof(values) / sizeof(values[0])));

    TEST_BOOL(jsl_str_set_intersection(&filled, &empty, &out_one));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out_one), (int64_t) 0);

    TEST_BOOL(jsl_str_set_intersection(&empty, &filled, &out_two));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out_two), (int64_t) 0);
}

static void test_jsl_str_set_union_collects_all_unique_values(void)
{
    JSLStrSet a = {0};
    JSLStrSet b = {0};
    JSLStrSet out = {0};
    bool ok = (
        jsl_str_set_init2(&a, &global_arena, 808, 6, 0.6f)
        && jsl_str_set_init2(&b, &global_arena, 909, 6, 0.6f)
        && jsl_str_set_init2(&out, &global_arena, 1001, 12, 0.75f)
    );
    TEST_BOOL(ok);
    if (!ok) return;

    const char* a_values[] = {"alpha", "beta", "shared", "shared-two"};
    const char* b_values[] = {"shared", "gamma", "shared-two", "delta"};

    TEST_BOOL(insert_values(&a, a_values, sizeof(a_values) / sizeof(a_values[0])));
    TEST_BOOL(insert_values(&b, b_values, sizeof(b_values) / sizeof(b_values[0])));

    TEST_BOOL(jsl_str_set_union(&a, &b, &out));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out), (int64_t) 6);

    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("alpha")));
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("beta")));
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("shared")));
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("shared-two")));
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("gamma")));
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("delta")));
}

static void test_jsl_str_set_union_with_empty_sets(void)
{
    JSLStrSet filled = {0};
    JSLStrSet empty = {0};
    JSLStrSet out_one = {0};
    JSLStrSet out_two = {0};
    JSLStrSet out_three = {0};

    bool ok = (
        jsl_str_set_init(&filled, &global_arena, 1111)
        && jsl_str_set_init(&empty, &global_arena, 1222)
        && jsl_str_set_init(&out_one, &global_arena, 1333)
        && jsl_str_set_init(&out_two, &global_arena, 1444)
        && jsl_str_set_init(&out_three, &global_arena, 1555)
    );
    TEST_BOOL(ok);
    if (!ok) return;

    const char* values[] = {"solo", "duo"};
    TEST_BOOL(insert_values(&filled, values, sizeof(values) / sizeof(values[0])));

    TEST_BOOL(jsl_str_set_union(&filled, &empty, &out_one));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out_one), (int64_t) 2);
    TEST_BOOL(jsl_str_set_has(&out_one, JSL_FATPTR_EXPRESSION("solo")));
    TEST_BOOL(jsl_str_set_has(&out_one, JSL_FATPTR_EXPRESSION("duo")));

    TEST_BOOL(jsl_str_set_union(&empty, &filled, &out_two));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out_two), (int64_t) 2);
    TEST_BOOL(jsl_str_set_has(&out_two, JSL_FATPTR_EXPRESSION("solo")));
    TEST_BOOL(jsl_str_set_has(&out_two, JSL_FATPTR_EXPRESSION("duo")));

    TEST_BOOL(jsl_str_set_union(&empty, &empty, &out_three));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out_three), (int64_t) 0);
}

static void test_jsl_str_set_difference_basic(void)
{
    JSLStrSet a = {0};
    JSLStrSet b = {0};
    JSLStrSet out = {0};
    bool ok = (
        jsl_str_set_init2(&a, &global_arena, 1666, 6, 0.6f)
        && jsl_str_set_init2(&b, &global_arena, 1777, 6, 0.6f)
        && jsl_str_set_init2(&out, &global_arena, 1888, 6, 0.6f)
    );
    TEST_BOOL(ok);
    if (!ok) return;

    const char* a_values[] = {"keep-one", "keep-two", "drop-me", "shared"};
    const char* b_values[] = {"drop-me", "shared", "other"};

    TEST_BOOL(insert_values(&a, a_values, sizeof(a_values) / sizeof(a_values[0])));
    TEST_BOOL(insert_values(&b, b_values, sizeof(b_values) / sizeof(b_values[0])));

    TEST_BOOL(jsl_str_set_difference(&a, &b, &out));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out), (int64_t) 2);
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("keep-one")));
    TEST_BOOL(jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("keep-two")));
    TEST_BOOL(!jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("drop-me")));
    TEST_BOOL(!jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("shared")));
    TEST_BOOL(!jsl_str_set_has(&out, JSL_FATPTR_EXPRESSION("other")));
}

static void test_jsl_str_set_difference_with_empty_sets(void)
{
    JSLStrSet filled = {0};
    JSLStrSet empty = {0};
    JSLStrSet superset = {0};
    JSLStrSet out_one = {0};
    JSLStrSet out_two = {0};
    JSLStrSet out_three = {0};
    bool ok = (
        jsl_str_set_init2(&filled, &global_arena, 1999, 4, 0.5f)
        && jsl_str_set_init2(&empty, &global_arena, 2110, 4, 0.5f)
        && jsl_str_set_init2(&superset, &global_arena, 2221, 6, 0.75f)
        && jsl_str_set_init2(&out_one, &global_arena, 2332, 4, 0.5f)
        && jsl_str_set_init2(&out_two, &global_arena, 2443, 4, 0.5f)
        && jsl_str_set_init2(&out_three, &global_arena, 2554, 6, 0.75f)
    );
    TEST_BOOL(ok);
    if (!ok) return;

    const char* base_values[] = {"a", "b"};
    const char* superset_values[] = {"a", "b", "c"};

    TEST_BOOL(insert_values(&filled, base_values, sizeof(base_values) / sizeof(base_values[0])));
    TEST_BOOL(insert_values(&superset, superset_values, sizeof(superset_values) / sizeof(superset_values[0])));

    TEST_BOOL(jsl_str_set_difference(&filled, &empty, &out_one));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out_one), (int64_t) 2);
    TEST_BOOL(jsl_str_set_has(&out_one, JSL_FATPTR_EXPRESSION("a")));
    TEST_BOOL(jsl_str_set_has(&out_one, JSL_FATPTR_EXPRESSION("b")));

    TEST_BOOL(jsl_str_set_difference(&empty, &filled, &out_two));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out_two), (int64_t) 0);

    TEST_BOOL(jsl_str_set_difference(&filled, &superset, &out_three));
    TEST_INT64_EQUAL(jsl_str_set_item_count(&out_three), (int64_t) 0);
}

static void test_jsl_str_set_set_operations_invalid_parameters(void)
{
    JSLStrSet a = {0};
    JSLStrSet b = {0};
    JSLStrSet out = {0};
    bool ok = (
        jsl_str_set_init(&a, &global_arena, 3000)
        && jsl_str_set_init(&b, &global_arena, 4000)
        && jsl_str_set_init(&out, &global_arena, 5000)
    );
    TEST_BOOL(ok);
    if (!ok) return;

    JSLStrSet uninitialized = {0};

    TEST_BOOL(!jsl_str_set_intersection(NULL, &b, &out));
    TEST_BOOL(!jsl_str_set_intersection(&a, NULL, &out));
    TEST_BOOL(!jsl_str_set_intersection(&a, &b, NULL));
    TEST_BOOL(!jsl_str_set_intersection(&uninitialized, &b, &out));
    TEST_BOOL(!jsl_str_set_intersection(&a, &uninitialized, &out));
    TEST_BOOL(!jsl_str_set_intersection(&a, &b, &uninitialized));

    TEST_BOOL(!jsl_str_set_union(NULL, &b, &out));
    TEST_BOOL(!jsl_str_set_union(&a, NULL, &out));
    TEST_BOOL(!jsl_str_set_union(&a, &b, NULL));
    TEST_BOOL(!jsl_str_set_union(&uninitialized, &b, &out));
    TEST_BOOL(!jsl_str_set_union(&a, &uninitialized, &out));
    TEST_BOOL(!jsl_str_set_union(&a, &b, &uninitialized));

    TEST_BOOL(!jsl_str_set_difference(NULL, &b, &out));
    TEST_BOOL(!jsl_str_set_difference(&a, NULL, &out));
    TEST_BOOL(!jsl_str_set_difference(&a, &b, NULL));
    TEST_BOOL(!jsl_str_set_difference(&uninitialized, &b, &out));
    TEST_BOOL(!jsl_str_set_difference(&a, &uninitialized, &out));
    TEST_BOOL(!jsl_str_set_difference(&a, &b, &uninitialized));

    TEST_INT64_EQUAL(jsl_str_set_item_count(&out), (int64_t) 0);
}

static void test_jsl_str_set_rehash_preserves_entries(void)
{
    JSLStrSet set = {0};
    bool ok = jsl_str_set_init2(&set, &global_arena, 6060, 4, 0.5f);
    TEST_BOOL(ok);
    if (!ok) return;

    const int32_t insert_count = 64;
    char buffer[32] = {0};

    for (int32_t i = 0; i < insert_count; ++i)
    {
        int len = snprintf(buffer, sizeof(buffer), "value-%d", i);
        (void) len;
        JSLFatPtr value = jsl_fatptr_from_cstr(buffer);
        TEST_BOOL(jsl_str_set_insert(&set, value, JSL_STRING_LIFETIME_TRANSIENT));
    }

    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) insert_count);

    int32_t checks[] = {0, insert_count / 2, insert_count - 1};
    for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); i++)
    {
        snprintf(buffer, sizeof(buffer), "value-%d", checks[i]);
        JSLFatPtr value = jsl_fatptr_from_cstr(buffer);
        TEST_BOOL(jsl_str_set_has(&set, value));
    }

    int64_t iterated = 0;
    JSLStrSetKeyValueIter iter;
    TEST_BOOL(jsl_str_set_iterator_init(&set, &iter));
    JSLFatPtr out_value = {0};
    while (jsl_str_set_iterator_next(&iter, &out_value))
    {
        iterated++;
    }
    TEST_INT64_EQUAL(iterated, (int64_t) insert_count);
}

static void test_jsl_str_set_rejects_invalid_parameters(void)
{
    JSLFatPtr value = JSL_FATPTR_INITIALIZER("value");
    TEST_BOOL(!jsl_str_set_insert(NULL, value, JSL_STRING_LIFETIME_STATIC));

    JSLStrSet set = (JSLStrSet) {0};
    TEST_BOOL(!jsl_str_set_insert(&set, value, JSL_STRING_LIFETIME_STATIC));

    bool ok = jsl_str_set_init(&set, &global_arena, 0);
    TEST_BOOL(ok);
    if (!ok) return;

    JSLFatPtr null_value = {0};
    TEST_BOOL(!jsl_str_set_insert(&set, null_value, JSL_STRING_LIFETIME_STATIC));

    JSLFatPtr negative_length = { (uint8_t*) "bad", -1 };
    TEST_BOOL(!jsl_str_set_insert(&set, negative_length, JSL_STRING_LIFETIME_STATIC));

    TEST_INT64_EQUAL(jsl_str_set_item_count(&set), (int64_t) 0);
}

int main(void)
{
    jsl_arena_init(&global_arena, malloc(arena_size), arena_size);

    RUN_TEST_FUNCTION("String set init success", test_jsl_str_set_init_success);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String set init invalid args", test_jsl_str_set_init_invalid_arguments);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String set insert and has", test_jsl_str_set_insert_and_has);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set lifetime rules", test_jsl_str_set_respects_lifetime_rules);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set iterator covers all", test_jsl_str_set_iterator_covers_all_values);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set iterator invalidation", test_jsl_str_set_iterator_invalidated_on_mutation);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set delete behavior", test_jsl_str_set_delete);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set clear behavior", test_jsl_str_set_clear);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set empty and binary values", test_jsl_str_set_handles_empty_and_binary_values);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set intersection basic cases", test_jsl_str_set_intersection_basic);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set intersection empty sets", test_jsl_str_set_intersection_with_empty_sets);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set union collects uniques", test_jsl_str_set_union_collects_all_unique_values);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set union with empty sets", test_jsl_str_set_union_with_empty_sets);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set difference basic cases", test_jsl_str_set_difference_basic);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set difference with empty sets", test_jsl_str_set_difference_with_empty_sets);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set operations invalid parameters", test_jsl_str_set_set_operations_invalid_parameters);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set rehash preserves entries", test_jsl_str_set_rehash_preserves_entries);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("String Set rejects invalid parameters", test_jsl_str_set_rejects_invalid_parameters);
    jsl_arena_reset(&global_arena);

    TEST_RESULTS();
    return lfails != 0;
}

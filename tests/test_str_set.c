/**
 * Copyright (c) 2025 Jack Stouffer
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

    RUN_TEST_FUNCTION("init success", test_jsl_str_set_init_success);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("init invalid args", test_jsl_str_set_init_invalid_arguments);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("insert and has", test_jsl_str_set_insert_and_has);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("lifetime rules", test_jsl_str_set_respects_lifetime_rules);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("iterator covers all", test_jsl_str_set_iterator_covers_all_values);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("iterator invalidation", test_jsl_str_set_iterator_invalidated_on_mutation);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("delete behavior", test_jsl_str_set_delete);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("clear behavior", test_jsl_str_set_clear);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("empty and binary values", test_jsl_str_set_handles_empty_and_binary_values);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("rehash preserves entries", test_jsl_str_set_rehash_preserves_entries);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("rejects invalid parameters", test_jsl_str_set_rejects_invalid_parameters);
    jsl_arena_reset(&global_arena);

    TEST_RESULTS();
    return lfails != 0;
}

/**
 * Copyright (c) 2025 Jack Stouffer
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
#include "../src/jsl_string_builder.h"

#include "minctest.h"

JSLArena global_arena;

/// @brief copy all of the chunks out to a buffer
static void debug_concatenate_builder(JSLStringBuilder* builder, JSLFatPtr* writer)
{
    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(builder, &iterator);

    JSLFatPtr slice;
    while (jsl_string_builder_iterator_next(&iterator, &slice))
    {
        int64_t memcpy_res = jsl_fatptr_memory_copy(writer, slice);
        TEST_INT64_EQUAL(memcpy_res, slice.length);
    }
}

static void test_jsl_string_builder_init(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init(&builder, &global_arena);
    TEST_BOOL(ok);
    TEST_POINTERS_EQUAL(builder.arena, &global_arena);
    TEST_INT64_EQUAL(builder.chunk_size, 1024);
    TEST_BOOL(builder.alignment == 8);
    TEST_BOOL(builder.head != NULL);
    TEST_POINTERS_EQUAL(builder.tail, builder.head);
    TEST_INT64_EQUAL(builder.head->buffer.length, builder.chunk_size);
    TEST_INT64_EQUAL(builder.head->writer.length, builder.chunk_size);
    TEST_POINTERS_EQUAL(builder.head->buffer.data, builder.head->writer.data);
}

static void test_jsl_string_builder_init2(void)
{
    JSLStringBuilder builder;

    const int32_t chunk_size = 64;
    const int32_t alignment = 16;

    bool ok = jsl_string_builder_init2(&builder, &global_arena, chunk_size, alignment);
    TEST_BOOL(ok);
    TEST_BOOL(builder.chunk_size == chunk_size);
    TEST_BOOL(builder.alignment == alignment);
    TEST_BOOL(builder.head != NULL && builder.tail != NULL);
    TEST_BOOL(builder.head->buffer.length == chunk_size);
    TEST_BOOL(builder.head->writer.length == chunk_size);
}

static void test_jsl_string_builder_init_invalid_arguments(void)
{
    JSLStringBuilder builder;

    TEST_BOOL(!jsl_string_builder_init2(NULL, &global_arena, 16, 8));
    TEST_BOOL(!jsl_string_builder_init2(&builder, NULL, 16, 8));
    TEST_BOOL(!jsl_string_builder_init2(&builder, &global_arena, 0, 8));
    TEST_BOOL(!jsl_string_builder_init2(&builder, &global_arena, 16, 0));
}

static void test_jsl_string_builder_insert_char_basic(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init(&builder, &global_arena);
    TEST_BOOL(ok);

    TEST_BOOL(jsl_string_builder_insert_char(&builder, 'A'));
    TEST_BOOL(jsl_string_builder_insert_char(&builder, 'B'));
    TEST_BOOL(jsl_string_builder_insert_char(&builder, 'C'));

    uint8_t actual[16] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    TEST_BOOL(len == 3);
    TEST_BOOL(actual[0] == 'A');
    TEST_BOOL(actual[1] == 'B');
    TEST_BOOL(actual[2] == 'C');

    JSLStringBuilder uninitialized = {0};
    TEST_BOOL(!jsl_string_builder_insert_char(&uninitialized, 'Z'));
}

static void test_jsl_string_builder_insert_char_grows_chunks(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 2, 2);
    TEST_BOOL(ok);

    TEST_BOOL(jsl_string_builder_insert_char(&builder, 'X'));
    TEST_BOOL(jsl_string_builder_insert_char(&builder, 'Y'));
    TEST_BOOL(jsl_string_builder_insert_char(&builder, 'Z'));

    TEST_BOOL(builder.head != builder.tail);

    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);
    JSLFatPtr first;
    TEST_BOOL(jsl_string_builder_iterator_next(&iterator, &first));
    TEST_BOOL(first.length == 2);
    TEST_BOOL(memcmp(first.data, "XY", 2) == 0);

    JSLFatPtr second;
    TEST_BOOL(jsl_string_builder_iterator_next(&iterator, &second));
    TEST_BOOL(second.length == 1);
    TEST_BOOL(second.data[0] == 'Z');

    TEST_BOOL(!jsl_string_builder_iterator_next(&iterator, &second));
}

static void test_jsl_string_builder_insert_uint8_t_binary(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 8, 4);
    TEST_BOOL(ok);

    TEST_BOOL(jsl_string_builder_insert_uint8_t(&builder, 0x00));
    TEST_BOOL(jsl_string_builder_insert_uint8_t(&builder, 0xFF));
    TEST_BOOL(jsl_string_builder_insert_uint8_t(&builder, 0x7F));

    uint8_t expected[] = {0x00, 0xFF, 0x7F};
    uint8_t actual[8] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    TEST_BOOL(len == 3);
    TEST_BOOL(memcmp(actual, expected, 3) == 0);

    JSLStringBuilder uninitialized = {0};
    TEST_BOOL(!jsl_string_builder_insert_uint8_t(&uninitialized, 0xAA));
}

static void test_jsl_string_builder_insert_fatptr_multi_chunk(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 4, 4);
    TEST_BOOL(ok);

    char text[] = "abcdefghij";
    JSLFatPtr data = jsl_fatptr_from_cstr(text);
    TEST_BOOL(jsl_string_builder_insert_fatptr(&builder, data));

    uint8_t actual[32] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;
    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);

    TEST_INT64_EQUAL(len, (int64_t) strlen(text));
    TEST_BUFFERS_EQUAL(actual, text, len);

    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);
    JSLFatPtr first;
    jsl_string_builder_iterator_next(&iterator, &first);
    JSLFatPtr second;
    jsl_string_builder_iterator_next(&iterator, &second);
    JSLFatPtr third;
    jsl_string_builder_iterator_next(&iterator, &third);

    TEST_INT64_EQUAL(first.length, (int64_t) 4);
    TEST_INT64_EQUAL(second.length, (int64_t) 4);
    TEST_INT64_EQUAL(third.length, (int64_t) 2);

    TEST_BUFFERS_EQUAL(first.data, "abcd", 4);
    TEST_BUFFERS_EQUAL(second.data, "efgh", 4);
    TEST_BUFFERS_EQUAL(third.data, "ij", 2);
}

static void test_jsl_string_builder_insert_fatptr_edge_cases(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 8, 8);
    TEST_BOOL(ok);

    JSLFatPtr empty = JSL_FATPTR_INITIALIZER("");
    TEST_BOOL(jsl_string_builder_insert_fatptr(&builder, empty));
    uint8_t actual[8] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    TEST_BOOL(len == 0);

    writer = buffer;
    uint8_t binary_data[] = {'A', '\0', 'B'};
    JSLFatPtr binary_ptr = jsl_fatptr_init(binary_data, 3);
    TEST_BOOL(jsl_string_builder_insert_fatptr(&builder, binary_ptr));
    debug_concatenate_builder(&builder, &writer);
    len = jsl_fatptr_total_write_length(buffer, writer);

    TEST_BOOL(len == 3);
    TEST_BOOL(actual[0] == 'A');
    TEST_BOOL(actual[1] == '\0');
    TEST_BOOL(actual[2] == 'B');

    JSLStringBuilder uninitialized = {0};
    TEST_BOOL(!jsl_string_builder_insert_fatptr(&uninitialized, binary_ptr));
}

static void test_jsl_string_builder_iterator_behavior(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 6, 2);
    TEST_BOOL(ok);

    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);
    TEST_POINTERS_EQUAL(iterator.current, builder.head);

    JSLFatPtr slice;
    jsl_string_builder_iterator_next(&iterator, &slice);
    TEST_BOOL(slice.data != NULL);
    TEST_BOOL(slice.length == 0);

    TEST_BOOL(jsl_string_builder_insert_char(&builder, '1'));
    TEST_BOOL(jsl_string_builder_insert_char(&builder, '2'));
    TEST_BOOL(jsl_string_builder_insert_char(&builder, '3'));

    jsl_string_builder_iterator_init(&builder, &iterator);
    TEST_BOOL(jsl_string_builder_iterator_next(&iterator, &slice));

    TEST_BOOL(slice.length == 3);
    TEST_BUFFERS_EQUAL(slice.data, "123", 3);

    JSLFatPtr end;
    jsl_string_builder_iterator_next(&iterator, &end);
    TEST_POINTERS_EQUAL(end.data, NULL);
    TEST_INT64_EQUAL(end.length, (int64_t) 0);

    JSLStringBuilder invalid = {0};
    JSLStringBuilderIterator invalid_iterator;
    jsl_string_builder_iterator_init(&invalid, &invalid_iterator);
    JSLFatPtr invalid_slice;
    TEST_BOOL(!jsl_string_builder_iterator_next(&invalid_iterator, &invalid_slice));
}

static void test_jsl_string_builder_format_success(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 32, 8);
    TEST_BOOL(ok);

    TEST_BOOL(jsl_string_builder_format(&builder, JSL_FATPTR_EXPRESSION("%s-%d"), "alpha", 42));
    TEST_BOOL(jsl_string_builder_format(&builder, JSL_FATPTR_EXPRESSION(":%02X"), 0xAB));

    uint8_t actual[64] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);

    TEST_INT64_EQUAL(len, (int64_t) strlen("alpha-42:AB"));
    TEST_BUFFERS_EQUAL(actual, "alpha-42:AB", len);
}

static void test_jsl_string_builder_format_needs_multiple_chunks(void)
{
    uint8_t arena_buffer[256];
    JSLArena arena = JSL_ARENA_FROM_STACK(arena_buffer);
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &arena, 16, 8);
    TEST_BOOL(ok);

    char long_fragment[] = "0123456789ABCDEF0123456789";
    TEST_BOOL(jsl_string_builder_format(&builder, JSL_FATPTR_EXPRESSION("%s"), long_fragment));

    uint8_t actual[128] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);

    TEST_INT64_EQUAL(len, (int64_t) strlen(long_fragment));
    TEST_BUFFERS_EQUAL(actual, long_fragment, len);
    TEST_BOOL(builder.head != builder.tail);
}

static void test_jsl_string_builder_format_invalid_builder(void)
{
    JSLStringBuilder builder = {0};
    TEST_BOOL(!jsl_string_builder_format(&builder, jsl_fatptr_from_cstr("abc")));
}

int main(void)
{
    jsl_arena_init(&global_arena, malloc(JSL_MEGABYTES(1)), JSL_MEGABYTES(1));

    RUN_TEST_FUNCTION("Test builder init", test_jsl_string_builder_init);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test builder init2", test_jsl_string_builder_init2);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test invalid init args", test_jsl_string_builder_init_invalid_arguments);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test insert char", test_jsl_string_builder_insert_char_basic);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test small chunk size", test_jsl_string_builder_insert_char_grows_chunks);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test binary data", test_jsl_string_builder_insert_uint8_t_binary);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test empty string and null bytes", test_jsl_string_builder_insert_fatptr_edge_cases);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test iterator", test_jsl_string_builder_iterator_behavior);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test iterator across chunks", test_jsl_string_builder_insert_fatptr_multi_chunk);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test format", test_jsl_string_builder_format_success);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test format across chunks", test_jsl_string_builder_format_needs_multiple_chunks);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test format invalid args", test_jsl_string_builder_format_invalid_builder);
    jsl_arena_reset(&global_arena);

    TEST_RESULTS();
    return lfails != 0;
}

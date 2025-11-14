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

#define JSL_IMPLEMENTATION
#include "../src/jacks_standard_library.h"

#include "minctest.h"

JSLArena global_arena;

/// @brief copy all of the chunks out to a buffer
static void debug_concatenate_builder(JSLStringBuilder* builder, JSLFatPtr* writer)
{
    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(builder, &iterator);

    while (true)
    {
        JSLFatPtr slice = jsl_string_builder_iterator_next(&iterator);
        if (slice.data == NULL)
            break;

        jsl_fatptr_memory_copy(writer, slice);
    }
}

void test_jsl_string_builder_init(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init(&builder, &global_arena);
    lok(ok);
    lok(builder.arena == &global_arena);
    lok(builder.chunk_size == 256);
    lok(builder.alignment == 8);
    lok(builder.head != NULL);
    lok(builder.tail == builder.head);
    lok(builder.head->buffer.length == builder.chunk_size);
    lok(builder.head->writer.length == builder.chunk_size);
    lok(builder.head->buffer.data == builder.head->writer.data);
}

void test_jsl_string_builder_init2(void)
{
    JSLStringBuilder builder;

    const int32_t chunk_size = 64;
    const int32_t alignment = 16;

    bool ok = jsl_string_builder_init2(&builder, &global_arena, chunk_size, alignment);
    lok(ok);
    lok(builder.chunk_size == chunk_size);
    lok(builder.alignment == alignment);
    lok(builder.head != NULL && builder.tail != NULL);
    lok(builder.head->buffer.length == chunk_size);
    lok(builder.head->writer.length == chunk_size);
}

void test_jsl_string_builder_init_invalid_arguments(void)
{
    JSLStringBuilder builder;

    lok(!jsl_string_builder_init2(NULL, &global_arena, 16, 8));
    lok(!jsl_string_builder_init2(&builder, NULL, 16, 8));
    lok(!jsl_string_builder_init2(&builder, &global_arena, 0, 8));
    lok(!jsl_string_builder_init2(&builder, &global_arena, 16, 0));
}

void test_jsl_string_builder_insert_char_basic(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init(&builder, &global_arena);
    lok(ok);

    lok(jsl_string_builder_insert_char(&builder, 'A'));
    lok(jsl_string_builder_insert_char(&builder, 'B'));
    lok(jsl_string_builder_insert_char(&builder, 'C'));

    uint8_t actual[16] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    lok(len == 3);
    lok(actual[0] == 'A');
    lok(actual[1] == 'B');
    lok(actual[2] == 'C');

    JSLStringBuilder uninitialized = {0};
    lok(!jsl_string_builder_insert_char(&uninitialized, 'Z'));
}

void test_jsl_string_builder_insert_char_grows_chunks(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 2, 2);
    lok(ok);

    lok(jsl_string_builder_insert_char(&builder, 'X'));
    lok(jsl_string_builder_insert_char(&builder, 'Y'));
    lok(jsl_string_builder_insert_char(&builder, 'Z'));

    lok(builder.head != builder.tail);

    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);
    JSLFatPtr first = jsl_string_builder_iterator_next(&iterator);
    lok(first.length == 2);
    lok(memcmp(first.data, "XY", 2) == 0);

    JSLFatPtr second = jsl_string_builder_iterator_next(&iterator);
    lok(second.length == 1);
    lok(second.data[0] == 'Z');

    JSLFatPtr done = jsl_string_builder_iterator_next(&iterator);
    lok(done.data == NULL);
    lok(done.length == 0);
}

void test_jsl_string_builder_insert_uint8_t_binary(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 8, 4);
    lok(ok);

    lok(jsl_string_builder_insert_uint8_t(&builder, 0x00));
    lok(jsl_string_builder_insert_uint8_t(&builder, 0xFF));
    lok(jsl_string_builder_insert_uint8_t(&builder, 0x7F));

    uint8_t expected[] = {0x00, 0xFF, 0x7F};
    uint8_t actual[8] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    lok(len == 3);
    lok(memcmp(actual, expected, 3) == 0);

    JSLStringBuilder uninitialized = {0};
    lok(!jsl_string_builder_insert_uint8_t(&uninitialized, 0xAA));
}

void test_jsl_string_builder_insert_fatptr_multi_chunk(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 4, 4);
    lok(ok);

    char text[] = "abcdefghij";
    JSLFatPtr data = jsl_fatptr_from_cstr(text);
    lok(jsl_string_builder_insert_fatptr(&builder, data));

    uint8_t actual[32] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;
    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    lok(len == (int64_t) strlen(text));
    lok(memcmp(actual, text, len) == 0);

    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);
    JSLFatPtr first = jsl_string_builder_iterator_next(&iterator);
    JSLFatPtr second = jsl_string_builder_iterator_next(&iterator);
    JSLFatPtr third = jsl_string_builder_iterator_next(&iterator);
    lok(first.length == 4);
    lok(second.length == 4);
    lok(third.length == 2);
    lok(memcmp(first.data, "abcd", 4) == 0);
    lok(memcmp(second.data, "efgh", 4) == 0);
    lok(memcmp(third.data, "ij", 2) == 0);
}

void test_jsl_string_builder_insert_fatptr_edge_cases(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 8, 8);
    lok(ok);

    JSLFatPtr empty = JSL_FATPTR_INITIALIZER("");
    lok(jsl_string_builder_insert_fatptr(&builder, empty));
    uint8_t actual[8] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    lok(len == 0);

    writer = buffer;
    uint8_t binary_data[] = {'A', '\0', 'B'};
    JSLFatPtr binary_ptr = jsl_fatptr_init(binary_data, 3);
    lok(jsl_string_builder_insert_fatptr(&builder, binary_ptr));
    debug_concatenate_builder(&builder, &writer);
    len = jsl_fatptr_total_write_length(buffer, writer);

    lok(len == 3);
    lok(actual[0] == 'A');
    lok(actual[1] == '\0');
    lok(actual[2] == 'B');

    JSLStringBuilder uninitialized = {0};
    lok(!jsl_string_builder_insert_fatptr(&uninitialized, binary_ptr));
}

void test_jsl_string_builder_iterator_behavior(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 6, 2);
    lok(ok);

    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);
    lok(iterator.current == builder.head);

    JSLFatPtr slice = jsl_string_builder_iterator_next(&iterator);
    lok(slice.data != NULL);
    lok(slice.length == 0);

    lok(jsl_string_builder_insert_char(&builder, '1'));
    lok(jsl_string_builder_insert_char(&builder, '2'));
    lok(jsl_string_builder_insert_char(&builder, '3'));

    jsl_string_builder_iterator_init(&builder, &iterator);
    slice = jsl_string_builder_iterator_next(&iterator);
    lok(slice.length == 3);
    lok(memcmp(slice.data, "123", 3) == 0);

    JSLFatPtr end = jsl_string_builder_iterator_next(&iterator);
    lok(end.data == NULL);
    lok(end.length == 0);

    JSLStringBuilder invalid = {0};
    JSLStringBuilderIterator invalid_iterator;
    jsl_string_builder_iterator_init(&invalid, &invalid_iterator);
    JSLFatPtr invalid_slice = jsl_string_builder_iterator_next(&invalid_iterator);
    lok(invalid_slice.data == NULL);
    lok(invalid_slice.length == 0);
}

void test_jsl_string_builder_format_success(void)
{
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &global_arena, 32, 8);
    lok(ok);

    lok(jsl_string_builder_format(&builder, JSL_FATPTR_EXPRESSION("%s-%d"), "alpha", 42));
    lok(jsl_string_builder_format(&builder, JSL_FATPTR_EXPRESSION(":%02X"), 0xAB));

    uint8_t actual[64] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);

    lok(len == (int64_t) strlen("alpha-42:AB"));
    lok(memcmp(actual, "alpha-42:AB", len) == 0);
}

void test_jsl_string_builder_format_needs_multiple_chunks(void)
{
    uint8_t arena_buffer[256];
    JSLArena arena = JSL_ARENA_FROM_STACK(arena_buffer);
    JSLStringBuilder builder;
    bool ok = jsl_string_builder_init2(&builder, &arena, 16, 8);
    lok(ok);

    char long_fragment[] = "0123456789ABCDEF0123456789";
    lok(jsl_string_builder_format(&builder, JSL_FATPTR_EXPRESSION("%s"), long_fragment));

    uint8_t actual[128] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    lok(len == (int64_t) strlen(long_fragment));
    lok(memcmp(actual, long_fragment, len) == 0);
    lok(builder.head != builder.tail);
}

void test_jsl_string_builder_format_invalid_builder(void)
{
    JSLStringBuilder builder = {0};
    lok(!jsl_string_builder_format(&builder, jsl_fatptr_from_cstr("abc")));
}

int main(void)
{
    jsl_arena_init(&global_arena, malloc(JSL_MEGABYTES(1)), JSL_MEGABYTES(1));

    lrun("Test builder init", test_jsl_string_builder_init);
    jsl_arena_reset(&global_arena);

    lrun("Test builder init2", test_jsl_string_builder_init2);
    jsl_arena_reset(&global_arena);

    lrun("Test invalid init args", test_jsl_string_builder_init_invalid_arguments);
    jsl_arena_reset(&global_arena);

    lrun("Test insert char", test_jsl_string_builder_insert_char_basic);
    jsl_arena_reset(&global_arena);

    lrun("Test small chunk size", test_jsl_string_builder_insert_char_grows_chunks);
    jsl_arena_reset(&global_arena);

    lrun("Test binary data", test_jsl_string_builder_insert_uint8_t_binary);
    jsl_arena_reset(&global_arena);

    lrun("Test empty string and null bytes", test_jsl_string_builder_insert_fatptr_edge_cases);
    jsl_arena_reset(&global_arena);

    lrun("Test iterator", test_jsl_string_builder_iterator_behavior);
    jsl_arena_reset(&global_arena);

    lrun("Test iterator across chunks", test_jsl_string_builder_insert_fatptr_multi_chunk);
    jsl_arena_reset(&global_arena);

    lrun("Test format", test_jsl_string_builder_format_success);
    jsl_arena_reset(&global_arena);

    lrun("Test format across chunks", test_jsl_string_builder_format_needs_multiple_chunks);
    jsl_arena_reset(&global_arena);

    lrun("Test format invalid args", test_jsl_string_builder_format_invalid_builder);
    jsl_arena_reset(&global_arena);

    lresults();
    return lfails != 0;
}

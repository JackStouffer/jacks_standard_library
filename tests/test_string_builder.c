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
#include "../src/jsl_string_builder.h"

#include "minctest.h"

JSLArena global_arena;

typedef struct JSLTestAllocatorContext
{
    int64_t alloc_count;
    int64_t free_count;
    int64_t active_allocations;
} JSLTestAllocatorContext;

static void* test_allocator_allocate(void* ctx, int64_t bytes, int32_t alignment, bool zeroed)
{
    (void) alignment;
    JSLTestAllocatorContext* context = (JSLTestAllocatorContext*) ctx;
    void* allocation = malloc((size_t) bytes);

    if (allocation == NULL)
        return NULL;

    if (zeroed)
        memset(allocation, 0, (size_t) bytes);

    context->alloc_count += 1;
    context->active_allocations += 1;

    return allocation;
}

static void* test_allocator_reallocate(void* ctx, void* allocation, int64_t new_bytes, int32_t alignment)
{
    (void) ctx;
    (void) alignment;
    return realloc(allocation, (size_t) new_bytes);
}

static bool test_allocator_free(void* ctx, void* allocation)
{
    JSLTestAllocatorContext* context = (JSLTestAllocatorContext*) ctx;
    free(allocation);
    context->free_count += 1;
    context->active_allocations -= 1;
    return true;
}

static bool test_allocator_free_all(void* ctx)
{
    (void) ctx;
    return true;
}

static JSLAllocatorInterface test_make_allocator(JSLTestAllocatorContext* context)
{
    JSLAllocatorInterface allocator;
    jsl_allocator_interface_init(
        &allocator,
        test_allocator_allocate,
        test_allocator_reallocate,
        test_allocator_free,
        test_allocator_free_all,
        context
    );
    return allocator;
}

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
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);
    bool ok = jsl_string_builder_init(&builder, &allocator);

    TEST_BOOL(ok);
    TEST_POINTERS_EQUAL(builder.allocator, &allocator);
    TEST_INT64_EQUAL(builder.chunk_size, 1024);
    TEST_BOOL(builder.chunk_alignment == 8);
    TEST_BOOL(builder.head != NULL);
    TEST_POINTERS_EQUAL(builder.tail, builder.head);
    TEST_INT64_EQUAL(builder.head->buffer.length, builder.chunk_size);
    TEST_INT64_EQUAL(builder.head->writer.length, builder.chunk_size);
    TEST_POINTERS_EQUAL(builder.head->buffer.data, builder.head->writer.data);
}

static void test_jsl_string_builder_init2(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);

    const int32_t chunk_size = 64;
    const int32_t alignment = 16;

    bool ok = jsl_string_builder_init2(&builder, &allocator, chunk_size, alignment);

    TEST_BOOL(ok);
    TEST_BOOL(builder.chunk_size == chunk_size);
    TEST_BOOL(builder.chunk_alignment == alignment);
    TEST_BOOL(builder.head != NULL && builder.tail != NULL);
    TEST_BOOL(builder.head->buffer.length == chunk_size);
    TEST_BOOL(builder.head->writer.length == chunk_size);
}

static void test_jsl_string_builder_init_invalid_arguments(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);

    TEST_BOOL(!jsl_string_builder_init2(NULL, &allocator, 16, 8));
    TEST_BOOL(!jsl_string_builder_init2(&builder, NULL, 16, 8));
    TEST_BOOL(!jsl_string_builder_init2(&builder, &allocator, 0, 8));
    TEST_BOOL(!jsl_string_builder_init2(&builder, &allocator, 16, 0));
}

static void test_jsl_string_builder_insert_fatptr_multi_chunk(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);

    bool ok = jsl_string_builder_init2(&builder, &allocator, 4, 4);
    TEST_BOOL(ok);

    char text[] = "abcdefghij";
    JSLFatPtr data = jsl_fatptr_from_cstr(text);
    TEST_INT64_EQUAL(jsl_string_builder_insert_fatptr(&builder, data), 10);

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
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);

    bool ok = jsl_string_builder_init2(&builder, &allocator, 8, 8);
    TEST_BOOL(ok);

    JSLFatPtr empty = JSL_FATPTR_INITIALIZER("");
    TEST_INT64_EQUAL(jsl_string_builder_insert_fatptr(&builder, empty), 0);
    uint8_t actual[8] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);
    TEST_BOOL(len == 0);

    writer = buffer;
    uint8_t binary_data[] = {'A', '\0', 'B'};
    JSLFatPtr binary_ptr = jsl_fatptr_init(binary_data, 3);
    TEST_INT64_EQUAL(jsl_string_builder_insert_fatptr(&builder, binary_ptr), 3);
    debug_concatenate_builder(&builder, &writer);
    len = jsl_fatptr_total_write_length(buffer, writer);

    TEST_BOOL(len == 3);
    TEST_BOOL(actual[0] == 'A');
    TEST_BOOL(actual[1] == '\0');
    TEST_BOOL(actual[2] == 'B');

    JSLStringBuilder uninitialized = {0};
    TEST_INT64_EQUAL(jsl_string_builder_insert_fatptr(&uninitialized, binary_ptr), -1);
}

static void test_jsl_string_builder_iterator_behavior(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);

    bool ok = jsl_string_builder_init2(&builder, &allocator, 6, 2);
    TEST_BOOL(ok);

    JSLOutputSink builder_sink = jsl_string_builder_output_sink(&builder);

    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);
    TEST_POINTERS_EQUAL(iterator.current, builder.head);

    JSLFatPtr slice;
    jsl_string_builder_iterator_next(&iterator, &slice);
    TEST_BOOL(slice.data != NULL);
    TEST_BOOL(slice.length == 0);

    TEST_INT64_EQUAL(jsl_output_sink_write_u8(builder_sink, '1'), 1);
    TEST_INT64_EQUAL(jsl_output_sink_write_u8(builder_sink, '2'), 1);
    TEST_INT64_EQUAL(jsl_output_sink_write_u8(builder_sink, '3'), 1);

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

static void test_jsl_string_builder_with_format(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&global_arena);

    bool ok = jsl_string_builder_init2(&builder, &allocator, 32, 8);
    TEST_BOOL(ok);

    JSLOutputSink builder_sink = jsl_string_builder_output_sink(&builder);

    TEST_BOOL(jsl_format_sink(builder_sink, JSL_FATPTR_EXPRESSION("%s-%d"), "alpha", 42) > -1);
    TEST_BOOL(jsl_format_sink(builder_sink, JSL_FATPTR_EXPRESSION(":%02X"), 0xAB) > -1);

    uint8_t actual[64] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);

    TEST_INT64_EQUAL(len, (int64_t) strlen("alpha-42:AB"));
    TEST_BUFFERS_EQUAL(actual, "alpha-42:AB", len);
}

static void test_jsl_string_builder_with_format_needs_multiple_chunks(void)
{
    uint8_t arena_buffer[256];
    JSLArena arena = JSL_ARENA_FROM_STACK(arena_buffer);
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&arena);
    
    bool ok = jsl_string_builder_init2(&builder, &allocator, 16, 8);
    TEST_BOOL(ok);

    JSLOutputSink builder_sink = jsl_string_builder_output_sink(&builder);

    char long_fragment[] = "0123456789ABCDEF0123456789";
    TEST_BOOL(jsl_format_sink(builder_sink, JSL_FATPTR_EXPRESSION("%s"), long_fragment) > -1);

    uint8_t actual[128] = {0};
    JSLFatPtr buffer = JSL_FATPTR_FROM_STACK(actual);
    JSLFatPtr writer = buffer;

    debug_concatenate_builder(&builder, &writer);
    int64_t len = jsl_fatptr_total_write_length(buffer, writer);

    TEST_INT64_EQUAL(len, (int64_t) strlen(long_fragment));
    TEST_BUFFERS_EQUAL(actual, long_fragment, len);
    TEST_BOOL(builder.head != builder.tail);
}

static void test_jsl_string_builder_with_format_invalid_builder(void)
{
    JSLStringBuilder builder = {0};
    JSLOutputSink builder_sink = jsl_string_builder_output_sink(&builder);
    TEST_INT64_EQUAL(jsl_format_sink(builder_sink, jsl_fatptr_from_cstr("abc")), 0);
}

static void test_jsl_string_builder_free_null_and_uninitialized(void)
{
    jsl_string_builder_free(NULL);

    JSLStringBuilder builder = {0};
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(builder.sentinel, (int64_t) 0);

    JSLOutputSink builder_sink = jsl_string_builder_output_sink(&builder);

    TEST_BOOL(jsl_output_sink_write_u8(builder_sink, 'X') < 0);
}

static void test_jsl_string_builder_free_invalid_sentinel_noop(void)
{
    JSLTestAllocatorContext context = {0};
    JSLAllocatorInterface allocator = test_make_allocator(&context);
    JSLStringBuilder builder;

    bool ok = jsl_string_builder_init2(&builder, &allocator, 8, 8);
    TEST_BOOL(ok);
    TEST_INT64_EQUAL(context.alloc_count, (int64_t) 2);

    uint64_t sentinel = builder.sentinel;
    builder.sentinel = 0;
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.free_count, (int64_t) 0);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 2);

    builder.sentinel = sentinel;
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);
}

static void test_jsl_string_builder_free_empty_builder(void)
{
    JSLTestAllocatorContext context = {0};
    JSLAllocatorInterface allocator = test_make_allocator(&context);
    JSLStringBuilder builder;

    bool ok = jsl_string_builder_init2(&builder, &allocator, 16, 8);
    TEST_BOOL(ok);
    TEST_INT64_EQUAL(context.alloc_count, (int64_t) 2);

    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);

    int64_t frees_before = context.free_count;
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.free_count, frees_before);
}

static void test_jsl_string_builder_free_single_chunk(void)
{
    JSLTestAllocatorContext context = {0};
    JSLAllocatorInterface allocator = test_make_allocator(&context);
    JSLStringBuilder builder;

    bool ok = jsl_string_builder_init2(&builder, &allocator, 16, 8);
    TEST_BOOL(ok);
    TEST_INT64_EQUAL(context.alloc_count, (int64_t) 2);

    JSLOutputSink builder_sink = jsl_string_builder_output_sink(&builder);

    TEST_INT64_EQUAL(jsl_output_sink_write_u8(builder_sink, 'A'), 1);
    TEST_INT64_EQUAL(jsl_output_sink_write_u8(builder_sink, 'B'), 1);

    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);
    TEST_INT64_EQUAL(builder.sentinel, (int64_t) 0);
    TEST_INT64_EQUAL(jsl_output_sink_write_u8(builder_sink, 'C'), -1);

    int64_t frees_before = context.free_count;
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.free_count, frees_before);
}

static void test_jsl_string_builder_free_multiple_chunks_and_reinit(void)
{
    JSLTestAllocatorContext context = {0};
    JSLAllocatorInterface allocator = test_make_allocator(&context);
    JSLStringBuilder builder;

    bool ok = jsl_string_builder_init2(&builder, &allocator, 4, 4);
    TEST_BOOL(ok);

    JSLOutputSink builder_sink = jsl_string_builder_output_sink(&builder);

    for (int i = 0; i < 10; ++i)
    {
        TEST_INT64_EQUAL(jsl_output_sink_write_u8(builder_sink, (uint8_t) ('a' + i)), 1);
    }

    TEST_INT64_EQUAL(context.alloc_count, (int64_t) 6);

    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);

    ok = jsl_string_builder_init2(&builder, &allocator, 8, 8);
    TEST_BOOL(ok);
    TEST_BOOL(jsl_output_sink_write_u8(builder_sink, 'Z'));
    jsl_string_builder_free(&builder);

    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);
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

    RUN_TEST_FUNCTION("Test empty string and null bytes", test_jsl_string_builder_insert_fatptr_edge_cases);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test iterator", test_jsl_string_builder_iterator_behavior);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test iterator across chunks", test_jsl_string_builder_insert_fatptr_multi_chunk);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test format", test_jsl_string_builder_with_format);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test format across chunks", test_jsl_string_builder_with_format_needs_multiple_chunks);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test format invalid args", test_jsl_string_builder_with_format_invalid_builder);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test free null and uninitialized", test_jsl_string_builder_free_null_and_uninitialized);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test free invalid sentinel no-op", test_jsl_string_builder_free_invalid_sentinel_noop);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test free empty builder", test_jsl_string_builder_free_empty_builder);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test free single chunk", test_jsl_string_builder_free_single_chunk);
    jsl_arena_reset(&global_arena);

    RUN_TEST_FUNCTION("Test free multiple chunks and reinit", test_jsl_string_builder_free_multiple_chunks_and_reinit);
    jsl_arena_reset(&global_arena);

    TEST_RESULTS();
    return lfails != 0;
}

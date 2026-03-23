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

#include "jsl/core.h"
#include "jsl/allocator.h"
#include "jsl/allocator_arena.h"
#include "jsl/allocator_infinite_arena.h"
#include "jsl/string_builder.h"

#include "minctest.h"
#include "test_string_builder.h"

extern JSLInfiniteArena global_arena;

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

static bool test_allocator_free(void* ctx, const void* allocation)
{
    JSLTestAllocatorContext* context = (JSLTestAllocatorContext*) ctx;
    free((void*) allocation);
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
        NULL,
        context
    );
    return allocator;
}

void test_jsl_string_builder_init(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_string_builder_init(&builder, allocator, 0);

    TEST_BOOL(ok);
    TEST_INT64_EQUAL(builder.length, (int64_t) 0);
    TEST_BOOL(builder.data != NULL);
    TEST_BOOL(builder.capacity >= 32);

    ok = jsl_string_builder_init(&builder, allocator, 128);
    TEST_BOOL(ok);
    TEST_INT64_EQUAL(builder.length, (int64_t) 0);
    TEST_BOOL(builder.data != NULL);
    TEST_BOOL(builder.capacity >= 128);
}

void test_jsl_string_builder_init_invalid_arguments(void)
{
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    TEST_BOOL(!jsl_string_builder_init(NULL, allocator, 16));

    JSLStringBuilder builder;
    TEST_BOOL(!jsl_string_builder_init(&builder, allocator, -1));
}

void test_jsl_string_builder_append(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    char text1[] = "hello";
    TEST_BOOL(jsl_string_builder_append(&builder, jsl_cstr_to_memory(text1)));
    TEST_INT64_EQUAL(builder.length, (int64_t) 5);
    TEST_BUFFERS_EQUAL(builder.data, "hello", 5);

    char text2[] = " world";
    TEST_BOOL(jsl_string_builder_append(&builder, jsl_cstr_to_memory(text2)));
    TEST_INT64_EQUAL(builder.length, (int64_t) 11);
    TEST_BUFFERS_EQUAL(builder.data, "hello world", 11);

    JSLStringBuilder small_builder;
    ok = jsl_string_builder_init(&small_builder, allocator, 4);
    TEST_BOOL(ok);

    char long_text[] = "abcdefghijklmnopqrstuvwxyz";
    TEST_BOOL(jsl_string_builder_append(&small_builder, jsl_cstr_to_memory(long_text)));
    TEST_INT64_EQUAL(small_builder.length, (int64_t) 26);
    TEST_BUFFERS_EQUAL(small_builder.data, long_text, 26);
    TEST_BOOL(small_builder.capacity >= 26);
}

void test_jsl_string_builder_append_edge_cases(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    JSLImmutableMemory empty = JSL_CSTR_INITIALIZER("");
    TEST_BOOL(jsl_string_builder_append(&builder, empty));
    TEST_INT64_EQUAL(builder.length, (int64_t) 0);

    uint8_t binary_data[] = {'A', '\0', 'B'};
    JSLImmutableMemory binary_ptr = jsl_immutable_memory(binary_data, 3);
    TEST_BOOL(jsl_string_builder_append(&builder, binary_ptr));
    TEST_INT64_EQUAL(builder.length, (int64_t) 3);
    TEST_BOOL(builder.data[0] == 'A');
    TEST_BOOL(builder.data[1] == '\0');
    TEST_BOOL(builder.data[2] == 'B');

    JSLStringBuilder uninitialized = {0};
    TEST_BOOL(!jsl_string_builder_append(&uninitialized, binary_ptr));
}

void test_jsl_string_builder_get_string(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    JSLImmutableMemory result = jsl_string_builder_get_string(&builder);
    TEST_POINTERS_EQUAL(result.data, builder.data);
    TEST_INT64_EQUAL(result.length, (int64_t) 0);

    char text[] = "test string";
    jsl_string_builder_append(&builder, jsl_cstr_to_memory(text));

    result = jsl_string_builder_get_string(&builder);
    TEST_POINTERS_EQUAL(result.data, builder.data);
    TEST_INT64_EQUAL(result.length, (int64_t) 11);
    TEST_BUFFERS_EQUAL(result.data, "test string", 11);
}

void test_jsl_string_builder_delete(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    char text[] = "abcdefghij";
    jsl_string_builder_append(&builder, jsl_cstr_to_memory(text));
    TEST_INT64_EQUAL(builder.length, (int64_t) 10);

    TEST_BOOL(jsl_string_builder_delete(&builder, 3, 4));
    TEST_INT64_EQUAL(builder.length, (int64_t) 6);
    TEST_BUFFERS_EQUAL(builder.data, "abchij", 6);

    TEST_BOOL(jsl_string_builder_delete(&builder, 4, 2));
    TEST_INT64_EQUAL(builder.length, (int64_t) 4);
    TEST_BUFFERS_EQUAL(builder.data, "abch", 4);

    TEST_BOOL(jsl_string_builder_delete(&builder, 0, 1));
    TEST_INT64_EQUAL(builder.length, (int64_t) 3);
    TEST_BUFFERS_EQUAL(builder.data, "bch", 3);

    TEST_BOOL(!jsl_string_builder_delete(&builder, -1, 1));
    TEST_BOOL(!jsl_string_builder_delete(&builder, 0, 0));
    TEST_BOOL(!jsl_string_builder_delete(&builder, 0, 10));
    TEST_INT64_EQUAL(builder.length, (int64_t) 3);

    JSLStringBuilder uninitialized = {0};
    TEST_BOOL(!jsl_string_builder_delete(&uninitialized, 0, 1));
}

void test_jsl_string_builder_clear(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    jsl_string_builder_append(&builder, jsl_cstr_to_memory("hello"));

    int64_t cap_before = builder.capacity;
    uint8_t* data_before = builder.data;

    jsl_string_builder_clear(&builder);
    TEST_INT64_EQUAL(builder.length, (int64_t) 0);
    TEST_INT64_EQUAL(builder.capacity, cap_before);
    TEST_POINTERS_EQUAL(builder.data, data_before);

    JSLStringBuilder uninitialized = {0};
    uninitialized.length = 5;
    jsl_string_builder_clear(&uninitialized);
    TEST_INT64_EQUAL(uninitialized.length, (int64_t) 5);
}

void test_jsl_string_builder_output_sink(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    JSLOutputSink sink = jsl_string_builder_output_sink(&builder);

    jsl_output_sink_write_u8(sink, '1');
    jsl_output_sink_write_u8(sink, '2');
    jsl_output_sink_write_u8(sink, '3');

    TEST_INT64_EQUAL(builder.length, (int64_t) 3);
    TEST_BUFFERS_EQUAL(builder.data, "123", 3);
}

void test_jsl_string_builder_format_via_sink(void)
{
    JSLStringBuilder builder;
    JSLAllocatorInterface allocator;
    jsl_infinite_arena_get_allocator_interface(&allocator, &global_arena);

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    JSLOutputSink sink = jsl_string_builder_output_sink(&builder);

    jsl_format_sink(sink, JSL_CSTR_EXPRESSION("%s-%d"), "alpha", 42);
    jsl_format_sink(sink, JSL_CSTR_EXPRESSION(":%02X"), 0xAB);

    JSLImmutableMemory result = jsl_string_builder_get_string(&builder);

    TEST_INT64_EQUAL(result.length, (int64_t) strlen("alpha-42:AB"));
    TEST_BUFFERS_EQUAL(result.data, "alpha-42:AB", result.length);
}

void test_jsl_string_builder_format_invalid_builder(void)
{
    JSLStringBuilder builder = {0};
    JSLOutputSink sink = jsl_string_builder_output_sink(&builder);
    jsl_format_sink(sink, jsl_cstr_to_memory("abc"));
}

void test_jsl_string_builder_free_null_and_uninitialized(void)
{
    jsl_string_builder_free(NULL);

    JSLStringBuilder builder = {0};
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(builder.sentinel, (int64_t) 0);

    JSLOutputSink sink = jsl_string_builder_output_sink(&builder);
    jsl_output_sink_write_u8(sink, 'X');
}

void test_jsl_string_builder_free_invalid_sentinel_noop(void)
{
    JSLTestAllocatorContext context = {0};
    JSLAllocatorInterface allocator = test_make_allocator(&context);
    JSLStringBuilder builder;

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);
    TEST_INT64_EQUAL(context.alloc_count, (int64_t) 1);

    uint64_t sentinel = builder.sentinel;
    builder.sentinel = 0;
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.free_count, (int64_t) 0);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 1);

    builder.sentinel = sentinel;
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);
}

void test_jsl_string_builder_free_empty_builder(void)
{
    JSLTestAllocatorContext context = {0};
    JSLAllocatorInterface allocator = test_make_allocator(&context);
    JSLStringBuilder builder;

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);
    TEST_INT64_EQUAL(context.alloc_count, (int64_t) 1);

    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);

    int64_t frees_before = context.free_count;
    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.free_count, frees_before);
}

void test_jsl_string_builder_free_and_reinit(void)
{
    JSLTestAllocatorContext context = {0};
    JSLAllocatorInterface allocator = test_make_allocator(&context);
    JSLStringBuilder builder;

    bool ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    jsl_string_builder_append(&builder, jsl_cstr_to_memory("hello world"));
    TEST_INT64_EQUAL(builder.length, (int64_t) 11);

    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);

    ok = jsl_string_builder_init(&builder, allocator, 0);
    TEST_BOOL(ok);

    jsl_string_builder_append(&builder, jsl_cstr_to_memory("reused"));
    JSLImmutableMemory result = jsl_string_builder_get_string(&builder);
    TEST_INT64_EQUAL(result.length, (int64_t) 6);
    TEST_BUFFERS_EQUAL(result.data, "reused", 6);

    jsl_string_builder_free(&builder);
    TEST_INT64_EQUAL(context.alloc_count, context.free_count);
    TEST_INT64_EQUAL(context.active_allocations, (int64_t) 0);
}

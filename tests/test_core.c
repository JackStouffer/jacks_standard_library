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

#include "jsl/core.h"
#include "jsl/allocator.h"
#include "jsl/allocator_arena.h"

#include "minctest.h"

JSLImmutableMemory medium_str = JSL_CSTR_INITIALIZER(
    "This is a very long string that is going to trigger SIMD code, "
    "as it's longer than a single AVX2 register when using 8-bit "
    "values, which we are since we're using ASCII/UTF-8."
);
JSLImmutableMemory long_str = JSL_CSTR_INITIALIZER(
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    "Nulla purus justo, iaculis sit amet interdum sit amet, "
    "tincidunt at erat. Etiam vulputate ornare dictum. Nullam "
    "dapibus at orci id dictum. Pellentesque id lobortis nibh, "
    "sit amet euismod lorem. Cras non ex vitae eros interdum blandit "
    "in non justo. Pellentesque tincidunt orci a ipsum sagittis, at "
    "interdum quam elementum. Mauris est elit, fringilla in placerat "
    "consectetur, venenatis nec felis. Nam tempus, justo sit amet "
    "sodales bibendum, tortor ipsum feugiat lectus, quis porta neque "
    "ipsum accumsan velit. Nam a malesuada urna. Quisque elementum, "
    "tellus auctor iaculis laoreet, dolor urna facilisis mauris, "
    "vitae dignissim nulla nibh ut velit. Class aptent taciti sociosqu "
    "ad litora torquent per conubia nostra, per inceptos himenaeos. Ut "
    "luctus semper bibendum. Cras sagittis, nulla in venenatis blandit, "
    "ante tortor pulvinar est, faucibus sollicitudin neque ante et diam. "
    "Morbi vulputate eu tortor nec vestibulum.\n"
    "Aliquam vel purus vel ipsum sollicitudin aliquet. Pellentesque "
    "habitant morbi tristique senectus et netus et malesuada fames ac "
    "turpis egestas. Phasellus ut varius nunc, sit amet placerat "
    "libero. Sed eu velit velit. Sed id tortor quis neque rhoncus "
    "tempor. Duis finibus at justo sed auctor. Fusce rhoncus nisi "
    "non venenatis dignissim. Praesent sapien elit, elementum id quam "
    "ut, volutpat imperdiet tellus. Nulla semper lorem id metus "
    "tincidunt luctus. Fusce sodales accumsan varius. Donec faucibus "
    "risus felis, vitae dapibus orci lobortis ut. Donec tincidunt eu "
    "risus et rutrum."
);

static void test_jsl_from_cstr(void)
{
    char* c_str = "This is a test string!";
    size_t length = strlen(c_str);

    JSLImmutableMemory str = jsl_cstr_to_memory(c_str);

    TEST_POINTERS_EQUAL(str.data, c_str);
    TEST_BOOL((int64_t) str.length == (int64_t) length);
    TEST_BOOL(memcmp(c_str, str.data, (size_t) str.length) == 0);
}

static void test_jsl_cstr_memory_copy(void)
{
    JSLMutableMemory buffer = jsl_mutable_memory(malloc(1024), 1024);
    TEST_BOOL((int64_t) buffer.length == (int64_t) 1024);
    JSLMutableMemory writer = buffer;

    char* str = "This is a test string!";
    int64_t length = (int64_t) strlen(str);
    int64_t memcpy_res = jsl_cstr_memory_copy(&writer, str, false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 22);

    TEST_POINTERS_EQUAL(writer.data, buffer.data + length);
    TEST_BOOL(writer.length == 1024 - length);
    TEST_BOOL((int64_t) buffer.length == (int64_t) 1024);

    TEST_BOOL(memcmp(str, buffer.data, (size_t) length) == 0);
}

static void test_jsl_memory_compare(void)
{
    JSLMutableMemory buffer1 = jsl_mutable_memory(malloc(13), 13);
    JSLMutableMemory buffer2 = jsl_mutable_memory(malloc(13), 13);
    JSLMutableMemory buffer3 = jsl_mutable_memory(malloc(13), 13);
    JSLMutableMemory buffer4 = jsl_mutable_memory(malloc(20), 20);

    JSLMutableMemory writer1 = buffer1;
    JSLMutableMemory writer2 = buffer2;
    JSLMutableMemory writer3 = buffer3;
    JSLMutableMemory writer4 = buffer4;

    int64_t memcpy_res = jsl_cstr_memory_copy(&writer1, "Hello, World!", false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);
    memcpy_res = jsl_cstr_memory_copy(&writer2, "Hello, Owrld!", false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);
    memcpy_res = jsl_cstr_memory_copy(&writer3, "Hello, World!", false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);
    memcpy_res = jsl_cstr_memory_copy(&writer4, "Hello, World!", false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);

    TEST_BOOL( jsl_memory_compare(buffer1, buffer1));
    TEST_BOOL(!jsl_memory_compare(buffer1, buffer2));
    TEST_BOOL( jsl_memory_compare(buffer1, buffer3));
    TEST_BOOL(!jsl_memory_compare(buffer1, buffer4));
}

static void test_jsl_slice(void)
{
    JSLMutableMemory buffer1 = jsl_mutable_memory(malloc(13), 13);

    {
        JSLMutableMemory writer1 = buffer1;
        int64_t memcpy_res = jsl_cstr_memory_copy(&writer1, "Hello, World!", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);

        JSLImmutableMemory slice1 = jsl_slice(buffer1, 0, buffer1.length);
        TEST_BOOL(jsl_memory_compare(buffer1, slice1));
    }

    {
        JSLMutableMemory buffer2 = jsl_mutable_memory(malloc(10), 10);
        JSLMutableMemory writer2 = buffer2;
        int64_t memcpy_res = jsl_cstr_memory_copy(&writer2, "Hello, Wor", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 10);

        JSLImmutableMemory slice2 = jsl_slice(buffer1, 0, 10);
        TEST_BOOL(jsl_memory_compare(buffer2, slice2));
    }

    {
        JSLMutableMemory buffer3 = jsl_mutable_memory(malloc(5), 5);
        JSLMutableMemory writer3 = buffer3;
        int64_t memcpy_res = jsl_cstr_memory_copy(&writer3, "lo, W", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 5);

        JSLImmutableMemory slice3 = jsl_slice(buffer1, 3, 8);
        TEST_BOOL(jsl_memory_compare(buffer3, slice3));
    }
}

static void test_jsl_total_write_length(void)
{
    {
        uint8_t buffer[32] = {0};
        JSLMutableMemory original = JSL_MEMORY_FROM_STACK(buffer);
        JSLMutableMemory writer = original;

        int64_t memcpy_res = jsl_cstr_memory_copy(&writer, "abc", false);
        TEST_INT64_EQUAL(memcpy_res, 3);
        memcpy_res = jsl_cstr_memory_copy(&writer, "defg", false);
        TEST_INT64_EQUAL(memcpy_res, 4);

        int64_t length_written = jsl_total_write_length(original, writer);
        TEST_INT64_EQUAL(length_written, 7);
        TEST_BOOL(memcmp(buffer, "abcdefg", 7) == 0);
    }

    {
        uint8_t buffer[8] = {0};
        JSLMutableMemory original = JSL_MEMORY_FROM_STACK(buffer);
        JSLMutableMemory writer = original;

        int64_t length_written = jsl_total_write_length(original, writer);
        TEST_INT64_EQUAL(length_written, 0);

        JSLMutableMemory writer2 = { original.data + original.length, 0 };
        length_written = jsl_total_write_length(original, writer2);
        TEST_INT64_EQUAL(length_written, original.length);
    }
}

static void test_jsl_auto_slice(void)
{
    {
        uint8_t buffer[32] = {0};
        JSLMutableMemory original = JSL_MEMORY_FROM_STACK(buffer);
        JSLMutableMemory writer = original;

        int64_t memcpy_res = jsl_cstr_memory_copy(&writer, "Hello", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 5);
        memcpy_res = jsl_cstr_memory_copy(&writer, "World", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 5);

        JSLImmutableMemory slice = jsl_auto_slice(original, writer);
        TEST_INT64_EQUAL(slice.length, (int64_t) 10);
        TEST_POINTERS_EQUAL(slice.data, original.data);
        TEST_BOOL(memcmp(slice.data, "HelloWorld", 10) == 0);
    }

    {
        uint8_t buffer[4] = {0};
        JSLImmutableMemory original = JSL_MEMORY_FROM_STACK(buffer);
        JSLImmutableMemory writer = original;

        JSLImmutableMemory slice = jsl_auto_slice(original, writer);
        TEST_INT64_EQUAL(slice.length, (int64_t) 0);
        TEST_POINTERS_EQUAL(slice.data, original.data);
    }

    {
        uint8_t buffer[] = {'x', 'y', 'z', 'w', 'q', 'p'};
        JSLImmutableMemory original = jsl_immutable_memory(buffer, (int64_t) sizeof(buffer));
        JSLImmutableMemory writer = jsl_slice(original, 4, original.length);

        JSLImmutableMemory slice = jsl_auto_slice(original, writer);
        TEST_INT64_EQUAL(slice.length, (int64_t) 4);
        TEST_POINTERS_EQUAL(slice.data, original.data);
        TEST_BOOL(memcmp(slice.data, "xyzw", 4) == 0);
    }
}

static void test_jsl_auto_slice_arena_reallocate(void)
{
    const int64_t arena_size = JSL_KILOBYTES(64);
    JSLArena arena;
    jsl_arena_init(&arena, malloc((size_t) arena_size), arena_size);

    JSLImmutableMemory buffer;
    buffer.data = jsl_arena_allocate(&arena, 4096, false);
    buffer.length = 4096;
    TEST_BOOL(buffer.data != NULL);
    const uint8_t* original_ptr = buffer.data;

    JSLImmutableMemory writer = buffer;

    // Fill the initial allocation
    JSL_MEMORY_ADVANCE(writer, 4096);
    TEST_INT64_EQUAL(writer.length, (int64_t) 0);

    // First grow should keep the pointer stable
    buffer.data = jsl_arena_reallocate(&arena, buffer.data, 8192);
    TEST_BOOL(buffer.data != NULL);
    TEST_POINTERS_EQUAL(buffer.data, original_ptr);
    buffer.length += 4096;
    writer.length += 4096;

    // Fill the grown region
    JSL_MEMORY_ADVANCE(writer, 4096);
    TEST_INT64_EQUAL(writer.length, (int64_t) 0);

    // Second grow must still stay in place; otherwise auto_slice asserts
    buffer.data = jsl_arena_reallocate(&arena, buffer.data, 12288);
    TEST_BOOL(buffer.data != NULL);
    TEST_POINTERS_EQUAL(buffer.data, original_ptr);
    buffer.length += 4096;
    writer.length += 4096;

    JSLImmutableMemory slice = jsl_auto_slice(buffer, writer);
    TEST_INT64_EQUAL(slice.length, (int64_t) 8192);
    TEST_POINTERS_EQUAL(slice.data, buffer.data);
}

static void test_jsl_strip_whitespace_left(void)
{
    {
        JSLImmutableMemory empty = {0};
        int64_t res = jsl_strip_whitespace_left(&empty);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory negative_length = {
            .data = (uint8_t*) "  Hello",
            .length = -5
        };
        int64_t res = jsl_strip_whitespace_left(&negative_length);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory str = JSL_CSTR_INITIALIZER("Hello");
        int64_t res = jsl_strip_whitespace_left(&str);
        TEST_INT64_EQUAL(res, (int64_t) 0);
        TEST_BOOL(jsl_memory_cstr_compare(str, "Hello"));
    }

    {
        JSLImmutableMemory original = JSL_CSTR_INITIALIZER(" \t\nHello");
        JSLImmutableMemory str = original;

        int64_t res = jsl_strip_whitespace_left(&str);
        TEST_INT64_EQUAL(res, (int64_t) 3);
        TEST_POINTERS_EQUAL(str.data, original.data + 3);
        TEST_INT64_EQUAL(str.length, (int64_t) 5);
        TEST_BOOL(jsl_memory_cstr_compare(str, "Hello"));
    }

    {
        JSLImmutableMemory original = JSL_CSTR_INITIALIZER(" \t\n\r");
        JSLImmutableMemory str = original;

        int64_t res = jsl_strip_whitespace_left(&str);
        TEST_INT64_EQUAL(res, (int64_t) original.length);
        TEST_POINTERS_EQUAL(str.data, original.data + original.length);
        TEST_INT64_EQUAL(str.length, (int64_t) 0);
    }
}

static void test_jsl_strip_whitespace_right(void)
{
    {
        JSLImmutableMemory empty = {0};
        int64_t res = jsl_strip_whitespace_right(&empty);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory negative_length = {
            .data = (uint8_t*) "Hello  ",
            .length = -2
        };
        int64_t res = jsl_strip_whitespace_right(&negative_length);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory str = JSL_CSTR_INITIALIZER("Hello");
        int64_t res = jsl_strip_whitespace_right(&str);
        TEST_INT64_EQUAL(res, (int64_t) 0);
        TEST_BOOL(jsl_memory_cstr_compare(str, "Hello"));
    }

    {
        JSLImmutableMemory original = JSL_CSTR_INITIALIZER("Hello\t  ");
        JSLImmutableMemory str = original;

        int64_t res = jsl_strip_whitespace_right(&str);
        TEST_INT64_EQUAL(res, (int64_t) 3);
        TEST_POINTERS_EQUAL(str.data, original.data);
        TEST_INT64_EQUAL(str.length, original.length - 3);
        TEST_BOOL(jsl_memory_cstr_compare(str, "Hello"));
    }

    {
        JSLImmutableMemory original = JSL_CSTR_INITIALIZER(" \t\n\r");
        JSLImmutableMemory str = original;

        int64_t res = jsl_strip_whitespace_right(&str);
        TEST_INT64_EQUAL(res, (int64_t) original.length);
        TEST_POINTERS_EQUAL(str.data, original.data);
        TEST_INT64_EQUAL(str.length, (int64_t) 0);
    }
}

static void test_jsl_strip_whitespace(void)
{
    {
        JSLImmutableMemory empty = {0};
        int64_t res = jsl_strip_whitespace(&empty);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory negative_length = {
            .data = (uint8_t*) "   Hello   ",
            .length = -10
        };
        int64_t res = jsl_strip_whitespace(&negative_length);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory str = JSL_CSTR_INITIALIZER("Hello");
        int64_t res = jsl_strip_whitespace(&str);
        TEST_INT64_EQUAL(res, (int64_t) 0);
        TEST_BOOL(jsl_memory_cstr_compare(str, "Hello"));
    }

    {
        JSLImmutableMemory original = JSL_CSTR_INITIALIZER("  Hello World \n\t");
        JSLImmutableMemory str = original;

        int64_t res = jsl_strip_whitespace(&str);
        TEST_INT64_EQUAL(res, (int64_t) 5);
        TEST_POINTERS_EQUAL(str.data, original.data + 2);
        TEST_INT64_EQUAL(str.length, original.length - 5);
        TEST_BOOL(jsl_memory_cstr_compare(str, "Hello World"));
    }

    {
        JSLImmutableMemory original = JSL_CSTR_INITIALIZER("\t \n ");
        JSLImmutableMemory str = original;

        int64_t res = jsl_strip_whitespace(&str);
        TEST_INT64_EQUAL(res, (int64_t) original.length);
        TEST_POINTERS_EQUAL(str.data, original.data + original.length);
        TEST_INT64_EQUAL(str.length, (int64_t) 0);
    }
}

static void test_jsl_substring_search(void)
{
    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("111111");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("111111");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("Hello, World!");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("Longer substring than the original string");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("111111");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("1");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) 0);
    }

    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("Hello, World!");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("W");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) 7);
    }

    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("Hello, World!");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("World");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) 7);
    }

    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("Hello, World!");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("Hello, World!");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) 0);
    }

    {
        JSLImmutableMemory string = JSL_CSTR_INITIALIZER("Hello, World!");
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("Blorp");
        int64_t res = jsl_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("8-bit");
        int64_t res = jsl_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 117);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("8-blit");
        int64_t res = jsl_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("Blorf");
        int64_t res = jsl_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("ASCII/UTF-8");
        int64_t res = jsl_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 162);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        int64_t res = jsl_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 85);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        int64_t res = jsl_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 0);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("i");
        int64_t res = jsl_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 6);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("at");
        int64_t res = jsl_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 122);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("Sed");
        int64_t res = jsl_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 1171);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("elit");
        int64_t res = jsl_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 51);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("vitae");
        int64_t res = jsl_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 263);
    }

    {
        JSLImmutableMemory substring = JSL_CSTR_INITIALIZER("Lorem");
        int64_t res = jsl_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 0);
    }
}

static void test_jsl_index_of(void)
{
    JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("");
    int64_t res1 = jsl_index_of(buffer1, '3');
    TEST_BOOL(res1 == -1);

    JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER(".");
    int64_t res2 = jsl_index_of(buffer2, '.');
    TEST_BOOL(res2 == 0);

    JSLImmutableMemory buffer3 = JSL_CSTR_INITIALIZER("......");
    int64_t res3 = jsl_index_of(buffer3, '.');
    TEST_BOOL(res3 == 0);

    JSLImmutableMemory buffer4 = JSL_CSTR_INITIALIZER("Hello.World");
    int64_t res4 = jsl_index_of(buffer4, '.');
    TEST_BOOL(res4 == 5);

    JSLImmutableMemory buffer5 = JSL_CSTR_INITIALIZER("Hello          . Hello");
    int64_t res5 = jsl_index_of(buffer5, '.');
    TEST_BOOL(res5 == 15);

    JSLImmutableMemory buffer6 = JSL_CSTR_INITIALIZER("Hello.World.");
    int64_t res6 = jsl_index_of(buffer6, '.');
    TEST_BOOL(res6 == 5);

    JSLImmutableMemory buffer7 = JSL_CSTR_INITIALIZER("Hello Hello ");
    int64_t res7 = jsl_index_of(buffer7, ' ');
    TEST_BOOL(res7 == 5);

    JSLImmutableMemory buffer8 = JSL_CSTR_INITIALIZER("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res8 = jsl_index_of(buffer8, '8');
    TEST_BOOL(res8 == 117);
}

static void test_jsl_index_of_reverse(void)
{
    JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("");
    int64_t res1 = jsl_index_of_reverse(buffer1, '3');
    TEST_BOOL(res1 == -1);

    JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER(".");
    int64_t res2 = jsl_index_of_reverse(buffer2, '.');
    TEST_BOOL(res2 == 0);

    JSLImmutableMemory buffer3 = JSL_CSTR_INITIALIZER("......");
    int64_t res3 = jsl_index_of_reverse(buffer3, '.');
    TEST_BOOL(res3 == 5);

    JSLImmutableMemory buffer4 = JSL_CSTR_INITIALIZER("Hello.World");
    int64_t res4 = jsl_index_of_reverse(buffer4, '.');
    TEST_BOOL(res4 == 5);

    JSLImmutableMemory buffer5 = JSL_CSTR_INITIALIZER("Hello          . Hello");
    int64_t res5 = jsl_index_of_reverse(buffer5, '.');
    TEST_BOOL(res5 == 15);

    JSLImmutableMemory buffer6 = JSL_CSTR_INITIALIZER("Hello.World.");
    int64_t res6 = jsl_index_of_reverse(buffer6, '.');
    TEST_BOOL(res6 == 11);

    JSLImmutableMemory buffer7 = JSL_CSTR_INITIALIZER("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res7 = jsl_index_of_reverse(buffer7, 'M');
    TEST_BOOL(res7 == 54);

    JSLImmutableMemory buffer8 = JSL_CSTR_INITIALIZER("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res8 = jsl_index_of_reverse(buffer8, 'w');
    TEST_BOOL(res8 == 150);
}

static void test_jsl_get_file_extension(void)
{
    JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("");
    JSLImmutableMemory res1 = jsl_get_file_extension(buffer1);
    TEST_POINTERS_EQUAL(res1.data, NULL);

    JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER(".");
    JSLImmutableMemory res2 = jsl_get_file_extension(buffer2);
    TEST_BOOL(jsl_memory_cstr_compare(res2, ""));

    JSLImmutableMemory buffer3 = JSL_CSTR_INITIALIZER("......");
    JSLImmutableMemory res3 = jsl_get_file_extension(buffer3);
    TEST_BOOL(jsl_memory_cstr_compare(res3, ""));

    JSLImmutableMemory buffer4 = JSL_CSTR_INITIALIZER("Hello.text");
    JSLImmutableMemory res4 = jsl_get_file_extension(buffer4);
    TEST_BOOL(jsl_memory_cstr_compare(res4, "text"));

    JSLImmutableMemory buffer5 = JSL_CSTR_INITIALIZER("Hello          .css");
    JSLImmutableMemory res5 = jsl_get_file_extension(buffer5);
    TEST_BOOL(jsl_memory_cstr_compare(res5, "css"));

    JSLImmutableMemory buffer6 = JSL_CSTR_INITIALIZER("Hello.min.css");
    JSLImmutableMemory res6 = jsl_get_file_extension(buffer6);
    TEST_BOOL(jsl_memory_cstr_compare(res6, "css"));
}

static void test_jsl_to_lowercase_ascii(void)
{
    uint8_t _stack_memory[JSL_KILOBYTES(4)];

    {
        JSLImmutableMemory input = jsl_cstr_to_memory("10023");
        JSLImmutableMemory expected = jsl_cstr_to_memory("10023");

        JSLMutableMemory memory = JSL_MEMORY_FROM_STACK(_stack_memory);
        JSLMutableMemory writer = memory;
        JSLOutputSink sink = jsl_memory_output_sink(&writer);

        jsl_to_lowercase_ascii(sink, input);
        JSLImmutableMemory result = jsl_auto_slice(memory, writer);
        TEST_BOOL(jsl_memory_compare(result, expected));
    }

    {
        JSLImmutableMemory input = jsl_cstr_to_memory("hello!@#$@*()");
        JSLImmutableMemory expected = jsl_cstr_to_memory("hello!@#$@*()");

        JSLMutableMemory memory = JSL_MEMORY_FROM_STACK(_stack_memory);
        JSLMutableMemory writer = memory;
        JSLOutputSink sink = jsl_memory_output_sink(&writer);

        jsl_to_lowercase_ascii(sink, input);
        JSLImmutableMemory result = jsl_auto_slice(memory, writer);
        TEST_BOOL(jsl_memory_compare(result, expected));
    }

    {
        JSLImmutableMemory input = jsl_cstr_to_memory("Population");
        JSLImmutableMemory expected = jsl_cstr_to_memory("population");

        JSLMutableMemory memory = JSL_MEMORY_FROM_STACK(_stack_memory);
        JSLMutableMemory writer = memory;
        JSLOutputSink sink = jsl_memory_output_sink(&writer);

        jsl_to_lowercase_ascii(sink, input);
        JSLImmutableMemory result = jsl_auto_slice(memory, writer);
        TEST_BOOL(jsl_memory_compare(result, expected));
    }

    {
        JSLImmutableMemory input = jsl_cstr_to_memory("ENTRUSTED");
        JSLImmutableMemory expected = jsl_cstr_to_memory("entrusted");

        JSLMutableMemory memory = JSL_MEMORY_FROM_STACK(_stack_memory);
        JSLMutableMemory writer = memory;
        JSLOutputSink sink = jsl_memory_output_sink(&writer);

        jsl_to_lowercase_ascii(sink, input);
        JSLImmutableMemory result = jsl_auto_slice(memory, writer);
        TEST_BOOL(jsl_memory_compare(result, expected));
    }

    {
        JSLImmutableMemory input = jsl_cstr_to_memory((const char*) u8"Footnotes Ω≈ç√∫");
        JSLImmutableMemory expected = jsl_cstr_to_memory((const char*) u8"footnotes Ω≈ç√∫");

        JSLMutableMemory memory = JSL_MEMORY_FROM_STACK(_stack_memory);
        JSLMutableMemory writer = memory;
        JSLOutputSink sink = jsl_memory_output_sink(&writer);

        jsl_to_lowercase_ascii(sink, input);
        JSLImmutableMemory result = jsl_auto_slice(memory, writer);
        TEST_BOOL(jsl_memory_compare(result, expected));
    }

    {
        JSLImmutableMemory input = jsl_cstr_to_memory((const char*) u8"Ω≈ç√∫");
        JSLImmutableMemory expected = jsl_cstr_to_memory((const char*) u8"Ω≈ç√∫");

        JSLMutableMemory memory = JSL_MEMORY_FROM_STACK(_stack_memory);
        JSLMutableMemory writer = memory;
        JSLOutputSink sink = jsl_memory_output_sink(&writer);

        jsl_to_lowercase_ascii(sink, input);
        JSLImmutableMemory result = jsl_auto_slice(memory, writer);
        TEST_BOOL(jsl_memory_compare(result, expected));
    }

    {
        JSLImmutableMemory input = jsl_cstr_to_memory((const char*) u8"Ω≈ç√∫ ENTRUSTED this is a longer string to activate the SIMD path!");
        JSLImmutableMemory expected = jsl_cstr_to_memory((const char*) u8"Ω≈ç√∫ entrusted this is a longer string to activate the simd path!");

        JSLMutableMemory memory = JSL_MEMORY_FROM_STACK(_stack_memory);
        JSLMutableMemory writer = memory;
        JSLOutputSink sink = jsl_memory_output_sink(&writer);

        jsl_to_lowercase_ascii(sink, input);
        JSLImmutableMemory result = jsl_auto_slice(memory, writer);
        TEST_BOOL(jsl_memory_compare(result, expected));
    }
}

static void test_jsl_memory_to_int32(void)
{
    int32_t result;

    JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("0");
    TEST_BOOL(jsl_memory_to_int32(buffer1, &result) == 1);
    TEST_BOOL(result == 0);

    JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("-0");
    TEST_BOOL(jsl_memory_to_int32(buffer2, &result) == 2);
    TEST_BOOL(result == 0);

    JSLImmutableMemory buffer3 = JSL_CSTR_INITIALIZER("11");
    TEST_BOOL(jsl_memory_to_int32(buffer3, &result) == 2);
    TEST_BOOL(result == 11);

    JSLImmutableMemory buffer4 = JSL_CSTR_INITIALIZER("-1243");
    TEST_BOOL(jsl_memory_to_int32(buffer4, &result) == 5);
    TEST_BOOL(result == -1243);

    JSLImmutableMemory buffer5 = JSL_CSTR_INITIALIZER("000003");
    TEST_BOOL(jsl_memory_to_int32(buffer5, &result) == 6);
    TEST_BOOL(result == 3);

    JSLImmutableMemory buffer6 = JSL_CSTR_INITIALIZER("000000");
    TEST_BOOL(jsl_memory_to_int32(buffer6, &result) == 6);
    TEST_BOOL(result == 0);

    JSLImmutableMemory buffer7 = JSL_CSTR_INITIALIZER("-000000");
    TEST_BOOL(jsl_memory_to_int32(buffer7, &result) == 7);
    TEST_BOOL(result == 0);

    JSLImmutableMemory buffer8 = JSL_CSTR_INITIALIZER("98468465");
    TEST_BOOL(jsl_memory_to_int32(buffer8, &result) == 8);
    TEST_BOOL(result == 98468465);

    JSLImmutableMemory buffer9 = JSL_CSTR_INITIALIZER("454 hello, world");
    TEST_BOOL(jsl_memory_to_int32(buffer9, &result) == 3);
    TEST_BOOL(result == 454);

    JSLImmutableMemory buffer10 = JSL_CSTR_INITIALIZER("+488 hello, world");
    TEST_BOOL(jsl_memory_to_int32(buffer10, &result) == 4);
    TEST_BOOL(result == 488);
}

static void test_jsl_starts_with(void)
{
    JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory prefix1 = JSL_CSTR_INITIALIZER("Hello, World!");
    TEST_BOOL(jsl_starts_with(buffer1, prefix1) == true);

    JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory prefix2 = JSL_CSTR_INITIALIZER("Hello");
    TEST_BOOL(jsl_starts_with(buffer2, prefix2) == true);

    JSLImmutableMemory buffer3 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory prefix3 = JSL_CSTR_INITIALIZER("World");
    TEST_BOOL(jsl_starts_with(buffer3, prefix3) == false);

    JSLImmutableMemory buffer4 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory prefix4 = JSL_CSTR_INITIALIZER("");
    TEST_BOOL(jsl_starts_with(buffer4, prefix4) == true);

    JSLImmutableMemory buffer5 = JSL_CSTR_INITIALIZER("");
    JSLImmutableMemory prefix5 = JSL_CSTR_INITIALIZER("");
    TEST_BOOL(jsl_starts_with(buffer5, prefix5) == true);

    JSLImmutableMemory buffer6 = JSL_CSTR_INITIALIZER("");
    JSLImmutableMemory prefix6 = JSL_CSTR_INITIALIZER("HHHHHHHHHHHHHHHHH");
    TEST_BOOL(jsl_starts_with(buffer6, prefix6) == false);

    JSLImmutableMemory buffer7 = JSL_CSTR_INITIALIZER("HHHHHHHHHHHHHHHH");
    JSLImmutableMemory prefix7 = JSL_CSTR_INITIALIZER("HHHHHHHHHHHHHHHHH");
    TEST_BOOL(jsl_starts_with(buffer7, prefix7) == false);

    JSLImmutableMemory buffer8 = JSL_CSTR_INITIALIZER("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
    JSLImmutableMemory prefix8 = JSL_CSTR_INITIALIZER("This is a string example that will ");
    TEST_BOOL(jsl_starts_with(buffer8, prefix8));
}

static void test_jsl_ends_with(void)
{
    JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory postfix1 = JSL_CSTR_INITIALIZER("Hello, World!");
    TEST_BOOL(jsl_ends_with(buffer1, postfix1) == true);

    JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory postfix2 = JSL_CSTR_INITIALIZER("World!");
    TEST_BOOL(jsl_ends_with(buffer2, postfix2) == true);

    JSLImmutableMemory buffer3 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory postfix3 = JSL_CSTR_INITIALIZER("Hello");
    TEST_BOOL(jsl_ends_with(buffer3, postfix3) == false);

    JSLImmutableMemory buffer4 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory postfix4 = JSL_CSTR_INITIALIZER("");
    TEST_BOOL(jsl_ends_with(buffer4, postfix4) == true);

    JSLImmutableMemory buffer5 = JSL_CSTR_INITIALIZER("");
    JSLImmutableMemory postfix5 = JSL_CSTR_INITIALIZER("");
    TEST_BOOL(jsl_ends_with(buffer5, postfix5) == true);

    JSLImmutableMemory buffer6 = JSL_CSTR_INITIALIZER("");
    JSLImmutableMemory postfix6 = JSL_CSTR_INITIALIZER("HHHHHHHHHHHHHHHHH");
    TEST_BOOL(jsl_ends_with(buffer6, postfix6) == false);

    JSLImmutableMemory buffer7 = JSL_CSTR_INITIALIZER("HHHHHHHHHHHHHHHH");
    JSLImmutableMemory postfix7 = JSL_CSTR_INITIALIZER("HHHHHHHHHHHHHHHHH");
    TEST_BOOL(jsl_ends_with(buffer7, postfix7) == false);

    JSLImmutableMemory buffer8 = JSL_CSTR_INITIALIZER("Hello, World!");
    JSLImmutableMemory postfix8 = JSL_CSTR_INITIALIZER("!");
    TEST_BOOL(jsl_ends_with(buffer8, postfix8) == true);

    JSLImmutableMemory postfix9 = JSL_CSTR_INITIALIZER(" are since we're using ASCII/UTF-8.");
    TEST_BOOL(jsl_ends_with(medium_str, postfix9));
}

static void test_jsl_compare_ascii_insensitive(void)
{
    {
        JSLImmutableMemory buffer1 = {
            .data = NULL,
            .length = 0
        };
        JSLImmutableMemory buffer2 = {
            .data = NULL,
            .length = 0
        };
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("Hello, World!");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("Hello, World!");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("Hello, World!");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("Hello, World!");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("hello, world!");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("AAAAAAAAAA");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("AaaaAaAaAA");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLImmutableMemory buffer1 = {
            .data = NULL,
            .length = 0
        };
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == false);
    }
    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("This is a string example that WILL span multiple AVX2 chunks so that we can test if the loop is working properly.");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLImmutableMemory buffer1 = JSL_CSTR_INITIALIZER("This is a string example that WILL span multiple AVX2 chunkz so that we can test if the loop is workING properly.");
        JSLImmutableMemory buffer2 = JSL_CSTR_INITIALIZER("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        TEST_BOOL(jsl_compare_ascii_insensitive(buffer1, buffer2) == false);
    }
}

static void test_jsl_count(void)
{
    {
        JSLImmutableMemory buffer = JSL_CSTR_INITIALIZER("");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_count(buffer, item), (int64_t) 0);
    }

    {
        JSLImmutableMemory buffer = JSL_CSTR_INITIALIZER("Test string");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_count(buffer, item), (int64_t) 0);
    }

    {
        JSLImmutableMemory buffer = JSL_CSTR_INITIALIZER("Test string a");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_count(buffer, item), (int64_t) 1);
    }

    {
        JSLImmutableMemory buffer = JSL_CSTR_INITIALIZER("a Test string");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_count(buffer, item), (int64_t) 1);
    }

    {
        JSLImmutableMemory buffer = JSL_CSTR_INITIALIZER("A Test string");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_count(buffer, item), (int64_t) 0);
    }

    {
        uint8_t item = 'i';
        TEST_INT64_EQUAL(jsl_count(medium_str, item), (int64_t) 14);
    }

    {
        uint8_t item = 'z';
        TEST_INT64_EQUAL(jsl_count(medium_str, item), (int64_t) 0);
    }

    {
        uint8_t item = 'i';
        TEST_INT64_EQUAL(jsl_count(long_str, item), (int64_t) 129);
    }

    {
        uint8_t item = '=';
        TEST_INT64_EQUAL(jsl_count(long_str, item), (int64_t) 0);
    }
}

static void test_jsl_to_cstr(void)
{
    JSLArena arena;
    jsl_arena_init(&arena, malloc(1024), 1024);
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &arena);

    {
        JSLImmutableMemory memory = {0};
        const char* cstr = jsl_memory_to_cstr(allocator, memory);
        TEST_POINTERS_EQUAL(cstr, NULL);
    }

    jsl_arena_reset(&arena);

    {
        JSLImmutableMemory memory = JSL_CSTR_INITIALIZER("10023");
        const char* cstr = jsl_memory_to_cstr(allocator, memory);
        TEST_BOOL(jsl_memory_cstr_compare(memory, cstr));
    }

    jsl_arena_reset(&arena);

    {
        JSLImmutableMemory memory = JSL_CSTR_INITIALIZER(u8"Ω≈ç√∫");
        const char* cstr = jsl_memory_to_cstr(allocator, memory);
        TEST_BOOL(jsl_memory_cstr_compare(memory, cstr));
    }
}

int main(void)
{
    RUN_TEST_FUNCTION("Test jsl_cstr_to_memory", test_jsl_from_cstr);
    RUN_TEST_FUNCTION("Test jsl_cstr_memory_copy", test_jsl_cstr_memory_copy);
    RUN_TEST_FUNCTION("Test jsl_memory_compare", test_jsl_memory_compare);
    RUN_TEST_FUNCTION("Test jsl_slice", test_jsl_slice);
    RUN_TEST_FUNCTION("Test jsl_total_write_length", test_jsl_total_write_length);
    RUN_TEST_FUNCTION("Test jsl_auto_slice", test_jsl_auto_slice);
    RUN_TEST_FUNCTION("Test jsl_auto_slice_arena_reallocate", test_jsl_auto_slice_arena_reallocate);
    RUN_TEST_FUNCTION("Test jsl_strip_whitespace_left", test_jsl_strip_whitespace_left);
    RUN_TEST_FUNCTION("Test jsl_strip_whitespace_right", test_jsl_strip_whitespace_right);
    RUN_TEST_FUNCTION("Test jsl_strip_whitespace", test_jsl_strip_whitespace);
    RUN_TEST_FUNCTION("Test jsl_index_of", test_jsl_index_of);
    RUN_TEST_FUNCTION("Test jsl_index_of_reverse", test_jsl_index_of_reverse);
    RUN_TEST_FUNCTION("Test jsl_to_lowercase_ascii", test_jsl_to_lowercase_ascii);
    RUN_TEST_FUNCTION("Test jsl_memory_to_int32", test_jsl_memory_to_int32);
    RUN_TEST_FUNCTION("Test jsl_substring_search", test_jsl_substring_search);
    RUN_TEST_FUNCTION("Test jsl_starts_with", test_jsl_starts_with);
    RUN_TEST_FUNCTION("Test jsl_ends_with", test_jsl_ends_with);
    RUN_TEST_FUNCTION("Test jsl_compare_ascii_insensitive", test_jsl_compare_ascii_insensitive);
    RUN_TEST_FUNCTION("Test jsl_count", test_jsl_count);
    RUN_TEST_FUNCTION("Test jsl_memory_to_cstr", test_jsl_to_cstr);
    RUN_TEST_FUNCTION("Test jsl_get_file_extension", test_jsl_get_file_extension);

    TEST_RESULTS();
    return lfails != 0;
}

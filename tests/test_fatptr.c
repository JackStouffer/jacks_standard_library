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

#define JSL_CORE_IMPLEMENTATION
#include "../src/jsl_core.h"

#include "minctest.h"

JSLFatPtr medium_str = JSL_FATPTR_INITIALIZER(
    "This is a very long string that is going to trigger SIMD code, "
    "as it's longer than a single AVX2 register when using 8-bit "
    "values, which we are since we're using ASCII/UTF-8."
);
JSLFatPtr long_str = JSL_FATPTR_INITIALIZER(
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

static void test_jsl_fatptr_from_cstr(void)
{
    char* c_str = "This is a test string!";
    size_t length = strlen(c_str);

    JSLFatPtr str = jsl_fatptr_from_cstr(c_str);

    TEST_BOOL((void*) str.data == (void*) c_str);
    TEST_BOOL((int64_t) str.length == (int64_t) length);
    TEST_BOOL(memcmp(c_str, str.data, (size_t) str.length) == 0);
}

static void test_jsl_fatptr_cstr_memory_copy(void)
{
    JSLFatPtr buffer = jsl_fatptr_init(malloc(1024), 1024);
    TEST_BOOL((int64_t) buffer.length == (int64_t) 1024);

    JSLFatPtr writer = buffer;
    char* str = "This is a test string!";
    int64_t length = (int64_t) strlen(str);
    int64_t memcpy_res = jsl_fatptr_cstr_memory_copy(&writer, str, false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 22);

    TEST_BOOL(writer.data == buffer.data + length);
    TEST_BOOL(writer.length == 1024 - length);
    TEST_BOOL((int64_t) buffer.length == (int64_t) 1024);

    TEST_BOOL(memcmp(str, buffer.data, (size_t) length) == 0);
}

static void test_jsl_fatptr_memory_compare(void)
{
    JSLFatPtr buffer1 = jsl_fatptr_init(malloc(13), 13);
    JSLFatPtr buffer2 = jsl_fatptr_init(malloc(13), 13);
    JSLFatPtr buffer3 = jsl_fatptr_init(malloc(13), 13);
    JSLFatPtr buffer4 = jsl_fatptr_init(malloc(20), 20);

    JSLFatPtr writer1 = buffer1;
    JSLFatPtr writer2 = buffer2;
    JSLFatPtr writer3 = buffer3;
    JSLFatPtr writer4 = buffer4;

    int64_t memcpy_res = jsl_fatptr_cstr_memory_copy(&writer1, "Hello, World!", false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);
    memcpy_res = jsl_fatptr_cstr_memory_copy(&writer2, "Hello, Owrld!", false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);
    memcpy_res = jsl_fatptr_cstr_memory_copy(&writer3, "Hello, World!", false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);
    memcpy_res = jsl_fatptr_cstr_memory_copy(&writer4, "Hello, World!", false);
    TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);

    TEST_BOOL( jsl_fatptr_memory_compare(buffer1, buffer1));
    TEST_BOOL(!jsl_fatptr_memory_compare(buffer1, buffer2));
    TEST_BOOL( jsl_fatptr_memory_compare(buffer1, buffer3));
    TEST_BOOL(!jsl_fatptr_memory_compare(buffer1, buffer4));
}

static void test_jsl_fatptr_slice(void)
{
    JSLFatPtr buffer1 = jsl_fatptr_init(malloc(13), 13);

    {
        JSLFatPtr writer1 = buffer1;
        int64_t memcpy_res = jsl_fatptr_cstr_memory_copy(&writer1, "Hello, World!", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 13);

        JSLFatPtr slice1 = jsl_fatptr_slice(buffer1, 0, buffer1.length);
        TEST_BOOL(jsl_fatptr_memory_compare(buffer1, slice1));
    }

    {
        JSLFatPtr buffer2 = jsl_fatptr_init(malloc(10), 10);
        JSLFatPtr writer2 = buffer2;
        int64_t memcpy_res = jsl_fatptr_cstr_memory_copy(&writer2, "Hello, Wor", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 10);

        JSLFatPtr slice2 = jsl_fatptr_slice(buffer1, 0, 10);
        TEST_BOOL(jsl_fatptr_memory_compare(buffer2, slice2));
    }

    {
        JSLFatPtr buffer3 = jsl_fatptr_init(malloc(5), 5);
        JSLFatPtr writer3 = buffer3;
        int64_t memcpy_res = jsl_fatptr_cstr_memory_copy(&writer3, "lo, W", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 5);

        JSLFatPtr slice3 = jsl_fatptr_slice(buffer1, 3, 8);
        TEST_BOOL(jsl_fatptr_memory_compare(buffer3, slice3));
    }
}

static void test_jsl_fatptr_total_write_length(void)
{
    {
        uint8_t buffer[32] = {0};
        JSLFatPtr original = JSL_FATPTR_FROM_STACK(buffer);
        JSLFatPtr writer = original;

        int64_t memcpy_res = jsl_fatptr_cstr_memory_copy(&writer, "abc", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 3);
        memcpy_res = jsl_fatptr_cstr_memory_copy(&writer, "defg", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 4);

        int64_t length_written = jsl_fatptr_total_write_length(original, writer);
        TEST_INT64_EQUAL(length_written, (int64_t) 7);
        TEST_BOOL(memcmp(buffer, "abcdefg", 7) == 0);
    }

    {
        uint8_t buffer[8] = {0};
        JSLFatPtr original = JSL_FATPTR_FROM_STACK(buffer);
        JSLFatPtr writer = original;

        int64_t length_written = jsl_fatptr_total_write_length(original, writer);
        TEST_INT64_EQUAL(length_written, (int64_t) 0);

        writer = jsl_fatptr_slice(original, original.length, original.length);
        length_written = jsl_fatptr_total_write_length(original, writer);
        TEST_INT64_EQUAL(length_written, (int64_t) original.length);
    }
}

static void test_jsl_fatptr_auto_slice(void)
{
    {
        uint8_t buffer[32] = {0};
        JSLFatPtr original = JSL_FATPTR_FROM_STACK(buffer);
        JSLFatPtr writer = original;

        int64_t memcpy_res = jsl_fatptr_cstr_memory_copy(&writer, "Hello", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 5);
        memcpy_res = jsl_fatptr_cstr_memory_copy(&writer, "World", false);
        TEST_INT64_EQUAL(memcpy_res, (int64_t) 5);

        JSLFatPtr slice = jsl_fatptr_auto_slice(original, writer);
        TEST_INT64_EQUAL(slice.length, (int64_t) 10);
        TEST_BOOL(slice.data == original.data);
        TEST_BOOL(memcmp(slice.data, "HelloWorld", 10) == 0);
    }

    {
        uint8_t buffer[4] = {0};
        JSLFatPtr original = JSL_FATPTR_FROM_STACK(buffer);
        JSLFatPtr writer = original;

        JSLFatPtr slice = jsl_fatptr_auto_slice(original, writer);
        TEST_INT64_EQUAL(slice.length, (int64_t) 0);
        TEST_BOOL(slice.data == original.data);
    }

    {
        uint8_t buffer[] = {'x', 'y', 'z', 'w', 'q', 'p'};
        JSLFatPtr original = jsl_fatptr_init(buffer, (int64_t) sizeof(buffer));
        JSLFatPtr writer = jsl_fatptr_slice(original, 4, original.length);

        JSLFatPtr slice = jsl_fatptr_auto_slice(original, writer);
        TEST_INT64_EQUAL(slice.length, (int64_t) 4);
        TEST_BOOL(slice.data == original.data);
        TEST_BOOL(memcmp(slice.data, "xyzw", 4) == 0);
    }
}

static void test_jsl_fatptr_strip_whitespace_left(void)
{
    {
        JSLFatPtr empty = {0};
        int64_t res = jsl_fatptr_strip_whitespace_left(&empty);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr negative_length = {
            .data = (uint8_t*) "  Hello",
            .length = -5
        };
        int64_t res = jsl_fatptr_strip_whitespace_left(&negative_length);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr str = JSL_FATPTR_INITIALIZER("Hello");
        int64_t res = jsl_fatptr_strip_whitespace_left(&str);
        TEST_INT64_EQUAL(res, (int64_t) 0);
        TEST_BOOL(jsl_fatptr_cstr_compare(str, "Hello"));
    }

    {
        JSLFatPtr original = JSL_FATPTR_INITIALIZER(" \t\nHello");
        JSLFatPtr str = original;

        int64_t res = jsl_fatptr_strip_whitespace_left(&str);
        TEST_INT64_EQUAL(res, (int64_t) 3);
        TEST_BOOL(str.data == original.data + 3);
        TEST_INT64_EQUAL(str.length, (int64_t) 5);
        TEST_BOOL(jsl_fatptr_cstr_compare(str, "Hello"));
    }

    {
        JSLFatPtr original = JSL_FATPTR_INITIALIZER(" \t\n\r");
        JSLFatPtr str = original;

        int64_t res = jsl_fatptr_strip_whitespace_left(&str);
        TEST_INT64_EQUAL(res, (int64_t) original.length);
        TEST_BOOL(str.data == original.data + original.length);
        TEST_INT64_EQUAL(str.length, (int64_t) 0);
    }
}

static void test_jsl_fatptr_strip_whitespace_right(void)
{
    {
        JSLFatPtr empty = {0};
        int64_t res = jsl_fatptr_strip_whitespace_right(&empty);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr negative_length = {
            .data = (uint8_t*) "Hello  ",
            .length = -2
        };
        int64_t res = jsl_fatptr_strip_whitespace_right(&negative_length);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr str = JSL_FATPTR_INITIALIZER("Hello");
        int64_t res = jsl_fatptr_strip_whitespace_right(&str);
        TEST_INT64_EQUAL(res, (int64_t) 0);
        TEST_BOOL(jsl_fatptr_cstr_compare(str, "Hello"));
    }

    {
        JSLFatPtr original = JSL_FATPTR_INITIALIZER("Hello\t  ");
        JSLFatPtr str = original;

        int64_t res = jsl_fatptr_strip_whitespace_right(&str);
        TEST_INT64_EQUAL(res, (int64_t) 3);
        TEST_BOOL(str.data == original.data);
        TEST_INT64_EQUAL(str.length, original.length - 3);
        TEST_BOOL(jsl_fatptr_cstr_compare(str, "Hello"));
    }

    {
        JSLFatPtr original = JSL_FATPTR_INITIALIZER(" \t\n\r");
        JSLFatPtr str = original;

        int64_t res = jsl_fatptr_strip_whitespace_right(&str);
        TEST_INT64_EQUAL(res, (int64_t) original.length);
        TEST_BOOL(str.data == original.data);
        TEST_INT64_EQUAL(str.length, (int64_t) 0);
    }
}

static void test_jsl_fatptr_strip_whitespace(void)
{
    {
        JSLFatPtr empty = {0};
        int64_t res = jsl_fatptr_strip_whitespace(&empty);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr negative_length = {
            .data = (uint8_t*) "   Hello   ",
            .length = -10
        };
        int64_t res = jsl_fatptr_strip_whitespace(&negative_length);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr str = JSL_FATPTR_INITIALIZER("Hello");
        int64_t res = jsl_fatptr_strip_whitespace(&str);
        TEST_INT64_EQUAL(res, (int64_t) 0);
        TEST_BOOL(jsl_fatptr_cstr_compare(str, "Hello"));
    }

    {
        JSLFatPtr original = JSL_FATPTR_INITIALIZER("  Hello World \n\t");
        JSLFatPtr str = original;

        int64_t res = jsl_fatptr_strip_whitespace(&str);
        TEST_INT64_EQUAL(res, (int64_t) 5);
        TEST_BOOL(str.data == original.data + 2);
        TEST_INT64_EQUAL(str.length, original.length - 5);
        TEST_BOOL(jsl_fatptr_cstr_compare(str, "Hello World"));
    }

    {
        JSLFatPtr original = JSL_FATPTR_INITIALIZER("\t \n ");
        JSLFatPtr str = original;

        int64_t res = jsl_fatptr_strip_whitespace(&str);
        TEST_INT64_EQUAL(res, (int64_t) original.length);
        TEST_BOOL(str.data == original.data + original.length);
        TEST_INT64_EQUAL(str.length, (int64_t) 0);
    }
}

static void test_jsl_fatptr_substring_search(void)
{
    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("111111");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("111111");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("Longer substring than the original string");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("111111");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("1");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) 0);
    }

    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("W");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) 7);
    }

    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("World");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) 7);
    }

    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("Hello, World!");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) 0);
    }

    {
        JSLFatPtr string = JSL_FATPTR_INITIALIZER("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("Blorp");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("8-bit");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 117);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("8-blit");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("Blorf");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) -1);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("ASCII/UTF-8");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 162);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 85);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 0);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("i");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 6);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("at");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 122);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("Sed");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 1171);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("elit");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 51);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("vitae");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 263);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_INITIALIZER("Lorem");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        TEST_INT64_EQUAL(res, (int64_t) 0);
    }
}

static void test_jsl_fatptr_index_of(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("");
    int64_t res1 = jsl_fatptr_index_of(buffer1, '3');
    TEST_BOOL(res1 == -1);

    JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER(".");
    int64_t res2 = jsl_fatptr_index_of(buffer2, '.');
    TEST_BOOL(res2 == 0);

    JSLFatPtr buffer3 = JSL_FATPTR_INITIALIZER("......");
    int64_t res3 = jsl_fatptr_index_of(buffer3, '.');
    TEST_BOOL(res3 == 0);

    JSLFatPtr buffer4 = JSL_FATPTR_INITIALIZER("Hello.World");
    int64_t res4 = jsl_fatptr_index_of(buffer4, '.');
    TEST_BOOL(res4 == 5);

    JSLFatPtr buffer5 = JSL_FATPTR_INITIALIZER("Hello          . Hello");
    int64_t res5 = jsl_fatptr_index_of(buffer5, '.');
    TEST_BOOL(res5 == 15);

    JSLFatPtr buffer6 = JSL_FATPTR_INITIALIZER("Hello.World.");
    int64_t res6 = jsl_fatptr_index_of(buffer6, '.');
    TEST_BOOL(res6 == 5);

    JSLFatPtr buffer7 = JSL_FATPTR_INITIALIZER("Hello Hello ");
    int64_t res7 = jsl_fatptr_index_of(buffer7, ' ');
    TEST_BOOL(res7 == 5);

    JSLFatPtr buffer8 = JSL_FATPTR_INITIALIZER("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res8 = jsl_fatptr_index_of(buffer8, '8');
    TEST_BOOL(res8 == 117);
}

static void test_jsl_fatptr_index_of_reverse(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("");
    int64_t res1 = jsl_fatptr_index_of_reverse(buffer1, '3');
    TEST_BOOL(res1 == -1);

    JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER(".");
    int64_t res2 = jsl_fatptr_index_of_reverse(buffer2, '.');
    TEST_BOOL(res2 == 0);

    JSLFatPtr buffer3 = JSL_FATPTR_INITIALIZER("......");
    int64_t res3 = jsl_fatptr_index_of_reverse(buffer3, '.');
    TEST_BOOL(res3 == 5);

    JSLFatPtr buffer4 = JSL_FATPTR_INITIALIZER("Hello.World");
    int64_t res4 = jsl_fatptr_index_of_reverse(buffer4, '.');
    TEST_BOOL(res4 == 5);

    JSLFatPtr buffer5 = JSL_FATPTR_INITIALIZER("Hello          . Hello");
    int64_t res5 = jsl_fatptr_index_of_reverse(buffer5, '.');
    TEST_BOOL(res5 == 15);

    JSLFatPtr buffer6 = JSL_FATPTR_INITIALIZER("Hello.World.");
    int64_t res6 = jsl_fatptr_index_of_reverse(buffer6, '.');
    TEST_BOOL(res6 == 11);

    JSLFatPtr buffer7 = JSL_FATPTR_INITIALIZER("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res7 = jsl_fatptr_index_of_reverse(buffer7, 'M');
    TEST_BOOL(res7 == 54);

    JSLFatPtr buffer8 = JSL_FATPTR_INITIALIZER("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res8 = jsl_fatptr_index_of_reverse(buffer8, 'w');
    TEST_BOOL(res8 == 150);
}

static void test_jsl_fatptr_get_file_extension(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("");
    JSLFatPtr res1 = jsl_fatptr_get_file_extension(buffer1);
    TEST_BOOL(res1.data == NULL);

    JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER(".");
    JSLFatPtr res2 = jsl_fatptr_get_file_extension(buffer2);
    TEST_BOOL(jsl_fatptr_cstr_compare(res2, ""));

    JSLFatPtr buffer3 = JSL_FATPTR_INITIALIZER("......");
    JSLFatPtr res3 = jsl_fatptr_get_file_extension(buffer3);
    TEST_BOOL(jsl_fatptr_cstr_compare(res3, ""));

    JSLFatPtr buffer4 = JSL_FATPTR_INITIALIZER("Hello.text");
    JSLFatPtr res4 = jsl_fatptr_get_file_extension(buffer4);
    TEST_BOOL(jsl_fatptr_cstr_compare(res4, "text"));

    JSLFatPtr buffer5 = JSL_FATPTR_INITIALIZER("Hello          .css");
    JSLFatPtr res5 = jsl_fatptr_get_file_extension(buffer5);
    TEST_BOOL(jsl_fatptr_cstr_compare(res5, "css"));

    JSLFatPtr buffer6 = JSL_FATPTR_INITIALIZER("Hello.min.css");
    JSLFatPtr res6 = jsl_fatptr_get_file_extension(buffer6);
    TEST_BOOL(jsl_fatptr_cstr_compare(res6, "css"));
}

static void test_jsl_fatptr_to_lowercase_ascii(void)
{
    JSLArena arena;
    jsl_arena_init(&arena, malloc(1024), 1024);

    JSLFatPtr buffer1 = jsl_cstr_to_fatptr(&arena, "10023");
    jsl_fatptr_to_lowercase_ascii(buffer1);
    TEST_BOOL(jsl_fatptr_cstr_compare(buffer1, "10023"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer2 = jsl_cstr_to_fatptr(&arena, "hello!@#$@*()");
    jsl_fatptr_to_lowercase_ascii(buffer2);
    TEST_BOOL(jsl_fatptr_cstr_compare(buffer2, "hello!@#$@*()"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer3 = jsl_cstr_to_fatptr(&arena, "Population");
    jsl_fatptr_to_lowercase_ascii(buffer3);
    TEST_BOOL(jsl_fatptr_cstr_compare(buffer3, "population"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer4 = jsl_cstr_to_fatptr(&arena, "ENTRUSTED");
    jsl_fatptr_to_lowercase_ascii(buffer4);
    TEST_BOOL(jsl_fatptr_cstr_compare(buffer4, "entrusted"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer5 = jsl_cstr_to_fatptr(&arena, (char*) u8"Footnotes Ω≈ç√∫");
    jsl_fatptr_to_lowercase_ascii(buffer5);
    TEST_BOOL(jsl_fatptr_cstr_compare(buffer5, (char*) u8"footnotes Ω≈ç√∫"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer6 = jsl_cstr_to_fatptr(&arena, (char*) u8"Ω≈ç√∫");
    jsl_fatptr_to_lowercase_ascii(buffer6);
    TEST_BOOL(jsl_fatptr_cstr_compare(buffer6, (char*) u8"Ω≈ç√∫"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer7 = jsl_cstr_to_fatptr(&arena, (char*) u8"Ω≈ç√∫ ENTRUSTED this is a longer string to activate the SIMD path!");
    jsl_fatptr_to_lowercase_ascii(buffer7);
    TEST_BOOL(jsl_fatptr_cstr_compare(buffer7, (char*) u8"Ω≈ç√∫ entrusted this is a longer string to activate the simd path!"));

    jsl_arena_reset(&arena);
}

static void test_jsl_fatptr_to_int32(void)
{
    int32_t result;

    JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("0");
    TEST_BOOL(jsl_fatptr_to_int32(buffer1, &result) == 1);
    TEST_BOOL(result == 0);

    JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("-0");
    TEST_BOOL(jsl_fatptr_to_int32(buffer2, &result) == 2);
    TEST_BOOL(result == 0);

    JSLFatPtr buffer3 = JSL_FATPTR_INITIALIZER("11");
    TEST_BOOL(jsl_fatptr_to_int32(buffer3, &result) == 2);
    TEST_BOOL(result == 11);

    JSLFatPtr buffer4 = JSL_FATPTR_INITIALIZER("-1243");
    TEST_BOOL(jsl_fatptr_to_int32(buffer4, &result) == 5);
    TEST_BOOL(result == -1243);

    JSLFatPtr buffer5 = JSL_FATPTR_INITIALIZER("000003");
    TEST_BOOL(jsl_fatptr_to_int32(buffer5, &result) == 6);
    TEST_BOOL(result == 3);

    JSLFatPtr buffer6 = JSL_FATPTR_INITIALIZER("000000");
    TEST_BOOL(jsl_fatptr_to_int32(buffer6, &result) == 6);
    TEST_BOOL(result == 0);

    JSLFatPtr buffer7 = JSL_FATPTR_INITIALIZER("-000000");
    TEST_BOOL(jsl_fatptr_to_int32(buffer7, &result) == 7);
    TEST_BOOL(result == 0);

    JSLFatPtr buffer8 = JSL_FATPTR_INITIALIZER("98468465");
    TEST_BOOL(jsl_fatptr_to_int32(buffer8, &result) == 8);
    TEST_BOOL(result == 98468465);

    JSLFatPtr buffer9 = JSL_FATPTR_INITIALIZER("454 hello, world");
    TEST_BOOL(jsl_fatptr_to_int32(buffer9, &result) == 3);
    TEST_BOOL(result == 454);

    JSLFatPtr buffer10 = JSL_FATPTR_INITIALIZER("+488 hello, world");
    TEST_BOOL(jsl_fatptr_to_int32(buffer10, &result) == 4);
    TEST_BOOL(result == 488);
}

static void test_jsl_fatptr_starts_with(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr prefix1 = JSL_FATPTR_INITIALIZER("Hello, World!");
    TEST_BOOL(jsl_fatptr_starts_with(buffer1, prefix1) == true);

    JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr prefix2 = JSL_FATPTR_INITIALIZER("Hello");
    TEST_BOOL(jsl_fatptr_starts_with(buffer2, prefix2) == true);

    JSLFatPtr buffer3 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr prefix3 = JSL_FATPTR_INITIALIZER("World");
    TEST_BOOL(jsl_fatptr_starts_with(buffer3, prefix3) == false);

    JSLFatPtr buffer4 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr prefix4 = JSL_FATPTR_INITIALIZER("");
    TEST_BOOL(jsl_fatptr_starts_with(buffer4, prefix4) == true);

    JSLFatPtr buffer5 = JSL_FATPTR_INITIALIZER("");
    JSLFatPtr prefix5 = JSL_FATPTR_INITIALIZER("");
    TEST_BOOL(jsl_fatptr_starts_with(buffer5, prefix5) == true);

    JSLFatPtr buffer6 = JSL_FATPTR_INITIALIZER("");
    JSLFatPtr prefix6 = JSL_FATPTR_INITIALIZER("HHHHHHHHHHHHHHHHH");
    TEST_BOOL(jsl_fatptr_starts_with(buffer6, prefix6) == false);

    JSLFatPtr buffer7 = JSL_FATPTR_INITIALIZER("HHHHHHHHHHHHHHHH");
    JSLFatPtr prefix7 = JSL_FATPTR_INITIALIZER("HHHHHHHHHHHHHHHHH");
    TEST_BOOL(jsl_fatptr_starts_with(buffer7, prefix7) == false);

    JSLFatPtr buffer8 = JSL_FATPTR_INITIALIZER("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
    JSLFatPtr prefix8 = JSL_FATPTR_INITIALIZER("This is a string example that will ");
    TEST_BOOL(jsl_fatptr_starts_with(buffer8, prefix8));
}

static void test_jsl_fatptr_ends_with(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr postfix1 = JSL_FATPTR_INITIALIZER("Hello, World!");
    TEST_BOOL(jsl_fatptr_ends_with(buffer1, postfix1) == true);

    JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr postfix2 = JSL_FATPTR_INITIALIZER("World!");
    TEST_BOOL(jsl_fatptr_ends_with(buffer2, postfix2) == true);

    JSLFatPtr buffer3 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr postfix3 = JSL_FATPTR_INITIALIZER("Hello");
    TEST_BOOL(jsl_fatptr_ends_with(buffer3, postfix3) == false);

    JSLFatPtr buffer4 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr postfix4 = JSL_FATPTR_INITIALIZER("");
    TEST_BOOL(jsl_fatptr_ends_with(buffer4, postfix4) == true);

    JSLFatPtr buffer5 = JSL_FATPTR_INITIALIZER("");
    JSLFatPtr postfix5 = JSL_FATPTR_INITIALIZER("");
    TEST_BOOL(jsl_fatptr_ends_with(buffer5, postfix5) == true);

    JSLFatPtr buffer6 = JSL_FATPTR_INITIALIZER("");
    JSLFatPtr postfix6 = JSL_FATPTR_INITIALIZER("HHHHHHHHHHHHHHHHH");
    TEST_BOOL(jsl_fatptr_ends_with(buffer6, postfix6) == false);

    JSLFatPtr buffer7 = JSL_FATPTR_INITIALIZER("HHHHHHHHHHHHHHHH");
    JSLFatPtr postfix7 = JSL_FATPTR_INITIALIZER("HHHHHHHHHHHHHHHHH");
    TEST_BOOL(jsl_fatptr_ends_with(buffer7, postfix7) == false);

    JSLFatPtr buffer8 = JSL_FATPTR_INITIALIZER("Hello, World!");
    JSLFatPtr postfix8 = JSL_FATPTR_INITIALIZER("!");
    TEST_BOOL(jsl_fatptr_ends_with(buffer8, postfix8) == true);

    JSLFatPtr postfix9 = JSL_FATPTR_INITIALIZER(" are since we're using ASCII/UTF-8.");
    TEST_BOOL(jsl_fatptr_ends_with(medium_str, postfix9));
}

static void test_jsl_fatptr_compare_ascii_insensitive(void)
{
    {
        JSLFatPtr buffer1 = {
            .data = NULL,
            .length = 0
        };
        JSLFatPtr buffer2 = {
            .data = NULL,
            .length = 0
        };
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("Hello, World!");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("Hello, World!");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("Hello, World!");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("Hello, World!");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("hello, world!");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("AAAAAAAAAA");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("AaaaAaAaAA");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = {
            .data = NULL,
            .length = 0
        };
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }
    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("This is a string example that WILL span multiple AVX2 chunks so that we can test if the loop is working properly.");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_INITIALIZER("This is a string example that WILL span multiple AVX2 chunkz so that we can test if the loop is workING properly.");
        JSLFatPtr buffer2 = JSL_FATPTR_INITIALIZER("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        TEST_BOOL(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }
}

static void test_jsl_fatptr_count(void)
{
    {
        JSLFatPtr buffer = JSL_FATPTR_INITIALIZER("");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_fatptr_count(buffer, item), (int64_t) 0);
    }

    {
        JSLFatPtr buffer = JSL_FATPTR_INITIALIZER("Test string");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_fatptr_count(buffer, item), (int64_t) 0);
    }

    {
        JSLFatPtr buffer = JSL_FATPTR_INITIALIZER("Test string a");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_fatptr_count(buffer, item), (int64_t) 1);
    }

    {
        JSLFatPtr buffer = JSL_FATPTR_INITIALIZER("a Test string");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_fatptr_count(buffer, item), (int64_t) 1);
    }

    {
        JSLFatPtr buffer = JSL_FATPTR_INITIALIZER("A Test string");
        uint8_t item = 'a';
        TEST_INT64_EQUAL(jsl_fatptr_count(buffer, item), (int64_t) 0);
    }

    {
        uint8_t item = 'i';
        TEST_INT64_EQUAL(jsl_fatptr_count(medium_str, item), (int64_t) 14);
    }

    {
        uint8_t item = 'z';
        TEST_INT64_EQUAL(jsl_fatptr_count(medium_str, item), (int64_t) 0);
    }

    {
        uint8_t item = 'i';
        TEST_INT64_EQUAL(jsl_fatptr_count(long_str, item), (int64_t) 129);
    }

    {
        uint8_t item = '=';
        TEST_INT64_EQUAL(jsl_fatptr_count(long_str, item), (int64_t) 0);
    }
}

static void test_jsl_fatptr_to_cstr(void)
{
    JSLArena arena;
    jsl_arena_init(&arena, malloc(1024), 1024);

    {
        JSLFatPtr fatptr = {0};
        char* cstr = jsl_fatptr_to_cstr(&arena, fatptr);
        TEST_BOOL(cstr == NULL);
    }

    jsl_arena_reset(&arena);

    {
        JSLFatPtr fatptr = JSL_FATPTR_INITIALIZER("10023");
        char* cstr = jsl_fatptr_to_cstr(&arena, fatptr);
        TEST_BOOL(jsl_fatptr_cstr_compare(fatptr, cstr));
    }

    jsl_arena_reset(&arena);

    {
        JSLFatPtr fatptr = JSL_FATPTR_INITIALIZER(u8"Ω≈ç√∫");
        char* cstr = jsl_fatptr_to_cstr(&arena, fatptr);
        TEST_BOOL(jsl_fatptr_cstr_compare(fatptr, cstr));
    }
}

int main(void)
{
    RUN_TEST_FUNCTION("Test jsl_fatptr_from_cstr", test_jsl_fatptr_from_cstr);
    RUN_TEST_FUNCTION("Test jsl_fatptr_cstr_memory_copy", test_jsl_fatptr_cstr_memory_copy);
    RUN_TEST_FUNCTION("Test jsl_fatptr_memory_compare", test_jsl_fatptr_memory_compare);
    RUN_TEST_FUNCTION("Test jsl_fatptr_slice", test_jsl_fatptr_slice);
    RUN_TEST_FUNCTION("Test jsl_fatptr_total_write_length", test_jsl_fatptr_total_write_length);
    RUN_TEST_FUNCTION("Test jsl_fatptr_auto_slice", test_jsl_fatptr_auto_slice);
    RUN_TEST_FUNCTION("Test jsl_fatptr_strip_whitespace_left", test_jsl_fatptr_strip_whitespace_left);
    RUN_TEST_FUNCTION("Test jsl_fatptr_strip_whitespace_right", test_jsl_fatptr_strip_whitespace_right);
    RUN_TEST_FUNCTION("Test jsl_fatptr_strip_whitespace", test_jsl_fatptr_strip_whitespace);
    RUN_TEST_FUNCTION("Test jsl_fatptr_index_of", test_jsl_fatptr_index_of);
    RUN_TEST_FUNCTION("Test jsl_fatptr_index_of_reverse", test_jsl_fatptr_index_of_reverse);
    RUN_TEST_FUNCTION("Test jsl_fatptr_to_lowercase_ascii", test_jsl_fatptr_to_lowercase_ascii);
    RUN_TEST_FUNCTION("Test jsl_fatptr_to_int32", test_jsl_fatptr_to_int32);
    RUN_TEST_FUNCTION("Test jsl_fatptr_substring_search", test_jsl_fatptr_substring_search);
    RUN_TEST_FUNCTION("Test jsl_fatptr_starts_with", test_jsl_fatptr_starts_with);
    RUN_TEST_FUNCTION("Test jsl_fatptr_ends_with", test_jsl_fatptr_ends_with);
    RUN_TEST_FUNCTION("Test jsl_fatptr_compare_ascii_insensitive", test_jsl_fatptr_compare_ascii_insensitive);
    RUN_TEST_FUNCTION("Test jsl_fatptr_count", test_jsl_fatptr_count);
    RUN_TEST_FUNCTION("Test jsl_fatptr_to_cstr", test_jsl_fatptr_to_cstr);
    RUN_TEST_FUNCTION("Test jsl_fatptr_get_file_extension", test_jsl_fatptr_get_file_extension);

    TEST_RESULTS();
    return lfails != 0;
}

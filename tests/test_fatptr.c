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
#define JSL_INCLUDE_FILE_UTILS
#include "../src/jacks_standard_library.h"

#include "minctest.h"

void test_jsl_fatptr_from_cstr(void)
{
    char* c_str = "This is a test string!";
    size_t length = strlen(c_str);

    JSLFatPtr str = jsl_fatptr_from_cstr(c_str);

    lok((void*) str.data == (void*) c_str);
    lok((int64_t) str.length == (int64_t) length);
    lok(memcmp(c_str, str.data, str.length) == 0);
}

void test_jsl_fatptr_cstr_memory_copy(void)
{
    JSLFatPtr buffer = jsl_fatptr_init(malloc(1024), 1024);
    lok((int64_t) buffer.length == (int64_t) 1024);

    JSLFatPtr writer = buffer;
    char* str = "This is a test string!";
    int64_t length = (int64_t) strlen(str);
    jsl_fatptr_cstr_memory_copy(&writer, str, false);

    lok(writer.data == buffer.data + length);
    lok(writer.length == 1024 - length);
    lok((int64_t) buffer.length == (int64_t) 1024);

    lok(memcmp(str, buffer.data, length) == 0);
}

void test_jsl_load_file_contents(void)
{
    #if defined(_WIN32)
        char* path = "tests\\example.txt";
    #else
        char* path = "./tests/example.txt";
    #endif

    char stack_buffer[4*1024] = {0};
    int64_t file_size;

    // Load the comparison using libc
    {
        FILE* file = fopen(path, "rb");
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        lok(file_size > 0);
        rewind(file);

        fread(stack_buffer, file_size, 1, file);
    }

    JSLArena arena;
    jsl_arena_init(&arena, malloc(4*1024), 4*1024);

    JSLFatPtr contents;
    JSLLoadFileResultEnum res = jsl_load_file_contents(
        &arena,
        jsl_fatptr_from_cstr(path),
        &contents,
        NULL
    );

    lok(res == JSL_FILE_LOAD_SUCCESS);
    // printf("stack_buffer %s\n", stack_buffer);
    // jsl_format_file(stdout, JSL_FATPTR_LITERAL("%y\n"), contents);
    lmemcmp(stack_buffer, contents.data, file_size);
}

void test_jsl_load_file_contents_buffer(void)
{
    char* path = "./tests/example.txt";
    char stack_buffer[4*1024];
    int64_t file_size;

    // Load the comparison using libc
    {
        FILE* file = fopen(path, "rb");
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        lok(file_size > 0);
        rewind(file);

        fread(stack_buffer, file_size, 1, file);
    }

    JSLFatPtr buffer = jsl_fatptr_init(malloc(4*1024), 4*1024);
    JSLFatPtr writer = buffer;

    JSLLoadFileResultEnum res = jsl_load_file_contents_buffer(
        &writer,
        JSL_FATPTR_LITERAL("./tests/example.txt"),
        NULL
    );

    lok(res == JSL_FILE_LOAD_SUCCESS);
    lok(memcmp(stack_buffer, buffer.data, file_size) == 0);
}

void test_jsl_fatptr_memory_compare(void)
{
    JSLFatPtr buffer1 = jsl_fatptr_init(malloc(13), 13);
    JSLFatPtr buffer2 = jsl_fatptr_init(malloc(13), 13);
    JSLFatPtr buffer3 = jsl_fatptr_init(malloc(13), 13);
    JSLFatPtr buffer4 = jsl_fatptr_init(malloc(20), 20);

    JSLFatPtr writer1 = buffer1;
    JSLFatPtr writer2 = buffer2;
    JSLFatPtr writer3 = buffer3;
    JSLFatPtr writer4 = buffer4;

    jsl_fatptr_cstr_memory_copy(&writer1, "Hello, World!", false);
    jsl_fatptr_cstr_memory_copy(&writer2, "Hello, Owrld!", false);
    jsl_fatptr_cstr_memory_copy(&writer3, "Hello, World!", false);
    jsl_fatptr_cstr_memory_copy(&writer4, "Hello, World!", false);

    lok( jsl_fatptr_memory_compare(buffer1, buffer1));
    lok(!jsl_fatptr_memory_compare(buffer1, buffer2));
    lok( jsl_fatptr_memory_compare(buffer1, buffer3));
    lok(!jsl_fatptr_memory_compare(buffer1, buffer4));
}

void test_jsl_fatptr_slice(void)
{
    JSLFatPtr buffer1 = jsl_fatptr_init(malloc(13), 13);

    {
        JSLFatPtr writer1 = buffer1;
        jsl_fatptr_cstr_memory_copy(&writer1, "Hello, World!", false);

        JSLFatPtr slice1 = jsl_fatptr_slice(buffer1, 0, buffer1.length);
        lok(jsl_fatptr_memory_compare(buffer1, slice1));
    }

    {
        JSLFatPtr buffer2 = jsl_fatptr_init(malloc(10), 10);
        JSLFatPtr writer2 = buffer2;
        jsl_fatptr_cstr_memory_copy(&writer2, "Hello, Wor", false);

        JSLFatPtr slice2 = jsl_fatptr_slice(buffer1, 0, 10);
        lok(jsl_fatptr_memory_compare(buffer2, slice2));
    }

    {
        JSLFatPtr buffer3 = jsl_fatptr_init(malloc(5), 5);
        JSLFatPtr writer3 = buffer3;
        jsl_fatptr_cstr_memory_copy(&writer3, "lo, W", false);

        JSLFatPtr slice3 = jsl_fatptr_slice(buffer1, 3, 8);
        lok(jsl_fatptr_memory_compare(buffer3, slice3));
    }
}

void test_jsl_fatptr_substring_search(void)
{
    JSLFatPtr medium_str = JSL_FATPTR_LITERAL(
        "This is a very long string that is going to trigger SIMD code, "
        "as it's longer than a single AVX2 register when using 8-bit "
        "values, which we are since we're using ASCII/UTF-8."
    );
    JSLFatPtr long_str = JSL_FATPTR_LITERAL(
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

    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, -1LL);
    }
    
    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("111111");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, -1LL);
    }

    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("111111");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, -1LL);
    }

    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("Longer substring than the original string");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, -1LL);
    }

    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("111111");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("1");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, 0LL);
    }

    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("W");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, 7LL);
    }

    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("World");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, 7LL);
    }

    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("Hello, World!");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, 0LL);
    }

    {
        JSLFatPtr string = JSL_FATPTR_LITERAL("Hello, World!");
        JSLFatPtr substring = JSL_FATPTR_LITERAL("Blorp");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        l_long_long_equal(res, -1LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("8-bit");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        l_long_long_equal(res, 117LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("8-blit");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        l_long_long_equal(res, -1LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("Blorf");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        l_long_long_equal(res, -1LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("ASCII/UTF-8");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        l_long_long_equal(res, 162LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        l_long_long_equal(res, 85LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        int64_t res = jsl_fatptr_substring_search(medium_str, substring);
        l_long_long_equal(res, 0LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("i");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        l_long_long_equal(res, 0LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("at");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        l_long_long_equal(res, 0LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("Sed");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        l_long_long_equal(res, 0LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("elit");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        l_long_long_equal(res, 0LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("vitae");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        l_long_long_equal(res, 0LL);
    }

    {
        JSLFatPtr substring = JSL_FATPTR_LITERAL("Lorem");
        int64_t res = jsl_fatptr_substring_search(long_str, substring);
        l_long_long_equal(res, 0LL);
    }
}

void test_jsl_fatptr_index_of(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("");
    int64_t res1 = jsl_fatptr_index_of(buffer1, '3');
    lok(res1 == -1);

    JSLFatPtr buffer2 = JSL_FATPTR_LITERAL(".");
    int64_t res2 = jsl_fatptr_index_of(buffer2, '.');
    lok(res2 == 0);

    JSLFatPtr buffer3 = JSL_FATPTR_LITERAL("......");
    int64_t res3 = jsl_fatptr_index_of(buffer3, '.');
    lok(res3 == 0);

    JSLFatPtr buffer4 = JSL_FATPTR_LITERAL("Hello.World");
    int64_t res4 = jsl_fatptr_index_of(buffer4, '.');
    lok(res4 == 5);

    JSLFatPtr buffer5 = JSL_FATPTR_LITERAL("Hello          . Hello");
    int64_t res5 = jsl_fatptr_index_of(buffer5, '.');
    lok(res5 == 15);

    JSLFatPtr buffer6 = JSL_FATPTR_LITERAL("Hello.World.");
    int64_t res6 = jsl_fatptr_index_of(buffer6, '.');
    lok(res6 == 5);

    JSLFatPtr buffer7 = JSL_FATPTR_LITERAL("Hello Hello ");
    int64_t res7 = jsl_fatptr_index_of(buffer7, ' ');
    lok(res7 == 5);

    JSLFatPtr buffer8 = JSL_FATPTR_LITERAL("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res8 = jsl_fatptr_index_of(buffer8, '8');
    lok(res8 == 117);
}

void test_jsl_fatptr_index_of_reverse(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("");
    int64_t res1 = jsl_fatptr_index_of_reverse(buffer1, '3');
    lok(res1 == -1);

    JSLFatPtr buffer2 = JSL_FATPTR_LITERAL(".");
    int64_t res2 = jsl_fatptr_index_of_reverse(buffer2, '.');
    lok(res2 == 0);

    JSLFatPtr buffer3 = JSL_FATPTR_LITERAL("......");
    int64_t res3 = jsl_fatptr_index_of_reverse(buffer3, '.');
    lok(res3 == 5);

    JSLFatPtr buffer4 = JSL_FATPTR_LITERAL("Hello.World");
    int64_t res4 = jsl_fatptr_index_of_reverse(buffer4, '.');
    lok(res4 == 5);

    JSLFatPtr buffer5 = JSL_FATPTR_LITERAL("Hello          . Hello");
    int64_t res5 = jsl_fatptr_index_of_reverse(buffer5, '.');
    lok(res5 == 15);

    JSLFatPtr buffer6 = JSL_FATPTR_LITERAL("Hello.World.");
    int64_t res6 = jsl_fatptr_index_of_reverse(buffer6, '.');
    lok(res6 == 11);

    JSLFatPtr buffer7 = JSL_FATPTR_LITERAL("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res7 = jsl_fatptr_index_of_reverse(buffer7, 'M');
    lok(res7 == 54);

    JSLFatPtr buffer8 = JSL_FATPTR_LITERAL("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res8 = jsl_fatptr_index_of_reverse(buffer8, 'w');
    lok(res8 == 150);
}

void test_jsl_fatptr_get_file_extension(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("");
    JSLFatPtr res1 = jsl_fatptr_get_file_extension(buffer1);
    lok(jsl_fatptr_cstr_compare(res1, ""));

    JSLFatPtr buffer2 = JSL_FATPTR_LITERAL(".");
    JSLFatPtr res2 = jsl_fatptr_get_file_extension(buffer2);
    lok(jsl_fatptr_cstr_compare(res2, ""));

    JSLFatPtr buffer3 = JSL_FATPTR_LITERAL("......");
    JSLFatPtr res3 = jsl_fatptr_get_file_extension(buffer3);
    lok(jsl_fatptr_cstr_compare(res3, ""));

    JSLFatPtr buffer4 = JSL_FATPTR_LITERAL("Hello.text");
    JSLFatPtr res4 = jsl_fatptr_get_file_extension(buffer4);
    lok(jsl_fatptr_cstr_compare(res4, "text"));

    JSLFatPtr buffer5 = JSL_FATPTR_LITERAL("Hello          .css");
    JSLFatPtr res5 = jsl_fatptr_get_file_extension(buffer5);
    lok(jsl_fatptr_cstr_compare(res5, "css"));

    JSLFatPtr buffer6 = JSL_FATPTR_LITERAL("Hello.min.css");
    JSLFatPtr res6 = jsl_fatptr_get_file_extension(buffer6);
    lok(jsl_fatptr_cstr_compare(res6, "css"));
}

void test_jsl_fatptr_to_lowercase_ascii(void)
{
    JSLArena arena;
    jsl_arena_init(&arena, malloc(1024), 1024);

    JSLFatPtr buffer1 = jsl_arena_cstr_to_fatptr(&arena, "10023");
    jsl_fatptr_to_lowercase_ascii(buffer1);
    lok(jsl_fatptr_cstr_compare(buffer1, "10023"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer2 = jsl_arena_cstr_to_fatptr(&arena, "hello!@#$@*()");
    jsl_fatptr_to_lowercase_ascii(buffer2);
    lok(jsl_fatptr_cstr_compare(buffer2, "hello!@#$@*()"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer3 = jsl_arena_cstr_to_fatptr(&arena, "Population");
    jsl_fatptr_to_lowercase_ascii(buffer3);
    lok(jsl_fatptr_cstr_compare(buffer3, "population"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer4 = jsl_arena_cstr_to_fatptr(&arena, "ENTRUSTED");
    jsl_fatptr_to_lowercase_ascii(buffer4);
    lok(jsl_fatptr_cstr_compare(buffer4, "entrusted"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer5 = jsl_arena_cstr_to_fatptr(&arena, u8"Footnotes Ω≈ç√∫");
    jsl_fatptr_to_lowercase_ascii(buffer5);
    lok(jsl_fatptr_cstr_compare(buffer5, u8"footnotes Ω≈ç√∫"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer6 = jsl_arena_cstr_to_fatptr(&arena, u8"Ω≈ç√∫");
    jsl_fatptr_to_lowercase_ascii(buffer6);
    lok(jsl_fatptr_cstr_compare(buffer6, u8"Ω≈ç√∫"));

    jsl_arena_reset(&arena);

    JSLFatPtr buffer7 = jsl_arena_cstr_to_fatptr(&arena, u8"Ω≈ç√∫ ENTRUSTED this is a longer string to activate the SIMD path!");
    jsl_fatptr_to_lowercase_ascii(buffer7);
    lok(jsl_fatptr_cstr_compare(buffer7, u8"Ω≈ç√∫ entrusted this is a longer string to activate the simd path!"));

    jsl_arena_reset(&arena);
}

void test_jsl_fatptr_to_int32(void)
{
    int32_t result;

    JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("0");
    lok(jsl_fatptr_to_int32(buffer1, &result) == 1);
    lok(result == 0);

    JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("-0");
    lok(jsl_fatptr_to_int32(buffer2, &result) == 2);
    lok(result == 0);

    JSLFatPtr buffer3 = JSL_FATPTR_LITERAL("11");
    lok(jsl_fatptr_to_int32(buffer3, &result) == 2);
    lok(result == 11);

    JSLFatPtr buffer4 = JSL_FATPTR_LITERAL("-1243");
    lok(jsl_fatptr_to_int32(buffer4, &result) == 5);
    lok(result == -1243);

    JSLFatPtr buffer5 = JSL_FATPTR_LITERAL("000003");
    lok(jsl_fatptr_to_int32(buffer5, &result) == 6);
    lok(result == 3);

    JSLFatPtr buffer6 = JSL_FATPTR_LITERAL("000000");
    lok(jsl_fatptr_to_int32(buffer6, &result) == 6);
    lok(result == 0);

    JSLFatPtr buffer7 = JSL_FATPTR_LITERAL("-000000");
    lok(jsl_fatptr_to_int32(buffer7, &result) == 7);
    lok(result == 0);

    JSLFatPtr buffer8 = JSL_FATPTR_LITERAL("98468465");
    lok(jsl_fatptr_to_int32(buffer8, &result) == 8);
    lok(result == 98468465);

    JSLFatPtr buffer9 = JSL_FATPTR_LITERAL("454 hello, world");
    lok(jsl_fatptr_to_int32(buffer9, &result) == 3);
    lok(result == 454);

    JSLFatPtr buffer10 = JSL_FATPTR_LITERAL("+488 hello, world");
    lok(jsl_fatptr_to_int32(buffer10, &result) == 4);
    lok(result == 488);
}

void test_jsl_fatptr_starts_with(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr prefix1 = JSL_FATPTR_LITERAL("Hello, World!");
    lok(jsl_fatptr_starts_with(buffer1, prefix1) == true);

    JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr prefix2 = JSL_FATPTR_LITERAL("Hello");
    lok(jsl_fatptr_starts_with(buffer2, prefix2) == true);

    JSLFatPtr buffer3 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr prefix3 = JSL_FATPTR_LITERAL("World");
    lok(jsl_fatptr_starts_with(buffer3, prefix3) == false);

    JSLFatPtr buffer4 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr prefix4 = JSL_FATPTR_LITERAL("");
    lok(jsl_fatptr_starts_with(buffer4, prefix4) == true);

    JSLFatPtr buffer5 = JSL_FATPTR_LITERAL("");
    JSLFatPtr prefix5 = JSL_FATPTR_LITERAL("");
    lok(jsl_fatptr_starts_with(buffer5, prefix5) == true);

    JSLFatPtr buffer6 = JSL_FATPTR_LITERAL("");
    JSLFatPtr prefix6 = JSL_FATPTR_LITERAL("HHHHHHHHHHHHHHHHH");
    lok(jsl_fatptr_starts_with(buffer6, prefix6) == false);

    JSLFatPtr buffer7 = JSL_FATPTR_LITERAL("HHHHHHHHHHHHHHHH");
    JSLFatPtr prefix7 = JSL_FATPTR_LITERAL("HHHHHHHHHHHHHHHHH");
    lok(jsl_fatptr_starts_with(buffer7, prefix7) == false);
}

void test_jsl_fatptr_ends_with(void)
{
    JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr postfix1 = JSL_FATPTR_LITERAL("Hello, World!");
    lok(jsl_fatptr_ends_with(buffer1, postfix1) == true);

    JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr postfix2 = JSL_FATPTR_LITERAL("World!");
    lok(jsl_fatptr_ends_with(buffer2, postfix2) == true);

    JSLFatPtr buffer3 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr postfix3 = JSL_FATPTR_LITERAL("Hello");
    lok(jsl_fatptr_ends_with(buffer3, postfix3) == false);

    JSLFatPtr buffer4 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr postfix4 = JSL_FATPTR_LITERAL("");
    lok(jsl_fatptr_ends_with(buffer4, postfix4) == true);

    JSLFatPtr buffer5 = JSL_FATPTR_LITERAL("");
    JSLFatPtr postfix5 = JSL_FATPTR_LITERAL("");
    lok(jsl_fatptr_ends_with(buffer5, postfix5) == true);

    JSLFatPtr buffer6 = JSL_FATPTR_LITERAL("");
    JSLFatPtr postfix6 = JSL_FATPTR_LITERAL("HHHHHHHHHHHHHHHHH");
    lok(jsl_fatptr_ends_with(buffer6, postfix6) == false);

    JSLFatPtr buffer7 = JSL_FATPTR_LITERAL("HHHHHHHHHHHHHHHH");
    JSLFatPtr postfix7 = JSL_FATPTR_LITERAL("HHHHHHHHHHHHHHHHH");
    lok(jsl_fatptr_ends_with(buffer7, postfix7) == false);

    JSLFatPtr buffer8 = JSL_FATPTR_LITERAL("Hello, World!");
    JSLFatPtr postfix8 = JSL_FATPTR_LITERAL("!");
    lok(jsl_fatptr_ends_with(buffer8, postfix8) == true);
}

void test_jsl_fatptr_compare_ascii_insensitive(void)
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
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("Hello, World!");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("Hello, World!");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("Hello, World!");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("Hello, World!");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("hello, world!");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("AAAAAAAAAA");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("AaaaAaAaAA");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = {
            .data = NULL,
            .length = 0
        };
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }
    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("This is a string example that WILL span multiple AVX2 chunks so that we can test if the loop is working properly.");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = JSL_FATPTR_LITERAL("This is a string example that WILL span multiple AVX2 chunkz so that we can test if the loop is workING properly.");
        JSLFatPtr buffer2 = JSL_FATPTR_LITERAL("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }
}

int main(void)
{
    lrun("Test jsl_fatptr_from_cstr", test_jsl_fatptr_from_cstr);
    lrun("Test jsl_fatptr_cstr_memory_copy", test_jsl_fatptr_cstr_memory_copy);
    lrun("Test jsl_fatptr_memory_compare", test_jsl_fatptr_memory_compare);
    lrun("Test jsl_fatptr_slice", test_jsl_fatptr_slice);
    lrun("Test jsl_fatptr_index_of", test_jsl_fatptr_index_of);
    lrun("Test jsl_fatptr_index_of_reverse", test_jsl_fatptr_index_of_reverse);
    lrun("Test jsl_fatptr_to_lowercase_ascii", test_jsl_fatptr_to_lowercase_ascii);
    lrun("Test jsl_fatptr_to_int32", test_jsl_fatptr_to_int32);
    lrun("Test jsl_fatptr_substring_search", test_jsl_fatptr_substring_search);
    lrun("Test jsl_fatptr_starts_with", test_jsl_fatptr_starts_with);
    lrun("Test jsl_fatptr_ends_with", test_jsl_fatptr_ends_with);
    lrun("Test jsl_fatptr_compare_ascii_insensitive", test_jsl_fatptr_compare_ascii_insensitive);

    lrun("Test jsl_fatptr_load_file_contents", test_jsl_load_file_contents);
    lrun("Test jsl_fatptr_load_file_contents_buffer", test_jsl_load_file_contents_buffer);

    lresults();
    return lfails != 0;
}

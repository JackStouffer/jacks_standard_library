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
#include "../jacks_standard_library.h"

#include "minctest.h"

void test_jsl_fatptr_from_cstr()
{
    char* c_str = "This is a test string!";
    size_t length = strlen(c_str);

    JSLFatPtr str = jsl_fatptr_from_cstr(c_str);

    lok((void*) str.data == (void*) c_str);
    lok((int64_t) str.length == (int64_t) length);
    lok(memcmp(c_str, str.data, str.length) == 0);
}

void test_jsl_fatptr_cstr_memory_copy()
{
    JSLFatPtr buffer = jsl_fatptr_ctor(malloc(1024), 1024);
    lok((int64_t) buffer.length == (int64_t) 1024);

    JSLFatPtr writer = buffer;
    char* str = "This is a test string!";
    size_t length = strlen(str);
    jsl_fatptr_cstr_memory_copy(&writer, str, false);

    lok(writer.data == buffer.data + length);
    lok(writer.length == 1024 - length);
    lok((int64_t) buffer.length == (int64_t) 1024);

    lok(memcmp(str, buffer.data, length) == 0);
}

void test_jsl_fatptr_load_file_contents()
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

    JSLArena arena = jsl_arena_ctor(malloc(4*1024), 4*1024);

    JSLFatPtr contents;
    JSLLoadFileResultEnum res = jsl_fatptr_load_file_contents(
        &arena,
        JSL_FATPTR_LITERAL("./tests/example.txt"),
        &contents,
        NULL
    );

    lok(res == JSL_FILE_LOAD_SUCCESS);
    lok(memcmp(stack_buffer, contents.data, file_size) == 0);
}

void test_jsl_fatptr_memory_compare()
{
    JSLFatPtr buffer1 = jsl_fatptr_ctor(malloc(13), 13);
    JSLFatPtr buffer2 = jsl_fatptr_ctor(malloc(13), 13);
    JSLFatPtr buffer3 = jsl_fatptr_ctor(malloc(13), 13);
    JSLFatPtr buffer4 = jsl_fatptr_ctor(malloc(20), 20);

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

void test_jsl_fatptr_slice()
{
    JSLFatPtr buffer1 = jsl_fatptr_ctor(malloc(13), 13);

    {
        JSLFatPtr writer1 = buffer1;
        jsl_fatptr_cstr_memory_copy(&writer1, "Hello, World!", false);

        JSLFatPtr slice1 = jsl_fatptr_slice(buffer1, 0, buffer1.length);
        lok(jsl_fatptr_memory_compare(buffer1, slice1));
    }

    {
        JSLFatPtr buffer2 = jsl_fatptr_ctor(malloc(10), 10);
        JSLFatPtr writer2 = buffer2;
        jsl_fatptr_cstr_memory_copy(&writer2, "Hello, Wor", false);

        JSLFatPtr slice2 = jsl_fatptr_slice(buffer1, 0, 10);
        lok(jsl_fatptr_memory_compare(buffer2, slice2));
    }

    {
        JSLFatPtr buffer3 = jsl_fatptr_ctor(malloc(5), 5);
        JSLFatPtr writer3 = buffer3;
        jsl_fatptr_cstr_memory_copy(&writer3, "lo, W", false);

        JSLFatPtr slice3 = jsl_fatptr_slice(buffer1, 3, 8);
        lok(jsl_fatptr_memory_compare(buffer3, slice3));
    }
}

void test_jsl_fatptr_substring_search()
{
    {
        JSLFatPtr string = jsl_fatptr_from_cstr("");
        JSLFatPtr substring = jsl_fatptr_from_cstr("");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == -1);
    }
    
    {
        JSLFatPtr string = jsl_fatptr_from_cstr("");
        JSLFatPtr substring = jsl_fatptr_from_cstr("111111");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == -1);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("111111");
        JSLFatPtr substring = jsl_fatptr_from_cstr("");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == -1);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("Hello, World!");
        JSLFatPtr substring = jsl_fatptr_from_cstr("Longer substring than the original string");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == -1);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("111111");
        JSLFatPtr substring = jsl_fatptr_from_cstr("1");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == 0);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("Hello, World!");
        JSLFatPtr substring = jsl_fatptr_from_cstr("W");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == 7);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("Hello, World!");
        JSLFatPtr substring = jsl_fatptr_from_cstr("World");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == 7);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("Hello, World!");
        JSLFatPtr substring = jsl_fatptr_from_cstr("Hello, World!");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == 0);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("Hello, World!");
        JSLFatPtr substring = jsl_fatptr_from_cstr("Blorp");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == -1);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        JSLFatPtr substring = jsl_fatptr_from_cstr("8-bit");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == 117);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        JSLFatPtr substring = jsl_fatptr_from_cstr("8-blit");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == -1);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        JSLFatPtr substring = jsl_fatptr_from_cstr("ASCII");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == 162);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        JSLFatPtr substring = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == 0);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        JSLFatPtr substring = jsl_fatptr_from_cstr("single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == 85);
    }

    {
        JSLFatPtr string = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
        JSLFatPtr substring = jsl_fatptr_from_cstr("Blorf");
        int64_t res = jsl_fatptr_substring_search(string, substring);
        lok(res == -1);
    }
}

void test_jsl_fatptr_index_of()
{
    JSLFatPtr buffer1 = jsl_fatptr_from_cstr("");
    int64_t res1 = jsl_fatptr_index_of(buffer1, '3');
    lok(res1 == -1);

    JSLFatPtr buffer2 = jsl_fatptr_from_cstr(".");
    int64_t res2 = jsl_fatptr_index_of(buffer2, '.');
    lok(res2 == 0);

    JSLFatPtr buffer3 = jsl_fatptr_from_cstr("......");
    int64_t res3 = jsl_fatptr_index_of(buffer3, '.');
    lok(res3 == 0);

    JSLFatPtr buffer4 = jsl_fatptr_from_cstr("Hello.World");
    int64_t res4 = jsl_fatptr_index_of(buffer4, '.');
    lok(res4 == 5);

    JSLFatPtr buffer5 = jsl_fatptr_from_cstr("Hello          . Hello");
    int64_t res5 = jsl_fatptr_index_of(buffer5, '.');
    lok(res5 == 15);

    JSLFatPtr buffer6 = jsl_fatptr_from_cstr("Hello.World.");
    int64_t res6 = jsl_fatptr_index_of(buffer6, '.');
    lok(res6 == 5);

    JSLFatPtr buffer7 = jsl_fatptr_from_cstr("Hello Hello ");
    int64_t res7 = jsl_fatptr_index_of(buffer7, ' ');
    lok(res7 == 5);

    JSLFatPtr buffer8 = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res8 = jsl_fatptr_index_of(buffer8, '8');
    lok(res8 == 117);
}

void test_jsl_fatptr_index_of_reverse()
{
    JSLFatPtr buffer1 = jsl_fatptr_from_cstr("");
    int64_t res1 = jsl_fatptr_index_of_reverse(buffer1, '3');
    lok(res1 == -1);

    JSLFatPtr buffer2 = jsl_fatptr_from_cstr(".");
    int64_t res2 = jsl_fatptr_index_of_reverse(buffer2, '.');
    lok(res2 == 0);

    JSLFatPtr buffer3 = jsl_fatptr_from_cstr("......");
    int64_t res3 = jsl_fatptr_index_of_reverse(buffer3, '.');
    lok(res3 == 5);

    JSLFatPtr buffer4 = jsl_fatptr_from_cstr("Hello.World");
    int64_t res4 = jsl_fatptr_index_of_reverse(buffer4, '.');
    lok(res4 == 5);

    JSLFatPtr buffer5 = jsl_fatptr_from_cstr("Hello          . Hello");
    int64_t res5 = jsl_fatptr_index_of_reverse(buffer5, '.');
    lok(res5 == 15);

    JSLFatPtr buffer6 = jsl_fatptr_from_cstr("Hello.World.");
    int64_t res6 = jsl_fatptr_index_of_reverse(buffer6, '.');
    lok(res6 == 11);

    JSLFatPtr buffer7 = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res7 = jsl_fatptr_index_of_reverse(buffer7, 'M');
    lok(res7 == 54);

    JSLFatPtr buffer8 = jsl_fatptr_from_cstr("This is a very long string that is going to trigger SIMD code, as it's longer than a single AVX2 register when using 8-bit values, which we are since we're using ASCII/UTF-8.");
    int64_t res8 = jsl_fatptr_index_of_reverse(buffer8, 'w');
    lok(res8 == 150);
}

void test_jsl_fatptr_get_file_extension()
{
    JSLFatPtr buffer1 = jsl_fatptr_from_cstr("");
    JSLFatPtr res1 = jsl_fatptr_get_file_extension(buffer1);
    lok(jsl_fatptr_cstr_compare(res1, ""));

    JSLFatPtr buffer2 = jsl_fatptr_from_cstr(".");
    JSLFatPtr res2 = jsl_fatptr_get_file_extension(buffer2);
    lok(jsl_fatptr_cstr_compare(res2, ""));

    JSLFatPtr buffer3 = jsl_fatptr_from_cstr("......");
    JSLFatPtr res3 = jsl_fatptr_get_file_extension(buffer3);
    lok(jsl_fatptr_cstr_compare(res3, ""));

    JSLFatPtr buffer4 = jsl_fatptr_from_cstr("Hello.text");
    JSLFatPtr res4 = jsl_fatptr_get_file_extension(buffer4);
    lok(jsl_fatptr_cstr_compare(res4, "text"));

    JSLFatPtr buffer5 = jsl_fatptr_from_cstr("Hello          .css");
    JSLFatPtr res5 = jsl_fatptr_get_file_extension(buffer5);
    lok(jsl_fatptr_cstr_compare(res5, "css"));

    JSLFatPtr buffer6 = jsl_fatptr_from_cstr("Hello.min.css");
    JSLFatPtr res6 = jsl_fatptr_get_file_extension(buffer6);
    lok(jsl_fatptr_cstr_compare(res6, "css"));
}

void test_jsl_fatptr_to_lowercase_ascii()
{
    JSLArena arena = jsl_arena_ctor(malloc(1024), 1024);

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

void test_jsl_fatptr_to_int32()
{
    int32_t result;

    JSLFatPtr buffer1 = jsl_fatptr_from_cstr("0");
    lok(jsl_fatptr_to_int32(buffer1, &result) == 1);
    lok(result == 0);

    JSLFatPtr buffer2 = jsl_fatptr_from_cstr("-0");
    lok(jsl_fatptr_to_int32(buffer2, &result) == 2);
    lok(result == 0);

    JSLFatPtr buffer3 = jsl_fatptr_from_cstr("11");
    lok(jsl_fatptr_to_int32(buffer3, &result) == 2);
    lok(result == 11);

    JSLFatPtr buffer4 = jsl_fatptr_from_cstr("-1243");
    lok(jsl_fatptr_to_int32(buffer4, &result) == 5);
    lok(result == -1243);

    JSLFatPtr buffer5 = jsl_fatptr_from_cstr("000003");
    lok(jsl_fatptr_to_int32(buffer5, &result) == 6);
    lok(result == 3);

    JSLFatPtr buffer6 = jsl_fatptr_from_cstr("000000");
    lok(jsl_fatptr_to_int32(buffer6, &result) == 6);
    lok(result == 0);

    JSLFatPtr buffer7 = jsl_fatptr_from_cstr("-000000");
    lok(jsl_fatptr_to_int32(buffer7, &result) == 7);
    lok(result == 0);

    JSLFatPtr buffer8 = jsl_fatptr_from_cstr("98468465");
    lok(jsl_fatptr_to_int32(buffer8, &result) == 8);
    lok(result == 98468465);

    JSLFatPtr buffer9 = jsl_fatptr_from_cstr("454 hello, world");
    lok(jsl_fatptr_to_int32(buffer9, &result) == 3);
    lok(result == 454);

    JSLFatPtr buffer10 = jsl_fatptr_from_cstr("+488 hello, world");
    lok(jsl_fatptr_to_int32(buffer10, &result) == 4);
    lok(result == 488);
}

void test_jsl_fatptr_starts_with()
{
    JSLFatPtr buffer1 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr prefix1 = jsl_fatptr_from_cstr("Hello, World!");
    lok(jsl_fatptr_starts_with(buffer1, prefix1) == true);

    JSLFatPtr buffer2 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr prefix2 = jsl_fatptr_from_cstr("Hello");
    lok(jsl_fatptr_starts_with(buffer2, prefix2) == true);

    JSLFatPtr buffer3 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr prefix3 = jsl_fatptr_from_cstr("World");
    lok(jsl_fatptr_starts_with(buffer3, prefix3) == false);

    JSLFatPtr buffer4 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr prefix4 = jsl_fatptr_from_cstr("");
    lok(jsl_fatptr_starts_with(buffer4, prefix4) == true);

    JSLFatPtr buffer5 = jsl_fatptr_from_cstr("");
    JSLFatPtr prefix5 = jsl_fatptr_from_cstr("");
    lok(jsl_fatptr_starts_with(buffer5, prefix5) == true);

    JSLFatPtr buffer6 = jsl_fatptr_from_cstr("");
    JSLFatPtr prefix6 = jsl_fatptr_from_cstr("HHHHHHHHHHHHHHHHH");
    lok(jsl_fatptr_starts_with(buffer6, prefix6) == false);

    JSLFatPtr buffer7 = jsl_fatptr_from_cstr("HHHHHHHHHHHHHHHH");
    JSLFatPtr prefix7 = jsl_fatptr_from_cstr("HHHHHHHHHHHHHHHHH");
    lok(jsl_fatptr_starts_with(buffer7, prefix7) == false);
}

void test_jsl_fatptr_ends_with()
{
    JSLFatPtr buffer1 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr postfix1 = jsl_fatptr_from_cstr("Hello, World!");
    lok(jsl_fatptr_ends_with(buffer1, postfix1) == true);

    JSLFatPtr buffer2 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr postfix2 = jsl_fatptr_from_cstr("World!");
    lok(jsl_fatptr_ends_with(buffer2, postfix2) == true);

    JSLFatPtr buffer3 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr postfix3 = jsl_fatptr_from_cstr("Hello");
    lok(jsl_fatptr_ends_with(buffer3, postfix3) == false);

    JSLFatPtr buffer4 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr postfix4 = jsl_fatptr_from_cstr("");
    lok(jsl_fatptr_ends_with(buffer4, postfix4) == true);

    JSLFatPtr buffer5 = jsl_fatptr_from_cstr("");
    JSLFatPtr postfix5 = jsl_fatptr_from_cstr("");
    lok(jsl_fatptr_ends_with(buffer5, postfix5) == true);

    JSLFatPtr buffer6 = jsl_fatptr_from_cstr("");
    JSLFatPtr postfix6 = jsl_fatptr_from_cstr("HHHHHHHHHHHHHHHHH");
    lok(jsl_fatptr_ends_with(buffer6, postfix6) == false);

    JSLFatPtr buffer7 = jsl_fatptr_from_cstr("HHHHHHHHHHHHHHHH");
    JSLFatPtr postfix7 = jsl_fatptr_from_cstr("HHHHHHHHHHHHHHHHH");
    lok(jsl_fatptr_ends_with(buffer7, postfix7) == false);

    JSLFatPtr buffer8 = jsl_fatptr_from_cstr("Hello, World!");
    JSLFatPtr postfix8 = jsl_fatptr_from_cstr("!");
    lok(jsl_fatptr_ends_with(buffer8, postfix8) == true);
}

void test_jsl_fatptr_compare_ascii_insensitive()
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
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("Hello, World!");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("Hello, World!");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("Hello, World!");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("Hello, World!");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("hello, world!");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("AAAAAAAAAA");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("AaaaAaAaAA");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = {
            .data = NULL,
            .length = 0
        };
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }

    {
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }
    {
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("This is a string example that will span multiple AVX2 chunks so that we can test if the loop is working properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("This is a string example that WILL span multiple AVX2 chunks so that we can test if the loop is working properly.");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == true);
    }

    {
        JSLFatPtr buffer1 = jsl_fatptr_from_cstr("This is a string example that WILL span multiple AVX2 chunkz so that we can test if the loop is workING properly.");
        JSLFatPtr buffer2 = jsl_fatptr_from_cstr("THIS is a string example THAT will span multiple AVX2 chunks so THAT we can test if the loop is workING properly.");
        lok(jsl_fatptr_compare_ascii_insensitive(buffer1, buffer2) == false);
    }
}

int main()
{
    lrun("Test jsl_fatptr_from_cstr", test_jsl_fatptr_from_cstr);
    lrun("Test jsl_fatptr_cstr_memory_copy", test_jsl_fatptr_cstr_memory_copy);
    lrun("Test jsl_fatptr_load_file_contents", test_jsl_fatptr_load_file_contents);
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

    lresults();
    return lfails != 0;
}

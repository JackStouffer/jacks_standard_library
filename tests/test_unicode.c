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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define JSL_CORE_IMPLEMENTATION
#include "../src/jsl_core.h"

#define JSL_UNICODE_IMPLEMENTATION
#include "../src/jsl_unicode.h"

#include "minctest.h"

JSLArena global_arena;

JSLFatPtr medium_str = JSL_FATPTR_INITIALIZER(
    u8"✋🏻 This is a very long 吟味 string that is going to trigger SIMD code, "
    u8"as it's longer than a single AVX2 register when using 8-bit 😀😃 "
    u8"values, which we are since we're using ASCII/UTF-8."
);
JSLFatPtr long_str = JSL_FATPTR_INITIALIZER(
    u8"✋🏻 Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    u8"Nulla purus justo, iaculis sit amet interdum sit amet, "
    u8"tincidunt at erat. Etiam vulputate ornare dictum. Nullam "
    u8"dapibus at orci id dictum. Pellentesque id lobortis nibh, "
    u8"sit amet euismod lorem. Cras non ex vitae eros interdum blandit "
    u8"in non justo. Pellentesque tincidunt orci a ipsum sagittis, at "
    u8"interdum quam elementum. Mauris est elit, fringilla in placerat "
    u8"consectetur, venenatis nec felis. Nam tempus, justo sit amet "
    u8"sodales bibendum, tortor ipsum feugiat lectus, quis porta neque "
    u8"ipsum accumsan velit. Nam a malesuada urna. Quisque elementum, "
    u8"tellus auctor iaculis laoreet, dolor urna facilisis mauris, "
    u8"vitae dignissim nulla nibh ut velit. Class aptent taciti sociosqu "
    u8"ad litora torquent per conubia nostra, per inceptos himenaeos. Ut "
    u8"luctus semper bibendum. Cras sagittis, nulla in venenatis blandit, "
    u8"ante tortor pulvinar est, faucibus sollicitudin neque ante et diam. "
    u8"Morbi vulputate eu tortor nec vestibulum.\n"
    u8"Aliquam vel purus vel ipsum sollicitudin aliquet. Pellentesque "
    u8"habitant morbi tristique senectus et netus et malesuada fames ac "
    u8"turpis egestas. Phasellus ut varius nunc, sit amet placerat "
    u8"libero. Sed eu velit velit. Sed id tortor quis neque rhoncus "
    u8"tempor. Duis finibus at justo sed auctor. Fusce rhoncus nisi "
    u8"non venenatis dignissim. Praesent sapien elit, elementum id quam "
    u8"ut, volutpat imperdiet tellus. Nulla semper lorem id metus "
    u8"tincidunt luctus. Fusce sodales accumsan varius. Donec faucibus "
    u8"risus felis, vitae dapibus orci lobortis ut. Donec tincidunt eu "
    u8"risus et rutrum. 🇺🇸"
);

JSLUTF16String medium_str_u16 = JSL_UTF16_INITIALIZER(
    u"✋🏻 This is a very long 吟味 string that is going to trigger SIMD code, "
    u"as it's longer than a single AVX2 register when using 8-bit 😀😃 "
    u"values, which we are since we're using ASCII/UTF-8."
);
JSLUTF16String long_str_u16 = JSL_UTF16_INITIALIZER(
    u"✋🏻 Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    u"Nulla purus justo, iaculis sit amet interdum sit amet, "
    u"tincidunt at erat. Etiam vulputate ornare dictum. Nullam "
    u"dapibus at orci id dictum. Pellentesque id lobortis nibh, "
    u"sit amet euismod lorem. Cras non ex vitae eros interdum blandit "
    u"in non justo. Pellentesque tincidunt orci a ipsum sagittis, at "
    u"interdum quam elementum. Mauris est elit, fringilla in placerat "
    u"consectetur, venenatis nec felis. Nam tempus, justo sit amet "
    u"sodales bibendum, tortor ipsum feugiat lectus, quis porta neque "
    u"ipsum accumsan velit. Nam a malesuada urna. Quisque elementum, "
    u"tellus auctor iaculis laoreet, dolor urna facilisis mauris, "
    u"vitae dignissim nulla nibh ut velit. Class aptent taciti sociosqu "
    u"ad litora torquent per conubia nostra, per inceptos himenaeos. Ut "
    u"luctus semper bibendum. Cras sagittis, nulla in venenatis blandit, "
    u"ante tortor pulvinar est, faucibus sollicitudin neque ante et diam. "
    u"Morbi vulputate eu tortor nec vestibulum.\n"
    u"Aliquam vel purus vel ipsum sollicitudin aliquet. Pellentesque "
    u"habitant morbi tristique senectus et netus et malesuada fames ac "
    u"turpis egestas. Phasellus ut varius nunc, sit amet placerat "
    u"libero. Sed eu velit velit. Sed id tortor quis neque rhoncus "
    u"tempor. Duis finibus at justo sed auctor. Fusce rhoncus nisi "
    u"non venenatis dignissim. Praesent sapien elit, elementum id quam "
    u"ut, volutpat imperdiet tellus. Nulla semper lorem id metus "
    u"tincidunt luctus. Fusce sodales accumsan varius. Donec faucibus "
    u"risus felis, vitae dapibus orci lobortis ut. Donec tincidunt eu "
    u"risus et rutrum. 🇺🇸"
);

static void test_jsl_convert_utf8_to_utf16le(void)
{
    {
        JSLUTF16String result_str = {0};
        JSLUnicodeConversionResult result_code = jsl_convert_utf8_to_utf16le(&global_arena, medium_str, &result_str);

        TEST_INT32_EQUAL(result_code, JSL_UNICODE_CONVERSION_SUCCESS);
        TEST_BUFFERS_EQUAL(result_str.data, medium_str_u16.data, (size_t) medium_str_u16.length);
    }

}

static void test_jsl_utf16le_length_from_utf8(void)
{
    {
        JSLFatPtr empty = {NULL, 0};
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(empty), (int64_t) -1);
    }

    {
        JSLFatPtr empty = JSL_FATPTR_INITIALIZER("");
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(empty), (int64_t) 0);
    }

    {
        JSLFatPtr ascii = JSL_FATPTR_INITIALIZER("Plain ASCII");
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(ascii), ascii.length);
    }

    {
        JSLFatPtr two_byte = JSL_FATPTR_INITIALIZER(u8"¢£¥");
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(two_byte), (int64_t) 3);
    }

    {
        JSLFatPtr three_byte = JSL_FATPTR_INITIALIZER(u8"你好");
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(three_byte), (int64_t) 2);
    }

    {
        JSLFatPtr four_byte = JSL_FATPTR_INITIALIZER(u8"😀😃");
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(four_byte), (int64_t) 4);
    }

    {
        JSLFatPtr mixed = JSL_FATPTR_INITIALIZER(u8"A¢€你好😀B");
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(mixed), (int64_t) 8);
    }

    {
        uint8_t data[] = { 'A', '\0', 0xC2, 0xA2 };
        JSLFatPtr with_nul = jsl_fatptr_init(data, 4);
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(with_nul), (int64_t) 3);
    }

    {
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(medium_str), (int64_t) 186);
    }

    {
        TEST_INT64_EQUAL(jsl_utf16le_length_from_utf8(long_str), (int64_t) 1562);
    }

}

static void test_jsl_utf8_length_from_utf16le(void)
{
    {
        JSLUTF16String empty = { NULL, 0 };
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(empty), (int64_t) -1);
    }

    {
        uint16_t ascii[] = { 'P', 'l', 'a', 'i', 'n', ' ', 'A', 'S', 'C', 'I', 'I' };
        JSLUTF16String ascii_str = { ascii, (int64_t) (sizeof(ascii) / sizeof(ascii[0])) };
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(ascii_str), (int64_t) 11);
    }

    {
        uint16_t two_byte[] = { 0x00A2, 0x00A3, 0x00A5 };
        JSLUTF16String two_byte_str = { two_byte, (int64_t) (sizeof(two_byte) / sizeof(two_byte[0])) };
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(two_byte_str), (int64_t) 6);
    }

    {
        uint16_t three_byte[] = { 0x4F60, 0x597D };
        JSLUTF16String three_byte_str = { three_byte, (int64_t) (sizeof(three_byte) / sizeof(three_byte[0])) };
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(three_byte_str), (int64_t) 6);
    }

    {
        uint16_t surrogate_pairs[] = { 0xD83D, 0xDE00, 0xD83D, 0xDE03 };
        JSLUTF16String emoji = { surrogate_pairs, (int64_t) (sizeof(surrogate_pairs) / sizeof(surrogate_pairs[0])) };
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(emoji), (int64_t) 8);
    }

    {
        uint16_t mixed[] = { 'A', 0x00A2, 0x20AC, 0x4F60, 0x597D, 0xD83D, 0xDE00, 'B' };
        JSLUTF16String mixed_str = { mixed, (int64_t) (sizeof(mixed) / sizeof(mixed[0])) };
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(mixed_str), (int64_t) 17);
    }

    {
        uint16_t with_nul[] = { 'A', 0x0000, 0xD83D, 0xDE00 };
        JSLUTF16String nul_str = { with_nul, (int64_t) (sizeof(with_nul) / sizeof(with_nul[0])) };
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(nul_str), (int64_t) 6);
    }

    {
        uint16_t ascii_block[64];
        for (int i = 0; i < 64; i++)
        {
            ascii_block[i] = (uint16_t) 'A';
        }

        JSLUTF16String long_ascii = { ascii_block, 64 };
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(long_ascii), (int64_t) 64);
    }

    {
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(medium_str_u16), (int64_t) 198);
    }

    {
        TEST_INT64_EQUAL(jsl_utf8_length_from_utf16le(long_str_u16), (int64_t) 1570);
    }

}

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    jsl_arena_init(&global_arena, malloc(JSL_MEGABYTES(2)), JSL_MEGABYTES(2));

    RUN_TEST_FUNCTION("Test jsl_convert_utf8_to_utf16le", test_jsl_convert_utf8_to_utf16le);
    RUN_TEST_FUNCTION("Test jsl_utf16le_length_from_utf8", test_jsl_utf16le_length_from_utf8);
    RUN_TEST_FUNCTION("Test jsl_utf8_length_from_utf16le", test_jsl_utf8_length_from_utf16le);

    TEST_RESULTS();
    return lfails != 0;
}

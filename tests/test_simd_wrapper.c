/**
 * Since this isn't our library, these tests just make sure that the library
 * links correctly with C code and is callable.
 * 
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/jsl_core.h"

#include "jsl_simdutf_wrapper.h"

#include "minctest.h"

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

uint16_t* medium_str_u16 = u""
    u"✋🏻 This is a very long 吟味 string that is going to trigger SIMD code, "
    u"as it's longer than a single AVX2 register when using 8-bit 😀😃 "
    u"values, which we are since we're using ASCII/UTF-8.";
uint16_t* long_str_u16 = u""
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
    u"risus et rutrum. 🇺🇸";

static void test(void)
{
    {
        uint16_t* buffer = (uint16_t*) malloc(
            sizeof(uint16_t)
            * simdutf_utf16_length_from_utf8((char*) medium_str.data, (size_t) medium_str.length)
        );

        size_t writen = simdutf_convert_valid_utf8_to_utf16(
            (char*) medium_str.data,
            (size_t) medium_str.length,
            buffer
        );

        TEST_INT64_EQUAL((int64_t) writen, (int64_t) 186);
        TEST_BUFFERS_EQUAL(medium_str_u16, buffer, writen);
    }

    {
        uint16_t* buffer = (uint16_t*) malloc(
            sizeof(uint16_t)
            * simdutf_utf16_length_from_utf8((char*) long_str.data, (size_t) long_str.length)
        );

        size_t writen = simdutf_convert_valid_utf8_to_utf16(
            (char*) long_str.data,
            (size_t) long_str.length,
            buffer
        );

        TEST_INT64_EQUAL((int64_t) writen, (int64_t) 1562);
        TEST_BUFFERS_EQUAL(long_str_u16, buffer, writen);
    }
}

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    RUN_TEST_FUNCTION("Test that everything links ok", test);

    TEST_RESULTS();
    return lfails != 0;
}

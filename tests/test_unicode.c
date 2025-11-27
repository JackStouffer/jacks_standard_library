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

static void test_jsl_utf16_length_from_utf8(void)
{
    {
        JSLFatPtr empty = JSL_FATPTR_INITIALIZER("");
        TEST_INT64_EQUAL(jsl_utf16_length_from_utf8(empty), (int64_t) 0);
    }

    {
        JSLFatPtr ascii = JSL_FATPTR_INITIALIZER("Plain ASCII");
        TEST_INT64_EQUAL(jsl_utf16_length_from_utf8(ascii), ascii.length);
    }

    {
        JSLFatPtr two_byte = JSL_FATPTR_INITIALIZER("¢£¥");
        TEST_INT64_EQUAL(jsl_utf16_length_from_utf8(two_byte), (int64_t) 3);
    }

    {
        JSLFatPtr three_byte = JSL_FATPTR_INITIALIZER("你好");
        TEST_INT64_EQUAL(jsl_utf16_length_from_utf8(three_byte), (int64_t) 2);
    }

    {
        JSLFatPtr four_byte = JSL_FATPTR_INITIALIZER("😀😃");
        TEST_INT64_EQUAL(jsl_utf16_length_from_utf8(four_byte), (int64_t) 4);
    }

    {
        JSLFatPtr mixed = JSL_FATPTR_INITIALIZER("A¢€你好😀B");
        TEST_INT64_EQUAL(jsl_utf16_length_from_utf8(mixed), (int64_t) 8);
    }

    {
        uint8_t data[] = { 'A', '\0', 0xC2, 0xA2 };
        JSLFatPtr with_nul = jsl_fatptr_init(data, 4);
        TEST_INT64_EQUAL(jsl_utf16_length_from_utf8(with_nul), (int64_t) 3);
    }

}

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    RUN_TEST_FUNCTION("Test jsl_utf16_length_from_utf8", test_jsl_utf16_length_from_utf8);

    TEST_RESULTS();
    return lfails != 0;
}

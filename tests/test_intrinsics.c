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

void test_jsl__count_trailing_zeros_u32(void)
{
    /* Basic powers of two */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(1u), 0u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(2u), 1u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(4u), 2u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(8u), 3u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(16u), 4u);

    /* Highest bit set */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(0x80000000u), 31u);

    /* Mixed values */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(0x00000010u), 4u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(0x00000100u), 8u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(0x00010000u), 16u);

    /* Values with multiple bits set */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(0xFFFFFFFFu), 0u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(3u), 0u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(6u), 1u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(12u), 2u);

    /* Systematic check for all powers of two (excluding 0) */
    for (uint32_t i = 0; i < 32; ++i) {
        uint32_t v = 1u << i;
        TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u32(v), i);
    }
}

void test_jsl__count_trailing_zeros_u64(void)
{
    /* Basic powers of two */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(1ull), 0u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(2ull), 1u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(4ull), 2u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(8ull), 3u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(16ull), 4u);

    /* Around 32-bit boundary */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(0x0000000100000000ull), 32u);

    /* Highest bit set */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(0x8000000000000000ull), 63u);

    /* Mixed values */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(0x0000000000000010ull), 4u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(0x0000000000010000ull), 16u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(0x0000000100000000ull), 32u);

    /* Values with multiple bits set */
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(0xFFFFFFFFFFFFFFFFull), 0u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(3ull), 0u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(6ull), 1u);
    TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(12ull), 2u);

    /* Systematic check for all powers of two (excluding 0) */
    for (uint32_t i = 0; i < 64; ++i) {
        uint64_t v = 1ull << i;
        TEST_UINT32_EQUAL(jsl__count_trailing_zeros_u64(v), i);
    }
}

void test_jsl__count_leading_zeros_u32(void)
{
    /* Defined zero behavior */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(0u), 32u);

    /* Powers of two */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(1u), 31u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(2u), 30u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(4u), 29u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(8u), 28u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(16u), 27u);

    /* Highest bit set */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(0x80000000u), 0u);

    /* Lower half filled */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(0x0000FFFFu), 16u);

    /* Randomish patterns */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(0x00F00000u), 8u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(0x0F000000u), 4u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(0x7FFFFFFFu), 1u);

    /* Systematic check for powers of two */
    for (uint32_t i = 0; i < 32; ++i) {
        uint32_t v = 1u << i;
        uint32_t expected = 31u - i;
        TEST_UINT32_EQUAL(jsl__count_leading_zeros_u32(v), expected);
    }
}

void test_jsl__count_leading_zeros_u64(void)
{
    /* Defined zero behavior */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(0ull), 64u);

    /* Powers of two */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(1ull), 63u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(2ull), 62u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(4ull), 61u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(8ull), 60u);

    /* Around 32-bit boundary */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(0x0000000100000000ull), 31u);

    /* Highest bit set */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(0x8000000000000000ull), 0u);

    /* Lower half filled */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(0x00000000FFFFFFFFull), 32u);

    /* Randomish patterns */
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(0x00F0000000000000ull), 8u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(0x0F00000000000000ull), 4u);
    TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(0x7FFFFFFFFFFFFFFFull), 1u);

    /* Systematic check for powers of two */
    for (uint32_t i = 0; i < 64; ++i) {
        uint64_t v = 1ull << i;
        uint32_t expected = 63u - i;
        TEST_UINT32_EQUAL(jsl__count_leading_zeros_u64(v), expected);
    }
}

void test_jsl__find_first_set_u32(void)
{
    /* Defined zero behavior */
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(0u), 0u);

    /* Single bits */
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(1u), 1u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(2u), 2u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(4u), 3u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(8u), 4u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(16u), 5u);

    /* Highest bit set */
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(0x80000000u), 32u);

    /* Multiple bits – should choose least significant */
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(0xFFFFFFFFu), 1u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(0xFFFFFFFEu), 2u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u32(0x0000F000u), 13u); /* 0x0000F000 = bits 12-15 set => first is 13 */

    /* Consistency with ctz for non-zero values: ffs(x) == ctz(x) + 1 */
    for (uint32_t i = 1; i != 0; i <<= 1) {
        TEST_UINT32_EQUAL(jsl__find_first_set_u32(i),
                          jsl__count_trailing_zeros_u32(i) + 1u);
        if (i == 0x80000000u) break;
    }
}

void test_jsl__find_first_set_u64(void)
{
    /* Defined zero behavior */
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(0ull), 0u);

    /* Single bits */
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(1ull), 1u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(2ull), 2u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(4ull), 3u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(8ull), 4u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(16ull), 5u);

    /* Around 32-bit boundary */
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(0x0000000100000000ull), 33u);

    /* Highest bit set */
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(0x8000000000000000ull), 64u);

    /* Multiple bits – least significant wins */
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(0xFFFFFFFFFFFFFFFFull), 1u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(0xFFFFFFFFFFFFFFFEull), 2u);
    TEST_UINT32_EQUAL(jsl__find_first_set_u64(0x0000F00000000000ull), 45u); /* bit 44..47 => first is 45 */

    /* Consistency with ctz for non-zero values: ffs(x) == ctz(x) + 1 */
    for (uint32_t i = 0; i < 64; ++i) {
        uint64_t v = 1ull << i;
        TEST_UINT32_EQUAL(jsl__find_first_set_u64(v),
                          jsl__count_trailing_zeros_u64(v) + 1u);
    }
}

void test_jsl__population_count_u32(void)
{
    TEST_UINT32_EQUAL(jsl__population_count_u32(0u), 0u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(1u), 1u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(2u), 1u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(3u), 2u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(0xFFFFFFFFu), 32u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(0x80000000u), 1u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(0x7FFFFFFFu), 31u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(0x55555555u), 16u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(0xAAAAAAAAu), 16u);
    TEST_UINT32_EQUAL(jsl__population_count_u32(0xF0F0F0F0u), 16u);

    /* Check that popcount of powers of two is 1 */
    for (uint32_t i = 0; i < 32; ++i) {
        uint32_t v = 1u << i;
        TEST_UINT32_EQUAL(jsl__population_count_u32(v), 1u);
    }

    /* Small brute force sanity on low range */
    for (uint32_t x = 0; x < 256u; ++x) {
        uint32_t expected = 0;
        uint32_t t = x;
        while (t) {
            expected += (t & 1u);
            t >>= 1;
        }
        TEST_UINT32_EQUAL(jsl__population_count_u32(x), expected);
    }
}

void test_jsl__population_count_u64(void)
{
    TEST_UINT32_EQUAL(jsl__population_count_u64(0ull), 0u);
    TEST_UINT32_EQUAL(jsl__population_count_u64(1ull), 1u);
    TEST_UINT32_EQUAL(jsl__population_count_u64(2ull), 1u);
    TEST_UINT32_EQUAL(jsl__population_count_u64(3ull), 2u);
    TEST_UINT32_EQUAL(jsl__population_count_u64(0xFFFFFFFFFFFFFFFFull), 64u);
    TEST_UINT32_EQUAL(jsl__population_count_u64(0x8000000000000000ull), 1u);
    TEST_UINT32_EQUAL(jsl__population_count_u64(0x7FFFFFFFFFFFFFFFull), 63u);
    TEST_UINT32_EQUAL(jsl__population_count_u64(0xAAAAAAAAAAAAAAAAull), 32u);
    TEST_UINT32_EQUAL(jsl__population_count_u64(0x0123456789ABCDEFull), 32u);

    /* Powers of two */
    for (uint32_t i = 0; i < 64; ++i) {
        uint64_t v = 1ull << i;
        TEST_UINT32_EQUAL(jsl__population_count_u64(v), 1u);
    }

    /* Small brute force sanity on low range */
    for (uint64_t x = 0; x < 256ull; ++x) {
        uint32_t expected = 0;
        uint64_t t = x;
        while (t) {
            expected += (uint32_t)(t & 1ull);
            t >>= 1;
        }
        TEST_UINT32_EQUAL(jsl__population_count_u64(x), expected);
    }
}

void test_jsl_next_power_of_two_u32(void)
{
    /* NOTE: implementation is not defined for x == 0 or x > 0x80000000 */

    /* Powers of two stay the same */
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(1u), 1u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(2u), 2u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(4u), 4u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(8u), 8u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(16u), 16u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(0x80000000u), 0x80000000u);

    /* Values between powers of two round up */
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(3u), 4u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(5u), 8u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(6u), 8u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(7u), 8u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(9u), 16u);

    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(17u), 32u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(31u), 32u);
    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(33u), 64u);

    TEST_UINT32_EQUAL(jsl_next_power_of_two_u32(0x7FFFFFFFu), 0x80000000u);
}

void test_jsl_next_power_of_two_u64(void)
{
    /* Powers of two stay the same */
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(1ull), 1ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(2ull), 2ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(4ull), 4ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(8ull), 8ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(16ull), 16ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(0x8000000000000000ull),
                      0x8000000000000000ull);

    /* Values between powers of two round up */
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(3ull), 4ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(5ull), 8ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(6ull), 8ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(7ull), 8ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(9ull), 16ull);

    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(17ull), 32ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(31ull), 32ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(33ull), 64ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(1000ull), 1024ull);
    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(123456789ull), 134217728ull);

    TEST_UINT64_EQUAL(jsl_next_power_of_two_u64(0x7FFFFFFFFFFFFFFFull),
                      0x8000000000000000ull);
}

void test_jsl_previous_power_of_two_u32(void)
{
    /* Powers of two stay themselves */
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(1u), 1u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(2u), 2u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(4u), 4u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(8u), 8u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(16u), 16u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(0x80000000u), 0x80000000u);

    /* Values between powers of two round down */
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(3u), 2u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(5u), 4u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(6u), 4u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(7u), 4u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(9u), 8u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(17u), 16u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(31u), 16u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(33u), 32u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(0xFFFFFFFFu), 0x80000000u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(1000u), 512u);
    TEST_UINT32_EQUAL(jsl_previous_power_of_two_u32(123456789u), 67108864u);
}

void test_jsl_previous_power_of_two_u64(void)
{
    /* Powers of two stay themselves */
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(1ull), 1ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(2ull), 2ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(4ull), 4ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(8ull), 8ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(16ull), 16ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(0x8000000000000000ull),
                      0x8000000000000000ull);

    /* Values between powers of two round down */
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(3ull), 2ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(5ull), 4ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(6ull), 4ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(7ull), 4ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(9ull), 8ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(17ull), 16ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(31ull), 16ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(33ull), 32ull);

    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(1000ull), 512ull);
    TEST_UINT64_EQUAL(jsl_previous_power_of_two_u64(123456789ull), 67108864ull);
}

int main(void)
{
    RUN_TEST_FUNCTION("Test count trailing zeros u32", test_jsl__count_trailing_zeros_u32);
    RUN_TEST_FUNCTION("Test count trailing zeros u64", test_jsl__count_trailing_zeros_u64);
    RUN_TEST_FUNCTION("Test count leading zeros u32", test_jsl__count_leading_zeros_u32);
    RUN_TEST_FUNCTION("Test count leading zeros u64", test_jsl__count_leading_zeros_u64);
    RUN_TEST_FUNCTION("Test find first set u32", test_jsl__find_first_set_u32);
    RUN_TEST_FUNCTION("Test find first set u64", test_jsl__find_first_set_u64);
    RUN_TEST_FUNCTION("Test population count u32", test_jsl__population_count_u32);
    RUN_TEST_FUNCTION("Test population count u64", test_jsl__population_count_u64);
    RUN_TEST_FUNCTION("Test next power of two u32", test_jsl_next_power_of_two_u32);
    RUN_TEST_FUNCTION("Test next power of two u64", test_jsl_next_power_of_two_u64);
    RUN_TEST_FUNCTION("Test previous power of two u32", test_jsl_previous_power_of_two_u32);
    RUN_TEST_FUNCTION("Test previous power of two u64", test_jsl_previous_power_of_two_u64);

    TEST_RESULTS();
    return lfails != 0;
}

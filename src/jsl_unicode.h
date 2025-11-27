/**
 * # Jack's Standard Library Unicode
 *
 * A C port of the simdutf library.
 *
 * See README.md for a detailed intro.
 *
 * See DESIGN.md for background on the design decisions.
 *
 * See DOCUMENTATION.md for a single markdown file containing all of the docstrings
 * from this file. It's more nicely formatted and contains hyperlinks.
 *
 * The convention of this library is that all symbols prefixed with either `jsl__`
 * or `JSL__` (with two underscores) are meant to be private to this library. They
 * are not a stable part of the API.
 *
 * ## External Preprocessor Definitions
 *
 * `JSL_DEBUG` - turns on some debugging features, like overwriting stale memory with
 * `0xfeefee`.
 *
 * ## License
 *
 * Copyright 2021 The simdutf authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef JSL_UNICODE

#define JSL_UNICODE

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JSLUTF16String {
    uint16_t* data;
    int64_t length;
} JSLUTF16String;

int64_t jsl_utf8_length_from_utf16(JSLUTF16String utf16_string);

int64_t jsl_utf16_length_from_utf8(JSLFatPtr utf8_string);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef JSL_UNICODE_IMPLEMENTATION

#if defined(__AVX2__)

    static inline int64_t jsl__horizontal_sum_epi64(__m256i v)
    {
        return (int64_t)_mm256_extract_epi64(v, 0) +
                (int64_t)_mm256_extract_epi64(v, 1) +
                (int64_t)_mm256_extract_epi64(v, 2) +
                (int64_t)_mm256_extract_epi64(v, 3);
    }

    static int64_t jsl__utf16_length_from_utf8_bytemask(JSLJSLFatPtr utf8_string)
    {
        const int64_t N = 32;                 // bytes per AVX2 register
        const int64_t max_iterations = 255 / 2; // avoid 8-bit counter overflow

        __m256i counters = _mm256_setzero_si256(); // aggregated 8-byte sums
        __m256i local = _mm256_setzero_si256();    // per-byte counters

        int64_t iterations = 0;
        int64_t pos = 0;
        int64_t count = 0;

        const __m256i continuation_threshold = _mm256_set1_epi8((char)-65);
        const __m256i utf4_threshold = _mm256_set1_epi8((char)240);
        const __m256i zero = _mm256_setzero_si256();

        for (; pos + N <= size; pos += N) {
            __m256i input =
                _mm256_loadu_si256((const __m256i *)(const void *)(in + pos));

            __m256i continuation = _mm256_cmpgt_epi8(input, continuation_threshold);
            __m256i utf4 =
                _mm256_cmpeq_epi8(_mm256_min_epu8(input, utf4_threshold), utf4_threshold);

            local = _mm256_sub_epi8(local, continuation);
            local = _mm256_sub_epi8(local, utf4);

            iterations += 1;
            if (iterations == max_iterations) {
                __m256i partial = _mm256_sad_epu8(local, zero);
                counters = _mm256_add_epi64(counters, partial);
                local = zero;
                iterations = 0;
            }
        }

        if (iterations > 0) {
            __m256i partial = _mm256_sad_epu8(local, zero);
            count += jsl__horizontal_sum_epi64(partial);
        }

        count += jsl__horizontal_sum_epi64(counters);

        // Inline scalar::utf8::utf16_length_from_utf8 for the tail bytes.
        const int8_t *tail = (const int8_t *)(in + pos);
        const int64_t remaining = size - pos;
        for (int64_t i = 0; i < remaining; i++) {
            if (tail[i] > -65) {
                count++;
            }
            if ((uint8_t) tail[i] >= 240) {
                count++;
            }
        }

        return count;
    }

#else

static int64_t jsl__utf16_length_from_utf8_bytemask(JSLFatPtr utf8_string)
{
    int64_t count = 0;

    for (int64_t i = 0; i < utf8_string.length; i++)
    {
        if (((int8_t) utf8_string.data[i]) > -65)
        {
            count++;
        }

        if ((uint8_t) utf8_string.data[i] >= 240)
        {
            count++;
        }
    }

    return count;
}

#endif

int64_t jsl_utf16_length_from_utf8(JSLFatPtr utf8_string)
{
    return jsl__utf16_length_from_utf8_bytemask(utf8_string);
}

#endif // JSL_UNICODE_IMPLEMENTATION

#endif // JSL_UNICODE

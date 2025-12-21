/**
 * # Jack's Standard Library
 *
 * A collection of utilities which are designed to replace much of the C standard
 * library.
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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"

#if JSL_IS_X86
    #include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
#elif JSL_IS_WEB_ASSEMBLY && defined(__wasm_simd128__)
    #include <wasm_simd128.h>
#endif

#if JSL_IS_MSVC
    #include <intrin.h>
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    static inline uint32_t jsl__neon_movemask(uint8x16_t v)
    {
        const uint8x8_t weights = {1,2,4,8,16,32,64,128};

        uint8x16_t msb = vshrq_n_u8(v, 7);
        uint16x8_t lo16 = vmull_u8(vget_low_u8(msb),  weights);
        uint16x8_t hi16 = vmull_u8(vget_high_u8(msb), weights);

        uint32_t lower = vaddvq_u16(lo16);   // reduce 8×u16 -> scalar
        uint32_t upper = vaddvq_u16(hi16);

        return lower | (upper << 8);
    }
#endif

void jsl__assert(int condition, char* file, int line)
{
    (void) file;
    (void) line;
    assert(condition);
}

JSL__FORCE_INLINE int32_t jsl__count_trailing_zeros_u32(uint32_t x)
{
    #if JSL_IS_MSVC
        unsigned long index;
        _BitScanForward(&index, x);
        return (int32_t) index;
    #else
        int32_t n = 0;
        while ((x & 1u) == 0)
        {
            x >>= 1;
            ++n;
        }
        return n;
    #endif
}

JSL__FORCE_INLINE int32_t jsl__count_trailing_zeros_u64(uint64_t x)
{
    #if JSL_IS_MSVC
        unsigned long index;
        _BitScanForward64(&index, x);
        return (int32_t) index;
    #else
        int32_t n = 0;
        while ((x & 1u) == 0)
        {
            x >>= 1;
            ++n;
        }
        return n;
    #endif
}

JSL__FORCE_INLINE int32_t jsl__count_leading_zeros_u32(uint32_t x)
{
    if (x == 0) return 32;
    int32_t n = 0;
    if ((x & 0xFFFF0000) == 0) { n += 16; x <<= 16; }
    if ((x & 0xFF000000) == 0) { n += 8;  x <<= 8;  }
    if ((x & 0xF0000000) == 0) { n += 4;  x <<= 4;  }
    if ((x & 0xC0000000) == 0) { n += 2;  x <<= 2;  }
    if ((x & 0x80000000) == 0) { n += 1; }
    return n;
}

JSL__FORCE_INLINE int32_t jsl__count_leading_zeros_u64(uint64_t x)
{
    if (x == 0) return 64;
    int32_t n = 0;
    if ((x & 0xFFFFFFFF00000000ULL) == 0) { n += 32; x <<= 32; }
    if ((x & 0xFFFF000000000000ULL) == 0) { n += 16; x <<= 16; }
    if ((x & 0xFF00000000000000ULL) == 0) { n += 8;  x <<= 8;  }
    if ((x & 0xF000000000000000ULL) == 0) { n += 4;  x <<= 4;  }
    if ((x & 0xC000000000000000ULL) == 0) { n += 2;  x <<= 2;  }
    if ((x & 0x8000000000000000ULL) == 0) { n += 1; }
    return n;
}

JSL__FORCE_INLINE int32_t jsl__find_first_set_u32(uint32_t x)
{
    #if JSL_IS_MSVC
        unsigned long index;
        _BitScanForward(&index, x);
        return (int32_t) (index + 1);
    #else
        if (x == 0) return 0;

        int32_t n = 1;
        if ((x & 0xFFFF) == 0) { x >>= 16; n += 16; }
        if ((x & 0xFF) == 0)   { x >>= 8;  n += 8;  }
        if ((x & 0xF) == 0)    { x >>= 4;  n += 4;  }
        if ((x & 0x3) == 0)    { x >>= 2;  n += 2;  }
        if ((x & 0x1) == 0)    { n += 1; }

        return n;
    #endif
}

JSL__FORCE_INLINE int32_t jsl__find_first_set_u64(uint64_t x)
{
    #if JSL_IS_MSVC
        unsigned long index;
        _BitScanForward64(&index, x);
        return (uint32_t) (index + 1);
    #else
        if (x == 0) return 0;

        int32_t n = 1;
        if ((x & 0xFFFFFFFFULL) == 0) { x >>= 32; n += 32; }
        if ((x & 0xFFFFULL) == 0)     { x >>= 16; n += 16; }
        if ((x & 0xFFULL) == 0)       { x >>= 8;  n += 8;  }
        if ((x & 0xFULL) == 0)        { x >>= 4;  n += 4;  }
        if ((x & 0x3ULL) == 0)        { x >>= 2;  n += 2;  }
        if ((x & 0x1ULL) == 0)        { n += 1; }

        return n;
    #endif
}

JSL__FORCE_INLINE int32_t jsl__population_count_u32(uint32_t x)
{
    // Branchless SWAR
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0Fu;
    x = x + (x >> 8);
    x = x + (x >> 16);
    return x & 0x3Fu;
}

JSL__FORCE_INLINE int32_t jsl__population_count_u64(uint64_t x)
{
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = x + (x >> 8);
    x = x + (x >> 16);
    x = x + (x >> 32);
    return x & 0x7F;  // result fits in 7 bits (0–64)
}

uint32_t jsl_next_power_of_two_u32(uint32_t x)
{
    #if JSL_IS_CLANG || JSL_IS_GCC

        return 1u << (32u - (uint32_t) JSL_PLATFORM_COUNT_LEADING_ZEROS(x - 1u));

    #else

        x--;
        x |= x >> 1u;
        x |= x >> 2u;
        x |= x >> 4u;
        x |= x >> 8u;
        x |= x >> 16u;
        x++;
        return x;

    #endif
}

int64_t jsl_next_power_of_two_i64(int64_t x)
{
    #if JSL_IS_CLANG || JSL_IS_GCC

        return ((int64_t) 1) << (((int64_t) 64) - (int64_t) JSL_PLATFORM_COUNT_LEADING_ZEROS64((uint64_t) x - 1));

    #else

        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x |= x >> 32;
        x++;
        return x;

    #endif
}

uint64_t jsl_next_power_of_two_u64(uint64_t x)
{
    #if JSL_IS_CLANG || JSL_IS_GCC

        return ((uint64_t) 1u) << (((uint64_t) 64u) - (uint64_t) JSL_PLATFORM_COUNT_LEADING_ZEROS64(x - 1u));

    #else

        x--;
        x |= x >> 1u;
        x |= x >> 2u;
        x |= x >> 4u;
        x |= x >> 8u;
        x |= x >> 16u;
        x |= x >> 32u;
        x++;
        return x;

    #endif
}

uint32_t jsl_previous_power_of_two_u32(uint32_t x)
{
    #if JSL_IS_CLANG || JSL_IS_GCC

        return 1u << (31u - (uint32_t) JSL_PLATFORM_COUNT_LEADING_ZEROS(x));

    #else

        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x - (x >> 1);

    #endif
}

uint64_t jsl_previous_power_of_two_u64(uint64_t x)
{
    #if JSL_IS_CLANG || JSL_IS_GCC

        return ((uint64_t) 1u) << (((uint64_t) 63u) - (uint64_t) JSL_PLATFORM_COUNT_LEADING_ZEROS64(x));

    #else

        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x |= x >> 32;
        return x - (x >> 1);

    #endif
}

JSLFatPtr jsl_fatptr_init(uint8_t* ptr, int64_t length)
{
    JSLFatPtr buffer = {
        .data = ptr,
        .length = length
    };
    return buffer;
}

JSLFatPtr jsl_fatptr_slice(JSLFatPtr fatptr, int64_t start, int64_t end)
{
    JSL_ASSERT(
        fatptr.data != NULL
        && start > -1
        && start <= end
        && end <= fatptr.length
    );

    #ifdef NDEBUG
        if (fatptr.data == NULL
            || start < 0
            || start > end
            || end > fatptr.length)
        {
            fatptr.data = NULL;
            fatptr.length = 0;
            return fatptr;
        }
    #endif

    fatptr.data += start;
    fatptr.length = end - start;
    return fatptr;
}

JSLFatPtr jsl_fatptr_slice_to_end(JSLFatPtr fatptr, int64_t start)
{
    JSL_ASSERT(
        fatptr.data != NULL
        && start > -1
        && start <= fatptr.length
    );

    #ifdef NDEBUG
        if (fatptr.data == NULL
            || start < 0
            || start > fatptr.length)
        {
            fatptr.data = NULL;
            fatptr.length = 0;
            return fatptr;
        }
    #endif

    fatptr.data += start;
    fatptr.length -= start;
    return fatptr;
}

int64_t jsl_fatptr_total_write_length(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr)
{
    uintptr_t orig = (uintptr_t) original_fatptr.data;
    uintptr_t writer = (uintptr_t) writer_fatptr.data;

    JSL_ASSERT(
        original_fatptr.data != NULL
        && writer_fatptr.data != NULL
        && original_fatptr.length > -1
        && writer_fatptr.length > -1
        && (uint64_t) original_fatptr.length <= UINTPTR_MAX - orig // avoid wrap
        && writer >= orig
        && writer - orig <= (uintptr_t) original_fatptr.length
    );

    #ifdef NDEBUG
        if (
            original_fatptr.data == NULL
            || writer_fatptr.data == NULL
            || original_fatptr.length < 0
            || writer_fatptr.length < 0
            || (uint64_t) original_fatptr.length > UINTPTR_MAX - orig
            || writer < orig
            || writer - orig > (uintptr_t) original_fatptr.length
        )
        {
            return -1;
        }
    #endif

    int64_t length_written = writer_fatptr.data - original_fatptr.data;
    return length_written;
}

JSLFatPtr jsl_fatptr_auto_slice(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr)
{
    uintptr_t orig = (uintptr_t) original_fatptr.data;
    uintptr_t writer = (uintptr_t) writer_fatptr.data;

    JSL_ASSERT(
        original_fatptr.data != NULL
        && writer_fatptr.data != NULL
        && original_fatptr.length > -1
        && writer_fatptr.length > -1
        && (uint64_t) original_fatptr.length <= UINTPTR_MAX - orig // avoid wrap
        && writer >= orig
        && writer - orig <= (uintptr_t) original_fatptr.length
    );

    #ifdef NDEBUG
        if (
            original_fatptr.data == NULL
            || writer_fatptr.data == NULL
            || original_fatptr.length < 0
            || writer_fatptr.length < 0
            || (uint64_t) original_fatptr.length > UINTPTR_MAX - orig
            || writer < orig
            || writer - orig > (uintptr_t) original_fatptr.length
        )
        {
            return -1;
        }
    #endif

    int64_t write_length = (int64_t)(writer - orig);
    original_fatptr.length = write_length;
    return original_fatptr;
}

JSLFatPtr jsl_fatptr_from_cstr(const char* str)
{
    JSLFatPtr ret = {
        .data = (uint8_t*) str,
        .length = str == NULL ? 0 : (int64_t) JSL_STRLEN(str)
    };
    return ret;
}

JSL_WARN_UNUSED int64_t jsl_fatptr_memory_copy(JSLFatPtr* destination, JSLFatPtr source)
{
    if (
        source.length < 0
        || source.data == NULL
        || destination->length < 0
        || destination->data == NULL
    )
        return -1;

    // Check for overflows, e.g. if source.data + source.length is
    // greater than UINTPTR_MAX
    const bool source_would_overflow = (uint64_t) source.length > UINTPTR_MAX - (uintptr_t) source.data;
    const bool destination_would_overflow = (uint64_t) destination->length > UINTPTR_MAX - (uintptr_t) destination->data;
    if (source_would_overflow || destination_would_overflow)
        return -1;

    // Check for overlapping buffers
    const uintptr_t source_start = (uintptr_t) source.data;
    const uintptr_t source_end = source_start + (uintptr_t) source.length;
    const uintptr_t dest_start = (uintptr_t) destination->data;
    const uintptr_t dest_end = dest_start + (uintptr_t) destination->length;
    if (source_start < dest_end && source_end > dest_start)
        return -1;

    const int64_t memcpy_length = JSL_MIN(source.length, destination->length);
    JSL_MEMCPY(destination->data, source.data, (size_t) memcpy_length);

    destination->data += memcpy_length;
    destination->length -= memcpy_length;

    return memcpy_length;
}

JSL_WARN_UNUSED int64_t jsl_fatptr_cstr_memory_copy(JSLFatPtr* destination, const char* cstring, bool include_null_terminator)
{
    if (
        cstring == NULL
        || destination->length < 0
        || destination->data == NULL
    )
        return -1;

    int64_t length = JSL_MIN(
        include_null_terminator ? (int64_t) JSL_STRLEN(cstring) + 1 : (int64_t) JSL_STRLEN(cstring),
        destination->length
    );
    JSL_MEMCPY(destination->data, cstring, (size_t) length);

    destination->data += length;
    destination->length -= length;

    return length;
}

bool jsl_fatptr_memory_compare(JSLFatPtr a, JSLFatPtr b)
{
    if (a.length != b.length || a.data == NULL || b.data == NULL)
        return false;

    if (a.data == b.data)
        return true;

    return JSL_MEMCMP(a.data, b.data, (size_t) a.length) == 0;
}

bool jsl_fatptr_cstr_compare(JSLFatPtr string, char* cstr)
{
    if (cstr == NULL || string.data == NULL)
        return false;

    int64_t cstr_length = (int64_t) JSL_STRLEN(cstr);

    if (string.length != cstr_length)
        return false;

    if ((void*) string.data == (void*) cstr)
        return true;

    return JSL_MEMCMP(string.data, cstr, (size_t) cstr_length) == 0;
}

#ifdef __AVX2__

    static JSL__FORCE_INLINE int64_t jsl__avx2_substring_search(JSLFatPtr string, JSLFatPtr substring)
    {
        /**
         * From http://0x80.pl/notesen/2016-11-28-simd-strfind.html
         *
         * Copyright (c) 2008-2016, Wojciech Muła
         * All rights reserved.
         *
         * Redistribution and use in source and binary forms, with or without
         * modification, are permitted provided that the following conditions are
         * met:
         *
         * 1. Redistributions of source code must retain the above copyright
         * notice, this list of conditions and the following disclaimer.
         *
         * 2. Redistributions in binary form must reproduce the above copyright
         * notice, this list of conditions and the following disclaimer in the
         * documentation and/or other materials provided with the distribution.
         *
         * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
         * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
         * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
         * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
         * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
         * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
         * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
         * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
         * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
         * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
         * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
        */

        int64_t i = 0;
        int64_t substring_end_index = substring.length - 1;

        __m256i first = _mm256_set1_epi8((char) substring.data[0]);
        __m256i last  = _mm256_set1_epi8((char) substring.data[substring_end_index]);

        int64_t stopping_point = string.length - substring_end_index - 64;
        for (; i <= stopping_point; i += 64)
        {
            __m256i block_first1 = _mm256_loadu_si256((__m256i*) (string.data + i));
            __m256i block_last1  = _mm256_loadu_si256((__m256i*) (string.data + i + substring_end_index));

            __m256i block_first2 = _mm256_loadu_si256((__m256i*) (string.data + i + 32));
            __m256i block_last2  = _mm256_loadu_si256((__m256i*) (string.data + i + substring_end_index + 32));

            __m256i eq_first1 = _mm256_cmpeq_epi8(first, block_first1);
            __m256i eq_last1  = _mm256_cmpeq_epi8(last, block_last1);

            __m256i eq_first2 = _mm256_cmpeq_epi8(first, block_first2);
            __m256i eq_last2  = _mm256_cmpeq_epi8(last, block_last2);

            uint32_t mask1 = (uint32_t) _mm256_movemask_epi8(_mm256_and_si256(eq_first1, eq_last1));
            uint32_t mask2 = (uint32_t) _mm256_movemask_epi8(_mm256_and_si256(eq_first2, eq_last2));
            uint64_t mask = mask1 | ((uint64_t) mask2 << 32);

            while (mask != 0)
            {
                int32_t bit_position = JSL_PLATFORM_COUNT_TRAILING_ZEROS64(mask);

                int32_t memcmp_res = JSL_MEMCMP(
                    string.data + i + bit_position + 1,
                    substring.data + 1,
                    (size_t) (substring.length - 2)
                );
                if (memcmp_res == 0)
                {
                    return i + bit_position;
                }

                // clear the least significant bit set
                mask &= (mask - 1);
            }
        }

        stopping_point = string.length - substring.length;
        for (; i <= stopping_point; ++i)
        {
            if (string.data[i] == substring.data[0] &&
                string.data[i + substring_end_index] == substring.data[substring_end_index])
            {
                if (substring.length <= 2)
                    return i;

                int32_t memcmp_res = JSL_MEMCMP(
                    string.data + i + 1,
                    substring.data + 1,
                    (size_t) (substring.length - 2)
                );
                if (memcmp_res == 0)
                    return i;
            }
        }

        return -1;
    }

#elif defined(__ARM_NEON) || defined(__ARM_NEON__)

    // TODO: incomplete

#endif

static JSL__FORCE_INLINE int64_t jsl__two_char_search(JSLFatPtr string, JSLFatPtr substring)
{
    uint8_t b0 = substring.data[0];
    uint8_t b1 = substring.data[1];

    for (int64_t i = 0; i + 1 < string.length; ++i)
    {
        if (string.data[i] == b0 && string.data[i + 1] == b1)
        {
            return i;
        }
    }

    return -1;
}

static JSL__FORCE_INLINE int64_t jsl__bndm_search(JSLFatPtr string, JSLFatPtr substring)
{
    uint64_t masks[256] = {0};

    // Map rightmost pattern byte to bit 0 (LSB), leftmost to bit m-1
    for (int64_t i = 0; i < substring.length; ++i)
    {
        uint32_t bit = (uint32_t)(substring.length - 1 - i);
        masks[(uint8_t) substring.data[i]] |= (1ULL << bit);
    }

    // Bitmask of the lowest m bits set to 1; careful with m==64
    uint64_t full = (substring.length == 64) ? ~0ULL : ((1ULL << substring.length) - 1ULL);
    uint64_t MSB  = (substring.length == 64) ? (1ULL << 63) : (1ULL << (substring.length - 1));

    int64_t pos = 0;
    int64_t last_start = string.length - substring.length;

    while (pos <= last_start)
    {
        uint64_t D = full;
        // how many chars left to verify in this window
        int64_t j = substring.length;
        // shift distance if mismatch
        int64_t last = substring.length;

        // Backward scan the window using masks
        while (D != 0)
        {
            uint8_t ch = string.data[pos + j - 1];
            D &= masks[ch];
            if (D != 0)
            {
                if (j == 1)
                {
                    return pos;
                }

                --j;
                // If MSB set, a prefix of the pattern is aligned -> shorter shift
                if (D & MSB) last = j;
            }
            /* Advance the simulated NFA: shift left one, keep to m bits */
            D <<= 1;
            if (substring.length < 64)
                D &= full; /* for m==64, &full is a no-op */
        }

        pos += last;
    }

    return -1;
}

/* Sunday / Quick Search for m > 64 */
static JSL__FORCE_INLINE int64_t jsl__sunday_search(JSLFatPtr string, JSLFatPtr substring)
{
    // Shift table with default m+1 for all 256 bytes
    int64_t shift[256];
    for (int64_t i = 0; i < 256; ++i)
    {
        shift[i] = substring.length + 1;
    }

    // Rightmost occurrence determines shift
    for (int64_t i = 0; i < substring.length; ++i)
    {
        shift[substring.data[i]] = substring.length - i;
    }

    int64_t pos = 0;
    while (pos + substring.length <= string.length)
    {
        if (JSL_MEMCMP(string.data + pos, substring.data, (size_t) substring.length) == 0)
        {
            return pos;
        }

        int64_t next = pos + substring.length;
        if (next < string.length)
        {
            pos += shift[string.data[next]];
        }
        else
        {
            break;
        }
    }

    return -1;
}

static JSL__FORCE_INLINE int64_t jsl__substring_search(JSLFatPtr string, JSLFatPtr substring)
{
    if (substring.length == 2)
    {
        return jsl__two_char_search(string, substring);
    }
    else if (substring.length <= 64)
    {
        return jsl__bndm_search(string, substring);
    }
    else
    {
        return jsl__sunday_search(string, substring);
    }
}


int64_t jsl_fatptr_substring_search(JSLFatPtr string, JSLFatPtr substring)
{
    if (JSL__UNLIKELY(
        string.data == NULL
        || string.length < 1
        || substring.data == NULL
        || substring.length < 1
        || substring.length > string.length
    ))
    {
        return -1;
    }
    else if (substring.length == 1)
    {
        return jsl_fatptr_index_of(string, substring.data[0]);
    }
    else if (string.length == substring.length)
    {
        if (JSL_MEMCMP(string.data, substring.data, (size_t) string.length) == 0)
            return 0;
        else
            return -1;
    }
    else
    {
        #ifdef __AVX2__
            if (string.length >= 64)
                return jsl__avx2_substring_search(string, substring);
            else
                return jsl__substring_search(string, substring);
        #else
            return jsl__substring_search(string, substring);
        #endif
    }
}

int64_t jsl_fatptr_index_of(JSLFatPtr string, uint8_t item)
{
    if (string.data == NULL || string.length < 1)
    {
        return -1;
    }
    else
    {
        int64_t i = 0;

        #ifdef __AVX2__
            __m256i needle = _mm256_set1_epi8((char) item);

            while (i < string.length - 32)
            {
                __m256i elements = _mm256_loadu_si256((__m256i*) (string.data + i));
                __m256i eq_needle = _mm256_cmpeq_epi8(elements, needle);

                uint32_t mask = (uint32_t) _mm256_movemask_epi8(eq_needle);

                if (mask != 0)
                {
                    int64_t bit_position = (int64_t) JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                    return i + bit_position;
                }

                i += 32;
            }
        #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
            uint8x16_t needle = vdupq_n_u8(item);

            while (i < string.length - 16)
            {
                const uint8x16_t chunk = vld1q_u8(string.data + i);
                const uint8x16_t cmp = vceqq_u8(chunk, needle);
                const uint8_t horizontal_maximum = vmaxvq_u8(cmp);

                if (horizontal_maximum == 0)
                {
                    i += 16;
                }
                else
                {
                    const uint32_t mask = jsl__neon_movemask(cmp);
                    int64_t bit_position = (int64_t) JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                    return i + bit_position;
                }
            }

        #endif

        while (i < string.length)
        {
            if (string.data[i] == item)
                return i;

            ++i;
        }

        return -1;
    }
}

int64_t jsl_fatptr_count(JSLFatPtr str, uint8_t item)
{
    int64_t count = 0;

    #ifdef JSL_IS_X86
        #ifdef __AVX2__
            __m256i item_wide_avx = _mm256_set1_epi8((char) item);

            while (str.length >= 32)
            {
                __m256i chunk = _mm256_loadu_si256((__m256i*) str.data);
                __m256i cmp = _mm256_cmpeq_epi8(chunk, item_wide_avx);
                uint32_t mask = (uint32_t) _mm256_movemask_epi8(cmp);

                count += JSL_PLATFORM_POPULATION_COUNT(mask);

                JSL_FATPTR_ADVANCE(str, 32);
            }
        #endif

        #ifdef __SSE3__
            __m128i item_wide_sse = _mm_set1_epi8((char) item);

            while (str.length >= 16)
            {
                __m128i chunk = _mm_loadu_si128((__m128i*) str.data);
                __m128i cmp = _mm_cmpeq_epi8(chunk, item_wide_sse);
                uint32_t mask = (uint32_t) _mm_movemask_epi8(cmp);

                count += JSL_PLATFORM_POPULATION_COUNT(mask);

                JSL_FATPTR_ADVANCE(str, 16);
            }
        #endif
    #elif JSL_IS_ARM
        #if defined(__ARM_NEON) || defined(__ARM_NEON__)
            uint8x16_t item_wide = vdupq_n_u8(item);

            while (str.length >= 16)
            {
                uint8x16_t chunk = vld1q_u8(str.data);
                uint8x16_t cmp = vceqq_u8(chunk, item_wide);
                uint8x16_t ones = vshrq_n_u8(cmp, 7);
                count += vaddvq_u8(ones);

                JSL_FATPTR_ADVANCE(str, 16);
            }
        #endif
    #endif

    while (str.length > 0)
    {
        if (str.data[0] == item)
            count++;

        JSL_FATPTR_ADVANCE(str, 1);
    }

    return count;
}

int64_t jsl_fatptr_index_of_reverse(JSLFatPtr string, uint8_t item)
{
    if (string.data == NULL || string.length < 1)
    {
        return -1;
    }

    #ifdef __AVX2__
        if (string.length < 32)
        {
            int64_t i = string.length - 1;

            while (i > -1)
            {
                if (string.data[i] == item)
                    return i;

                --i;
            }

            return -1;
        }
        else
        {
            int64_t i = string.length - 32;
            __m256i needle = _mm256_set1_epi8((char) item);

            while (i >= 32)
            {
                __m256i elements = _mm256_loadu_si256((__m256i*) (string.data + i));
                __m256i eq_needle = _mm256_cmpeq_epi8(elements, needle);

                uint32_t mask = (uint32_t) _mm256_movemask_epi8(eq_needle);

                if (mask != 0)
                {
                    int32_t bit_position = JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                    return i + bit_position;
                }

                i -= 32;
            }

            while (i > -1)
            {
                if (string.data[i] == item)
                    return i;

                --i;
            }

            return -1;
        }
    #else
        int64_t i = string.length - 1;

        while (i > -1)
        {
            if (string.data[i] == item)
                return i;

            --i;
        }

        return -1;
    #endif
}

#ifdef __AVX2__
    static inline bool jsl__fatptr_small_prefix_match(const uint8_t* str_ptr, const uint8_t* pre_ptr, int64_t len)
    {
        if (len <= 3)
        {
            if (len == 1)
                return str_ptr[0] == pre_ptr[0];

            if (len == 2)
                return (str_ptr[0] == pre_ptr[0]) && (str_ptr[1] == pre_ptr[1]);

            return (str_ptr[0] == pre_ptr[0]) && (str_ptr[1] == pre_ptr[1]) && (str_ptr[2] == pre_ptr[2]);
        }
        else if (len <= 8)
        {
            uint32_t head_a = 0;
            uint32_t head_b = 0;
            uint32_t tail_a = 0;
            uint32_t tail_b = 0;

            JSL_MEMCPY(&head_a, str_ptr, 4);
            JSL_MEMCPY(&head_b, pre_ptr, 4);
            JSL_MEMCPY(&tail_a, str_ptr + 4, (size_t) (len - 4));
            JSL_MEMCPY(&tail_b, pre_ptr + 4, (size_t) (len - 4));

            return (head_a ^ head_b) == 0 && (tail_a ^ tail_b) == 0;
        }
        else
        {
            uint64_t head_a = 0;
            uint64_t head_b = 0;
            uint64_t tail_a = 0;
            uint64_t tail_b = 0;

            JSL_MEMCPY(&head_a, str_ptr, 8);
            JSL_MEMCPY(&head_b, pre_ptr, 8);
            JSL_MEMCPY(&tail_a, str_ptr + 8, (size_t) (len - 8));
            JSL_MEMCPY(&tail_b, pre_ptr + 8, (size_t) (len - 8));

            return (head_a ^ head_b) == 0 && (tail_a ^ tail_b) == 0;
        }
    }

    static inline bool jsl__fatptr_compare_upto_32(const uint8_t* str_ptr, const uint8_t* pre_ptr, int64_t len)
    {
        if (len <= 16)
        {
            return jsl__fatptr_small_prefix_match(str_ptr, pre_ptr, len);
        }

        __m128i head = _mm_loadu_si128((__m128i*) str_ptr);
        __m128i head_pre = _mm_loadu_si128((__m128i*) pre_ptr);
        __m128i tail = _mm_loadu_si128((__m128i*) (str_ptr + len - 16));
        __m128i tail_pre = _mm_loadu_si128((__m128i*) (pre_ptr + len - 16));

        uint32_t head_mask = (uint32_t) _mm_movemask_epi8(_mm_cmpeq_epi8(head, head_pre));
        uint32_t tail_mask = (uint32_t) _mm_movemask_epi8(_mm_cmpeq_epi8(tail, tail_pre));

        return head_mask == 0xFFFFu && tail_mask == 0xFFFFu;
    }
#endif

bool jsl_fatptr_starts_with(JSLFatPtr str, JSLFatPtr prefix)
{
    if (JSL__UNLIKELY(str.data == NULL || prefix.data == NULL))
        return false;

    if (prefix.length == 0)
        return true;

    if (JSL__UNLIKELY(prefix.length > str.length))
        return false;

    return JSL_MEMCMP(str.data, prefix.data, (size_t) prefix.length) == 0;
}

bool jsl_fatptr_ends_with(JSLFatPtr str, JSLFatPtr postfix)
{
    if (JSL__UNLIKELY(str.data == NULL || postfix.data == NULL))
        return false;

    if (postfix.length == 0)
        return true;

    if (JSL__UNLIKELY(postfix.length > str.length))
        return false;

    return JSL_MEMCMP(
        &str.data[str.length - postfix.length],
        postfix.data,
        (size_t) postfix.length
    ) == 0;
}

JSLFatPtr jsl_fatptr_get_file_extension(JSLFatPtr filename)
{
    JSLFatPtr ret;
    int64_t index_of_dot = jsl_fatptr_index_of_reverse(filename, '.');

    if (index_of_dot > -1)
    {
        ret = jsl_fatptr_slice(filename, index_of_dot + 1, filename.length);
    }
    else
    {
        ret.data = NULL;
        ret.length = 0;
    }

    return ret;
}

JSLFatPtr jsl_fatptr_basename(JSLFatPtr filename)
{
    JSLFatPtr ret;
    int64_t slash_postion = jsl_fatptr_index_of_reverse(filename, '/');

    if (filename.length - slash_postion > 2)
    {
        ret = jsl_fatptr_slice(
            filename,
            slash_postion + 1,
            filename.length
        );
    }
    else
    {
        ret = filename;
    }

    return ret;
}

char* jsl_fatptr_to_cstr(JSLArena* arena, JSLFatPtr str)
{
    if (arena == NULL || str.data == NULL || str.length < 1)
        return NULL;

    int64_t allocation_size = str.length + 1;
    JSLFatPtr allocation = jsl_arena_allocate(arena, allocation_size, false);

    if (allocation.data == NULL || allocation.length < allocation_size)
        return NULL;

    JSL_MEMCPY(allocation.data, str.data, (size_t) str.length);
    allocation.data[str.length] = '\0';
    return (char*) allocation.data;
}

JSLFatPtr jsl_cstr_to_fatptr(JSLArena* arena, char* str)
{
    JSLFatPtr ret = {0};
    if (arena == NULL || str == NULL)
        return ret;

    int64_t length = (int64_t) JSL_STRLEN(str);
    if (length == 0)
        return ret;

    int64_t allocation_size = length * (int64_t) sizeof(uint8_t);
    JSLFatPtr allocation = jsl_arena_allocate(
        arena,
        allocation_size,
        false
    );
    if (allocation.data == NULL || allocation.length < length)
        return ret;

    ret = allocation;
    JSL_MEMCPY(ret.data, str, (size_t) length);
    return ret;
}

JSLFatPtr jsl_fatptr_duplicate(JSLArena* arena, JSLFatPtr str)
{
    JSLFatPtr res = {0};
    if (arena == NULL || str.data == NULL || str.length < 1)
        return res;

    JSLFatPtr allocation = jsl_arena_allocate(arena, str.length, false);

    if (allocation.data == NULL || allocation.length < str.length)
        return res;

    JSL_MEMCPY(allocation.data, str.data, (size_t) str.length);
    res = allocation;
    return res;
}

void jsl_fatptr_to_lowercase_ascii(JSLFatPtr str)
{
    if (str.data == NULL || str.length < 1)
        return;

    #ifdef __AVX2__
        __m256i asciiA = _mm256_set1_epi8('A' - 1);
        __m256i asciiZ = _mm256_set1_epi8('Z' + 1);
        __m256i diff   = _mm256_set1_epi8('a' - 'A');

        // TODO, SIMD: add masked loads when length < 32
        while (str.length >= 32)
        {
            __m256i base_data = _mm256_loadu_si256((__m256i*) str.data);

            /* > 'A': 0xff, < 'A': 0x00 */
            __m256i is_greater_or_equal_A = _mm256_cmpgt_epi8(base_data, asciiA);

            /* <= 'Z': 0xff, > 'Z': 0x00 */
            __m256i is_less_or_equal_Z = _mm256_cmpgt_epi8(asciiZ, base_data);

            /* 'Z' >= x >= 'A': 0xFF, else 0x00 */
            __m256i mask = _mm256_and_si256(is_greater_or_equal_A, is_less_or_equal_Z);

            /* 'Z' >= x >= 'A': 'a' - 'A', else 0x00 */
            __m256i to_add = _mm256_and_si256(mask, diff);

            /* add to change to lowercase */
            __m256i added = _mm256_add_epi8(base_data, to_add);
            _mm256_storeu_si256((__m256i *) str.data, added);

            str.length -= 32;
            str.data += 32;
        }
    #endif

    for (int64_t i = 0; i < str.length; i++)
    {
        if (str.data[i] >= 'A' && str.data[i] <= 'Z')
        {
            str.data[i] += 32;
        }
    }
}

static inline uint8_t ascii_to_lower(uint8_t ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return ch + 32;
    }
    return ch;
}

#if defined(__AVX2__)
    static inline __m256i ascii_to_lower_avx2(__m256i data)
    {
        __m256i upper_A = _mm256_set1_epi8('A' - 1);
        __m256i upper_Z = _mm256_set1_epi8('Z' + 1);
        __m256i case_diff = _mm256_set1_epi8(32);

        // Check if character is between 'A' and 'Z'
        __m256i is_upper = _mm256_and_si256(
            _mm256_cmpgt_epi8(data, upper_A),
            _mm256_cmpgt_epi8(upper_Z, data)
        );

        return _mm256_add_epi8(data, _mm256_and_si256(is_upper, case_diff));
    }
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    static inline uint8x16_t ascii_to_lower_neon(uint8x16_t data)
    {
        const uint8x16_t upper_A = vdupq_n_u8('A' - 1);
        const uint8x16_t upper_Z = vdupq_n_u8('Z' + 1);
        const uint8x16_t case_diff = vdupq_n_u8(32);

        uint8x16_t is_upper = vandq_u8(
            vcgtq_u8(data, upper_A),
            vcgtq_u8(upper_Z, data)
        );

        return vaddq_u8(data, vandq_u8(is_upper, case_diff));
    }
#endif

bool jsl_fatptr_compare_ascii_insensitive(JSLFatPtr a, JSLFatPtr b)
{
    if (JSL__UNLIKELY(a.data == NULL || b.data == NULL || a.length != b.length))
        return false;

    int64_t i = 0;

    #if defined(__AVX2__)
        for (; i <= a.length - 32; i += 32)
        {
            __m256i a_vec = _mm256_loadu_si256((__m256i*)(a.data + i));
            __m256i b_vec = _mm256_loadu_si256((__m256i*)(b.data + i));

            a_vec = ascii_to_lower_avx2(a_vec);
            b_vec = ascii_to_lower_avx2(b_vec);

            __m256i cmp = _mm256_cmpeq_epi8(a_vec, b_vec);
            if (_mm256_movemask_epi8(cmp) != -1)
                return false;
        }
    #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
        for (; i <= a.length - 16; i += 16)
        {
            uint8x16_t a_vec = vld1q_u8(a.data + i);
            uint8x16_t b_vec = vld1q_u8(b.data + i);

            a_vec = ascii_to_lower_neon(a_vec);
            b_vec = ascii_to_lower_neon(b_vec);

            uint8x16_t cmp = vceqq_u8(a_vec, b_vec);
            if (jsl__neon_movemask(cmp) != 0xFFFF)
                return false;
        }
    #endif

    for (; i < a.length; i++)
    {
        if (ascii_to_lower(a.data[i]) != ascii_to_lower(b.data[i]))
            return false;
    }

    return true;
}

int32_t jsl_fatptr_to_int32(JSLFatPtr str, int32_t* result)
{
    if (JSL__UNLIKELY(str.data == NULL || str.length < 1))
        return 0;

    bool negative = false;
    int32_t ret = 0;
    int32_t i = 0;

    if (str.data[0] == '-')
    {
        ++i;
        negative = true;
    }
    else if (str.data[0] == '+')
    {
        ++i;
    }

    while (str.data[i] == '0' && i < str.length)
    {
        ++i;
    }

    for (; i < str.length; i++)
    {
        uint8_t digit = str.data[i] - '0';
        if (digit > 9)
            break;

        ret = (ret * 10) + digit;
    }

    if (negative)
        ret = -ret;

    if (i > 0)
        *result = ret;

    return i;
}

int64_t jsl_fatptr_strip_whitespace_left(JSLFatPtr* str)
{
    int64_t bytes_read = str->data != NULL && str->length > -1 ?
        0 : -1;

    while (
        str->data != NULL && str->length > 0
        && (str->data[0] == ' '
        || str->data[0] == '\r'
        || str->data[0] == '\n'
        || str->data[0] == '\v'
        || str->data[0] == '\f'
        || str->data[0] == '\t')
    )
    {
        ++str->data;
        --str->length;
        ++bytes_read;
    }

    return bytes_read;
}

int64_t jsl_fatptr_strip_whitespace_right(JSLFatPtr* str)
{
    int64_t bytes_read = str->data != NULL && str->length > -1 ?
        0 : -1;

    while (
        str->data != NULL && str->length > 0
        && (
            str->data[str->length - 1] == ' '
            || str->data[str->length - 1] == '\r'
            || str->data[str->length - 1] == '\n'
            || str->data[str->length - 1] == '\v'
            || str->data[str->length - 1] == '\f'
            || str->data[str->length - 1] == '\t'
        )
    )
    {
        --str->length;
        ++bytes_read;
    }

    return bytes_read;
}

int64_t jsl_fatptr_strip_whitespace(JSLFatPtr* str)
{
    int64_t bytes_read = -1;

    if (str->data != NULL && str->length > -1)
    {
        bytes_read = jsl_fatptr_strip_whitespace_left(str);
        bytes_read += jsl_fatptr_strip_whitespace_right(str);
    }

    return bytes_read;
}

void jsl_arena_init(JSLArena* arena, void* memory, int64_t length)
{
    arena->start = memory;
    arena->current = memory;
    arena->end = (uint8_t*) memory + length;

    ASAN_POISON_MEMORY_REGION(memory, length);
}

void jsl_arena_init2(JSLArena* arena, JSLFatPtr memory)
{
    arena->start = memory.data;
    arena->current = memory.data;
    arena->end = memory.data + memory.length;

    ASAN_POISON_MEMORY_REGION(memory.data, memory.length);
}

JSLFatPtr jsl_arena_allocate(JSLArena* arena, int64_t bytes, bool zeroed)
{
    return jsl_arena_allocate_aligned(arena, bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT, zeroed);
}

static inline bool jsl__is_power_of_two(int32_t x)
{
    return (x & (x-1)) == 0;
}

static inline uint8_t* align_ptr_upwards(uint8_t* ptr, int32_t align)
{
    uintptr_t addr   = (uintptr_t)ptr;
    uintptr_t ualign = (uintptr_t)(uint32_t)align;

    uintptr_t mask = ualign - 1;
    addr = (addr + mask) & ~mask;

    return (uint8_t*)addr;
}

JSLFatPtr jsl_arena_allocate_aligned(JSLArena* arena, int64_t bytes, int32_t alignment, bool zeroed)
{
    JSL_ASSERT(
        alignment > 0
        && jsl__is_power_of_two(alignment)
    );

    JSLFatPtr res = {0};

    #ifdef NDEBUG
        if (alignment < 1 || !jsl__is_power_of_two(alignment))
        {
            return res;
        }
    #endif

    if (bytes < 0)
        return res;

    uintptr_t arena_end = (uintptr_t) arena->end;
    uintptr_t aligned_current_addr = (uintptr_t) align_ptr_upwards(arena->current, alignment);

    if (aligned_current_addr > arena_end)
        return res;

    // When ASAN is enabled, we leave poisoned guard
    // zones between allocations to catch buffer overflows
    #if JSL__HAS_ASAN
        uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        uintptr_t guard_size = 0;
    #endif

    if ((uint64_t) bytes > UINTPTR_MAX - aligned_current_addr)
        return res;

    uintptr_t potential_end = aligned_current_addr + (uintptr_t) bytes;
    uintptr_t next_current_addr = potential_end + guard_size;

    if (next_current_addr <= arena_end)
    {
        uint8_t* aligned_current = (uint8_t*) aligned_current_addr;

        res.data = aligned_current;
        res.length = bytes;

        #if JSL__HAS_ASAN
            arena->current = (uint8_t*) next_current_addr;
            ASAN_UNPOISON_MEMORY_REGION(res.data, res.length);
        #else
            arena->current = (uint8_t*) next_current_addr;
        #endif

        if (zeroed)
            JSL_MEMSET((void*) res.data, 0, (size_t) res.length);
    }

    return res;
}

JSLFatPtr jsl_arena_reallocate(JSLArena* arena, JSLFatPtr original_allocation, int64_t new_num_bytes)
{
    return jsl_arena_reallocate_aligned(
        arena, original_allocation, new_num_bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT
    );
}

JSLFatPtr jsl_arena_reallocate_aligned(
    JSLArena* arena,
    JSLFatPtr original_allocation,
    int64_t new_num_bytes,
    int32_t align
)
{
    JSL_ASSERT(align > 0 && jsl__is_power_of_two(align));

    #ifdef NDEBUG
        if (alignment < 1 || !jsl__is_power_of_two(alignment))
        {
            return res;
        }
    #endif

    JSLFatPtr res = {0};

    if (new_num_bytes < 0 || original_allocation.length < 0)
        return res;

    const uintptr_t arena_start = (uintptr_t) arena->start;
    const uintptr_t arena_end = (uintptr_t) arena->end;

    #if JSL__HAS_ASAN
        const uintptr_t guard_size = (uintptr_t) JSL__ASAN_GUARD_SIZE;
    #else
        const uintptr_t guard_size = 0;
    #endif

    // Only resize if this given allocation was the last thing alloc-ed
    uintptr_t original_data_addr = (uintptr_t) original_allocation.data;
    bool same_pointer = false;
    uintptr_t aligned_original_addr = 0;
    uintptr_t original_end_addr = 0;

    if (
        original_data_addr >= arena_start
        && original_data_addr <= arena_end
        && (uint64_t) original_allocation.length <= UINTPTR_MAX - original_data_addr
    )
    {
        original_end_addr = original_data_addr + (uintptr_t) original_allocation.length;
        uintptr_t arena_current_addr = (uintptr_t) arena->current;

        bool matches_no_guard = arena_current_addr == original_end_addr;
        bool matches_guard = false;

        if (guard_size <= UINTPTR_MAX - original_end_addr)
        {
            uintptr_t guarded_end = original_end_addr + guard_size;
            matches_guard = arena_current_addr == guarded_end;
        }

        same_pointer = matches_no_guard || matches_guard;
        aligned_original_addr = (uintptr_t) align_ptr_upwards(
            (uint8_t*) original_data_addr,
            align
        );
    }

    if (same_pointer)
    {
        bool can_hold_bytes = (
            (uint64_t) new_num_bytes <= UINTPTR_MAX - aligned_original_addr
        );

        if (can_hold_bytes)
        {
            uintptr_t potential_end = aligned_original_addr + (uintptr_t) new_num_bytes;
            bool guard_overflow = guard_size > UINTPTR_MAX - potential_end;
            if (!guard_overflow)
            {
                uintptr_t next_current_addr = potential_end + guard_size;
                bool is_space_left = next_current_addr <= arena_end;

                if (is_space_left)
                {
                    res.data = (uint8_t*) original_data_addr;
                    res.length = new_num_bytes;
                    arena->current = (uint8_t*) next_current_addr;

                    ASAN_UNPOISON_MEMORY_REGION(res.data, res.length);

                    if (new_num_bytes < original_allocation.length)
                    {
                        ASAN_POISON_MEMORY_REGION(
                            res.data + new_num_bytes,
                            original_allocation.length - new_num_bytes
                        );
                    }

                    return res;
                }
            }
        }
    }

    res = jsl_arena_allocate_aligned(arena, new_num_bytes, align, false);
    if (res.data != NULL)
    {
        JSL_MEMCPY(
            res.data,
            original_allocation.data,
            (size_t) original_allocation.length
        );

        #ifdef JSL_DEBUG
            JSL_MEMSET(
                (void*) original_allocation.data,
                (int32_t) 0xfeeefeee,
                (size_t) original_allocation.length
            );
        #endif

        ASAN_POISON_MEMORY_REGION(original_allocation.data, original_allocation.length);
    }

    return res;
}

void jsl_arena_reset(JSLArena* arena)
{
    ASAN_UNPOISON_MEMORY_REGION(arena->start, arena->end - arena->start);

    #ifdef JSL_DEBUG
        JSL_MEMSET(
            (void*) arena->start,
            (int32_t) 0xfeeefeee,
            (size_t) (arena->current - arena->start)
        );
    #endif

    arena->current = arena->start;

    ASAN_POISON_MEMORY_REGION(arena->start, arena->end - arena->start);
}

uint8_t* jsl_arena_save_restore_point(JSLArena* arena)
{
    return arena->current;
}

void jsl_arena_load_restore_point(JSLArena* arena, uint8_t* restore_point)
{
    const uintptr_t restore_addr = (uintptr_t) restore_point;
    const uintptr_t start_addr = (uintptr_t) arena->start;
    const uintptr_t end_addr = (uintptr_t) arena->end;
    const uintptr_t current_addr = (uintptr_t) arena->current;

    const bool in_bounds = restore_addr >= start_addr && restore_addr <= end_addr;
    const bool before_current = restore_addr <= current_addr;

    JSL_ASSERT(in_bounds && before_current);
    #ifdef NDEBUG
        if (!in_bounds || !before_current)
            return;
    #endif

    ASAN_UNPOISON_MEMORY_REGION(
        restore_point,
        (size_t) (current_addr - restore_addr)
    );

    #ifdef JSL_DEBUG
        JSL_MEMSET(
            (void*) restore_point,
            (int32_t) 0xfeeefeee,
            (size_t) (current_addr - restore_addr)
        );
    #endif

    ASAN_POISON_MEMORY_REGION(
        restore_point,
        (size_t) (current_addr - restore_addr)
    );

    arena->current = restore_point;
}

static int32_t stbsp__real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits);
static int32_t stbsp__real_to_parts(int64_t *bits, int32_t *expo, double value);
#define STBSP__SPECIAL 0x7000

static char stbsp__period = '.';
static char stbsp__comma = ',';
static struct
{
    short temp; // force next field to be 2-byte aligned
    char pair[201];
} stbsp__digitpair =
{
    0,
    "00010203040506070809101112131415161718192021222324"
    "25262728293031323334353637383940414243444546474849"
    "50515253545556575859606162636465666768697071727374"
    "75767778798081828384858687888990919293949596979899"
};

JSL__ASAN_OFF void jsl_format_set_separators(char pcomma, char pperiod)
{
    stbsp__period = pperiod;
    stbsp__comma = pcomma;
}

#define STBSP__LEFTJUST 1
#define STBSP__LEADINGPLUS 2
#define STBSP__LEADINGSPACE 4
#define STBSP__LEADING_0X 8
#define STBSP__LEADINGZERO 16
#define STBSP__INTMAX 32
#define STBSP__TRIPLET_COMMA 64
#define STBSP__NEGATIVE 128
#define STBSP__METRIC_SUFFIX 256
#define STBSP__HALFWIDTH 512
#define STBSP__METRIC_NOSPACE 1024
#define STBSP__METRIC_1024 2048
#define STBSP__METRIC_JEDEC 4096

static void stbsp__lead_sign(uint32_t formatting_flags, char *sign)
{
    sign[0] = 0;
    if (formatting_flags & STBSP__NEGATIVE) {
        sign[0] = 1;
        sign[1] = '-';
    } else if (formatting_flags & STBSP__LEADINGSPACE) {
        sign[0] = 1;
        sign[1] = ' ';
    } else if (formatting_flags & STBSP__LEADINGPLUS) {
        sign[0] = 1;
        sign[1] = '+';
    }
}

static JSL__ASAN_OFF uint32_t stbsp__strlen_limited(char const *string, uint32_t limit)
{
    char const* source_ptr = string;

    #if defined(__AVX2__)

        for (;;)
        {
            if (!limit || *source_ptr == 0)
                return (uint32_t)(source_ptr - string);

            if ((((uintptr_t) source_ptr) & 31) == 0)
                break;

            ++source_ptr;
            --limit;
        }

        __m256i zero_wide = _mm256_setzero_si256();

        while (limit >= 32)
        {
            __m256i* source_wide = (__m256i*) source_ptr;
            __m256i data = _mm256_load_si256(source_wide);
            __m256i null_terminator_mask = _mm256_cmpeq_epi8(data, zero_wide);
            int mask = _mm256_movemask_epi8(null_terminator_mask);

            if (mask == 0)
            {
                source_ptr += 32;
                limit -= 32;
            }
            else
            {
                int null_pos = JSL_PLATFORM_FIND_FIRST_SET(mask) - 1;
                source_ptr += null_pos;
                limit -= (uint32_t) null_pos;
                break;
            }
        }

    #else

        // get up to 4-byte alignment
        for (;;)
        {
            if (((uintptr_t) source_ptr & 3) == 0)
                break;

            if (!limit || *source_ptr == 0)
                return (uint32_t)(source_ptr - string);

            ++source_ptr;
            --limit;
        }

        // scan over 4 bytes at a time to find terminating 0
        // this will intentionally scan up to 3 bytes past the end of buffers,
        // but because it works 4B aligned, it will never cross page boundaries
        // (hence the STBSP__ASAN markup; the over-read here is intentional
        // and harmless)
        while (limit >= 4)
        {
            uint32_t v = *(uint32_t *)source_ptr;
            // bit hack to find if there's a 0 byte in there
            if ((v - 0x01010101) & (~v) & 0x80808080UL)
                break;

            source_ptr += 4;
            limit -= 4;
        }

    #endif

    // handle the last few characters to find actual size
    while (limit && *source_ptr)
    {
        ++source_ptr;
        --limit;
    }

    return (uint32_t)(source_ptr - string);
}

JSL__ASAN_OFF int64_t jsl_format_callback(
    JSL_FORMAT_CALLBACK* callback,
    void* user,
    uint8_t* buffer,
    JSLFatPtr fmt,
    va_list va
)
{
    static char hex[] = "0123456789abcdefxp";
    static char hexu[] = "0123456789ABCDEFXP";
    static JSLFatPtr err_string = JSL_FATPTR_INITIALIZER("(ERROR)");
    uint8_t* buffer_cursor;
    JSLFatPtr f;
    int32_t tlen = 0;

    buffer_cursor = buffer;
    f = fmt;

    #if defined(__AVX2__)
        const __m256i percent_wide = _mm256_set1_epi8('%');
    #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
        const uint8x16_t percent_wide = vdupq_n_u8('%');
    #endif

    while (f.length > 0)
    {
        int32_t field_width, precision, trailing_zeros;
        uint32_t formatting_flags;

        // macros for the callback buffer stuff
        #define stbsp__chk_cb_bufL(bytes)                                                   \
            {                                                                               \
                int32_t len = (int32_t)(buffer_cursor - buffer);                            \
                if ((len + (bytes)) >= JSL_FORMAT_MIN_BUFFER) {                             \
                    tlen += len;                                                            \
                    if (0 == (buffer_cursor = buffer = callback(buffer, user, len)))        \
                        goto done;                                                          \
                }                                                                           \
            }

        #define stbsp__chk_cb_buf(bytes)                                    \
            {                                                               \
                if (callback) {                                             \
                    stbsp__chk_cb_bufL(bytes);                              \
                }                                                           \
            }

        #define stbsp__flush_cb()                                                           \
            {                                                                               \
                stbsp__chk_cb_bufL(JSL_FORMAT_MIN_BUFFER - 1);                              \
            } // flush if there is even one byte in the buffer

        #define stbsp__cb_buf_clamp(cl, v)                                                  \
            cl = v;                                                                         \
            if (callback) {                                                                 \
                int32_t lg = JSL_FORMAT_MIN_BUFFER - (int32_t)(buffer_cursor - buffer);     \
                if (cl > lg)                                                                \
                cl = lg;                                                                    \
            }

        #if defined(__AVX2__)

            // Get 32 byte aligned address instead of doing unaligned loads
            // which will give big performance wins to long format strings and
            // are not that big of a deal to short ones.
            //
            // The performance win comes from not loading across a page boundary,
            // thus requiring two loads for each load intrinsic
            while (((uintptr_t) f.data) & 31 && f.length > 0)
            {
                schk1:
                if (f.data[0] == '%')
                    goto L_PROCESS_PERCENT;

                stbsp__chk_cb_buf(1);
                *buffer_cursor = f.data[0];
                ++buffer_cursor;
                JSL_FATPTR_ADVANCE(f, 1);
            }

            while (f.length > 31)
            {
                const __m256i* source_wide = (const __m256i*) f.data;
                __m256i* wide_dest = (__m256i*) buffer_cursor;

                __m256i data = _mm256_load_si256(source_wide);
                __m256i percent_mask = _mm256_cmpeq_epi8(data, percent_wide);
                uint32_t mask = (uint32_t) _mm256_movemask_epi8(percent_mask);

                if (mask == 0)
                {
                    // No special characters found, store entire block
                    stbsp__chk_cb_buf(32);
                    _mm256_storeu_si256(wide_dest, data);
                    JSL_FATPTR_ADVANCE(f, 32);
                    buffer_cursor += 32;
                }
                else
                {
                    int special_pos = JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                    stbsp__chk_cb_buf(special_pos);

                    JSL_MEMCPY(buffer_cursor, f.data, (size_t) special_pos);
                    JSL_FATPTR_ADVANCE(f, special_pos);
                    buffer_cursor += special_pos;

                    goto L_PROCESS_PERCENT;
                }
            }

            if (f.length == 0)
                goto L_END_FORMAT;
            else
                goto schk1;

        #elif defined(__ARM_NEON) || defined(__ARM_NEON__)

            // Get 16 byte aligned address instead of doing unaligned loads
            // which will give big performance wins to long format strings and
            // are not that big of a deal to short ones.
            //
            // The performance win comes from not loading across a page boundary,
            // thus requiring two loads for each load intrinsic.
            while (((uintptr_t) f.data) & 15 && f.length > 0)
            {
                schk1:
                if (f.data[0] == '%')
                    goto L_PROCESS_PERCENT;

                stbsp__chk_cb_buf(1);
                *buffer_cursor = f.data[0];
                ++buffer_cursor;
                JSL_FATPTR_ADVANCE(f, 1);
            }

            while (f.length > 31)
            {
                const uint8x16_t data0 = vld1q_u8(f.data);
                const uint8x16_t data1 = vld1q_u8(f.data + 16);

                const uint8x16_t percent_cmp0 = vceqq_u8(data0, percent_wide);
                const uint8x16_t percent_cmp1 = vceqq_u8(data1, percent_wide);
                const uint8_t horizontal_maximum0 = vmaxvq_u8(percent_cmp0);
                const uint8_t horizontal_maximum1 = vmaxvq_u8(percent_cmp1);

                const uint32_t combined_presence = horizontal_maximum0 | horizontal_maximum1;
                if (combined_presence == 0)
                {
                    stbsp__chk_cb_buf(32);
                    vst1q_u8(buffer_cursor, data0);
                    vst1q_u8(buffer_cursor + 16, data1);
                    JSL_FATPTR_ADVANCE(f, 32);
                    buffer_cursor += 32;
                }
                else
                {
                    const uint32_t mask0 = jsl__neon_movemask(percent_cmp0);
                    const uint32_t mask1 = jsl__neon_movemask(percent_cmp1);
                    const uint32_t mask  = mask0 | (mask1 << 16);

                    const int32_t special_pos = (int32_t) JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                    stbsp__chk_cb_buf(special_pos);
                    JSL_MEMCPY(buffer_cursor, f.data, (size_t) special_pos);

                    JSL_FATPTR_ADVANCE(f, special_pos);
                    buffer_cursor += special_pos;

                    goto L_PROCESS_PERCENT;
                }
            }

            if (f.length == 0)
                goto L_END_FORMAT;
            else
                goto schk1;

        #else

            // loop one byte at a time to get up to 4-byte alignment
            while (((uintptr_t) f.data) & 3 && f.length > 0)
            {
                schk1:
                if (f.data[0] == '%')
                    goto L_PROCESS_PERCENT;

                stbsp__chk_cb_buf(1);
                *buffer_cursor = f.data[0];
                ++buffer_cursor;
                JSL_FATPTR_ADVANCE(f, 1);
            }

            // fast copy everything up to the next %
            while (f.length > 3)
            {
                // Check if the next 4 bytes contain %
                // Using the 'hasless' trick:
                // https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
                uint32_t v, c;
                v = *(uint32_t *) f.data;
                c = (~v) & 0x80808080;

                if (((v ^ 0x25252525) - 0x01010101) & c)
                    goto schk1;

                if (callback)
                {
                    if ((JSL_FORMAT_MIN_BUFFER - (int32_t)(buffer_cursor - buffer)) < 4)
                        goto schk1;
                }

                if(((uintptr_t) buffer_cursor) & 3)
                {
                    buffer_cursor[0] = f.data[0];
                    buffer_cursor[1] = f.data[1];
                    buffer_cursor[2] = f.data[2];
                    buffer_cursor[3] = f.data[3];
                }
                else
                {
                    *((uint32_t*) buffer_cursor) = v;
                }

                buffer_cursor += 4;
                JSL_FATPTR_ADVANCE(f, 4);
            }

            if (f.length == 0)
                goto L_END_FORMAT;
            else
                goto schk1;

        #endif

        L_PROCESS_PERCENT:

        JSL_FATPTR_ADVANCE(f, 1);

        // ok, we have a percent, read the modifiers first
        field_width = 0;
        precision = -1;
        formatting_flags = 0;
        trailing_zeros = 0;

        // flags
        for (;;) {
            switch (f.data[0]) {
            // if we have left justify
            case '-':
                formatting_flags |= STBSP__LEFTJUST;
                JSL_FATPTR_ADVANCE(f, 1);
                continue;
            // if we have leading plus
            case '+':
                formatting_flags |= STBSP__LEADINGPLUS;
                JSL_FATPTR_ADVANCE(f, 1);
                continue;
            // if we have leading space
            case ' ':
                formatting_flags |= STBSP__LEADINGSPACE;
                JSL_FATPTR_ADVANCE(f, 1);
                continue;
            // if we have leading 0x
            case '#':
                formatting_flags |= STBSP__LEADING_0X;
                JSL_FATPTR_ADVANCE(f, 1);
                continue;
            // if we have thousand commas
            case '\'':
                formatting_flags |= STBSP__TRIPLET_COMMA;
                JSL_FATPTR_ADVANCE(f, 1);
                continue;
            // if we have kilo marker (none->kilo->kibi->jedec)
            case '$':
                if (formatting_flags & STBSP__METRIC_SUFFIX) {
                if (formatting_flags & STBSP__METRIC_1024) {
                    formatting_flags |= STBSP__METRIC_JEDEC;
                } else {
                    formatting_flags |= STBSP__METRIC_1024;
                }
                } else {
                formatting_flags |= STBSP__METRIC_SUFFIX;
                }
                JSL_FATPTR_ADVANCE(f, 1);
                continue;
            // if we don't want space between metric suffix and number
            case '_':
                formatting_flags |= STBSP__METRIC_NOSPACE;
                JSL_FATPTR_ADVANCE(f, 1);
                continue;
            // if we have leading zero
            case '0':
                formatting_flags |= STBSP__LEADINGZERO;
                JSL_FATPTR_ADVANCE(f, 1);
                goto flags_done;
            default: goto flags_done;
            }
        }
        flags_done:

        // get the field width
        if (f.data[0] == '*') {
            field_width = (int32_t) va_arg(va, uint32_t);
            JSL_FATPTR_ADVANCE(f, 1);
        } else {
            while ((f.data[0] >= '0') && (f.data[0] <= '9')) {
                field_width = field_width * 10 + f.data[0] - '0';
                JSL_FATPTR_ADVANCE(f, 1);
            }
        }
        // get the precision
        if (f.data[0] == '.') {
            JSL_FATPTR_ADVANCE(f, 1);
            if (f.data[0] == '*') {
                precision = (int32_t) va_arg(va, uint32_t);
                JSL_FATPTR_ADVANCE(f, 1);
            } else {
                precision = 0;
                while ((f.data[0] >= '0') && (f.data[0] <= '9')) {
                precision = precision * 10 + f.data[0] - '0';
                JSL_FATPTR_ADVANCE(f, 1);
                }
            }
        }

        // handle integer size overrides
        switch (f.data[0])
        {
            // are we halfwidth?
            case 'h':
                formatting_flags |= STBSP__HALFWIDTH;
                JSL_FATPTR_ADVANCE(f, 1);
                if (f.data[0] == 'h')
                    JSL_FATPTR_ADVANCE(f, 1);  // QUARTERWIDTH
                break;
            // are we 64-bit (unix style)
            case 'l':
                formatting_flags |= ((sizeof(long) == 8) ? STBSP__INTMAX : 0);
                JSL_FATPTR_ADVANCE(f, 1);
                if (f.data[0] == 'l') {
                    formatting_flags |= STBSP__INTMAX;
                    JSL_FATPTR_ADVANCE(f, 1);
                }
                break;
            // are we 64-bit on intmax? (c99)
            case 'j':
                formatting_flags |= (sizeof(size_t) == 8) ? STBSP__INTMAX : 0;
                JSL_FATPTR_ADVANCE(f, 1);
                break;
            // are we 64-bit on size_t or ptrdiff_t? (c99)
            case 'z':
                formatting_flags |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
                JSL_FATPTR_ADVANCE(f, 1);
                break;
            case 't':
                formatting_flags |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
                JSL_FATPTR_ADVANCE(f, 1);
                break;
            // are we 64-bit (msft style)
            case 'I':
                if ((f.data[1] == '6') && (f.data[2] == '4')) {
                    formatting_flags |= STBSP__INTMAX;
                    JSL_FATPTR_ADVANCE(f, 3);
                } else if ((f.data[1] == '3') && (f.data[2] == '2')) {
                    JSL_FATPTR_ADVANCE(f, 3);
                } else {
                    formatting_flags |= ((sizeof(void *) == 8) ? STBSP__INTMAX : 0);
                    JSL_FATPTR_ADVANCE(f, 1);
                }
                break;
            default: break;
        }

        // handle each replacement
        switch (f.data[0])
        {
            #define STBSP__NUMSZ 512 // big enough for e308 (with commas) or e-307
            char num[STBSP__NUMSZ];
            char lead[8];
            char tail[8];
            char* string;
            char const* h;
            uint32_t l, n, comma_spacing;
            uint64_t n64;
            double float_value;
            int32_t decimal_precision;
            char const* source_ptr;

            case 's':
                string = va_arg(va, char *);

                if (JSL__UNLIKELY(string == NULL))
                {
                    string = (char*) err_string.data;
                    l = (uint32_t) err_string.length;
                }
                else
                {
                    // get the length, limited to desired precision
                    // always limit to ~0u chars since our counts are 32b
                    l = (precision >= 0)
                        ? stbsp__strlen_limited(string, (uint32_t) precision)
                        : (uint32_t) JSL_STRLEN(string);
                }

                lead[0] = 0;
                tail[0] = 0;
                precision = 0;
                decimal_precision = 0;
                comma_spacing = 0;
                // copy the string in
                goto L_STRING_COPY;

            case 'y':
            {
                JSLFatPtr fat_string = va_arg(va, JSLFatPtr);

                if (JSL__UNLIKELY(formatting_flags != 0
                    || field_width != 0
                    || precision != -1
                    || fat_string.data == NULL
                    || fat_string.length < 0
                    || fat_string.length > UINT32_MAX))
                {
                    string = (char*) err_string.data;
                    l = (uint32_t) err_string.length;
                }
                else
                {
                    string = (char*) fat_string.data;
                    l = (uint32_t) fat_string.length;
                }

                field_width = 0;
                formatting_flags = 0;
                lead[0] = 0;
                tail[0] = 0;
                precision = 0;
                decimal_precision = 0;
                comma_spacing = 0;
                goto L_STRING_COPY;
            }

            case 'c': // char
                // get the character
                string = num + STBSP__NUMSZ - 1;
                *string = (char)va_arg(va, int32_t);
                l = 1;
                lead[0] = 0;
                tail[0] = 0;
                precision = 0;
                decimal_precision = 0;
                comma_spacing = 0;
                goto L_STRING_COPY;

            case 'n': // weird write-bytes specifier
            {
                int32_t *d = va_arg(va, int32_t *);
                *d = tlen + (int32_t)(buffer_cursor - buffer);
            } break;

            case 'A': // hex float
            case 'a': // hex float
                h = (f.data[0] == 'A') ? hexu : hex;
                float_value = va_arg(va, double);
                if (precision == -1)
                    precision = 6; // default is 6
                // read the double into a string
                if (stbsp__real_to_parts((int64_t *)&n64, &decimal_precision, float_value))
                    formatting_flags |= STBSP__NEGATIVE;

                string = num + 64;

                stbsp__lead_sign(formatting_flags, lead);

                if (decimal_precision == -1023)
                    decimal_precision = (n64) ? -1022 : 0;
                else
                    n64 |= (((uint64_t)1) << 52);
                n64 <<= (64 - 56);
                if (precision < 15)
                    n64 += ((((uint64_t)8) << 56) >> (precision * 4));
                // add leading chars

                lead[1 + lead[0]] = '0';
                lead[2 + lead[0]] = 'x';
                lead[0] += 2;

                *string++ = h[(n64 >> 60) & 15];
                n64 <<= 4;
                if (precision)
                    *string++ = stbsp__period;
                source_ptr = string;

                // print the bits
                n = (uint32_t) precision;
                if (n > 13)
                    n = 13;
                if (precision > (int32_t)n)
                    trailing_zeros = precision - (int32_t)n;
                precision = 0;
                while (n--) {
                    *string++ = h[(n64 >> 60) & 15];
                    n64 <<= 4;
                }

                // print the expo
                tail[1] = h[17];
                if (decimal_precision < 0) {
                    tail[2] = '-';
                    decimal_precision = -decimal_precision;
                } else
                    tail[2] = '+';
                n = (decimal_precision >= 1000) ? 6 : ((decimal_precision >= 100) ? 5 : ((decimal_precision >= 10) ? 4 : 3));
                tail[0] = (char)n;
                for (;;) {
                    tail[n] = '0' + decimal_precision % 10;
                    if (n <= 3)
                    break;
                    --n;
                    decimal_precision /= 10;
                }

                decimal_precision = (int32_t)(string - source_ptr);
                l = (uint32_t)(string - (num + 64));
                string = num + 64;
                comma_spacing = 1u + (3u << 24);
                goto L_STRING_COPY;

            case 'G': // float
            case 'g': // float
                h = (f.data[0] == 'G') ? hexu : hex;
                float_value = va_arg(va, double);
                if (precision == -1)
                    precision = 6;
                else if (precision == 0)
                    precision = 1; // default is 6
                // read the double into a string
                if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, (uint32_t)(((uint32_t)(precision - 1)) | 0x80000000u)))
                    formatting_flags |= STBSP__NEGATIVE;

                // clamp the precision and delete extra zeros after clamp
                n = (uint32_t) precision;
                if (l > (uint32_t)precision)
                    l = (uint32_t) precision;
                while ((l > 1) && (precision) && (source_ptr[l - 1] == '0')) {
                    --precision;
                    --l;
                }

                // should we use %e
                if ((decimal_precision <= -4) || (decimal_precision > (int32_t)n)) {
                    if (precision > (int32_t)l)
                    precision = (int32_t) l - 1;
                    else if (precision)
                    --precision; // when using %e, there is one digit before the decimal
                    goto L_DO_EXP_FROMG;
                }
                // this is the insane action to get the precision to match %g semantics for %f
                if (decimal_precision > 0) {
                    precision = (decimal_precision < (int32_t)l) ? (int32_t)(l - (uint32_t) decimal_precision) : 0;
                } else {
                    precision = -decimal_precision + ((precision > (int32_t)l) ? (int32_t) l : precision);
                }
                goto L_DO_FLOAT_FROMG;

            case 'E': // float
            case 'e': // float
                h = (f.data[0] == 'E') ? hexu : hex;
                float_value = va_arg(va, double);
                if (precision == -1)
                    precision = 6; // default is 6
                // read the double into a string
                if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, ((uint32_t) precision) | 0x80000000u))
                    formatting_flags |= STBSP__NEGATIVE;
            L_DO_EXP_FROMG:
                tail[0] = 0;
                stbsp__lead_sign(formatting_flags, lead);
                if (decimal_precision == STBSP__SPECIAL) {
                    string = (char *)source_ptr;
                    comma_spacing = 0;
                    precision = 0;
                    goto L_STRING_COPY;
                }
                string = num + 64;
                // handle leading chars
                *string++ = source_ptr[0];

                if (precision)
                    *string++ = stbsp__period;

                // handle after decimal
                if ((l - 1u) > (uint32_t)precision)
                    l = (uint32_t) precision + 1u;
                for (n = 1; n < l; n++)
                    *string++ = source_ptr[n];
                // trailing zeros
                trailing_zeros = precision - (int32_t)(l - 1u);
                precision = 0;
                // dump expo
                tail[1] = h[0xe];
                decimal_precision -= 1;
                if (decimal_precision < 0) {
                    tail[2] = '-';
                    decimal_precision = -decimal_precision;
                } else
                    tail[2] = '+';

                n = (decimal_precision >= 100) ? 5 : 4;

                tail[0] = (char)n;
                for (;;) {
                    tail[n] = '0' + decimal_precision % 10;
                    if (n <= 3)
                    break;
                    --n;
                    decimal_precision /= 10;
                }
                comma_spacing = 1u + (3u << 24); // how many tens
                goto flt_lead;

            case 'f': // float
                float_value = va_arg(va, double);
            doafloat:
                // do kilos
                if (formatting_flags & STBSP__METRIC_SUFFIX) {
                    double divisor;
                    divisor = 1000.0f;
                    if (formatting_flags & STBSP__METRIC_1024)
                    divisor = 1024.0;
                    while (formatting_flags < 0x4000000) {
                    if ((float_value < divisor) && (float_value > -divisor))
                        break;
                    float_value /= divisor;
                    formatting_flags += 0x1000000;
                    }
                }
                if (precision == -1)
                    precision = 6; // default is 6
                // read the double into a string
                if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, (uint32_t) precision))
                    formatting_flags |= STBSP__NEGATIVE;
            L_DO_FLOAT_FROMG:
                tail[0] = 0;
                stbsp__lead_sign(formatting_flags, lead);
                if (decimal_precision == STBSP__SPECIAL) {
                    string = (char *)source_ptr;
                    comma_spacing = 0;
                    precision = 0;
                    goto L_STRING_COPY;
                }
                string = num + 64;

                // handle the three decimal varieties
                if (decimal_precision <= 0)
                {
                    // handle 0.000*000xxxx
                    *string++ = '0';
                    if (precision)
                    *string++ = stbsp__period;
                    n = (uint32_t)(-decimal_precision);
                    if ((int32_t)n > precision)
                    n = (uint32_t) precision;

                    JSL_MEMSET(string, '0', n);
                    string += n;

                    if ((int32_t)(l + n) > precision)
                    l = (uint32_t)(precision - (int32_t) n);

                    JSL_MEMCPY(string, source_ptr, l);
                    string += l;
                    source_ptr += l;

                    trailing_zeros = precision - (int32_t)(n + l);
                    comma_spacing = 1u + (3u << 24); // how many tens did we write (for commas below)
                }
                else
                {
                    comma_spacing = (formatting_flags & STBSP__TRIPLET_COMMA) ? ((600 - (uint32_t)decimal_precision) % 3) : 0;
                    if ((uint32_t)decimal_precision >= l) {
                    // handle xxxx000*000.0
                    n = 0;
                    for (;;) {
                        if ((formatting_flags & STBSP__TRIPLET_COMMA) && (++comma_spacing == 4)) {
                            comma_spacing = 0;
                            *string++ = stbsp__comma;
                        } else {
                            *string++ = source_ptr[n];
                            ++n;
                            if (n >= l)
                                break;
                        }
                    }
                    if (n < (uint32_t)decimal_precision) {
                        n = (uint32_t)(decimal_precision - (int32_t) n);
                        if ((formatting_flags & STBSP__TRIPLET_COMMA) == 0) {
                            while (n) {
                                if ((((uintptr_t)string) & 3) == 0)
                                break;
                                *string++ = '0';
                                --n;
                            }
                            while (n >= 4) {
                                *(uint32_t *)string = 0x30303030;
                                string += 4;
                                n -= 4;
                            }
                        }
                        while (n) {
                            if ((formatting_flags & STBSP__TRIPLET_COMMA) && (++comma_spacing == 4)) {
                                comma_spacing = 0;
                                *string++ = stbsp__comma;
                            } else {
                                *string++ = '0';
                                --n;
                            }
                        }
                    }
                    comma_spacing = (uint32_t)(string - (num + 64)); // comma_spacing is how many tens
                    comma_spacing += (3u << 24);
                    if (precision) {
                        *string++ = stbsp__period;
                        trailing_zeros = precision;
                    }
                    } else {
                    // handle xxxxx.xxxx000*000
                    n = 0;
                    for (;;) {
                        if ((formatting_flags & STBSP__TRIPLET_COMMA) && (++comma_spacing == 4)) {
                            comma_spacing = 0;
                            *string++ = stbsp__comma;
                        } else {
                            *string++ = source_ptr[n];
                            ++n;
                            if (n >= (uint32_t)decimal_precision)
                                break;
                        }
                    }
                    comma_spacing = (uint32_t)(string - (num + 64)); // comma_spacing is how many tens
                    comma_spacing += (3u << 24);
                    if (precision)
                        *string++ = stbsp__period;
                    if ((l - (uint32_t) decimal_precision) > (uint32_t)precision)
                        l = (uint32_t)(precision + decimal_precision);
                    while (n < l) {
                        *string++ = source_ptr[n];
                        ++n;
                    }
                    trailing_zeros = precision - (int32_t)(l - (uint32_t) decimal_precision);
                    }
                }
                precision = 0;

                // handle k,m,g,t
                if (formatting_flags & STBSP__METRIC_SUFFIX) {
                    char idx;
                    idx = 1;
                    if (formatting_flags & STBSP__METRIC_NOSPACE)
                    idx = 0;
                    tail[0] = idx;
                    tail[1] = ' ';
                    {
                    if (formatting_flags >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                        if (formatting_flags & STBSP__METRIC_1024)
                            tail[idx + 1] = "_KMGT"[formatting_flags >> 24];
                        else
                            tail[idx + 1] = "_kMGT"[formatting_flags >> 24];
                        idx++;
                        // If printing kibits and not in jedec, add the 'i'.
                        if (formatting_flags & STBSP__METRIC_1024 && !(formatting_flags & STBSP__METRIC_JEDEC)) {
                            tail[idx + 1] = 'i';
                            idx++;
                        }
                        tail[0] = idx;
                    }
                    }
                };

            flt_lead:
                // get the length that we copied
                l = (uint32_t)(string - (num + 64));
                string = num + 64;
                goto L_STRING_COPY;

            case 'B': // upper binary
            case 'b': // lower binary
                h = (f.data[0] == 'B') ? hexu : hex;
                lead[0] = 0;
                if (formatting_flags & STBSP__LEADING_0X) {
                    lead[0] = 2;
                    lead[1] = '0';
                    lead[2] = h[0xb];
                }
                l = (8 << 4) | (1 << 8);
                goto L_RADIX_NUM;

            case 'o': // octal
                h = hexu;
                lead[0] = 0;
                if (formatting_flags & STBSP__LEADING_0X) {
                    lead[0] = 1;
                    lead[1] = '0';
                }
                l = (3 << 4) | (3 << 8);
                goto L_RADIX_NUM;

            case 'p': // pointer
                formatting_flags |= (sizeof(void *) == 8) ? STBSP__INTMAX : 0;
                precision = sizeof(void *) * 2;
                formatting_flags &= (uint32_t) ~STBSP__LEADINGZERO; // 'p' only prints the pointer with zeros
                                            // fall through - to X

                JSL_SWITCH_FALLTHROUGH;

            case 'X': // upper hex
            case 'x': // lower hex
                h = (f.data[0] == 'X') ? hexu : hex;
                l = (4 << 4) | (4 << 8);
                lead[0] = 0;
                if (formatting_flags & STBSP__LEADING_0X) {
                    lead[0] = 2;
                    lead[1] = '0';
                    lead[2] = h[16];
                }

            L_RADIX_NUM:
                // get the number
                if (formatting_flags & STBSP__INTMAX)
                    n64 = va_arg(va, uint64_t);
                else
                    n64 = va_arg(va, uint32_t);

                string = num + STBSP__NUMSZ;
                decimal_precision = 0;
                // clear tail, and clear leading if value is zero
                tail[0] = 0;
                if (n64 == 0) {
                    lead[0] = 0;
                    if (precision == 0) {
                    l = 0;
                    comma_spacing = 0;
                    goto L_STRING_COPY;
                    }
                }
                // convert to string
                for (;;) {
                    *--string = h[n64 & ((1 << (l >> 8)) - 1)];
                    n64 >>= (l >> 8);
                    if (!((n64) || ((int32_t)((num + STBSP__NUMSZ) - string) < precision)))
                    break;
                    if (formatting_flags & STBSP__TRIPLET_COMMA) {
                    ++l;
                    if ((l & 15) == ((l >> 4) & 15)) {
                        l &= ~UINT32_C(15);
                        *--string = stbsp__comma;
                    }
                    }
                };
                // get the tens and the comma pos
                comma_spacing = (uint32_t)((num + STBSP__NUMSZ) - string) + ((((l >> 4) & 15)) << 24);
                // get the length that we copied
                l = (uint32_t)((num + STBSP__NUMSZ) - string);
                // copy it
                goto L_STRING_COPY;

            case 'u': // unsigned
            case 'i':
            case 'd': // integer
                // get the integer and abs it
                if (formatting_flags & STBSP__INTMAX) {
                    int64_t i64 = va_arg(va, int64_t);
                    n64 = (uint64_t)i64;
                    if ((f.data[0] != 'u') && (i64 < 0)) {
                    n64 = (uint64_t)-i64;
                    formatting_flags |= STBSP__NEGATIVE;
                    }
                } else {
                    int32_t i = va_arg(va, int32_t);
                    n64 = (uint32_t)i;
                    if ((f.data[0] != 'u') && (i < 0)) {
                    n64 = (uint32_t)-i;
                    formatting_flags |= STBSP__NEGATIVE;
                    }
                }

                if (formatting_flags & STBSP__METRIC_SUFFIX) {
                    if (n64 < 1024)
                    precision = 0;
                    else if (precision == -1)
                    precision = 1;
                    float_value = (double)(int64_t)n64;
                    goto doafloat;
                }

                // convert to string
                string = num + STBSP__NUMSZ;
                l = 0;

                for (;;) {
                    // do in 32-bit chunks (avoid lots of 64-bit divides even with constant denominators)
                    char *o = string - 8;
                    if (n64 >= 100000000) {
                    n = (uint32_t)(n64 % 100000000);
                    n64 /= 100000000;
                    } else {
                    n = (uint32_t)n64;
                    n64 = 0;
                    }
                    if ((formatting_flags & STBSP__TRIPLET_COMMA) == 0) {
                    do {
                        string -= 2;
                        *(uint16_t *)string = *(uint16_t *)&stbsp__digitpair.pair[(n % 100) * 2];
                        n /= 100;
                    } while (n);
                    }
                    while (n) {
                    if ((formatting_flags & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                        l = 0;
                        *--string = stbsp__comma;
                        --o;
                    } else {
                        *--string = (char)(n % 10) + '0';
                        n /= 10;
                    }
                    }
                    if (n64 == 0) {
                    if ((string[0] == '0') && (string != (num + STBSP__NUMSZ)))
                        ++string;
                    break;
                    }
                    while (string != o)
                    if ((formatting_flags & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                        l = 0;
                        *--string = stbsp__comma;
                        --o;
                    } else {
                        *--string = '0';
                    }
                }

                tail[0] = 0;
                stbsp__lead_sign(formatting_flags, lead);

                // get the length that we copied
                l = (uint32_t)((num + STBSP__NUMSZ) - string);
                if (l == 0) {
                    *--string = '0';
                    l = 1;
                }
                comma_spacing = l + (3u << 24);
                if (precision < 0)
                    precision = 0;

            L_STRING_COPY:
                // get field_width=leading/trailing space, precision=leading zeros
                if (precision < (int32_t)l)
                    precision = (int32_t) l;
                n = (uint32_t)(precision + lead[0] + tail[0] + trailing_zeros);
                if (field_width < (int32_t)n)
                    field_width = (int32_t) n;
                field_width -= (int32_t) n;
                precision -= (int32_t) l;

                // handle right justify and leading zeros
                if ((formatting_flags & STBSP__LEFTJUST) == 0)
                {
                    if (formatting_flags & STBSP__LEADINGZERO) // if leading zeros, everything is in precision
                    {
                        precision = (field_width > precision) ? field_width : precision;
                        field_width = 0;
                    }
                    else
                    {
                        formatting_flags &= (uint32_t) ~STBSP__TRIPLET_COMMA; // if no leading zeros, then no commas
                    }
                }

                // copy the spaces and/or zeros
                if (field_width + precision)
                {
                    int32_t i;
                    uint32_t c;

                    // copy leading spaces (or when doing %8.4d stuff)
                    if ((formatting_flags & STBSP__LEFTJUST) == 0)
                    {
                        while (field_width > 0)
                        {
                            stbsp__cb_buf_clamp(i, field_width);

                            field_width -= i;
                            JSL_MEMSET(buffer_cursor, ' ', (size_t) i);
                            buffer_cursor += i;

                            stbsp__chk_cb_buf(1);
                        }
                    }

                    // copy leader
                    source_ptr = lead + 1;
                    while (lead[0])
                    {
                        stbsp__cb_buf_clamp(i, lead[0]);

                        lead[0] -= (char) i;
                        JSL_MEMCPY(buffer_cursor, source_ptr, (size_t) i);
                        buffer_cursor += i;
                        source_ptr += i;

                        stbsp__chk_cb_buf(1);
                    }

                    // copy leading zeros
                    c = comma_spacing >> 24;
                    comma_spacing &= 0xffffff;
                    comma_spacing = (formatting_flags & STBSP__TRIPLET_COMMA) ? ((uint32_t)(c - ((((uint32_t) precision) + comma_spacing) % (c + 1u)))) : 0;

                    while (precision > 0)
                    {
                    stbsp__cb_buf_clamp(i, precision);
                    precision -= i;

                    if (JSL_IS_BITFLAG_NOT_SET(formatting_flags, STBSP__TRIPLET_COMMA))
                    {
                        JSL_MEMSET(buffer_cursor, '0', (size_t) i);
                        buffer_cursor += i;
                    }
                    else
                    {
                        while (i)
                        {
                            if (comma_spacing == c)
                            {
                                comma_spacing = 0;
                                *buffer_cursor = (uint8_t) stbsp__comma;
                            }
                            else
                            {
                                *buffer_cursor = '0';
                            }

                            ++buffer_cursor;
                            ++comma_spacing;
                            --i;
                        }
                    }

                    stbsp__chk_cb_buf(1);
                    }

                }

                // copy leader if there is still one
                source_ptr = lead + 1;
                while (lead[0])
                {
                    int32_t i;
                    stbsp__cb_buf_clamp(i, lead[0]);

                    lead[0] -= (char) i;
                    JSL_MEMCPY(buffer_cursor, source_ptr, (size_t) i);
                    buffer_cursor += i;
                    source_ptr += i;

                    stbsp__chk_cb_buf(1);
                }

                // copy the string
                n = l;
                while (n)
                {
                    int32_t i;
                    stbsp__cb_buf_clamp(i, (int32_t) n);

                    JSL_MEMCPY(buffer_cursor, string, (size_t) i);

                    n -= (uint32_t) i;
                    buffer_cursor += i;
                    string += i;

                    stbsp__chk_cb_buf(1);
                }

                // copy trailing zeros
                while (trailing_zeros)
                {
                    int32_t i;
                    stbsp__cb_buf_clamp(i, trailing_zeros);

                    trailing_zeros -= i;
                    JSL_MEMSET(buffer_cursor, '0', (size_t) i);
                    buffer_cursor += i;

                    stbsp__chk_cb_buf(1);
                }

                // copy tail if there is one
                source_ptr = tail + 1;
                while (tail[0])
                {
                    int32_t i;
                    stbsp__cb_buf_clamp(i, tail[0]);

                    tail[0] -= (char)i;
                    JSL_MEMCPY(buffer_cursor, source_ptr, (size_t) i);
                    buffer_cursor += i;
                    source_ptr += i;

                    stbsp__chk_cb_buf(1);
                }

                // handle the left justify
                if (formatting_flags & STBSP__LEFTJUST)
                {
                    if (field_width > 0)
                    {
                    while (field_width)
                    {
                        int32_t i;
                        stbsp__cb_buf_clamp(i, field_width);

                        field_width -= i;
                        JSL_MEMSET(buffer_cursor, ' ', (size_t) i);
                        buffer_cursor += i;

                        stbsp__chk_cb_buf(1);
                    }
                    }
                }

                break;

            default: // unknown, just copy code
                string = num + STBSP__NUMSZ - 1;
                *string = (char) f.data[0];
                l = 1;
                field_width = formatting_flags = 0;
                lead[0] = 0;
                tail[0] = 0;
                precision = 0;
                decimal_precision = 0;
                comma_spacing = 0;
                goto L_STRING_COPY;
        }

        JSL_FATPTR_ADVANCE(f, 1);
    }


    L_END_FORMAT:

    if (!callback)
        *buffer_cursor = 0;
    else
        stbsp__flush_cb();

    done:
    return tlen + (int32_t)(buffer_cursor - buffer);
}

// cleanup
#undef STBSP__LEFTJUST
#undef STBSP__LEADINGPLUS
#undef STBSP__LEADINGSPACE
#undef STBSP__LEADING_0X
#undef STBSP__LEADINGZERO
#undef STBSP__INTMAX
#undef STBSP__TRIPLET_COMMA
#undef STBSP__NEGATIVE
#undef STBSP__METRIC_SUFFIX
#undef STBSP__NUMSZ
#undef stbsp__chk_cb_bufL
#undef stbsp__chk_cb_buf
#undef stbsp__flush_cb
#undef stbsp__cb_buf_clamp

// ============================================================================
//   wrapper functions

typedef struct stbsp__context {
    JSLFatPtr buffer;
    int64_t length;
    uint8_t tmp[JSL_FORMAT_MIN_BUFFER];
} stbsp__context;

static uint8_t* stbsp__clamp_callback(uint8_t* buf, void *user, int64_t len)
{
    stbsp__context* context = (stbsp__context*) user;
    context->length += len;

    if (len > context->buffer.length)
        len = context->buffer.length;

    if (len)
    {
        if (buf != context->buffer.data)
        {
            JSL_MEMCPY(context->buffer.data, buf, (size_t) len);
        }

        context->buffer.data += len;
        context->buffer.length -= len;
    }

    if (context->buffer.length <= 0)
        return context->tmp;

    return (context->buffer.length >= JSL_FORMAT_MIN_BUFFER) ?
        context->buffer.data
        : context->tmp; // go direct into buffer if you can
}

static uint8_t* stbsp__count_clamp_callback(uint8_t* buf, void* user, int64_t len)
{
    stbsp__context* context = (stbsp__context*) user;
    (void) sizeof(buf);

    context->length += len;
    return context->tmp; // go direct into buffer if you can
}

JSL__ASAN_OFF int64_t jsl_format_valist(JSLFatPtr* buffer, JSLFatPtr fmt, va_list va )
{
    stbsp__context context;
    context.length = 0;

    if ((buffer->length == 0) && buffer->data == NULL)
    {
        context.buffer.data = NULL;
        context.buffer.length = 0;

        jsl_format_callback(
            stbsp__count_clamp_callback,
            &context,
            context.tmp,
            fmt,
            va
        );
    }
    else
    {
        context.buffer = *buffer;

        jsl_format_callback(
            stbsp__clamp_callback,
            &context,
            stbsp__clamp_callback(0, &context, 0),
            fmt,
            va
        );
    }

    *buffer = context.buffer;

    return context.length;
}

struct JSL__ArenaContext
{
    JSLArena* arena;
    JSLFatPtr current_allocation;
    uint8_t* cursor;
    uint8_t buffer[JSL_FORMAT_MIN_BUFFER];
};

static uint8_t* format_arena_callback(uint8_t *buf, void *user, int64_t len)
{
    struct JSL__ArenaContext* context = (struct JSL__ArenaContext*) user;

    // First call
    if (context->cursor == NULL)
    {
        context->current_allocation = jsl_arena_allocate(context->arena, len, false);
        if (context->current_allocation.data == NULL)
            return 0;

        context->cursor = context->current_allocation.data;
    }
    else
    {
        int64_t new_length = context->current_allocation.length + len;
        context->current_allocation = jsl_arena_reallocate(
            context->arena,
            context->current_allocation,
            new_length
        );
        if (context->current_allocation.data == NULL)
            return 0;
    }

    JSL_MEMCPY(context->cursor, buf, (size_t) len);
    context->cursor += len;

    return context->buffer;
}

JSL__ASAN_OFF JSLFatPtr jsl_format(JSLArena* arena, JSLFatPtr fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    struct JSL__ArenaContext context;
    context.arena = arena;
    context.current_allocation.data = NULL;
    context.current_allocation.length = 0;
    context.cursor = NULL;

    jsl_format_callback(
        format_arena_callback,
        &context,
        context.buffer,
        fmt,
        va
    );

    va_end(va);

    JSLFatPtr ret = {0};
    int64_t write_length = context.cursor - context.current_allocation.data;
    if (context.cursor != NULL && write_length > 0)
    {
        ret.data = context.current_allocation.data;
        ret.length = write_length;
    }

    return ret;
}

JSL__ASAN_OFF int64_t jsl_format_buffer(JSLFatPtr* buffer, JSLFatPtr fmt, ...)
{
    int64_t result;
    va_list va;
    va_start(va, fmt);

    result = jsl_format_valist(buffer, fmt, va);
    va_end(va);

    return result;
}

// =======================================================================
//   low level float utility functions

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#define STBSP__COPYFP(dest, src)                   \
{                                               \
    int32_t counter;                                      \
    for (counter = 0; counter < 8; counter++)                   \
        ((char *)&dest)[counter] = ((char *)&src)[counter]; \
}

// get float info
static int32_t stbsp__real_to_parts(int64_t *bits, int32_t *expo, double value)
{
double d;
int64_t b = 0;

// load value and round at the frac_digits
d = value;

STBSP__COPYFP(b, d);

const int64_t mantissa_mask = (int64_t)((((uint64_t)1u) << 52u) - 1u);
*bits = b & mantissa_mask;
*expo = (int32_t)(((b >> 52) & 2047) - 1023);

return (int32_t)((uint64_t) b >> 63);
}

static double const stbsp__bot[23] = {
    1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
    1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022
};
static double const stbsp__negbot[22] = {
    1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
    1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022
};
static double const stbsp__negboterr[22] = {
    -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
    4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
    -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032, 2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
    2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,  -4.8596774326570872e-039
};
static double const stbsp__top[13] = {
    1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299
};
static double const stbsp__negtop[13] = {
    1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299
};
static double const stbsp__toperr[13] = {
    8388608,
    6.8601809640529717e+028,
    -7.253143638152921e+052,
    -4.3377296974619174e+075,
    -1.5559416129466825e+098,
    -3.2841562489204913e+121,
    -3.7745893248228135e+144,
    -1.7356668416969134e+167,
    -3.8893577551088374e+190,
    -9.9566444326005119e+213,
    6.3641293062232429e+236,
    -5.2069140800249813e+259,
    -5.2504760255204387e+282
};
static double const stbsp__negtoperr[13] = {
    3.9565301985100693e-040,  -2.299904345391321e-063,  3.6506201437945798e-086,  1.1875228833981544e-109,
    -5.0644902316928607e-132, -6.7156837247865426e-155, -2.812077463003139e-178,  -5.7778912386589953e-201,
    7.4997100559334532e-224,  -4.6439668915134491e-247, -6.3691100762962136e-270, -9.436808465446358e-293,
    8.0970921678014997e-317
};

static uint64_t const stbsp__powten[20] = {
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000,
    1000000000,
    10000000000ULL,
    100000000000ULL,
    1000000000000ULL,
    10000000000000ULL,
    100000000000000ULL,
    1000000000000000ULL,
    10000000000000000ULL,
    100000000000000000ULL,
    1000000000000000000ULL,
    10000000000000000000ULL
};
#define stbsp__tento19th (1000000000000000000ULL)

#define stbsp__ddmulthi(oh, ol, xh, yh)                             \
{                                                                   \
    double ahi = 0, alo, bhi = 0, blo;                              \
    int64_t bt = 0;                                                 \
    oh = xh * yh;                                                   \
    STBSP__COPYFP(bt, xh);                                          \
    bt &= ((~(uint64_t)0) << 27);                                   \
    STBSP__COPYFP(ahi, bt);                                         \
    alo = xh - ahi;                                                 \
    STBSP__COPYFP(bt, yh);                                          \
    bt &= ((~(uint64_t)0) << 27);                                   \
    STBSP__COPYFP(bhi, bt);                                         \
    blo = yh - bhi;                                                 \
    ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo;    \
}

#define stbsp__ddtoS64(ob, xh, xl)                                  \
{                                                                   \
    double ahi = 0, alo, vh, t;                                     \
    ob = (int64_t)xh;                                               \
    vh = (double)ob;                                                \
    ahi = (xh - vh);                                                \
    t = (ahi - xh);                                                 \
    alo = (xh - (ahi - t)) - (vh + t);                              \
    ob += (int64_t)(ahi + alo + xl);                                \
}

#define stbsp__ddrenorm(oh, ol) \
{                            \
    double s;                 \
    s = oh + ol;              \
    ol = ol - (s - oh);       \
    oh = s;                   \
}

#define stbsp__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#define stbsp__ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);

static void stbsp__raise_to_power10(double *ohi, double *olo, double d, int32_t power) // power can be -323 to +350
{
    double ph, pl;
    if ((power >= 0) && (power <= 22)) {
        stbsp__ddmulthi(ph, pl, d, stbsp__bot[power]);
    } else {
        int32_t e, et, eb;
        double p2h, p2l;

        e = power;
        if (power < 0)
            e = -e;
        et = (e * 0x2c9) >> 14; /* %23 */
        if (et > 13)
            et = 13;
        eb = e - (et * 23);

        ph = d;
        pl = 0.0;
        if (power < 0) {
            if (eb) {
                --eb;
                stbsp__ddmulthi(ph, pl, d, stbsp__negbot[eb]);
                stbsp__ddmultlos(ph, pl, d, stbsp__negboterr[eb]);
            }
            if (et) {
                stbsp__ddrenorm(ph, pl);
                --et;
                stbsp__ddmulthi(p2h, p2l, ph, stbsp__negtop[et]);
                stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__negtop[et], stbsp__negtoperr[et]);
                ph = p2h;
                pl = p2l;
            }
        } else {
            if (eb) {
                e = eb;
                if (eb > 22)
                eb = 22;
                e -= eb;
                stbsp__ddmulthi(ph, pl, d, stbsp__bot[eb]);
                if (e) {
                stbsp__ddrenorm(ph, pl);
                stbsp__ddmulthi(p2h, p2l, ph, stbsp__bot[e]);
                stbsp__ddmultlos(p2h, p2l, stbsp__bot[e], pl);
                ph = p2h;
                pl = p2l;
                }
            }
            if (et) {
                stbsp__ddrenorm(ph, pl);
                --et;
                stbsp__ddmulthi(p2h, p2l, ph, stbsp__top[et]);
                stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__top[et], stbsp__toperr[et]);
                ph = p2h;
                pl = p2l;
            }
        }
    }
    stbsp__ddrenorm(ph, pl);
    *ohi = ph;
    *olo = pl;
}

// given a float value, returns the significant bits in bits, and the position of the
//   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
//   returned in the decimal_pos parameter.
// frac_digits is absolute normally, but if you want from first significant digits (got %g and %e), or in 0x80000000
static int32_t stbsp__real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits)
{
    double d;
    int64_t bits = 0;
    int32_t expo, e, ng, tens;

    d = value;
    STBSP__COPYFP(bits, d);
    expo = (int32_t)((bits >> 52) & 2047);
    ng = (int32_t)((uint64_t) bits >> 63);
    if (ng)
        d = -d;

    if (expo == 2047) // is nan or inf?
    {
        *start = ((uint64_t) bits & ((((uint64_t) 1u) << 52u) - 1u)) ? "NaN" : "Inf";
        *decimal_pos = STBSP__SPECIAL;
        *len = 3;
        return ng;
    }

    if (expo == 0) // is zero or denormal
    {
        if (((uint64_t) bits << 1) == 0) // do zero
        {
            *decimal_pos = 1;
            *start = out;
            out[0] = '0';
            *len = 1;
            return ng;
        }
        // find the right expo for denormals
        {
            int64_t v = ((uint64_t)1) << 51;
            while ((bits & v) == 0) {
                --expo;
                v >>= 1;
            }
        }
    }

    // find the decimal exponent as well as the decimal bits of the value
    {
        double ph, pl;

        // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of log10 of all expos 1..2046
        tens = expo - 1023;
        tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

        // move the significant bits into position and stick them into an int32_t
        stbsp__raise_to_power10(&ph, &pl, d, 18 - tens);

        // get full as much precision from double-double as possible
        stbsp__ddtoS64(bits, ph, pl);

        // check if we undershot
        if (((uint64_t)bits) >= stbsp__tento19th)
            ++tens;
    }

    // now do the rounding in integer land
    frac_digits = (frac_digits & 0x80000000u) ? ((frac_digits & 0x7ffffffu) + 1u) : (frac_digits + (uint32_t) tens);
    if ((frac_digits < 24)) {
        uint32_t dg = 1;
        if ((uint64_t)bits >= stbsp__powten[9])
            dg = 10;
        while ((uint64_t)bits >= stbsp__powten[dg]) {
            ++dg;
            if (dg == 20)
                goto L_NO_ROUND;
        }
        if (frac_digits < dg) {
            uint64_t r;
            // add 0.5 at the right position and round
            e = (int32_t) (dg - frac_digits);
            if ((uint32_t)e >= 24)
                goto L_NO_ROUND;
            r = stbsp__powten[e];
            bits = bits + (int64_t) (r / 2);
            if ((uint64_t) bits >= stbsp__powten[dg])
                ++tens;
            bits /= r;
        }
    L_NO_ROUND:;
    }

    // kill long trailing runs of zeros
    if (bits) {
        uint32_t n;
        for (;;) {
            if (bits <= 0xffffffff)
                break;
            if (bits % 1000)
                goto donez;
            bits /= 1000;
        }
        n = (uint32_t)bits;
        while ((n % 1000) == 0)
            n /= 1000;
        bits = n;
    donez:;
    }

    // convert to string
    out += 64;
    e = 0;
    for (;;) {
        uint32_t n;
        char *o = out - 8;
        // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant denomiators be damned)
        if (bits >= 100000000) {
            n = (uint32_t)(bits % 100000000);
            bits /= 100000000;
        } else {
            n = (uint32_t)bits;
            bits = 0;
        }
        while (n) {
            out -= 2;
            *(uint16_t *)out = *(uint16_t *)&stbsp__digitpair.pair[(n % 100) * 2];
            n /= 100;
            e += 2;
        }
        if (bits == 0) {
            if ((e) && (out[0] == '0')) {
                ++out;
                --e;
            }
            break;
        }
        while (out != o) {
            *--out = '0';
            ++e;
        }
    }

    *decimal_pos = tens;
    *start = out;
    *len = (uint32_t) e;
    return ng;
}

// clean up
#undef stbsp__ddmulthi
#undef stbsp__ddrenorm
#undef stbsp__ddmultlo
#undef stbsp__ddmultlos
#undef STBSP__SPECIAL
#undef STBSP__COPYFP
#undef STBSP__UNALIGNED

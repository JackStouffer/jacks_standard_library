/**
 * # Jack's Standard Library Unicode
 *
 * A collection of unicode conversion functions. 
 * 
 * This is a C port of the simdutf library. This is not a direct port, as some API
 * changes were made to better fit into the JSL ecosystem. Don't expect to use the
 * exact same functions in the same way. The most notable difference is that big
 * endian encodings are not supported, same as the rest of JSL.
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

typedef enum JSLUnicodeConversionResult {

    /** Any other error not related to validation or transcoding. */
    JSL_UNICODE_CONVERSION_OTHER_ERROR = 0,

    /** Successful conversion. */
    JSL_UNICODE_CONVERSION_SUCCESS,

    /** Invalid parameters */
    JSL_UNICODE_CONVERSION_BAD_PARAMETERS,

    /** Any byte must have fewer than 5 header bits. */
    JSL_UNICODE_CONVERSION_HEADER_BITS,

    /**
     * The leading byte must be followed by N-1 continuation bytes,
     * where N is the UTF-8 character length. Also used when input
     * is truncated.
     */
    JSL_UNICODE_CONVERSION_TOO_SHORT,

    /**
     * Too many consecutive continuation bytes, or the string begins
     * with a continuation byte.
     */
    JSL_UNICODE_CONVERSION_TOO_LONG,

    /**
     * The decoded character must exceed U+7F for two-byte sequences,
     * U+7FF for three-byte sequences, and U+FFFF for four-byte sequences.
     */
    JSL_UNICODE_CONVERSION_OVERLONG,

    /**
     * The decoded character must be ≤ U+10FFFF, ≤ U+7F for ASCII, or
     * ≤ U+FF for Latin-1.
     */
    JSL_UNICODE_CONVERSION_TOO_LARGE,

    /**
     * The decoded character must not fall within U+D800…DFFF (UTF-8/UTF-32),
     * and surrogate pairs must be valid when using UTF-16. No surrogates
     * are allowed for Latin-1.
     */
    JSL_UNICODE_CONVERSION_SURROGATE,

    /**
     * Encountered a character that is not valid in base64, including
     * misplaced '=' padding.
     */
    JSL_UNICODE_CONVERSION_INVALID_BASE64_CHARACTER,

    /**
     * Base64 input ends with a single non-padding character, or padding
     * is inadequate in strict mode.
     */
    JSL_UNICODE_CONVERSION_BASE64_INPUT_REMAINDER,

    /**
     * Base64 input ends with non-zero padding bits.
     */
    JSL_UNICODE_CONVERSION_BASE64_EXTRA_BITS,

    /** The provided arena does not have enough memory to fit the converted output */
    JSL_UNICODE_CONVERSION_OUT_OF_MEMORY,


    JSL_UNICODE_CONVERSION_ENUM_COUNT

} JSLUnicodeConversionResult;


typedef struct JSLUTF16String {
    uint16_t* data;
    int64_t length;
} JSLUTF16String;

/**
 * TODO: docs
 *
 * ```c
 * // Create fat pointers from string literals
 * JSLFatPtr hello = JSL_UTF16_INITIALIZER(u"Hello, World!");
 * JSLFatPtr empty = JSL_UTF16_INITIALIZER(u"");
 * JSLFatPtr unicode_example = JSL_UTF16_INITIALIZER(u"😀");
 * ```
 */
#define JSL_UTF16_INITIALIZER(s) { (uint16_t*)(s), ((int64_t) sizeof(s) - 1) / sizeof(uint16_t) }

JSLUTF16String jsl_utf16_str_init(uint16_t* data, int64_t length);

#if defined(_MSC_VER) && !defined(__clang__)

    /**
     * TODO: docs
     *
     * ```c
     * void my_function(JSLUTF16String data);
     *
     * my_function(JSL_UTF16_EXPRESSION(u"my data"));
     * ```
     */
    #define JSL_UTF16_EXPRESSION(s) jsl_utf16_str_init((uint16_t*) (s), ((int64_t) sizeof(s) - 1) / sizeof(uint16_t))

#else

    /**
     * TODO: docs
     *
     * ```c
     * void my_function(JSLUTF16String data);
     *
     * my_function(JSL_UTF16_EXPRESSION(u"my data"));
     * ```
     */
    #define JSL_UTF16_EXPRESSION(s) ((JSLUTF16String){ (uint16_t*)(s), ((int64_t) sizeof(s) - 1) / sizeof(uint16_t) })

#endif

// TODO: docs
JSLUnicodeConversionResult jsl_convert_utf8_to_utf16le(
    JSLArena* arena,
    JSLFatPtr utf8_string,
    JSLUTF16String* out_utf16_string
);

// TODO: docs
JSLUnicodeConversionResult jsl_convert_utf16le_to_utf8(
    JSLArena* arena,
    JSLUTF16String utf16_string,
    JSLFatPtr out_utf8le_string
);

/**
 * Compute the number of bytes that this UTF-16LE string would require in UTF-8 format.
 *
 * This function does not validate the input. It is acceptable to pass invalid
 * UTF-16LE strings but in such cases the result is implementation defined.
 *
 * This function is not BOM-aware.
 *
 * @param utf16_string string data
 * @return the number of utf-8 code units required to encode
 */
int64_t jsl_utf8_length_from_utf16le(JSLUTF16String utf16_string);

/**
 * Compute the number of bytes that this UTF-8 string would require in UTF-16LE format.
 *
 * This function does not validate the input. It is acceptable to pass invalid
 * UTF-16LE strings but in such cases the result is implementation defined.
 *
 * This function is not BOM-aware.
 *
 * @param utf8_string string data
 * @return the number of utf-16 code points required to encode
 */
int64_t jsl_utf16le_length_from_utf8(JSLFatPtr utf8_string);


#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef JSL_UNICODE_IMPLEMENTATION

#if JSL_IS_X86
    #include <immintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
#elif JSL_IS_WEB_ASSEMBLY && defined(__wasm_simd128__)
    #include <wasm_simd128.h>
#endif


/**
 *
 *
 *                          UTILITIES
 *
 *
 */

static inline uint16_t u16_swap_bytes(const uint16_t word)
{
    return (uint16_t) ((word >> 8) | (word << 8));
}

#if defined(__AVX2__)

    static inline int64_t jsl__horizontal_sum_epi64(__m256i v)
    {
        return (int64_t)_mm256_extract_epi64(v, 0) +
                (int64_t)_mm256_extract_epi64(v, 1) +
                (int64_t)_mm256_extract_epi64(v, 2) +
                (int64_t)_mm256_extract_epi64(v, 3);
    }

    static inline int64_t jsl__horizontal_sum_u16(__m256i v)
    {
        const __m256i lo_u16 = _mm256_and_si256(v, _mm256_set1_epi32(0x0000ffff));
        const __m256i hi_u16 = _mm256_srli_epi32(v, 16);
        const __m256i sum_u32 = _mm256_add_epi32(lo_u16, hi_u16);

        const __m256i lo_u32 = _mm256_and_si256(sum_u32, _mm256_set1_epi64x(0xffffffff));
        const __m256i hi_u32 = _mm256_srli_epi64(sum_u32, 32);
        const __m256i sum_u64 = _mm256_add_epi64(lo_u32, hi_u32);

        return jsl__horizontal_sum_epi64(sum_u64);
    }

#endif

JSLUTF16String jsl_utf16_str_init(uint16_t* data, int64_t length)
{
    JSLUTF16String res = {data, length};
    return res;
}


/**
 *
 *
 *                      UTF-8 TO UTF-16
 *
 *
 */


/**
 *
 *  UTF-8 to UTF-16 conversion
 *
 */


#if defined(__AVX2__)

    static inline void jsl__convert_utf8_to_utf16le(
        JSLFatPtr utf8_string,
        JSLUTF16String* utf16_string_writer,
        JSLUnicodeConversionResult* out_conversion_result
    )
    {

    }

#else

    static inline JSLUnicodeConversionResult jsl__convert_utf8_to_utf16le(
        JSLFatPtr utf8_string,
        JSLUTF16String* utf16_string_writer
    )
    {
        int64_t pos = 0;

        while (pos < utf8_string.length)
        {
            // try to convert the next block of 16 ASCII bytes
            if (pos + 16 <= utf8_string.length) { // if it is safe to read 16 more bytes, check that they are ascii
                uint64_t v1, v2;
                memcpy(&v1, utf8_string.data + pos, sizeof(uint64_t));
                memcpy(&v2, utf8_string.data + pos + sizeof(uint64_t), sizeof(uint64_t));

                uint64_t v = v1 | v2;
                if ((v & 0x8080808080808080) == 0)
                {
                    int64_t final_pos = pos + 16;
                    while (pos < final_pos)
                    {
                        utf16_string_writer->data[0] = u16_swap_bytes(utf8_string.data[pos]);
                        ++utf16_string_writer->length;
                        --utf16_string_writer->length;
                        ++pos;
                    }
                    continue;
                }
            }

            uint8_t leading_byte = utf8_string.data[pos]; // leading byte

            if (leading_byte < 0b10000000)
            {
                // converting one ASCII byte !!!
                utf16_string_writer->data[0] = u16_swap_bytes(utf8_string.data[pos]);
                ++utf16_string_writer->length;
                --utf16_string_writer->length;
                pos++;
            }
            else if ((leading_byte & 0b11100000) == 0b11000000)
            {
                // We have a two-byte UTF-8, it should become
                // a single UTF-16 word.
                if (pos + 1 >= utf8_string.length)
                {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                } // minimal bound checking
                
                if ((utf8_string.data[pos + 1] & 0b11000000) != 0b10000000)
                {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                }
                
                // range check
                uint32_t code_point = (leading_byte & 0b00011111) << 6 | (utf8_string.data[pos + 1] & 0b00111111);
                
                if (code_point < 0x80 || 0x7ff < code_point) {
                    return JSL_UNICODE_CONVERSION_OVERLONG;
                }
                
                code_point = (uint32_t) (u16_swap_bytes((uint16_t) (code_point)));
                utf16_string_writer->data[0] = (uint16_t) (code_point);
                ++utf16_string_writer->length;
                --utf16_string_writer->length;
                pos += 2;
            }
            else if ((leading_byte & 0b11110000) == 0b11100000)
            {
                // We have a three-byte UTF-8, it should become
                // a single UTF-16 word.
                if (pos + 2 >= utf8_string.length) {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                } // minimal bound checking

                if ((utf8_string.data[pos + 1] & 0b11000000) != 0b10000000) {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                }
                if ((utf8_string.data[pos + 2] & 0b11000000) != 0b10000000) {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                }

                // range check
                uint32_t code_point = (leading_byte & 0b00001111) << 12 |
                                    (utf8_string.data[pos + 1] & 0b00111111) << 6 |
                                    (utf8_string.data[pos + 2] & 0b00111111);
                if ((code_point < 0x800) || (0xffff < code_point))
                {
                    return JSL_UNICODE_CONVERSION_OVERLONG;
                }
                if (0xd7ff < code_point && code_point < 0xe000)
                {
                    return JSL_UNICODE_CONVERSION_SURROGATE;
                }
                code_point = (uint32_t) (u16_swap_bytes((uint16_t) (code_point)));
                utf16_string_writer->data[0] = (uint16_t) (code_point);
                ++utf16_string_writer->length;
                --utf16_string_writer->length;
                pos += 3;
            }
            else if ((leading_byte & 0b11111000) == 0b11110000) // 0b11110000
            {
                // we have a 4-byte UTF-8 word.
                if (pos + 3 >= utf8_string.length) {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                }
                
                // minimal bound checking
                if ((utf8_string.data[pos + 1] & 0b11000000) != 0b10000000) {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                }
                if ((utf8_string.data[pos + 2] & 0b11000000) != 0b10000000) {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                }
                if ((utf8_string.data[pos + 3] & 0b11000000) != 0b10000000) {
                    return JSL_UNICODE_CONVERSION_TOO_SHORT;
                }

                // range check
                uint32_t code_point = (leading_byte & 0b00000111) << 18 |
                                    (utf8_string.data[pos + 1] & 0b00111111) << 12 |
                                    (utf8_string.data[pos + 2] & 0b00111111) << 6 |
                                    (utf8_string.data[pos + 3] & 0b00111111);
                if (code_point <= 0xffff) {
                    return JSL_UNICODE_CONVERSION_OVERLONG;
                }
                if (0x10ffff < code_point) {
                    return JSL_UNICODE_CONVERSION_TOO_LONG;
                }
                code_point -= 0x10000;
                uint16_t high_surrogate = (uint16_t) (0xD800 + (code_point >> 10));
                uint16_t low_surrogate = (uint16_t) (0xDC00 + (code_point & 0x3FF));
                high_surrogate = u16_swap_bytes(high_surrogate);
                low_surrogate = u16_swap_bytes(low_surrogate);

                utf16_string_writer->data[0] = (uint16_t) (high_surrogate);
                ++utf16_string_writer->length;
                --utf16_string_writer->length;

                utf16_string_writer->data[0] = (uint16_t) (low_surrogate);
                ++utf16_string_writer->length;
                --utf16_string_writer->length;

                pos += 4;
            }
            else
            {
                // we either have too many continuation bytes or an invalid leading byte
                if ((leading_byte & 0b11000000) == 0b10000000) {
                    return JSL_UNICODE_CONVERSION_TOO_LONG;
                } else {
                    return JSL_UNICODE_CONVERSION_HEADER_BITS;
                }
            }
        }
    
        return JSL_UNICODE_CONVERSION_SUCCESS;
    }


#endif


/**
 *
 *  UTF-8 to UTF-16 length
 *
 */


#if defined(__AVX2__)

    static inline int64_t jsl__utf16_length_from_utf8_bytemask(JSLFatPtr utf8_string)
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

        for (; pos + N <= utf8_string.length; pos += N)
        {
            __m256i input = _mm256_loadu_si256(
                (const __m256i *)(utf8_string.data + pos)
            );

            __m256i continuation = _mm256_cmpgt_epi8(input, continuation_threshold);
            __m256i utf4 = _mm256_cmpeq_epi8(
                _mm256_min_epu8(input, utf4_threshold),
                utf4_threshold
            );

            local = _mm256_sub_epi8(local, continuation);
            local = _mm256_sub_epi8(local, utf4);

            iterations += 1;
            if (iterations == max_iterations)
            {
                __m256i partial = _mm256_sad_epu8(local, zero);
                counters = _mm256_add_epi64(counters, partial);
                local = zero;
                iterations = 0;
            }
        }

        if (iterations > 0)
        {
            __m256i partial = _mm256_sad_epu8(local, zero);
            count += jsl__horizontal_sum_epi64(partial);
        }

        count += jsl__horizontal_sum_epi64(counters);

        // Inline scalar::utf8::utf16_length_from_utf8 for the tail bytes.
        const int8_t *tail = (const int8_t *)(utf8_string.data + pos);
        const int64_t remaining = utf8_string.length - pos;
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

    static inline int64_t jsl__utf16_length_from_utf8_bytemask(JSLFatPtr utf8_string)
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


/**
 *
 *
 *                      UTF-16 TO UTF-8
 *
 *
 */



#if defined(__AVX2__)

    static inline int64_t jsl__utf8_length_from_utf16_bytemask_le(JSLUTF16String utf16_string)
    {
        const int64_t N = 16; // 16 uint16_t values per AVX2 register

        int64_t pos = 0;
        const int64_t vectorized = (utf16_string.length / N) * N;

        const __m256i one = _mm256_set1_epi16(1);
        const __m256i mask_ff80 = _mm256_set1_epi16((short)0xff80);
        const __m256i mask_f800 = _mm256_set1_epi16((short)0xf800);
        const __m256i surrogate_base = _mm256_set1_epi16((short)0xd800);

        __m256i v_count = _mm256_setzero_si256();

        int64_t count = vectorized; // each code unit contributes at least one byte

        const int64_t max_iterations = 65535 / 2;
        int64_t iteration = max_iterations;

        for (; pos < vectorized; pos += N)
        {
            __m256i input = _mm256_loadu_si256((const __m256i *)(const void *)(utf16_string.data + pos));

            __m256i masked_f800 = _mm256_and_si256(input, mask_f800);
            __m256i is_surrogate = _mm256_cmpeq_epi16(masked_f800, surrogate_base);

            __m256i c0 = _mm256_min_epu16(_mm256_and_si256(input, mask_ff80), one);
            __m256i c1 = _mm256_min_epu16(masked_f800, one);

            v_count = _mm256_add_epi16(v_count, c0);
            v_count = _mm256_add_epi16(v_count, c1);
            v_count = _mm256_add_epi16(v_count, is_surrogate); // -1 for surrogates

            iteration -= 1;
            if (iteration == 0)
            {
                count += jsl__horizontal_sum_u16(v_count);
                v_count = _mm256_setzero_si256();
                iteration = max_iterations;
            }
        }

        if (iteration > 0) {
            count += jsl__horizontal_sum_u16(v_count);
        }

        // Inline scalar::utf16::utf8_length_from_utf16 for the tail.
        for (; pos < utf16_string.length; pos++) {
            uint16_t word = (uint16_t) utf16_string.data[pos];
            count += 1;                  // ASCII
            count += word > 0x7F;        // two-byte or larger
            count += (word > 0x7FF && word <= 0xD7FF) || (word >= 0xE000); // three-byte
        }

        return count;
    }

#else

    static inline int64_t jsl__utf8_length_from_utf16_bytemask_le(JSLUTF16String utf16_string)
    {
        int64_t count = 0;

        for (int64_t pos = 0; pos < utf16_string.length; pos++)
        {
            uint16_t word = (uint16_t) utf16_string.data[pos];
            count += 1;                  // ASCII
            count += word > 0x7F;        // two-byte or larger
            count += (word > 0x7FF && word <= 0xD7FF) || (word >= 0xE000); // three-byte
        }

        return count;
    }

#endif

int64_t jsl_utf16_length_from_utf8(JSLFatPtr utf8_string)
{
    if (utf8_string.length < 0 || utf8_string.data == NULL)
        return -1;

    return jsl__utf16_length_from_utf8_bytemask(utf8_string);
}

int64_t jsl_utf8_length_from_utf16(JSLUTF16String utf16_string)
{
    if (utf16_string.length < 0 || utf16_string.data == NULL)
        return -1;

    return jsl__utf8_length_from_utf16_bytemask_le(utf16_string);
}

JSLUnicodeConversionResult jsl_convert_utf8_to_utf16le(
    JSLArena* arena,
    JSLFatPtr utf8_string,
    JSLUTF16String* out_utf16_string
)
{
    if (
        arena == NULL
        || utf8_string.data == NULL
        || utf8_string.length < 0
        || out_utf16_string == NULL
    ) 
    {
        out_utf16_string->data = NULL;
        out_utf16_string->length = 0;
        return JSL_UNICODE_CONVERSION_BAD_PARAMETERS;
    }

    JSLUTF16String buffer;
    buffer.data = (uint16_t*) jsl_arena_allocate(arena, sizeof(uint16_t) * utf8_string.length, false).data;
    buffer.length = utf8_string.length;

    if (buffer.data == NULL)
    {
        out_utf16_string->data = NULL;
        out_utf16_string->length = 0;
        return JSL_UNICODE_CONVERSION_OUT_OF_MEMORY;
    }
    else
    {
        JSLUTF16String writer = buffer;
        JSLUnicodeConversionResult res = jsl__convert_utf8_to_utf16le(utf8_string, &writer);

        out_utf16_string->data = buffer.data;
        out_utf16_string->length = writer.data - buffer.data;

        return res;
    }
}

#endif // JSL_UNICODE_IMPLEMENTATION

#endif // JSL_UNICODE

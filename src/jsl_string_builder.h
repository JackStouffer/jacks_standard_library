/**
 * # JSL String Builder
 * 
 * A string builder is a container for building large strings. It's specialized for
 * situations where many different smaller operations result in small strings being
 * coalesced into a final result, specifically using an arena as its allocator.
 *
 * While this is called string builder, the underlying data store is just bytes, so
 * any binary data which is built in chunks can use the string builder.
 *
 * ## Implementation
 *
 * A string builder is different from a normal dynamic array in two ways. One, it
 * has specific operations for writing string data in both fat pointer form but also
 * as a `snprintf` like operation. Two, the resulting string data is not stored as a
 * contiguous range of memory, but as a series of chunks which is given to the user
 * as an iterator when the string is finished.
 *
 * This is due to the nature of arena allocations. If you have some part of your
 * program which generates string output, the most common form of that code would be:
 *
 * 1. You do some operations, these operations themselves allocate
 * 2. You generate a string from the operations
 * 3. The string is concatenated into some buffer
 * 4. Repeat
 *
 * A dynamically sized array which grows would mean throwing away the old memory when
 * the array resizes. This would be fine for your typical heap but for an arena this
 * the old memory is unavailable until the arena is reset. A separate arena that's
 * used purely for the array would work, but that sort of defeats the whole purpose
 * of an arena, which is it's supposed to make lifetime tracking easier. Having a
 * whole bunch of separate arenas for different objects makes the program more
 * complicated than it should be.
 *
 * Having the memory in chunks means that a single arena is not wasteful with its
 * available memory.
 *
 * By default, each chunk is 256 bytes and is aligned to a 8 byte address. These are
 * tuneable parameters that you can set during init. The custom alignment helps if you
 * want to use SIMD code on the consuming code.
 * 
 * ## License
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

#pragma once
#include <stdint.h>
#include <stddef.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"

/* Versioning to catch mismatches across deps */
#ifndef JSL_STRING_BUILDER_VERSION
    #define JSL_STRING_BUILDER_VERSION 0x010000  /* 1.0.0 */
#else
    #if JSL_STRING_BUILDER_VERSION != 0x010200
        #error "jsl_string_builder.h version mismatch across includes"
    #endif
#endif

#ifndef JSL_STRING_BUILDER_DEF
    #define JSL_STRING_BUILDER_DEF
#endif

#ifdef __cplusplus
extern "C" {
#endif


struct JSL__StringBuilderChunk
{
    JSLFatPtr buffer;
    JSLFatPtr writer;
    struct JSL__StringBuilderChunk* next;
};

// private. use the fields directly at your own risk
struct JSL__StringBuilder
{
    // putting the sentinel first means it's much more likely to get
    // corrupted from accidental overwrites, therefore making it
    // more likely that memory bugs are caught.
    uint64_t sentinel;

    JSLAllocatorInterface* allocator;
    struct JSL__StringBuilderChunk* head;
    struct JSL__StringBuilderChunk* tail;
    int32_t alignment;
    int32_t chunk_size;
};

/**
 * Container type for the string builder. See the top level docstring for an overview.
 * 
 * This container holds a reference to the arena used, so it must have a lifetime greater
 * or equal to the string builder.
 *
 * ## Functions
 *
 * * jsl_string_builder_init
 * * jsl_string_builder_init2
 * * jsl_string_builder_insert_char
 * * jsl_string_builder_insert_uint8_t
 * * jsl_string_builder_insert_fatptr
 * * jsl_string_builder_format
 */
typedef struct JSL__StringBuilder JSLStringBuilder;

/**
 * The iterator type for a JSLStringBuilder instance. This keeps track of
 * where the iterator is over the course of calling the next function.
 *
 * @warning It is not valid to modify a string builder while iterating over it.
 *
 * Functions:
 *
 * * jsl_string_builder_iterator_init
 * * jsl_string_builder_iterator_next
 */
typedef struct JSLStringBuilderIterator
{
    struct JSL__StringBuilderChunk* current;
} JSLStringBuilderIterator;

/**
 * Initialize a JSLStringBuilder using the default settings. See the JSLStringBuilder
 * for more information on the container. A chunk is allocated right away and if that
 * fails this returns false.
 *
 * @param builder The builder instance to initialize; must not be NULL.
 * @param allocator The allocator that backs all allocations made by the builder; must not be NULL.
 * @return `true` if the builder was initialized successfully, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_init(JSLStringBuilder* builder, JSLAllocatorInterface* allocator);

/**
 * Initialize a JSLStringBuilder with a custom chunk size and chunk allocation alignment.
 * See the JSLStringBuilder for more information on the container. A chunk is allocated
 * right away and if that fails this returns false.
 *
 * @param builder The builder instance to initialize; must not be NULL.
 * @param allocator The allocator that backs all allocations made by the builder; must not be NULL.
 * @param chunk_size The number of bytes that are allocated each time the container needs to grow
 * @param alignment The allocation alignment of the chunks of data
 * @returns `true` if the builder was initialized successfully, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_init2(
    JSLStringBuilder* builder,
    JSLAllocatorInterface* allocator,
    int32_t chunk_size,
    int32_t chunk_alignment
);

/**
 * Add boolean to the string builder as an unsigned 8 bit int. Each append may result in an
 * allocation if there's no more space. If that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The bool to append.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_bool(JSLStringBuilder* builder, bool data);

/**
 * Append a single raw byte to the string builder.
 * The value is written as-is, so it can be used for arbitrary binary data, including
 * zero bytes. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The byte to append.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_i8(JSLStringBuilder* builder, int8_t data);

/**
 * Append a single raw byte to the end of the string builder.
 * The value is written as-is, so it can be used for arbitrary binary data, including
 * zero bytes. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The byte to append.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_u8(JSLStringBuilder* builder, uint8_t data);

/**
 * Append the binary, host ordered, representation of the signed 16 bit int to the 
 * string builder. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The number to write.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_i16(JSLStringBuilder* builder, int16_t data);

/**
 * Append the binary, host ordered, representation of the unsigned 16 bit int to the 
 * string builder. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The number to write.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_u16(JSLStringBuilder* builder, uint16_t data);

/**
 * Append the binary, host ordered, representation of the signed 32 bit int to the 
 * string builder. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The number to write.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_i32(JSLStringBuilder* builder, int32_t data);

/**
 * Append the binary, host ordered, representation of the unsigned 32 bit int to the 
 * string builder. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The number to write.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_u32(JSLStringBuilder* builder, uint32_t data);

/**
 * Append the binary, host ordered, representation of the signed 64 bit int to the 
 * string builder. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The number to write.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_i64(JSLStringBuilder* builder, int64_t data);

/**
 * Append the binary, host ordered, representation of the unsigned 64 bit int to the 
 * string builder. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The number to write.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_u64(JSLStringBuilder* builder, uint64_t data);

/**
 * Append the binary, host ordered, representation of the 32 bit floating point number to the 
 * string builder. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The number to write.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_f32(JSLStringBuilder* builder, float data);

/**
 * Append the binary, host ordered, representation of the 64 bit floating point number to the 
 * string builder. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data The number to write.
 * @returns `true` inserted was successful, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_f64(JSLStringBuilder* builder, double data);

/**
 * Append the contents of a fat pointer. Additional chunks are allocated as needed
 * while copying so if any of the allocations fail this returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data A fat pointer describing the bytes to copy; its length may be zero.
 * @returns `true` if the data was appended successfully, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_fatptr(JSLStringBuilder* builder, JSLFatPtr data);

/**
 * Append the contents of a null terminated C string. Additional chunks are allocated as needed
 * while copying so if any of the allocations fail this returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data A null terminated C string
 * @returns `true` if the data was appended successfully, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_cstr(JSLStringBuilder* builder, const char* data);

/**
 * Format a string using the jsl_format logic and write the result directly into
 * the string builder.
 *
 * @param builder The string builder that receives the formatted output; must be initialized.
 * @param fmt A fat pointer describing the format string.
 * @param ... Variadic arguments consumed by the formatter.
 * @returns `true` if formatting succeeded and the formatted bytes were appended, otherwise `false`.
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_format(JSLStringBuilder* builder, JSLFatPtr fmt, ...);

/**
 * TODO: docs
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_clear(
    JSLStringBuilder* builder
);

/**
 * Initialize an iterator instance so it will traverse the given string builder
 * from the begining. It's easiest to just put an empty iterator on the stack
 * and then call this function.
 *
 * ```
 * JSLStringBuilder builder = ...;
 *
 * JSLStringBuilderIterator iter;
 * jsl_string_builder_iterator_init(&builder, &iter);
 * ```
 *
 * @param builder    The string builder whose data will be traversed.
 * @param iterator   The iterator instance to initialize.
 */
JSL_STRING_BUILDER_DEF void jsl_string_builder_iterator_init(JSLStringBuilder* builder, JSLStringBuilderIterator* iterator);

/**
 * Get the next chunk of data a string builder iterator.
 *
 * This example program prints all the data in a string builder to stdout:
 *
 * ```
 * #include <stdio.h>
 *
 * JSLStringBuilderIterator iter;
 * jsl_string_builder_iterator_init(&builder, &iter);
 *
 * JSLFatPtr str;
 * while (jsl_string_builder_iterator_next(&iter. &str))
 * {
 *      jsl_format_to_c_file(stdout, str);
 * }
 * ```
 *
 * @param iterator The iterator instance
 * @returns If this iteration has data
 */
JSL_STRING_BUILDER_DEF bool jsl_string_builder_iterator_next(
    JSLStringBuilderIterator* iterator,
    JSLFatPtr* out_chunk
);

#ifdef __cplusplus
}
#endif

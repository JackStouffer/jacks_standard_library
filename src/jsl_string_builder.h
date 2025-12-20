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


#ifndef JSL_STRING_BUILDER_H_INCLUDED
    #define JSL_STRING_BUILDER_H_INCLUDED

    // type definition or macro only headers 
    #include <stdint.h>
    #include <stddef.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "jsl_core.h"

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
    typedef struct JSLStringBuilder
    {
        JSLArena* arena;
        struct JSLStringBuilderChunk* head;
        struct JSLStringBuilderChunk* tail;
        int32_t alignment;
        int32_t chunk_size;
    } JSLStringBuilder;

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
        struct JSLStringBuilderChunk* current;
    } JSLStringBuilderIterator;

    /**
     * Initialize a JSLStringBuilder using the default settings. See the JSLStringBuilder
     * for more information on the container. A chunk is allocated right away and if that
     * fails this returns false.
     *
     * @param builder The builder instance to initialize; must not be NULL.
     * @param arena The arena that backs all allocations made by the builder; must not be NULL.
     * @return `true` if the builder was initialized successfully, otherwise `false`.
     */
    JSL_STRING_BUILDER_DEF bool jsl_string_builder_init(JSLStringBuilder* builder, JSLArena* arena);

    /**
     * Initialize a JSLStringBuilder with a custom chunk size and chunk allocation alignment.
     * See the JSLStringBuilder for more information on the container. A chunk is allocated
     * right away and if that fails this returns false.
     *
     * @param builder The builder instance to initialize; must not be NULL.
     * @param arena The arena that backs all allocations made by the builder; must not be NULL.
     * @param chunk_size The number of bytes that are allocated each time the container needs to grow
     * @param alignment The allocation alignment of the chunks of data
     * @returns `true` if the builder was initialized successfully, otherwise `false`.
     */
    JSL_STRING_BUILDER_DEF bool jsl_string_builder_init2(JSLStringBuilder* builder, JSLArena* arena, int32_t chunk_size, int32_t alignment);

    /**
     * Append a char value to the end of the string builder without interpretation. Each append
     * may result in an allocation if there's no more space. If that allocation fails then this
     * function returns false.
     *
     * @param builder The string builder to append to; must be initialized.
     * @param c The byte to append.
     * @returns `true` if the byte was inserted successfully, otherwise `false`.
     */
    JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_char(JSLStringBuilder* builder, char c);

    /**
     * Append a single raw byte to the end of the string builder without interpretation.
     * The value is written as-is, so it can be used for arbitrary binary data, including
     * zero bytes. Each append may result in an allocation if there's no more space. If
     * that allocation fails then this function returns false.
     *
     * @param builder The string builder to append to; must be initialized.
     * @param c The byte to append.
     * @returns `true` if the byte was inserted successfully, otherwise `false`.
     */
    JSL_STRING_BUILDER_DEF bool jsl_string_builder_insert_uint8_t(JSLStringBuilder* builder, uint8_t c);

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
     * Get the next chunk of data a string builder iterator. The chunk will
     * have a `NULL` data pointer when iteration is over.
     *
     * This example program prints all the data in a string builder to stdout:
     *
     * ```
     * #include <stdio.h>
     *
     * JSLStringBuilder builder = ...;
     *
     * JSLStringBuilderIterator iter;
     * jsl_string_builder_iterator_init(&builder, &iter);
     *
     * while (true)
     * {
     *      JSLFatPtr str = jsl_string_builder_iterator_next(&iter);
     *
     *      if (str.data == NULL)
     *          break;
     *
     *      jsl_format_file(stdout, str);
     * }
     * ```
     *
     * @param iterator   The iterator instance
     * @returns The next chunk of data from the string builder
     */
    JSL_STRING_BUILDER_DEF JSLFatPtr jsl_string_builder_iterator_next(JSLStringBuilderIterator* iterator);
    
    #ifdef __cplusplus
    }
    #endif
    
#endif /* JSL_STRING_BUILDER_H_INCLUDED */

#ifdef JSL_STRING_BUILDER_IMPLEMENTATION

    struct JSLStringBuilderChunk
    {
        JSLFatPtr buffer;
        JSLFatPtr writer;
        struct JSLStringBuilderChunk* next;
    };

    static bool jsl__string_builder_add_chunk(JSLStringBuilder* builder)
    {
        struct JSLStringBuilderChunk* chunk = JSL_ARENA_TYPED_ALLOCATE(
            struct JSLStringBuilderChunk,
            builder->arena
        );
        chunk->next = NULL;
        chunk->buffer = jsl_arena_allocate_aligned(
            builder->arena, builder->chunk_size,
            builder->alignment,
            false
        );

        if (chunk->buffer.data != NULL)
        {
            chunk->writer = chunk->buffer;

            if (builder->head == NULL)
                builder->head = chunk;

            if (builder->tail == NULL)
            {
                builder->tail = chunk;
            }
            else
            {
                builder->tail->next = chunk;
                builder->tail = chunk;
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    bool jsl_string_builder_init(JSLStringBuilder* builder, JSLArena* arena)
    {
        return jsl_string_builder_init2(
            builder,
            arena,
            256,
            8
        );
    }

    bool jsl_string_builder_init2(
        JSLStringBuilder* builder,
        JSLArena* arena,
        int32_t chunk_size,
        int32_t alignment
    )
    {
        bool res = false;

        if (builder != NULL)
        {
            JSL_MEMSET(builder, 0, sizeof(JSLStringBuilder));

            if (arena != NULL && chunk_size > 0 && alignment > 0)
            {
                builder->arena = arena;
                builder->chunk_size = chunk_size;
                builder->alignment = alignment;
                res = jsl__string_builder_add_chunk(builder);
            }
        }

        return res;
    }

    bool jsl_string_builder_insert_char(JSLStringBuilder* builder, char c)
    {
        if (builder == NULL || builder->head == NULL || builder->tail == NULL)
            return false;

        bool res = false;

        bool needs_alloc = false;
        if (builder->tail->writer.length > 0)
        {
            builder->tail->writer.data[0] = (uint8_t) c;
            JSL_FATPTR_ADVANCE(builder->tail->writer, 1);
            res = true;
        }
        else
        {
            needs_alloc = true;
        }

        bool has_new_chunk = false;
        if (needs_alloc)
        {
            has_new_chunk = jsl__string_builder_add_chunk(builder);
        }

        if (has_new_chunk)
        {
            builder->tail->writer.data[0] = (uint8_t) c;
            JSL_FATPTR_ADVANCE(builder->tail->writer, 1);
            res = true;
        }

        return res;
    }

    bool jsl_string_builder_insert_uint8_t(JSLStringBuilder* builder, uint8_t c)
    {
        if (builder == NULL || builder->head == NULL || builder->tail == NULL)
            return false;

        bool res = false;

        bool needs_alloc = false;
        if (builder->tail->writer.length > 0)
        {
            builder->tail->writer.data[0] = c;
            JSL_FATPTR_ADVANCE(builder->tail->writer, 1);
            res = true;
        }
        else
        {
            needs_alloc = true;
        }

        bool has_new_chunk = false;
        if (needs_alloc)
        {
            has_new_chunk = jsl__string_builder_add_chunk(builder);
        }

        if (has_new_chunk)
        {
            builder->tail->writer.data[0] = c;
            JSL_FATPTR_ADVANCE(builder->tail->writer, 1);
            res = true;
        }

        return res;
    }

    bool jsl_string_builder_insert_fatptr(JSLStringBuilder* builder, JSLFatPtr data)
    {
        if (builder == NULL || builder->head == NULL || builder->tail == NULL)
            return false;

        bool res = true;

        while (data.length > 0)
        {
            if (builder->tail->writer.length == 0)
            {
                res = jsl__string_builder_add_chunk(builder);
                if (!res)
                    break;
            }

            int64_t bytes_written = jsl_fatptr_memory_copy(&builder->tail->writer, data);
            JSL_FATPTR_ADVANCE(data, bytes_written);
        }

        return res;
    }

    bool jsl_string_builder_insert_cstr(JSLStringBuilder* builder, const char* data)
    {
        if (
            builder == NULL
            || builder->head == NULL
            || builder->tail == NULL
            || data == NULL
        )
            return false;

        bool res = true;

        size_t bytes_remaining = JSL_STRLEN(data);

        while (bytes_remaining > 0)
        {
            if (builder->tail->writer.length == 0)
            {
                res = jsl__string_builder_add_chunk(builder);
                if (!res)
                    break;
            }

            int64_t bytes_written = jsl_fatptr_cstr_memory_copy(
                &builder->tail->writer,
                data,
                false
            );
            bytes_remaining -= bytes_written;
            data += bytes_written;
        }

        return res;
    }

    void jsl_string_builder_iterator_init(JSLStringBuilder* builder, JSLStringBuilderIterator* iterator)
    {
        if (builder != NULL && iterator != NULL)
            iterator->current = builder->head;
    }

    JSLFatPtr jsl_string_builder_iterator_next(JSLStringBuilderIterator* iterator)
    {
        JSLFatPtr ret = {0};
        if (iterator == NULL)
            return ret;

        struct JSLStringBuilderChunk* current = iterator->current;
        if (current == NULL || current->buffer.data == NULL)
            return ret;

        iterator->current = current->next;
        ret = jsl_fatptr_auto_slice(current->buffer, current->writer);
        return ret;
    }

    struct JSL__StringBuilderContext
    {
        JSLStringBuilder* builder;
        bool failure_flag;
        uint8_t buffer[JSL_FORMAT_MIN_BUFFER];
    };

    static uint8_t* format_string_builder_callback(uint8_t *buf, void *user, int64_t len)
    {
        struct JSL__StringBuilderContext* context = (struct JSL__StringBuilderContext*) user;

        if (context->builder->head == NULL || context->builder->tail == NULL || len > JSL_FORMAT_MIN_BUFFER)
            return NULL;

        bool res = jsl_string_builder_insert_fatptr(context->builder, jsl_fatptr_init(buf, len));

        if (res)
        {
            return context->buffer;
        }
        else
        {
            context->failure_flag = true;
            return NULL;
        }
    }

    JSL__ASAN_OFF bool jsl_string_builder_format(JSLStringBuilder* builder, JSLFatPtr fmt, ...)
    {
        if (builder == NULL || builder->head == NULL || builder->tail == NULL)
            return false;

        va_list va;
        va_start(va, fmt);

        struct JSL__StringBuilderContext context;
        context.builder = builder;
        context.failure_flag = false;

        jsl_format_callback(
            format_string_builder_callback,
            &context,
            context.buffer,
            fmt,
            va
        );

        va_end(va);

        return !context.failure_flag;
    }

#endif /* JSL_STRING_BUILDER_IMPLEMENTATION */

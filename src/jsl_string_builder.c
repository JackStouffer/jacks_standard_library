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

#include <stdint.h>
#include <stddef.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"
#include "jsl_string_builder.h"

#define JSL__BUILDER_PRIVATE_SENTINEL 4401537694999363085U

static bool jsl__string_builder_add_chunk(JSLStringBuilder* builder)
{
    struct JSL__StringBuilderChunk* chunk = JSL_TYPED_ALLOCATE(
        struct JSL__StringBuilderChunk,
        builder->allocator
    );
    chunk->next = NULL;
    chunk->buffer.data = jsl_allocator_interface_alloc(
        builder->allocator,
        builder->chunk_size,
        builder->chunk_alignment,
        false
    );
    chunk->buffer.length = builder->chunk_size;

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

bool jsl_string_builder_init(JSLStringBuilder* builder, JSLAllocatorInterface* alocator)
{
    return jsl_string_builder_init2(
        builder,
        alocator,
        1024,
        8
    );
}

bool jsl_string_builder_init2(
    JSLStringBuilder* builder,
    JSLAllocatorInterface* allocator,
    int32_t chunk_size,
    int32_t chunk_alignment
)
{
    bool res = false;

    if (builder != NULL)
    {
        JSL_MEMSET(builder, 0, sizeof(JSLStringBuilder));
    }
    
    if (
        builder != NULL
        && allocator != NULL
        && chunk_size > 0
        && chunk_alignment > 0
    )
    {
        builder->sentinel = JSL__BUILDER_PRIVATE_SENTINEL;
        builder->allocator = allocator;
        builder->chunk_size = chunk_size;
        builder->chunk_alignment = chunk_alignment;
        res = jsl__string_builder_add_chunk(builder);
    }

    return res;
}

int64_t jsl_string_builder_insert_fatptr(JSLStringBuilder* builder, JSLFatPtr data)
{
    if (
        builder == NULL
        || builder->sentinel != JSL__BUILDER_PRIVATE_SENTINEL
        || builder->head == NULL
        || builder->tail == NULL
    )
        return -1;

    int64_t bytes_written = 0;

    while (data.length > 0)
    {
        if (builder->tail->writer.length == 0 && !jsl__string_builder_add_chunk(builder))
        {
            break;
        }

        int64_t copy_res = jsl_fatptr_memory_copy(&builder->tail->writer, data);
        
        if (copy_res > 0)
        {
            JSL_FATPTR_ADVANCE(data, copy_res);
            bytes_written += copy_res;
        }
        else
        {
            break;
        }
    }

    return bytes_written;
}

void jsl_string_builder_iterator_init(JSLStringBuilder* builder, JSLStringBuilderIterator* iterator)
{
    if (iterator != NULL)
        JSL_MEMSET(iterator, 0, sizeof(JSLStringBuilderIterator));

    if (
        builder != NULL
        && iterator != NULL
        && builder->sentinel == JSL__BUILDER_PRIVATE_SENTINEL
    )
        iterator->current = builder->head;
}

bool jsl_string_builder_iterator_next(JSLStringBuilderIterator* iterator, JSLFatPtr* out_chunk)
{
    if (iterator == NULL || out_chunk == NULL)
        return false;

    *out_chunk = (JSLFatPtr){0};

    struct JSL__StringBuilderChunk* current = iterator->current;
    if (current == NULL || current->buffer.data == NULL)
        return false;

    iterator->current = current->next;
    *out_chunk = jsl_fatptr_auto_slice(current->buffer, current->writer);
    return true;
}

static int64_t jsl__format_string_builder_callback(void* user, JSLFatPtr data)
{
    return jsl_string_builder_insert_fatptr((JSLStringBuilder*) user, data);
}

JSLOutputSink jsl_string_builder_output_sink(JSLStringBuilder* builder)
{
    JSLOutputSink sink;
    sink.write_fp = jsl__format_string_builder_callback;
    sink.user_data = builder;
    return sink;
}

void jsl_string_builder_free(
    JSLStringBuilder* builder
)
{
    if (
        builder == NULL
        || builder->sentinel != JSL__BUILDER_PRIVATE_SENTINEL
    )
        return;

    struct JSL__StringBuilderChunk* current = builder->head;
    while (current != NULL)
    {
        struct JSL__StringBuilderChunk* next = current->next;
        if (current->buffer.data != NULL)
            jsl_allocator_interface_free(builder->allocator, current->buffer.data);

        jsl_allocator_interface_free(builder->allocator, current);
        current = next;
    }

    builder->sentinel = 0;
}

#undef JSL__BUILDER_PRIVATE_SENTINEL

/**
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
#include "jsl/hash_map_common.h"

#include <stdint.h>
#include <stddef.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl/core.h"
#include "jsl/allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Dynamic array of uint8_t.
 *
 * Example:
 *
 * ```
 * JSLStringBuilder array;
 * jsl_string_builder_init(&array, &arena);
 *
 * jsl_string_builder_insert(&array, ... );
 *
 * for (int64_t i = 0; i < array.length; ++i)
 * {
 *      uint8_t* value = &array.data[i];
 *      ...
 * }
 * ```
 *
 * ## Functions
 *
 *  * jsl_string_builder_init
 *  * jsl_string_builder_insert
 *  * jsl_string_builder_insert_at
 *  * jsl_string_builder_delete_at
 *  * jsl_string_builder_clear
 *
 */
typedef struct JSLStringBuilder {
    // putting the sentinel first means it's much more likely to get
    // corrupted from accidental overwrites, therefore making it
    // more likely that memory bugs are caught.
    uint64_t sentinel;
    uint8_t* data;
    int64_t length;
    int64_t capacity;

    JSLAllocatorInterface allocator;
} JSLStringBuilder;

/**
 * Initialize an instance of JSLStringBuilder. Enough room will be allocated
 * for `initial_capacity` elements.
 *
 * @param array The pointer to the array instance to initialize
 * @param arena The arena that this array will use to allocate memory
 * @param initial_capacity Allocate enough space to hold this many elements
 * @returns If the allocation succeed
 */
bool jsl_string_builder_init(
    JSLStringBuilder* builder,
    JSLAllocatorInterface allocator,
    int64_t initial_capacity
);

/// TODO: docs
JSLImmutableMemory jsl_string_builder_get_string(
    JSLStringBuilder* builder
);

/**
 * Insert an `uint8_t` at the end of the array.
 *
 * @param array The pointer to the array
 * @param value The value to add
 */
bool jsl_string_builder_append(
    JSLStringBuilder* builder,
    JSLImmutableMemory str_data
);

/**
 * Delete the elements starting at the specified index, moving everything after
 * that range down into the empty space
 *
 * @param builder The pointer to the builder
 * @param index The index to start deleting at
 * @param count The number of items to delete
 * @returns if deletion succeed
 */
bool jsl_string_builder_delete(
    JSLStringBuilder* builder,
    int64_t index,
    int64_t count
);

/**
 * Set the length of the array back to zero. Does not shrink the underlying capacity.
 *
 * @param array The pointer to the array
 */
void jsl_string_builder_clear(
    JSLStringBuilder* builder
);

JSLOutputSink jsl_string_builder_output_sink(
    JSLStringBuilder* builder
);

/**
 * Free the underlying memory of the array. This sets the array into an invalid state.
 * You will have to call init again if you wish to use this array instance.
 *
 * @param array The pointer to the array
 */
void jsl_string_builder_free(
    JSLStringBuilder* builder
);

#ifdef __cplusplus
}
#endif

/**
 * # JSL String to String Multimap
 * 
 * This file is a single header file library that implements a multimap data
 * structure, which maps length based string keys to multiple length based
 * string values, and is optimized around the arena allocator design.
 * This file is part of the Jack's Standard Library project.
 * 
 * ## Documentation
 * 
 * See `docs/jsl_str_to_str_multimap.md` for a formatted documentation page.
 * 
 * ## Caveats
 * 
 * This multimap uses arenas, so some wasted memory is indeveatble. Care has
 * been taken to reuse as much allocated memory as possible. But if your
 * multimap is long lived it's possible to start exhausting the arena with
 * old memory.
 * 
 * Remember to
 * 
 * * have an initial item count guess as accurate as you can to reduce rehashes
 * * have the arena have as short a lifetime as possible
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
#include "jsl_hash_map_common.h"
#include "jsl_allocator.h"

/* Versioning to catch mismatches across deps */
#ifndef JSL_STR_TO_STR_MULTIMAP_VERSION
    #define JSL_STR_TO_STR_MULTIMAP_VERSION 0x010000  /* 1.0.0 */
#else
    #if JSL_STR_TO_STR_MULTIMAP_VERSION != 0x010000
        #error "jsl_str_to_str_multimap.h version mismatch across includes"
    #endif
#endif

#ifndef JSL_STR_TO_STR_MULTIMAP_DEF
    #define JSL_STR_TO_STR_MULTIMAP_DEF
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum JSLStrToStrMultimapKeyState {
    JSL__MULTIMAP_EMPTY = 1UL,
    JSL__MULTIMAP_TOMBSTONE = 2UL
};

#define JSL__MULTIMAP_KEY_SSO_LENGTH 24
#define JSL__MULTIMAP_VALUE_SSO_LENGTH 32

struct JSL__StrToStrMultimapValue
{
    union
    {
        struct
        {
            int64_t sso_len;
            uint8_t small_string_buffer[JSL__MULTIMAP_VALUE_SSO_LENGTH];
        };
        JSLFatPtr value;
    };
    
    struct JSL__StrToStrMultimapValue* next;
    uint8_t value_state;
};

struct JSL__StrToStrMultimapEntry {
    union
    {
        struct
        {
            /// @brief small string optimization buffer to hold the key if len < JSL__MULTIMAP_SSO_LENGTH
            uint8_t small_string_buffer[JSL__MULTIMAP_KEY_SSO_LENGTH];
            int64_t sso_len;
        };
        // TODO: docs
        JSLFatPtr key;
        /// @brief Used to store in the free list, ignored otherwise
        struct JSL__StrToStrMultimapEntry* next;
    };
    
    // TODO: docs
    uint64_t hash;

    // TODO: docs
    struct JSL__StrToStrMultimapValue* values_head;

    int64_t value_count;
    uint8_t key_state;
};

struct JSL__StrToStrMultimapKeyValueIter {
    struct JSL__StrToStrMultimap* map;
    struct JSL__StrToStrMultimapEntry* current_entry;
    struct JSL__StrToStrMultimapValue* current_value;
    int64_t current_lut_index;
    int64_t generational_id;
    uint64_t sentinel;
};

struct JSL__StrToStrMultimapValueIter {
    struct JSL__StrToStrMultimap* map;
    struct JSL__StrToStrMultimapEntry* entry;
    struct JSL__StrToStrMultimapValue* current_value;
    int64_t generational_id;
    uint64_t sentinel;
};

struct JSL__StrToStrMultimap {
    // putting the sentinel first means it's much more likely to get
    // corrupted from accidental overwrites, therefore making it
    // more likely that memory bugs are caught.
    uint64_t sentinel;

    JSLAllocatorInterface* allocator;

    uintptr_t* entry_lookup_table;
    int64_t entry_lookup_table_length;

    int64_t key_count;
    int64_t value_count;
    int64_t tombstone_count;

    struct JSL__StrToStrMultimapEntry* entry_free_list;
    struct JSL__StrToStrMultimapValue* value_free_list;

    uint64_t hash_seed;
    float load_factor;
    int32_t generational_id;
};

/**
 * This is an open addressed, hash based multimap with linear probing that maps
 * JSLFatPtr keys to multiple JSLFatPtr values.
 * 
 * Example:
 *
 * ```
 * uint8_t buffer[JSL_KILOBYTES(16)];
 * JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 *
 * JSLStrToStrMultimap map;
 * jsl_str_to_str_multimap_init(&map, &stack_arena, 0);
 *
 * JSLFatPtr key = JSL_FATPTR_INITIALIZER("hello-key");
 * 
 * jsl_str_to_str_multimap_insert(
 *     &map,
 *     key,
 *     JSL_STRING_LIFETIME_LONGER,
 *     JSL_FATPTR_EXPRESSION("hello-value"),
 *     JSL_STRING_LIFETIME_LONGER
 * );
 *
 * jsl_str_to_str_multimap_insert(
 *     &map,
 *     key,
 *     JSL_STRING_LIFETIME_LONGER,
 *     JSL_FATPTR_EXPRESSION("hello-value2"),
 *     JSL_STRING_LIFETIME_LONGER
 * );
 * 
 * jsl_str_to_str_multimap_get_value_count_for_key(&map, key); // 2
 * ```
 * 
 * ## Functions
 *
 * * jsl_str_to_str_multimap_init
 * * jsl_str_to_str_multimap_init2
 * * jsl_str_to_str_multimap_get_key_count
 * * jsl_str_to_str_multimap_get_value_count
 * * jsl_str_to_str_multimap_has_key
 * * jsl_str_to_str_multimap_insert
 * * jsl_str_to_str_multimap_get_value_count_for_key
 * * jsl_str_to_str_multimap_key_value_iterator_init
 * * jsl_str_to_str_multimap_key_value_iterator_next
 * * jsl_str_to_str_multimap_get_values_for_key_iterator_init
 * * jsl_str_to_str_multimap_get_values_for_key_iterator_next
 * * jsl_str_to_str_multimap_delete_key
 * * jsl_str_to_str_multimap_delete_value
 * * jsl_str_to_str_multimap_clear
 */
typedef struct JSL__StrToStrMultimap JSLStrToStrMultimap;

/**
 * State tracking struct for iterating over all of the keys and values
 * in the map.
 * 
 * @note If you mutate the map this iterator is automatically invalidated
 * and any operations on this iterator will terminate with failure return
 * values.
 * 
 * ## Functions
 *
 * * jsl_str_to_str_multimap_key_value_iterator_init
 * * jsl_str_to_str_multimap_key_value_iterator_next
 */
typedef struct JSL__StrToStrMultimapKeyValueIter JSLStrToStrMultimapKeyValueIter;

/**
 * State tracking struct for iterating over all of the values for a given
 * key.
 * 
 * @note If you mutate the map this iterator is automatically invalidated
 * and any operations on this iterator will terminate with failure return
 * values.
 * 
 * ## Functions
 *
 * * jsl_str_to_str_multimap_get_values_for_key_iterator_init
 * * jsl_str_to_str_multimap_get_values_for_key_iterator_next
 */
typedef struct JSL__StrToStrMultimapValueIter JSLStrToStrMultimapValueIter;

/**
 * Initialize a multimap with default sizing parameters.
 *
 * This sets up internal tables in the provided arena, using a 32 entry
 * initial capacity guess and a 0.75 load factor. The `seed` value is to
 * protect against hash flooding attacks. If you're absolutely sure this
 * map cannot be attacked, then zero is valid seed value.
 *
 * @param map Pointer to the multimap to initialize.
 * @param arena Arena used for all allocations.
 * @param seed Arbitrary seed value for hashing.
 * @return `true` on success, `false` if any parameter is invalid or out of memory.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_init(
    JSLStrToStrMultimap* map,
    JSLAllocatorInterface* allocator,
    uint64_t seed
);

/**
 * Initialize a multimap with explicit sizing parameters.
 *
 * This is identical to `jsl_str_to_str_multimap_init`, but lets callers
 * provide an initial `item_count_guess` and a `load_factor`. The initial
 * lookup table is sized to the next power of two above `item_count_guess`,
 * clamped to at least 32 entries. `load_factor` must be in the range
 * `(0.0f, 1.0f)` and controls when the table rehashes. The `seed` value
 * is to protect against hash flooding attacks. If you're absolutely sure 
 * this map cannot be attacked, then zero is valid seed value
 *
 * @param map Pointer to the multimap to initialize.
 * @param arena Arena used for all allocations.
 * @param seed Arbitrary seed value for hashing.
 * @param item_count_guess Expected max number of keys
 * @param load_factor Desired load factor before rehashing
 * @return `true` on success, `false` if any parameter is invalid or out of memory.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_init2(
    JSLStrToStrMultimap* map,
    JSLAllocatorInterface* allocator,
    uint64_t seed,
    int64_t item_count_guess,
    float load_factor
);

/**
 * Get the number of distinct keys currently stored.
 *
 * @param map Pointer to the multimap.
 * @return Key count, or `-1` on error
 */
JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_key_count(
    JSLStrToStrMultimap* map
);

/**
 * Get the number of values currently stored.
 *
 * @param map Pointer to the multimap.
 * @return Key count, or `-1` on error
 */
JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count(
    JSLStrToStrMultimap* map
);

/**
 * Does the map have the given key.
 *
 * @param map Pointer to the multimap.
 * @return `true` if yes, `false` if no or error
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_has_key(
    JSLStrToStrMultimap* map,
    JSLFatPtr key
);

/**
 * Insert a key/value pair.
 *
 * If the key already exists, the value is appended to that key's value list;
 * otherwise a new key entry is created. Values are not deduplicated. The
 * `*_lifetime` parameters control whether the data is referenced directly
 * (`JSL_STRING_LIFETIME_LONGER`) or copied into the map
 * (`JSL_STRING_LIFETIME_SHORTER`). Use the transient lifetime if the string's
 * lifetime is less than that of the map.
 *
 * @param map Multimap to mutate.
 * @param key Key to insert.
 * @param key_lifetime Lifetime semantics for the key data.
 * @param value Value to insert.
 * @param value_lifetime Lifetime semantics for the value data.
 * @return `true` on success, `false` on invalid parameters or OOM.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_insert(
    JSLStrToStrMultimap* map,
    JSLFatPtr key,
    JSLStringLifeTime key_lifetime,
    JSLFatPtr value,
    JSLStringLifeTime value_lifetime
);

/**
 * Get the number of values for the given key.
 *
 * @param map Pointer to the multimap.
 * @param key Key to check.
 * @return Key count, or `-1` on error
 */
JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count_for_key(
    JSLStrToStrMultimap* map,
    JSLFatPtr key
);

/**
 * Initialize an iterator that visits every key/value pair in the multimap.
 * 
 * Example:
 *
 * ```
 * JSLStrToStrMultimapKeyValueIter iter;
 * jsl_str_to_str_multimap_key_value_iterator_init(
 *     &map, &iter
 * );
 * 
 * JSLFatPtr key;
 * JSLFatPtr value;
 * while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &key, &value))
 * {
 *    ...
 * }
 * ```
 *
 * Values for a key are returned before moving on to the next occupied slot,
 * and the overall traversal order is undefined. The iterator is invalidated
 * if the map is mutated after initialization.
 *
 * @param map Multimap to iterate over; must be initialized.
 * @param iterator Iterator instance to initialize.
 * @return `true` on success, `false` if parameters are invalid.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_key_value_iterator_init(
    JSLStrToStrMultimap* map,
    JSLStrToStrMultimapKeyValueIter* iterator
);

/**
 * Advance the key/value iterator.
 * 
 * Example:
 *
 * ```
 * JSLStrToStrMultimapKeyValueIter iter;
 * jsl_str_to_str_multimap_key_value_iterator_init(
 *     &map, &iter
 * );
 * 
 * JSLFatPtr key;
 * JSLFatPtr value;
 * while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &key, &value))
 * {
 *    ...
 * }
 * ```
 *
 * Returns the next key/value pair for the multimap, visiting all values for
 * a key before moving on. The iterator must be initialized and is
 * invalidated if the map is mutated; iteration order is undefined.
 *
 * @param iterator Iterator to advance.
 * @param out_key Output for the current key.
 * @param out_value Output for the current value.
 * @return `true` if a pair was produced, `false` if exhausted or invalid.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_key_value_iterator_next(
    JSLStrToStrMultimapKeyValueIter* iterator,
    JSLFatPtr* out_key,
    JSLFatPtr* out_value
);

/**
 * Initialize an iterator over all values associated with a key.
 * 
 * Example:
 *
 * ```
 * JSLStrToStrMultimapValueIter iter;
 * jsl_str_to_str_multimap_get_values_for_key_iterator_init(
 *     &map, &iter
 * );
 * 
 * JSLFatPtr value;
 * while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &value))
 * {
 *    ...
 * }
 * ```
 *
 * The iterator is valid only while the map remains unchanged. If the key
 * is absent or has no values, initialization still succeeds but the first
 * call to `jsl_str_to_str_multimap_get_values_for_key_iterator_next` will
 * immediately return `false`.
 *
 * @param map Multimap to read from; must be initialized.
 * @param iterator Iterator instance to initialize.
 * @param key Key whose values should be iterated.
 * @return `true` on success, `false` if parameters are invalid.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_get_values_for_key_iterator_init(
    JSLStrToStrMultimap* map,
    JSLStrToStrMultimapValueIter* iterator,
    JSLFatPtr key
);

/**
 * Advance a value iterator for a single key.
 * 
 * Example:
 *
 * ```
 * JSLStrToStrMultimapValueIter iter;
 * jsl_str_to_str_multimap_get_values_for_key_iterator_init(
 *     &map, &iter
 * );
 * 
 * JSLFatPtr value;
 * while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &value))
 * {
 *    ...
 * }
 * ```
 *
 * Returns the next value for the key supplied to
 * `jsl_str_to_str_multimap_get_values_for_key_iterator_init`. The iterator
 * must be initialized and becomes invalid if the map is mutated; iteration
 * order is undefined. When the values are exhausted or the iterator is
 * invalid, the function returns `false`.
 *
 * @param iterator Iterator to advance; must be initialized.
 * @param out_value Output for the current value.
 * @return `true` if a value was produced, `false` otherwise.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_get_values_for_key_iterator_next(
    JSLStrToStrMultimapValueIter* iterator,
    JSLFatPtr* out_value
);

/**
 * Remove a key and all of its values.
 *
 * Iterators become invalid. If the key is not present or parameters are invalid,
 * the map is unchanged and `false` is returned.
 *
 * @param map Multimap to mutate.
 * @param key Key to remove.
 * @return `true` if the key existed and was removed, `false` otherwise.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_key(
    JSLStrToStrMultimap* map,
    JSLFatPtr key
);

/**
 * Remove a single value for the given key.
 *
 * If the value is found, it is removed from the key's list and recycled
 * into the value free list. When the last value is removed, the key entry
 * itself is recycled. No action is taken if the key or value is missing
 * or parameters are invalid.
 *
 * @param map Multimap to mutate.
 * @param key Key whose value should be removed.
 * @param value Exact value to remove.
 * @return `true` if a value was removed, `false` otherwise.
 */
JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_value(
    JSLStrToStrMultimap* map,
    JSLFatPtr key,
    JSLFatPtr value
);

/**
 * Remove all keys and values from the map. Iterators become invalid.
 *
 * @param map Multimap to clear.
 */
JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_clear(
    JSLStrToStrMultimap* map
);

#ifdef __cplusplus
}
#endif

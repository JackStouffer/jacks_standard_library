/**
 * # JSL String to String Map
 * 
 * This file is a single header file library that implements a hash map data
 * structure, which maps length based string keys to length based string values,
 * and is optimized around the arena allocator design. This file is part of
 * the Jack's Standard Library project.
 * 
 * ## Documentation
 * 
 * See `docs/jsl_str_to_str_map.md` for a formatted documentation page.
 *
 * ## Caveats
 * 
 * This map uses arenas, so some wasted memory is indeveatble. Care has
 * been taken to reuse as much allocated memory as possible. But if your
 * map is long lived it's possible to start exhausting the arena with
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
#ifndef JSL_STR_TO_STR_MAP_VERSION
    #define JSL_STR_TO_STR_MAP_VERSION 0x010000  /* 1.0.0 */
#else
    #if JSL_STR_TO_STR_MAP_VERSION != 0x010000
        #error "jsl_str_to_str_map.h version mismatch across includes"
    #endif
#endif

#ifndef JSL_STR_TO_STR_MAP_DEF
    #define JSL_STR_TO_STR_MAP_DEF
#endif

#ifdef __cplusplus
extern "C" {
#endif


enum JSLStrToStrMapKeyState {
    JSL__MAP_EMPTY = 0UL,
    JSL__MAP_TOMBSTONE = 1UL
};

#define JSL__MAP_SSO_LENGTH 8

struct JSL__StrToStrMapEntry
{
    union
    {
        struct
        {
            uint8_t key_sso_buffer[JSL__MAP_SSO_LENGTH];
            int64_t key_sso_buffer_length;
        };
        JSLFatPtr key;
        /// @brief Used to store in the free list, ignored otherwise
        struct JSL__StrToStrMapEntry* next;
    };

    union
    {
        struct
        {
            uint8_t value_sso_buffer[JSL__MAP_SSO_LENGTH];
            int64_t value_sso_buffer_length;
        };
        JSLFatPtr value;
    };
    
    uint64_t hash;

    uint8_t key_lifetime;
    uint8_t value_lifetime;
};

const int a = sizeof(struct JSL__StrToStrMapEntry);

struct JSL__StrToStrMapKeyValueIter {
    struct JSL__StrToStrMap* map;
    int64_t current_lut_index;
    int64_t generational_id;
    uint64_t sentinel;
};

struct JSL__StrToStrMap {
    // putting the sentinel first means it's much more likely to get
    // corrupted from accidental overwrites, therefore making it
    // more likely that memory bugs are caught.
    uint64_t sentinel;

    JSLAllocatorInterface* allocator;

    uintptr_t* entry_lookup_table;
    int64_t entry_lookup_table_length;

    int64_t item_count;
    int64_t tombstone_count;

    struct JSL__StrToStrMapEntry* entry_free_list;

    uint64_t hash_seed;
    float load_factor;
    int32_t generational_id;
};

/**
 * This is an open addressed hash map with linear probing that maps
 * JSLFatPtr keys to JSLFatPtr values. This map uses rapidhash, which
 * is a avalanche hash with a configurable seed value for protection
 * against hash flooding attacks.
 * 
 * Example:
 *
 * ```
 * uint8_t buffer[JSL_KILOBYTES(16)];
 * JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 *
 * JSLStrToStrMap map;
 * jsl_str_to_str_map_init(&map, &stack_arena, 0);
 *
 * JSLFatPtr key = JSL_FATPTR_INITIALIZER("hello-key");
 * 
 * jsl_str_to_str_multimap_insert(
 *     &map,
 *     key,
 *     JSL_STRING_LIFETIME_STATIC,
 *     JSL_FATPTR_EXPRESSION("hello-value"),
 *     JSL_STRING_LIFETIME_STATIC
 * );
 * 
 * JSLFatPtr value
 * jsl_str_to_str_map_get(&map, key, &value);
 * ```
 * 
 * ## Functions
 *
 *  * jsl_str_to_str_map_init
 *  * jsl_str_to_str_map_init2
 *  * jsl_str_to_str_map_item_count
 *  * jsl_str_to_str_map_has_key
 *  * jsl_str_to_str_map_insert
 *  * jsl_str_to_str_map_get
 *  * jsl_str_to_str_map_key_value_iterator_init
 *  * jsl_str_to_str_map_key_value_iterator_next
 *  * jsl_str_to_str_map_delete
 *  * jsl_str_to_str_map_clear
 *
 */
typedef struct JSL__StrToStrMap JSLStrToStrMap;

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
 *  * jsl_str_to_str_map_key_value_iterator_init
 *  * jsl_str_to_str_map_key_value_iterator_next
 */
typedef struct JSL__StrToStrMapKeyValueIter JSLStrToStrMapKeyValueIter;

/**
 * Initialize a map with default sizing parameters.
 *
 * This sets up internal tables in the provided arena, using a 32 entry
 * initial capacity guess and a 0.75 load factor. The `seed` value is to
 * protect against hash flooding attacks. If you're absolutely sure this
 * map cannot be attacked, then zero is valid seed value.
 *
 * @param map Pointer to the map to initialize.
 * @param arena Arena used for all allocations.
 * @param seed Arbitrary seed value for hashing.
 * @return `true` on success, `false` if any parameter is invalid or out of memory.
 */
JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_init(
    JSLStrToStrMap* map,
    JSLAllocatorInterface* allocator,
    uint64_t seed
);

/**
 * Initialize a map with explicit sizing parameters.
 *
 * This is identical to `jsl_str_to_str_map_init`, but lets callers
 * provide an initial `item_count_guess` and a `load_factor`. The initial
 * lookup table is sized to the next power of two above `item_count_guess`,
 * clamped to at least 32 entries. `load_factor` must be in the range
 * `(0.0f, 1.0f)` and controls when the table rehashes. The `seed` value
 * is to protect against hash flooding attacks. If you're absolutely sure 
 * this map cannot be attacked, then zero is valid seed value
 *
 * @param map Pointer to the map to initialize.
 * @param arena Arena used for all allocations.
 * @param seed Arbitrary seed value for hashing.
 * @param item_count_guess Expected max number of keys
 * @param load_factor Desired load factor before rehashing
 * @return `true` on success, `false` if any parameter is invalid or out of memory.
 */
JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_init2(
    JSLStrToStrMap* map,
    JSLAllocatorInterface* allocator,
    uint64_t seed,
    int64_t item_count_guess,
    float load_factor
);

/**
 * Get the number of items currently stored.
 *
 * @param map Pointer to the map.
 * @return Key count, or `-1` on error
 */
JSL_STR_TO_STR_MAP_DEF int64_t jsl_str_to_str_map_item_count(
    JSLStrToStrMap* map
);

/**
 * Does the map have the given key.
 *
 * @param map Pointer to the map.
 * @return `true` if yes, `false` if no or error
 */
JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_has_key(
    JSLStrToStrMap* map,
    JSLFatPtr key
);

/**
 * Insert a key/value pair.
 *
 * @param map Map to mutate.
 * @param key Key to insert.
 * @param key_lifetime Lifetime semantics for the key data.
 * @param value Value to insert.
 * @param value_lifetime Lifetime semantics for the value data.
 * @return `true` on success, `false` on invalid parameters or OOM.
 */
JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_insert(
    JSLStrToStrMap* map,
    JSLFatPtr key,
    JSLStringLifeTime key_lifetime,
    JSLFatPtr value,
    JSLStringLifeTime value_lifetime
);

/**
 * Get the value of the key.
 *
 * @param map Map to search.
 * @param key Key to search for.
 * @param out_value Output parameter that will be filled with the value if successful
 * @returns A bool indicating success or failure
 */
JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_get(
    JSLStrToStrMap* map,
    JSLFatPtr key,
    JSLFatPtr* out_value
);

/**
 * Initialize an iterator that visits every key/value pair in the map.
 * 
 * Example:
 *
 * ```
 * JSLStrToStrMapKeyValueIter iter;
 * jsl_str_to_str_map_key_value_iterator_init(
 *     &map, &iter
 * );
 * 
 * JSLFatPtr key;
 * JSLFatPtr value;
 * while (jsl_str_to_str_map_key_value_iterator_next(&iter, &key, &value))
 * {
 *    ...
 * }
 * ```
 *
 * Overall traversal order is undefined. The iterator is invalidated
 * if the map is mutated after initialization.
 *
 * @param map Map to iterate over; must be initialized.
 * @param iterator Iterator instance to initialize.
 * @return `true` on success, `false` if parameters are invalid.
 */
JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_key_value_iterator_init(
    JSLStrToStrMap* map,
    JSLStrToStrMapKeyValueIter* iterator
);

/**
 * Advance the key/value iterator.
 * 
 * Example:
 *
 * ```
 * JSLStrToStrMapKeyValueIter iter;
 * jsl_str_to_str_map_key_value_iterator_init(
 *     &map, &iter
 * );
 * 
 * JSLFatPtr key;
 * JSLFatPtr value;
 * while (jsl_str_to_str_map_key_value_iterator_next(&iter, &key, &value))
 * {
 *    ...
 * }
 * ```
 *
 * Returns the next key/value pair for the map. The iterator must be
 * initialized and is invalidated if the map is mutated; iteration order
 * is undefined.
 *
 * @param iterator Iterator to advance.
 * @param out_key Output for the current key.
 * @param out_value Output for the current value.
 * @return `true` if a pair was produced, `false` if exhausted or invalid.
 */
JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_key_value_iterator_next(
    JSLStrToStrMapKeyValueIter* iterator,
    JSLFatPtr* out_key,
    JSLFatPtr* out_value
);

/**
 * Remove a key/value.
 *
 * Iterators become invalid. If the key is not present or parameters are invalid,
 * the map is unchanged and `false` is returned.
 *
 * @param map Map to mutate.
 * @param key Key to remove.
 * @return `true` if the key existed and was removed, `false` otherwise.
 */
JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_delete(
    JSLStrToStrMap* map,
    JSLFatPtr key
);

/**
 * Remove all keys and values from the map.  Iterators become invalid.
 *
 * @param map Map to clear.
 */
JSL_STR_TO_STR_MAP_DEF void jsl_str_to_str_map_clear(
    JSLStrToStrMap* map
);

#ifdef __cplusplus
}
#endif

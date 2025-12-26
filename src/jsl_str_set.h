/**
 * # JSL String to String Map
 * 
 * This file is a single header file library that implements a hash set data
 * structure, which 
 * 
 * ## Documentation
 * 
 * See `docs/jsl_str_set.md` for a formatted documentation page.
 *
 * ## Caveats
 * 
 * This set uses arenas, so some wasted memory is indeveatble. Care has
 * been taken to reuse as much allocated memory as possible. But if your
 * set is long lived it's possible to start exhausting the arena with
 * old memory.
 * 
 * Remember to
 * 
 * * have an initial item count guess as accurate as you can to reduce rehashes
 * * have the arena have as short a lifetime as possible
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

#pragma once

#include <stdint.h>
#include <stddef.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_hash_map_common.h"

/* Versioning to catch mismatches across deps */
#ifndef JSL_STR_SET_VERSION
    #define JSL_STR_SET_VERSION 0x010000  /* 1.0.0 */
#else
    #if JSL_STR_SET_VERSION != 0x010000
        #error "jsl_str_set.h version mismatch across includes"
    #endif
#endif

#ifndef JSL_STR_SET_DEF
    #define JSL_STR_SET_DEF
#endif

#ifdef __cplusplus
extern "C" {
#endif


enum JSLStrSetKeyState {
    JSL__SET_EMPTY = 0UL,
    JSL__SET_TOMBSTONE = 1UL
};

#define JSL__SET_SSO_LENGTH 16

struct JSL__StrSetEntry {
    uint8_t value_sso_buffer[JSL__SET_SSO_LENGTH];

    JSLFatPtr value;
    uint64_t hash;

    /// @brief Used to store in the free list, ignored otherwise
    struct JSL__StrSetEntry* next;
};

struct JSL__StrSetKeyValueIter {
    struct JSL__StrSet* set;
    int64_t current_lut_index;
    int64_t generational_id;
    uint64_t sentinel;
};

struct JSL__StrSet {
    // putting the sentinel first means it's much more likely to get
    // corrupted from accidental overwrites, therefore making it
    // more likely that memory bugs are caught.
    uint64_t sentinel;

    JSLArena* arena;

    uintptr_t* entry_lookup_table;
    int64_t entry_lookup_table_length;

    int64_t item_count;
    int64_t tombstone_count;

    struct JSL__StrSetEntry* entry_free_list;

    uint64_t hash_seed;
    float load_factor;
    int32_t generational_id;
};

/**
 * This is an open addressed hash set with linear probing that maps
 * JSLFatPtr keys to JSLFatPtr values. This set uses rapidhash, which
 * is a avalanche hash with a configurable seed value for protection
 * against hash flooding attacks.
 * 
 * Example:
 *
 * ```
 * uint8_t buffer[JSL_KILOBYTES(16)];
 * JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 *
 * JSLStrSet set;
 * jsl_str_set_init(&set, &stack_arena, 0);
 *
 * JSLFatPtr key = JSL_FATPTR_INITIALIZER("hello-key");
 * 
 * jsl_str_to_str_multimap_insert(
 *     &set,
 *     key,
 *     JSL_STRING_LIFETIME_STATIC,
 *     JSL_FATPTR_EXPRESSION("hello-value"),
 *     JSL_STRING_LIFETIME_STATIC
 * );
 * 
 * JSLFatPtr value
 * jsl_str_set_get(&set, key, &value);
 * ```
 * 
 * ## Functions
 *
 *  * jsl_str_set_init
 *  * jsl_str_set_init2
 *  * jsl_str_set_item_count
 *  * jsl_str_set_has_key
 *  * jsl_str_set_insert
 *  * jsl_str_set_get
 *  * jsl_str_set_key_value_iterator_init
 *  * jsl_str_set_key_value_iterator_next
 *  * jsl_str_set_delete
 *  * jsl_str_set_clear
 *
 */
typedef struct JSL__StrSet JSLStrSet;

/**
 * State tracking struct for iterating over all of the keys and values
 * in the set.
 * 
 * @note If you mutate the set this iterator is automatically invalidated
 * and any operations on this iterator will terminate with failure return
 * values.
 * 
 * ## Functions
 *
 *  * jsl_str_set_key_value_iterator_init
 *  * jsl_str_set_key_value_iterator_next
 */
typedef struct JSL__StrSetKeyValueIter JSLStrSetKeyValueIter;

/**
 * Initialize a set with default sizing parameters.
 *
 * This sets up internal tables in the provided arena, using a 32 entry
 * initial capacity guess and a 0.75 load factor. The `seed` value is to
 * protect against hash flooding attacks. If you're absolutely sure this
 * set cannot be attacked, then zero is valid seed value.
 *
 * @param set Pointer to the set to initialize.
 * @param arena Arena used for all allocations.
 * @param seed Arbitrary seed value for hashing.
 * @return `true` on success, `false` if any parameter is invalid or out of memory.
 */
JSL_STR_SET_DEF bool jsl_str_set_init(
    JSLStrSet* set,
    JSLArena* arena,
    uint64_t seed
);

/**
 * Initialize a set with explicit sizing parameters.
 *
 * This is identical to `jsl_str_set_init`, but lets callers
 * provide an initial `item_count_guess` and a `load_factor`. The initial
 * lookup table is sized to the next power of two above `item_count_guess`,
 * clamped to at least 32 entries. `load_factor` must be in the range
 * `(0.0f, 1.0f)` and controls when the table rehashes. The `seed` value
 * is to protect against hash flooding attacks. If you're absolutely sure 
 * this set cannot be attacked, then zero is valid seed value
 *
 * @param set Pointer to the set to initialize.
 * @param arena Arena used for all allocations.
 * @param seed Arbitrary seed value for hashing.
 * @param item_count_guess Expected max number of keys
 * @param load_factor Desired load factor before rehashing
 * @return `true` on success, `false` if any parameter is invalid or out of memory.
 */
JSL_STR_SET_DEF bool jsl_str_set_init2(
    JSLStrSet* set,
    JSLArena* arena,
    uint64_t seed,
    int64_t item_count_guess,
    float load_factor
);

/**
 * Get the number of items currently stored.
 *
 * @param set Pointer to the set.
 * @return Key count, or `-1` on error
 */
JSL_STR_SET_DEF int64_t jsl_str_set_item_count(
    JSLStrSet* set
);

/**
 * Does the set have the given key.
 *
 * @param set Pointer to the set.
 * @return `true` if yes, `false` if no or error
 */
JSL_STR_SET_DEF bool jsl_str_set_has_value(
    JSLStrSet* set,
    JSLFatPtr value
);

/**
 * Insert a key/value pair.
 *
 * @param set Map to mutate.
 * @param key Key to insert.
 * @param key_lifetime Lifetime semantics for the key data.
 * @param value Value to insert.
 * @param value_lifetime Lifetime semantics for the value data.
 * @return `true` on success, `false` on invalid parameters or OOM.
 */
JSL_STR_SET_DEF bool jsl_str_set_insert(
    JSLStrSet* set,
    JSLFatPtr value,
    JSLStringLifeTime value_lifetime
);

/**
 * Initialize an iterator that visits every key/value pair in the set.
 * 
 * Example:
 *
 * ```
 * JSLStrSetKeyValueIter iter;
 * jsl_str_set_key_value_iterator_init(
 *     &set, &iter
 * );
 * 
 * JSLFatPtr key;
 * JSLFatPtr value;
 * while (jsl_str_set_key_value_iterator_next(&iter, &key, &value))
 * {
 *    ...
 * }
 * ```
 *
 * Overall traversal order is undefined. The iterator is invalidated
 * if the set is mutated after initialization.
 *
 * @param set Map to iterate over; must be initialized.
 * @param iterator Iterator instance to initialize.
 * @return `true` on success, `false` if parameters are invalid.
 */
JSL_STR_SET_DEF bool jsl_str_set_iterator_init(
    JSLStrSet* set,
    JSLStrSetKeyValueIter* iterator
);

/**
 * Advance the key/value iterator.
 * 
 * Example:
 *
 * ```
 * JSLStrSetKeyValueIter iter;
 * jsl_str_set_key_value_iterator_init(
 *     &set, &iter
 * );
 * 
 * JSLFatPtr key;
 * JSLFatPtr value;
 * while (jsl_str_set_key_value_iterator_next(&iter, &key, &value))
 * {
 *    ...
 * }
 * ```
 *
 * Returns the next key/value pair for the set. The iterator must be
 * initialized and is invalidated if the set is mutated; iteration order
 * is undefined.
 *
 * @param iterator Iterator to advance.
 * @param out_key Output for the current key.
 * @param out_value Output for the current value.
 * @return `true` if a pair was produced, `false` if exhausted or invalid.
 */
JSL_STR_SET_DEF bool jsl_str_set_iterator_next(
    JSLStrSetKeyValueIter* iterator,
    JSLFatPtr* out_value
);

/**
 * Remove a key/value.
 *
 * Iterators become invalid. If the key is not present or parameters are invalid,
 * the set is unchanged and `false` is returned.
 *
 * @param set Map to mutate.
 * @param key Key to remove.
 * @return `true` if the key existed and was removed, `false` otherwise.
 */
JSL_STR_SET_DEF bool jsl_str_set_delete(
    JSLStrSet* set,
    JSLFatPtr key
);

/**
 * Remove all keys and values from the set.  Iterators become invalid.
 *
 * @param set Map to clear.
 */
JSL_STR_SET_DEF void jsl_str_set_clear(
    JSLStrSet* set
);

#ifdef __cplusplus
}
#endif

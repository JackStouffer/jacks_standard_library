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
 * ## Usage
 * 
 * 1. Copy the `jsl_str_to_str_multimap.h` file into your repo
 * 2. Include the header like normally in each source file where you use it:
 * 
 * ```c
 * #include "jsl_str_to_str_multimap.h"
 * ```
 * 
 * 3. Then, in ONE AND ONLY ONE file, do this:
 * 
 * ```c
 * #define JSL_STR_TO_STR_MULTIMAP_IMPLEMENTATION
 * #include "jsl_str_to_str_multimap.h"
 * ```
 * 
 * **IMPORTANT**: The multimap also requires that the implementation of
 * `jsl_core.h` be in the same executable.
 * 
 * This should probably be in the same file as your entrypoint function,
 * but it doesn't have to be. It's also common to put this into an otherwise
 * empty file for easier integration to standard C/C++ build systems.
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


#ifndef JSL_STR_TO_STR_MULTIMAP_H_INCLUDED
    #define JSL_STR_TO_STR_MULTIMAP_H_INCLUDED

    #include <stdint.h>
    #include <stddef.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "jsl_core.h"
    #include "jsl_hash_map_common.h"

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
     *     JSL_STRING_LIFETIME_STATIC,
     *     JSL_FATPTR_EXPRESSION("hello-value"),
     *     JSL_STRING_LIFETIME_STATIC
     * );
     *
     * jsl_str_to_str_multimap_insert(
     *     &map,
     *     key,
     *     JSL_STRING_LIFETIME_STATIC,
     *     JSL_FATPTR_EXPRESSION("hello-value2"),
     *     JSL_STRING_LIFETIME_STATIC
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
        JSLArena* arena,
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
        JSLArena* arena,
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
     * (`JSL_STRING_LIFETIME_STATIC`) or copied into the map
     * (`JSL_STRING_LIFETIME_TRANSIENT`). Use the transient lifetime if the string's
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
     * Reclaims the key entry and value nodes into internal free lists without
     * releasing arena memory. Iterators become invalid. If the key is not
     * present or parameters are invalid, the map is unchanged and `false` is
     * returned.
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
     * Remove all keys and values from the map.
     *
     * All entries and values are recycled into free lists for reuse; arena
     * allocations are retained. Iterators become invalid.
     *
     * @param map Multimap to clear.
     */
    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_clear(
        JSLStrToStrMultimap* map
    );
    
    #ifdef __cplusplus
    }
    #endif
    
#endif /* JSL_STR_TO_STR_MULTIMAP_H_INCLUDED */

#ifdef JSL_STR_TO_STR_MULTIMAP_IMPLEMENTATION

    enum JSLStrToStrMultimapKeyState {
        JSL__MULTIMAP_EMPTY = 1UL,
        JSL__MULTIMAP_TOMBSTONE = 2UL
    };

    #define JSL__MULTIMAP_SSO_LENGTH 16
    #define JSL__MULTIMAP_PRIVATE_SENTINEL 15280798434051232421UL

    struct JSL__StrToStrMultimapValue {
        uint8_t small_string_buffer[JSL__MULTIMAP_SSO_LENGTH];

        JSLFatPtr value;
        struct JSL__StrToStrMultimapValue* next;
    };

    struct JSL__StrToStrMultimapEntry {
        /// @brief small string optimization buffer to hold the key if len < 16
        uint8_t small_string_buffer[JSL__MULTIMAP_SSO_LENGTH];

        // TODO: docs
        JSLFatPtr key;
        // TODO: docs
        uint64_t hash;

        /// @brief Used to store in the free list, ignored otherwise
        struct JSL__StrToStrMultimapEntry* next;

        // TODO: docs
        struct JSL__StrToStrMultimapValue* values_head;

        int64_t value_count;
    };

    struct JSL__StrToStrMultimapKeyValueIter {
        JSLStrToStrMultimap* map;
        struct JSL__StrToStrMultimapEntry* current_entry;
        struct JSL__StrToStrMultimapValue* current_value;
        int64_t current_lut_index;
        int64_t generational_id;
        uint64_t sentinel;
    };

    struct JSL__StrToStrMultimapValueIter {
        JSLStrToStrMultimap* map;
        struct JSL__StrToStrMultimapEntry* entry;
        struct JSL__StrToStrMultimapValue* current_value;
        int64_t generational_id;
        uint64_t sentinel;
    };

    struct JSL__StrToStrMultimap {
        JSLArena* arena;

        uintptr_t* entry_lookup_table;
        int64_t entry_lookup_table_length;

        int64_t key_count;
        int64_t value_count;
        int64_t tombstone_count;

        struct JSL__StrToStrMultimapEntry* entry_free_list;
        struct JSL__StrToStrMultimapValue* value_free_list;

        uint64_t sentinel;

        uint64_t hash_seed;
        float load_factor;
        int32_t generational_id;
    };

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_init(
        JSLStrToStrMultimap* map,
        JSLArena* arena,
        uint64_t seed
    )
    {
        return jsl_str_to_str_multimap_init2(
            map,
            arena,
            seed,
            32,
            0.75f
        );
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_init2(
        JSLStrToStrMultimap* map,
        JSLArena* arena,
        uint64_t seed,
        int64_t item_count_guess,
        float load_factor
    )
    {
        bool res = true;

        if (
            map == NULL
            || arena == NULL
            || item_count_guess <= 0
            || load_factor <= 0.0f
            || load_factor >= 1.0f
        )
            res = false;

        if (res)
        {
            JSL_MEMSET(map, 0, sizeof(JSLStrToStrMultimap));
            map->arena = arena;
            map->load_factor = load_factor;
            map->hash_seed = seed;

            item_count_guess = JSL_MAX(32L, item_count_guess);
            int64_t items = jsl_next_power_of_two_i64(item_count_guess + 1);

            map->entry_lookup_table = (uintptr_t*) jsl_arena_allocate_aligned(
                arena,
                (int64_t) sizeof(uintptr_t) * items,
                _Alignof(uintptr_t),
                false
            ).data;

            for (int64_t i = 0; i < items; i++)
            {
                map->entry_lookup_table[i] = JSL__MULTIMAP_EMPTY;
            }
            
            map->entry_lookup_table_length = items;

            map->sentinel = JSL__MULTIMAP_PRIVATE_SENTINEL;
        }

        return res;
    }

    static bool jsl__str_to_str_multimap_rehash(
        JSLStrToStrMultimap* map
    )
    {
        bool res = false;

        bool params_valid = (
            map != NULL
            && map->arena != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && map->entry_lookup_table != NULL
            && map->entry_lookup_table_length > 0
        );

        uintptr_t* old_table = params_valid ? map->entry_lookup_table : NULL;
        int64_t old_length = params_valid ? map->entry_lookup_table_length : 0;

        int64_t new_length = params_valid ? jsl_next_power_of_two_i64(old_length + 1) : 0;
        bool length_valid = params_valid && new_length > old_length && new_length > 0;

        bool bytes_possible = length_valid
            && new_length <= (INT64_MAX / (int64_t) sizeof(uintptr_t));

        int64_t bytes_needed = bytes_possible
            ? (int64_t) sizeof(uintptr_t) * new_length
            : 0;

        JSLFatPtr new_table_mem = {0};
        if (bytes_possible)
        {
            new_table_mem = jsl_arena_allocate_aligned(
                map->arena,
                bytes_needed,
                _Alignof(uintptr_t),
                false
            );
        }

        uintptr_t* new_table = (bytes_possible && new_table_mem.data != NULL)
            ? (uintptr_t*) new_table_mem.data
            : NULL;

        bool allocation_ok = new_table != NULL;

        int64_t init_index = 0;
        while (allocation_ok && init_index < new_length)
        {
            new_table[init_index] = JSL__MULTIMAP_EMPTY;
            ++init_index;
        }

        uint64_t lut_mask = new_length > 0 ? ((uint64_t) new_length - 1u) : 0;
        int64_t old_index = 0;
        bool migrate_ok = allocation_ok;

        while (migrate_ok && old_index < old_length)
        {
            uintptr_t lut_res = old_table[old_index];

            bool occupied = (
                lut_res != 0
                && lut_res != JSL__MULTIMAP_EMPTY
                && lut_res != JSL__MULTIMAP_TOMBSTONE
            );

            struct JSL__StrToStrMultimapEntry* entry = occupied
                ? (struct JSL__StrToStrMultimapEntry*) lut_res
                : NULL;

            bool has_values = occupied
                && entry != NULL
                && entry->values_head != NULL
                && entry->value_count > 0;

            int64_t probe_index = has_values
                ? (int64_t) (entry->hash & lut_mask)
                : 0;

            int64_t probes = 0;
            bool insert_needed = has_values;

            while (migrate_ok && insert_needed && probes < new_length)
            {
                uintptr_t probe_res = new_table[probe_index];
                bool slot_free = (
                    probe_res == 0
                    || probe_res == JSL__MULTIMAP_EMPTY
                    || probe_res == JSL__MULTIMAP_TOMBSTONE
                );

                if (slot_free)
                {
                    new_table[probe_index] = (uintptr_t) entry;
                    insert_needed = false;
                }

                bool advance_probe = insert_needed;
                if (advance_probe)
                {
                    probe_index = (int64_t) (((uint64_t) probe_index + 1u) & lut_mask);
                    ++probes;
                }
            }

            bool placement_failed = insert_needed;
            if (placement_failed)
            {
                migrate_ok = false;
            }

            ++old_index;
        }

        bool should_commit = migrate_ok && allocation_ok && length_valid;
        if (should_commit)
        {
            map->entry_lookup_table = new_table;
            map->entry_lookup_table_length = new_length;
            map->tombstone_count = 0;
            ++map->generational_id;
            res = true;
        }

        bool failed = !should_commit;
        if (failed)
        {
            res = false;
        }

        return res;
    }

    static JSL__FORCE_INLINE bool jsl__str_to_str_multimap_add_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLStringLifeTime key_lifetime,
        int64_t lut_index,
        uint64_t hash
    )
    {
        struct JSL__StrToStrMultimapEntry* entry = NULL;
        bool replacing_tombstone = map->entry_lookup_table[lut_index] == JSL__MULTIMAP_TOMBSTONE;

        if (map->entry_free_list == NULL)
        {
            entry = JSL_ARENA_TYPED_ALLOCATE(struct JSL__StrToStrMultimapEntry, map->arena);
        }
        else
        {
            struct JSL__StrToStrMultimapEntry* next = map->entry_free_list->next;
            entry = map->entry_free_list;
            map->entry_free_list = next;
        }

        if (entry != NULL)
        {
            entry->values_head = NULL;
            entry->value_count = 0;
            entry->hash = hash;
            
            map->entry_lookup_table[lut_index] = (uintptr_t) entry;
            ++map->key_count;
        }

        if (entry != NULL && replacing_tombstone)
        {
            --map->tombstone_count;
        }

        if (entry != NULL && key_lifetime == JSL_STRING_LIFETIME_STATIC)
        {
            entry->key = key;
        }
        else if (
            entry != NULL
            && key_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && key.length <= JSL__MULTIMAP_SSO_LENGTH
        )
        {
            JSL_MEMCPY(entry->small_string_buffer, key.data, (size_t) key.length);
            entry->key.data = entry->small_string_buffer;
            entry->key.length = key.length;
        }
        else if (
            entry != NULL
            && key_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && key.length > JSL__MULTIMAP_SSO_LENGTH
        )
        {
            entry->key = jsl_fatptr_duplicate(map->arena, key);
        }

        return entry != NULL;
    }

    static JSL__FORCE_INLINE bool jsl__str_to_str_multimap_add_value_to_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr value,
        JSLStringLifeTime value_lifetime,
        int64_t lut_index
    )
    {
        struct JSL__StrToStrMultimapValue* value_record = NULL;
        uintptr_t lut_res = map->entry_lookup_table[lut_index];
        struct JSL__StrToStrMultimapEntry* entry = (struct JSL__StrToStrMultimapEntry*) lut_res;

        bool values_ok = lut_res != 0
            && lut_res != JSL__MULTIMAP_EMPTY
            && lut_res != JSL__MULTIMAP_TOMBSTONE;

        if (values_ok && map->value_free_list == NULL)
        {
            value_record = JSL_ARENA_TYPED_ALLOCATE(struct JSL__StrToStrMultimapValue, map->arena);
        }
        else if (values_ok)
        {
            struct JSL__StrToStrMultimapValue* next = map->value_free_list->next;
            value_record = map->value_free_list;
            map->value_free_list = next;
        }

        if (value_record != NULL)
        {
            ++entry->value_count;
            ++map->value_count;
            value_record->next = entry->values_head;
            entry->values_head = value_record;
        }

        if (
            value_record != NULL
            && value_lifetime == JSL_STRING_LIFETIME_STATIC
        )
        {
            value_record->value = value;
        }
        else if (
            value_record != NULL
            && value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && value.length <= JSL__MULTIMAP_SSO_LENGTH
        )
        {
            JSL_MEMCPY(value_record->small_string_buffer, value.data, (size_t) value.length);
            value_record->value.data = value_record->small_string_buffer;
            value_record->value.length = value.length;
        }
        else if (
            value_record != NULL
            && value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && value.length > JSL__MULTIMAP_SSO_LENGTH
        )
        {
            value_record->value = jsl_fatptr_duplicate(map->arena, value);
        }

        return value_record != NULL;
    }

    static inline void jsl__str_to_str_multimap_probe(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        int64_t* out_lut_index,
        uint64_t* out_hash,
        bool* out_found
    )
    {
        *out_lut_index = -1;
        *out_found = false;

        int64_t first_tombstone = -1;
        bool tombstone_seen = false;
        bool searching = true;

        *out_hash = jsl__rapidhash_withSeed(key.data, (size_t) key.length, map->hash_seed);

        int64_t lut_length = map->entry_lookup_table_length;
        uint64_t lut_mask = (uint64_t) lut_length - 1u;
        int64_t lut_index = (int64_t) (*out_hash & lut_mask);
        int64_t probes = 0;

        while (searching && probes < lut_length)
        {
            uintptr_t lut_res = map->entry_lookup_table[lut_index];

            bool is_empty = lut_res == JSL__MULTIMAP_EMPTY || lut_res == 0;
            bool is_tombstone = lut_res == JSL__MULTIMAP_TOMBSTONE;

            if (is_empty)
            {
                *out_lut_index = tombstone_seen ? first_tombstone : lut_index;
                searching = false;
            }

            bool record_tombstone = searching && is_tombstone && !tombstone_seen;
            if (record_tombstone)
            {
                first_tombstone = lut_index;
                tombstone_seen = true;
            }

            bool slot_has_entry = searching && !is_empty && !is_tombstone;
            struct JSL__StrToStrMultimapEntry* entry = slot_has_entry
                ? (struct JSL__StrToStrMultimapEntry*) lut_res
                : NULL;

            bool entry_valid = slot_has_entry
                && entry != NULL
                && entry->value_count > 0;

            bool matches = entry_valid
                && *out_hash == entry->hash
                && jsl_fatptr_memory_compare(key, entry->key);

            if (matches)
            {
                *out_found = true;
                *out_lut_index = lut_index;
                searching = false;
            }

            bool empty_entry = slot_has_entry
                && (entry == NULL || entry->value_count <= 0);

            if (empty_entry)
            {
                ++map->tombstone_count;
                map->entry_lookup_table[lut_index] = JSL__MULTIMAP_TOMBSTONE;
                if (!tombstone_seen)
                {
                    first_tombstone = lut_index;
                    tombstone_seen = true;
                }
            }

            bool advance_probe = searching;
            if (advance_probe)
            {
                lut_index = (int64_t) (((uint64_t) lut_index + 1u) & lut_mask);
                ++probes;
            }
        }

        bool exhausted = searching && probes >= lut_length;
        if (exhausted)
        {
            *out_lut_index = tombstone_seen ? first_tombstone : -1;
        }
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_insert(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLStringLifeTime key_lifetime,
        JSLFatPtr value,
        JSLStringLifeTime value_lifetime
    )
    {
        bool res = (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && key.data != NULL 
            && key.length > -1
            && value.data != NULL
            && value.length > -1
        );

        bool needs_rehash = false;
        if (res)
        {
            float occupied_count = (float) (map->key_count + map->tombstone_count);
            float current_load_factor =  occupied_count / (float) map->entry_lookup_table_length;
            bool too_many_tombstones = map->tombstone_count > (map->entry_lookup_table_length / 4);
            needs_rehash = current_load_factor >= map->load_factor || too_many_tombstones;
        }

        if (JSL__UNLIKELY(needs_rehash))
        {
            res = jsl__str_to_str_multimap_rehash(map);
        }

        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;
        if (res)
        {
            jsl__str_to_str_multimap_probe(map, key, &lut_index, &hash, &existing_found);
        }
        
        // insert into existing
        if (lut_index > -1 && existing_found)
        {
            res = jsl__str_to_str_multimap_add_value_to_key(map, value, value_lifetime, lut_index);
        }
        // new key
        else if (lut_index > -1 && !existing_found)
        {
            bool key_add_res = jsl__str_to_str_multimap_add_key(map, key, key_lifetime, lut_index, hash);
            bool value_add_res = key_add_res ? 
                jsl__str_to_str_multimap_add_value_to_key(map, value, value_lifetime, lut_index)
                : false;
            res = key_add_res && value_add_res;
        }

        if (res)
        {
            ++map->generational_id;
        }

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_has_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    )
    {
        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;

        if (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && key.data != NULL 
            && key.length > -1
        )
        {
            jsl__str_to_str_multimap_probe(map, key, &lut_index, &hash, &existing_found);
        }

        return lut_index > -1 && existing_found;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_key_count(
        JSLStrToStrMultimap* map
    )
    {
        int64_t res = -1;
        bool valid = (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
        );

        if (valid)
        {
            res = map->key_count;
        }

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count(
        JSLStrToStrMultimap* map
    )
    {
        int64_t res = -1;
        bool valid = (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
        );

        if (valid)
        {
            res = map->value_count;
        }

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count_for_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    )
    {
        int64_t res = -1;
        bool proceed = false;

        if (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && key.data != NULL
            && key.length > -1
        )
            proceed = true;

        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;
        if (proceed)
        {
            jsl__str_to_str_multimap_probe(map, key, &lut_index, &hash, &existing_found);
            proceed = lut_index > -1 && existing_found;
        }

        if (proceed)
        {
            struct JSL__StrToStrMultimapEntry* entry = (struct JSL__StrToStrMultimapEntry*) 
                map->entry_lookup_table[lut_index];
            res = entry->value_count;
        }
        else if (map != NULL && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL)
        {
            res = 0;
        }

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_key_value_iterator_init(
        JSLStrToStrMultimap* map,
        JSLStrToStrMultimapKeyValueIter* iterator
    )
    {
        bool res = false;

        if (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && iterator != NULL
        )
        {
            iterator->map = map;
            iterator->current_lut_index = 0;
            iterator->current_entry = NULL;
            iterator->current_value = NULL;
            iterator->sentinel = JSL__MULTIMAP_PRIVATE_SENTINEL;
            iterator->generational_id = map->generational_id;
            res = true;
        }

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_key_value_iterator_next(
        JSLStrToStrMultimapKeyValueIter* iterator,
        JSLFatPtr* out_key,
        JSLFatPtr* out_value
    )
    {
        bool found = false;

        bool params_valid = (
            iterator != NULL
            && out_key != NULL
            && out_value != NULL
            && iterator->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && iterator->map != NULL
            && iterator->map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && iterator->map->entry_lookup_table != NULL
            && iterator->generational_id == iterator->map->generational_id
        );

        bool try_same_entry = (
            params_valid
            && iterator->current_entry != NULL
            && iterator->current_value != NULL
        );

        struct JSL__StrToStrMultimapValue* next_value = NULL;
        if (try_same_entry)
        {
            next_value = iterator->current_value->next;
        }

        bool next_value_ready = try_same_entry && next_value != NULL;
        if (next_value_ready)
        {
            iterator->current_value = next_value;
            *out_key = iterator->current_entry->key;
            *out_value = iterator->current_value->value;
            found = true;
        }

        bool reached_end_of_entry = try_same_entry && !next_value_ready;
        if (reached_end_of_entry)
        {
            iterator->current_entry = NULL;
            iterator->current_value = NULL;
        }

        bool search_for_entry = params_valid && !found;
        int64_t lut_length = params_valid ? iterator->map->entry_lookup_table_length : 0;
        int64_t lut_index = iterator->current_lut_index;
        struct JSL__StrToStrMultimapEntry* found_entry = NULL;

        while (search_for_entry && lut_index < lut_length)
        {
            uintptr_t lut_res = iterator->map->entry_lookup_table[lut_index];

            bool occupied = (
                lut_res != 0
                && lut_res != JSL__MULTIMAP_EMPTY
                && lut_res != JSL__MULTIMAP_TOMBSTONE
            );

            struct JSL__StrToStrMultimapEntry* candidate_entry = NULL;
            if (occupied)
            {
                candidate_entry = (struct JSL__StrToStrMultimapEntry*) lut_res;
            }

            bool has_values = false;
            if (occupied)
            {
                has_values = (
                    candidate_entry->values_head != NULL
                    && candidate_entry->value_count > 0
                );
            }

            if (has_values)
            {
                found_entry = candidate_entry;
                search_for_entry = false;
            }

            bool advance_index = search_for_entry;
            if (advance_index)
            {
                ++lut_index;
            }
        }

        bool entry_found = found_entry != NULL && !search_for_entry;
        if (entry_found)
        {
            iterator->current_entry = found_entry;
            iterator->current_value = found_entry->values_head;
            iterator->current_lut_index = lut_index + 1;
            *out_key = iterator->current_entry->key;
            *out_value = iterator->current_value->value;
            found = true;
        }

        bool exhausted = params_valid && !found;
        if (exhausted)
        {
            iterator->current_entry = NULL;
            iterator->current_value = NULL;
            iterator->current_lut_index = lut_length;
        }

        bool invalidated = !params_valid;
        if (invalidated)
        {
            found = false;
        }

        return found;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_get_values_for_key_iterator_init(
        JSLStrToStrMultimap* map,
        JSLStrToStrMultimapValueIter* iterator,
        JSLFatPtr key
    )
    {
        bool iterator_valid = iterator != NULL;

        if (iterator_valid)
        {
            iterator->map = NULL;
            iterator->entry = NULL;
            iterator->current_value = NULL;
            iterator->generational_id = 0;
            iterator->sentinel = 0;
        }

        bool params_valid = (
            iterator_valid
            && map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && map->entry_lookup_table != NULL
            && key.data != NULL
            && key.length > -1
        );

        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;
        if (params_valid)
        {
            iterator->map = map;
            iterator->generational_id = map->generational_id;
            iterator->sentinel = JSL__MULTIMAP_PRIVATE_SENTINEL;

            jsl__str_to_str_multimap_probe(map, key, &lut_index, &hash, &existing_found);
        }

        struct JSL__StrToStrMultimapEntry* found_entry = NULL;
        bool entry_found = existing_found && lut_index > -1;
        if (entry_found)
        {
            found_entry = (struct JSL__StrToStrMultimapEntry*) map->entry_lookup_table[lut_index];
        }

        bool has_values = entry_found
            && found_entry != NULL
            && found_entry->values_head != NULL
            && found_entry->value_count > 0;

        if (has_values)
        {
            iterator->entry = found_entry;
            iterator->current_value = NULL;
        }
        else
        {
            iterator->entry = NULL;
            iterator->current_value = NULL;
        }

        return params_valid;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_get_values_for_key_iterator_next(
        JSLStrToStrMultimapValueIter* iterator,
        JSLFatPtr* out_value
    )
    {
        bool found = false;

        bool params_valid = (
            iterator != NULL
            && out_value != NULL
            && iterator->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && iterator->map != NULL
            && iterator->map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && iterator->map->entry_lookup_table != NULL
            && iterator->generational_id == iterator->map->generational_id
        );

        bool entry_available = params_valid && iterator->entry != NULL;

        struct JSL__StrToStrMultimapValue* next_value = NULL;
        if (entry_available && iterator->current_value == NULL)
        {
            next_value = iterator->entry->values_head;
        }
        else if (entry_available && iterator->current_value != NULL)
        {
            next_value = iterator->current_value->next;
        }

        bool next_ready = entry_available && next_value != NULL;
        if (next_ready)
        {
            iterator->current_value = next_value;
            *out_value = next_value->value;
            found = true;
        }

        bool exhausted = entry_available && !next_ready;
        if (exhausted)
        {
            iterator->entry = NULL;
            iterator->current_value = NULL;
        }

        return found;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    )
    {
        bool res = false;

        bool params_valid = (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && map->entry_lookup_table != NULL
            && key.data != NULL
            && key.length > -1
        );

        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;
        if (params_valid)
        {
            jsl__str_to_str_multimap_probe(map, key, &lut_index, &hash, &existing_found);
        }

        bool key_found = existing_found && lut_index > -1;

        struct JSL__StrToStrMultimapEntry* entry = NULL;
        if (key_found)
        {
            entry = (struct JSL__StrToStrMultimapEntry*) map->entry_lookup_table[lut_index];
        }

        bool entry_valid = key_found && entry != NULL;

        int64_t removed_value_count = 0;
        struct JSL__StrToStrMultimapValue* value_record = NULL;
        if (entry_valid)
        {
            value_record = entry->values_head;
        }

        while (entry_valid && value_record != NULL)
        {
            struct JSL__StrToStrMultimapValue* next = value_record->next;
            value_record->next = map->value_free_list;
            map->value_free_list = value_record;
            value_record = next;
            ++removed_value_count;
        }

        if (entry_valid)
        {
            entry->values_head = NULL;
            entry->value_count = 0;

            map->value_count -= removed_value_count;
            map->entry_lookup_table[lut_index] = JSL__MULTIMAP_TOMBSTONE;

            entry->next = map->entry_free_list;
            map->entry_free_list = entry;

            --map->key_count;
            ++map->generational_id;
            ++map->tombstone_count;

            res = true;
        }

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_value(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLFatPtr value
    )
    {
        bool res = false;

        bool params_valid = (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && map->entry_lookup_table != NULL
            && key.data != NULL
            && key.length > -1
            && value.data != NULL
            && value.length > -1
        );

        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;
        if (params_valid)
        {
            jsl__str_to_str_multimap_probe(map, key, &lut_index, &hash, &existing_found);
        }

        bool key_found = params_valid && existing_found && lut_index > -1;

        struct JSL__StrToStrMultimapEntry* entry = NULL;
        if (key_found)
        {
            entry = (struct JSL__StrToStrMultimapEntry*) map->entry_lookup_table[lut_index];
        }

        bool entry_valid = key_found
            && entry != NULL
            && entry->values_head != NULL
            && entry->value_count > 0;

        struct JSL__StrToStrMultimapValue* previous = NULL;
        struct JSL__StrToStrMultimapValue* current = NULL;
        if (entry_valid)
        {
            current = entry->values_head;
        }

        bool value_found = false;
        while (entry_valid && current != NULL && !value_found)
        {
            bool equal = jsl_fatptr_memory_compare(current->value, value);
            if (equal)
            {
                value_found = true;
            }

            bool advance = !value_found;
            if (advance)
            {
                previous = current;
                current = current->next;
            }
        }

        bool remove_from_list = entry_valid && value_found && current != NULL;
        struct JSL__StrToStrMultimapValue* next_node = NULL;
        if (remove_from_list)
        {
            next_node = current->next;
        }

        bool removing_head = remove_from_list && previous == NULL;
        if (removing_head)
        {
            entry->values_head = next_node;
        }

        bool removing_non_head = remove_from_list && previous != NULL;
        if (removing_non_head)
        {
            previous->next = next_node;
        }

        if (remove_from_list)
        {
            current->next = map->value_free_list;
            map->value_free_list = current;
            --entry->value_count;
            --map->value_count;
        }

        bool entry_empty = remove_from_list && entry->value_count == 0;
        if (entry_empty)
        {
            map->entry_lookup_table[lut_index] = JSL__MULTIMAP_TOMBSTONE;
            ++map->tombstone_count;

            entry->next = map->entry_free_list;
            map->entry_free_list = entry;
            --map->key_count;
        }

        bool modified = remove_from_list;
        if (modified)
        {
            ++map->generational_id;
            res = true;
        }

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_clear(
        JSLStrToStrMultimap* map
    )
    {
        bool params_valid = (
            map != NULL
            && map->sentinel == JSL__MULTIMAP_PRIVATE_SENTINEL
            && map->entry_lookup_table != NULL
        );

        int64_t lut_length = params_valid ? map->entry_lookup_table_length : 0;
        int64_t index = 0;

        while (params_valid && index < lut_length)
        {
            uintptr_t lut_res = map->entry_lookup_table[index];
            bool occupied = (
                lut_res != 0
                && lut_res != JSL__MULTIMAP_EMPTY
                && lut_res != JSL__MULTIMAP_TOMBSTONE
            );

            struct JSL__StrToStrMultimapEntry* entry = NULL;
            if (occupied)
            {
                entry = (struct JSL__StrToStrMultimapEntry*) lut_res;
            }

            bool entry_has_values = occupied
                && entry != NULL
                && entry->values_head != NULL
                && entry->value_count > 0;

            struct JSL__StrToStrMultimapValue* value_record = NULL;
            if (entry_has_values)
            {
                value_record = entry->values_head;
            }

            while (entry_has_values && value_record != NULL)
            {
                struct JSL__StrToStrMultimapValue* next_value = value_record->next;
                value_record->next = map->value_free_list;
                map->value_free_list = value_record;
                value_record = next_value;
            }

            if (entry_has_values)
            {
                entry->values_head = NULL;
                entry->value_count = 0;
            }

            bool recycle_entry = occupied && entry != NULL;
            if (recycle_entry)
            {
                entry->next = map->entry_free_list;
                map->entry_free_list = entry;
                map->entry_lookup_table[index] = JSL__MULTIMAP_EMPTY;
            }
            else if (params_valid && lut_res == JSL__MULTIMAP_TOMBSTONE)
            {
                map->entry_lookup_table[index] = JSL__MULTIMAP_EMPTY;
            }

            ++index;
        }

        if (params_valid)
        {
            map->key_count = 0;
            map->value_count = 0;
            map->tombstone_count = 0;
            ++map->generational_id;
        }

        return;
    }

    #undef JSL__MULTIMAP_SSO_LENGTH
    #undef JSL__MULTIMAP_PRIVATE_SENTINEL

#endif /* JSL_STR_TO_STR_MULTIMAP_IMPLEMENTATION */

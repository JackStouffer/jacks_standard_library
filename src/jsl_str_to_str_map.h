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
 * ## Usage
 * 
 * 1. Copy the `jsl_str_to_str_map.h` file into your repo
 * 2. Include the header like normally in each source file where you use it:
 * 
 * ```c
 * #include "jsl_str_to_str_map.h"
 * ```
 * 
 * 3. Then, in ONE AND ONLY ONE file, do this:
 * 
 * ```c
 * #define JSL_STR_TO_STR_MAP_IMPLEMENTATION
 * #include "jsl_str_to_str_map.h"
 * ```
 * 
 * **IMPORTANT**: The map also requires that the implementation of
 * `jsl_core.h` be available at link time.
 * 
 * The implementation should probably be in the same file as your entrypoint function,
 * but it doesn't have to be. It's also common to put this into an otherwise
 * empty file for easier integration to standard C/C++ build systems.
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


#ifndef JSL_STR_TO_STR_MAP_H_INCLUDED
    #define JSL_STR_TO_STR_MAP_H_INCLUDED

    #include <stdint.h>
    #include <stddef.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "jsl_core.h"
    #include "jsl_hash_map_common.h"

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
        JSLArena* arena,
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
        JSLArena* arena,
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
    
#endif /* JSL_STR_TO_STR_MAP_H_INCLUDED */

#ifdef JSL_STR_TO_STR_MAP_IMPLEMENTATION

    enum JSLStrToStrMapKeyState {
        JSL__MAP_EMPTY = 0UL,
        JSL__MAP_TOMBSTONE = 1UL
    };

    #define JSL__MAP_SSO_LENGTH 8
    #define JSL__MAP_PRIVATE_SENTINEL 15280798434051232421UL

    struct JSL__StrToStrMapEntry {
        uint8_t key_sso_buffer[JSL__MAP_SSO_LENGTH];
        uint8_t value_sso_buffer[JSL__MAP_SSO_LENGTH];

        JSLFatPtr key;
        JSLFatPtr value;
        uint64_t hash;

        /// @brief Used to store in the free list, ignored otherwise
        struct JSL__StrToStrMapEntry* next;
    };

    struct JSL__StrToStrMapKeyValueIter {
        JSLStrToStrMap* map;
        int64_t current_lut_index;
        int64_t generational_id;
        uint64_t sentinel;
    };

    struct JSL__StrToStrMap {
        JSLArena* arena;

        uintptr_t* entry_lookup_table;
        int64_t entry_lookup_table_length;

        int64_t item_count;
        int64_t tombstone_count;

        struct JSL__StrToStrMapEntry* entry_free_list;

        uint64_t sentinel;

        uint64_t hash_seed;
        float load_factor;
        int32_t generational_id;
    };

    JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_init(
        JSLStrToStrMap* map,
        JSLArena* arena,
        uint64_t seed
    )
    {
        return jsl_str_to_str_map_init2(
            map,
            arena,
            seed,
            32,
            0.75f
        );
    }

    JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_init2(
        JSLStrToStrMap* map,
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
            JSL_MEMSET(map, 0, sizeof(JSLStrToStrMap));
            map->arena = arena;
            map->load_factor = load_factor;
            map->hash_seed = seed;

            item_count_guess = JSL_MAX(32L, item_count_guess);
            int64_t items = jsl_next_power_of_two_i64(item_count_guess + 1);

            map->entry_lookup_table = (uintptr_t*) jsl_arena_allocate_aligned(
                arena,
                (int64_t) sizeof(uintptr_t) * items,
                _Alignof(uintptr_t),
                true
            ).data;
            
            map->entry_lookup_table_length = items;

            map->sentinel = JSL__MAP_PRIVATE_SENTINEL;
        }

        return res;
    }

    static bool jsl__str_to_str_map_rehash(
        JSLStrToStrMap* map
    )
    {
        bool res = false;

        bool params_valid = (
            map != NULL
            && map->arena != NULL
            && map->sentinel == JSL__MAP_PRIVATE_SENTINEL
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
                true
            );
        }

        uintptr_t* new_table = (bytes_possible && new_table_mem.data != NULL)
            ? (uintptr_t*) new_table_mem.data
            : NULL;

        uint64_t lut_mask = new_length > 0 ? ((uint64_t) new_length - 1u) : 0;
        int64_t old_index = 0;
        bool migrate_ok = new_table != NULL;

        while (migrate_ok && old_index < old_length)
        {
            uintptr_t lut_res = old_table[old_index];

            bool occupied = (
                lut_res != 0
                && lut_res != JSL__MAP_EMPTY
                && lut_res != JSL__MAP_TOMBSTONE
            );

            struct JSL__StrToStrMapEntry* entry = occupied
                ? (struct JSL__StrToStrMapEntry*) lut_res
                : NULL;

            int64_t probe_index = entry != NULL
                ? (int64_t) (entry->hash & lut_mask)
                : 0;

            int64_t probes = 0;

            bool insert_needed = entry != NULL;
            while (migrate_ok && insert_needed && probes < new_length)
            {
                uintptr_t probe_res = new_table[probe_index];
                bool slot_free = (
                    probe_res == JSL__MAP_EMPTY
                    || probe_res == JSL__MAP_TOMBSTONE
                );

                if (slot_free)
                {
                    new_table[probe_index] = (uintptr_t) entry;
                    insert_needed = false;
                    break;
                }

                probe_index = (int64_t) (((uint64_t) probe_index + 1u) & lut_mask);
                ++probes;
            }

            bool placement_failed = insert_needed;
            if (placement_failed)
            {
                migrate_ok = false;
            }

            ++old_index;
        }

        bool should_commit = migrate_ok && new_table != NULL && length_valid;
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

    static JSL__FORCE_INLINE void jsl__str_to_str_map_update_value(
        JSLStrToStrMap* map,
        JSLFatPtr value,
        JSLStringLifeTime value_lifetime,
        int64_t lut_index
    )
    {
        uintptr_t lut_res = map->entry_lookup_table[lut_index];
        struct JSL__StrToStrMapEntry* entry = (struct JSL__StrToStrMapEntry*) lut_res;

        if (value_lifetime == JSL_STRING_LIFETIME_STATIC)
        {
            entry->value = value;
        }
        else if (
            value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && value.length <= JSL__MAP_SSO_LENGTH
        )
        {
            JSL_MEMCPY(entry->value_sso_buffer, value.data, (size_t) value.length);
            entry->value.data = entry->value_sso_buffer;
            entry->value.length = value.length;
        }
        else if (
            value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && value.length > JSL__MAP_SSO_LENGTH
        )
        {
            entry->value = jsl_fatptr_duplicate(map->arena, value);
        }
    }

    static JSL__FORCE_INLINE bool jsl__str_to_str_map_add(
        JSLStrToStrMap* map,
        JSLFatPtr key,
        JSLStringLifeTime key_lifetime,
        JSLFatPtr value,
        JSLStringLifeTime value_lifetime,
        int64_t lut_index,
        uint64_t hash
    )
    {
        struct JSL__StrToStrMapEntry* entry = NULL;
        bool replacing_tombstone = map->entry_lookup_table[lut_index] == JSL__MAP_TOMBSTONE;

        if (map->entry_free_list == NULL)
        {
            entry = JSL_ARENA_TYPED_ALLOCATE(struct JSL__StrToStrMapEntry, map->arena);
        }
        else
        {
            struct JSL__StrToStrMapEntry* next = map->entry_free_list->next;
            entry = map->entry_free_list;
            map->entry_free_list = next;
        }

        if (entry != NULL)
        {
            entry->hash = hash;
            
            map->entry_lookup_table[lut_index] = (uintptr_t) entry;
            ++map->item_count;
        }

        if (entry != NULL && replacing_tombstone)
        {
            --map->tombstone_count;
        }

        // 
        // Copy the key
        // 

        if (entry != NULL && key_lifetime == JSL_STRING_LIFETIME_STATIC)
        {
            entry->key = key;
        }
        else if (
            entry != NULL
            && key_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && key.length <= JSL__MAP_SSO_LENGTH
        )
        {
            JSL_MEMCPY(entry->key_sso_buffer, key.data, (size_t) key.length);
            entry->key.data = entry->key_sso_buffer;
            entry->key.length = key.length;
        }
        else if (
            entry != NULL
            && key_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && key.length > JSL__MAP_SSO_LENGTH
        )
        {
            entry->key = jsl_fatptr_duplicate(map->arena, key);
        }

        // 
        // Copy the value
        // 

        if (entry != NULL && value_lifetime == JSL_STRING_LIFETIME_STATIC)
        {
            entry->value = value;
        }
        else if (
            entry != NULL
            && value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && value.length <= JSL__MAP_SSO_LENGTH
        )
        {
            JSL_MEMCPY(entry->value_sso_buffer, value.data, (size_t) value.length);
            entry->value.data = entry->value_sso_buffer;
            entry->value.length = value.length;
        }
        else if (
            entry != NULL
            && value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
            && value.length > JSL__MAP_SSO_LENGTH
        )
        {
            entry->value = jsl_fatptr_duplicate(map->arena, value);
        }

        return entry != NULL;
    }

    static inline void jsl__str_to_str_map_probe(
        JSLStrToStrMap* map,
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

            bool is_empty = lut_res == JSL__MAP_EMPTY;
            bool is_tombstone = lut_res == JSL__MAP_TOMBSTONE;

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
            struct JSL__StrToStrMapEntry* entry = slot_has_entry
                ? (struct JSL__StrToStrMapEntry*) lut_res
                : NULL;

            bool matches = entry != NULL
                && *out_hash == entry->hash
                && jsl_fatptr_memory_compare(key, entry->key);

            if (matches)
            {
                *out_found = true;
                *out_lut_index = lut_index;
                searching = false;
            }

            if (entry == NULL)
            {
                ++map->tombstone_count;
                map->entry_lookup_table[lut_index] = JSL__MAP_TOMBSTONE;
            }

            if (entry == NULL && !tombstone_seen)
            {
                first_tombstone = lut_index;
                tombstone_seen = true;
            }

            if (searching)
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

    JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_insert(
        JSLStrToStrMap* map,
        JSLFatPtr key,
        JSLStringLifeTime key_lifetime,
        JSLFatPtr value,
        JSLStringLifeTime value_lifetime
    )
    {
        bool res = (
            map != NULL
            && map->sentinel == JSL__MAP_PRIVATE_SENTINEL
            && key.data != NULL 
            && key.length > -1
            && value.data != NULL
            && value.length > -1
        );

        bool needs_rehash = false;
        if (res)
        {
            float occupied_count = (float) (map->item_count + map->tombstone_count);
            float current_load_factor =  occupied_count / (float) map->entry_lookup_table_length;
            bool too_many_tombstones = map->tombstone_count > (map->entry_lookup_table_length / 4);
            needs_rehash = current_load_factor >= map->load_factor || too_many_tombstones;
        }

        if (JSL__UNLIKELY(needs_rehash))
        {
            res = jsl__str_to_str_map_rehash(map);
        }

        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;
        if (res)
        {
            jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);
        }
        
        // new key
        if (lut_index > -1 && !existing_found)
        {
            res = jsl__str_to_str_map_add(
                map,
                key, key_lifetime,
                value, value_lifetime,
                lut_index,
                hash
            );
        }
        // update
        else if (lut_index > -1 && existing_found)
        {
            jsl__str_to_str_map_update_value(map, value, value_lifetime, lut_index);
        }

        if (res)
        {
            ++map->generational_id;
        }

        return res;
    }

    JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_has_key(
        JSLStrToStrMap* map,
        JSLFatPtr key
    )
    {
        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;

        if (
            map != NULL
            && map->sentinel == JSL__MAP_PRIVATE_SENTINEL
            && key.data != NULL 
            && key.length > -1
        )
        {
            jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);
        }

        return lut_index > -1 && existing_found;
    }
    
    JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_get(
        JSLStrToStrMap* map,
        JSLFatPtr key,
        JSLFatPtr* out_value
    )
    {
        bool res = false;

        bool params_valid = (
            map != NULL
            && map->sentinel == JSL__MAP_PRIVATE_SENTINEL
            && map->entry_lookup_table != NULL
            && out_value != NULL
            && key.data != NULL 
            && key.length > -1
        );

        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;

        if (params_valid)
        {
            jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);
        }

        if (params_valid && existing_found && lut_index > -1)
        {
            struct JSL__StrToStrMapEntry* entry =
                (struct JSL__StrToStrMapEntry*) map->entry_lookup_table[lut_index];
            *out_value = entry->value;
            res = true;
        }
        else if (out_value != NULL)
        {
            *out_value = (JSLFatPtr) {0};
        }

        return res;
    }

    JSL_STR_TO_STR_MAP_DEF int64_t jsl_str_to_str_map_item_count(
        JSLStrToStrMap* map
    )
    {
        int64_t res = -1;

        if (
            map != NULL
            && map->sentinel == JSL__MAP_PRIVATE_SENTINEL
        )
        {
            res = map->item_count;
        }

        return res;
    }

    JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_key_value_iterator_init(
        JSLStrToStrMap* map,
        JSLStrToStrMapKeyValueIter* iterator
    )
    {
        bool res = false;

        if (
            map != NULL
            && map->sentinel == JSL__MAP_PRIVATE_SENTINEL
            && iterator != NULL
        )
        {
            iterator->map = map;
            iterator->current_lut_index = 0;
            iterator->sentinel = JSL__MAP_PRIVATE_SENTINEL;
            iterator->generational_id = map->generational_id;
            res = true;
        }

        return res;
    }

    JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_key_value_iterator_next(
        JSLStrToStrMapKeyValueIter* iterator,
        JSLFatPtr* out_key,
        JSLFatPtr* out_value
    )
    {
        bool found = false;

        bool params_valid = (
            iterator != NULL
            && out_key != NULL
            && out_value != NULL
            && iterator->sentinel == JSL__MAP_PRIVATE_SENTINEL
            && iterator->map != NULL
            && iterator->map->sentinel == JSL__MAP_PRIVATE_SENTINEL
            && iterator->map->entry_lookup_table != NULL
            && iterator->generational_id == iterator->map->generational_id
        );

        int64_t lut_length = params_valid ? iterator->map->entry_lookup_table_length : 0;
        int64_t lut_index = iterator->current_lut_index;
        struct JSL__StrToStrMapEntry* found_entry = NULL;

        while (params_valid && lut_index < lut_length)
        {
            uintptr_t lut_res = iterator->map->entry_lookup_table[lut_index];
            bool occupied = lut_res != JSL__MAP_EMPTY && lut_res != JSL__MAP_TOMBSTONE;

            if (occupied)
            {
                found_entry = (struct JSL__StrToStrMapEntry*) lut_res;
                break;
            }
            else
            {
                ++lut_index;
            }
        }

        if (found_entry != NULL)
        {
            iterator->current_lut_index = lut_index + 1;
            *out_key = found_entry->key;
            *out_value = found_entry->value;
            found = true;
        }

        bool exhausted = params_valid && found_entry == NULL;
        if (exhausted)
        {
            iterator->current_lut_index = lut_length;
            found = false;
        }

        return found;
    }

    JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_delete(
        JSLStrToStrMap* map,
        JSLFatPtr key
    )
    {
        bool res = false;

        bool params_valid = (
            map != NULL
            && map->sentinel == JSL__MAP_PRIVATE_SENTINEL
            && map->entry_lookup_table != NULL
            && key.data != NULL
            && key.length > -1
        );

        uint64_t hash = 0;
        int64_t lut_index = -1;
        bool existing_found = false;
        if (params_valid)
        {
            jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);
        }

        if (existing_found && lut_index > -1)
        {
            struct JSL__StrToStrMapEntry* entry =
                (struct JSL__StrToStrMapEntry*) map->entry_lookup_table[lut_index];

            entry->next = map->entry_free_list;
            map->entry_free_list = entry;

            --map->item_count;
            ++map->generational_id;

            map->entry_lookup_table[lut_index] = JSL__MAP_TOMBSTONE;
            ++map->tombstone_count;

            res = true;
        }

        return res;
    }

    JSL_STR_TO_STR_MAP_DEF void jsl_str_to_str_map_clear(
        JSLStrToStrMap* map
    )
    {
        bool params_valid = (
            map != NULL
            && map->sentinel == JSL__MAP_PRIVATE_SENTINEL
            && map->entry_lookup_table != NULL
        );

        int64_t lut_length = params_valid ? map->entry_lookup_table_length : 0;
        int64_t index = 0;

        while (params_valid && index < lut_length)
        {
            uintptr_t lut_res = map->entry_lookup_table[index];

            if (lut_res != JSL__MAP_EMPTY && lut_res != JSL__MAP_TOMBSTONE)
            {
                struct JSL__StrToStrMapEntry* entry = (struct JSL__StrToStrMapEntry*) lut_res;
                entry->next = map->entry_free_list;
                map->entry_free_list = entry;
                map->entry_lookup_table[index] = JSL__MAP_EMPTY;
            }
            else if (lut_res == JSL__MAP_TOMBSTONE)
            {
                map->entry_lookup_table[index] = JSL__MAP_EMPTY;
            }

            ++index;
        }

        if (params_valid)
        {
            map->item_count = 0;
            map->tombstone_count = 0;
            ++map->generational_id;
        }

        return;
    }

    #undef JSL__MAP_SSO_LENGTH
    #undef JSL__MAP_PRIVATE_SENTINEL

#endif /* JSL_STR_TO_STR_MAP_IMPLEMENTATION */

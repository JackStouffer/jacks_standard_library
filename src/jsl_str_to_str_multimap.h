/**
 * # JSL String to String Multimap
 * 
 * TODO: docs
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

    typedef struct JSL__StrToStrMultimapKeyValueIter JSLStrToStrMultimapKeyValueIter;

    typedef struct JSL__StrToStrMultimapValueIter JSLStrToStrMultimapValueIter;

    typedef struct JSL__StrToStrMultimap JSLStrToStrMultimap;

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_init(
        JSLStrToStrMultimap* map,
        JSLArena* arena,
        uint64_t seed
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_init2(
        JSLStrToStrMultimap* map,
        JSLArena* arena,
        uint64_t seed,
        int64_t item_count_guess,
        float load_factor
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_key_count(
        JSLStrToStrMultimap* map
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count(
        JSLStrToStrMultimap* map
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_has_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_insert(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLStringLifeTime key_lifetime,
        JSLFatPtr value,
        JSLStringLifeTime value_lifetime
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count_for_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_key_value_iterator_init(
        JSLStrToStrMultimap* map,
        JSLStrToStrMultimapKeyValueIter* iterator
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_key_value_iterator_next(
        JSLStrToStrMultimapKeyValueIter* iterator,
        JSLFatPtr* out_key,
        JSLFatPtr* out_value
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_get_key_iterator_init(
        JSLStrToStrMultimap* map,
        JSLStrToStrMultimapValueIter* iterator,
        JSLFatPtr key
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF JSLFatPtr jsl_str_to_str_multimap_get_key_iterator_next(
        JSLStrToStrMultimapValueIter* iterator
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    );

    // TODO: docs
    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_value(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLFatPtr value
    );

    // TODO: docs
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
    #define JSL__MULTIMAP_PRIVATE_SENTINEL 81284211UL

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
    };

    struct JSL__StrToStrMultimapValueIter {
        JSLStrToStrMultimap* map;
    };

    struct JSL__StrToStrMultimap {
        JSLArena* arena;

        uintptr_t* entry_lookup_table;
        int64_t entry_lookup_table_length;

        int64_t key_count;
        int64_t value_count;

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
        (void) map;
        return false;
    }

    static bool jsl__str_to_str_multimap_add_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLStringLifeTime key_lifetime,
        int64_t lut_index,
        uint64_t hash
    )
    {
        struct JSL__StrToStrMultimapEntry* entry = NULL;

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

    static bool jsl__str_to_str_multimap_add_value_to_key(
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

        *out_hash = jsl__rapidhash_withSeed(key.data, (size_t) key.length, map->hash_seed);
        int64_t lut_index = (int64_t) (*out_hash & ((uint64_t) map->entry_lookup_table_length - 1u));

        for (;;)
        {
            uintptr_t lut_res = map->entry_lookup_table[lut_index];

            if (lut_res == JSL__MULTIMAP_EMPTY)
            {
                *out_lut_index = tombstone_seen ? first_tombstone : lut_index;
                break;
            }
            else if (lut_res == JSL__MULTIMAP_TOMBSTONE)
            {
                if (!tombstone_seen)
                {
                    first_tombstone = lut_index;
                    tombstone_seen = true;
                }
            }
            else
            {
                struct JSL__StrToStrMultimapEntry* entry = (struct JSL__StrToStrMultimapEntry*) lut_res;

                if (entry->value_count > 0)
                {
                    if (*out_hash == entry->hash && jsl_fatptr_memory_compare(key, entry->key))
                    {
                        *out_found = true;
                        *out_lut_index = lut_index;
                        break;
                    }
                }
                else
                {
                    map->entry_lookup_table[lut_index] = JSL__MULTIMAP_TOMBSTONE;
                    if (!tombstone_seen)
                    {
                        first_tombstone = lut_index;
                        tombstone_seen = true;
                    }
                }
            }

            ++lut_index;
        }
        
        return;
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
            float current_load_factor = (float) map->key_count / (float) map->entry_lookup_table_length;
            needs_rehash = current_load_factor >= map->load_factor;
        }

        if (needs_rehash)
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
        (void) map;
        (void) key;
        return false;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_key_count(
        JSLStrToStrMultimap* map
    )
    {
        return map == NULL ? -1 : map->key_count;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count(
        JSLStrToStrMultimap* map
    )
    {
        return map == NULL ? -1 : map->value_count;
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

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_key_value_iterator_init(
        JSLStrToStrMultimap* map,
        JSLStrToStrMultimapKeyValueIter* iterator
    )
    {
        (void) map;
        (void) iterator;
        return;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_key_value_iterator_next(
        JSLStrToStrMultimapKeyValueIter* iterator,
        JSLFatPtr* out_key,
        JSLFatPtr* out_value
    )
    {
        (void) iterator;
        (void) out_key;
        (void) out_value;
        return false;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_get_key_iterator_init(
        JSLStrToStrMultimap* map,
        JSLStrToStrMultimapValueIter* iterator,
        JSLFatPtr key
    )
    {
        (void) map;
        (void) iterator;
        (void) key;
        return;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF JSLFatPtr jsl_str_to_str_multimap_get_key_iterator_next(
        JSLStrToStrMultimapValueIter* iterator
    )
    {
        (void) iterator;
        JSLFatPtr val = {0};
        return val;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    )
    {
        (void) map;
        (void) key;
        return false;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_value(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLFatPtr value
    )
    {
        (void) map;
        (void) key;
        (void) value;
        return false;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_clear(
        JSLStrToStrMultimap* map
    )
    {
        (void) map;
        return;
    }

#endif /* JSL_STR_TO_STR_MULTIMAP_IMPLEMENTATION */

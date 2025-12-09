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

    
    struct JSL__StrToStrMultimapValue {
        JSLFatPtr value;
        struct JSL__StrToStrMultimapValueNode* next;
    };

    enum JSLStrToStrMultimapKeyState {
        JSL__MULTIMAP_EMPTY = UINTPTR_MAX,
        JSL__MULTIMAP_TOMBSTONE = UINTPTR_MAX - 1u
    };

    struct JSL__StrToStrMultimapProbeResult {
        int64_t lut_index;
        // index of first tombstone seen, or -1
        int64_t first_tombstone;
        // bitpacking isn't really important here since this is a result struct
        bool tombstone_seen;
        bool found;
    };

    #define JSL__CHUNK_SIZE 32
    // This is probably an unnecessary optimization but w/e
    #define JSL__DIV_BY_32(x) x >> 5L

    struct JSL__StrToStrMultimapEntry {
        JSLFatPtr key;
        struct JSL__StrToStrMultimapValueNode* values_head;
        uint64_t hash;
        enum JSLStrToStrMultimapKeyState state;
    };

    typedef struct JSLStrToStrMultimap {
        JSLArena* arena;

        uintptr_t* entry_lookup_table;
        int64_t entry_lookup_table_length;

        struct JSL__StrToStrMultimapChunk* chunks_list_head;
        struct JSL__StrToStrMultimapChunk* chunks_list_tail;
        int64_t chunks_count; // capacity
        int64_t item_count;

        struct JSL__StrToStrMultimapValue* value_free_list;

        uint64_t hash_seed;
        float load_factor;
    } JSLStrToStrMultimap;

    typedef struct JSLStrToStrMultimapKeyValueIter {
        JSLStrToStrMultimap* map;
    } JSLStrToStrMultimapKeyValueIter;

    typedef struct JSLStrToStrMultimapGetValueIter {
        JSLStrToStrMultimap* map;
    } JSLStrToStrMultimapGetValueIter;


    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_init(
        JSLStrToStrMultimap* map,
        JSLArena* arena,
        int64_t item_count_guess,
        uint64_t seed,
        float load_factor
    );

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_insert(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLStringLifeTime key_lifetime,
        JSLFatPtr value,
        JSLStringLifeTime value_lifetime
    );

    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    );

    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_key_value_iterator_init(
        JSLStrToStrMultimap* map,
        JSLStrToStrMultimapKeyValueIter* iterator
    );

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_key_value_iterator_next(
        JSLStrToStrMultimapKeyValueIter* iterator,
        JSLFatPtr* out_key,
        JSLFatPtr* out_value
    );

    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_get_key_iterator_init(
        JSLStrToStrMultimap* map,
        JSLStrToStrMultimapGetValueIter* iterator,
        JSLFatPtr key
    );

    JSL_STR_TO_STR_MULTIMAP_DEF JSLFatPtr jsl_str_to_str_multimap_get_key_iterator_next(
        JSLStrToStrMultimapGetValueIter* iterator
    );

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_key(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    );

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_delete_value(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLFatPtr value
    );

    JSL_STR_TO_STR_MULTIMAP_DEF void jsl_str_to_str_multimap_clear(
        JSLStrToStrMultimap* map
    );
    
    #ifdef __cplusplus
    }
    #endif
    
#endif /* JSL_STR_TO_STR_MULTIMAP_H_INCLUDED */

#ifdef JSL_STR_TO_STR_MULTIMAP_IMPLEMENTATION

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_init(
        JSLStrToStrMultimap* map,
        JSLArena* arena,
        int64_t item_count_guess,
        uint64_t seed,
        float load_factor
    )
    {
        bool res = true;

        if (
            map == NULL
            || arena == NULL
            || item_count_guess < 0
            || load_factor < 0.0f
        )
            res = false;

        if (res)
        {
            JSL_MEMSET(map, 0, sizeof(JSLStrToStrMultimap));
            map->arena = arena;
            map->load_factor = load_factor;
            map->hash_seed = seed;

            item_count_guess = JSL_MAX(32, item_count_guess);
            int64_t items = jsl_next_power_of_two_u64(item_count_guess + 1);

            map->entry_lookup_table = (uintptr_t*) jsl_arena_allocate_aligned(
                arena,
                (uintptr_t) sizeof(uintptr_t) * items,
                _Alignof(uintptr_t),
                false
            ).data;
            JSL_MEMSET(map->entry_lookup_table, JSL__MULTIMAP_EMPTY, sizeof(uintptr_t) * items);
            map->entry_lookup_table_length = items;
        }

        return res;
    }

    static bool jsl__str_to_str_multimap_rehash(
        JSLStrToStrMultimap* map
    )
    {
        return false;
    }

    static void jsl__probe(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        struct JSL__StrToStrMultimapProbeResult* out_result
    )
    {
        out_result->found = false;
        out_result->tombstone_seen = false;

        uint64_t hash = jsl__rapidhash_withSeed(key.data, key.length, map->hash_seed);
        int64_t lut_index = (int64_t) (hash & (map->entry_lookup_table_length - 1));

        for (;;)
        {
            uintptr_t lut_res = map->entry_lookup_table[lut_index];

            if (lut_res == JSL__MULTIMAP_EMPTY)
            {
                out_result->lut_index = lut_index;
                break;
            }
            else if (lut_res == JSL__MULTIMAP_TOMBSTONE)
            {
                if (!out_result->tombstone_seen)
                {
                    out_result->first_tombstone = lut_index;
                    out_result->tombstone_seen = true;
                }
            }
            else
            {

            }
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
            map == NULL
            || key.data == NULL 
            || key.length < 0
            || value.data == NULL
            || value.length < 0
        );

        uint64_t capacity = (uint64_t) (map->chunks_count * JSL__CHUNK_SIZE);

        bool needs_rehash = false;
        if (res)
        {
            float current_load_factor = (float) map->item_count / (float) capacity;
            needs_rehash = current_load_factor >= map->load_factor;
        }

        if (needs_rehash)
        {
            res = jsl__str_to_str_multimap_rehash(map);
        }

        if (res)
        {
        }

        if (res)
        {
            ++map->item_count;
        }

        return res;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF int64_t jsl_str_to_str_multimap_get_value_count(
        JSLStrToStrMultimap* map,
        JSLFatPtr key
    )
    {
        (void) map;
        (void) key;
        return -1;
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
        JSLStrToStrMultimapGetValueIter* iterator,
        JSLFatPtr key
    )
    {
        (void) map;
        (void) iterator;
        (void) key;
        return;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF JSLFatPtr jsl_str_to_str_multimap_get_key_iterator_next(
        JSLStrToStrMultimapGetValueIter* iterator
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

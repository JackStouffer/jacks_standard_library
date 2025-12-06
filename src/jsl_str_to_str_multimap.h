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


    typedef struct JSLStrToStrMultimap {
        JSLArena* arena;
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
        (void) map;
        (void) arena;
        (void) item_count_guess;
        (void) seed;
        (void) load_factor;
        return false;
    }

    JSL_STR_TO_STR_MULTIMAP_DEF bool jsl_str_to_str_multimap_insert(
        JSLStrToStrMultimap* map,
        JSLFatPtr key,
        JSLStringLifeTime key_lifetime,
        JSLFatPtr value,
        JSLStringLifeTime value_lifetime
    )
    {
        (void) map;
        (void) key;
        (void) key_lifetime;
        (void) value;
        (void) value_lifetime;
        return false;
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

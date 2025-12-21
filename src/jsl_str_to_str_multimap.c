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

#include <stdint.h>
#include <stddef.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_hash_map_common.h"
#include "jsl_str_to_str_multimap.h"

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

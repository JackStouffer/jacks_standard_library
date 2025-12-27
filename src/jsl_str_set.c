
#include <stdint.h>
#include <stddef.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_hash_map_common.h"
#include "jsl_str_set.h"

#define JSL__SET_PRIVATE_SENTINEL 4086971745778309672U

JSL_STR_SET_DEF bool jsl_str_set_init(
    JSLStrSet* set,
    JSLArena* arena,
    uint64_t seed
)
{
    return jsl_str_set_init2(
        set,
        arena,
        seed,
        32,
        0.75f
    );
}

JSL_STR_SET_DEF bool jsl_str_set_init2(
    JSLStrSet* set,
    JSLArena* arena,
    uint64_t seed,
    int64_t item_count_guess,
    float load_factor
)
{
    bool res = (
        set != NULL
        && arena != NULL
        && item_count_guess > 0
        && load_factor > 0.0f
        && load_factor < 1.0f
    );

    if (res)
    {
        JSL_MEMSET(set, 0, sizeof(JSLStrSet));
        set->arena = arena;
        set->load_factor = load_factor;
        set->hash_seed = seed;

        item_count_guess = JSL_MAX(32L, item_count_guess);
        int64_t items = jsl_next_power_of_two_i64(item_count_guess + 1);

        set->entry_lookup_table = (uintptr_t*) jsl_arena_allocate_aligned(
            arena,
            (int64_t) sizeof(uintptr_t) * items,
            _Alignof(uintptr_t),
            true
        ).data;
        
        set->entry_lookup_table_length = items;

        set->sentinel = JSL__SET_PRIVATE_SENTINEL;
    }

    return res;
}

JSL_STR_SET_DEF int64_t jsl_str_set_item_count(
    JSLStrSet* set
)
{
    return set != NULL && set->sentinel == JSL__SET_PRIVATE_SENTINEL
        ? set->item_count
        : -1;
}

static inline void jsl__str_set_probe(
    JSLStrSet* map,
    JSLFatPtr value,
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

    *out_hash = jsl__rapidhash_withSeed(value.data, (size_t) value.length, map->hash_seed);

    int64_t lut_length = map->entry_lookup_table_length;
    uint64_t lut_mask = (uint64_t) lut_length - 1u;
    int64_t lut_index = (int64_t) (*out_hash & lut_mask);
    int64_t num_probes = 0;

    while (num_probes < lut_length)
    {
        uintptr_t lut_res = map->entry_lookup_table[lut_index];

        bool is_empty = lut_res == JSL__HASHMAP_EMPTY;
        bool is_tombstone = lut_res == JSL__HASHMAP_TOMBSTONE;

        if (is_empty)
        {
            *out_lut_index = tombstone_seen ? first_tombstone : lut_index;
            break;
        }

        if (is_tombstone && !tombstone_seen)
        {
            first_tombstone = lut_index;
            tombstone_seen = true;
        }

        bool slot_has_entry = !is_empty && !is_tombstone;
        struct JSL__StrSetEntry* entry = slot_has_entry
            ? (struct JSL__StrSetEntry*) lut_res
            : NULL;

        bool matches = entry != NULL
            && *out_hash == entry->hash
            && jsl_fatptr_memory_compare(value, entry->value);

        if (matches)
        {
            *out_found = true;
            *out_lut_index = lut_index;
            break;
        }

        if (entry == NULL)
        {
            map->entry_lookup_table[lut_index] = JSL__HASHMAP_TOMBSTONE;
            ++map->tombstone_count;
        }

        if (entry == NULL && !tombstone_seen)
        {
            first_tombstone = lut_index;
            tombstone_seen = true;
        }

        if (searching)
        {
            lut_index = (int64_t) (((uint64_t) lut_index + 1u) & lut_mask);
            ++num_probes;
        }
    }

    if (num_probes >= lut_length)
    {
        *out_lut_index = tombstone_seen ? first_tombstone : -1;
    }
}

JSL_STR_SET_DEF bool jsl_str_set_has(
    JSLStrSet* set,
    JSLFatPtr value
)
{
    uint64_t hash = 0;
    int64_t lut_index = -1;
    bool existing_found = false;

    if (
        set != NULL
        && set->sentinel == JSL__SET_PRIVATE_SENTINEL
        && value.data != NULL
        && value.length > -1
    )
    {
        jsl__str_set_probe(set, value, &lut_index, &hash, &existing_found);
    }

    return lut_index > -1 && existing_found;
}

static JSL__FORCE_INLINE bool jsl__str_set_add(
    JSLStrSet* map,
    JSLFatPtr value,
    JSLStringLifeTime value_lifetime,
    int64_t lut_index,
    uint64_t hash
)
{
    struct JSL__StrSetEntry* entry = NULL;
    bool replacing_tombstone = map->entry_lookup_table[lut_index] == JSL__HASHMAP_TOMBSTONE;

    if (map->entry_free_list == NULL)
    {
        entry = JSL_ARENA_TYPED_ALLOCATE(struct JSL__StrSetEntry, map->arena);
    }
    else
    {
        struct JSL__StrSetEntry* next = map->entry_free_list->next;
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
    // Copy the value
    // 

    if (entry != NULL && value_lifetime == JSL_STRING_LIFETIME_STATIC)
    {
        entry->value = value;
    }
    else if (
        entry != NULL
        && value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
        && value.length <= JSL__SET_SSO_LENGTH
    )
    {
        JSL_MEMCPY(entry->value_sso_buffer, value.data, (size_t) value.length);
        entry->value.data = entry->value_sso_buffer;
        entry->value.length = value.length;
    }
    else if (
        entry != NULL
        && value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
        && value.length > JSL__SET_SSO_LENGTH
    )
    {
        entry->value = jsl_fatptr_duplicate(map->arena, value);
    }

    return entry != NULL;
}

static JSL__FORCE_INLINE void jsl__str_set_update_value(
    JSLStrSet* map,
    JSLFatPtr value,
    JSLStringLifeTime value_lifetime,
    int64_t lut_index
)
{
    uintptr_t lut_res = map->entry_lookup_table[lut_index];
    struct JSL__StrSetEntry* entry = (struct JSL__StrSetEntry*) lut_res;

    if (value_lifetime == JSL_STRING_LIFETIME_STATIC)
    {
        entry->value = value;
    }
    else if (
        value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
        && value.length <= JSL__SET_SSO_LENGTH
    )
    {
        JSL_MEMCPY(entry->value_sso_buffer, value.data, (size_t) value.length);
        entry->value.data = entry->value_sso_buffer;
        entry->value.length = value.length;
    }
    else if (
        value_lifetime == JSL_STRING_LIFETIME_TRANSIENT
        && value.length > JSL__SET_SSO_LENGTH
    )
    {
        entry->value = jsl_fatptr_duplicate(map->arena, value);
    }
}

JSL_STR_SET_DEF bool jsl_str_set_insert(
    JSLStrSet* set,
    JSLFatPtr value,
    JSLStringLifeTime value_lifetime
)
{ 
    bool res = (
        set != NULL
        && set->sentinel == JSL__SET_PRIVATE_SENTINEL
        && value.data != NULL
        && value.length > -1
    );

    bool needs_rehash = false;
    if (res)
    {
        float occupied_count = (float) (set->item_count + set->tombstone_count);
        float current_load_factor =  occupied_count / (float) set->entry_lookup_table_length;
        bool too_many_tombstones = set->tombstone_count > (set->entry_lookup_table_length / 4);
        needs_rehash = current_load_factor >= set->load_factor || too_many_tombstones;
    }

    if (JSL__UNLIKELY(needs_rehash))
    {
        res = jsl__str_set_rehash(set);
    }

    uint64_t hash = 0;
    int64_t lut_index = -1;
    bool existing_found = false;
    if (res)
    {
        jsl__str_set_probe(map, key, &lut_index, &hash, &existing_found);
    }
    
    // new key
    if (lut_index > -1 && !existing_found)
    {
        res = jsl__str_set_add(
            set,
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

JSL_STR_SET_DEF bool jsl_str_set_iterator_init(
    JSLStrSet* set,
    JSLStrSetKeyValueIter* iterator
)
{

}

JSL_STR_SET_DEF bool jsl_str_set_iterator_next(
    JSLStrSetKeyValueIter* iterator,
    JSLFatPtr* out_value
)
{

}

JSL_STR_SET_DEF bool jsl_str_set_delete(
    JSLStrSet* set,
    JSLFatPtr key
)
{

}

JSL_STR_SET_DEF void jsl_str_set_clear(
    JSLStrSet* set
)
{

}

#undef JSL__SET_PRIVATE_SENTINEL

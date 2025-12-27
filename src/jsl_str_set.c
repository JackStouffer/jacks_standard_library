
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
    bool res = true;

    if (
        set == NULL
        || arena == NULL
        || item_count_guess <= 0
        || load_factor <= 0.0f
        || load_factor >= 1.0f
    )
        res = false;

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
    int64_t res = -1;

    if (
        set != NULL
        && set->sentinel == JSL__SET_PRIVATE_SENTINEL
    )
    {
        res = set->item_count;
    }

    return res;
}

JSL_STR_SET_DEF bool jsl_str_set_has(
    JSLStrSet* set,
    JSLFatPtr value
)
{

}

static JSL__FORCE_INLINE bool jsl__str_to_str_map_add(
    JSLStrSet* map,
    JSLFatPtr value,
    JSLStringLifeTime value_lifetime,
    int64_t lut_index,
    uint64_t hash
)
{
    struct JSL__StrToStrMapEntry* entry = NULL;
    bool replacing_tombstone = map->entry_lookup_table[lut_index] == JSL__HASHMAP_TOMBSTONE;

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

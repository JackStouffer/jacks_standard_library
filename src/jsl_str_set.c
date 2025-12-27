
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

static bool jsl__str_set_rehash(
    JSLStrSet* set
)
{
    bool res = false;

    bool params_valid = (
        set != NULL
        && set->arena != NULL
        && set->sentinel == JSL__SET_PRIVATE_SENTINEL
        && set->entry_lookup_table != NULL
        && set->entry_lookup_table_length > 0
    );

    uintptr_t* old_table = params_valid ? set->entry_lookup_table : NULL;
    int64_t old_length = params_valid ? set->entry_lookup_table_length : 0;

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
            set->arena,
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
            && lut_res != JSL__HASHMAP_EMPTY
            && lut_res != JSL__HASHMAP_TOMBSTONE
        );

        struct JSL__StrSetEntry* entry = occupied
            ? (struct JSL__StrSetEntry*) lut_res
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
                probe_res == JSL__HASHMAP_EMPTY
                || probe_res == JSL__HASHMAP_TOMBSTONE
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
        set->entry_lookup_table = new_table;
        set->entry_lookup_table_length = new_length;
        set->tombstone_count = 0;
        ++set->generational_id;
        res = true;
    }

    bool failed = !should_commit;
    if (failed)
    {
        res = false;
    }

    return res;
}

static inline void jsl__str_set_probe(
    JSLStrSet* set,
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

    *out_hash = jsl__rapidhash_withSeed(value.data, (size_t) value.length, set->hash_seed);

    int64_t lut_length = set->entry_lookup_table_length;
    uint64_t lut_mask = (uint64_t) lut_length - 1u;
    int64_t lut_index = (int64_t) (*out_hash & lut_mask);
    int64_t num_probes = 0;

    while (num_probes < lut_length)
    {
        uintptr_t lut_res = set->entry_lookup_table[lut_index];

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
            set->entry_lookup_table[lut_index] = JSL__HASHMAP_TOMBSTONE;
            ++set->tombstone_count;
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
    JSLStrSet* set,
    JSLFatPtr value,
    JSLStringLifeTime value_lifetime,
    int64_t lut_index,
    uint64_t hash
)
{
    struct JSL__StrSetEntry* entry = NULL;
    bool replacing_tombstone = set->entry_lookup_table[lut_index] == JSL__HASHMAP_TOMBSTONE;

    // 
    // Allocate a new entry or copy one from the free list
    // 

    if (set->entry_free_list == NULL)
    {
        entry = JSL_ARENA_TYPED_ALLOCATE(struct JSL__StrSetEntry, set->arena);
    }
    else
    {
        struct JSL__StrSetEntry* next = set->entry_free_list->next;
        entry = set->entry_free_list;
        set->entry_free_list = next;
    }

    if (entry != NULL)
    {
        entry->hash = hash;
        
        set->entry_lookup_table[lut_index] = (uintptr_t) entry;
        ++set->item_count;
    }

    if (entry != NULL && replacing_tombstone)
    {
        --set->tombstone_count;
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
        entry->value = jsl_fatptr_duplicate(set->arena, value);
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
        jsl__str_set_probe(set, value, &lut_index, &hash, &existing_found);
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

    if (res)
    {
        ++set->generational_id;
    }

    return res;
}

JSL_STR_SET_DEF bool jsl_str_set_iterator_init(
    JSLStrSet* set,
    JSLStrSetKeyValueIter* iterator
)
{
    bool res = false;

    if (
        set != NULL
        && set->sentinel == JSL__SET_PRIVATE_SENTINEL
        && iterator != NULL
    )
    {
        iterator->set = set;
        iterator->current_lut_index = 0;
        iterator->sentinel = JSL__SET_PRIVATE_SENTINEL;
        iterator->generational_id = set->generational_id;
        res = true;
    }

    return res;
}

JSL_STR_SET_DEF bool jsl_str_set_iterator_next(
    JSLStrSetKeyValueIter* iterator,
    JSLFatPtr* out_value
)
{
    bool found = false;

    bool params_valid = (
        iterator != NULL
        && out_value != NULL
        && iterator->sentinel == JSL__SET_PRIVATE_SENTINEL
        && iterator->set != NULL
        && iterator->set->sentinel == JSL__SET_PRIVATE_SENTINEL
        && iterator->set->entry_lookup_table != NULL
        && iterator->generational_id == iterator->set->generational_id
    );

    int64_t lut_length = params_valid ? iterator->set->entry_lookup_table_length : 0;
    int64_t lut_index = iterator->current_lut_index;
    struct JSL__StrSetEntry* found_entry = NULL;

    while (params_valid && lut_index < lut_length)
    {
        uintptr_t lut_res = iterator->set->entry_lookup_table[lut_index];
        bool occupied = lut_res != JSL__HASHMAP_EMPTY && lut_res != JSL__HASHMAP_TOMBSTONE;

        if (occupied)
        {
            found_entry = (struct JSL__StrSetEntry*) lut_res;
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

JSL_STR_SET_DEF bool jsl_str_set_delete(
    JSLStrSet* set,
    JSLFatPtr value
)
{
    bool res = false;

    bool params_valid = (
        set != NULL
        && set->sentinel == JSL__SET_PRIVATE_SENTINEL
        && set->entry_lookup_table != NULL
        && value.data != NULL
        && value.length > -1
    );

    uint64_t hash = 0;
    int64_t lut_index = -1;
    bool existing_found = false;
    if (params_valid)
    {
        jsl__str_set_probe(set, value, &lut_index, &hash, &existing_found);
    }

    if (existing_found && lut_index > -1)
    {
        struct JSL__StrSetEntry* entry =
            (struct JSL__StrSetEntry*) set->entry_lookup_table[lut_index];

        entry->next = set->entry_free_list;
        set->entry_free_list = entry;

        --set->item_count;
        ++set->generational_id;

        set->entry_lookup_table[lut_index] = JSL__HASHMAP_TOMBSTONE;
        ++set->tombstone_count;

        res = true;
    }

    return res;
}

JSL_STR_SET_DEF void jsl_str_set_clear(
    JSLStrSet* set
)
{
    bool params_valid = (
        set != NULL
        && set->sentinel == JSL__SET_PRIVATE_SENTINEL
        && set->entry_lookup_table != NULL
    );

    int64_t lut_length = params_valid ? set->entry_lookup_table_length : 0;
    int64_t index = 0;

    while (params_valid && index < lut_length)
    {
        uintptr_t lut_res = set->entry_lookup_table[index];

        if (lut_res != JSL__HASHMAP_EMPTY && lut_res != JSL__HASHMAP_TOMBSTONE)
        {
            struct JSL__StrSetEntry* entry = (struct JSL__StrSetEntry*) lut_res;
            entry->next = set->entry_free_list;
            set->entry_free_list = entry;
            set->entry_lookup_table[index] = JSL__HASHMAP_EMPTY;
        }
        else if (lut_res == JSL__HASHMAP_TOMBSTONE)
        {
            set->entry_lookup_table[index] = JSL__HASHMAP_EMPTY;
        }

        ++index;
    }

    if (params_valid)
    {
        set->item_count = 0;
        set->tombstone_count = 0;
        ++set->generational_id;
    }

    return;
}

JSL_STR_SET_DEF bool jsl_str_set_intersection(
    JSLStrSet* a,
    JSLStrSet* b,
    JSLStrSet* out
)
{
    bool params_valid = (
        a != NULL
        && b != NULL
        && out != NULL
        && a->sentinel == JSL__SET_PRIVATE_SENTINEL
        && a->entry_lookup_table != NULL
        && b->sentinel == JSL__SET_PRIVATE_SENTINEL
        && b->entry_lookup_table != NULL
        && out->sentinel == JSL__SET_PRIVATE_SENTINEL
        && out->entry_lookup_table != NULL
    );

    JSLStrSet* smaller = NULL;
    JSLStrSet* larger = NULL;

    if (params_valid)
    {
        smaller = a->item_count <= b->item_count ? a : b;
        larger = a->item_count > b->item_count ? a : b;
    }

    JSLStrSetKeyValueIter iterator = {0};
    bool valid_iterator = false;
    if (smaller != NULL && larger != NULL)
    {
        jsl_str_set_iterator_init(smaller, &iterator);
    }

    bool success = true;
    JSLFatPtr out_value = {0};
    while (jsl_str_set_iterator_next(&iterator, &out_value))
    {
        if (jsl_str_set_has(larger, out_value) && !jsl_str_set_insert(out, out_value, JSL_STRING_LIFETIME_TRANSIENT))
        {
            success = false;
            break;
        }
    }

    return success;
}

JSL_STR_SET_DEF bool jsl_str_set_union(
    JSLStrSet* a,
    JSLStrSet* b,
    JSLStrSet* out
)
{
    bool params_valid = (
        a != NULL
        && b != NULL
        && out != NULL
        && a->sentinel == JSL__SET_PRIVATE_SENTINEL
        && a->entry_lookup_table != NULL
        && b->sentinel == JSL__SET_PRIVATE_SENTINEL
        && b->entry_lookup_table != NULL
        && out->sentinel == JSL__SET_PRIVATE_SENTINEL
        && out->entry_lookup_table != NULL
    );

    JSLStrSetKeyValueIter a_iterator = {0};
    JSLStrSetKeyValueIter b_iterator = {0};

    if (params_valid)
    {
        jsl_str_set_iterator_init(a, &a_iterator);
        jsl_str_set_iterator_init(b, &b_iterator);
    }

    bool success = true;
    JSLFatPtr out_value = {0};
    while (jsl_str_set_iterator_next(&a_iterator, &out_value))
    {
        if (!jsl_str_set_insert(out, out_value, JSL_STRING_LIFETIME_TRANSIENT))
        {
            success = false;
            break;
        }
    }

    if (params_valid && success)
    {
        while (jsl_str_set_iterator_next(&b_iterator, &out_value))
        {
            if (!jsl_str_set_insert(out, out_value, JSL_STRING_LIFETIME_TRANSIENT))
            {
                success = false;
                break;
            }
        }
    }

    return params_valid && success;
}

JSL_STR_SET_DEF bool jsl_str_set_difference(
    JSLStrSet* a,
    JSLStrSet* b,
    JSLStrSet* out
)
{
    bool params_valid = (
        a != NULL
        && b != NULL
        && out != NULL
        && a->sentinel == JSL__SET_PRIVATE_SENTINEL
        && a->entry_lookup_table != NULL
        && b->sentinel == JSL__SET_PRIVATE_SENTINEL
        && b->entry_lookup_table != NULL
        && out->sentinel == JSL__SET_PRIVATE_SENTINEL
        && out->entry_lookup_table != NULL
    );

    JSLStrSetKeyValueIter iterator = {0};
    jsl_str_set_iterator_init(a, &iterator);

    bool success = true;
    JSLFatPtr out_value = {0};
    while (jsl_str_set_iterator_next(&iterator, &out_value))
    {
        bool do_insert = jsl_str_set_has(b, out_value) == false;
        if (do_insert && !jsl_str_set_insert(out, out_value, JSL_STRING_LIFETIME_TRANSIENT))
        {
            success = false;
            break;
        }
    }

    return success;
}

#undef JSL__SET_PRIVATE_SENTINEL

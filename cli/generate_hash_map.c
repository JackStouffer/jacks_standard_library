/**
 *
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

#ifdef INCLUDE_MAIN
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef INCLUDE_MAIN
    #define JSL_CORE_IMPLEMENTATION
#endif
#include "../src/jsl_core.h"

#ifdef INCLUDE_MAIN
    #define JSL_STRING_BUILDER_IMPLEMENTATION
#endif
#include "../src/jsl_string_builder.h"

#ifdef INCLUDE_MAIN
    #define JSL_FILES_IMPLEMENTATION
#endif
#include "../src/jsl_files.h"


#include "generate_hash_map.h"

/**
 * TODO: Documentation: talk about
 *  - must use arena with lifetime greater than the hash_map
 *  - flat hash map with open addressing
 *  - uses PRNG hash, so protected against hash flooding
 *  - large init bucket size because rehashing is expensive
 *  - aggressive growth rate with .5 load factor
 *  - pow 2 bucket size
 *  - large memory usage
 *  - doesn't give up when runs out of memory so you can use a separate arena
 *  - generational ids
 *  - Give warning about composite keys and zero initialization, garbage memory in the padding
 */

JSLFatPtr static_hash_map_docstring = JSL_FATPTR_INITIALIZER("/**\n"
" * AUTO GENERATED FILE\n"
" *\n"
" * This file contains the %y for a hash map `%y` which maps\n"
" * `%y` keys to `%y` values.\n"
" *\n"
" * This file was auto generated from the hash map generation utility that's part of the \"Jack's Standard Library\" project.\n"
" * The utility generates a header file and a C file for a type safe, open addressed, hash map.\n"
" * By generating the code rather than using macros, two benefits are gained. One, the code is\n"
" * much easier to debug. Two, it's much more obvious how much code you're generating, which means\n"
" * you are much less likely to accidentally create the combinatoric explosion of code that's\n"
" * so common in C++ projects. Adding friction to things is actually good sometimes.\n"
" *\n"
" * Much like the arena allocator it uses, this hash map is designed for situations where\n"
" * you can set an upper bound on the number of items you will have and that upper bound is\n"
" * still a reasonable amount of memory. This represents the vast majority case, as most hash\n"
" * maps will never have more than 100 items. Even in cases where the struct is quite large\n"
" * e.g. over a kilobyte, and you have a large upper bound, say 100k, thats still ~100MB of\n"
" * data. This is an incredibly rare case and you probably only have one of these in your\n"
" * program; this hash map would still work for that case.\n"
" *\n"
" * This hash map is not suited for cases where the hash map will shrink and grow quite\n"
" * substantially or there's no known upper bound. The most common example would be user\n"
" * input that cannot reasonably be limited, e.g. a word processing application cannot simply\n"
" * refuse to open very large (+10gig) documents. If you have some hash map which is built\n"
" * from the document file then you need some other allocation strategy (you probably don't\n"
" * want a normal hash map either as you'd be streaming things in and out of memory).\n"
" */\n\n");

JSLFatPtr static_map_type_typedef = JSL_FATPTR_INITIALIZER("/**\n"
" * A hash map which maps `%y` keys to `%y` values.\n"
" *\n"
" * This hash map uses open addressing with linear probing. However, it never grows.\n"
" * When initalized with the init function, all the memory this hash map will have\n"
" * is allocated right away.\n"
" */\n"
"typedef struct %y {\n"
"    %y* keys_array;\n"
"    %y* items_array;\n"
"    /** length of both keys_array and items_array */\n"
"    int64_t arrays_length;\n"
"    uint32_t* is_set_flags_array;\n"
"    int64_t is_set_flags_array_length;\n"
"    int64_t item_count;\n"
"    int64_t max_item_count;\n"
"    uint64_t seed;\n"
"    uint16_t generational_id;\n"
"} %y;\n"
"\n"
);

JSLFatPtr static_map_iterator_typedef = JSL_FATPTR_INITIALIZER("/**\n"
" * Iterator type which is used by the iterator functions to\n"
" * allow you to loop over the hash map contents.\n"
" */\n"
"typedef struct %yIterator {\n"
"    %y* hash_map;\n"
"    int64_t current_slot_index;\n"
"    uint16_t generational_id;\n"
"} %yIterator;\n"
"\n"
);

JSLFatPtr static_find_res_struct = JSL_FATPTR_INITIALIZER(""
"struct %yFindRes {\n"
"    int64_t value_index;\n"
"    int64_t is_set_array_index;\n"
"    uint32_t is_set_array_bit;\n"
"    bool is_update;\n"
"};\n"
"\n"
);

JSLFatPtr static_init_function_signature = JSL_FATPTR_INITIALIZER("/**\n"
" * Initialize an instance of the hash map.\n"
" *\n"
" * All of the memory that this hash map will need will be allocated from the passed in arena.\n"
" * The hash map does not save a reference to the arena, but the arena memory must have the same\n"
" * or greater lifetime than the hash map itself.\n"
" *\n"
" * As this hash map does not grow, the speed of insertion and retrieval will decrease\n"
" * exponentially as the load factor approaches 1. The true internal max item count is\n"
" * the next highest power of two of the given parameter with a minimum value of 32.\n"
" *\n"
" * @warning This hash map uses a well distributed hash. But in order to properly protect against\n"
" * hash flooding attacks you must do two things. One, provide good random data for the\n"
" * seed value. This means using your OS's secure random number generator, not `rand`.\n"
" * As this is very platform specific JSL does not come with a mechanism for getting these\n"
" * random numbers; you must do it yourself. Two, use a different seed value as often as\n"
" * possible, ideally every user interaction. This would make hash flooding attacks almost\n"
" * impossible. If you are absolutely sure that this hash map cannot be attacked with hash\n"
" * flooding then zero is a valid seed value.\n"
" *\n"
" * @param hash_map The pointer to the hash map instance to initialize\n"
" * @param arena The arena that this hash map will use to allocate memory\n"
" * @param max_item_count The maximum amount of items this hash map can hold\n"
" * @param seed Seed value for the hash function to protect against hash flooding attacks\n"
" */\n"
"bool %y_init(\n"
"    %y* hash_map,\n"
"    JSLArena* arena,\n"
"    int64_t max_item_count,\n"
"    uint64_t seed\n"
");\n\n");

JSLFatPtr static_insert_function_signature = JSL_FATPTR_INITIALIZER("/**\n"
" * Insert the given value into the hash map. If the key already exists in "
" * the map the value will be overwritten. If the key type for this hash map\n"
" * is a pointer, then a NULL key is a valid key type.\n"
" *\n"
" * @param hash_map The pointer to the hash map instance to initialize\n"
" * @param key Hash map key\n"
" * @param value Value to store\n"
" * @returns A bool representing success or failure of insertion.\n"
" */\n"
"bool %y_insert(\n"
"    %y* hash_map,\n"
"    %y key,\n"
"    %y value\n"
");\n\n");

JSLFatPtr static_get_function_signature = JSL_FATPTR_INITIALIZER("/**\n"
" * Get a value from the hash map if it exists. If it does not NULL is returned\n"
" *\n"
" * The pointer returned actually points to value stored inside of hash map.\n"
" * You can change the value though the pointer.\n"
" *\n"
" * @param hash_map The pointer to the hash map instance to initialize\n"
" * @param key Hash map key\n"
" * @param value Value to store\n"
" * @returns The pointer to the value in the hash map, or null.\n"
" */\n"
"%y* %y_get(\n"
"    %y* hash_map,\n"
"    %y key\n"
");\n\n"
);

JSLFatPtr static_delete_function_signature = JSL_FATPTR_INITIALIZER("/**\n"
" * Remove a key/value pair from the hash map if it exists.\n"
" * If it does not false is returned\n"
" */\n"
"bool %y_delete(\n"
"    %y* hash_map,\n"
"    %y key\n"
");\n\n"
);

JSLFatPtr static_iterator_start_function_signature = JSL_FATPTR_INITIALIZER("/**\n"
" * Create a new iterator over this hash map.\n"
" *\n"
" * An iterator is a struct which holds enough state that it allows a loop to visit\n"
" * each key/value pair in the hash map.\n"
" *\n"
" * Iterating over a hash map while adding items does not have guaranteed\n"
" * correctness. Deleting items will iterating over this map does have the\n"
" * correct behavior."
" *\n"
" * Example usage:\n"
" * @code\n"
" * %y key;\n"
" * %y value;\n"
" * %yIterator iterator;\n"
" * %y_iterator_start(hash_map, &iterator);\n"
" * while (%y_iterator_next(&iterator, &key, &value))\n"
" * {\n"
" *     ...\n"
" * }\n"
" * @endcode\n"
" */\n"
"bool %y_iterator_start(\n"
"    %y* hash_map,\n"
"    %yIterator* iterator\n"
");\n\n");

JSLFatPtr static_iterator_next_function_signature = JSL_FATPTR_INITIALIZER("/**\n"
" * Iterate over the hash map. If a key/value was found then true is returned.\n"
" *\n"
" * Example usage:\n"
" * @code\n"
" * %y key;\n"
" * %y value;\n"
" * %y iterator = %y_iterator_start(hash_map);\n"
" * while (%y_iterator_next(&iterator, &key, &value))\n"
" * {\n"
" *     ...\n"
" * }\n"
" * @endcode\n"
" */\n"
"bool %y_iterator_next(\n"
"    %yIterator* iterator,\n"
"    %y* key,\n"
"    %y* value\n"
");\n\n"
);

JSLFatPtr static_init_function_code = JSL_FATPTR_INITIALIZER(""
"bool %y_init(\n"
"    %y* hash_map,\n"
"    JSLArena* arena,\n"
"    int64_t max_item_count,\n"
"    uint64_t seed\n"
")\n"
"{\n"
"    JSL_MEMSET(hash_map, 0, sizeof(%y));\n"
"\n"
"    if (hash_map == NULL || arena == NULL || max_item_count < 0)\n"
"        return false;\n"
"\n"
"    hash_map->seed = seed;\n"
"    hash_map->max_item_count = max_item_count;\n"
"    hash_map->arrays_length = (int64_t) jsl_next_power_of_two_u64((uint64_t) (max_item_count + 2));\n"
"    hash_map->arrays_length = JSL_MAX(hash_map->arrays_length, 32);\n"
"    hash_map->is_set_flags_array_length = hash_map->arrays_length >> 5L;\n"
"\n"
"    hash_map->keys_array = (%y*) jsl_arena_allocate_aligned(\n"
"       arena,\n"
"       ((int64_t) sizeof(%y)) * hash_map->arrays_length,\n"
"       (int32_t) _Alignof(%y),\n"
"       false\n"
"    ).data;\n"
"    if (hash_map->keys_array == NULL)\n"
"        return false;\n"
"\n"
"    hash_map->items_array = (%y*) jsl_arena_allocate_aligned(\n"
"        arena,\n"
"        ((int64_t) sizeof(%y)) * hash_map->arrays_length,\n"
"        (int32_t) _Alignof(%y),\n"
"        false\n"
"    ).data;\n"
"    if (hash_map->items_array == NULL)\n"
"        return false;\n"
"\n"
"    hash_map->is_set_flags_array = (uint32_t*) jsl_arena_allocate(\n"
"        arena, ((int64_t) sizeof(uint32_t)) * hash_map->is_set_flags_array_length, true\n"
"    ).data;\n"
"    if (hash_map->is_set_flags_array == NULL)\n"
"        return false;\n"
"\n"
"    return true;\n"
"}\n\n");

JSLFatPtr static_hash_function_code = JSL_FATPTR_INITIALIZER(""
"static inline struct %yFindRes %y_hash_and_find_slot(\n"
"    %y* hash_map,\n"
"    %y key,\n"
"    bool is_insert\n"
")\n"
"{\n"
"    struct %yFindRes return_value;\n"
"    return_value.value_index = -1;\n"
"\n"
"    %y;\n"
"\n"
"    int64_t total_checked = 0;\n"
"    // Since our slot array length is always a pow 2, we can avoid a modulo\n"
"    int64_t slot_index = (int64_t) (hash & ((uint64_t) hash_map->arrays_length - 1u));\n"
"    return_value.is_set_array_index = (int64_t) JSL__HASH_MAP_GET_SET_FLAG_INDEX(slot_index);\n"
"    // Manual remainder here too\n"
"    return_value.is_set_array_bit = (uint32_t) (slot_index - (return_value.is_set_array_index * 32));\n"
"\n"
"    for (;;)\n"
"    {\n"
"        uint32_t bit_flag = JSL_MAKE_BITFLAG(return_value.is_set_array_bit);\n"
"        uint32_t is_slot_set = JSL_IS_BITFLAG_SET(\n"
"            hash_map->is_set_flags_array[return_value.is_set_array_index],\n"
"            bit_flag\n"
"        );\n"
"\n"
"        if (is_slot_set == 0 && is_insert)\n"
"        {\n"
"            return_value.value_index = slot_index;\n"
"            return_value.is_update = false;\n"
"            break;\n"
"        }\n"
"        /* Updating value */\n"
"        else if (is_slot_set == 1)\n"
"        {\n"
"            int32_t memcmp_res = JSL_MEMCMP(\n"
"                &hash_map->keys_array[slot_index],\n"
"                &key,\n"
"                sizeof(%y)\n"
"            );\n"
"            if (memcmp_res == 0)\n"
"            {\n"
"                return_value.value_index = slot_index;\n"
"                return_value.is_update = true;\n"
"                break;\n"
"            }\n"
"        }\n"
"\n"
"        // Collision. Move to the next spot with linear probing\n"
"\n"
"        ++total_checked;\n"
"        ++return_value.is_set_array_bit;\n"
"        ++slot_index;\n"
"\n"
"        // is the hash_map is completely full\n"
"        if (total_checked == hash_map->arrays_length)\n"
"        {\n"
"            break;\n"
"        }\n"
"\n"
"        // if we've reached the end of this is_set_array item, move on \n"
"        if (return_value.is_set_array_bit == 32)\n"
"        {\n"
"            ++return_value.is_set_array_index;\n"
"            return_value.is_set_array_bit = 0;\n"
"        }\n"
"\n"
"        // Loop all the way back around\n"
"        if (slot_index == hash_map->arrays_length)\n"
"        {\n"
"            slot_index = 0;\n"
"            return_value.is_set_array_bit = 0;\n"
"            return_value.is_set_array_index = 0;\n"
"        }\n"
"    }\n"
"\n"
"    return return_value;\n"
"}\n\n");

JSLFatPtr static_insert_function_code = JSL_FATPTR_INITIALIZER(""
"bool %y_insert(\n"
"    %y* hash_map,\n"
"    %y key,\n"
"    %y value\n"
")\n"
"{\n"
"    bool insert_success = false;\n"
"\n"
"    if (\n"
"        hash_map == NULL\n"
"        || hash_map->items_array == NULL\n"
"        || hash_map->keys_array == NULL\n"
"        || hash_map->is_set_flags_array == NULL\n"
"        || hash_map->item_count == hash_map->max_item_count\n"
"    )\n"
"        return insert_success;\n"
"\n"
"    if (hash_map->item_count == hash_map->arrays_length)\n"
"    {\n"
"        return insert_success;\n"
"    }\n"
"\n"
"    struct %yFindRes find_res = %y_hash_and_find_slot(\n"
"        hash_map,\n"
"        key,\n"
"        true\n"
"    );\n"
"    if (find_res.value_index != -1)\n"
"    {\n"
"        if (find_res.is_update)\n"
"        {\n"
"            hash_map->items_array[find_res.value_index] = value;\n"
"            insert_success = true;\n"
"        }\n"
"        else\n"
"        {\n"
"            hash_map->keys_array[find_res.value_index] = key;\n"
"            hash_map->items_array[find_res.value_index] = value;\n"
"            uint32_t bit_flag = JSL_MAKE_BITFLAG(find_res.is_set_array_bit);\n"
"            JSL_SET_BITFLAG(\n"
"                &hash_map->is_set_flags_array[find_res.is_set_array_index],\n"
"                bit_flag\n"
"            );\n"
"            ++hash_map->item_count;\n"
"            insert_success = true;\n"
"        }\n"
"\n"
"        ++hash_map->generational_id;\n"
"    }\n"
"\n"
"    return insert_success;\n"
"}\n\n");

JSLFatPtr static_get_function_code = JSL_FATPTR_INITIALIZER(""
"%y* %y_get(\n"
"    %y* hash_map,\n"
"    %y key\n"
")\n"
"{\n"
"    %y* res = NULL;\n"
"\n"
"    if (\n"
"        hash_map == NULL\n"
"        || hash_map->items_array == NULL\n"
"        || hash_map->keys_array == NULL\n"
"        || hash_map->is_set_flags_array == NULL\n"
"    )\n"
"        return res;\n"
"\n"
"    struct %yFindRes find_res = %y_hash_and_find_slot(hash_map, key, false);\n"
"    if (find_res.value_index != -1 && find_res.is_update)\n"
"    {\n"
"        res = &hash_map->items_array[find_res.value_index];\n"
"    }\n"
"\n"
"    return res;\n"
"}\n\n");

JSLFatPtr static_delete_function_code = JSL_FATPTR_INITIALIZER(""
"bool %y_delete(\n"
"    %y* hash_map,\n"
"    %y key\n"
")\n"
"{\n"
"    bool success = false;\n"
"\n"
"    if (\n"
"        hash_map == NULL\n"
"        || hash_map->items_array == NULL\n"
"        || hash_map->keys_array == NULL\n"
"        || hash_map->is_set_flags_array == NULL\n"
"    )\n"
"        return success;\n"
"\n"
"    struct %yFindRes find_res = %y_hash_and_find_slot(hash_map, key, false);\n"
"\n"
"    if (find_res.value_index != -1 && find_res.is_update)\n"
"    {\n"
"        uint32_t bit_flag = JSL_MAKE_BITFLAG(find_res.is_set_array_bit);\n"
"        JSL_UNSET_BITFLAG(\n"
"            &hash_map->is_set_flags_array[find_res.is_set_array_index],\n"
"            bit_flag\n"
"        );\n"
"        --hash_map->item_count;\n"
"        success = true;\n"
"    }\n"
"\n"
"    return success;\n"
"}\n\n");

JSLFatPtr static_iterator_start_function_code = JSL_FATPTR_INITIALIZER(""
"bool %y_iterator_start(\n"
"    %y* hash_map,\n"
"    %yIterator* iterator\n"
")\n"
"{\n"
"    bool success = false;\n"
"\n"
"    if (\n"
"        hash_map == NULL\n"
"        || hash_map->items_array == NULL\n"
"        || hash_map->keys_array == NULL\n"
"        || hash_map->is_set_flags_array == NULL\n"
"    )\n"
"        return success;\n"
"\n"
"    iterator->hash_map = hash_map;\n"
"    iterator->current_slot_index = 0;\n"
"    iterator->generational_id = hash_map->generational_id;\n"
"\n"
"    return iterator;\n"
"}\n\n");

JSLFatPtr static_iterator_next_function_code = JSL_FATPTR_INITIALIZER(""
"bool %y_iterator_next(\n"
"    %yIterator* iterator,\n"
"    %y* key,\n"
"    %y* value\n"
")\n"
"{\n"
"    bool result = false;\n"
"\n"
"    if (\n"
"        iterator == NULL\n"
"        || iterator->hash_map == NULL\n"
"        || iterator->hash_map->items_array == NULL\n"
"        || iterator->hash_map->keys_array == NULL\n"
"        || iterator->hash_map->is_set_flags_array == NULL\n"
"    )\n"
"        return result;\n"
"\n"
"    while (iterator->current_slot_index < iterator->hash_map->arrays_length)\n"
"    {\n"
"        int64_t is_set_flags_index = JSL__HASH_MAP_GET_SET_FLAG_INDEX(iterator->current_slot_index);\n"
"        uint32_t is_set_flags = iterator->hash_map->is_set_flags_array[is_set_flags_index];\n"
"        bool at_start_of_flags = (iterator->current_slot_index & 31) == 0;  // modulo 32\n"
"\n"
"        if (at_start_of_flags && is_set_flags == 0)\n"
"        {\n"
"            iterator->current_slot_index += 32;\n"
"        }\n"
"        else if (at_start_of_flags)\n"
"        {\n"
"            iterator->current_slot_index += JSL_PLATFORM_COUNT_TRAILING_ZEROS(is_set_flags);\n"
"\n"
"            *key = iterator->hash_map->keys_array[iterator->current_slot_index];\n"
"            *value = iterator->hash_map->items_array[iterator->current_slot_index];\n"
"\n"
"            ++iterator->current_slot_index;\n"
"            result = true;\n"
"            break;\n"
"        }\n"
"        else\n"
"        {\n"
"            uint32_t current_is_set_flags_bit = (uint32_t) (iterator->current_slot_index - (is_set_flags_index * 32));\n"
"            uint32_t bitflag = JSL_MAKE_BITFLAG(current_is_set_flags_bit);\n"
"            bool is_set = JSL_IS_BITFLAG_SET(is_set_flags, bitflag);\n"
"\n"
"            if (is_set)\n"
"            {\n"
"                *key = iterator->hash_map->keys_array[iterator->current_slot_index];\n"
"                *value = iterator->hash_map->items_array[iterator->current_slot_index];\n"
"\n"
"                ++iterator->current_slot_index;\n"
"                result = true;\n"
"                break;\n"
"            }\n"
"            else\n"
"            {\n"
"                ++iterator->current_slot_index;\n"
"            }\n"
"        }\n"
"    }\n"
"\n"
"    return result;\n"
"}\n\n");

JSLFatPtr dynamic_expand_function_code = JSL_FATPTR_INITIALIZER(""
"static bool function_prefix##_expand(JSL_HASHMAP_TYPE_NAME(name)* hash_map)\n"
"{\n"
"    JSL_DEBUG_ASSERT(hash_map != NULL);\n"
"    JSL_DEBUG_ASSERT(hash_map->arena != NULL);\n"
"    JSL_DEBUG_ASSERT(hash_map->slots_array != NULL);\n"
"    JSL_DEBUG_ASSERT(hash_map->is_set_flags_array != NULL);\n"
"\n"
"    bool success;\n"
"\n"
"    JSL_HASHMAP_ITEM_TYPE_NAME(name)* old_slots_array = hash_map->slots_array;\n"
"    int64_t old_slots_array_length = hash_map->slots_array_length;\n"
"\n"
"    uint32_t* old_is_set_flags_array = hash_map->is_set_flags_array;\n"
"    int64_t old_is_set_flags_array_length = hash_map->is_set_flags_array_length;\n"
"\n"
"    int64_t new_slots_array_length = jsl__hashmap_expand_size(old_slots_array_length);\n"
"    JSL_HASHMAP_ITEM_TYPE_NAME(name)* new_slots_array = (JSL_HASHMAP_ITEM_TYPE_NAME(name)*) jsl_arena_allocate(\n"
"        hash_map->arena, sizeof(JSL_HASHMAP_ITEM_TYPE_NAME(name)) * new_slots_array_length, false\n"
"    ).data;\n"
"\n"
"    int64_t new_is_set_flags_array_length = new_slots_array_length >> 5L;\n"
"    uint32_t* new_is_set_flags_array = (uint32_t*) jsl_arena_allocate(\n"
"        hash_map->arena, sizeof(uint32_t) * new_is_set_flags_array_length, true\n"
"    ).data;\n"
"\n"
"    if (new_slots_array != NULL && new_is_set_flags_array != NULL)\n"
"    {\n"
"        hash_map->item_count = 0;\n"
"        hash_map->slots_array = new_slots_array;\n"
"        hash_map->slots_array_length = new_slots_array_length;\n"
"        hash_map->is_set_flags_array = new_is_set_flags_array;\n"
"        hash_map->is_set_flags_array_length = new_is_set_flags_array_length;\n"
"\n"
"        int64_t slot_index = 0;\n"
"        for (\n"
"            int64_t is_set_flags_index = 0;\n"
"            is_set_flags_index < old_is_set_flags_array_length;\n"
"            is_set_flags_index++\n"
"        )\n"
"        {\n"
"            for (uint32_t current_bit = 0; current_bit < 32; current_bit++)\n"
"            {\n"
"                uint32_t bitflag = JSL_MAKE_BITFLAG(current_bit);\n"
"                if (JSL_IS_BITFLAG_SET(old_is_set_flags_array[is_set_flags_index], bitflag))\n"
"                {\n"
"                    function_prefix##_insert(hash_map, old_slots_array[slot_index].key, old_slots_array[slot_index].value);\n"
"                }\n"
"                ++slot_index;\n"
"            }\n"
"        }\n"
"\n"
"        success = true;\n"
"    }\n"
"    else\n"
"    {\n"
"        success = false;\n"
"    }\n"
"\n"
"    return success;\n"
"}\n\n");

/**
 * Generates the header file data for your hash map. This file includes all the typedefs
 * and function signatures for this hash map.
 *
 * The generated header file includes "jacks_hash_map.h", and it's assumed to be in the
 * same directory as where this header file will live.
 *
 * If your type needs a custom hash function, it must have the function signature
 * `uint64_t my_hash_function(void* data, int64_t length, uint64_t seed);`.
 *
 * @param arena Arena allocator used for memory allocation. The arena must have
 *              sufficient space (at least 512KB recommended) to hold the generated
 *              header content.
 * @param hash_map_name The name of the hash map type (e.g., "StringIntHashMap").
 *                      This will be used as the main type name in the generated code.
 * @param function_prefix The prefix for all generated function names (e.g., "string_int_map").
 *                        Functions will be named like: {function_prefix}_init, {function_prefix}_insert, etc.
 * @param key_type_name The C type name for hash map keys (e.g., "int", "MyStruct").
 * @param value_type_name The C type name for hash map values (e.g., "int", "MyData*", "float").
 * @param hash_function_name Name of your custom hash function if you have one, NULL otherwise.
 * @param include_header_count Number of additional header files to include in the generated header.
 * @param include_header_count Number of additional header files to include in the generated header.

 * @return JSLFatPtr containing the generated header file content
 *
 * @warning Ensure the arena has sufficient space (minimum 512KB recommended) to avoid
 *          allocation failures during header generation.
 */
void write_hash_map_header(
    HashMapImplementation impl,
    JSLStringBuilder* builder,
    JSLFatPtr hash_map_name,
    JSLFatPtr function_prefix,
    JSLFatPtr key_type_name,
    JSLFatPtr value_type_name,
    JSLFatPtr hash_function_name,
    JSLFatPtr* include_header_array,
    int32_t include_header_count
)
{
    (void) impl;
    (void) hash_function_name;

    jsl_string_builder_format(
        builder,
        static_hash_map_docstring,
        JSL_FATPTR_EXPRESSION("header"),
        hash_map_name,
        key_type_name,
        value_type_name
    );

    jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#pragma once\n\n"));
    jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#include <stdint.h>\n"));
    jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#include \"jsl_hash_map_common.h\"\n\n"));

    for (int32_t i = 0; i < include_header_count; ++i)
    {
        jsl_string_builder_format(builder, JSL_FATPTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
    }

    jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("\n"));

    jsl_string_builder_format(
        builder,
        static_map_type_typedef,
        key_type_name,
        value_type_name,
        hash_map_name,
        key_type_name,
        value_type_name,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        static_map_iterator_typedef,
        hash_map_name,
        hash_map_name,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        static_find_res_struct,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        static_init_function_signature,
        function_prefix,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        static_insert_function_signature,
        function_prefix,
        hash_map_name,
        key_type_name,
        value_type_name
    );

    jsl_string_builder_format(
        builder,
        static_get_function_signature,
        value_type_name,
        function_prefix,
        hash_map_name,
        key_type_name
    );

    jsl_string_builder_format(
        builder,
        static_delete_function_signature,
        function_prefix,
        hash_map_name,
        key_type_name
    );

    jsl_string_builder_format(
        builder,
        static_iterator_start_function_signature,
        key_type_name,
        value_type_name,
        hash_map_name,
        function_prefix,
        function_prefix,
        function_prefix,
        hash_map_name,
        hash_map_name,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        static_iterator_next_function_signature,
        key_type_name,
        value_type_name,
        hash_map_name,
        function_prefix,
        function_prefix,
        function_prefix,
        hash_map_name,
        key_type_name,
        value_type_name
    );
}

void write_hash_map_source(
    HashMapImplementation impl,
    JSLStringBuilder* builder,
    JSLFatPtr hash_map_name,
    JSLFatPtr function_prefix,
    JSLFatPtr key_type_name,
    JSLFatPtr value_type_name,
    JSLFatPtr hash_function_name,
    JSLFatPtr* include_header_array,
    int32_t include_header_count
)
{
    (void) impl;

    jsl_string_builder_format(
        builder,
        static_hash_map_docstring,
        JSL_FATPTR_EXPRESSION("source"),
        hash_map_name,
        key_type_name,
        value_type_name
    );

    jsl_string_builder_insert_fatptr(
        builder,
        JSL_FATPTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
    );

    jsl_string_builder_insert_fatptr(
        builder,
        JSL_FATPTR_EXPRESSION("#include <stddef.h>\n")
    );
    jsl_string_builder_insert_fatptr(
        builder,
        JSL_FATPTR_EXPRESSION("#include <stdint.h>\n")
    );
    jsl_string_builder_insert_fatptr(
        builder,
        JSL_FATPTR_EXPRESSION("#include \"jsl_core.h\"\n")
    );
    jsl_string_builder_insert_fatptr(
        builder,
        JSL_FATPTR_EXPRESSION("#include \"jsl_hash_map_common.h\"\n\n")
    );

    jsl_string_builder_insert_fatptr(
        builder,
        JSL_FATPTR_EXPRESSION("// USER INCLUDED HEADERS\n")
    );

    for (int32_t i = 0; i < include_header_count; ++i)
    {
        jsl_string_builder_format(builder, JSL_FATPTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
    }

    jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("\n"));

    jsl_string_builder_format(
        builder,
        static_init_function_code,
        function_prefix,
        hash_map_name,
        hash_map_name,
        key_type_name,
        key_type_name,
        key_type_name,
        value_type_name,
        value_type_name,
        value_type_name
    );

    // hash and find slot
    {
        static JSLFatPtr int32_t_str = JSL_FATPTR_INITIALIZER("int32_t");
        static JSLFatPtr int_str = JSL_FATPTR_INITIALIZER("int");
        static JSLFatPtr unsigned_str = JSL_FATPTR_INITIALIZER("unsigned");
        static JSLFatPtr unsigned_int_str = JSL_FATPTR_INITIALIZER("unsigned int");
        static JSLFatPtr uint32_t_str = JSL_FATPTR_INITIALIZER("uint32_t");
        static JSLFatPtr int64_t_str = JSL_FATPTR_INITIALIZER("int64_t");
        static JSLFatPtr long_str = JSL_FATPTR_INITIALIZER("long");
        static JSLFatPtr uint64_t_str = JSL_FATPTR_INITIALIZER("uint64_t");
        static JSLFatPtr unsigned_long_str = JSL_FATPTR_INITIALIZER("unsigned long");

        uint8_t hash_function_call_buffer[4098];
        JSLArena hash_function_scratch_arena = JSL_ARENA_FROM_STACK(hash_function_call_buffer);

        JSLFatPtr resolved_hash_function_call;
        if (hash_function_name.data != NULL && hash_function_name.length > 0)
        {
            resolved_hash_function_call = jsl_format(
                &hash_function_scratch_arena,
                JSL_FATPTR_EXPRESSION("uint64_t hash = %y(&key, sizeof(%y), hash_map->seed)"),
                hash_function_name,
                key_type_name
            );
        }
        else if (
            jsl_fatptr_memory_compare(key_type_name, int32_t_str)
            || jsl_fatptr_memory_compare(key_type_name, int_str)
            || jsl_fatptr_memory_compare(key_type_name, unsigned_str)
            || jsl_fatptr_memory_compare(key_type_name, unsigned_int_str)
            || jsl_fatptr_memory_compare(key_type_name, uint32_t_str)
            || jsl_fatptr_memory_compare(key_type_name, int64_t_str)
            || jsl_fatptr_memory_compare(key_type_name, long_str)
            || jsl_fatptr_memory_compare(key_type_name, uint64_t_str)
            || jsl_fatptr_memory_compare(key_type_name, unsigned_long_str)
            || key_type_name.data[key_type_name.length - 1] == '*'
        )
        {
            resolved_hash_function_call = jsl_format(
                &hash_function_scratch_arena,
                JSL_FATPTR_EXPRESSION("uint64_t hash = murmur3_fmix_u64((uint64_t) key, hash_map->seed)"),
                key_type_name
            );
        }
        else
        {
            resolved_hash_function_call = jsl_format(
                &hash_function_scratch_arena,
                JSL_FATPTR_EXPRESSION("uint64_t hash = jsl__rapidhash_withSeed(&key, sizeof(%y), hash_map->seed)"),
                key_type_name
            );
        }

        jsl_string_builder_format(
            builder,
            static_hash_function_code,
            hash_map_name,
            function_prefix,
            hash_map_name,
            key_type_name,
            hash_map_name,
            resolved_hash_function_call,
            key_type_name
        );
    }

    jsl_string_builder_format(
        builder,
        static_insert_function_code,
        function_prefix,
        hash_map_name,
        key_type_name,
        value_type_name,
        hash_map_name,
        function_prefix
    );

    jsl_string_builder_format(
        builder,
        static_get_function_code,
        value_type_name,
        function_prefix,
        hash_map_name,
        key_type_name,
        value_type_name,
        hash_map_name,
        function_prefix
    );

    jsl_string_builder_format(
        builder,
        static_delete_function_code,
        function_prefix,
        hash_map_name,
        key_type_name,
        hash_map_name,
        function_prefix
    );

    jsl_string_builder_format(
        builder,
        static_iterator_start_function_code,
        function_prefix,
        hash_map_name,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        static_iterator_next_function_code,
        function_prefix,
        hash_map_name,
        key_type_name,
        value_type_name
    );

}

#ifdef INCLUDE_MAIN

#if JSL_IS_WINDOWS
    #include <windows.h>
    #include <shellapi.h>
    #pragma comment(lib, "shell32.lib")
#endif

JSLFatPtr help_message = JSL_FATPTR_INITIALIZER(
    "OVERVIEW:\n\n"
    "Hash map C code generation utility\n\n"
    "This program generates both a C source and header file for a hash map with the given\n"
    "key and value types. More documentation is included in the source file.\n\n"
    "USAGE:\n\n"
    "\tgenerate_hash_map --name TYPE_NAME --function_prefix PREFIX --key_type TYPE --value_type TYPE [--static | --dynamic] [--header | --source] [--add-header=FILE]...\n\n"
    "Required arguments:\n"
    "\t--name\t\t\tThe name to give the hash map container type\n"
    "\t--function_prefix\tThe prefix added to each of the functions for the hash map\n"
    "\t--key_type\t\tThe C type name for the key\n"
    "\t--value_type\t\tThe C type name for the value\n\n"
    "Optional arguments:\n"
    "\t--header\t\tWrite the header file to stdout\n"
    "\t--source\t\tWrite the source file to stdout\n"
    "\t--dynamic\t\tGenerate a hash map which grows dynamically\n"
    "\t--static\t\tGenerate a statically sized hash map\n"
    "\t--add-header\t\tPath to a C header which will be added with a #include directive at the top of the generated file\n"
    "\t--custom-hash\t\tOverride the included hash call with the given function name\n"
);

static int32_t entrypoint(JSLArena* arena, JSLFatPtr* args, int32_t arg_count)
{
    bool show_help = false;
    bool print_header = false;
    JSLFatPtr name = {0};
    JSLFatPtr function_prefix = {0};
    JSLFatPtr key_type = {0};
    JSLFatPtr value_type = {0};
    JSLFatPtr hash_function_name = {0};
    HashMapImplementation impl = IMPL_ERROR;
    JSLFatPtr* header_includes = NULL;
    int32_t header_includes_count = 0;

    JSLFatPtr h_flag_str = JSL_FATPTR_INITIALIZER("-h");
    JSLFatPtr help_flag_str = JSL_FATPTR_INITIALIZER("--help");
    JSLFatPtr name_flag_str = JSL_FATPTR_INITIALIZER("--name");
    JSLFatPtr name_flag_eq_str = JSL_FATPTR_INITIALIZER("--name=");
    JSLFatPtr function_prefix_flag_str = JSL_FATPTR_INITIALIZER("--function_prefix");
    JSLFatPtr function_prefix_flag_eq_str = JSL_FATPTR_INITIALIZER("--function_prefix=");

    // Parse command line arguments
    for (int i = 1; i < arg_count; i++)
    {
        JSLFatPtr arg = args[i];

        if (
            jsl_fatptr_memory_compare(arg, h_flag_str)
            || jsl_fatptr_memory_compare(arg, help_flag_str)
        )
        {
            show_help = true;
        }
        else if (jsl_fatptr_memory_compare(arg, name_flag_eq_str))
        {
            name = jsl_fatptr_slice_to_end(arg, 7);
        }
        else if (jsl_fatptr_memory_compare(arg, name_flag_str))
        {
            if (i + 1 < arg_count)
            {
                name = args[++i];
            }
            else
            {
                fprintf(stderr, "Error: --name requires a value\n");
                return EXIT_FAILURE;
            }
        }
        else if (jsl_fatptr_memory_compare(arg, function_prefix_flag_eq_str))
        {
            function_prefix = jsl_fatptr_slice_to_end(arg, 18);
        }
        else if (jsl_fatptr_memory_compare(arg, function_prefix_flag_str))
        {
            if (i + 1 < arg_count)
            {
                function_prefix = args[++i];
            }
            else
            {
                fprintf(stderr, "Error: --function_prefix requires a value\n");
                return EXIT_FAILURE;
            }
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--key_type=")))
        {
            key_type = arg;
            JSL_FATPTR_ADVANCE(key_type, 11);
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--key_type")))
        {
            if (i + 1 < arg_count)
            {
                key_type = args[++i];
            }
            else
            {
                fprintf(stderr, "Error: --key_type requires a value\n");
                return EXIT_FAILURE;
            }
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--value_type=")))
        {
            value_type = jsl_fatptr_slice_to_end(arg, 13);
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--value_type")))
        {
            if (i + 1 < arg_count)
            {
                value_type = args[++i];
            }
            else
            {
                fprintf(stderr, "Error: --value_type requires a value\n");
                return EXIT_FAILURE;
            }
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--static")))
        {
            impl = IMPL_STATIC;
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--dynamic")))
        {
            impl = IMPL_DYNAMIC;
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--header")))
        {
            print_header = true;
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--source")))
        {
            print_header = false;
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--add-header=")))
        {
            JSLFatPtr header = jsl_fatptr_slice_to_end(arg, 13);

            ++header_includes_count;
            header_includes = realloc(
                header_includes,
                sizeof(JSLFatPtr) * (size_t) header_includes_count
            );
            header_includes[header_includes_count - 1] = header;
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--add-header")))
        {
            ++header_includes_count;
            header_includes = realloc(
                header_includes,
                sizeof(JSLFatPtr) * (size_t) header_includes_count
            );
            header_includes[header_includes_count - 1] = args[++i];
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--custom-hash=")))
        {
            hash_function_name = jsl_fatptr_slice_to_end(arg, 14);
        }
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--custom-hash")))
        {
            if (i + 1 < arg_count)
            {
                hash_function_name = args[++i];
            }
            else
            {
                fprintf(stderr, "Error: --custom-hash requires a value\n");
                return EXIT_FAILURE;
            }
        }
        else
        {
            jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Error: Unknown argument: %y\n"), arg);
            return EXIT_FAILURE;
        }
    }

    if (show_help)
    {
        jsl_format_file(stdout, help_message);
        return EXIT_SUCCESS;
    }
    // Check that all required parameters are provided
    else if (name.data == NULL)
    {
        jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Error: --name is required\n"));
        return EXIT_FAILURE;
    }

    if (function_prefix.data == NULL)
    {
        jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Error: --function_prefix is required\n"));
        return EXIT_FAILURE;
    }

    if (key_type.data == NULL)
    {
        jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Error: --key_type is required\n"));
        return EXIT_FAILURE;
    }

    if (value_type.data == NULL)
    {
        jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Error: --value_type is required\n"));
        return EXIT_FAILURE;
    }

    if (impl == IMPL_ERROR)
    {
        impl = IMPL_DYNAMIC;
    }

    JSLStringBuilder builder;
    jsl_string_builder_init2(&builder, arena, 1024, 8);

    if (print_header)
    {
        write_hash_map_header(
            impl,
            &builder,
            name,
            function_prefix,
            key_type,
            value_type,
            hash_function_name,
            header_includes,
            header_includes_count
        );
    }
    else
    {
        write_hash_map_source(
            impl,
            &builder,
            name,
            function_prefix,
            key_type,
            value_type,
            hash_function_name,
            header_includes,
            header_includes_count
        );
    }

    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);

    while (true)
    {
        JSLFatPtr slice = jsl_string_builder_iterator_next(&iterator);
        if (slice.data == NULL)
            break;

        jsl_format_file(stdout, slice);
    }


    return EXIT_SUCCESS;
}


#if JSL_IS_WINDOWS

// annoyingly, clang does not special case wmain like main
// for missing prototypes.
int32_t wmain(int32_t argc, wchar_t** argv);

int32_t wmain(int32_t argc, wchar_t** argv)
{
    
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);

#else

int32_t main(int32_t argc, char** argv)
{

#endif

    int64_t arena_size = JSL_MEGABYTES(32);
    JSLArena arena;
    void* backing_data = malloc((size_t) arena_size);
    if (backing_data == NULL)
        return EXIT_FAILURE;

    jsl_arena_init(&arena, backing_data, arena_size);

    JSLFatPtr* arg_array = JSL_ARENA_TYPED_ARRAY_ALLOCATE(JSLFatPtr, &arena, argc);

    for (int32_t i = 0; i < argc; ++i)
    {
        // Convert to UTF-8 on windows
        // #if JSL_IS_WINDOWS
        //     size_t arg_length = wcslen(argv[i]);
        //     size_t buffer_length = simdutf_utf8_length_from_utf16(argv[i], arg_length);
        //     JSLFatPtr result_buffer = jsl_arena_allocate(&arena, (int64_t) buffer_length, false);

        //     size_t result_length = simdutf_convert_utf16_to_utf8(
        //         argv[i],
        //         arg_length,
        //         (char*) result_buffer.data
        //     ); 

        //     JSLFatPtr arg = jsl_fatptr_slice(result_buffer, 0, (int64_t) result_length);
        // #else
            JSLFatPtr arg = jsl_fatptr_from_cstr(argv[i]);
        // #endif
        
        arg_array[i] = arg;
    }

    return entrypoint(&arena, arg_array, argc);
}


#endif // INCLUDE_MAIN

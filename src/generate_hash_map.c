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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <stdalign.h>

#define JSL_INCLUDE_FILE_UTILS
#define JSL_IMPLEMENTATION
#include "jacks_standard_library.h"

#include "generate_hash_map.h"

/**
 * TODO: Documentation: talk about
 *  - must use arena with lifetime greater than the hashmap
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


// #define JSL_GET_SET_FLAG_INDEX(slot_number) slot_number >> 5L // divide by 32
// #define JSL_HASHMAP_TYPE_NAME(name) name
// #define JSL_HASHMAP_ITEM_TYPE_NAME(name) name##Item
// #define JSL_HASHMAP_ITERATOR_TYPE_NAME(name) name##Iterator
// #define JSL_HASHMAP_FIND_RES_TYPE_NAME(name) name##FindRes
// #define JSL_HASHMAP_ITERATOR_RET_TYPE_NAME(name) name##IteratorReturn

// #define JSL_HASHMAP_TYPES(name, key_type, value_type)\
//         typedef struct JSL_HASHMAP_ITEM_TYPE_NAME(name) {\
//             key_type key;\
//             value_type value;\
//         } JSL_HASHMAP_ITEM_TYPE_NAME(name);\
//\
//         typedef struct JSL_HASHMAP_TYPE_NAME(name) {\
//             JSLArena* arena;\
//             JSL_HASHMAP_ITEM_TYPE_NAME(name)* slots_array;\
//             int64_t slots_array_length;\
//             uint32_t* is_set_flags_array;\
//             int64_t is_set_flags_array_length;\
//             int64_t item_count;\
//             uint16_t generational_id;\
//             uint8_t flags;\
//         } JSL_HASHMAP_TYPE_NAME(name);\
//\
//         typedef struct JSL_HASHMAP_FIND_RES_TYPE_NAME(name) {\
//             JSL_HASHMAP_ITEM_TYPE_NAME(name)* slot;\
//             int64_t is_set_array_index;\
//             uint32_t is_set_array_bit;\
//             bool is_update;\
//         } JSL_HASHMAP_FIND_RES_TYPE_NAME(name);\
//\
//         typedef struct JSL_HASHMAP_ITERATOR_TYPE_NAME(name) {\
//             JSL_HASHMAP_TYPE_NAME(name)* hashmap;\
//             int64_t current_slot_index;\
//             uint16_t generational_id;\
//         } JSL_HASHMAP_ITERATOR_TYPE_NAME(name);

// #ifdef JSL_DEBUG
//     #define JSL_HASHMAP_CHECK_EMPTY(return_value)\
//         JSL_ASSERT(hashmap != NULL);\
//         JSL_ASSERT(hashmap->arena != NULL);\
//         JSL_ASSERT(hashmap->slots_array != NULL);\
//         JSL_ASSERT(hashmap->is_set_flags_array != NULL);
// #else
//     #define JSL_HASHMAP_CHECK_EMPTY(return_value)\
//         if (\
//             hashmap == NULL\
//             || hashmap->arena == NULL\
//             || hashmap->slots_array == NULL\
//             || hashmap->is_set_flags_array == NULL\
//         )\
//             return return_value;
// #endif


// #define JSL_HASHMAP_PROTOTYPES(name, function_prefix, key_type, value_type)\
//         void function_prefix##_init(JSL_HASHMAP_TYPE_NAME(name)* hashmap, JSLArena* arena, uint64_t seed);\
//         void function_prefix##_init2(JSL_HASHMAP_TYPE_NAME(name)* hashmap, JSLArena* arena, uint64_t seed, int64_t item_count_guess);\
//         bool function_prefix##_insert(JSL_HASHMAP_TYPE_NAME(name)* hashmap, key_type key, value_type value);\
//         value_type* function_prefix##_get(JSL_HASHMAP_TYPE_NAME(name)* hashmap, key_type key);\
//         bool function_prefix##_delete(JSL_HASHMAP_TYPE_NAME(name)* hashmap, key_type key);\
//         JSL_HASHMAP_ITERATOR_TYPE_NAME(name) function_prefix##_iterator_start(JSL_HASHMAP_TYPE_NAME(name)* hashmap);\
//         JSL_HASHMAP_ITEM_TYPE_NAME(name)* function_prefix##_iterator_next(JSL_HASHMAP_ITERATOR_TYPE_NAME(name)* iterator);

JSLFatPtr static_hash_map_header_docstring = JSL_FATPTR_LITERAL("/**\n"
" * AUTO GENERATED FILE\n"
" *\n"
" * This file contains the header for a hash map %y which maps `%y` keys to `%y` values.\n"
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

JSLFatPtr static_map_type_typedef = JSL_FATPTR_LITERAL("/**\n"
    " * A hash map which maps `%y` keys to `%y` values.\n"
    " *\n"
    " * This hash map uses open addressing with linear probing. However, it never grows.\n"
    " * When initalized with the init function, all the memory this hash map will have\n"
    " * is allocated right away.\n"
    " */\n"
    "typedef struct %y {\n"
    "    %y* keys_array;\n"
    "    %y* items_array;\n"
    "    int64_t slots_array_length;\n"
    "    uint32_t* is_set_flags_array;\n"
    "    int64_t is_set_flags_array_length;\n"
    "    int64_t item_count;\n"
    "    uint16_t generational_id;\n"
    "    uint8_t flags;\n"
    "} %y;\n"
    "\n");

/// @brief param 1 is the hash map type name, param 2 is the function prefix, param 3 is the hash map type name
JSLFatPtr static_init_function_signature = JSL_FATPTR_LITERAL("/**\n"
    " * Initialize an instance of the hash map.\n"
    " *\n"
    " * All of the memory that this hash map will need will be allocated from the passed in arena.\n"
    " * The hash map does not save a reference to the arena, but the arena memory must have the same\n"
    " * or greater lifetime than the hash map itself.\n"
    " *\n"
    " * @note This hash map uses a well distributed hash \"rapidhash\". But in order to properly protect\n"
    " * against hash flooding attacks you must provide good random data for the seed value. This means\n"
    " * using your OS's secure random number generator, not `rand`\n"
    " *\n"
    " * @param hash_map The pointer to the hash map instance to initialize\n"
    " * @param arena The arena that this hash map will use to allocate memory\n"
    " * @param seed Seed value for the hash function to protect against hash flooding attacks\n"
    " * @param max_item_count The maximum amount of items this hash map can hold\n"
    " */\n"
    "void %y_init(%y* hash_map, JSLArena* arena, int64_t max_item_count, uint64_t seed);\n\n");

/// @brief param 1 is the hash map type name, param 2 is the key type, param 3 is the value type
JSLFatPtr static_insert_function_signature = JSL_FATPTR_LITERAL("/**\n"
    " * Insert the given value into the hash map. This function will allocate if there's not\n"
    " * enough space. If the key already exists in the map the value will be overwritten. If\n"
    " * the key type for this hash map is a pointer, then a NULL key is accepted.\n"
    " *\n"
    " * @param hash_map The pointer to the hash map instance to initialize\n"
    " * @param key Hash map key\n"
    " * @param value Value to store\n"
    " * @returns A bool representing success or failure of insertion. Insertion can fail if memory cannot be allocated.\n"
    " */\n"
    "bool %y_insert(%y* hash_map, %y key, %y value);\n\n");

JSLFatPtr static_get_function_signature = JSL_FATPTR_LITERAL("/**\n"
    " * Get a value from the hash map if it exists. If it does not NULL is returned\n"
    " *\n"
    " * @warning The pointer returned actually points to value stored inside of hash map.\n"
    " * If you change the value though the pointer you change the hash, therefore screwing\n"
    " * up the map. Don't do this.\n"
    " *\n"
    " * @param hash_map The pointer to the hash map instance to initialize\n"
    " * @param key Hash map key\n"
    " * @param value Value to store\n"
    " * @returns The pointer to the value in the hash map, or null.\n"
    " */\n"
    "%y* %y_get(%y* hash_map, %y key);\n\n");

JSLFatPtr static_delete_function_signature = JSL_FATPTR_LITERAL("/**\n"
    " * Remove a key/value pair from the hash map if it exists. If it does not false is returned\n"
    " */\n"
    "bool %y_delete(%y* hashmap, %y key);\n\n");

JSLFatPtr static_iterator_start_function_signature = JSL_FATPTR_LITERAL("/**\n"
    " * Create a new iterator over this hash map.\n"
    " *\n"
    " * An iterator is a struct which holds enough state that it allows a loop to visit\n"
    " * each key/value pair in the hash map.\n"
    " *\n"
    " * Iterating over a hash map while modifying it is allowed. However, it's likely you\n"
    " * will iterate over items you've added during the iteration.\n"
    " *\n"
    " * Example usage:\n"
    " * @code\n"
    " * %y key;\n"
    " * %y value;\n"
    " * %yIterator iterator;\n"
    " * %y_iterator_start(hash_map);\n"
    " * while (%y_iterator_next(&iterator, &key, &value))\n"
    " * {\n"
    " *     ...\n"
    " * }\n"
    " * @endcode\n"
    " */\n"
    "void %y_iterator_start(%y* hashmap, %yIterator* iterator);\n\n");

JSLFatPtr static_iterator_next_function_signature = JSL_FATPTR_LITERAL("/**\n"
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
    "bool %y_iterator_next(%yIterator* iterator, %y key, %y value);\n\n");

JSLFatPtr static_init_function_code = JSL_FATPTR_LITERAL(""
"void %y_init(%y* hash_map, JSLArena* arena, int64_t max_item_count, uint64_t seed)\n"
"{\n"
"    JSL_DEBUG_ASSERT(hashmap != NULL);\n"
"    JSL_DEBUG_ASSERT(arena != NULL);\n"
"\n"
"    hashmap->arena = arena;\n"
"    hashmap->item_count = 0;\n"
"    hashmap->flags = 0;\n"
"    hashmap->generational_id = 0;\n"
"\n"
"    if (item_count_guess <= 16)\n"
"        hashmap->slots_array_length = 32;\n"
"    else if (jss__is_power_of_two(item_count_guess))\n"
"        hashmap->slots_array_length = item_count_guess * 2;\n"
"    else\n"
"        hashmap->slots_array_length = jss__next_power_of_two(item_count_guess) * 2;\n"
"\n"
"    hashmap->is_set_flags_array_length = hashmap->slots_array_length >> 5L;\n"
"\n"
"    hashmap->slots_array = (%y*) jss_arena_allocate(\n"
"        arena, sizeof(%y) * hashmap->slots_array_length, false\n"
"    ).data;\n"
"\n"
"    hashmap->is_set_flags_array = (uint32_t*) jss_arena_allocate(\n"
"        arena, sizeof(uint32_t) * hashmap->is_set_flags_array_length, true\n"
"    ).data;\n"
"}\n\n");

JSLFatPtr static_hash_function_code = JSL_FATPTR_LITERAL(""
"static inline %y %y_hash_and_find_slot(\n"
"    %y* hashmap,\n"
"    key_type key,\n"
"    bool is_insert\n"
")\n"
"{\n"
"    JSL_HASHMAP_FIND_RES_TYPE_NAME(name) return_value;\n"
"    return_value.slot = NULL;\n"
"\n"
"    uint64_t hash = jss__wyhash(&key, sizeof(%y), jss__hash_seed, jss__wyhash_secret);\n"
"\n"
"    int64_t total_checked = 0;\n"
"    /* Since our slot array length is always a pow 2, we can avoid a modulo  */\n"
"    int64_t slot_index = (int64_t) (hash & (hashmap->slots_array_length - 1));\n"
"    return_value.is_set_array_index = (int64_t) JSL_GET_SET_FLAG_INDEX(slot_index);\n"
"    /* Manual remainder here too  */\n"
"    return_value.is_set_array_bit = slot_index - (return_value.is_set_array_index * 32);\n"
"\n"
"    for (;;)\n"
"    {\n"
"        uint32_t bit_flag = JSL_MAKE_BITFLAG(return_value.is_set_array_bit);\n"
"        uint32_t is_slot_set = JSL_IS_BITFLAG_SET(\n"
"            hashmap->is_set_flags_array[return_value.is_set_array_index],\n"
"            bit_flag\n"
"        );\n"
"\n"
"        if (is_slot_set == 0 && is_insert)\n"
"        {\n"
"            return_value.slot = &hashmap->slots_array[slot_index];\n"
"            return_value.is_update = false;\n"
"            break;\n"
"        }\n"
"        /* Updating value */\n"
"        else if (is_slot_set == 1)\n"
"        {\n"
"            int32_t memcmp_res = memcmp(\n"
"                &hashmap->slots_array[slot_index].key,\n"
"                &key,\n"
"                sizeof(%y)\n"
"            );\n"
"            if (memcmp_res == 0)\n"
"            {\n"
"                return_value.slot = &hashmap->slots_array[slot_index];\n"
"                return_value.is_update = true;\n"
"                break;\n"
"            }\n"
"        }\n"
"\n"
"        /* Collision. Move to the next spot with linear probing  */\n"
"\n"
"        ++total_checked;\n"
"        ++return_value.is_set_array_bit;\n"
"        ++slot_index;\n"
"\n"
"        /* We can't expand and the hashmap is completely full  */\n"
"        if (total_checked == hashmap->slots_array_length)\n"
"        {\n"
"            break;\n"
"        }\n"
"\n"
"        if (return_value.is_set_array_bit == 32)\n"
"        {\n"
"            ++return_value.is_set_array_index;\n"
"            return_value.is_set_array_bit = 0;\n"
"        }\n"
"\n"
"        /* Loop all the way back around */\n"
"        if (slot_index == hashmap->slots_array_length)\n"
"        {\n"
"            slot_index = 0;\n"
"            return_value.is_set_array_bit = 0;\n"
"            return_value.is_set_array_index = 0;\n"
"        }\n"
"    }\n"
"\n"
"    return return_value;\n"
"}\n\n");

JSLFatPtr static_insert_function_code = JSL_FATPTR_LITERAL(""
"bool function_prefix##_insert(JSL_HASHMAP_TYPE_NAME(name)* hashmap, key_type key, value_type value)\n"
"{\n"
"    JSL_HASHMAP_CHECK_EMPTY(false)\n"
"    bool insert_success = false;\n"
"\n"
"    if (JSL_IS_BITFLAG_NOT_SET(hashmap->flags, JSL__HASHMAP_CANT_EXPAND)\n"
"        && jss__hashmap_should_expand(hashmap->slots_array_length, hashmap->item_count + 1))\n"
"    {\n"
"        bool expand_res = function_prefix##_expand(hashmap);\n"
"        if (!expand_res)\n"
"        {\n"
"            JSL_SET_BITFLAG(&hashmap->flags, JSL__HASHMAP_CANT_EXPAND);\n"
"        }\n"
"    }\n"
"\n"
"    JSL_HASHMAP_FIND_RES_TYPE_NAME(name) find_res = function_prefix##_hash_and_find_slot(\n"
"        hashmap,\n"
"        key,\n"
"        true\n"
"    );\n"
"    if (find_res.slot != NULL)\n"
"    {\n"
"        if (find_res.is_update)\n"
"        {\n"
"            find_res.slot->value = value;\n"
"            insert_success = true;\n"
"        }\n"
"        else\n"
"        {\n"
"            find_res.slot->key = key;\n"
"            find_res.slot->value = value;\n"
"            uint32_t bit_flag = JSL_MAKE_BITFLAG(find_res.is_set_array_bit);\n"
"            JSL_SET_BITFLAG(\n"
"                &hashmap->is_set_flags_array[find_res.is_set_array_index],\n"
"                bit_flag\n"
"            );\n"
"            ++hashmap->item_count;\n"
"            insert_success = true;\n"
"        }\n"
"\n"
"        ++hashmap->generational_id;\n"
"    }\n"
"\n"
"    return insert_success;\n"
"}\n\n");

JSLFatPtr static_get_function_code = JSL_FATPTR_LITERAL(""
"value_type* function_prefix##_get(JSL_HASHMAP_TYPE_NAME(name)* hashmap, key_type key)\n"
"{\n"
"    JSL_HASHMAP_CHECK_EMPTY(NULL)\n"
"    value_type* res = NULL;\n"
"\n"
"    JSL_HASHMAP_FIND_RES_TYPE_NAME(name) find_res = function_prefix##_hash_and_find_slot(hashmap, key, false);\n"
"    if (find_res.slot != NULL && find_res.is_update)\n"
"    {\n"
"        res = &find_res.slot->value;\n"
"    }\n"
"\n"
"    return res;\n"
"}\n\n");

JSLFatPtr static_delete_function_code = JSL_FATPTR_LITERAL(""
"bool function_prefix##_delete(JSL_HASHMAP_TYPE_NAME(name)* hashmap, key_type key)\n"
"{\n"
"    JSL_HASHMAP_CHECK_EMPTY(false)\n"
"    bool success = false;\n"
"    JSL_HASHMAP_FIND_RES_TYPE_NAME(name) find_res = function_prefix##_hash_and_find_slot(hashmap, key, false);\n"
"\n"
"    if (find_res.slot != NULL && find_res.is_update)\n"
"    {\n"
"        uint32_t bit_flag = JSL_MAKE_BITFLAG(find_res.is_set_array_bit);\n"
"        JSL_UNSET_BITFLAG(\n"
"            &hashmap->is_set_flags_array[find_res.is_set_array_index],\n"
"            bit_flag\n"
"        );\n"
"        --hashmap->item_count;\n"
"        success = true;\n"
"    }\n"
"\n"
"    return success;\n"
"}\n\n");

JSLFatPtr static_iterator_start_function_code = JSL_FATPTR_LITERAL(""
"JSL_HASHMAP_ITERATOR_TYPE_NAME(name) function_prefix##_iterator_start(JSL_HASHMAP_TYPE_NAME(name)* hashmap)\n"
"{\n"
"    JSL_DEBUG_ASSERT(hashmap != NULL);\n"
"    JSL_DEBUG_ASSERT(hashmap->arena != NULL);\n"
"    JSL_DEBUG_ASSERT(hashmap->slots_array != NULL);\n"
"    JSL_DEBUG_ASSERT(hashmap->is_set_flags_array != NULL);\n"
"\n"
"    JSL_HASHMAP_ITERATOR_TYPE_NAME(name) iterator = {\n"
"        .hashmap = hashmap,\n"
"        .current_slot_index = 0\n"
"    };\n"
"\n"
"    iterator.generational_id = hashmap->generational_id;\n"
"\n"
"    return iterator;\n"
"}\n\n");

JSLFatPtr static_iterator_next_function_code = JSL_FATPTR_LITERAL(""
"JSL_HASHMAP_ITEM_TYPE_NAME(name)* function_prefix##_iterator_next(JSL_HASHMAP_ITERATOR_TYPE_NAME(name)* iterator)\n"
"{\n"
"    JSL_DEBUG_ASSERT(iterator != NULL);\n"
"    JSL_DEBUG_ASSERT(iterator->hashmap != NULL);\n"
"    JSL_DEBUG_ASSERT(iterator->hashmap->slots_array != NULL);\n"
"    JSL_DEBUG_ASSERT(iterator->hashmap->is_set_flags_array != NULL);\n"
"    JSL_DEBUG_ASSERT(iterator->generational_id == iterator->hashmap->generational_id);\n"
"\n"
"    JSL_HASHMAP_ITEM_TYPE_NAME(name)* result = NULL;\n"
"\n"
"    for (; iterator->current_slot_index < iterator->hashmap->slots_array_length; iterator->current_slot_index++)\n"
"    {\n"
"        int64_t is_set_flags_index = JSL_GET_SET_FLAG_INDEX(iterator->current_slot_index);\n"
"        int32_t current_is_set_flags_bit = iterator->current_slot_index - (is_set_flags_index * 32);\n"
"        uint32_t bitflag = JSL_MAKE_BITFLAG(current_is_set_flags_bit);\n"
"\n"
"        if (JSL_IS_BITFLAG_SET(\n"
"            iterator->hashmap->is_set_flags_array[is_set_flags_index], bitflag\n"
"        ))\n"
"        {\n"
"            result = &iterator->hashmap->slots_array[iterator->current_slot_index];\n"
"            ++iterator->current_slot_index;\n"
"            break;\n"
"        }\n"
"    }\n"
"\n"
"    return result;\n"
"}\n\n");

JSLFatPtr dynamic_expand_function_code = JSL_FATPTR_LITERAL(""
"static bool function_prefix##_expand(JSL_HASHMAP_TYPE_NAME(name)* hashmap)\n"
"{\n"
"    JSL_DEBUG_ASSERT(hashmap != NULL);\n"
"    JSL_DEBUG_ASSERT(hashmap->arena != NULL);\n"
"    JSL_DEBUG_ASSERT(hashmap->slots_array != NULL);\n"
"    JSL_DEBUG_ASSERT(hashmap->is_set_flags_array != NULL);\n"
"\n"
"    bool success;\n"
"\n"
"    JSL_HASHMAP_ITEM_TYPE_NAME(name)* old_slots_array = hashmap->slots_array;\n"
"    int64_t old_slots_array_length = hashmap->slots_array_length;\n"
"\n"
"    uint32_t* old_is_set_flags_array = hashmap->is_set_flags_array;\n"
"    int64_t old_is_set_flags_array_length = hashmap->is_set_flags_array_length;\n"
"\n"
"    int64_t new_slots_array_length = jss__hashmap_expand_size(old_slots_array_length);\n"
"    JSL_HASHMAP_ITEM_TYPE_NAME(name)* new_slots_array = (JSL_HASHMAP_ITEM_TYPE_NAME(name)*) jss_arena_allocate(\n"
"        hashmap->arena, sizeof(JSL_HASHMAP_ITEM_TYPE_NAME(name)) * new_slots_array_length, false\n"
"    ).data;\n"
"\n"
"    int64_t new_is_set_flags_array_length = new_slots_array_length >> 5L;\n"
"    uint32_t* new_is_set_flags_array = (uint32_t*) jss_arena_allocate(\n"
"        hashmap->arena, sizeof(uint32_t) * new_is_set_flags_array_length, true\n"
"    ).data;\n"
"\n"
"    if (new_slots_array != NULL && new_is_set_flags_array != NULL)\n"
"    {\n"
"        hashmap->item_count = 0;\n"
"        hashmap->slots_array = new_slots_array;\n"
"        hashmap->slots_array_length = new_slots_array_length;\n"
"        hashmap->is_set_flags_array = new_is_set_flags_array;\n"
"        hashmap->is_set_flags_array_length = new_is_set_flags_array_length;\n"
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
"                    function_prefix##_insert(hashmap, old_slots_array[slot_index].key, old_slots_array[slot_index].value);\n"
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

static bool cstring_compare(const char* c1, const char* c2)
{
    bool res = false;
    size_t c1_len = strlen(c1);
    size_t c2_len = strlen(c2);

    if (c1_len == c2_len)
    {
        res = memcmp(c1, c2, c1_len) == 0;
    }

    return res;
}

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
 * @param key_type_name The C type name for hash map keys (e.g., "const char*", "int", "MyStruct").
 * @param value_type_name The C type name for hash map values (e.g., "int", "MyData*", "float").
 * @param hash_function_name Name of your custom hash function if you have one, NULL otherwise.
 * @param include_header_count Number of additional header files to include in the generated header.
 * @param ... Variable argument list of const char* strings representing additional header
 *            file names to include (e.g., "my_types.h", "custom_structs.h").
 *            Must provide exactly `include_header_count` string arguments.
 *
 * @return JSLFatPtr containing the generated header file content
 *
 * @warning Ensure the arena has sufficient space (minimum 512KB recommended) to avoid
 *          allocation failures during header generation.
 *
 * Example usage:
 * @code
 * JSLArena arena;
 * jsl_arena_init(&arena, malloc(JSL_MEGABYTES(1)), JSL_MEGABYTES(1));
 * 
 * JSLFatPtr header = write_hash_map_header(
 *     &arena,
 *     "StringIntMap",           // hash_map_name
 *     "string_int_map",         // function_prefix  
 *     "const char*",            // key_type_name
 *     "int",                    // value_type_name
 *     NULL,                     // hash_function_name (unused)
 *     2,                        // include_header_count
 *     "my_string_utils.h",      // additional include 1
 *     "my_common_types.h"       // additional include 2
 * );
 * 
 * // Write to file or use the generated header content
 * printf("%.*s", (int)header.length, (char*)header.data);
 * @endcode
 */
void write_hash_map_header(
    HashMapImplementation impl,
    JSLStringBuilder* builder,
    JSLFatPtr hash_map_name,
    JSLFatPtr function_prefix,
    JSLFatPtr key_type_name,
    JSLFatPtr value_type_name,
    JSLFatPtr hash_function_name,
    int32_t include_header_count,
    ...
)
{
    jsl_string_builder_format(builder, static_hash_map_header_docstring, hash_map_name, key_type_name, value_type_name);

    jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#pragma once\n\n"));
    jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#include <stdint.h>\n"));
    jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#include \"jacks_hash_map.h\"\n\n"));

    va_list args;
    va_start(args, include_header_count);

    for (int32_t i = 0; i < include_header_count; ++i)
    {
        jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#include \"%s\"\n"), va_arg(args, char*));
    }

    va_end(args);
    
    jsl_string_builder_format(builder, static_map_type_typedef, key_type_name, value_type_name, hash_map_name, key_type_name, value_type_name, hash_map_name);

    jsl_string_builder_format(builder, static_init_function_signature, function_prefix, hash_map_name);

    jsl_string_builder_format(builder, static_insert_function_signature, function_prefix, hash_map_name, key_type_name, value_type_name);

    jsl_string_builder_format(
        builder,
        static_get_function_signature,
        value_type_name,
        function_prefix,
        hash_map_name,
        key_type_name
    );

    jsl_string_builder_format(builder, static_delete_function_signature, function_prefix, hash_map_name, key_type_name);

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
    int32_t include_header_count,
    ...
)
{
    jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#include <stddef.h>\n"));
    jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#include <stdint.h>\n"));
    jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#include \"jacks_standard_library.h\"\n"));
    jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#include \"jacks_hash_map.h\"\n\n"));

    va_list args;
    va_start(args, include_header_count);

    for (int32_t i = 0; i < include_header_count; ++i)
    {
        jsl_string_builder_format(builder, JSL_FATPTR_LITERAL("#include \"%s\"\n"), va_arg(args, char*));
    }

    va_end(args);
    
    jsl_string_builder_format(
        builder,
        static_init_function_code,
        function_prefix,
        hash_map_name,
        value_type_name,
        value_type_name,
        value_type_name
    );
    
    jsl_string_builder_format(
        builder,
        static_hash_function_code,
        value_type_name,
        function_prefix,
        hash_map_name,
        key_type_name,
        key_type_name
    );
    
    jsl_string_builder_format(
        builder,
        static_insert_function_code
    );
    
    jsl_string_builder_format(
        builder,
        static_get_function_code
    );
    
    jsl_string_builder_format(
        builder,
        static_delete_function_code
    );
    
    jsl_string_builder_format(
        builder,
        static_iterator_start_function_code
    );
    
    jsl_string_builder_format(
        builder,
        static_iterator_next_function_code
    );

}

#ifdef INCLUDE_MAIN

JSLFatPtr help_message = JSL_FATPTR_LITERAL(
    "OVERVIEW:\n\n"
    "Hash map C code generation utility\n\n"
    "This program generates both a C source and header file for a hash map with the given\n"
    "key and value types. More documentation is included in the source file.\n\n"
    "USAGE:\n\n"
    "\tgenerate_hash_map --name TYPE_NAME --function_prefix PREFIX --key_type TYPE --value_type TYPE [--static | --dynamic] [--header | --source]\n\n"
    "Required arguments:\n"
    "\t--name\t\t\tThe name to give the hash map container type\n"
    "\t--function_prefix\tThe prefix on each of the functions for the hash map\n\n"
    "\t--key_type\t\tThe C type name for the key\n"
    "\t--value_type\t\tThe C type name for the value\n\n"
    "Optional arguments:\n"
    "\t--header\t\tWrite the header file to stdout\n"
    "\t--source\t\tWrite the source file to stdout\n"
    "\t--dynamic\t\tGenerate a hash map which grows dynamically\n"
    "\t--static\t\tGenerate a statically sized hash map\n"
);

int32_t main(int32_t argc, char** argv)
{
    bool show_help = false;
    bool print_header = false;
    JSLFatPtr name = {0};
    JSLFatPtr function_prefix = {0};
    JSLFatPtr key_type = {0};
    JSLFatPtr value_type = {0};
    JSLFatPtr hash_function_name = {0};
    HashMapImplementation impl = IMPL_ERROR;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        char* arg = argv[i];
        
        if (cstring_compare(arg, "-h") || cstring_compare(arg, "--help"))
        {
            show_help = true;
        }
        else if (cstring_compare(arg, "--name="))
        {
            name = jsl_fatptr_from_cstr(arg + 7);
        }
        else if (cstring_compare(arg, "--name"))
        {
            if (i + 1 < argc)
            {
                name = jsl_fatptr_from_cstr(argv[++i]);
            }
            else
            {
                fprintf(stderr, "Error: --name requires a value\n");
                return 1;
            }
        }
        else if (cstring_compare(arg, "--function_prefix="))
        {
            function_prefix = jsl_fatptr_from_cstr(arg + 18);
        }
        else if (cstring_compare(arg, "--function_prefix"))
        {
            if (i + 1 < argc) {
                function_prefix = jsl_fatptr_from_cstr(argv[++i]);
            } else {
                fprintf(stderr, "Error: --function_prefix requires a value\n");
                return 1;
            }
        }
        else if (cstring_compare(arg, "--key_type="))
        {
            key_type = jsl_fatptr_from_cstr(arg + 11);
        }
        else if (cstring_compare(arg, "--key_type"))
        {
            if (i + 1 < argc)
            {
                key_type = jsl_fatptr_from_cstr(argv[++i]);
            }
            else
            {
                fprintf(stderr, "Error: --key_type requires a value\n");
                return 1;
            }
        }
        else if (cstring_compare(arg, "--value_type="))
        {
            value_type = jsl_fatptr_from_cstr(arg + 13);
        }
        else if (cstring_compare(arg, "--value_type"))
        {
            if (i + 1 < argc)
            {
                value_type = jsl_fatptr_from_cstr(argv[++i]);
            }
            else
            {
                fprintf(stderr, "Error: --value_type requires a value\n");
                return 1;
            }
        }
        else if (cstring_compare(arg, "--static"))
        {
            impl = IMPL_STATIC;
        }
        else if (cstring_compare(arg, "--dynamic"))
        {
            impl = IMPL_DYNAMIC;
        }
        else if (cstring_compare(arg, "--header"))
        {
            print_header = true;
        }
        else if (cstring_compare(arg, "--source"))
        {
            print_header = false;
        }
        else
        {
            fprintf(stderr, "Error: Unknown argument: %s\n", arg);
            return 1;
        }
    }
    
    if (show_help)
    {
        jsl_format_file(stdout, help_message);
        return 0;
    }
    // Check that all required parameters are provided
    else if (name.data == NULL)
    {
        jsl_format_file(stderr, JSL_FATPTR_LITERAL("Error: --name is required\n"));
        return 1;
    }

    if (function_prefix.data == NULL)
    {
        jsl_format_file(stderr, JSL_FATPTR_LITERAL("Error: --function_prefix is required\n"));
        return 1;
    }

    if (key_type.data == NULL)
    {
        jsl_format_file(stderr, JSL_FATPTR_LITERAL("Error: --key_type is required\n"));
        return 1;
    }

    if (value_type.data == NULL)
    {
        jsl_format_file(stderr, JSL_FATPTR_LITERAL("Error: --value_type is required\n"));
        return 1;
    }

    if (impl == IMPL_ERROR)
    {
        impl = IMPL_DYNAMIC;
    }
    
    JSLArena arena;
    jsl_arena_init(&arena, malloc(JSL_MEGABYTES(8)), JSL_MEGABYTES(8));
    
    JSLStringBuilder builder;
    jsl_string_builder_init2(&builder, &arena, 512, 32);

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
            0
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
            0
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

#endif // INCLUDE_MAIN

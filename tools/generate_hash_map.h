/**
 * # Generate Hash Map Tool
 * 
 * Generate the C header and source files for a hash map before compilation.
 * 
 * The utility generates a header file and a C file for a type safe, open addressed,
 * linear probed, hash map. By generating the code rather than using macros, two
 * benefits are gained. One, the code is much easier to debug. Two, it's much more
 * obvious how much code you're generating, which means you are much less likely to
 * accidentally create the combinatoric explosion of code that's so common in C++
 * projects. Sometimes, adding friction to things is good.
 * 
 * There are two implementations of hash map that this utility can generate.
 * 
 * 1. A fixed size hash map that cannot grow. You set the max item count at
 *    init. This reduces memory fragmentation in arenas and it reduces failure
 *    modes in later parts of the program
 * 2. A standard dynamic hash map.
 * 
 * ## Usage
 * 
 * This tool is usable as both a command line tool and a C library. Use the
 * command line tool for traditional GNU make style builds and use the C
 * library for "metaprogram" style builds.
 * 
 * ### CLI Program
 * 
 * Compile the program with
 * 
 * ```
 * $ cc -o generate_hash_map tools/generate_hash_map.c
 * ```
 * 
 * or on Windows with
 * 
 * ```
 * > cl.exe /Fegenerate_hash_map tools\generate_hash_map.c
 * ```
 * 
 * Get the help message with
 * 
 * ```
 * $ ./generate_hash_map --help
 * ```
 * 
 * ### Library
 * 
 * The `generate_hash_map.h` file is a single-header-file library. In
 * every file that uses the code include the header normally.
 * 
 * ```
 * #include "generate_hash_map.h"
 * ```
 * 
 * Then in one, and only one, file define the implementation macro
 * 
 * ```
 * #define GENERATE_HASH_MAP_IMPLEMENTATION
 * #include "generate_hash_map.h"
 * ```
 * 
 * The two relevent functions are write_hash_map_header and write_hash_map_source
 * 
 * ## License
 *
 * Copyright (c) 2026 Jack Stouffer
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

#ifndef GENERATE_HASH_MAP_H_INCLUDED
    #define GENERATE_HASH_MAP_H_INCLUDED

    #include <stdint.h>
    #include <stddef.h>
    #include <inttypes.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "../src/jsl_core.h"
    #include "../src/jsl_allocator.h"
    #include "../src/jsl_allocator_arena.h"

    /* Versioning to catch mismatches across deps */
    #ifndef GENERATE_HASH_MAP_VERSION
        #define GENERATE_HASH_MAP_VERSION 0x010000  /* 1.0.0 */
    #else
        #if GENERATE_HASH_MAP_VERSION != 0x010000
            #error "generate_hash_map.h version mismatch across includes"
        #endif
    #endif

    #ifndef GENERATE_HASH_MAP_DEF
        #define GENERATE_HASH_MAP_DEF
    #endif

    #ifdef __cplusplus
    extern "C" {
    #endif

    typedef enum {
        IMPL_ERROR,
        IMPL_FIXED,
        IMPL_DYNAMIC
    } HashMapImplementation;

    /**
     * Generate the text of the C header and insert it into the string sink.
     * 
     * @param arena Used for all memory allocations
     * @param sink Used to insert the generated text
     * @param impl Which hash map implementation to use
     * @param hash_map_name The name of the container type
     * @param function_prefix The prefix plus "_" for each function
     * @param key_type_name The type of the hash map key
     * @param value_type_name The type of the hash map value
     * @param hash_function_name If you have a custom hash function, put it here, otherwise pass NULL
     * @param include_header_array If you need custom header includes then set this, otherwise pass NULL
     * @param include_header_count The length of the header array
     */
    GENERATE_HASH_MAP_DEF void write_hash_map_header(
        JSLAllocatorInterface* allocator,
        JSLOutputSink sink,
        HashMapImplementation impl,
        JSLImmutableMemory hash_map_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory key_type_name,
        JSLImmutableMemory value_type_name,
        JSLImmutableMemory hash_function_name,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    );

    /**
     * Generate the text of the C source and insert it into the string sink.
     * 
     * @param arena Used for all memory allocations
     * @param sink Used to insert the generated text
     * @param impl Which hash map implementation to use
     * @param hash_map_name The name of the container type
     * @param function_prefix The prefix plus "_" for each function
     * @param key_type_name The type of the hash map key
     * @param value_type_name The type of the hash map value
     * @param hash_function_name If you have a custom hash function, put it here, otherwise pass NULL
     * @param include_header_array If you need custom header includes then set this, otherwise pass NULL
     * @param include_header_count The length of the header array
     */
    GENERATE_HASH_MAP_DEF void write_hash_map_source(
        JSLAllocatorInterface* allocator,
        JSLOutputSink sink,
        HashMapImplementation impl,
        JSLImmutableMemory hash_map_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory key_type_name,
        JSLImmutableMemory value_type_name,
        JSLImmutableMemory hash_function_name,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    );
    
    #ifdef __cplusplus
    }
    #endif
    
#endif /* GENERATE_HASH_MAP_H_INCLUDED */

#ifdef GENERATE_HASH_MAP_IMPLEMENTATION

    #include <stdlib.h>
    #include <time.h>
    #include <assert.h>

    #include "../tools/templates/dynamic_hash_map_header.h"
    #include "../tools/templates/dynamic_hash_map_source.h"

    static JSLImmutableMemory fixed_header_template = JSL_FATPTR_INITIALIZER(
        "/**\n"
        " * AUTO GENERATED FILE\n"
        " *\n"
        " * This file contains the header for a hash map `{{ hash_map_name }}` which maps\n"
        " * `{{ key_type_name }}` keys to `{{ value_type_name }}` values.\n"
        " *\n"
        " * This file was auto generated from the hash map generation utility that's part of\n"
        " * the \"Jack's Standard Library\" project. The utility generates a header file and a\n"
        " * C file for a type safe, open addressed, hash map. By generating the code rather\n"
        " * than using macros, two benefits are gained. One, the code is much easier to debug.\n"
        " * Two, it's much more obvious how much code you're generating, which means you are\n"
        " * much less likely to accidentally create the combinatoric explosion of code that's\n"
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
        " */\n"
        "\n"
        "/**\n"
        " * A hash map which maps `{{ key_type_name }}` keys to `{{ value_type_name }}` values.\n"
        " *\n"
        " * This hash map uses open addressing with linear probing. However, it never grows.\n"
        " * When initialized with the init function, all the memory this hash map will have\n"
        " * is allocated right away.\n"
        " */\n"
        "typedef struct {{ hash_map_name }} {\n"
        "    // putting the sentinel first means it's much more likely to get\n"
        "    // corrupted from accidental overwrites, therefore making it\n"
        "    // more likely that memory bugs are caught.\n"
        "    uint32_t sentinel;\n"
        "    uint32_t generational_id;\n"
        "\n"
        "    {{ key_type_name }}* keys_array;\n"
        "    {{ value_type_name }}* values_array;\n"
        "    uint64_t* hashes_array;\n"
        "    int64_t arrays_length;\n"
        "    int64_t item_count;\n"
        "    int64_t max_item_count;\n"
        "    uint64_t seed;\n"
        "} {{ hash_map_name }};\n"
        "\n"
        "/**\n"
        " * Iterator type which is used by the iterator functions to\n"
        " * allow you to loop over the hash map contents.\n"
        " */\n"
        "typedef struct {{ hash_map_name }}Iterator {\n"
        "    {{ hash_map_name }}* hash_map;\n"
        "    int64_t current_slot;\n"
        "    uint64_t generational_id;\n"
        "} {{ hash_map_name }}Iterator;\n"
        "\n"
        "/**\n"
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
        "bool {{ function_prefix }}_init(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    JSLAllocatorInterface* allocator,\n"
        "    int64_t max_item_count,\n"
        "    uint64_t seed\n"
        ");\n"
        "\n"
        "/**\n"
        " * Insert the given value into the hash map. If the key already exists in \n"
        " * the map the value will be overwritten. If the key type for this hash map\n"
        " * is a pointer, then a NULL key is a valid key type.\n"
        " *\n"
        " * @param hash_map The pointer to the hash map instance to initialize\n"
        " * @param key Hash map key\n"
        " * @param value Value to store\n"
        " * @returns A bool representing success or failure of insertion.\n"
        " */\n"
        "bool {{ function_prefix }}_insert(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ key_type_name }} key,\n"
        "    {{ value_type_name }} value\n"
        ");\n"
        "\n"
        "/**\n"
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
        "{{ value_type_name }}* {{ function_prefix }}_get(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ key_type_name }} key\n"
        ");\n"
        "\n"
        "/**\n"
        " * Remove a key/value pair from the hash map if it exists.\n"
        " * If it does not false is returned.\n"
        " *\n"
        " * This hash map uses backshift deletion instead of tombstones\n"
        " * due to the lack of rehashing. Deletion can be expensive in\n"
        " * medium sized maps.\n"
        " */\n"
        "bool {{ function_prefix }}_delete(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ key_type_name }} key\n"
        ");\n"
        "\n"
        "/**\n"
        " * Create a new iterator over this hash map.\n"
        " *\n"
        " * An iterator is a struct which holds enough state that it allows a loop to visit\n"
        " * each key/value pair in the hash map.\n"
        " *\n"
        " * Iterating over a hash map while modifying it does not have guaranteed\n"
        " * correctness. Any insertion or deletion after the iterator is created will\n"
        " * invalidate the iteration.\n"
        " *\n"
        " * Example usage:\n"
        " * @code\n"
        " * {{ key_type_name }} key;\n"
        " * {{ value_type_name }} value;\n"
        " * {{ hash_map_name }}Iterator iterator;\n"
        " * {{ function_prefix }}_iterator_start(hash_map, &iterator);\n"
        " * while ({{ function_prefix }}_iterator_next(&iterator, &key, &value))\n"
        " * {\n"
        " *     ...\n"
        " * }\n"
        " * @endcode\n"
        " */\n"
        "bool {{ function_prefix }}_iterator_start(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ hash_map_name }}Iterator* iterator\n"
        ");\n"
        "\n"
        "/**\n"
        " * Iterate over the hash map. If a key/value was found then true is returned.\n"
        " *\n"
        " * Example usage:\n"
        " * @code\n"
        " * {{ key_type_name }} key;\n"
        " * {{ value_type_name }} value;\n"
        " * {{ hash_map_name }}Iterator iterator;\n"
        " * {{ function_prefix }}_iterator_start(hash_map, &iterator);\n"
        " * while ({{ function_prefix }}_iterator_next(&iterator, &key, &value))\n"
        " * {\n"
        " *     ...\n"
        " * }\n"
        " * @endcode\n"
        " */\n"
        "bool {{ function_prefix }}_iterator_next(\n"
        "    {{ hash_map_name }}Iterator* iterator,\n"
        "    {{ key_type_name }}* key,\n"
        "    {{ value_type_name }}* value\n"
        ");\n"
    );

    static JSLImmutableMemory fixed_source_template = JSL_FATPTR_INITIALIZER(
        "/**\n"
        " * AUTO GENERATED FILE\n"
        " *\n"
        " * This file contains the source for a hash map `{{ hash_map_name }}` which maps\n"
        " * `{{ key_type_name }}` keys to `{{ value_type_name }}` values.\n"
        " *\n"
        " * This file was auto generated from the hash map generation utility that's part of\n"
        " * the \"Jack's Standard Library\" project. The utility generates a header file and a\n"
        " * C file for a type safe, open addressed, linear probed, hash map. By generating\n"
        " * the code rather than using macros, two benefits are gained. One, the code is much\n"
        " * easier to debug. Two, it's much more obvious how much code you're generating,\n"
        " * which means you are much less likely to accidentally create the combinatoric\n"
        " * explosion of code that's so common in C++ projects. Sometimes, adding friction\n"
        " * to things is good.\n"
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
        " */\n"
        "\n"
        "bool {{ function_prefix }}_init(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    JSLAllocatorInterface* allocator,\n"
        "    int64_t max_item_count,\n"
        "    uint64_t seed\n"
        ")\n"
        "{\n"
        "    if (hash_map == NULL || allocator == NULL || max_item_count < 0)\n"
        "        return false;\n"
        "\n"
        "    JSL_MEMSET(hash_map, 0, sizeof({{ hash_map_name }}));\n"
        "\n"
        "    hash_map->seed = seed;\n"
        "    hash_map->max_item_count = max_item_count;\n"
        "\n"
        "    int64_t max_with_load_factor = (int64_t) ((float) max_item_count / 0.75f);\n"
        "\n"
        "    hash_map->arrays_length = jsl_next_power_of_two_i64(max_with_load_factor);\n"
        "    hash_map->arrays_length = JSL_MAX(hash_map->arrays_length, 32);\n"
        "\n"
        "    hash_map->keys_array = ({{ key_type_name }}*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof({{ key_type_name }})) * hash_map->arrays_length,\n"
        "        (int32_t) _Alignof({{ key_type_name }}),\n"
        "        false\n"
        "    );\n"
        "    if (hash_map->keys_array == NULL)\n"
        "        return false;\n"
        "\n"
        "    hash_map->values_array = ({{ value_type_name }}*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof({{ value_type_name }})) * hash_map->arrays_length,\n"
        "        (int32_t) _Alignof({{ value_type_name }}),\n"
        "        false\n"
        "    );\n"
        "    if (hash_map->values_array == NULL)\n"
        "        return false;\n"
        "\n"
        "    hash_map->hashes_array = (uint64_t*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof(uint64_t)) * hash_map->arrays_length,\n"
        "        (int32_t) _Alignof(uint64_t),\n"
        "        true\n"
        "    );\n"
        "    if (hash_map->hashes_array == NULL)\n"
        "        return false;\n"
        "\n"
        "    hash_map->sentinel = PRIVATE_SENTINEL_{{ hash_map_name }};\n"
        "    return true;\n"
        "}\n"
        "\n"
        "static inline void {{ function_prefix }}_probe(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ key_type_name }} key,\n"
        "    int64_t* out_slot,\n"
        "    uint64_t* out_hash,\n"
        "    bool* out_found\n"
        ")\n"
        "{\n"
        "    *out_slot = -1;\n"
        "    *out_found = false;\n"
        "\n"
        "    {{ hash_function }};\n"
        "\n"
        "    // Avoid clashing with sentinel values\n"
        "    if (*out_hash <= (uint64_t) JSL__HASHMAP_TOMBSTONE)\n"
        "    {\n"
        "        *out_hash = (uint64_t) JSL__HASHMAP_VALUE_OK;\n"
        "    }\n"
        "\n"
        "    int64_t total_checked = 0;\n"
        "    uint64_t slot_mask = (uint64_t) hash_map->arrays_length - 1u;\n"
        "    // Since our slot array length is always a pow 2, we can avoid a modulo\n"
        "    int64_t slot = (int64_t) (*out_hash & slot_mask);\n"
        "\n"
        "    while (total_checked < hash_map->arrays_length)\n"
        "    {\n"
        "        uint64_t slot_hash_value = hash_map->hashes_array[slot];\n"
        "\n"
        "        if (slot_hash_value == JSL__HASHMAP_EMPTY)\n"
        "        {\n"
        "            *out_slot = slot;\n"
        "            break;\n"
        "        }\n"
        "\n"
        "        if (slot_hash_value == *out_hash && {{ key_compare }})\n"
        "        {\n"
        "            *out_found = true;\n"
        "            *out_slot = slot;\n"
        "            break;\n"
        "        }\n"
        "\n"
        "        slot = (int64_t) (((uint64_t) slot + 1u) & slot_mask);\n"
        "        ++total_checked;\n"
        "    }\n"
        "\n"
        "    if (total_checked >= hash_map->arrays_length)\n"
        "    {\n"
        "        *out_slot = -1;\n"
        "    }\n"
        "}\n"
        "\n"
        "static inline void {{ function_prefix }}_backshift(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    int64_t start_slot\n"
        ")\n"
        "{\n"
        "    uint64_t slot_mask = (uint64_t) hash_map->arrays_length - 1u;\n"
        "\n"
        "    int64_t hole = start_slot;\n"
        "    int64_t current = (int64_t) (((uint64_t) start_slot + 1u) & slot_mask);\n"
        "\n"
        "    int64_t loop_check = 0;\n"
        "    while (loop_check < hash_map->arrays_length)\n"
        "    {\n"
        "        uint64_t hash_value = hash_map->hashes_array[current];\n"
        "\n"
        "        if (hash_value == JSL__HASHMAP_EMPTY)\n"
        "        {\n"
        "            hash_map->hashes_array[hole] = JSL__HASHMAP_EMPTY;\n"
        "            break;\n"
        "        }\n"
        "\n"
        "        int64_t ideal_slot = (int64_t) (hash_value & slot_mask);\n"
        "\n"
        "        bool should_move = (current > hole)\n"
        "            ? (ideal_slot <= hole || ideal_slot > current)\n"
        "            : (ideal_slot <= hole && ideal_slot > current);\n"
        "\n"
        "        if (should_move)\n"
        "        {\n"
        "            hash_map->keys_array[hole] = hash_map->keys_array[current];\n"
        "            hash_map->values_array[hole] = hash_map->values_array[current];\n"
        "            hash_map->hashes_array[hole] = hash_map->hashes_array[current];\n"
        "            hole = current;\n"
        "        }\n"
        "\n"
        "        current = (int64_t) (((uint64_t) current + 1u) & slot_mask);\n"
        "\n"
        "        ++loop_check;\n"
        "    }\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_insert(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ key_type_name }} key,\n"
        "    {{ value_type_name }} value\n"
        ")\n"
        "{\n"
        "    bool insert_success = false;\n"
        "\n"
        "    if (\n"
        "        hash_map == NULL\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\n"
        "        || hash_map->values_array == NULL\n"
        "        || hash_map->keys_array == NULL\n"
        "        || hash_map->hashes_array == NULL\n"
        "        || hash_map->item_count >= hash_map->max_item_count\n"
        "    )\n"
        "        return insert_success;\n"
        "\n"
        "    uint64_t hash = 0;\n"
        "    int64_t slot = -1;\n"
        "    bool existing_found = false;\n"
        "    {{ function_prefix }}_probe(hash_map, key, &slot, &hash, &existing_found);\n"
        "\n"
        "    // new key\n"
        "    if (slot > -1 && !existing_found)\n"
        "    {\n"
        "        hash_map->keys_array[slot] = key;\n"
        "        hash_map->values_array[slot] = value;\n"
        "        hash_map->hashes_array[slot] = hash;\n"
        "        ++hash_map->item_count;\n"
        "        insert_success = true;\n"
        "    }\n"
        "    // update\n"
        "    else if (slot > -1 && existing_found)\n"
        "    {\n"
        "        hash_map->values_array[slot] = value;\n"
        "        insert_success = true;\n"
        "    }\n"
        "\n"
        "    if (insert_success)\n"
        "    {\n"
        "        ++hash_map->generational_id;\n"
        "    }\n"
        "\n"
        "    return insert_success;\n"
        "}\n"
        "\n"
        "{{ value_type_name }}* {{ function_prefix }}_get(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ key_type_name }} key\n"
        ")\n"
        "{\n"
        "    {{ value_type_name }}* res = NULL;\n"
        "\n"
        "    if (\n"
        "        hash_map == NULL\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\n"
        "        || hash_map->values_array == NULL\n"
        "        || hash_map->keys_array == NULL\n"
        "        || hash_map->hashes_array == NULL\n"
        "    )\n"
        "        return res;\n"
        "\n"
        "    uint64_t hash = 0;\n"
        "    int64_t slot = -1;\n"
        "    bool existing_found = false;\n"
        "    {{ function_prefix }}_probe(hash_map, key, &slot, &hash, &existing_found);\n"
        "    \n"
        "    if (slot > -1 && existing_found)\n"
        "    {\n"
        "        res = &hash_map->values_array[slot];\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_delete(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ key_type_name }} key\n"
        ")\n"
        "{\n"
        "    bool success = false;\n"
        "\n"
        "    if (\n"
        "        hash_map == NULL\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\n"
        "        || hash_map->values_array == NULL\n"
        "        || hash_map->keys_array == NULL\n"
        "        || hash_map->hashes_array == NULL\n"
        "    )\n"
        "        return success;\n"
        "\n"
        "    uint64_t hash = 0;\n"
        "    int64_t slot = -1;\n"
        "    bool existing_found = false;\n"
        "    {{ function_prefix }}_probe(hash_map, key, &slot, &hash, &existing_found);\n"
        "\n"
        "    if (slot > -1 && existing_found)\n"
        "    {\n"
        "        {{ function_prefix }}_backshift(hash_map, slot);\n"
        "        --hash_map->item_count;\n"
        "        ++hash_map->generational_id;\n"
        "        success = true;\n"
        "    }\n"
        "\n"
        "    return success;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_iterator_start(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {{ hash_map_name }}Iterator* iterator\n"
        ")\n"
        "{\n"
        "    if (\n"
        "        hash_map == NULL\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\n"
        "        || hash_map->values_array == NULL\n"
        "        || hash_map->keys_array == NULL\n"
        "        || hash_map->hashes_array == NULL\n"
        "    )\n"
        "        return false;\n"
        "\n"
        "    iterator->hash_map = hash_map;\n"
        "    iterator->current_slot = 0;\n"
        "    iterator->generational_id = hash_map->generational_id;\n"
        "\n"
        "    return true;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_iterator_next(\n"
        "    {{ hash_map_name }}Iterator* iterator,\n"
        "    {{ key_type_name }}* out_key,\n"
        "    {{ value_type_name }}* out_value\n"
        ")\n"
        "{\n"
        "    bool found = false;\n"
        "\n"
        "    if (\n"
        "        iterator == NULL\n"
        "        || iterator->hash_map == NULL\n"
        "        || iterator->hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\n"
        "        || iterator->hash_map->generational_id != iterator->generational_id\n"
        "        || iterator->hash_map->values_array == NULL\n"
        "        || iterator->hash_map->keys_array == NULL\n"
        "        || iterator->hash_map->hashes_array == NULL\n"
        "    )\n"
        "        return found;\n"
        "\n"
        "    int64_t found_entry = -1;\n"
        "\n"
        "    while (iterator->current_slot < iterator->hash_map->arrays_length)\n"
        "    {\n"
        "        uint64_t hash_value = iterator->hash_map->hashes_array[iterator->current_slot];\n"
        "\n"
        "        bool occupied = hash_value != JSL__HASHMAP_EMPTY;\n"
        "\n"
        "        if (occupied)\n"
        "        {\n"
        "            found_entry = iterator->current_slot;\n"
        "            break;\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            ++iterator->current_slot;\n"
        "        }\n"
        "    }\n"
        "\n"
        "    if (found_entry > -1)\n"
        "    {\n"
        "        *out_key = iterator->hash_map->keys_array[iterator->current_slot];\n"
        "        *out_value = iterator->hash_map->values_array[iterator->current_slot];\n"
        "        ++iterator->current_slot;\n"
        "        found = true;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        iterator->current_slot = iterator->hash_map->arrays_length;\n"
        "        found = false;\n"
        "    }\n"
        "\n"
        "    return found;\n"
        "}\n"
    );

    static JSLImmutableMemory hash_map_name_key = JSL_FATPTR_INITIALIZER("hash_map_name");
    static JSLImmutableMemory key_type_name_key = JSL_FATPTR_INITIALIZER("key_type_name");
    static JSLImmutableMemory value_type_name_key = JSL_FATPTR_INITIALIZER("value_type_name");
    static JSLImmutableMemory function_prefix_key = JSL_FATPTR_INITIALIZER("function_prefix");
    static JSLImmutableMemory hash_function_key = JSL_FATPTR_INITIALIZER("hash_function");
    static JSLImmutableMemory key_compare_key = JSL_FATPTR_INITIALIZER("key_compare");

    static JSLImmutableMemory int32_t_str = JSL_FATPTR_INITIALIZER("int32_t");
    static JSLImmutableMemory int_str = JSL_FATPTR_INITIALIZER("int");
    static JSLImmutableMemory unsigned_str = JSL_FATPTR_INITIALIZER("unsigned");
    static JSLImmutableMemory unsigned_int_str = JSL_FATPTR_INITIALIZER("unsigned int");
    static JSLImmutableMemory uint32_t_str = JSL_FATPTR_INITIALIZER("uint32_t");
    static JSLImmutableMemory int64_t_str = JSL_FATPTR_INITIALIZER("int64_t");
    static JSLImmutableMemory long_str = JSL_FATPTR_INITIALIZER("long");
    static JSLImmutableMemory long_int_str = JSL_FATPTR_INITIALIZER("long int");
    static JSLImmutableMemory long_long_str = JSL_FATPTR_INITIALIZER("long long");
    static JSLImmutableMemory long_long_int_str = JSL_FATPTR_INITIALIZER("long long int");
    static JSLImmutableMemory uint64_t_str = JSL_FATPTR_INITIALIZER("uint64_t");
    static JSLImmutableMemory unsigned_long_str = JSL_FATPTR_INITIALIZER("unsigned long");
    static JSLImmutableMemory unsigned_long_long_str = JSL_FATPTR_INITIALIZER("unsigned long long");
    static JSLImmutableMemory unsigned_long_long_int_str = JSL_FATPTR_INITIALIZER("unsigned long long int");

    // because rand max on some platforms is 32k
    static inline uint32_t rand_u32(void)
    {
        uint32_t value = 0;

        value = (value << 8) | (uint32_t)(rand() & 0xFF);
        value = (value << 8) | (uint32_t)(rand() & 0xFF);
        value = (value << 8) | (uint32_t)(rand() & 0xFF);
        value = (value << 8) | (uint32_t)(rand() & 0xFF);

        return value;
    }

    static void render_template(
        JSLOutputSink sink,
        JSLImmutableMemory template,
        JSLStrToStrMap* variables
    )
    {
        static JSLImmutableMemory open_param = JSL_FATPTR_INITIALIZER("{{");
        static JSLImmutableMemory close_param = JSL_FATPTR_INITIALIZER("}}");
        JSLImmutableMemory template_reader = template;
        
        while (template_reader.length > 0)
        {
            int64_t index_of_open = jsl_fatptr_substring_search(template_reader, open_param);

            // No more variables, write everything
            if (index_of_open == -1)
            {
                jsl_output_sink_write_fatptr(sink, template_reader);
                break;
            }

            if (index_of_open > 0)
            {
                JSLImmutableMemory slice = jsl_slice(template_reader, 0, index_of_open);
                jsl_output_sink_write_fatptr(sink, slice);
            }

            JSL_FATPTR_ADVANCE(template_reader, index_of_open + open_param.length);

            int64_t index_of_close = jsl_fatptr_substring_search(template_reader, close_param);

            // Improperly closed template param, write everything including the open marker
            if (index_of_close == -1)
            {
                jsl_output_sink_write_fatptr(sink, open_param);
                jsl_output_sink_write_fatptr(sink, template_reader);
                break;
            }

            JSLImmutableMemory var_name = jsl_slice(template_reader, 0, index_of_close);
            jsl_fatptr_strip_whitespace(&var_name);

            JSLImmutableMemory var_value;
            if (jsl_str_to_str_map_get(variables, var_name, &var_value))
            {
                jsl_output_sink_write_fatptr(sink, var_value);
            }

            JSL_FATPTR_ADVANCE(template_reader, index_of_close + close_param.length);
        }
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
     * @param key_type_name The C type name for hash map keys (e.g., "int", "MyStruct").
     * @param value_type_name The C type name for hash map values (e.g., "int", "MyData*", "float").
     * @param hash_function_name Name of your custom hash function if you have one, NULL otherwise.
     * @param include_header_count Number of additional header files to include in the generated header.
     * @param include_header_count Number of additional header files to include in the generated header.

    * @return JSLImmutableMemory containing the generated header file content
    *
    * @warning Ensure the arena has sufficient space (minimum 512KB recommended) to avoid
    *          allocation failures during header generation.
    */
    GENERATE_HASH_MAP_DEF void write_hash_map_header(
        JSLAllocatorInterface* allocator,
        JSLOutputSink sink,
        HashMapImplementation impl,
        JSLImmutableMemory hash_map_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory key_type_name,
        JSLImmutableMemory value_type_name,
        JSLImmutableMemory hash_function_name,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    )
    {
        (void) hash_function_name;

        srand((uint32_t) (time(NULL) % UINT32_MAX));

        jsl_output_sink_write_fatptr(
            sink,
            JSL_FATPTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
        );

        jsl_output_sink_write_fatptr(sink, JSL_FATPTR_EXPRESSION("#pragma once\n\n"));
        jsl_output_sink_write_fatptr(sink, JSL_FATPTR_EXPRESSION("#include <stdint.h>\n"));
        jsl_output_sink_write_fatptr(sink, JSL_FATPTR_EXPRESSION("#include \"jsl_allocator.h\"\n"));
        jsl_output_sink_write_fatptr(sink, JSL_FATPTR_EXPRESSION("#include \"jsl_hash_map_common.h\"\n"));
        jsl_output_sink_write_u8(
            sink,
            '\n'
        );

        jsl_output_sink_write_fatptr(
            sink,
            JSL_FATPTR_EXPRESSION("// USER INCLUDED HEADERS\n")
        );

        for (int32_t i = 0; i < include_header_count; ++i)
        {
            jsl_format_sink(sink, JSL_FATPTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
        }

        jsl_output_sink_write_fatptr(sink, JSL_FATPTR_EXPRESSION("\n"));
        
        jsl_format_sink(
            sink,
            JSL_FATPTR_EXPRESSION("#define PRIVATE_SENTINEL_%y %" PRIu32 "U \n"),
            hash_map_name,
            rand_u32()
        );

        jsl_output_sink_write_fatptr(sink, JSL_FATPTR_EXPRESSION("\n"));

        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, allocator, 0x123456789);

        jsl_str_to_str_map_insert(
            &map,
            hash_map_name_key,
            JSL_STRING_LIFETIME_STATIC,
            hash_map_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            key_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            key_type_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            value_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            value_type_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            function_prefix_key,
            JSL_STRING_LIFETIME_STATIC,
            function_prefix,
            JSL_STRING_LIFETIME_STATIC
        );

        if (impl == IMPL_FIXED)
            render_template(sink, fixed_header_template, &map);
        else if (impl == IMPL_DYNAMIC)
            render_template(sink, dynamic_header_template, &map);
        else
            assert(0);
    }

    GENERATE_HASH_MAP_DEF void write_hash_map_source(
        JSLAllocatorInterface* allocator,
        JSLOutputSink sink,
        HashMapImplementation impl,
        JSLImmutableMemory hash_map_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory key_type_name,
        JSLImmutableMemory value_type_name,
        JSLImmutableMemory hash_function_name,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    )
    {
        (void) impl;

        jsl_output_sink_write_fatptr(
            sink,
            JSL_FATPTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
        );

        jsl_output_sink_write_fatptr(
            sink,
            JSL_FATPTR_EXPRESSION("#include <stddef.h>\n")
        );
        jsl_output_sink_write_fatptr(
            sink,
            JSL_FATPTR_EXPRESSION("#include <stdint.h>\n")
        );
        jsl_output_sink_write_fatptr(
            sink,
            JSL_FATPTR_EXPRESSION("#include \"jsl_core.h\"\n")
        );
        jsl_output_sink_write_fatptr(sink, JSL_FATPTR_EXPRESSION("#include \"jsl_allocator.h\"\n"));
        jsl_output_sink_write_fatptr(
            sink,
            JSL_FATPTR_EXPRESSION("#include \"jsl_hash_map_common.h\"\n\n")
        );

        jsl_output_sink_write_fatptr(
            sink,
            JSL_FATPTR_EXPRESSION("// USER INCLUDED HEADERS\n")
        );

        for (int32_t i = 0; i < include_header_count; ++i)
        {
            jsl_format_sink(sink, JSL_FATPTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
        }

        jsl_output_sink_write_fatptr(sink, JSL_FATPTR_EXPRESSION("\n"));


        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, allocator, 0x123456789);

        jsl_str_to_str_map_insert(
            &map,
            hash_map_name_key,
            JSL_STRING_LIFETIME_STATIC,
            hash_map_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            key_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            key_type_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            value_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            value_type_name,
            JSL_STRING_LIFETIME_STATIC
        );
        jsl_str_to_str_map_insert(
            &map,
            function_prefix_key,
            JSL_STRING_LIFETIME_STATIC,
            function_prefix,
            JSL_STRING_LIFETIME_STATIC
        );

        // hash and find slot
        {
            uint8_t hash_function_call_buffer[JSL_KILOBYTES(4)];
            JSLArena hash_function_scratch_arena = JSL_ARENA_FROM_STACK(hash_function_call_buffer);
            JSLAllocatorInterface scratch_interface;
            jsl_arena_get_allocator_interface(&scratch_interface, &hash_function_scratch_arena);

            JSLImmutableMemory resolved_hash_function_call;
            if (hash_function_name.data != NULL && hash_function_name.length > 0)
            {
                resolved_hash_function_call = jsl_format(
                    &scratch_interface,
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
                || jsl_fatptr_memory_compare(key_type_name, long_int_str)
                || jsl_fatptr_memory_compare(key_type_name, long_long_str)
                || jsl_fatptr_memory_compare(key_type_name, long_long_int_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_long_long_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_long_long_int_str)
                || key_type_name.data[key_type_name.length - 1] == '*'
            )
            {
                resolved_hash_function_call = jsl_format(
                    &scratch_interface,
                    JSL_FATPTR_EXPRESSION("*out_hash = jsl__murmur3_fmix_u64((uint64_t) key, hash_map->seed)"),
                    key_type_name
                );
            }
            else
            {
                resolved_hash_function_call = jsl_format(
                    &scratch_interface,
                    JSL_FATPTR_EXPRESSION("*out_hash = jsl__rapidhash_withSeed(&key, sizeof(%y), hash_map->seed)"),
                    key_type_name
                );
            }

            jsl_str_to_str_map_insert(
                &map,
                hash_function_key,
                JSL_STRING_LIFETIME_STATIC,
                resolved_hash_function_call,
                JSL_STRING_LIFETIME_STATIC
            );
        }

        // key comparison
        {
            uint8_t resolved_key_buffer[JSL_KILOBYTES(4)];
            JSLArena scratch_arena = JSL_ARENA_FROM_STACK(resolved_key_buffer);
            JSLAllocatorInterface scratch_interface;
            jsl_arena_get_allocator_interface(&scratch_interface, &scratch_arena);

            JSLImmutableMemory resolved_key_compare;

            if (
                jsl_fatptr_memory_compare(key_type_name, int32_t_str)
                || jsl_fatptr_memory_compare(key_type_name, int_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_int_str)
                || jsl_fatptr_memory_compare(key_type_name, uint32_t_str)
                || jsl_fatptr_memory_compare(key_type_name, int64_t_str)
                || jsl_fatptr_memory_compare(key_type_name, long_str)
                || jsl_fatptr_memory_compare(key_type_name, uint64_t_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_long_str)
                || jsl_fatptr_memory_compare(key_type_name, long_int_str)
                || jsl_fatptr_memory_compare(key_type_name, long_long_str)
                || jsl_fatptr_memory_compare(key_type_name, long_long_int_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_long_long_str)
                || jsl_fatptr_memory_compare(key_type_name, unsigned_long_long_int_str)
                || key_type_name.data[key_type_name.length - 1] == '*'
            )
            {
                resolved_key_compare = jsl_format(
                    &scratch_interface,
                    JSL_FATPTR_EXPRESSION("key == hash_map->keys_array[slot]"),
                    key_type_name
                );
            }
            else
            {
                resolved_key_compare = jsl_format(
                    &scratch_interface,
                    JSL_FATPTR_EXPRESSION("JSL_MEMCMP(&key, &hash_map->keys_array[slot], sizeof(%y)) == 0"),
                    key_type_name
                );
            }

            jsl_str_to_str_map_insert(
                &map,
                key_compare_key,
                JSL_STRING_LIFETIME_STATIC,
                resolved_key_compare,
                JSL_STRING_LIFETIME_STATIC
            );
        }

        render_template(sink, fixed_source_template, &map);
    }

#endif /* GENERATE_HASH_MAP_IMPLEMENTATION */

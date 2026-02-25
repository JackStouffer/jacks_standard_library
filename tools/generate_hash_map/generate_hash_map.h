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

    #include "jsl/core.h"
    #include "jsl/allocator.h"
    #include "jsl/allocator_arena.h"

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
        JSLAllocatorInterface allocator,
        JSLOutputSink sink,
        HashMapImplementation impl,
        JSLImmutableMemory hash_map_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory key_type_name,
        bool key_is_str,
        JSLImmutableMemory value_type_name,
        bool value_is_str,
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
        JSLAllocatorInterface allocator,
        JSLOutputSink sink,
        HashMapImplementation impl,
        JSLImmutableMemory hash_map_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory key_type_name,
        bool key_is_str,
        JSLImmutableMemory value_type_name,
        bool value_is_str,
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

    static JSLImmutableMemory fixed_header_template = JSL_CSTR_INITIALIZER(
        "/**\n"
        " * AUTO GENERATED FILE\n"
        " *\n"
        " * This file contains the header for a hash map `{{ hash_map_name }}` which maps\n"
        " * `{{ key_type_name }}` keys to `{{ value_type_name }}` values.\n"
        " *\n"
        " * This hash map is designed for situations where you can set an upper bound on the\n"
        " * number of items you will have and that upper bound is still a reasonable amount of\n"
        " * memory. This represents the vast majority case, as most hash maps will never have more\n"
        " * than 100 items. Even in cases where the struct is quite large e.g. over a kilobyte, and\n"
        " * you have a large upper bound, say 100k, thats still ~100MB of data. This is an incredibly\n"
        " * rare case and you probably only have one of these in your program; this hash map would\n"
        " * still work for that case.\n"
        " *\n"
        " * This hash map is not suited for cases where the hash map will shrink and grow quite\n"
        " * substantially or there's no known upper bound. The most common example would be user\n"
        " * input that cannot reasonably be limited, e.g. a word processing application cannot simply\n"
        " * refuse to open very large (+10gig) documents. If you have some hash map which is built\n"
        " * from the document file then you need some other allocation strategy (you probably don't\n"
        " * want a normal hash map either as you'd be streaming things in and out of memory).\n"
        " *\n"
        " * This file was auto generated from the hash map generation utility that's part of\n"
        " * the \"Jack's Standard Library\" project. The utility generates a header file and a\n"
        " * C file for a type safe, open addressed, hash map. By generating the code rather\n"
        " * than using macros, two benefits are gained. One, the code is much easier to debug.\n"
        " * Two, it's much more obvious how much code you're generating, which means you are\n"
        " * much less likely to accidentally create the combinatoric explosion of code that's\n"
        " * so common in C++ projects. Adding friction to things is actually good sometimes.\n"
        " *\n"
        " * ## LICENSE\n"
        " *\n"
        " * Copyright (c) 2026 Jack Stouffer\n"
        " *\n"
        " * Permission is hereby granted, free of charge, to any person obtaining a\n"
        " * copy of this software and associated documentation files (the \"Software\"),\n"
        " * to deal in the Software without restriction, including without limitation\n"
        " * the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
        " * and/or sell copies of the Software, and to permit persons to whom the Software\n"
        " * is furnished to do so, subject to the following conditions:\n"
        " *\n"
        " * The above copyright notice and this permission notice shall be included in all\n"
        " * copies or substantial portions of the Software.\n"
        " *\n"
        " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
        " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
        " * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
        " * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
        " * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
        " * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
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
        "    JSLAllocatorInterface allocator;\n"
        "\n"
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory* keys_array;\n"
        "    JSLStringLifeTime* key_lifetime_array;\n"
        "    {% else %}\n"
        "    {{ key_type_name }}* keys_array;\n"
        "    {% endif %}\n"
        "\n"
        "    {% if value_is_str %}\n"
        "    JSLImmutableMemory* values_array;\n"
        "    JSLStringLifeTime* value_lifetime_array;\n"
        "    {% else %}\n"
        "    {{ value_type_name }}* values_array;\n"
        "    {% endif %}\n"
        "\n"
        "    uint64_t* hashes_array;\n"
        "    int64_t arrays_length;\n"
        "\n"
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
        " * @param allocator The allocator that this hash map will use\n"
        " * @param max_item_count The maximum amount of items this hash map can hold\n"
        " * @param seed Seed value for the hash function to protect against hash flooding attacks\n"
        " */\n"
        "bool {{ function_prefix }}_init(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    JSLAllocatorInterface allocator,\n"
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
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory key,\n"
        "    JSLStringLifeTime key_lifetime,\n"
        "    {% else %}\n"
        "    {{ key_type_name }} key,\n"
        "    {% endif %}\n"
        "    {% if value_is_str %}\n"
        "    JSLImmutableMemory value,\n"
        "    JSLStringLifeTime value_lifetime\n"
        "    {% else %}\n"
        "    {{ value_type_name }} value\n"
        "    {% endif %}\n"
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
        "{% if value_is_str %}\n"
        "JSLImmutableMemory {{ function_prefix }}_get(\n"
        "{% else %}\n"
        "{{ value_type_name }}* {{ function_prefix }}_get(\n"
        "{% endif %}\n"
        "\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory key\n"
        "    {% else %}\n"
        "    {{ key_type_name }} key\n"
        "    {% endif %}\n"
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
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory key\n"
        "    {% else %}\n"
        "    {{ key_type_name }} key\n"
        "    {% endif %}\n"
        ");\n"
        "\n"
        "/**\n"
        " * Free all the underlying memory that was allocated by this hash map on the given\n"
        " * allocator.\n"
        " */\n"
        "void {{ function_prefix }}_free(\n"
        "    {{ hash_map_name }}* hash_map\n"
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
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory* out_key,\n"
        "    {% else %}\n"
        "    {{ key_type_name }}* out_key,\n"
        "    {% endif %}\n"
        "    {% if value_is_str %}\n"
        "    JSLImmutableMemory* out_value\n"
        "    {% else %}\n"
        "    {{ value_type_name }}* out_value\n"
        "    {% endif %}\n"
        ");\n"
        "\n"
    );

    static JSLImmutableMemory fixed_source_template = JSL_CSTR_INITIALIZER(
        "/**\n"
        " * AUTO GENERATED FILE\n"
        " *\n"
        " * See the header for more information.\n"
        " *\n"
        " * ## LICENSE\n"
        " *\n"
        " * Copyright (c) 2026 Jack Stouffer\n"
        " *\n"
        " * Permission is hereby granted, free of charge, to any person obtaining a\n"
        " * copy of this software and associated documentation files (the \"Software\"),\n"
        " * to deal in the Software without restriction, including without limitation\n"
        " * the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
        " * and/or sell copies of the Software, and to permit persons to whom the Software\n"
        " * is furnished to do so, subject to the following conditions:\n"
        " *\n"
        " * The above copyright notice and this permission notice shall be included in all\n"
        " * copies or substantial portions of the Software.\n"
        " *\n"
        " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
        " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
        " * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
        " * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
        " * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
        " * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
        " */\n"
        "\n"
        "bool {{ function_prefix }}_init(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    JSLAllocatorInterface allocator,\n"
        "    int64_t max_item_count,\n"
        "    uint64_t seed\n"
        ")\n"
        "{\n"
        "    if (hash_map == NULL || max_item_count < 0)\n"
        "        return false;\n"
        "\n"
        "    JSL_MEMSET(hash_map, 0, sizeof({{ hash_map_name }}));\n"
        "\n"
        "    hash_map->seed = seed;\n"
        "    hash_map->allocator = allocator;\n"
        "    hash_map->max_item_count = max_item_count;\n"
        "\n"
        "    int64_t max_with_load_factor = (int64_t) ((float) max_item_count / 0.75f);\n"
        "\n"
        "    hash_map->arrays_length = jsl_next_power_of_two_i64(max_with_load_factor);\n"
        "    hash_map->arrays_length = JSL_MAX(hash_map->arrays_length, 32);\n"
        "\n"
        "    {% if key_is_str %}\n"
        "    hash_map->keys_array = (JSLImmutableMemory*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof(JSLImmutableMemory)) * hash_map->arrays_length,\n"
        "        JSL_DEFAULT_ALLOCATION_ALIGNMENT,\n"
        "        false\n"
        "    );\n"
        "    if (hash_map->keys_array == NULL)\n"
        "        return false;\n"
        "    hash_map->key_lifetime_array = (JSLStringLifeTime*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof(JSLStringLifeTime)) * hash_map->arrays_length,\n"
        "        JSL_DEFAULT_ALLOCATION_ALIGNMENT,\n"
        "        false\n"
        "    );\n"
        "    if (hash_map->key_lifetime_array == NULL)\n"
        "        return false;\n"
        "    {% else %}\n"
        "    hash_map->keys_array = ({{ key_type_name }}*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof({{ key_type_name }})) * hash_map->arrays_length,\n"
        "        (int32_t) _Alignof({{ key_type_name }}),\n"
        "        false\n"
        "    );\n"
        "    if (hash_map->keys_array == NULL)\n"
        "        return false;\n"
        "    {% endif %}\n"
        "\n"
        "\n"
        "    {% if value_is_str %}\n"
        "    hash_map->values_array = (JSLImmutableMemory*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof(JSLImmutableMemory)) * hash_map->arrays_length,\n"
        "        JSL_DEFAULT_ALLOCATION_ALIGNMENT,\n"
        "        false\n"
        "    );\n"
        "    if (hash_map->values_array == NULL)\n"
        "        return false;\n"
        "    hash_map->value_lifetime_array = (JSLStringLifeTime*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof(JSLStringLifeTime)) * hash_map->arrays_length,\n"
        "        JSL_DEFAULT_ALLOCATION_ALIGNMENT,\n"
        "        false\n"
        "    );\n"
        "    if (hash_map->value_lifetime_array == NULL)\n"
        "        return false;\n"
        "    {% else %}\n"
        "    hash_map->values_array = ({{ value_type_name }}*) jsl_allocator_interface_alloc(\n"
        "        allocator,\n"
        "        ((int64_t) sizeof({{ value_type_name }})) * hash_map->arrays_length,\n"
        "        (int32_t) _Alignof({{ value_type_name }}),\n"
        "        false\n"
        "    );\n"
        "    if (hash_map->values_array == NULL)\n"
        "        return false;\n"
        "    {% endif %}\n"
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
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory key,\n"
        "    {% else %}\n"
        "    {{ key_type_name }} key,\n"
        "    {% endif %}\n"
        "    int64_t* out_slot,\n"
        "    uint64_t* out_hash,\n"
        "    bool* out_found\n"
        ")\n"
        "{\n"
        "    *out_slot = -1;\n"
        "    *out_found = false;\n"
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
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory key,\n"
        "    JSLStringLifeTime key_lifetime,\n"
        "    {% else %}\n"
        "    {{ key_type_name }} key,\n"
        "    {% endif %}\n"
        "    {% if value_is_str %}\n"
        "    JSLImmutableMemory value,\n"
        "    JSLStringLifeTime value_lifetime\n"
        "    {% else %}\n"
        "    {{ value_type_name }} value\n"
        "    {% endif %}\n"
        ")\n"
        "{\n"
        "    bool insert_success = false;\n"
        "\n"
        "    if (\n"
        "        hash_map == NULL\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\n"
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
        "        {% if key_is_str %}\n"
        "        if (key_lifetime == JSL_STRING_LIFETIME_SHORTER)\n"
        "            hash_map->keys_array[slot] = jsl_duplicate(hash_map->allocator, key);\n"
        "        else\n"
        "            hash_map->keys_array[slot] = key;\n"
        "\n"
        "        hash_map->key_lifetime_array[slot] = key_lifetime;\n"
        "        {% else %}\n"
        "        hash_map->keys_array[slot] = key;\n"
        "        {% endif %}\n"
        "\n"
        "        {% if value_is_str %}\n"
        "        if (value_lifetime == JSL_STRING_LIFETIME_SHORTER)\n"
        "            hash_map->values_array[slot] = jsl_duplicate(hash_map->allocator, value);\n"
        "        else\n"
        "            hash_map->values_array[slot] = value;\n"
        "\n"
        "        hash_map->value_lifetime_array[slot] = value_lifetime;\n"
        "        {% else %}\n"
        "        hash_map->values_array[slot] = value;\n"
        "        {% endif %}\n"
        "\n"
        "        hash_map->hashes_array[slot] = hash;\n"
        "        ++hash_map->item_count;\n"
        "        insert_success = true;\n"
        "    }\n"
        "    // update\n"
        "    else if (slot > -1 && existing_found)\n"
        "    {\n"
        "        {% if value_is_str %}\n"
        "        if (hash_map->value_lifetime_array[slot] == JSL_STRING_LIFETIME_SHORTER)\n"
        "            jsl_allocator_interface_free(hash_map->allocator, hash_map->values_array[slot].data);\n"
        "\n"
        "        if (value_lifetime == JSL_STRING_LIFETIME_SHORTER)\n"
        "            hash_map->values_array[slot] = jsl_duplicate(hash_map->allocator, value);\n"
        "        else\n"
        "            hash_map->values_array[slot] = value;\n"
        "\n"
        "        hash_map->value_lifetime_array[slot] = value_lifetime;\n"
        "        {% else %}\n"
        "        hash_map->values_array[slot] = value;\n"
        "        {% endif %}\n"
        "\n"
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
        "{% if value_is_str %}\n"
        "JSLImmutableMemory {{ function_prefix }}_get(\n"
        "{% else %}\n"
        "{{ value_type_name }}* {{ function_prefix }}_get(\n"
        "{% endif %}\n"
        "\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory key\n"
        "    {% else %}\n"
        "    {{ key_type_name }} key\n"
        "    {% endif %}\n"
        ")\n"
        "{\n"
        "    {% if value_is_str %}\n"
        "    JSLImmutableMemory res = {0};\n"
        "    {% else %}\n"
        "    {{ value_type_name }}* res = NULL;\n"
        "    {% endif %}\n"
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
        "\n"
        "    {{ function_prefix }}_probe(hash_map, key, &slot, &hash, &existing_found);\n"
        "    \n"
        "    if (slot > -1 && existing_found)\n"
        "    {\n"
        "        {% if value_is_str %}\n"
        "        res = hash_map->values_array[slot];\n"
        "        {% else %}\n"
        "        res = &hash_map->values_array[slot];\n"
        "        {% endif %}\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_delete(\n"
        "    {{ hash_map_name }}* hash_map,\n"
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory key\n"
        "    {% else %}\n"
        "    {{ key_type_name }} key\n"
        "    {% endif %}\n"
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
        "void {{ function_prefix }}_free(\n"
        "    {{ hash_map_name }}* hash_map\n"
        ")\n"
        "{\n"
        "    if (\n"
        "        hash_map == NULL\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\n"
        "    )\n"
        "        return;\n"
        "\n"
        "    {% if key_is_str or value_is_str %}\n"
        "    for (int64_t current_slot = 0; current_slot < hash_map->arrays_length; ++current_slot)\n"
        "    {\n"
        "        uint64_t hash_value = hash_map->hashes_array[current_slot];\n"
        "        {% if key_is_str %}\n"
        "        JSLStringLifeTime lifetime = hash_map->key_lifetime_array[current_slot];\n"
        "        if (hash_value != JSL__HASHMAP_EMPTY && lifetime == JSL_STRING_LIFETIME_SHORTER)\n"
        "        {\n"
        "            jsl_allocator_interface_free(hash_map->allocator, hash_map->keys_array[current_slot].data);\n"
        "        }\n"
        "        {% elif value_is_str %}\n"
        "        JSLStringLifeTime lifetime = hash_map->value_lifetime_array[current_slot];\n"
        "        if (hash_value != JSL__HASHMAP_EMPTY && lifetime == JSL_STRING_LIFETIME_SHORTER)\n"
        "        {\n"
        "            jsl_allocator_interface_free(hash_map->allocator, hash_map->values_array[current_slot].data);\n"
        "        }\n"
        "        {% endif %}\n"
        "    }\n"
        "\n"
        "    {% if key_is_str %}\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->key_lifetime_array);\n"
        "    {% elif value_is_str %}\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->value_lifetime_array);\n"
        "    {% endif %}\n"
        "\n"
        "    {% endif %}\n"
        "\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->keys_array);\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->values_array);\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->hashes_array);\n"
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
        "    {% if key_is_str %}\n"
        "    JSLImmutableMemory* out_key,\n"
        "    {% else %}\n"
        "    {{ key_type_name }}* out_key,\n"
        "    {% endif %}\n"
        "    {% if value_is_str %}\n"
        "    JSLImmutableMemory* out_value\n"
        "    {% else %}\n"
        "    {{ value_type_name }}* out_value\n"
        "    {% endif %}\n"
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

    static JSLImmutableMemory dynamic_header_template = JSL_CSTR_INITIALIZER(
        "#pragma once\n"
        "\n"
        "#include <stdint.h>\n"
        "#include <stddef.h>\n"
        "#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L\n"
        "    #include <stdbool.h>\n"
        "#endif\n"
        "\n"
        "#include \"jsl/core.h\"\n"
        "#include \"jsl/hash_map_common.h\"\n"
        "\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n"
        "\n"
        "struct {{ hash_map_name }}Entry {\n"
        "    uint64_t hash;\n"
        "    {{ key_type_name }} key;\n"
        "    {{ value_type_name }} value;\n"
        "};\n"
        "\n"
        "/**\n"
        " * State tracking struct for iterating over all of the keys and values\n"
        " * in the map.\n"
        " * \n"
        " * @note If you mutate the map this iterator is automatically invalidated\n"
        " * and any operations on this iterator will terminate with failure return\n"
        " * values.\n"
        " * \n"
        " * ## Functions\n"
        " *\n"
        " *  * jsl_str_to_str_map_key_value_iterator_init\n"
        " *  * jsl_str_to_str_map_key_value_iterator_next\n"
        " */\n"
        "typedef struct {{ hash_map_name }}KeyValueIter {\n"
        "    struct JSL__StrToStrMap* map;\n"
        "    int64_t current_lut_index;\n"
        "    int64_t generational_id;\n"
        "    uint64_t sentinel;\n"
        "} {{ hash_map_name }}KeyValueIter;\n"
        "\n"
        "/**\n"
        " * This is an open addressed hash map with linear probing that maps\n"
        " * {{ key_type_name }} keys to {{ value_type_name }} values. This map uses\n"
        " * rapidhash, which is a avalanche hash with a configurable seed\n"
        " * value for protection against hash flooding attacks.\n"
        " * \n"
        " * Example:\n"
        " *\n"
        " * ```\n"
        " * uint8_t buffer[JSL_KILOBYTES(16)];\n"
        " * JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);\n"
        " *\n"
        " * {{ hash_map_name }} map;\n"
        " * jsl_str_to_str_map_init(&map, &stack_arena, 0);\n"
        " *\n"
        " * JSLFatPtr key = JSL_FATPTR_INITIALIZER(\"hello-key\");\n"
        " * \n"
        " * jsl_str_to_str_multimap_insert(\n"
        " *     &map,\n"
        " *     key,\n"
        " *     JSL_STRING_LIFETIME_LONGER,\n"
        " *     JSL_FATPTR_EXPRESSION(\"hello-value\"),\n"
        " *     JSL_STRING_LIFETIME_LONGER\n"
        " * );\n"
        " * \n"
        " * {{ value_type_name }} value;\n"
        " * jsl_str_to_str_map_get(&map, key, &value);\n"
        " * ```\n"
        " * \n"
        " * ## Functions\n"
        " *\n"
        " *  * {{ function_prefix }}_init\n"
        " *  * {{ function_prefix }}_init2\n"
        " *  * {{ function_prefix }}_item_count\n"
        " *  * {{ function_prefix }}_has_key\n"
        " *  * {{ function_prefix }}_insert\n"
        " *  * {{ function_prefix }}_get\n"
        " *  * {{ function_prefix }}_key_value_iterator_init\n"
        " *  * {{ function_prefix }}_key_value_iterator_next\n"
        " *  * {{ function_prefix }}_delete\n"
        " *  * {{ function_prefix }}_clear\n"
        " *\n"
        " */\n"
        "typedef struct {{ hash_map_name }} {\n"
        "    // putting the sentinel first means it's much more likely to get\n"
        "    // corrupted from accidental overwrites, therefore making it\n"
        "    // more likely that memory bugs are caught.\n"
        "    uint64_t sentinel;\n"
        "\n"
        "    JSLArena* arena;\n"
        "\n"
        "    {{ hash_map_name }}Entry* entry_array;\n"
        "    int64_t entry_array_length;\n"
        "\n"
        "    int64_t item_count;\n"
        "    int64_t tombstone_count;\n"
        "\n"
        "    uint64_t seed;\n"
        "    float load_factor;\n"
        "    int32_t generational_id;\n"
        "} {{ hash_map_name }};\n"
        "\n"
        "/**\n"
        " * Initialize a map with default sizing parameters.\n"
        " *\n"
        " * This sets up internal tables in the provided arena, using a 32 entry\n"
        " * initial capacity guess and a 0.75 load factor. The `seed` value is to\n"
        " * protect against hash flooding attacks. If you're absolutely sure this\n"
        " * map cannot be attacked, then zero is valid seed value.\n"
        " *\n"
        " * @param map Pointer to the map to initialize.\n"
        " * @param arena Arena used for all allocations.\n"
        " * @param seed Arbitrary seed value for hashing.\n"
        " * @return `true` on success, `false` if any parameter is invalid or out of memory.\n"
        " */\n"
        "bool {{ function_prefix }}_init(\n"
        "    {{ hash_map_name }}* map,\n"
        "    JSLArena* arena,\n"
        "    uint64_t seed\n"
        ");\n"
        "\n"
        "/**\n"
        " * Initialize a map with explicit sizing parameters.\n"
        " *\n"
        " * This is identical to `jsl_str_to_str_map_init`, but lets callers\n"
        " * provide an initial `item_count_guess` and a `load_factor`. The initial\n"
        " * lookup table is sized to the next power of two above `item_count_guess`,\n"
        " * clamped to at least 32 entries. `load_factor` must be in the range\n"
        " * `(0.0f, 1.0f)` and controls when the table rehashes. The `seed` value\n"
        " * is to protect against hash flooding attacks. If you're absolutely sure \n"
        " * this map cannot be attacked, then zero is valid seed value\n"
        " *\n"
        " * @param map Pointer to the map to initialize.\n"
        " * @param arena Arena used for all allocations.\n"
        " * @param seed Arbitrary seed value for hashing.\n"
        " * @param item_count_guess Expected max number of keys\n"
        " * @param load_factor Desired load factor before rehashing\n"
        " * @return `true` on success, `false` if any parameter is invalid or out of memory.\n"
        " */\n"
        "bool {{ function_prefix }}_init2(\n"
        "    {{ hash_map_name }}* map,\n"
        "    JSLArena* arena,\n"
        "    uint64_t seed,\n"
        "    int64_t item_count_guess,\n"
        "    float load_factor\n"
        ");\n"
        "\n"
        "/**\n"
        " * Does the map have the given key.\n"
        " *\n"
        " * @param map Pointer to the map.\n"
        " * @return `true` if yes, `false` if no or error\n"
        " */\n"
        "bool {{ function_prefix }}_has_key(\n"
        "    {{ hash_map_name }}* map,\n"
        "    {{ key_type_name }} key\n"
        ");\n"
        "\n"
        "/**\n"
        " * Insert a key/value pair.\n"
        " *\n"
        " * @param map Map to mutate.\n"
        " * @param key Key to insert.\n"
        " * @param key_lifetime Lifetime semantics for the key data.\n"
        " * @param value Value to insert.\n"
        " * @param value_lifetime Lifetime semantics for the value data.\n"
        " * @return `true` on success, `false` on invalid parameters or OOM.\n"
        " */\n"
        "bool {{ function_prefix }}_insert(\n"
        "    {{ hash_map_name }}* map,\n"
        "    {{ key_type_name }} key,\n"
        "    {{ value_type_name }} value\n"
        ");\n"
        "\n"
        "/**\n"
        " * Get the value of the key.\n"
        " *\n"
        " * @param map Map to search.\n"
        " * @param key Key to search for.\n"
        " * @param out_value Output parameter that will be filled with the value if successful\n"
        " * @returns A bool indicating success or failure\n"
        " */\n"
        "bool {{ function_prefix }}_get(\n"
        "    {{ hash_map_name }}* map,\n"
        "    {{ key_type_name }} key,\n"
        "    {{ value_type_name }}* out_value\n"
        ");\n"
        "\n"
        "/**\n"
        " * Initialize an iterator that visits every key/value pair in the map.\n"
        " * \n"
        " * Example:\n"
        " *\n"
        " * ```\n"
        " * {{ hash_map_name }}KeyValueIter iter;\n"
        " * {{ function_prefix }}_key_value_iterator_init(\n"
        " *     &map, &iter\n"
        " * );\n"
        " * \n"
        " * {{ key_type_name }} key;\n"
        " * {{ value_type_name }} value;\n"
        " * while ({{ function_prefix }}_key_value_iterator_next(&iter, &key, &value))\n"
        " * {\n"
        " *    ...\n"
        " * }\n"
        " * ```\n"
        " *\n"
        " * Overall traversal order is undefined. The iterator is invalidated\n"
        " * if the map is mutated after initialization.\n"
        " *\n"
        " * @param map Map to iterate over; must be initialized.\n"
        " * @param iterator Iterator instance to initialize.\n"
        " * @return `true` on success, `false` if parameters are invalid.\n"
        " */\n"
        "bool {{ function_prefix }}_key_value_iterator_init(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLStrToStrMapKeyValueIter* iterator\n"
        ");\n"
        "\n"
        "/**\n"
        " * Advance the key/value iterator.\n"
        " * \n"
        " * Example:\n"
        " *\n"
        " * ```\n"
        " * {{ hash_map_name }}KeyValueIter iter;\n"
        " * {{ function_prefix }}_key_value_iterator_init(\n"
        " *     &map, &iter\n"
        " * );\n"
        " * \n"
        " * {{ key_type_name }} key;\n"
        " * {{ value_type_name }} value;\n"
        " * while ({{ function_prefix }}_key_value_iterator_next(&iter, &key, &value))\n"
        " * {\n"
        " *    ...\n"
        " * }\n"
        " * ```\n"
        " *\n"
        " * Returns the next key/value pair for the map. The iterator must be\n"
        " * initialized and is invalidated if the map is mutated; iteration order\n"
        " * is undefined.\n"
        " *\n"
        " * @param iterator Iterator to advance.\n"
        " * @param out_key Output for the current key.\n"
        " * @param out_value Output for the current value.\n"
        " * @return `true` if a pair was produced, `false` if exhausted or invalid.\n"
        " */\n"
        "bool {{ function_prefix }}_key_value_iterator_next(\n"
        "    {{ hash_map_name }}KeyValueIter* iterator,\n"
        "    {{ key_type_name }}* out_key,\n"
        "    {{ value_type_name }}* out_value\n"
        ");\n"
        "\n"
        "/**\n"
        " * Remove a key/value.\n"
        " *\n"
        " * Iterators become invalid. If the key is not present or parameters are invalid,\n"
        " * the map is unchanged and `false` is returned.\n"
        " *\n"
        " * @param map Map to mutate.\n"
        " * @param key Key to remove.\n"
        " * @return `true` if the key existed and was removed, `false` otherwise.\n"
        " */\n"
        "bool {{ function_prefix }}_delete(\n"
        "    {{ hash_map_name }}* map,\n"
        "    {{ key_type_name }} key\n"
        ");\n"
        "\n"
        "/**\n"
        " * Remove all keys and values from the map.  Iterators become invalid.\n"
        " *\n"
        " * @param map Map to clear.\n"
        " */\n"
        "void {{ function_prefix }}_clear(\n"
        "    {{ hash_map_name }}* map\n"
        ");\n"
        "\n"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n"
    );
    static JSLImmutableMemory dynamic_source_template = JSL_CSTR_INITIALIZER(
        "/**\n"
        " * # JSL String to String Map\n"
        " * \n"
        " * This file is a single header file library that implements a hash map data\n"
        " * structure, which maps length based string keys to length based string values,\n"
        " * and is optimized around the arena allocator design. This file is part of\n"
        " * the Jack's Standard Library project.\n"
        " * \n"
        " * ## Documentation\n"
        " * \n"
        " * See `docs/jsl_str_to_str_map.md` for a formatted documentation page.\n"
        " *\n"
        " * ## Caveats\n"
        " * \n"
        " * This map uses arenas, so some wasted memory is indeveatble. Care has\n"
        " * been taken to reuse as much allocated memory as possible. But if your\n"
        " * map is long lived it's possible to start exhausting the arena with\n"
        " * old memory.\n"
        " * \n"
        " * Remember to\n"
        " * \n"
        " * * have an initial item count guess as accurate as you can to reduce rehashes\n"
        " * * have the arena have as short a lifetime as possible\n"
        " * \n"
        " * ## License\n"
        " *\n"
        " * Copyright (c) 2025 Jack Stouffer\n"
        " *\n"
        " * Permission is hereby granted, free of charge, to any person obtaining a\n"
        " * copy of this software and associated documentation files (the “Software”),\n"
        " * to deal in the Software without restriction, including without limitation\n"
        " * the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
        " * and/or sell copies of the Software, and to permit persons to whom the Software\n"
        " * is furnished to do so, subject to the following conditions:\n"
        " *\n"
        " * The above copyright notice and this permission notice shall be included in all\n"
        " * copies or substantial portions of the Software.\n"
        " *\n"
        " * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
        " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
        " * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
        " * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
        " * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\n"
        " * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
        " */\n"
        "\n"
        "#include <stdint.h>\n"
        "#include <stddef.h>\n"
        "#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L\n"
        "    #include <stdbool.h>\n"
        "#endif\n"
        "\n"
        "#include \"jsl/core.h\"\n"
        "#include \"jsl/hash_map_common.h\"\n"
        "#include \"jsl/str_to_str_map.h\"\n"
        "\n"
        "#define JSL__MAP_PRIVATE_SENTINEL 8973815015742603881U\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_init(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLArena* arena,\n"
        "    uint64_t seed\n"
        ")\n"
        "{\n"
        "    return jsl_str_to_str_map_init2(\n"
        "        map,\n"
        "        arena,\n"
        "        seed,\n"
        "        32,\n"
        "        0.75f\n"
        "    );\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_init2(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLArena* arena,\n"
        "    uint64_t seed,\n"
        "    int64_t item_count_guess,\n"
        "    float load_factor\n"
        ")\n"
        "{\n"
        "    bool res = true;\n"
        "\n"
        "    if (\n"
        "        map == NULL\n"
        "        || arena == NULL\n"
        "        || item_count_guess <= 0\n"
        "        || load_factor <= 0.0f\n"
        "        || load_factor >= 1.0f\n"
        "    )\n"
        "        res = false;\n"
        "\n"
        "    if (res)\n"
        "    {\n"
        "        JSL_MEMSET(map, 0, sizeof(JSLStrToStrMap));\n"
        "        map->arena = arena;\n"
        "        map->load_factor = load_factor;\n"
        "        map->hash_seed = seed;\n"
        "\n"
        "        item_count_guess = JSL_MAX(32L, item_count_guess);\n"
        "        int64_t items = jsl_next_power_of_two_i64(item_count_guess + 1);\n"
        "\n"
        "        map->entry_lookup_table = (uintptr_t*) jsl_arena_allocate_aligned(\n"
        "            arena,\n"
        "            (int64_t) sizeof(uintptr_t) * items,\n"
        "            _Alignof(uintptr_t),\n"
        "            true\n"
        "        ).data;\n"
        "        \n"
        "        map->entry_lookup_table_length = items;\n"
        "\n"
        "        map->sentinel = JSL__MAP_PRIVATE_SENTINEL;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "static bool jsl__str_to_str_map_rehash(\n"
        "    JSLStrToStrMap* map\n"
        ")\n"
        "{\n"
        "    bool res = false;\n"
        "\n"
        "    bool params_valid = (\n"
        "        map != NULL\n"
        "        && map->arena != NULL\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && map->entry_lookup_table != NULL\n"
        "        && map->entry_lookup_table_length > 0\n"
        "    );\n"
        "\n"
        "    uintptr_t* old_table = params_valid ? map->entry_lookup_table : NULL;\n"
        "    int64_t old_length = params_valid ? map->entry_lookup_table_length : 0;\n"
        "\n"
        "    int64_t new_length = params_valid ? jsl_next_power_of_two_i64(old_length + 1) : 0;\n"
        "    bool length_valid = params_valid && new_length > old_length && new_length > 0;\n"
        "\n"
        "    bool bytes_possible = length_valid\n"
        "        && new_length <= (INT64_MAX / (int64_t) sizeof(uintptr_t));\n"
        "\n"
        "    int64_t bytes_needed = bytes_possible\n"
        "        ? (int64_t) sizeof(uintptr_t) * new_length\n"
        "        : 0;\n"
        "\n"
        "    JSLFatPtr new_table_mem = {0};\n"
        "    if (bytes_possible)\n"
        "    {\n"
        "        new_table_mem = jsl_arena_allocate_aligned(\n"
        "            map->arena,\n"
        "            bytes_needed,\n"
        "            _Alignof(uintptr_t),\n"
        "            true\n"
        "        );\n"
        "    }\n"
        "\n"
        "    uintptr_t* new_table = (bytes_possible && new_table_mem.data != NULL)\n"
        "        ? (uintptr_t*) new_table_mem.data\n"
        "        : NULL;\n"
        "\n"
        "    uint64_t lut_mask = new_length > 0 ? ((uint64_t) new_length - 1u) : 0;\n"
        "    int64_t old_index = 0;\n"
        "    bool migrate_ok = new_table != NULL;\n"
        "\n"
        "    while (migrate_ok && old_index < old_length)\n"
        "    {\n"
        "        uintptr_t lut_res = old_table[old_index];\n"
        "\n"
        "        bool occupied = (\n"
        "            lut_res != 0\n"
        "            && lut_res != JSL__MAP_EMPTY\n"
        "            && lut_res != JSL__MAP_TOMBSTONE\n"
        "        );\n"
        "\n"
        "        struct JSL__StrToStrMapEntry* entry = occupied\n"
        "            ? (struct JSL__StrToStrMapEntry*) lut_res\n"
        "            : NULL;\n"
        "\n"
        "        int64_t probe_index = entry != NULL\n"
        "            ? (int64_t) (entry->hash & lut_mask)\n"
        "            : 0;\n"
        "\n"
        "        int64_t probes = 0;\n"
        "\n"
        "        bool insert_needed = entry != NULL;\n"
        "        while (migrate_ok && insert_needed && probes < new_length)\n"
        "        {\n"
        "            uintptr_t probe_res = new_table[probe_index];\n"
        "            bool slot_free = (\n"
        "                probe_res == JSL__MAP_EMPTY\n"
        "                || probe_res == JSL__MAP_TOMBSTONE\n"
        "            );\n"
        "\n"
        "            if (slot_free)\n"
        "            {\n"
        "                new_table[probe_index] = (uintptr_t) entry;\n"
        "                insert_needed = false;\n"
        "                break;\n"
        "            }\n"
        "\n"
        "            probe_index = (int64_t) (((uint64_t) probe_index + 1u) & lut_mask);\n"
        "            ++probes;\n"
        "        }\n"
        "\n"
        "        bool placement_failed = insert_needed;\n"
        "        if (placement_failed)\n"
        "        {\n"
        "            migrate_ok = false;\n"
        "        }\n"
        "\n"
        "        ++old_index;\n"
        "    }\n"
        "\n"
        "    bool should_commit = migrate_ok && new_table != NULL && length_valid;\n"
        "    if (should_commit)\n"
        "    {\n"
        "        map->entry_lookup_table = new_table;\n"
        "        map->entry_lookup_table_length = new_length;\n"
        "        map->tombstone_count = 0;\n"
        "        ++map->generational_id;\n"
        "        res = true;\n"
        "    }\n"
        "\n"
        "    bool failed = !should_commit;\n"
        "    if (failed)\n"
        "    {\n"
        "        res = false;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "static JSL__FORCE_INLINE void jsl__str_to_str_map_update_value(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLFatPtr value,\n"
        "    JSLStringLifeTime value_lifetime,\n"
        "    int64_t lut_index\n"
        ")\n"
        "{\n"
        "    uintptr_t lut_res = map->entry_lookup_table[lut_index];\n"
        "    struct JSL__StrToStrMapEntry* entry = (struct JSL__StrToStrMapEntry*) lut_res;\n"
        "\n"
        "    if (value_lifetime == JSL_STRING_LIFETIME_LONGER)\n"
        "    {\n"
        "        entry->value = value;\n"
        "    }\n"
        "    else if (\n"
        "        value_lifetime == JSL_STRING_LIFETIME_SHORTER\n"
        "        && value.length <= JSL__MAP_SSO_LENGTH\n"
        "    )\n"
        "    {\n"
        "        JSL_MEMCPY(entry->value_sso_buffer, value.data, (size_t) value.length);\n"
        "        entry->value.data = entry->value_sso_buffer;\n"
        "        entry->value.length = value.length;\n"
        "    }\n"
        "    else if (\n"
        "        value_lifetime == JSL_STRING_LIFETIME_SHORTER\n"
        "        && value.length > JSL__MAP_SSO_LENGTH\n"
        "    )\n"
        "    {\n"
        "        entry->value = jsl_fatptr_duplicate(map->arena, value);\n"
        "    }\n"
        "}\n"
        "\n"
        "static JSL__FORCE_INLINE bool jsl__str_to_str_map_add(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLFatPtr key,\n"
        "    JSLStringLifeTime key_lifetime,\n"
        "    JSLFatPtr value,\n"
        "    JSLStringLifeTime value_lifetime,\n"
        "    int64_t lut_index,\n"
        "    uint64_t hash\n"
        ")\n"
        "{\n"
        "    struct JSL__StrToStrMapEntry* entry = NULL;\n"
        "    bool replacing_tombstone = map->entry_lookup_table[lut_index] == JSL__MAP_TOMBSTONE;\n"
        "\n"
        "    if (map->entry_free_list == NULL)\n"
        "    {\n"
        "        entry = JSL_ARENA_TYPED_ALLOCATE(struct JSL__StrToStrMapEntry, map->arena);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        struct JSL__StrToStrMapEntry* next = map->entry_free_list->next;\n"
        "        entry = map->entry_free_list;\n"
        "        map->entry_free_list = next;\n"
        "    }\n"
        "\n"
        "    if (entry != NULL)\n"
        "    {\n"
        "        entry->hash = hash;\n"
        "        \n"
        "        map->entry_lookup_table[lut_index] = (uintptr_t) entry;\n"
        "        ++map->item_count;\n"
        "    }\n"
        "\n"
        "    if (entry != NULL && replacing_tombstone)\n"
        "    {\n"
        "        --map->tombstone_count;\n"
        "    }\n"
        "\n"
        "    // \n"
        "    // Copy the key\n"
        "    // \n"
        "\n"
        "    if (entry != NULL && key_lifetime == JSL_STRING_LIFETIME_LONGER)\n"
        "    {\n"
        "        entry->key = key;\n"
        "    }\n"
        "    else if (\n"
        "        entry != NULL\n"
        "        && key_lifetime == JSL_STRING_LIFETIME_SHORTER\n"
        "        && key.length <= JSL__MAP_SSO_LENGTH\n"
        "    )\n"
        "    {\n"
        "        JSL_MEMCPY(entry->key_sso_buffer, key.data, (size_t) key.length);\n"
        "        entry->key.data = entry->key_sso_buffer;\n"
        "        entry->key.length = key.length;\n"
        "    }\n"
        "    else if (\n"
        "        entry != NULL\n"
        "        && key_lifetime == JSL_STRING_LIFETIME_SHORTER\n"
        "        && key.length > JSL__MAP_SSO_LENGTH\n"
        "    )\n"
        "    {\n"
        "        entry->key = jsl_fatptr_duplicate(map->arena, key);\n"
        "    }\n"
        "\n"
        "    // \n"
        "    // Copy the value\n"
        "    // \n"
        "\n"
        "    if (entry != NULL && value_lifetime == JSL_STRING_LIFETIME_LONGER)\n"
        "    {\n"
        "        entry->value = value;\n"
        "    }\n"
        "    else if (\n"
        "        entry != NULL\n"
        "        && value_lifetime == JSL_STRING_LIFETIME_SHORTER\n"
        "        && value.length <= JSL__MAP_SSO_LENGTH\n"
        "    )\n"
        "    {\n"
        "        JSL_MEMCPY(entry->value_sso_buffer, value.data, (size_t) value.length);\n"
        "        entry->value.data = entry->value_sso_buffer;\n"
        "        entry->value.length = value.length;\n"
        "    }\n"
        "    else if (\n"
        "        entry != NULL\n"
        "        && value_lifetime == JSL_STRING_LIFETIME_SHORTER\n"
        "        && value.length > JSL__MAP_SSO_LENGTH\n"
        "    )\n"
        "    {\n"
        "        entry->value = jsl_fatptr_duplicate(map->arena, value);\n"
        "    }\n"
        "\n"
        "    return entry != NULL;\n"
        "}\n"
        "\n"
        "static inline void jsl__str_to_str_map_probe(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLFatPtr key,\n"
        "    int64_t* out_lut_index,\n"
        "    uint64_t* out_hash,\n"
        "    bool* out_found\n"
        ")\n"
        "{\n"
        "    *out_lut_index = -1;\n"
        "    *out_found = false;\n"
        "\n"
        "    int64_t first_tombstone = -1;\n"
        "    bool tombstone_seen = false;\n"
        "    bool searching = true;\n"
        "\n"
        "    *out_hash = jsl__rapidhash_withSeed(key.data, (size_t) key.length, map->hash_seed);\n"
        "\n"
        "    int64_t lut_length = map->entry_lookup_table_length;\n"
        "    uint64_t lut_mask = (uint64_t) lut_length - 1u;\n"
        "    int64_t lut_index = (int64_t) (*out_hash & lut_mask);\n"
        "    int64_t probes = 0;\n"
        "\n"
        "    while (searching && probes < lut_length)\n"
        "    {\n"
        "        uintptr_t lut_res = map->entry_lookup_table[lut_index];\n"
        "\n"
        "        bool is_empty = lut_res == JSL__MAP_EMPTY;\n"
        "        bool is_tombstone = lut_res == JSL__MAP_TOMBSTONE;\n"
        "\n"
        "        if (is_empty)\n"
        "        {\n"
        "            *out_lut_index = tombstone_seen ? first_tombstone : lut_index;\n"
        "            searching = false;\n"
        "        }\n"
        "\n"
        "        bool record_tombstone = searching && is_tombstone && !tombstone_seen;\n"
        "        if (record_tombstone)\n"
        "        {\n"
        "            first_tombstone = lut_index;\n"
        "            tombstone_seen = true;\n"
        "        }\n"
        "\n"
        "        bool slot_has_entry = searching && !is_empty && !is_tombstone;\n"
        "        struct JSL__StrToStrMapEntry* entry = slot_has_entry\n"
        "            ? (struct JSL__StrToStrMapEntry*) lut_res\n"
        "            : NULL;\n"
        "\n"
        "        bool matches = entry != NULL\n"
        "            && *out_hash == entry->hash\n"
        "            && jsl_fatptr_memory_compare(key, entry->key);\n"
        "\n"
        "        if (matches)\n"
        "        {\n"
        "            *out_found = true;\n"
        "            *out_lut_index = lut_index;\n"
        "            searching = false;\n"
        "        }\n"
        "\n"
        "        if (entry == NULL)\n"
        "        {\n"
        "            ++map->tombstone_count;\n"
        "            map->entry_lookup_table[lut_index] = JSL__MAP_TOMBSTONE;\n"
        "        }\n"
        "\n"
        "        if (entry == NULL && !tombstone_seen)\n"
        "        {\n"
        "            first_tombstone = lut_index;\n"
        "            tombstone_seen = true;\n"
        "        }\n"
        "\n"
        "        if (searching)\n"
        "        {\n"
        "            lut_index = (int64_t) (((uint64_t) lut_index + 1u) & lut_mask);\n"
        "            ++probes;\n"
        "        }\n"
        "    }\n"
        "\n"
        "    bool exhausted = searching && probes >= lut_length;\n"
        "    if (exhausted)\n"
        "    {\n"
        "        *out_lut_index = tombstone_seen ? first_tombstone : -1;\n"
        "    }\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_insert(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLFatPtr key,\n"
        "    JSLStringLifeTime key_lifetime,\n"
        "    JSLFatPtr value,\n"
        "    JSLStringLifeTime value_lifetime\n"
        ")\n"
        "{\n"
        "    bool res = (\n"
        "        map != NULL\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && key.data != NULL \n"
        "        && key.length > -1\n"
        "        && value.data != NULL\n"
        "        && value.length > -1\n"
        "    );\n"
        "\n"
        "    bool needs_rehash = false;\n"
        "    if (res)\n"
        "    {\n"
        "        float occupied_count = (float) (map->item_count + map->tombstone_count);\n"
        "        float current_load_factor =  occupied_count / (float) map->entry_lookup_table_length;\n"
        "        bool too_many_tombstones = map->tombstone_count > (map->entry_lookup_table_length / 4);\n"
        "        needs_rehash = current_load_factor >= map->load_factor || too_many_tombstones;\n"
        "    }\n"
        "\n"
        "    if (JSL__UNLIKELY(needs_rehash))\n"
        "    {\n"
        "        res = jsl__str_to_str_map_rehash(map);\n"
        "    }\n"
        "\n"
        "    uint64_t hash = 0;\n"
        "    int64_t lut_index = -1;\n"
        "    bool existing_found = false;\n"
        "    if (res)\n"
        "    {\n"
        "        jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);\n"
        "    }\n"
        "    \n"
        "    // new key\n"
        "    if (lut_index > -1 && !existing_found)\n"
        "    {\n"
        "        res = jsl__str_to_str_map_add(\n"
        "            map,\n"
        "            key, key_lifetime,\n"
        "            value, value_lifetime,\n"
        "            lut_index,\n"
        "            hash\n"
        "        );\n"
        "    }\n"
        "    // update\n"
        "    else if (lut_index > -1 && existing_found)\n"
        "    {\n"
        "        jsl__str_to_str_map_update_value(map, value, value_lifetime, lut_index);\n"
        "    }\n"
        "\n"
        "    if (res)\n"
        "    {\n"
        "        ++map->generational_id;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_has_key(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLFatPtr key\n"
        ")\n"
        "{\n"
        "    uint64_t hash = 0;\n"
        "    int64_t lut_index = -1;\n"
        "    bool existing_found = false;\n"
        "\n"
        "    if (\n"
        "        map != NULL\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && key.data != NULL \n"
        "        && key.length > -1\n"
        "    )\n"
        "    {\n"
        "        jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);\n"
        "    }\n"
        "\n"
        "    return lut_index > -1 && existing_found;\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_get(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLFatPtr key,\n"
        "    JSLFatPtr* out_value\n"
        ")\n"
        "{\n"
        "    bool res = false;\n"
        "\n"
        "    bool params_valid = (\n"
        "        map != NULL\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && map->entry_lookup_table != NULL\n"
        "        && out_value != NULL\n"
        "        && key.data != NULL \n"
        "        && key.length > -1\n"
        "    );\n"
        "\n"
        "    uint64_t hash = 0;\n"
        "    int64_t lut_index = -1;\n"
        "    bool existing_found = false;\n"
        "\n"
        "    if (params_valid)\n"
        "    {\n"
        "        jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);\n"
        "    }\n"
        "\n"
        "    if (params_valid && existing_found && lut_index > -1)\n"
        "    {\n"
        "        struct JSL__StrToStrMapEntry* entry =\n"
        "            (struct JSL__StrToStrMapEntry*) map->entry_lookup_table[lut_index];\n"
        "        *out_value = entry->value;\n"
        "        res = true;\n"
        "    }\n"
        "    else if (out_value != NULL)\n"
        "    {\n"
        "        *out_value = (JSLFatPtr) {0};\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF int64_t jsl_str_to_str_map_item_count(\n"
        "    JSLStrToStrMap* map\n"
        ")\n"
        "{\n"
        "    int64_t res = -1;\n"
        "\n"
        "    if (\n"
        "        map != NULL\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "    )\n"
        "    {\n"
        "        res = map->item_count;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_key_value_iterator_init(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLStrToStrMapKeyValueIter* iterator\n"
        ")\n"
        "{\n"
        "    bool res = false;\n"
        "\n"
        "    if (\n"
        "        map != NULL\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && iterator != NULL\n"
        "    )\n"
        "    {\n"
        "        iterator->map = map;\n"
        "        iterator->current_lut_index = 0;\n"
        "        iterator->sentinel = JSL__MAP_PRIVATE_SENTINEL;\n"
        "        iterator->generational_id = map->generational_id;\n"
        "        res = true;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_key_value_iterator_next(\n"
        "    JSLStrToStrMapKeyValueIter* iterator,\n"
        "    JSLFatPtr* out_key,\n"
        "    JSLFatPtr* out_value\n"
        ")\n"
        "{\n"
        "    bool found = false;\n"
        "\n"
        "    bool params_valid = (\n"
        "        iterator != NULL\n"
        "        && out_key != NULL\n"
        "        && out_value != NULL\n"
        "        && iterator->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && iterator->map != NULL\n"
        "        && iterator->map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && iterator->map->entry_lookup_table != NULL\n"
        "        && iterator->generational_id == iterator->map->generational_id\n"
        "    );\n"
        "\n"
        "    int64_t lut_length = params_valid ? iterator->map->entry_lookup_table_length : 0;\n"
        "    int64_t lut_index = iterator->current_lut_index;\n"
        "    struct JSL__StrToStrMapEntry* found_entry = NULL;\n"
        "\n"
        "    while (params_valid && lut_index < lut_length)\n"
        "    {\n"
        "        uintptr_t lut_res = iterator->map->entry_lookup_table[lut_index];\n"
        "        bool occupied = lut_res != JSL__MAP_EMPTY && lut_res != JSL__MAP_TOMBSTONE;\n"
        "\n"
        "        if (occupied)\n"
        "        {\n"
        "            found_entry = (struct JSL__StrToStrMapEntry*) lut_res;\n"
        "            break;\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            ++lut_index;\n"
        "        }\n"
        "    }\n"
        "\n"
        "    if (found_entry != NULL)\n"
        "    {\n"
        "        iterator->current_lut_index = lut_index + 1;\n"
        "        *out_key = found_entry->key;\n"
        "        *out_value = found_entry->value;\n"
        "        found = true;\n"
        "    }\n"
        "\n"
        "    bool exhausted = params_valid && found_entry == NULL;\n"
        "    if (exhausted)\n"
        "    {\n"
        "        iterator->current_lut_index = lut_length;\n"
        "        found = false;\n"
        "    }\n"
        "\n"
        "    return found;\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_delete(\n"
        "    JSLStrToStrMap* map,\n"
        "    JSLFatPtr key\n"
        ")\n"
        "{\n"
        "    bool res = false;\n"
        "\n"
        "    bool params_valid = (\n"
        "        map != NULL\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && map->entry_lookup_table != NULL\n"
        "        && key.data != NULL\n"
        "        && key.length > -1\n"
        "    );\n"
        "\n"
        "    uint64_t hash = 0;\n"
        "    int64_t lut_index = -1;\n"
        "    bool existing_found = false;\n"
        "    if (params_valid)\n"
        "    {\n"
        "        jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);\n"
        "    }\n"
        "\n"
        "    if (existing_found && lut_index > -1)\n"
        "    {\n"
        "        struct JSL__StrToStrMapEntry* entry =\n"
        "            (struct JSL__StrToStrMapEntry*) map->entry_lookup_table[lut_index];\n"
        "\n"
        "        entry->next = map->entry_free_list;\n"
        "        map->entry_free_list = entry;\n"
        "\n"
        "        --map->item_count;\n"
        "        ++map->generational_id;\n"
        "\n"
        "        map->entry_lookup_table[lut_index] = JSL__MAP_TOMBSTONE;\n"
        "        ++map->tombstone_count;\n"
        "\n"
        "        res = true;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "JSL_STR_TO_STR_MAP_DEF void jsl_str_to_str_map_clear(\n"
        "    JSLStrToStrMap* map\n"
        ")\n"
        "{\n"
        "    bool params_valid = (\n"
        "        map != NULL\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\n"
        "        && map->entry_lookup_table != NULL\n"
        "    );\n"
        "\n"
        "    int64_t lut_length = params_valid ? map->entry_lookup_table_length : 0;\n"
        "    int64_t index = 0;\n"
        "\n"
        "    while (params_valid && index < lut_length)\n"
        "    {\n"
        "        uintptr_t lut_res = map->entry_lookup_table[index];\n"
        "\n"
        "        if (lut_res != JSL__MAP_EMPTY && lut_res != JSL__MAP_TOMBSTONE)\n"
        "        {\n"
        "            struct JSL__StrToStrMapEntry* entry = (struct JSL__StrToStrMapEntry*) lut_res;\n"
        "            entry->next = map->entry_free_list;\n"
        "            map->entry_free_list = entry;\n"
        "            map->entry_lookup_table[index] = JSL__MAP_EMPTY;\n"
        "        }\n"
        "        else if (lut_res == JSL__MAP_TOMBSTONE)\n"
        "        {\n"
        "            map->entry_lookup_table[index] = JSL__MAP_EMPTY;\n"
        "        }\n"
        "\n"
        "        ++index;\n"
        "    }\n"
        "\n"
        "    if (params_valid)\n"
        "    {\n"
        "        map->item_count = 0;\n"
        "        map->tombstone_count = 0;\n"
        "        ++map->generational_id;\n"
        "    }\n"
        "\n"
        "    return;\n"
        "}\n"
        "\n"
        "#undef JSL__MAP_SSO_LENGTH\n"
        "#undef JSL__MAP_PRIVATE_SENTINEL\n"
    );

    static JSLImmutableMemory hash_map_name_key = JSL_CSTR_INITIALIZER("hash_map_name");
    static JSLImmutableMemory key_type_name_key = JSL_CSTR_INITIALIZER("key_type_name");
    static JSLImmutableMemory key_is_str_key = JSL_CSTR_INITIALIZER("key_is_str");
    static JSLImmutableMemory value_type_name_key = JSL_CSTR_INITIALIZER("value_type_name");
    static JSLImmutableMemory value_is_str_key = JSL_CSTR_INITIALIZER("value_is_str");
    static JSLImmutableMemory function_prefix_key = JSL_CSTR_INITIALIZER("function_prefix");
    static JSLImmutableMemory hash_function_key = JSL_CSTR_INITIALIZER("hash_function");
    static JSLImmutableMemory key_compare_key = JSL_CSTR_INITIALIZER("key_compare");

    static JSLImmutableMemory int32_t_str = JSL_CSTR_INITIALIZER("int32_t");
    static JSLImmutableMemory int_str = JSL_CSTR_INITIALIZER("int");
    static JSLImmutableMemory unsigned_str = JSL_CSTR_INITIALIZER("unsigned");
    static JSLImmutableMemory unsigned_int_str = JSL_CSTR_INITIALIZER("unsigned int");
    static JSLImmutableMemory uint32_t_str = JSL_CSTR_INITIALIZER("uint32_t");
    static JSLImmutableMemory int64_t_str = JSL_CSTR_INITIALIZER("int64_t");
    static JSLImmutableMemory long_str = JSL_CSTR_INITIALIZER("long");
    static JSLImmutableMemory long_int_str = JSL_CSTR_INITIALIZER("long int");
    static JSLImmutableMemory long_long_str = JSL_CSTR_INITIALIZER("long long");
    static JSLImmutableMemory long_long_int_str = JSL_CSTR_INITIALIZER("long long int");
    static JSLImmutableMemory uint64_t_str = JSL_CSTR_INITIALIZER("uint64_t");
    static JSLImmutableMemory unsigned_long_str = JSL_CSTR_INITIALIZER("unsigned long");
    static JSLImmutableMemory unsigned_long_long_str = JSL_CSTR_INITIALIZER("unsigned long long");
    static JSLImmutableMemory unsigned_long_long_int_str = JSL_CSTR_INITIALIZER("unsigned long long int");

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

    typedef struct TemplateCondFrame {
        bool parent_active;
        bool branch_taken;
        bool currently_active;
    } TemplateCondFrame;

    // Evaluate a template condition expression. Supports `and` and `or`
    // operators with standard precedence (and > or). Each operand is a
    // variable name checked for existence in the variables map.
    // Examples: "key_is_str", "key_is_str and value_is_str",
    //           "a or b", "a and b or c and d"
    // Maximum 16 tokens (8 variables + 7 operators).
    static bool evaluate_template_condition(
        JSLImmutableMemory argument,
        JSLStrToStrMap* variables
    )
    {
        static JSLImmutableMemory kw_and = JSL_CSTR_INITIALIZER("and");
        static JSLImmutableMemory kw_or = JSL_CSTR_INITIALIZER("or");

        bool and_accum = true;
        bool or_accum = false;
        bool seen_var = false;

        JSLImmutableMemory remaining = argument;

        while (remaining.length > 0)
        {
            // Skip leading whitespace
            int64_t start = 0;
            while (start < remaining.length
                && (remaining.data[start] == ' '
                    || remaining.data[start] == '\t'))
            {
                ++start;
            }
            JSL_MEMORY_ADVANCE(remaining, start);

            // Find end of current token
            int64_t end = 0;
            while (end < remaining.length
                && remaining.data[end] != ' '
                && remaining.data[end] != '\t')
            {
                ++end;
            }

            bool has_token = end > 0;
            JSLImmutableMemory token = {0};
            bool is_and = false;
            bool is_or = false;
            bool is_var = false;

            if (has_token)
            {
                token = jsl_slice(remaining, 0, end);
                is_and = jsl_memory_compare(token, kw_and);
                is_or = jsl_memory_compare(token, kw_or);
                is_var = !is_and && !is_or;
            }

            if (is_or)
            {
                or_accum = or_accum || and_accum;
                and_accum = true;
            }

            if (is_var)
            {
                bool exists = jsl_str_to_str_map_has_key(variables, token);
                and_accum = and_accum && exists;
                seen_var = true;
            }

            JSL_MEMORY_ADVANCE(remaining, end);
        }

        bool result = seen_var && (or_accum || and_accum);
        return result;
    }

    static void render_template(
        JSLOutputSink sink,
        JSLImmutableMemory template,
        JSLStrToStrMap* variables
    )
    {
        static JSLImmutableMemory open_var = JSL_CSTR_INITIALIZER("{{");
        static JSLImmutableMemory close_var = JSL_CSTR_INITIALIZER("}}");
        static JSLImmutableMemory open_tag = JSL_CSTR_INITIALIZER("{%");
        static JSLImmutableMemory close_tag = JSL_CSTR_INITIALIZER("%}");

        static JSLImmutableMemory kw_if = JSL_CSTR_INITIALIZER("if");
        static JSLImmutableMemory kw_elif = JSL_CSTR_INITIALIZER("elif");
        static JSLImmutableMemory kw_else = JSL_CSTR_INITIALIZER("else");
        static JSLImmutableMemory kw_endif = JSL_CSTR_INITIALIZER("endif");

        TemplateCondFrame cond_stack[32];
        int64_t cond_depth = 0;

        JSLImmutableMemory reader = template;

        while (reader.length > 0)
        {
            int64_t idx_var = jsl_substring_search(reader, open_var);
            int64_t idx_tag = jsl_substring_search(reader, open_tag);

            bool no_markers = idx_var == -1 && idx_tag == -1;
            if (no_markers)
            {
                bool active = cond_depth == 0
                    || cond_stack[cond_depth - 1].currently_active;
                if (active)
                {
                    jsl_output_sink_write(sink, reader);
                }
                break;
            }

            // Determine which marker comes first
            bool tag_first = idx_tag != -1
                && (idx_var == -1 || idx_tag < idx_var);

            //
            // Process {% %} conditional tag
            //
            if (tag_first)
            {
                bool active = cond_depth == 0
                    || cond_stack[cond_depth - 1].currently_active;

                // Write text before the tag, stripping the whitespace-only
                // prefix on the tag's line so that {% %} lines on their own
                // don't inject extra indentation into the output.
                if (idx_tag > 0 && active)
                {
                    int64_t last_nl = -1;
                    for (int64_t i = idx_tag - 1; i >= 0; --i)
                    {
                        if (reader.data[i] == '\n')
                        {
                            last_nl = i;
                            break;
                        }
                    }

                    int64_t line_start = last_nl + 1;
                    bool ws_only = true;
                    for (int64_t i = line_start; i < idx_tag; ++i)
                    {
                        bool is_ws = reader.data[i] == ' '
                            || reader.data[i] == '\t';
                        if (!is_ws)
                        {
                            ws_only = false;
                            break;
                        }
                    }

                    int64_t write_end = ws_only ? line_start : idx_tag;
                    if (write_end > 0)
                    {
                        JSLImmutableMemory before = jsl_slice(
                            reader, 0, write_end
                        );
                        jsl_output_sink_write(sink, before);
                    }
                }

                JSL_MEMORY_ADVANCE(reader, idx_tag + open_tag.length);

                int64_t idx_close = jsl_substring_search(reader, close_tag);
                bool malformed = idx_close == -1;
                if (malformed)
                {
                    if (active)
                    {
                        jsl_output_sink_write(sink, open_tag);
                        jsl_output_sink_write(sink, reader);
                    }
                    break;
                }

                JSLImmutableMemory tag_content = jsl_slice(reader, 0, idx_close);
                jsl_strip_whitespace(&tag_content);

                JSL_MEMORY_ADVANCE(reader, idx_close + close_tag.length);

                // Consume one trailing newline after the tag
                bool has_lf = reader.length >= 2
                    && reader.data[0] == '\r'
                    && reader.data[1] == '\n';
                if (has_lf)
                {
                    JSL_MEMORY_ADVANCE(reader, 2);
                }
                bool has_nl = !has_lf
                    && reader.length >= 1
                    && reader.data[0] == '\n';
                if (has_nl)
                {
                    JSL_MEMORY_ADVANCE(reader, 1);
                }

                // Parse directive: split on first whitespace
                JSLImmutableMemory directive = tag_content;
                JSLImmutableMemory argument = {0};

                int64_t space_pos = -1;
                for (int64_t i = 0; i < tag_content.length; ++i)
                {
                    bool is_ws = tag_content.data[i] == ' '
                        || tag_content.data[i] == '\t';
                    if (is_ws)
                    {
                        space_pos = i;
                        break;
                    }
                }

                bool has_argument = space_pos != -1;
                if (has_argument)
                {
                    directive = jsl_slice(tag_content, 0, space_pos);
                    argument = jsl_slice(
                        tag_content, space_pos + 1, tag_content.length
                    );
                    jsl_strip_whitespace(&argument);
                }

                bool is_if = jsl_memory_compare(directive, kw_if);
                bool is_elif = jsl_memory_compare(directive, kw_elif);
                bool is_else = jsl_memory_compare(directive, kw_else);
                bool is_endif = jsl_memory_compare(directive, kw_endif);

                if (is_if && cond_depth < 32)
                {
                    bool parent = cond_depth == 0
                        || cond_stack[cond_depth - 1].currently_active;
                    bool truthy = parent
                        && evaluate_template_condition(argument, variables);

                    cond_stack[cond_depth].parent_active = parent;
                    cond_stack[cond_depth].branch_taken = truthy;
                    cond_stack[cond_depth].currently_active = truthy;
                    ++cond_depth;
                }

                if (is_elif && cond_depth > 0)
                {
                    TemplateCondFrame* frame = &cond_stack[cond_depth - 1];
                    bool truthy = frame->parent_active
                        && !frame->branch_taken
                        && evaluate_template_condition(argument, variables);

                    frame->currently_active = truthy;
                    if (truthy)
                    {
                        frame->branch_taken = true;
                    }
                }

                if (is_else && cond_depth > 0)
                {
                    TemplateCondFrame* frame = &cond_stack[cond_depth - 1];
                    bool should_activate = frame->parent_active
                        && !frame->branch_taken;

                    frame->currently_active = should_activate;
                    if (should_activate)
                    {
                        frame->branch_taken = true;
                    }
                }

                if (is_endif && cond_depth > 0)
                {
                    --cond_depth;
                }
            }

            //
            // Process {{ }} variable substitution
            //
            bool var_first = !tag_first;
            if (var_first)
            {
                bool active = cond_depth == 0
                    || cond_stack[cond_depth - 1].currently_active;

                // Write text before the variable
                if (idx_var > 0 && active)
                {
                    JSLImmutableMemory before = jsl_slice(reader, 0, idx_var);
                    jsl_output_sink_write(sink, before);
                }
                if (idx_var > 0 && !active)
                {
                    // skip text silently
                }

                JSL_MEMORY_ADVANCE(reader, idx_var + open_var.length);

                int64_t idx_close = jsl_substring_search(reader, close_var);
                bool malformed = idx_close == -1;
                if (malformed)
                {
                    if (active)
                    {
                        jsl_output_sink_write(sink, open_var);
                        jsl_output_sink_write(sink, reader);
                    }
                    break;
                }

                JSLImmutableMemory var_name = jsl_slice(reader, 0, idx_close);
                jsl_strip_whitespace(&var_name);

                JSLImmutableMemory var_value;
                bool found = jsl_str_to_str_map_get(
                    variables, var_name, &var_value
                );
                if (active && found)
                {
                    jsl_output_sink_write(sink, var_value);
                }

                JSL_MEMORY_ADVANCE(reader, idx_close + close_var.length);
            }
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
        JSLAllocatorInterface allocator,
        JSLOutputSink sink,
        HashMapImplementation impl,
        JSLImmutableMemory hash_map_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory key_type_name,
        bool key_is_str,
        JSLImmutableMemory value_type_name,
        bool value_is_str,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    )
    {
        assert(hash_map_name.data != NULL && hash_map_name.length > 0);
        assert(function_prefix.data != NULL && function_prefix.length > 0);
        assert(!(include_header_array != NULL && include_header_count < 1));
        assert(!(key_type_name.data == NULL && !key_is_str));
        assert(!(value_type_name.data == NULL && !value_is_str));
        assert(!(key_is_str && value_is_str));

        srand((uint32_t) (time(NULL) % UINT32_MAX));

        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
        );

        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("#pragma once\n\n"));
        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("#include <stdint.h>\n"));
        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("#include \"jsl/allocator.h\"\n"));
        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("#include \"jsl/hash_map_common.h\"\n"));
        jsl_output_sink_write_u8(
            sink,
            '\n'
        );

        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("// USER INCLUDED HEADERS\n")
        );

        for (int32_t i = 0; i < include_header_count; ++i)
        {
            jsl_format_sink(sink, JSL_CSTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
        }

        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("\n"));
        
        jsl_format_sink(
            sink,
            JSL_CSTR_EXPRESSION("#define PRIVATE_SENTINEL_%y %" PRIu32 "U \n"),
            hash_map_name,
            rand_u32()
        );

        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("\n"));

        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, allocator, 0x123456789);

        jsl_str_to_str_map_insert(
            &map,
            hash_map_name_key,
            JSL_STRING_LIFETIME_LONGER,
            hash_map_name,
            JSL_STRING_LIFETIME_LONGER
        );

        if (key_is_str)
            jsl_str_to_str_map_insert(
                &map,
                key_is_str_key,
                JSL_STRING_LIFETIME_LONGER,
                JSL_CSTR_EXPRESSION(""),
                JSL_STRING_LIFETIME_LONGER
            );
        else
            jsl_str_to_str_map_insert(
                &map,
                key_type_name_key,
                JSL_STRING_LIFETIME_LONGER,
                key_type_name,
                JSL_STRING_LIFETIME_LONGER
            );

        if (value_is_str)
            jsl_str_to_str_map_insert(
                &map,
                value_is_str_key,
                JSL_STRING_LIFETIME_LONGER,
                JSL_CSTR_EXPRESSION(""),
                JSL_STRING_LIFETIME_LONGER
            );
        else
            jsl_str_to_str_map_insert(
                &map,
                value_type_name_key,
                JSL_STRING_LIFETIME_LONGER,
                value_type_name,
                JSL_STRING_LIFETIME_LONGER
            );

        jsl_str_to_str_map_insert(
            &map,
            function_prefix_key,
            JSL_STRING_LIFETIME_LONGER,
            function_prefix,
            JSL_STRING_LIFETIME_LONGER
        );

        if (impl == IMPL_FIXED)
            render_template(sink, fixed_header_template, &map);
        else if (impl == IMPL_DYNAMIC)
            render_template(sink, dynamic_header_template, &map);
        else
            assert(0);
    }

    GENERATE_HASH_MAP_DEF void write_hash_map_source(
        JSLAllocatorInterface allocator,
        JSLOutputSink sink,
        HashMapImplementation impl,
        JSLImmutableMemory hash_map_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory key_type_name,
        bool key_is_str,
        JSLImmutableMemory value_type_name,
        bool value_is_str,
        JSLImmutableMemory hash_function_name,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    )
    {
        (void) impl;

        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
        );

        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("#include <stddef.h>\n")
        );
        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("#include <stdint.h>\n")
        );
        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("#include \"jsl/core.h\"\n")
        );
        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("#include \"jsl/allocator.h\"\n"));
        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("#include \"jsl/hash_map_common.h\"\n\n")
        );

        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("// USER INCLUDED HEADERS\n")
        );

        for (int32_t i = 0; i < include_header_count; ++i)
        {
            jsl_format_sink(sink, JSL_CSTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
        }

        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("\n"));


        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, allocator, 0x123456789);

        jsl_str_to_str_map_insert(
            &map,
            hash_map_name_key,
            JSL_STRING_LIFETIME_LONGER,
            hash_map_name,
            JSL_STRING_LIFETIME_LONGER
        );

        if (key_is_str)
            jsl_str_to_str_map_insert(
                &map,
                key_is_str_key,
                JSL_STRING_LIFETIME_LONGER,
                JSL_CSTR_EXPRESSION(""),
                JSL_STRING_LIFETIME_LONGER
            );
        else
            jsl_str_to_str_map_insert(
                &map,
                key_type_name_key,
                JSL_STRING_LIFETIME_LONGER,
                key_type_name,
                JSL_STRING_LIFETIME_LONGER
            );

        if (value_is_str)
            jsl_str_to_str_map_insert(
                &map,
                value_is_str_key,
                JSL_STRING_LIFETIME_LONGER,
                JSL_CSTR_EXPRESSION(""),
                JSL_STRING_LIFETIME_LONGER
            );
        else
            jsl_str_to_str_map_insert(
                &map,
                value_type_name_key,
                JSL_STRING_LIFETIME_LONGER,
                value_type_name,
                JSL_STRING_LIFETIME_LONGER
            );

        jsl_str_to_str_map_insert(
            &map,
            function_prefix_key,
            JSL_STRING_LIFETIME_LONGER,
            function_prefix,
            JSL_STRING_LIFETIME_LONGER
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
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("uint64_t hash = %y(&key, sizeof(%y), hash_map->seed)"),
                    hash_function_name,
                    key_type_name
                );
            }
            else if (key_is_str)
            {
                resolved_hash_function_call = JSL_CSTR_EXPRESSION("*out_hash = jsl__rapidhash_withSeed(key.data, (size_t) key.length, hash_map->seed)");
            }
            else if (
                key_type_name.data != NULL
                && (jsl_memory_compare(key_type_name, int32_t_str)
                || jsl_memory_compare(key_type_name, int_str)
                || jsl_memory_compare(key_type_name, unsigned_str)
                || jsl_memory_compare(key_type_name, unsigned_int_str)
                || jsl_memory_compare(key_type_name, uint32_t_str)
                || jsl_memory_compare(key_type_name, int64_t_str)
                || jsl_memory_compare(key_type_name, long_str)
                || jsl_memory_compare(key_type_name, uint64_t_str)
                || jsl_memory_compare(key_type_name, unsigned_long_str)
                || jsl_memory_compare(key_type_name, long_int_str)
                || jsl_memory_compare(key_type_name, long_long_str)
                || jsl_memory_compare(key_type_name, long_long_int_str)
                || jsl_memory_compare(key_type_name, unsigned_long_long_str)
                || jsl_memory_compare(key_type_name, unsigned_long_long_int_str)
                || key_type_name.data[key_type_name.length - 1] == '*')
            )
            {
                resolved_hash_function_call = jsl_format(
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("*out_hash = jsl__murmur3_fmix_u64((uint64_t) key, hash_map->seed)"),
                    key_type_name
                );
            }
            else
            {
                resolved_hash_function_call = jsl_format(
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("*out_hash = jsl__rapidhash_withSeed(&key, sizeof(%y), hash_map->seed)"),
                    key_type_name
                );
            }

            jsl_str_to_str_map_insert(
                &map,
                hash_function_key,
                JSL_STRING_LIFETIME_LONGER,
                resolved_hash_function_call,
                JSL_STRING_LIFETIME_LONGER
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
                key_type_name.data != NULL
                && (
                    jsl_memory_compare(key_type_name, int32_t_str)
                    || jsl_memory_compare(key_type_name, int_str)
                    || jsl_memory_compare(key_type_name, unsigned_str)
                    || jsl_memory_compare(key_type_name, unsigned_int_str)
                    || jsl_memory_compare(key_type_name, uint32_t_str)
                    || jsl_memory_compare(key_type_name, int64_t_str)
                    || jsl_memory_compare(key_type_name, long_str)
                    || jsl_memory_compare(key_type_name, uint64_t_str)
                    || jsl_memory_compare(key_type_name, unsigned_long_str)
                    || jsl_memory_compare(key_type_name, long_int_str)
                    || jsl_memory_compare(key_type_name, long_long_str)
                    || jsl_memory_compare(key_type_name, long_long_int_str)
                    || jsl_memory_compare(key_type_name, unsigned_long_long_str)
                    || jsl_memory_compare(key_type_name, unsigned_long_long_int_str)
                    || key_type_name.data[key_type_name.length - 1] == '*'
                )
            )
            {
                resolved_key_compare = jsl_format(
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("key == hash_map->keys_array[slot]"),
                    key_type_name
                );
            }
            else if (key_is_str)
            {
                resolved_key_compare = JSL_CSTR_EXPRESSION("jsl_memory_compare(key, hash_map->keys_array[slot])");
            }
            else
            {
                resolved_key_compare = jsl_format(
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("JSL_MEMCMP(&key, &hash_map->keys_array[slot], sizeof(%y)) == 0"),
                    key_type_name
                );
            }

            jsl_str_to_str_map_insert(
                &map,
                key_compare_key,
                JSL_STRING_LIFETIME_LONGER,
                resolved_key_compare,
                JSL_STRING_LIFETIME_LONGER
            );
        }

        render_template(sink, fixed_source_template, &map);
    }

#endif /* GENERATE_HASH_MAP_IMPLEMENTATION */

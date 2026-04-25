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
 * The largest benefit from using this as a library is that it's way faster
 * than using it as a program. The vast majority of the time spent by the
 * program is 1. waiting for memory from the OS 2. writing to stdout. This
 * problem is exacerbated if you're running the program multiple times for
 * multiple hash map instantiations. When using the library with your
 * provided allocator and output mechanism this cost is paid once and
 * amortized over each invocation.
 * 
 * The benefit to using the program instead of the library is that it is
 * very, very easy to multiprocess.
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
        JSLImmutableMemory compare_function_name,
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
        "/**\r\n"
        " * AUTO GENERATED FILE\r\n"
        " *\r\n"
        "{% if key_is_str %}\r\n"
        " * This file contains the header for a hash map `{{ hash_map_name }}` which maps\r\n"
        " * `JSLImmutableMemory` keys to `{{ value_type_name }}` values.\r\n"
        "{% elif value_is_str %}\r\n"
        " * This file contains the header for a hash map `{{ hash_map_name }}` which maps\r\n"
        " * `{{ key_type_name }}` keys to `JSLImmutableMemory` values.\r\n"
        "{% else %}\r\n"
        " * This file contains the header for a hash map `{{ hash_map_name }}` which maps\r\n"
        " * `{{ key_type_name }}` keys to `{{ value_type_name }}` values.\r\n"
        "{% endif %}\r\n"
        " *\r\n"
        " * This hash map is designed for situations where you can set an upper bound on the\r\n"
        " * number of items you will have and that upper bound is still a reasonable amount of\r\n"
        " * memory. This represents the vast majority case, as most hash maps will never have more\r\n"
        " * than 100 items. Even in cases where the struct is quite large e.g. over a kilobyte, and\r\n"
        " * you have a large upper bound, say 100k, thats still ~100MB of data. This is an incredibly\r\n"
        " * rare case and you probably only have one of these in your program; this hash map would\r\n"
        " * still work for that case.\r\n"
        " *\r\n"
        " * This hash map is not suited for cases where the hash map will shrink and grow quite\r\n"
        " * substantially or there's no known upper bound. The most common example would be user\r\n"
        " * input that cannot reasonably be limited, e.g. a word processing application cannot simply\r\n"
        " * refuse to open very large (+10gig) documents. If you have some hash map which is built\r\n"
        " * from the document file then you need some other allocation strategy (you probably don't\r\n"
        " * want a normal hash map either as you'd be streaming things in and out of memory).\r\n"
        " *\r\n"
        " * This file was auto generated from the hash map generation utility that's part of\r\n"
        " * the \"Jack's Standard Library\" project. The utility generates a header file and a\r\n"
        " * C file for a type safe, open addressed, hash map. By generating the code rather\r\n"
        " * than using macros, two benefits are gained. One, the code is much easier to debug.\r\n"
        " * Two, it's much more obvious how much code you're generating, which means you are\r\n"
        " * much less likely to accidentally create the combinatoric explosion of code that's\r\n"
        " * so common in C++ projects. Adding friction to things is actually good sometimes.\r\n"
        " *\r\n"
        " * ## LICENSE\r\n"
        " *\r\n"
        " * Copyright (c) 2026 Jack Stouffer\r\n"
        " *\r\n"
        " * Permission is hereby granted, free of charge, to any person obtaining a\r\n"
        " * copy of this software and associated documentation files (the \"Software\"),\r\n"
        " * to deal in the Software without restriction, including without limitation\r\n"
        " * the rights to use, copy, modify, merge, publish, distribute, sublicense,\r\n"
        " * and/or sell copies of the Software, and to permit persons to whom the Software\r\n"
        " * is furnished to do so, subject to the following conditions:\r\n"
        " *\r\n"
        " * The above copyright notice and this permission notice shall be included in all\r\n"
        " * copies or substantial portions of the Software.\r\n"
        " *\r\n"
        " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n"
        " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\r\n"
        " * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\r\n"
        " * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\r\n"
        " * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\r\n"
        " * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\r\n"
        " */\r\n"
        "\r\n"
        "/**\r\n"
        " * A hash map which maps `{{ key_type_name }}` keys to `{{ value_type_name }}` values.\r\n"
        " *\r\n"
        " * This hash map uses open addressing with linear probing. However, it never grows.\r\n"
        " * When initialized with the init function, all the memory this hash map will have\r\n"
        " * is allocated right away.\r\n"
        " */\r\n"
        "typedef struct {{ hash_map_name }} {\r\n"
        "    // putting the sentinel first means it's much more likely to get\r\n"
        "    // corrupted from accidental overwrites, therefore making it\r\n"
        "    // more likely that memory bugs are caught.\r\n"
        "    uint32_t sentinel;\r\n"
        "    uint32_t generational_id;\r\n"
        "    JSLAllocatorInterface allocator;\r\n"
        "\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory* keys_array;\r\n"
        "    JSLStringLifeTime* key_lifetime_array;\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }}* keys_array;\r\n"
        "    {% endif %}\r\n"
        "\r\n"
        "    {% if value_is_str %}\r\n"
        "    JSLImmutableMemory* values_array;\r\n"
        "    JSLStringLifeTime* value_lifetime_array;\r\n"
        "    {% else %}\r\n"
        "    {{ value_type_name }}* values_array;\r\n"
        "    {% endif %}\r\n"
        "\r\n"
        "    uint64_t* hashes_array;\r\n"
        "    int64_t arrays_length;\r\n"
        "\r\n"
        "    int64_t item_count;\r\n"
        "    int64_t max_item_count;\r\n"
        "    uint64_t seed;\r\n"
        "} {{ hash_map_name }};\r\n"
        "\r\n"
        "/**\r\n"
        " * Iterator type which is used by the iterator functions to\r\n"
        " * allow you to loop over the hash map contents.\r\n"
        " */\r\n"
        "typedef struct {{ hash_map_name }}Iterator {\r\n"
        "    {{ hash_map_name }}* hash_map;\r\n"
        "    int64_t current_slot;\r\n"
        "    uint64_t generational_id;\r\n"
        "} {{ hash_map_name }}Iterator;\r\n"
        "\r\n"
        "/**\r\n"
        " * Initialize an instance of the hash map.\r\n"
        " *\r\n"
        " * All of the memory that this hash map will need will be allocated from the passed in arena.\r\n"
        " * The hash map does not save a reference to the arena, but the arena memory must have the same\r\n"
        " * or greater lifetime than the hash map itself.\r\n"
        " *\r\n"
        " * @warning This hash map uses a well distributed hash. But in order to properly protect against\r\n"
        " * hash flooding attacks you must do two things. One, provide good random data for the\r\n"
        " * seed value. This means using your OS's secure random number generator, not `rand`.\r\n"
        " * As this is very platform specific JSL does not come with a mechanism for getting these\r\n"
        " * random numbers; you must do it yourself. Two, use a different seed value as often as\r\n"
        " * possible, ideally every user interaction. This would make hash flooding attacks almost\r\n"
        " * impossible. If you are absolutely sure that this hash map cannot be attacked with hash\r\n"
        " * flooding then zero is a valid seed value.\r\n"
        " *\r\n"
        " * @param hash_map The pointer to the hash map instance to initialize\r\n"
        " * @param allocator The allocator that this hash map will use\r\n"
        " * @param max_item_count The maximum amount of items this hash map can hold\r\n"
        " * @param seed Seed value for the hash function to protect against hash flooding attacks\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_init(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    JSLAllocatorInterface allocator,\r\n"
        "    int64_t max_item_count,\r\n"
        "    uint64_t seed\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Insert the given value into the hash map. If the key already exists in \r\n"
        " * the map the value will be overwritten. If the key type for this hash map\r\n"
        " * is a pointer, then a NULL key is a valid key type.\r\n"
        " *\r\n"
        "{% if key_is_struct %}\r\n"
        " * With struct keys, struct padding can be filled with random-ish, garbage bytes.\r\n"
        " * This will cause the hash probe to fail. It is *very* important to either \r\n"
        " * 1, initialize the struct with memset to zero 2, use a canonicalization function\r\n"
        " * before using the struct in the hash map or 3. use a custom comparison function\r\n"
        " * (requires regenerating the source with the proper command line option). Do not\r\n"
        " * rely on `{0}` init! The compiler is allowed to cheat and skip padding bytes.\r\n"
        "{% endif %}\r\n"
        " *\r\n"
        " * @param hash_map The pointer to the hash map instance to initialize\r\n"
        " * @param key Hash map key\r\n"
        " * @param value Value to store\r\n"
        " * @returns A bool representing success or failure of insertion.\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_insert(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory key,\r\n"
        "    JSLStringLifeTime key_lifetime,\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}* key,\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }} key,\r\n"
        "    {% endif %}\r\n"
        "    {% if value_is_str %}\r\n"
        "    JSLImmutableMemory value,\r\n"
        "    JSLStringLifeTime value_lifetime\r\n"
        "    {% else %}\r\n"
        "    {{ value_type_name }} value\r\n"
        "    {% endif %}\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Get a value from the hash map if it exists. If it does not NULL is returned\r\n"
        " *\r\n"
        " * The pointer returned actually points to value stored inside of hash map.\r\n"
        " * You can change the value though the pointer.\r\n"
        " *\r\n"
        " * @param hash_map The pointer to the hash map instance to initialize\r\n"
        " * @param key Hash map key\r\n"
        " * @param value Value to store\r\n"
        " * @returns The pointer to the value in the hash map, or null.\r\n"
        " */\r\n"
        "{% if value_is_str %}\r\n"
        "JSLImmutableMemory {{ function_prefix }}_get(\r\n"
        "{% else %}\r\n"
        "{{ value_type_name }}* {{ function_prefix }}_get(\r\n"
        "{% endif %}\r\n"
        "\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory key\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}* key\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }} key\r\n"
        "    {% endif %}\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Remove a key/value pair from the hash map if it exists.\r\n"
        " * If it does not false is returned.\r\n"
        " *\r\n"
        " * This hash map uses backshift deletion instead of tombstones\r\n"
        " * due to the lack of rehashing. Deletion can be expensive in\r\n"
        " * medium sized maps.\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_delete(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory key\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}* key\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }} key\r\n"
        "    {% endif %}\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Free all the underlying memory that was allocated by this hash map on the given\r\n"
        " * allocator.\r\n"
        " */\r\n"
        "void {{ function_prefix }}_free(\r\n"
        "    {{ hash_map_name }}* hash_map\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Create a new iterator over this hash map.\r\n"
        " *\r\n"
        " * An iterator is a struct which holds enough state that it allows a loop to visit\r\n"
        " * each key/value pair in the hash map.\r\n"
        " *\r\n"
        " * Iterating over a hash map while modifying it does not have guaranteed\r\n"
        " * correctness. Any insertion or deletion after the iterator is created will\r\n"
        " * invalidate the iteration.\r\n"
        " *\r\n"
        " * Example usage:\r\n"
        " * @code\r\n"
        " * {{ key_type_name }} key;\r\n"
        " * {{ value_type_name }} value;\r\n"
        " * {{ hash_map_name }}Iterator iterator;\r\n"
        " * {{ function_prefix }}_iterator_start(hash_map, &iterator);\r\n"
        " * while ({{ function_prefix }}_iterator_next(&iterator, &key, &value))\r\n"
        " * {\r\n"
        " *     ...\r\n"
        " * }\r\n"
        " * @endcode\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_iterator_start(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {{ hash_map_name }}Iterator* iterator\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Iterate over the hash map. If a key/value was found then true is returned.\r\n"
        " *\r\n"
        " * Example usage:\r\n"
        " * @code\r\n"
        " * {{ key_type_name }} key;\r\n"
        " * {{ value_type_name }} value;\r\n"
        " * {{ hash_map_name }}Iterator iterator;\r\n"
        " * {{ function_prefix }}_iterator_start(hash_map, &iterator);\r\n"
        " * while ({{ function_prefix }}_iterator_next(&iterator, &key, &value))\r\n"
        " * {\r\n"
        " *     ...\r\n"
        " * }\r\n"
        " * @endcode\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_iterator_next(\r\n"
        "    {{ hash_map_name }}Iterator* iterator,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory* out_key,\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}** out_key,\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }}* out_key,\r\n"
        "    {% endif %}\r\n"
        "    {% if value_is_str %}\r\n"
        "    JSLImmutableMemory* out_value\r\n"
        "    {% else %}\r\n"
        "    {{ value_type_name }}* out_value\r\n"
        "    {% endif %}\r\n"
        ");\r\n"
        "\r\n"
    );

    static JSLImmutableMemory fixed_source_template = JSL_CSTR_INITIALIZER(
        "/**\r\n"
        " * AUTO GENERATED FILE\r\n"
        " *\r\n"
        " * See the header for more information.\r\n"
        " *\r\n"
        " * ## LICENSE\r\n"
        " *\r\n"
        " * Copyright (c) 2026 Jack Stouffer\r\n"
        " *\r\n"
        " * Permission is hereby granted, free of charge, to any person obtaining a\r\n"
        " * copy of this software and associated documentation files (the \"Software\"),\r\n"
        " * to deal in the Software without restriction, including without limitation\r\n"
        " * the rights to use, copy, modify, merge, publish, distribute, sublicense,\r\n"
        " * and/or sell copies of the Software, and to permit persons to whom the Software\r\n"
        " * is furnished to do so, subject to the following conditions:\r\n"
        " *\r\n"
        " * The above copyright notice and this permission notice shall be included in all\r\n"
        " * copies or substantial portions of the Software.\r\n"
        " *\r\n"
        " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n"
        " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\r\n"
        " * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\r\n"
        " * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\r\n"
        " * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\r\n"
        " * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\r\n"
        " */\r\n"
        "\r\n"
        "bool {{ function_prefix }}_init(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    JSLAllocatorInterface allocator,\r\n"
        "    int64_t max_item_count,\r\n"
        "    uint64_t seed\r\n"
        ")\r\n"
        "{\r\n"
        "    if (hash_map == NULL || max_item_count < 0)\r\n"
        "        return false;\r\n"
        "\r\n"
        "    JSL_MEMSET(hash_map, 0, sizeof({{ hash_map_name }}));\r\n"
        "\r\n"
        "    hash_map->seed = seed;\r\n"
        "    hash_map->allocator = allocator;\r\n"
        "    hash_map->max_item_count = max_item_count;\r\n"
        "\r\n"
        "    int64_t max_with_load_factor = (int64_t) ((float) max_item_count / 0.75f);\r\n"
        "\r\n"
        "    hash_map->arrays_length = jsl_next_power_of_two_i64(max_with_load_factor);\r\n"
        "    hash_map->arrays_length = JSL_MAX(hash_map->arrays_length, 32);\r\n"
        "\r\n"
        "    {% if key_is_str %}\r\n"
        "    hash_map->keys_array = (JSLImmutableMemory*) jsl_allocator_interface_alloc(\r\n"
        "        allocator,\r\n"
        "        ((int64_t) sizeof(JSLImmutableMemory)) * hash_map->arrays_length,\r\n"
        "        JSL_DEFAULT_ALLOCATION_ALIGNMENT,\r\n"
        "        false\r\n"
        "    );\r\n"
        "    if (hash_map->keys_array == NULL)\r\n"
        "        return false;\r\n"
        "    hash_map->key_lifetime_array = (JSLStringLifeTime*) jsl_allocator_interface_alloc(\r\n"
        "        allocator,\r\n"
        "        ((int64_t) sizeof(JSLStringLifeTime)) * hash_map->arrays_length,\r\n"
        "        JSL_DEFAULT_ALLOCATION_ALIGNMENT,\r\n"
        "        false\r\n"
        "    );\r\n"
        "    if (hash_map->key_lifetime_array == NULL)\r\n"
        "        return false;\r\n"
        "    {% else %}\r\n"
        "    hash_map->keys_array = ({{ key_type_name }}*) jsl_allocator_interface_alloc(\r\n"
        "        allocator,\r\n"
        "        ((int64_t) sizeof({{ key_type_name }})) * hash_map->arrays_length,\r\n"
        "        (int32_t) _Alignof({{ key_type_name }}),\r\n"
        "        false\r\n"
        "    );\r\n"
        "    if (hash_map->keys_array == NULL)\r\n"
        "        return false;\r\n"
        "    {% endif %}\r\n"
        "\r\n"
        "\r\n"
        "    {% if value_is_str %}\r\n"
        "    hash_map->values_array = (JSLImmutableMemory*) jsl_allocator_interface_alloc(\r\n"
        "        allocator,\r\n"
        "        ((int64_t) sizeof(JSLImmutableMemory)) * hash_map->arrays_length,\r\n"
        "        JSL_DEFAULT_ALLOCATION_ALIGNMENT,\r\n"
        "        false\r\n"
        "    );\r\n"
        "    if (hash_map->values_array == NULL)\r\n"
        "        return false;\r\n"
        "    hash_map->value_lifetime_array = (JSLStringLifeTime*) jsl_allocator_interface_alloc(\r\n"
        "        allocator,\r\n"
        "        ((int64_t) sizeof(JSLStringLifeTime)) * hash_map->arrays_length,\r\n"
        "        JSL_DEFAULT_ALLOCATION_ALIGNMENT,\r\n"
        "        false\r\n"
        "    );\r\n"
        "    if (hash_map->value_lifetime_array == NULL)\r\n"
        "        return false;\r\n"
        "    {% else %}\r\n"
        "    hash_map->values_array = ({{ value_type_name }}*) jsl_allocator_interface_alloc(\r\n"
        "        allocator,\r\n"
        "        ((int64_t) sizeof({{ value_type_name }})) * hash_map->arrays_length,\r\n"
        "        (int32_t) _Alignof({{ value_type_name }}),\r\n"
        "        false\r\n"
        "    );\r\n"
        "    if (hash_map->values_array == NULL)\r\n"
        "        return false;\r\n"
        "    {% endif %}\r\n"
        "\r\n"
        "    hash_map->hashes_array = (uint64_t*) jsl_allocator_interface_alloc(\r\n"
        "        allocator,\r\n"
        "        ((int64_t) sizeof(uint64_t)) * hash_map->arrays_length,\r\n"
        "        (int32_t) _Alignof(uint64_t),\r\n"
        "        true\r\n"
        "    );\r\n"
        "    if (hash_map->hashes_array == NULL)\r\n"
        "        return false;\r\n"
        "\r\n"
        "    hash_map->sentinel = PRIVATE_SENTINEL_{{ hash_map_name }};\r\n"
        "    return true;\r\n"
        "}\r\n"
        "\r\n"
        "static inline void {{ function_prefix }}_probe(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory key,\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}* key,\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }} key,\r\n"
        "    {% endif %}\r\n"
        "    int64_t* out_slot,\r\n"
        "    uint64_t* out_hash,\r\n"
        "    bool* out_found\r\n"
        ")\r\n"
        "{\r\n"
        "    *out_slot = -1;\r\n"
        "    *out_found = false;\r\n"
        "\r\n"
        "    {% if key_is_struct %}\r\n"
        "    // In JSL_DEBUG, check that the key has zeroed struct padding to help catch\r\n"
        "    // garbage byte errors\r\n"
        "    #if defined(JSL_DEBUG)\r\n"
        "        #ifdef __clang__\r\n"
        "            #if __has_builtin(__builtin_clear_padding)\r\n"
        "                {\r\n"
        "                    {{ key_type_name }} padding_check_copy = *key;\r\n"
        "                    __builtin_clear_padding(&padding_check_copy);\r\n"
        "                    JSL_ASSERT(\r\n"
        "                        JSL_MEMCMP(key, &padding_check_copy, sizeof({{ key_type_name }})) == 0\r\n"
        "                        && \"Hash map struct key has non-zero padding bytes. Initialize struct keys with JSL_MEMSET before setting fields.\"\r\n"
        "                    );\r\n"
        "                }\r\n"
        "            #endif\r\n"
        "        #elif defined(__GNUC__) && __GNUC__ >= 11\r\n"
        "            {\r\n"
        "                {{ key_type_name }} padding_check_copy = *key;\r\n"
        "                __builtin_clear_padding(&padding_check_copy);\r\n"
        "                JSL_ASSERT(\r\n"
        "                    JSL_MEMCMP(key, &padding_check_copy, sizeof({{ key_type_name }})) == 0\r\n"
        "                    && \"Hash map struct key has non-zero padding bytes. Initialize struct keys with JSL_MEMSET before setting fields.\"\r\n"
        "                );\r\n"
        "            }\r\n"
        "        #endif\r\n"
        "    #endif\r\n"
        "    {% endif %}\r\n"
        "\r\n"
        "    {{ hash_function }};\r\n"
        "\r\n"
        "    // Avoid clashing with sentinel values\r\n"
        "    if (*out_hash <= (uint64_t) JSL__HASHMAP_TOMBSTONE)\r\n"
        "    {\r\n"
        "        *out_hash = (uint64_t) JSL__HASHMAP_VALUE_OK;\r\n"
        "    }\r\n"
        "\r\n"
        "    int64_t total_checked = 0;\r\n"
        "    uint64_t slot_mask = (uint64_t) hash_map->arrays_length - 1u;\r\n"
        "    // Since our slot array length is always a pow 2, we can avoid a modulo\r\n"
        "    int64_t slot = (int64_t) (*out_hash & slot_mask);\r\n"
        "\r\n"
        "    while (total_checked < hash_map->arrays_length)\r\n"
        "    {\r\n"
        "        uint64_t slot_hash_value = hash_map->hashes_array[slot];\r\n"
        "\r\n"
        "        if (slot_hash_value == JSL__HASHMAP_EMPTY)\r\n"
        "        {\r\n"
        "            *out_slot = slot;\r\n"
        "            break;\r\n"
        "        }\r\n"
        "\r\n"
        "        if (slot_hash_value == *out_hash && {{ key_compare }})\r\n"
        "        {\r\n"
        "            *out_found = true;\r\n"
        "            *out_slot = slot;\r\n"
        "            break;\r\n"
        "        }\r\n"
        "\r\n"
        "        slot = (int64_t) (((uint64_t) slot + 1u) & slot_mask);\r\n"
        "        ++total_checked;\r\n"
        "    }\r\n"
        "\r\n"
        "    if (total_checked >= hash_map->arrays_length)\r\n"
        "    {\r\n"
        "        *out_slot = -1;\r\n"
        "    }\r\n"
        "}\r\n"
        "\r\n"
        "static inline void {{ function_prefix }}_backshift(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    int64_t start_slot\r\n"
        ")\r\n"
        "{\r\n"
        "    uint64_t slot_mask = (uint64_t) hash_map->arrays_length - 1u;\r\n"
        "\r\n"
        "    int64_t hole = start_slot;\r\n"
        "    int64_t current = (int64_t) (((uint64_t) start_slot + 1u) & slot_mask);\r\n"
        "\r\n"
        "    int64_t loop_check = 0;\r\n"
        "    while (loop_check < hash_map->arrays_length)\r\n"
        "    {\r\n"
        "        uint64_t hash_value = hash_map->hashes_array[current];\r\n"
        "\r\n"
        "        if (hash_value == JSL__HASHMAP_EMPTY)\r\n"
        "        {\r\n"
        "            hash_map->hashes_array[hole] = JSL__HASHMAP_EMPTY;\r\n"
        "            break;\r\n"
        "        }\r\n"
        "\r\n"
        "        int64_t ideal_slot = (int64_t) (hash_value & slot_mask);\r\n"
        "\r\n"
        "        bool should_move = (current > hole)\r\n"
        "            ? (ideal_slot <= hole || ideal_slot > current)\r\n"
        "            : (ideal_slot <= hole && ideal_slot > current);\r\n"
        "\r\n"
        "        if (should_move)\r\n"
        "        {\r\n"
        "            hash_map->keys_array[hole] = hash_map->keys_array[current];\r\n"
        "            hash_map->values_array[hole] = hash_map->values_array[current];\r\n"
        "            hash_map->hashes_array[hole] = hash_map->hashes_array[current];\r\n"
        "            hole = current;\r\n"
        "        }\r\n"
        "\r\n"
        "        current = (int64_t) (((uint64_t) current + 1u) & slot_mask);\r\n"
        "\r\n"
        "        ++loop_check;\r\n"
        "    }\r\n"
        "}\r\n"
        "\r\n"
        "bool {{ function_prefix }}_insert(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory key,\r\n"
        "    JSLStringLifeTime key_lifetime,\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}* key,\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }} key,\r\n"
        "    {% endif %}\r\n"
        "    {% if value_is_str %}\r\n"
        "    JSLImmutableMemory value,\r\n"
        "    JSLStringLifeTime value_lifetime\r\n"
        "    {% else %}\r\n"
        "    {{ value_type_name }} value\r\n"
        "    {% endif %}\r\n"
        ")\r\n"
        "{\r\n"
        "    bool insert_success = false;\r\n"
        "\r\n"
        "    if (\r\n"
        "        hash_map == NULL\r\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\r\n"
        "        || hash_map->item_count >= hash_map->max_item_count\r\n"
        "    )\r\n"
        "        return insert_success;\r\n"
        "\r\n"
        "    uint64_t hash = 0;\r\n"
        "    int64_t slot = -1;\r\n"
        "    bool existing_found = false;\r\n"
        "    {{ function_prefix }}_probe(hash_map, key, &slot, &hash, &existing_found);\r\n"
        "\r\n"
        "    // new key\r\n"
        "    if (slot > -1 && !existing_found)\r\n"
        "    {\r\n"
        "        {% if key_is_str %}\r\n"
        "        if (key_lifetime == JSL_STRING_LIFETIME_SHORTER)\r\n"
        "            hash_map->keys_array[slot] = jsl_duplicate(hash_map->allocator, key);\r\n"
        "        else\r\n"
        "            hash_map->keys_array[slot] = key;\r\n"
        "\r\n"
        "        hash_map->key_lifetime_array[slot] = key_lifetime;\r\n"
        "        {% elif key_is_struct %}\r\n"
        "        hash_map->keys_array[slot] = *key;\r\n"
        "        {% else %}\r\n"
        "        hash_map->keys_array[slot] = key;\r\n"
        "        {% endif %}\r\n"
        "\r\n"
        "        {% if value_is_str %}\r\n"
        "        if (value_lifetime == JSL_STRING_LIFETIME_SHORTER)\r\n"
        "            hash_map->values_array[slot] = jsl_duplicate(hash_map->allocator, value);\r\n"
        "        else\r\n"
        "            hash_map->values_array[slot] = value;\r\n"
        "\r\n"
        "        hash_map->value_lifetime_array[slot] = value_lifetime;\r\n"
        "        {% else %}\r\n"
        "        hash_map->values_array[slot] = value;\r\n"
        "        {% endif %}\r\n"
        "\r\n"
        "        hash_map->hashes_array[slot] = hash;\r\n"
        "        ++hash_map->item_count;\r\n"
        "        insert_success = true;\r\n"
        "    }\r\n"
        "    // update\r\n"
        "    else if (slot > -1 && existing_found)\r\n"
        "    {\r\n"
        "        {% if value_is_str %}\r\n"
        "        if (hash_map->value_lifetime_array[slot] == JSL_STRING_LIFETIME_SHORTER)\r\n"
        "            jsl_allocator_interface_free(hash_map->allocator, hash_map->values_array[slot].data);\r\n"
        "\r\n"
        "        if (value_lifetime == JSL_STRING_LIFETIME_SHORTER)\r\n"
        "            hash_map->values_array[slot] = jsl_duplicate(hash_map->allocator, value);\r\n"
        "        else\r\n"
        "            hash_map->values_array[slot] = value;\r\n"
        "\r\n"
        "        hash_map->value_lifetime_array[slot] = value_lifetime;\r\n"
        "        {% else %}\r\n"
        "        hash_map->values_array[slot] = value;\r\n"
        "        {% endif %}\r\n"
        "\r\n"
        "        insert_success = true;\r\n"
        "    }\r\n"
        "\r\n"
        "    if (insert_success)\r\n"
        "    {\r\n"
        "        ++hash_map->generational_id;\r\n"
        "    }\r\n"
        "\r\n"
        "    return insert_success;\r\n"
        "}\r\n"
        "\r\n"
        "{% if value_is_str %}\r\n"
        "JSLImmutableMemory {{ function_prefix }}_get(\r\n"
        "{% else %}\r\n"
        "{{ value_type_name }}* {{ function_prefix }}_get(\r\n"
        "{% endif %}\r\n"
        "\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory key\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}* key\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }} key\r\n"
        "    {% endif %}\r\n"
        ")\r\n"
        "{\r\n"
        "    {% if value_is_str %}\r\n"
        "    JSLImmutableMemory res = {0};\r\n"
        "    {% else %}\r\n"
        "    {{ value_type_name }}* res = NULL;\r\n"
        "    {% endif %}\r\n"
        "\r\n"
        "    if (\r\n"
        "        hash_map == NULL\r\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\r\n"
        "        || hash_map->values_array == NULL\r\n"
        "        || hash_map->keys_array == NULL\r\n"
        "        || hash_map->hashes_array == NULL\r\n"
        "    )\r\n"
        "        return res;\r\n"
        "\r\n"
        "    uint64_t hash = 0;\r\n"
        "    int64_t slot = -1;\r\n"
        "    bool existing_found = false;\r\n"
        "\r\n"
        "    {{ function_prefix }}_probe(hash_map, key, &slot, &hash, &existing_found);\r\n"
        "    \r\n"
        "    if (slot > -1 && existing_found)\r\n"
        "    {\r\n"
        "        {% if value_is_str %}\r\n"
        "        res = hash_map->values_array[slot];\r\n"
        "        {% else %}\r\n"
        "        res = &hash_map->values_array[slot];\r\n"
        "        {% endif %}\r\n"
        "    }\r\n"
        "\r\n"
        "    return res;\r\n"
        "}\r\n"
        "\r\n"
        "bool {{ function_prefix }}_delete(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory key\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}* key\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }} key\r\n"
        "    {% endif %}\r\n"
        ")\r\n"
        "{\r\n"
        "    bool success = false;\r\n"
        "\r\n"
        "    if (\r\n"
        "        hash_map == NULL\r\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\r\n"
        "        || hash_map->values_array == NULL\r\n"
        "        || hash_map->keys_array == NULL\r\n"
        "        || hash_map->hashes_array == NULL\r\n"
        "    )\r\n"
        "        return success;\r\n"
        "\r\n"
        "    uint64_t hash = 0;\r\n"
        "    int64_t slot = -1;\r\n"
        "    bool existing_found = false;\r\n"
        "    {{ function_prefix }}_probe(hash_map, key, &slot, &hash, &existing_found);\r\n"
        "\r\n"
        "    if (slot > -1 && existing_found)\r\n"
        "    {\r\n"
        "        {{ function_prefix }}_backshift(hash_map, slot);\r\n"
        "        --hash_map->item_count;\r\n"
        "        ++hash_map->generational_id;\r\n"
        "        success = true;\r\n"
        "    }\r\n"
        "\r\n"
        "    return success;\r\n"
        "}\r\n"
        "\r\n"
        "void {{ function_prefix }}_free(\r\n"
        "    {{ hash_map_name }}* hash_map\r\n"
        ")\r\n"
        "{\r\n"
        "    if (\r\n"
        "        hash_map == NULL\r\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\r\n"
        "    )\r\n"
        "        return;\r\n"
        "\r\n"
        "    {% if key_is_str or value_is_str %}\r\n"
        "    for (int64_t current_slot = 0; current_slot < hash_map->arrays_length; ++current_slot)\r\n"
        "    {\r\n"
        "        uint64_t hash_value = hash_map->hashes_array[current_slot];\r\n"
        "        {% if key_is_str %}\r\n"
        "        JSLStringLifeTime lifetime = hash_map->key_lifetime_array[current_slot];\r\n"
        "        if (hash_value != JSL__HASHMAP_EMPTY && lifetime == JSL_STRING_LIFETIME_SHORTER)\r\n"
        "        {\r\n"
        "            jsl_allocator_interface_free(hash_map->allocator, hash_map->keys_array[current_slot].data);\r\n"
        "        }\r\n"
        "        {% elif value_is_str %}\r\n"
        "        JSLStringLifeTime lifetime = hash_map->value_lifetime_array[current_slot];\r\n"
        "        if (hash_value != JSL__HASHMAP_EMPTY && lifetime == JSL_STRING_LIFETIME_SHORTER)\r\n"
        "        {\r\n"
        "            jsl_allocator_interface_free(hash_map->allocator, hash_map->values_array[current_slot].data);\r\n"
        "        }\r\n"
        "        {% endif %}\r\n"
        "    }\r\n"
        "\r\n"
        "    {% if key_is_str %}\r\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->key_lifetime_array);\r\n"
        "    {% elif value_is_str %}\r\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->value_lifetime_array);\r\n"
        "    {% endif %}\r\n"
        "\r\n"
        "    {% endif %}\r\n"
        "\r\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->keys_array);\r\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->values_array);\r\n"
        "    jsl_allocator_interface_free(hash_map->allocator, hash_map->hashes_array);\r\n"
        "}\r\n"
        "\r\n"
        "bool {{ function_prefix }}_iterator_start(\r\n"
        "    {{ hash_map_name }}* hash_map,\r\n"
        "    {{ hash_map_name }}Iterator* iterator\r\n"
        ")\r\n"
        "{\r\n"
        "    if (\r\n"
        "        hash_map == NULL\r\n"
        "        || hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\r\n"
        "    )\r\n"
        "        return false;\r\n"
        "\r\n"
        "    iterator->hash_map = hash_map;\r\n"
        "    iterator->current_slot = 0;\r\n"
        "    iterator->generational_id = hash_map->generational_id;\r\n"
        "\r\n"
        "    return true;\r\n"
        "}\r\n"
        "\r\n"
        "bool {{ function_prefix }}_iterator_next(\r\n"
        "    {{ hash_map_name }}Iterator* iterator,\r\n"
        "    {% if key_is_str %}\r\n"
        "    JSLImmutableMemory* out_key,\r\n"
        "    {% elif key_is_struct %}\r\n"
        "    const {{ key_type_name }}** out_key,\r\n"
        "    {% else %}\r\n"
        "    {{ key_type_name }}* out_key,\r\n"
        "    {% endif %}\r\n"
        "    {% if value_is_str %}\r\n"
        "    JSLImmutableMemory* out_value\r\n"
        "    {% else %}\r\n"
        "    {{ value_type_name }}* out_value\r\n"
        "    {% endif %}\r\n"
        ")\r\n"
        "{\r\n"
        "    bool found = false;\r\n"
        "\r\n"
        "    if (\r\n"
        "        iterator == NULL\r\n"
        "        || iterator->hash_map == NULL\r\n"
        "        || iterator->hash_map->sentinel != PRIVATE_SENTINEL_{{ hash_map_name }}\r\n"
        "        || iterator->hash_map->generational_id != iterator->generational_id\r\n"
        "        || iterator->hash_map->values_array == NULL\r\n"
        "        || iterator->hash_map->keys_array == NULL\r\n"
        "        || iterator->hash_map->hashes_array == NULL\r\n"
        "    )\r\n"
        "        return found;\r\n"
        "\r\n"
        "    int64_t found_entry = -1;\r\n"
        "\r\n"
        "    while (iterator->current_slot < iterator->hash_map->arrays_length)\r\n"
        "    {\r\n"
        "        uint64_t hash_value = iterator->hash_map->hashes_array[iterator->current_slot];\r\n"
        "\r\n"
        "        bool occupied = hash_value != JSL__HASHMAP_EMPTY;\r\n"
        "\r\n"
        "        if (occupied)\r\n"
        "        {\r\n"
        "            found_entry = iterator->current_slot;\r\n"
        "            break;\r\n"
        "        }\r\n"
        "        else\r\n"
        "        {\r\n"
        "            ++iterator->current_slot;\r\n"
        "        }\r\n"
        "    }\r\n"
        "\r\n"
        "    if (found_entry > -1)\r\n"
        "    {\r\n"
        "        {% if key_is_struct %}\r\n"
        "        *out_key = &iterator->hash_map->keys_array[iterator->current_slot];\r\n"
        "        {% else %}\r\n"
        "        *out_key = iterator->hash_map->keys_array[iterator->current_slot];\r\n"
        "        {% endif %}\r\n"
        "        *out_value = iterator->hash_map->values_array[iterator->current_slot];\r\n"
        "        ++iterator->current_slot;\r\n"
        "        found = true;\r\n"
        "    }\r\n"
        "    else\r\n"
        "    {\r\n"
        "        iterator->current_slot = iterator->hash_map->arrays_length;\r\n"
        "        found = false;\r\n"
        "    }\r\n"
        "\r\n"
        "    return found;\r\n"
        "}\r\n"
    );

    static JSLImmutableMemory dynamic_header_template = JSL_CSTR_INITIALIZER(
        "#pragma once\r\n"
        "\r\n"
        "#include <stdint.h>\r\n"
        "#include <stddef.h>\r\n"
        "#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L\r\n"
        "    #include <stdbool.h>\r\n"
        "#endif\r\n"
        "\r\n"
        "#include \"jsl/core.h\"\r\n"
        "#include \"jsl/hash_map_common.h\"\r\n"
        "\r\n"
        "#ifdef __cplusplus\r\n"
        "extern \"C\" {\r\n"
        "#endif\r\n"
        "\r\n"
        "struct {{ hash_map_name }}Entry {\r\n"
        "    uint64_t hash;\r\n"
        "    {{ key_type_name }} key;\r\n"
        "    {{ value_type_name }} value;\r\n"
        "};\r\n"
        "\r\n"
        "/**\r\n"
        " * State tracking struct for iterating over all of the keys and values\r\n"
        " * in the map.\r\n"
        " * \r\n"
        " * @note If you mutate the map this iterator is automatically invalidated\r\n"
        " * and any operations on this iterator will terminate with failure return\r\n"
        " * values.\r\n"
        " * \r\n"
        " * ## Functions\r\n"
        " *\r\n"
        " *  * jsl_str_to_str_map_key_value_iterator_init\r\n"
        " *  * jsl_str_to_str_map_key_value_iterator_next\r\n"
        " */\r\n"
        "typedef struct {{ hash_map_name }}KeyValueIter {\r\n"
        "    struct JSL__StrToStrMap* map;\r\n"
        "    int64_t current_lut_index;\r\n"
        "    int64_t generational_id;\r\n"
        "    uint64_t sentinel;\r\n"
        "} {{ hash_map_name }}KeyValueIter;\r\n"
        "\r\n"
        "/**\r\n"
        " * This is an open addressed hash map with linear probing that maps\r\n"
        " * {{ key_type_name }} keys to {{ value_type_name }} values. This map uses\r\n"
        " * rapidhash, which is a avalanche hash with a configurable seed\r\n"
        " * value for protection against hash flooding attacks.\r\n"
        " * \r\n"
        " * Example:\r\n"
        " *\r\n"
        " * ```\r\n"
        " * uint8_t buffer[JSL_KILOBYTES(16)];\r\n"
        " * JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);\r\n"
        " *\r\n"
        " * {{ hash_map_name }} map;\r\n"
        " * jsl_str_to_str_map_init(&map, &stack_arena, 0);\r\n"
        " *\r\n"
        " * JSLFatPtr key = JSL_FATPTR_INITIALIZER(\"hello-key\");\r\n"
        " * \r\n"
        " * jsl_str_to_str_multimap_insert(\r\n"
        " *     &map,\r\n"
        " *     key,\r\n"
        " *     JSL_STRING_LIFETIME_LONGER,\r\n"
        " *     JSL_FATPTR_EXPRESSION(\"hello-value\"),\r\n"
        " *     JSL_STRING_LIFETIME_LONGER\r\n"
        " * );\r\n"
        " * \r\n"
        " * {{ value_type_name }} value;\r\n"
        " * jsl_str_to_str_map_get(&map, key, &value);\r\n"
        " * ```\r\n"
        " * \r\n"
        " * ## Functions\r\n"
        " *\r\n"
        " *  * {{ function_prefix }}_init\r\n"
        " *  * {{ function_prefix }}_init2\r\n"
        " *  * {{ function_prefix }}_item_count\r\n"
        " *  * {{ function_prefix }}_has_key\r\n"
        " *  * {{ function_prefix }}_insert\r\n"
        " *  * {{ function_prefix }}_get\r\n"
        " *  * {{ function_prefix }}_key_value_iterator_init\r\n"
        " *  * {{ function_prefix }}_key_value_iterator_next\r\n"
        " *  * {{ function_prefix }}_delete\r\n"
        " *  * {{ function_prefix }}_clear\r\n"
        " *\r\n"
        " */\r\n"
        "typedef struct {{ hash_map_name }} {\r\n"
        "    // putting the sentinel first means it's much more likely to get\r\n"
        "    // corrupted from accidental overwrites, therefore making it\r\n"
        "    // more likely that memory bugs are caught.\r\n"
        "    uint64_t sentinel;\r\n"
        "\r\n"
        "    JSLArena* arena;\r\n"
        "\r\n"
        "    {{ hash_map_name }}Entry* entry_array;\r\n"
        "    int64_t entry_array_length;\r\n"
        "\r\n"
        "    int64_t item_count;\r\n"
        "    int64_t tombstone_count;\r\n"
        "\r\n"
        "    uint64_t seed;\r\n"
        "    float load_factor;\r\n"
        "    int32_t generational_id;\r\n"
        "} {{ hash_map_name }};\r\n"
        "\r\n"
        "/**\r\n"
        " * Initialize a map with default sizing parameters.\r\n"
        " *\r\n"
        " * This sets up internal tables in the provided arena, using a 32 entry\r\n"
        " * initial capacity guess and a 0.75 load factor. The `seed` value is to\r\n"
        " * protect against hash flooding attacks. If you're absolutely sure this\r\n"
        " * map cannot be attacked, then zero is valid seed value.\r\n"
        " *\r\n"
        " * @param map Pointer to the map to initialize.\r\n"
        " * @param arena Arena used for all allocations.\r\n"
        " * @param seed Arbitrary seed value for hashing.\r\n"
        " * @return `true` on success, `false` if any parameter is invalid or out of memory.\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_init(\r\n"
        "    {{ hash_map_name }}* map,\r\n"
        "    JSLArena* arena,\r\n"
        "    uint64_t seed\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Initialize a map with explicit sizing parameters.\r\n"
        " *\r\n"
        " * This is identical to `jsl_str_to_str_map_init`, but lets callers\r\n"
        " * provide an initial `item_count_guess` and a `load_factor`. The initial\r\n"
        " * lookup table is sized to the next power of two above `item_count_guess`,\r\n"
        " * clamped to at least 32 entries. `load_factor` must be in the range\r\n"
        " * `(0.0f, 1.0f)` and controls when the table rehashes. The `seed` value\r\n"
        " * is to protect against hash flooding attacks. If you're absolutely sure \r\n"
        " * this map cannot be attacked, then zero is valid seed value\r\n"
        " *\r\n"
        " * @param map Pointer to the map to initialize.\r\n"
        " * @param arena Arena used for all allocations.\r\n"
        " * @param seed Arbitrary seed value for hashing.\r\n"
        " * @param item_count_guess Expected max number of keys\r\n"
        " * @param load_factor Desired load factor before rehashing\r\n"
        " * @return `true` on success, `false` if any parameter is invalid or out of memory.\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_init2(\r\n"
        "    {{ hash_map_name }}* map,\r\n"
        "    JSLArena* arena,\r\n"
        "    uint64_t seed,\r\n"
        "    int64_t item_count_guess,\r\n"
        "    float load_factor\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Does the map have the given key.\r\n"
        " *\r\n"
        " * @param map Pointer to the map.\r\n"
        " * @return `true` if yes, `false` if no or error\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_has_key(\r\n"
        "    {{ hash_map_name }}* map,\r\n"
        "    {{ key_type_name }} key\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Insert a key/value pair.\r\n"
        " *\r\n"
        " * @param map Map to mutate.\r\n"
        " * @param key Key to insert.\r\n"
        " * @param key_lifetime Lifetime semantics for the key data.\r\n"
        " * @param value Value to insert.\r\n"
        " * @param value_lifetime Lifetime semantics for the value data.\r\n"
        " * @return `true` on success, `false` on invalid parameters or OOM.\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_insert(\r\n"
        "    {{ hash_map_name }}* map,\r\n"
        "    {{ key_type_name }} key,\r\n"
        "    {{ value_type_name }} value\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Get the value of the key.\r\n"
        " *\r\n"
        " * @param map Map to search.\r\n"
        " * @param key Key to search for.\r\n"
        " * @param out_value Output parameter that will be filled with the value if successful\r\n"
        " * @returns A bool indicating success or failure\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_get(\r\n"
        "    {{ hash_map_name }}* map,\r\n"
        "    {{ key_type_name }} key,\r\n"
        "    {{ value_type_name }}* out_value\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Initialize an iterator that visits every key/value pair in the map.\r\n"
        " * \r\n"
        " * Example:\r\n"
        " *\r\n"
        " * ```\r\n"
        " * {{ hash_map_name }}KeyValueIter iter;\r\n"
        " * {{ function_prefix }}_key_value_iterator_init(\r\n"
        " *     &map, &iter\r\n"
        " * );\r\n"
        " * \r\n"
        " * {{ key_type_name }} key;\r\n"
        " * {{ value_type_name }} value;\r\n"
        " * while ({{ function_prefix }}_key_value_iterator_next(&iter, &key, &value))\r\n"
        " * {\r\n"
        " *    ...\r\n"
        " * }\r\n"
        " * ```\r\n"
        " *\r\n"
        " * Overall traversal order is undefined. The iterator is invalidated\r\n"
        " * if the map is mutated after initialization.\r\n"
        " *\r\n"
        " * @param map Map to iterate over; must be initialized.\r\n"
        " * @param iterator Iterator instance to initialize.\r\n"
        " * @return `true` on success, `false` if parameters are invalid.\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_key_value_iterator_init(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLStrToStrMapKeyValueIter* iterator\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Advance the key/value iterator.\r\n"
        " * \r\n"
        " * Example:\r\n"
        " *\r\n"
        " * ```\r\n"
        " * {{ hash_map_name }}KeyValueIter iter;\r\n"
        " * {{ function_prefix }}_key_value_iterator_init(\r\n"
        " *     &map, &iter\r\n"
        " * );\r\n"
        " * \r\n"
        " * {{ key_type_name }} key;\r\n"
        " * {{ value_type_name }} value;\r\n"
        " * while ({{ function_prefix }}_key_value_iterator_next(&iter, &key, &value))\r\n"
        " * {\r\n"
        " *    ...\r\n"
        " * }\r\n"
        " * ```\r\n"
        " *\r\n"
        " * Returns the next key/value pair for the map. The iterator must be\r\n"
        " * initialized and is invalidated if the map is mutated; iteration order\r\n"
        " * is undefined.\r\n"
        " *\r\n"
        " * @param iterator Iterator to advance.\r\n"
        " * @param out_key Output for the current key.\r\n"
        " * @param out_value Output for the current value.\r\n"
        " * @return `true` if a pair was produced, `false` if exhausted or invalid.\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_key_value_iterator_next(\r\n"
        "    {{ hash_map_name }}KeyValueIter* iterator,\r\n"
        "    {{ key_type_name }}* out_key,\r\n"
        "    {{ value_type_name }}* out_value\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Remove a key/value.\r\n"
        " *\r\n"
        " * Iterators become invalid. If the key is not present or parameters are invalid,\r\n"
        " * the map is unchanged and `false` is returned.\r\n"
        " *\r\n"
        " * @param map Map to mutate.\r\n"
        " * @param key Key to remove.\r\n"
        " * @return `true` if the key existed and was removed, `false` otherwise.\r\n"
        " */\r\n"
        "bool {{ function_prefix }}_delete(\r\n"
        "    {{ hash_map_name }}* map,\r\n"
        "    {{ key_type_name }} key\r\n"
        ");\r\n"
        "\r\n"
        "/**\r\n"
        " * Remove all keys and values from the map.  Iterators become invalid.\r\n"
        " *\r\n"
        " * @param map Map to clear.\r\n"
        " */\r\n"
        "void {{ function_prefix }}_clear(\r\n"
        "    {{ hash_map_name }}* map\r\n"
        ");\r\n"
        "\r\n"
        "#ifdef __cplusplus\r\n"
        "}\r\n"
        "#endif\r\n"
    );
    static JSLImmutableMemory dynamic_source_template = JSL_CSTR_INITIALIZER(
        "/**\r\n"
        " * # JSL String to String Map\r\n"
        " * \r\n"
        " * This file is a single header file library that implements a hash map data\r\n"
        " * structure, which maps length based string keys to length based string values,\r\n"
        " * and is optimized around the arena allocator design. This file is part of\r\n"
        " * the Jack's Standard Library project.\r\n"
        " * \r\n"
        " * ## Documentation\r\n"
        " * \r\n"
        " * See `docs/jsl_str_to_str_map.md` for a formatted documentation page.\r\n"
        " *\r\n"
        " * ## Caveats\r\n"
        " * \r\n"
        " * This map uses arenas, so some wasted memory is indeveatble. Care has\r\n"
        " * been taken to reuse as much allocated memory as possible. But if your\r\n"
        " * map is long lived it's possible to start exhausting the arena with\r\n"
        " * old memory.\r\n"
        " * \r\n"
        " * Remember to\r\n"
        " * \r\n"
        " * * have an initial item count guess as accurate as you can to reduce rehashes\r\n"
        " * * have the arena have as short a lifetime as possible\r\n"
        " * \r\n"
        " * ## License\r\n"
        " *\r\n"
        " * Copyright (c) 2025 Jack Stouffer\r\n"
        " *\r\n"
        " * Permission is hereby granted, free of charge, to any person obtaining a\r\n"
        " * copy of this software and associated documentation files (the “Software”),\r\n"
        " * to deal in the Software without restriction, including without limitation\r\n"
        " * the rights to use, copy, modify, merge, publish, distribute, sublicense,\r\n"
        " * and/or sell copies of the Software, and to permit persons to whom the Software\r\n"
        " * is furnished to do so, subject to the following conditions:\r\n"
        " *\r\n"
        " * The above copyright notice and this permission notice shall be included in all\r\n"
        " * copies or substantial portions of the Software.\r\n"
        " *\r\n"
        " * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n"
        " * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\r\n"
        " * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\r\n"
        " * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\r\n"
        " * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN\r\n"
        " * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\r\n"
        " */\r\n"
        "\r\n"
        "#include <stdint.h>\r\n"
        "#include <stddef.h>\r\n"
        "#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L\r\n"
        "    #include <stdbool.h>\r\n"
        "#endif\r\n"
        "\r\n"
        "#include \"jsl/core.h\"\r\n"
        "#include \"jsl/hash_map_common.h\"\r\n"
        "#include \"jsl/str_to_str_map.h\"\r\n"
        "\r\n"
        "#define JSL__MAP_PRIVATE_SENTINEL 8973815015742603881U\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_init(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLArena* arena,\r\n"
        "    uint64_t seed\r\n"
        ")\r\n"
        "{\r\n"
        "    return jsl_str_to_str_map_init2(\r\n"
        "        map,\r\n"
        "        arena,\r\n"
        "        seed,\r\n"
        "        32,\r\n"
        "        0.75f\r\n"
        "    );\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_init2(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLArena* arena,\r\n"
        "    uint64_t seed,\r\n"
        "    int64_t item_count_guess,\r\n"
        "    float load_factor\r\n"
        ")\r\n"
        "{\r\n"
        "    bool res = true;\r\n"
        "\r\n"
        "    if (\r\n"
        "        map == NULL\r\n"
        "        || arena == NULL\r\n"
        "        || item_count_guess <= 0\r\n"
        "        || load_factor <= 0.0f\r\n"
        "        || load_factor >= 1.0f\r\n"
        "    )\r\n"
        "        res = false;\r\n"
        "\r\n"
        "    if (res)\r\n"
        "    {\r\n"
        "        JSL_MEMSET(map, 0, sizeof(JSLStrToStrMap));\r\n"
        "        map->arena = arena;\r\n"
        "        map->load_factor = load_factor;\r\n"
        "        map->hash_seed = seed;\r\n"
        "\r\n"
        "        item_count_guess = JSL_MAX(32L, item_count_guess);\r\n"
        "        int64_t items = jsl_next_power_of_two_i64(item_count_guess + 1);\r\n"
        "\r\n"
        "        map->entry_lookup_table = (uintptr_t*) jsl_arena_allocate_aligned(\r\n"
        "            arena,\r\n"
        "            (int64_t) sizeof(uintptr_t) * items,\r\n"
        "            _Alignof(uintptr_t),\r\n"
        "            true\r\n"
        "        ).data;\r\n"
        "        \r\n"
        "        map->entry_lookup_table_length = items;\r\n"
        "\r\n"
        "        map->sentinel = JSL__MAP_PRIVATE_SENTINEL;\r\n"
        "    }\r\n"
        "\r\n"
        "    return res;\r\n"
        "}\r\n"
        "\r\n"
        "static bool jsl__str_to_str_map_rehash(\r\n"
        "    JSLStrToStrMap* map\r\n"
        ")\r\n"
        "{\r\n"
        "    bool res = false;\r\n"
        "\r\n"
        "    bool params_valid = (\r\n"
        "        map != NULL\r\n"
        "        && map->arena != NULL\r\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && map->entry_lookup_table != NULL\r\n"
        "        && map->entry_lookup_table_length > 0\r\n"
        "    );\r\n"
        "\r\n"
        "    uintptr_t* old_table = params_valid ? map->entry_lookup_table : NULL;\r\n"
        "    int64_t old_length = params_valid ? map->entry_lookup_table_length : 0;\r\n"
        "\r\n"
        "    int64_t new_length = params_valid ? jsl_next_power_of_two_i64(old_length + 1) : 0;\r\n"
        "    bool length_valid = params_valid && new_length > old_length && new_length > 0;\r\n"
        "\r\n"
        "    bool bytes_possible = length_valid\r\n"
        "        && new_length <= (INT64_MAX / (int64_t) sizeof(uintptr_t));\r\n"
        "\r\n"
        "    int64_t bytes_needed = bytes_possible\r\n"
        "        ? (int64_t) sizeof(uintptr_t) * new_length\r\n"
        "        : 0;\r\n"
        "\r\n"
        "    JSLFatPtr new_table_mem = {0};\r\n"
        "    if (bytes_possible)\r\n"
        "    {\r\n"
        "        new_table_mem = jsl_arena_allocate_aligned(\r\n"
        "            map->arena,\r\n"
        "            bytes_needed,\r\n"
        "            _Alignof(uintptr_t),\r\n"
        "            true\r\n"
        "        );\r\n"
        "    }\r\n"
        "\r\n"
        "    uintptr_t* new_table = (bytes_possible && new_table_mem.data != NULL)\r\n"
        "        ? (uintptr_t*) new_table_mem.data\r\n"
        "        : NULL;\r\n"
        "\r\n"
        "    uint64_t lut_mask = new_length > 0 ? ((uint64_t) new_length - 1u) : 0;\r\n"
        "    int64_t old_index = 0;\r\n"
        "    bool migrate_ok = new_table != NULL;\r\n"
        "\r\n"
        "    while (migrate_ok && old_index < old_length)\r\n"
        "    {\r\n"
        "        uintptr_t lut_res = old_table[old_index];\r\n"
        "\r\n"
        "        bool occupied = (\r\n"
        "            lut_res != 0\r\n"
        "            && lut_res != JSL__MAP_EMPTY\r\n"
        "            && lut_res != JSL__MAP_TOMBSTONE\r\n"
        "        );\r\n"
        "\r\n"
        "        struct JSL__StrToStrMapEntry* entry = occupied\r\n"
        "            ? (struct JSL__StrToStrMapEntry*) lut_res\r\n"
        "            : NULL;\r\n"
        "\r\n"
        "        int64_t probe_index = entry != NULL\r\n"
        "            ? (int64_t) (entry->hash & lut_mask)\r\n"
        "            : 0;\r\n"
        "\r\n"
        "        int64_t probes = 0;\r\n"
        "\r\n"
        "        bool insert_needed = entry != NULL;\r\n"
        "        while (migrate_ok && insert_needed && probes < new_length)\r\n"
        "        {\r\n"
        "            uintptr_t probe_res = new_table[probe_index];\r\n"
        "            bool slot_free = (\r\n"
        "                probe_res == JSL__MAP_EMPTY\r\n"
        "                || probe_res == JSL__MAP_TOMBSTONE\r\n"
        "            );\r\n"
        "\r\n"
        "            if (slot_free)\r\n"
        "            {\r\n"
        "                new_table[probe_index] = (uintptr_t) entry;\r\n"
        "                insert_needed = false;\r\n"
        "                break;\r\n"
        "            }\r\n"
        "\r\n"
        "            probe_index = (int64_t) (((uint64_t) probe_index + 1u) & lut_mask);\r\n"
        "            ++probes;\r\n"
        "        }\r\n"
        "\r\n"
        "        bool placement_failed = insert_needed;\r\n"
        "        if (placement_failed)\r\n"
        "        {\r\n"
        "            migrate_ok = false;\r\n"
        "        }\r\n"
        "\r\n"
        "        ++old_index;\r\n"
        "    }\r\n"
        "\r\n"
        "    bool should_commit = migrate_ok && new_table != NULL && length_valid;\r\n"
        "    if (should_commit)\r\n"
        "    {\r\n"
        "        map->entry_lookup_table = new_table;\r\n"
        "        map->entry_lookup_table_length = new_length;\r\n"
        "        map->tombstone_count = 0;\r\n"
        "        ++map->generational_id;\r\n"
        "        res = true;\r\n"
        "    }\r\n"
        "\r\n"
        "    bool failed = !should_commit;\r\n"
        "    if (failed)\r\n"
        "    {\r\n"
        "        res = false;\r\n"
        "    }\r\n"
        "\r\n"
        "    return res;\r\n"
        "}\r\n"
        "\r\n"
        "static JSL__FORCE_INLINE void jsl__str_to_str_map_update_value(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLFatPtr value,\r\n"
        "    JSLStringLifeTime value_lifetime,\r\n"
        "    int64_t lut_index\r\n"
        ")\r\n"
        "{\r\n"
        "    uintptr_t lut_res = map->entry_lookup_table[lut_index];\r\n"
        "    struct JSL__StrToStrMapEntry* entry = (struct JSL__StrToStrMapEntry*) lut_res;\r\n"
        "\r\n"
        "    if (value_lifetime == JSL_STRING_LIFETIME_LONGER)\r\n"
        "    {\r\n"
        "        entry->value = value;\r\n"
        "    }\r\n"
        "    else if (\r\n"
        "        value_lifetime == JSL_STRING_LIFETIME_SHORTER\r\n"
        "        && value.length <= JSL__MAP_SSO_LENGTH\r\n"
        "    )\r\n"
        "    {\r\n"
        "        JSL_MEMCPY(entry->value_sso_buffer, value.data, (size_t) value.length);\r\n"
        "        entry->value.data = entry->value_sso_buffer;\r\n"
        "        entry->value.length = value.length;\r\n"
        "    }\r\n"
        "    else if (\r\n"
        "        value_lifetime == JSL_STRING_LIFETIME_SHORTER\r\n"
        "        && value.length > JSL__MAP_SSO_LENGTH\r\n"
        "    )\r\n"
        "    {\r\n"
        "        entry->value = jsl_fatptr_duplicate(map->arena, value);\r\n"
        "    }\r\n"
        "}\r\n"
        "\r\n"
        "static JSL__FORCE_INLINE bool jsl__str_to_str_map_add(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLFatPtr key,\r\n"
        "    JSLStringLifeTime key_lifetime,\r\n"
        "    JSLFatPtr value,\r\n"
        "    JSLStringLifeTime value_lifetime,\r\n"
        "    int64_t lut_index,\r\n"
        "    uint64_t hash\r\n"
        ")\r\n"
        "{\r\n"
        "    struct JSL__StrToStrMapEntry* entry = NULL;\r\n"
        "    bool replacing_tombstone = map->entry_lookup_table[lut_index] == JSL__MAP_TOMBSTONE;\r\n"
        "\r\n"
        "    if (map->entry_free_list == NULL)\r\n"
        "    {\r\n"
        "        entry = JSL_ARENA_TYPED_ALLOCATE(struct JSL__StrToStrMapEntry, map->arena);\r\n"
        "    }\r\n"
        "    else\r\n"
        "    {\r\n"
        "        struct JSL__StrToStrMapEntry* next = map->entry_free_list->next;\r\n"
        "        entry = map->entry_free_list;\r\n"
        "        map->entry_free_list = next;\r\n"
        "    }\r\n"
        "\r\n"
        "    if (entry != NULL)\r\n"
        "    {\r\n"
        "        entry->hash = hash;\r\n"
        "        \r\n"
        "        map->entry_lookup_table[lut_index] = (uintptr_t) entry;\r\n"
        "        ++map->item_count;\r\n"
        "    }\r\n"
        "\r\n"
        "    if (entry != NULL && replacing_tombstone)\r\n"
        "    {\r\n"
        "        --map->tombstone_count;\r\n"
        "    }\r\n"
        "\r\n"
        "    // \r\n"
        "    // Copy the key\r\n"
        "    // \r\n"
        "\r\n"
        "    if (entry != NULL && key_lifetime == JSL_STRING_LIFETIME_LONGER)\r\n"
        "    {\r\n"
        "        entry->key = key;\r\n"
        "    }\r\n"
        "    else if (\r\n"
        "        entry != NULL\r\n"
        "        && key_lifetime == JSL_STRING_LIFETIME_SHORTER\r\n"
        "        && key.length <= JSL__MAP_SSO_LENGTH\r\n"
        "    )\r\n"
        "    {\r\n"
        "        JSL_MEMCPY(entry->key_sso_buffer, key.data, (size_t) key.length);\r\n"
        "        entry->key.data = entry->key_sso_buffer;\r\n"
        "        entry->key.length = key.length;\r\n"
        "    }\r\n"
        "    else if (\r\n"
        "        entry != NULL\r\n"
        "        && key_lifetime == JSL_STRING_LIFETIME_SHORTER\r\n"
        "        && key.length > JSL__MAP_SSO_LENGTH\r\n"
        "    )\r\n"
        "    {\r\n"
        "        entry->key = jsl_fatptr_duplicate(map->arena, key);\r\n"
        "    }\r\n"
        "\r\n"
        "    // \r\n"
        "    // Copy the value\r\n"
        "    // \r\n"
        "\r\n"
        "    if (entry != NULL && value_lifetime == JSL_STRING_LIFETIME_LONGER)\r\n"
        "    {\r\n"
        "        entry->value = value;\r\n"
        "    }\r\n"
        "    else if (\r\n"
        "        entry != NULL\r\n"
        "        && value_lifetime == JSL_STRING_LIFETIME_SHORTER\r\n"
        "        && value.length <= JSL__MAP_SSO_LENGTH\r\n"
        "    )\r\n"
        "    {\r\n"
        "        JSL_MEMCPY(entry->value_sso_buffer, value.data, (size_t) value.length);\r\n"
        "        entry->value.data = entry->value_sso_buffer;\r\n"
        "        entry->value.length = value.length;\r\n"
        "    }\r\n"
        "    else if (\r\n"
        "        entry != NULL\r\n"
        "        && value_lifetime == JSL_STRING_LIFETIME_SHORTER\r\n"
        "        && value.length > JSL__MAP_SSO_LENGTH\r\n"
        "    )\r\n"
        "    {\r\n"
        "        entry->value = jsl_fatptr_duplicate(map->arena, value);\r\n"
        "    }\r\n"
        "\r\n"
        "    return entry != NULL;\r\n"
        "}\r\n"
        "\r\n"
        "static inline void jsl__str_to_str_map_probe(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLFatPtr key,\r\n"
        "    int64_t* out_lut_index,\r\n"
        "    uint64_t* out_hash,\r\n"
        "    bool* out_found\r\n"
        ")\r\n"
        "{\r\n"
        "    *out_lut_index = -1;\r\n"
        "    *out_found = false;\r\n"
        "\r\n"
        "    int64_t first_tombstone = -1;\r\n"
        "    bool tombstone_seen = false;\r\n"
        "    bool searching = true;\r\n"
        "\r\n"
        "    *out_hash = jsl__rapidhash_withSeed(key.data, (size_t) key.length, map->hash_seed);\r\n"
        "\r\n"
        "    int64_t lut_length = map->entry_lookup_table_length;\r\n"
        "    uint64_t lut_mask = (uint64_t) lut_length - 1u;\r\n"
        "    int64_t lut_index = (int64_t) (*out_hash & lut_mask);\r\n"
        "    int64_t probes = 0;\r\n"
        "\r\n"
        "    while (searching && probes < lut_length)\r\n"
        "    {\r\n"
        "        uintptr_t lut_res = map->entry_lookup_table[lut_index];\r\n"
        "\r\n"
        "        bool is_empty = lut_res == JSL__MAP_EMPTY;\r\n"
        "        bool is_tombstone = lut_res == JSL__MAP_TOMBSTONE;\r\n"
        "\r\n"
        "        if (is_empty)\r\n"
        "        {\r\n"
        "            *out_lut_index = tombstone_seen ? first_tombstone : lut_index;\r\n"
        "            searching = false;\r\n"
        "        }\r\n"
        "\r\n"
        "        bool record_tombstone = searching && is_tombstone && !tombstone_seen;\r\n"
        "        if (record_tombstone)\r\n"
        "        {\r\n"
        "            first_tombstone = lut_index;\r\n"
        "            tombstone_seen = true;\r\n"
        "        }\r\n"
        "\r\n"
        "        bool slot_has_entry = searching && !is_empty && !is_tombstone;\r\n"
        "        struct JSL__StrToStrMapEntry* entry = slot_has_entry\r\n"
        "            ? (struct JSL__StrToStrMapEntry*) lut_res\r\n"
        "            : NULL;\r\n"
        "\r\n"
        "        bool matches = entry != NULL\r\n"
        "            && *out_hash == entry->hash\r\n"
        "            && jsl_fatptr_memory_compare(key, entry->key);\r\n"
        "\r\n"
        "        if (matches)\r\n"
        "        {\r\n"
        "            *out_found = true;\r\n"
        "            *out_lut_index = lut_index;\r\n"
        "            searching = false;\r\n"
        "        }\r\n"
        "\r\n"
        "        if (entry == NULL)\r\n"
        "        {\r\n"
        "            ++map->tombstone_count;\r\n"
        "            map->entry_lookup_table[lut_index] = JSL__MAP_TOMBSTONE;\r\n"
        "        }\r\n"
        "\r\n"
        "        if (entry == NULL && !tombstone_seen)\r\n"
        "        {\r\n"
        "            first_tombstone = lut_index;\r\n"
        "            tombstone_seen = true;\r\n"
        "        }\r\n"
        "\r\n"
        "        if (searching)\r\n"
        "        {\r\n"
        "            lut_index = (int64_t) (((uint64_t) lut_index + 1u) & lut_mask);\r\n"
        "            ++probes;\r\n"
        "        }\r\n"
        "    }\r\n"
        "\r\n"
        "    bool exhausted = searching && probes >= lut_length;\r\n"
        "    if (exhausted)\r\n"
        "    {\r\n"
        "        *out_lut_index = tombstone_seen ? first_tombstone : -1;\r\n"
        "    }\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_insert(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLFatPtr key,\r\n"
        "    JSLStringLifeTime key_lifetime,\r\n"
        "    JSLFatPtr value,\r\n"
        "    JSLStringLifeTime value_lifetime\r\n"
        ")\r\n"
        "{\r\n"
        "    bool res = (\r\n"
        "        map != NULL\r\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && key.data != NULL \r\n"
        "        && key.length > -1\r\n"
        "        && value.data != NULL\r\n"
        "        && value.length > -1\r\n"
        "    );\r\n"
        "\r\n"
        "    bool needs_rehash = false;\r\n"
        "    if (res)\r\n"
        "    {\r\n"
        "        float occupied_count = (float) (map->item_count + map->tombstone_count);\r\n"
        "        float current_load_factor =  occupied_count / (float) map->entry_lookup_table_length;\r\n"
        "        bool too_many_tombstones = map->tombstone_count > (map->entry_lookup_table_length / 4);\r\n"
        "        needs_rehash = current_load_factor >= map->load_factor || too_many_tombstones;\r\n"
        "    }\r\n"
        "\r\n"
        "    if (JSL__UNLIKELY(needs_rehash))\r\n"
        "    {\r\n"
        "        res = jsl__str_to_str_map_rehash(map);\r\n"
        "    }\r\n"
        "\r\n"
        "    uint64_t hash = 0;\r\n"
        "    int64_t lut_index = -1;\r\n"
        "    bool existing_found = false;\r\n"
        "    if (res)\r\n"
        "    {\r\n"
        "        jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);\r\n"
        "    }\r\n"
        "    \r\n"
        "    // new key\r\n"
        "    if (lut_index > -1 && !existing_found)\r\n"
        "    {\r\n"
        "        res = jsl__str_to_str_map_add(\r\n"
        "            map,\r\n"
        "            key, key_lifetime,\r\n"
        "            value, value_lifetime,\r\n"
        "            lut_index,\r\n"
        "            hash\r\n"
        "        );\r\n"
        "    }\r\n"
        "    // update\r\n"
        "    else if (lut_index > -1 && existing_found)\r\n"
        "    {\r\n"
        "        jsl__str_to_str_map_update_value(map, value, value_lifetime, lut_index);\r\n"
        "    }\r\n"
        "\r\n"
        "    if (res)\r\n"
        "    {\r\n"
        "        ++map->generational_id;\r\n"
        "    }\r\n"
        "\r\n"
        "    return res;\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_has_key(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLFatPtr key\r\n"
        ")\r\n"
        "{\r\n"
        "    uint64_t hash = 0;\r\n"
        "    int64_t lut_index = -1;\r\n"
        "    bool existing_found = false;\r\n"
        "\r\n"
        "    if (\r\n"
        "        map != NULL\r\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && key.data != NULL \r\n"
        "        && key.length > -1\r\n"
        "    )\r\n"
        "    {\r\n"
        "        jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);\r\n"
        "    }\r\n"
        "\r\n"
        "    return lut_index > -1 && existing_found;\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_get(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLFatPtr key,\r\n"
        "    JSLFatPtr* out_value\r\n"
        ")\r\n"
        "{\r\n"
        "    bool res = false;\r\n"
        "\r\n"
        "    bool params_valid = (\r\n"
        "        map != NULL\r\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && map->entry_lookup_table != NULL\r\n"
        "        && out_value != NULL\r\n"
        "        && key.data != NULL \r\n"
        "        && key.length > -1\r\n"
        "    );\r\n"
        "\r\n"
        "    uint64_t hash = 0;\r\n"
        "    int64_t lut_index = -1;\r\n"
        "    bool existing_found = false;\r\n"
        "\r\n"
        "    if (params_valid)\r\n"
        "    {\r\n"
        "        jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);\r\n"
        "    }\r\n"
        "\r\n"
        "    if (params_valid && existing_found && lut_index > -1)\r\n"
        "    {\r\n"
        "        struct JSL__StrToStrMapEntry* entry =\r\n"
        "            (struct JSL__StrToStrMapEntry*) map->entry_lookup_table[lut_index];\r\n"
        "        *out_value = entry->value;\r\n"
        "        res = true;\r\n"
        "    }\r\n"
        "    else if (out_value != NULL)\r\n"
        "    {\r\n"
        "        *out_value = (JSLFatPtr) {0};\r\n"
        "    }\r\n"
        "\r\n"
        "    return res;\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF int64_t jsl_str_to_str_map_item_count(\r\n"
        "    JSLStrToStrMap* map\r\n"
        ")\r\n"
        "{\r\n"
        "    int64_t res = -1;\r\n"
        "\r\n"
        "    if (\r\n"
        "        map != NULL\r\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "    )\r\n"
        "    {\r\n"
        "        res = map->item_count;\r\n"
        "    }\r\n"
        "\r\n"
        "    return res;\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_key_value_iterator_init(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLStrToStrMapKeyValueIter* iterator\r\n"
        ")\r\n"
        "{\r\n"
        "    bool res = false;\r\n"
        "\r\n"
        "    if (\r\n"
        "        map != NULL\r\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && iterator != NULL\r\n"
        "    )\r\n"
        "    {\r\n"
        "        iterator->map = map;\r\n"
        "        iterator->current_lut_index = 0;\r\n"
        "        iterator->sentinel = JSL__MAP_PRIVATE_SENTINEL;\r\n"
        "        iterator->generational_id = map->generational_id;\r\n"
        "        res = true;\r\n"
        "    }\r\n"
        "\r\n"
        "    return res;\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_key_value_iterator_next(\r\n"
        "    JSLStrToStrMapKeyValueIter* iterator,\r\n"
        "    JSLFatPtr* out_key,\r\n"
        "    JSLFatPtr* out_value\r\n"
        ")\r\n"
        "{\r\n"
        "    bool found = false;\r\n"
        "\r\n"
        "    bool params_valid = (\r\n"
        "        iterator != NULL\r\n"
        "        && out_key != NULL\r\n"
        "        && out_value != NULL\r\n"
        "        && iterator->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && iterator->map != NULL\r\n"
        "        && iterator->map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && iterator->map->entry_lookup_table != NULL\r\n"
        "        && iterator->generational_id == iterator->map->generational_id\r\n"
        "    );\r\n"
        "\r\n"
        "    int64_t lut_length = params_valid ? iterator->map->entry_lookup_table_length : 0;\r\n"
        "    int64_t lut_index = iterator->current_lut_index;\r\n"
        "    struct JSL__StrToStrMapEntry* found_entry = NULL;\r\n"
        "\r\n"
        "    while (params_valid && lut_index < lut_length)\r\n"
        "    {\r\n"
        "        uintptr_t lut_res = iterator->map->entry_lookup_table[lut_index];\r\n"
        "        bool occupied = lut_res != JSL__MAP_EMPTY && lut_res != JSL__MAP_TOMBSTONE;\r\n"
        "\r\n"
        "        if (occupied)\r\n"
        "        {\r\n"
        "            found_entry = (struct JSL__StrToStrMapEntry*) lut_res;\r\n"
        "            break;\r\n"
        "        }\r\n"
        "        else\r\n"
        "        {\r\n"
        "            ++lut_index;\r\n"
        "        }\r\n"
        "    }\r\n"
        "\r\n"
        "    if (found_entry != NULL)\r\n"
        "    {\r\n"
        "        iterator->current_lut_index = lut_index + 1;\r\n"
        "        *out_key = found_entry->key;\r\n"
        "        *out_value = found_entry->value;\r\n"
        "        found = true;\r\n"
        "    }\r\n"
        "\r\n"
        "    bool exhausted = params_valid && found_entry == NULL;\r\n"
        "    if (exhausted)\r\n"
        "    {\r\n"
        "        iterator->current_lut_index = lut_length;\r\n"
        "        found = false;\r\n"
        "    }\r\n"
        "\r\n"
        "    return found;\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF bool jsl_str_to_str_map_delete(\r\n"
        "    JSLStrToStrMap* map,\r\n"
        "    JSLFatPtr key\r\n"
        ")\r\n"
        "{\r\n"
        "    bool res = false;\r\n"
        "\r\n"
        "    bool params_valid = (\r\n"
        "        map != NULL\r\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && map->entry_lookup_table != NULL\r\n"
        "        && key.data != NULL\r\n"
        "        && key.length > -1\r\n"
        "    );\r\n"
        "\r\n"
        "    uint64_t hash = 0;\r\n"
        "    int64_t lut_index = -1;\r\n"
        "    bool existing_found = false;\r\n"
        "    if (params_valid)\r\n"
        "    {\r\n"
        "        jsl__str_to_str_map_probe(map, key, &lut_index, &hash, &existing_found);\r\n"
        "    }\r\n"
        "\r\n"
        "    if (existing_found && lut_index > -1)\r\n"
        "    {\r\n"
        "        struct JSL__StrToStrMapEntry* entry =\r\n"
        "            (struct JSL__StrToStrMapEntry*) map->entry_lookup_table[lut_index];\r\n"
        "\r\n"
        "        entry->next = map->entry_free_list;\r\n"
        "        map->entry_free_list = entry;\r\n"
        "\r\n"
        "        --map->item_count;\r\n"
        "        ++map->generational_id;\r\n"
        "\r\n"
        "        map->entry_lookup_table[lut_index] = JSL__MAP_TOMBSTONE;\r\n"
        "        ++map->tombstone_count;\r\n"
        "\r\n"
        "        res = true;\r\n"
        "    }\r\n"
        "\r\n"
        "    return res;\r\n"
        "}\r\n"
        "\r\n"
        "JSL_STR_TO_STR_MAP_DEF void jsl_str_to_str_map_clear(\r\n"
        "    JSLStrToStrMap* map\r\n"
        ")\r\n"
        "{\r\n"
        "    bool params_valid = (\r\n"
        "        map != NULL\r\n"
        "        && map->sentinel == JSL__MAP_PRIVATE_SENTINEL\r\n"
        "        && map->entry_lookup_table != NULL\r\n"
        "    );\r\n"
        "\r\n"
        "    int64_t lut_length = params_valid ? map->entry_lookup_table_length : 0;\r\n"
        "    int64_t index = 0;\r\n"
        "\r\n"
        "    while (params_valid && index < lut_length)\r\n"
        "    {\r\n"
        "        uintptr_t lut_res = map->entry_lookup_table[index];\r\n"
        "\r\n"
        "        if (lut_res != JSL__MAP_EMPTY && lut_res != JSL__MAP_TOMBSTONE)\r\n"
        "        {\r\n"
        "            struct JSL__StrToStrMapEntry* entry = (struct JSL__StrToStrMapEntry*) lut_res;\r\n"
        "            entry->next = map->entry_free_list;\r\n"
        "            map->entry_free_list = entry;\r\n"
        "            map->entry_lookup_table[index] = JSL__MAP_EMPTY;\r\n"
        "        }\r\n"
        "        else if (lut_res == JSL__MAP_TOMBSTONE)\r\n"
        "        {\r\n"
        "            map->entry_lookup_table[index] = JSL__MAP_EMPTY;\r\n"
        "        }\r\n"
        "\r\n"
        "        ++index;\r\n"
        "    }\r\n"
        "\r\n"
        "    if (params_valid)\r\n"
        "    {\r\n"
        "        map->item_count = 0;\r\n"
        "        map->tombstone_count = 0;\r\n"
        "        ++map->generational_id;\r\n"
        "    }\r\n"
        "\r\n"
        "    return;\r\n"
        "}\r\n"
        "\r\n"
        "#undef JSL__MAP_SSO_LENGTH\r\n"
        "#undef JSL__MAP_PRIVATE_SENTINEL\r\n"
    );

    static JSLImmutableMemory hash_map_name_key = JSL_CSTR_INITIALIZER("hash_map_name");
    static JSLImmutableMemory key_type_name_key = JSL_CSTR_INITIALIZER("key_type_name");
    static JSLImmutableMemory key_is_str_key = JSL_CSTR_INITIALIZER("key_is_str");
    static JSLImmutableMemory key_is_struct_key = JSL_CSTR_INITIALIZER("key_is_struct");
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

        bool key_is_struct = !key_is_str
            && key_type_name.data != NULL
            && key_type_name.data[key_type_name.length - 1] != '*'
            && !(jsl_memory_compare(key_type_name, int32_t_str)
                || jsl_memory_compare(key_type_name, int_str)
                || jsl_memory_compare(key_type_name, unsigned_str)
                || jsl_memory_compare(key_type_name, unsigned_int_str)
                || jsl_memory_compare(key_type_name, uint32_t_str)
                || jsl_memory_compare(key_type_name, int64_t_str)
                || jsl_memory_compare(key_type_name, long_str)
                || jsl_memory_compare(key_type_name, long_int_str)
                || jsl_memory_compare(key_type_name, long_long_str)
                || jsl_memory_compare(key_type_name, long_long_int_str)
                || jsl_memory_compare(key_type_name, uint64_t_str)
                || jsl_memory_compare(key_type_name, unsigned_long_str)
                || jsl_memory_compare(key_type_name, unsigned_long_long_str)
                || jsl_memory_compare(key_type_name, unsigned_long_long_int_str));

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

        if (key_is_struct)
            jsl_str_to_str_map_insert(
                &map,
                key_is_struct_key,
                JSL_STRING_LIFETIME_LONGER,
                JSL_CSTR_EXPRESSION(""),
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
        JSLImmutableMemory compare_function_name,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    )
    {
        (void) impl;

        bool key_is_struct = !key_is_str
            && key_type_name.data != NULL
            && key_type_name.data[key_type_name.length - 1] != '*'
            && !(jsl_memory_compare(key_type_name, int32_t_str)
                || jsl_memory_compare(key_type_name, int_str)
                || jsl_memory_compare(key_type_name, unsigned_str)
                || jsl_memory_compare(key_type_name, unsigned_int_str)
                || jsl_memory_compare(key_type_name, uint32_t_str)
                || jsl_memory_compare(key_type_name, int64_t_str)
                || jsl_memory_compare(key_type_name, long_str)
                || jsl_memory_compare(key_type_name, long_int_str)
                || jsl_memory_compare(key_type_name, long_long_str)
                || jsl_memory_compare(key_type_name, long_long_int_str)
                || jsl_memory_compare(key_type_name, uint64_t_str)
                || jsl_memory_compare(key_type_name, unsigned_long_str)
                || jsl_memory_compare(key_type_name, unsigned_long_long_str)
                || jsl_memory_compare(key_type_name, unsigned_long_long_int_str));

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

        if (key_is_struct)
            jsl_str_to_str_map_insert(
                &map,
                key_is_struct_key,
                JSL_STRING_LIFETIME_LONGER,
                JSL_CSTR_EXPRESSION(""),
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
            if (key_is_struct && hash_function_name.data != NULL && hash_function_name.length > 0)
            {
                // Struct keys: custom hash signature is (const TYPE* key, uint64_t seed)
                resolved_hash_function_call = jsl_format(
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("*out_hash = %y(key, hash_map->seed)"),
                    hash_function_name
                );
            }
            else if (key_is_str)
            {
                resolved_hash_function_call = JSL_CSTR_EXPRESSION("*out_hash = jsl__rapidhash_withSeed(key.data, (size_t) key.length, hash_map->seed)");
            }
            else if (!key_is_struct
                && key_type_name.data != NULL
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
                // Struct key (no custom hash): key is already const TYPE*, no & needed
                resolved_hash_function_call = jsl_format(
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("*out_hash = jsl__rapidhash_withSeed(key, sizeof(%y), hash_map->seed)"),
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
                !key_is_struct
                && key_type_name.data != NULL
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
            else if (key_is_struct && compare_function_name.data != NULL && compare_function_name.length > 0)
            {
                // Struct key with custom compare: fn(const TYPE* a, const TYPE* b) -> bool
                resolved_key_compare = jsl_format(
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("%y(key, &hash_map->keys_array[slot])"),
                    compare_function_name
                );
            }
            else
            {
                // Struct key (no custom compare): key is already const TYPE*, no & needed
                resolved_key_compare = jsl_format(
                    scratch_interface,
                    JSL_CSTR_EXPRESSION("JSL_MEMCMP(key, &hash_map->keys_array[slot], sizeof(%y)) == 0"),
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

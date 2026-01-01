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
 * #define GENERATE_ARRAY_IMPLEMENTATION
 * #include "generate_hash_map.h"
 * ```
 * 
 * The two relevent functions are write_array_header and write_array_source
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

#ifndef GENERATE_ARRAY_H_INCLUDED
    #define GENERATE_ARRAY_H_INCLUDED

    #include <stdint.h>
    #include <stddef.h>
    #include <inttypes.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "../src/jsl_core.h"

    /* Versioning to catch mismatches across deps */
    #ifndef GENERATE_ARRAY_VERSION
        #define GENERATE_ARRAY_VERSION 0x010000  /* 1.0.0 */
    #else
        #if GENERATE_ARRAY_VERSION != 0x010000
            #error "generate_hash_map.h version mismatch across includes"
        #endif
    #endif

    #ifndef GENERATE_ARRAY_DEF
        #define GENERATE_ARRAY_DEF
    #endif

    #ifdef __cplusplus
    extern "C" {
    #endif

    typedef enum {
        IMPL_ERROR,
        IMPL_FIXED,
        IMPL_DYNAMIC
    } ArrayImplementation;

    /**
     * Generate the text of the C header and insert it into the string builder.
     * 
     * @param arena Used for all memory allocations
     * @param builder Used to insert the generated text
     * @param impl Which hash map implementation to use
     * @param array_type_name The name of the container type
     * @param function_prefix The prefix plus "_" for each function
     * @param value_type_name The type of the hash map value
     * @param include_header_array If you need custom header includes then set this, otherwise pass NULL
     * @param include_header_count The length of the header array
     */
    GENERATE_ARRAY_DEF void write_array_header(
        JSLArena* arena,
        JSLStringBuilder* builder,
        ArrayImplementation impl,
        JSLFatPtr array_type_name,
        JSLFatPtr function_prefix,
        JSLFatPtr value_type_name,
        JSLFatPtr* include_header_array,
        int32_t include_header_count
    );

    /**
     * Generate the text of the C source and insert it into the string builder.
     * 
     * @param arena Used for all memory allocations
     * @param builder Used to insert the generated text
     * @param impl Which implementation to use
     * @param array_type_name The name of the container type
     * @param function_prefix The prefix plus "_" for each function
     * @param value_type_name The type of the hash map value
     * @param include_header_array If you need custom header includes then set this, otherwise pass NULL
     * @param include_header_count The length of the header array
     */
    GENERATE_ARRAY_DEF void write_array_source(
        JSLArena* arena,
        JSLStringBuilder* builder,
        ArrayImplementation impl,
        JSLFatPtr array_type_name,
        JSLFatPtr function_prefix,
        JSLFatPtr value_type_name,
        JSLFatPtr* include_header_array,
        int32_t include_header_count
    );
    
    #ifdef __cplusplus
    }
    #endif
    
#endif /* GENERATE_ARRAY_H_INCLUDED */

#ifdef GENERATE_ARRAY_IMPLEMENTATION

    #include <stdlib.h>
    #include <time.h>
    #include <assert.h>

    #include "../tools/templates/dynamic_array_header.h"
    #include "../tools/templates/dynamic_array_source.h"

    static JSLFatPtr array_type_name_key = JSL_FATPTR_INITIALIZER("array_type_name");
    static JSLFatPtr value_type_name_key = JSL_FATPTR_INITIALIZER("value_type_name");
    static JSLFatPtr function_prefix_key = JSL_FATPTR_INITIALIZER("function_prefix");

    // because rand max on some platforms is 32k
    static inline uint64_t rand_u64(void)
    {
        uint64_t value = 0;

        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);
        value = (value << 8) | (uint64_t)(rand() & 0xFF);

        return value;
    }

    static void render_template(
        JSLStringBuilder* str_builder,
        JSLFatPtr template,
        JSLStrToStrMap* variables
    )
    {
        static JSLFatPtr open_param = JSL_FATPTR_INITIALIZER("{{");
        static JSLFatPtr close_param = JSL_FATPTR_INITIALIZER("}}");
        JSLFatPtr template_reader = template;
        
        while (template_reader.length > 0)
        {
            int64_t index_of_open = jsl_fatptr_substring_search(template_reader, open_param);

            // No more variables, write everything
            if (index_of_open == -1)
            {
                jsl_string_builder_insert_fatptr(str_builder, template_reader);
                break;
            }

            if (index_of_open > 0)
            {
                JSLFatPtr slice = jsl_fatptr_slice(template_reader, 0, index_of_open);
                jsl_string_builder_insert_fatptr(str_builder, slice);
            }

            JSL_FATPTR_ADVANCE(template_reader, index_of_open + open_param.length);

            int64_t index_of_close = jsl_fatptr_substring_search(template_reader, close_param);

            // Improperly closed template param, write everything including the open marker
            if (index_of_close == -1)
            {
                jsl_string_builder_insert_fatptr(str_builder, open_param);
                jsl_string_builder_insert_fatptr(str_builder, template_reader);
                break;
            }

            JSLFatPtr var_name = jsl_fatptr_slice(template_reader, 0, index_of_close);
            jsl_fatptr_strip_whitespace(&var_name);

            JSLFatPtr var_value;
            if (jsl_str_to_str_map_get(variables, var_name, &var_value))
            {
                jsl_string_builder_insert_fatptr(str_builder, var_value);
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

    * @return JSLFatPtr containing the generated header file content
    *
    * @warning Ensure the arena has sufficient space (minimum 512KB recommended) to avoid
    *          allocation failures during header generation.
    */
    GENERATE_ARRAY_DEF void write_array_header(
        JSLArena* arena,
        JSLStringBuilder* builder,
        ArrayImplementation impl,
        JSLFatPtr array_type_name,
        JSLFatPtr function_prefix,
        JSLFatPtr value_type_name,
        JSLFatPtr* include_header_array,
        int32_t include_header_count
    )
    {
        (void) impl;
        srand((uint32_t) (time(NULL) % UINT32_MAX));

        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#pragma once\n\n"));

        jsl_string_builder_insert_fatptr(
            builder,
            JSL_FATPTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
        );
        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#include <stdint.h>\n"));
        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("#include \"jsl_hash_map_common.h\"\n\n"));

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
            JSL_FATPTR_EXPRESSION("#define PRIVATE_SENTINEL_%y %" PRIu64 "U \n"),
            array_type_name,
            rand_u64()
        );

        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("\n"));

        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, arena, 0x123456789);

        jsl_str_to_str_map_insert(
            &map,
            array_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            array_type_name,
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

        // if (impl == IMPL_FIXED)
        //     render_template(builder, fixed_header_template, &map);
        // else if (impl == IMPL_DYNAMIC)
            render_template(builder, dynamic_header_template, &map);
        // else
        //     assert(0);
    }

    GENERATE_ARRAY_DEF void write_array_source(
        JSLArena* arena,
        JSLStringBuilder* builder,
        ArrayImplementation impl,
        JSLFatPtr array_type_name,
        JSLFatPtr function_prefix,
        JSLFatPtr value_type_name,
        JSLFatPtr* include_header_array,
        int32_t include_header_count
    )
    {
        (void) impl;
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
            JSL_FATPTR_EXPRESSION("// USER INCLUDED HEADERS\n")
        );

        for (int32_t i = 0; i < include_header_count; ++i)
        {
            jsl_string_builder_format(builder, JSL_FATPTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
        }

        jsl_string_builder_insert_fatptr(builder, JSL_FATPTR_EXPRESSION("\n"));

        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, arena, 0x123456789);

        jsl_str_to_str_map_insert(
            &map,
            array_type_name_key,
            JSL_STRING_LIFETIME_STATIC,
            array_type_name,
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

        // if (impl == IMPL_FIXED)
        //     render_template(builder, fixed_header_template, &map);
        // else if (impl == IMPL_DYNAMIC)
            render_template(builder, dynamic_source_template, &map);
        // else
        //     assert(0);
    }

#endif /* GENERATE_ARRAY_IMPLEMENTATION */

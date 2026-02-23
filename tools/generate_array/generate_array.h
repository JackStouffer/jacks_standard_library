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

#ifndef GENERATE_ARRAY_H_INCLUDED
    #define GENERATE_ARRAY_H_INCLUDED

    #include <stdint.h>
    #include <stddef.h>
    #include <inttypes.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "jsl/core.h"
    #include "jsl/allocator.h"

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
     * Generate the text of the C header and insert it into the string sink.
     * 
     * @param arena Used for all memory allocations
     * @param sink Used to insert the generated text
     * @param impl Which hash map implementation to use
     * @param array_type_name The name of the container type
     * @param function_prefix The prefix plus "_" for each function
     * @param value_type_name The type of the hash map value
     * @param include_header_array If you need custom header includes then set this, otherwise pass NULL
     * @param include_header_count The length of the header array
     */
    GENERATE_ARRAY_DEF void write_array_header(
        JSLAllocatorInterface* allocator,
        JSLOutputSink sink,
        ArrayImplementation impl,
        JSLImmutableMemory array_type_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory value_type_name,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    );

    /**
     * Generate the text of the C source and insert it into the string sink.
     * 
     * @param arena Used for all memory allocations
     * @param sink Used to insert the generated text
     * @param impl Which implementation to use
     * @param array_type_name The name of the container type
     * @param function_prefix The prefix plus "_" for each function
     * @param value_type_name The type of the hash map value
     * @param include_header_array If you need custom header includes then set this, otherwise pass NULL
     * @param include_header_count The length of the header array
     */
    GENERATE_ARRAY_DEF void write_array_source(
        JSLAllocatorInterface* allocator,
        JSLOutputSink sink,
        ArrayImplementation impl,
        JSLImmutableMemory array_type_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory value_type_name,
        JSLImmutableMemory* include_header_array,
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

    static JSLImmutableMemory dynamic_header_template = JSL_CSTR_INITIALIZER(
        "/**\n"
        " * AUTO GENERATED FILE\n"
        " *\n"
        " * This file contains the header for a dynamic array `{{ array_type_name }}` of\n"
        " * `{{ value_type_name }}` values.\n"
        " *\n"
        " * This file was auto generated from the array code generation utility that's part of\n"
        " * the \"Jack's Standard Library\" project. The utility generates a header file and a\n"
        " * C file for a type safe dynamic array . By generating the code rather than using macros,\n"
        " * two benefits are gained. One, the code is much easier to debug. Two, it's much more\n"
        " * obvious how much code you're generating, which means you are much less likely to accidentally\n"
        " * create the combinatoric explosion of code that's so common in C++ projects. Adding friction \n"
        " * to things is actually good sometimes.\n"
        " */\n"
        "\n"
        "\n"
        "#pragma once\n"
        "\n"
        "#include <stdint.h>\n"
        "#include <stddef.h>\n"
        "#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L\n"
        "    #include <stdbool.h>\n"
        "#endif\n"
        "\n"
        "#include \"jsl/core.h\"\n"
        "#include \"jsl/allocator.h\"\n"
        "\n"
        "#ifdef __cplusplus\n"
        "extern \"C\" {\n"
        "#endif\n"
        "\n"
        "/**\n"
        " * Dynamic array of {{ value_type_name }}.\n"
        " * \n"
        " * Example:\n"
        " *\n"
        " * ```\n"
        " * {{ array_type_name }} array;\n"
        " * {{ function_prefix }}_init(&array, &arena);\n"
        " *\n"
        " * {{ function_prefix }}_insert(&array, ... );\n"
        " *\n"
        " * for (int64_t i = 0; i < array.length; ++i)\n"
        " * {\n"
        " *      {{ value_type_name }}* value = &array.data[i];\n"
        " *      ...\n"
        " * }\n"
        " * ```\n"
        " * \n"
        " * ## Functions\n"
        " *\n"
        " *  * {{ function_prefix }}_init\n"
        " *  * {{ function_prefix }}_insert\n"
        " *  * {{ function_prefix }}_insert_at\n"
        " *  * {{ function_prefix }}_delete_at\n"
        " *  * {{ function_prefix }}_clear\n"
        " *\n"
        " */\n"
        "typedef struct {{ array_type_name }} {\n"
        "    // putting the sentinel first means it's much more likely to get\n"
        "    // corrupted from accidental overwrites, therefore making it\n"
        "    // more likely that memory bugs are caught.\n"
        "    uint64_t sentinel;\n"
        "    JSLAllocatorInterface* allocator;\n"
        "    {{ value_type_name }}* data;\n"
        "    int64_t length;\n"
        "    int64_t capacity;\n"
        "} {{ array_type_name }};\n"
        "\n"
        "/**\n"
        " * Initialize an instance of {{ array_type_name }}. Enough room will be allocated\n"
        " * for `initial_capacity` elements.\n"
        " *\n"
        " * @param array The pointer to the array instance to initialize\n"
        " * @param arena The arena that this array will use to allocate memory\n"
        " * @param initial_capacity Allocate enough space to hold this many elements \n"
        " * @returns If the allocation succeed\n"
        " */\n"
        "bool {{ function_prefix }}_init(\n"
        "    {{ array_type_name }}* array,\n"
        "    JSLAllocatorInterface* allocator,\n"
        "    int64_t initial_capacity\n"
        ");\n"
        "\n"
        "/**\n"
        " * Insert an `{{ value_type_name }}` at the end of the array.\n"
        " *\n"
        " * @param array The pointer to the array\n"
        " * @param value The value to add\n"
        " * @returns If the insertion succeed\n"
        " */\n"
        "bool {{ function_prefix }}_insert(\n"
        "    {{ array_type_name }}* array,\n"
        "    {{ value_type_name }} value\n"
        ");\n"
        "\n"
        "/**\n"
        " * Insert multiple `{{ value_type_name }}` at once at the end of the array.\n"
        " *\n"
        " * @param array The pointer to the array\n"
        " * @param value The pointer to the start of the values\n"
        " * @returns If the insertion succeed\n"
        " */\n"
        "bool {{ function_prefix }}_insert_multiple(\n"
        "    {{ array_type_name }}* array,\n"
        "    {{ value_type_name }}* values,\n"
        "    int64_t value_count\n"
        ");\n"
        "\n"
        "/**\n"
        " * Insert an `{{ value_type_name }}` at the specified index, moving everything after\n"
        " * that index to its index plus one.\n"
        " *\n"
        " * @param array The pointer to the array\n"
        " * @param value The value to add\n"
        " * @param index The index to place the element\n"
        " * @returns If the insertion succeed\n"
        " */\n"
        "bool {{ function_prefix }}_insert_at(\n"
        "    {{ array_type_name }}* array,\n"
        "    {{ value_type_name }} value,\n"
        "    int64_t index\n"
        ");\n"
        "\n"
        "/**\n"
        " * Delete the element at the specified index, moving everything after\n"
        " * that index to its index minus one.\n"
        " *\n"
        " * @param array The pointer to the array\n"
        " * @param index The index to delete\n"
        " * @returns if deletion succeed\n"
        " */\n"
        "bool {{ function_prefix }}_delete_at(\n"
        "    {{ array_type_name }}* array,\n"
        "    int64_t index\n"
        ");\n"
        "\n"
        "/**\n"
        " * Set the length of the array back to zero. Does not shrink the underlying capacity.\n"
        " *\n"
        " * @param array The pointer to the array\n"
        " */\n"
        "void {{ function_prefix }}_clear(\n"
        "    {{ array_type_name }}* array\n"
        ");\n"
        "\n"
        "/**\n"
        " * Free the underlying memory of the array. This sets the array into an invalid state.\n"
        " * You will have to call init again if you wish to use this array instance.\n"
        " *\n"
        " * @param array The pointer to the array\n"
        " */\n"
        "void {{ function_prefix }}_free(\n"
        "    {{ array_type_name }}* array\n"
        ");\n"
        "\n"
        "#ifdef __cplusplus\n"
        "}\n"
        "#endif\n"
    );

    static JSLImmutableMemory dynamic_source_template = JSL_CSTR_INITIALIZER(
        "/**\n"
        " * AUTO GENERATED FILE\n"
        " *\n"
        " * This file contains the header for a dynamic array `{{ array_type_name }}` of\n"
        " * `{{ value_type_name }}` values.\n"
        " *\n"
        " * This file was auto generated from the array code generation utility that's part of\n"
        " * the \"Jack's Standard Library\" project. The utility generates a header file and a\n"
        " * C file for a type safe dynamic array . By generating the code rather than using macros,\n"
        " * two benefits are gained. One, the code is much easier to debug. Two, it's much more\n"
        " * obvious how much code you're generating, which means you are much less likely to accidentally\n"
        " * create the combinatoric explosion of code that's so common in C++ projects. Adding friction \n"
        " * to things is actually good sometimes.\n"
        " */\n"
        "\n"
        "\n"
        "#include <stdint.h>\n"
        "#include <stddef.h>\n"
        "#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L\n"
        "    #include <stdbool.h>\n"
        "#endif\n"
        "#include <string.h>\n"
        "\n"
        "#include \"jsl/core.h\"\n"
        "#include \"jsl/allocator.h\"\n"
        "\n"
        "static inline bool {{ function_prefix }}__ensure_capacity(\n"
        "    {{ array_type_name }}* array,\n"
        "    int64_t needed_capacity\n"
        ")\n"
        "{\n"
        "    if (JSL__LIKELY(needed_capacity <= array->capacity))\n"
        "        return true;\n"
        "\n"
        "    bool res = false;\n"
        "    int64_t target_capacity = jsl_next_power_of_two_i64(needed_capacity);\n"
        "    int64_t new_bytes = ((int64_t) sizeof({{ value_type_name }})) * target_capacity;\n"
        "\n"
        "    void* new_mem = NULL;\n"
        "\n"
        "    if (array->data != NULL && array->capacity > 0)\n"
        "    {\n"
        "        new_mem = jsl_allocator_interface_realloc(\n"
        "            array->allocator,\n"
        "            array->data,\n"
        "            new_bytes,\n"
        "            _Alignof({{ value_type_name }})\n"
        "        );\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        new_mem = jsl_allocator_interface_alloc(\n"
        "            array->allocator,\n"
        "            new_bytes,\n"
        "            _Alignof({{ value_type_name }}),\n"
        "            false\n"
        "        );\n"
        "    }\n"
        "\n"
        "    if (new_mem != NULL)\n"
        "    {\n"
        "        array->data = ({{ value_type_name }}*) new_mem;\n"
        "        array->capacity = target_capacity;\n"
        "        res = true;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_init(\n"
        "    {{ array_type_name }}* array,\n"
        "    JSLAllocatorInterface* allocator,\n"
        "    int64_t initial_capacity\n"
        ")\n"
        "{\n"
        "    bool res = array != NULL && allocator != NULL && initial_capacity > -1;\n"
        "\n"
        "    if (res)\n"
        "    {\n"
        "        JSL_MEMSET(array, 0, sizeof({{ array_type_name }}));\n"
        "        array->allocator = allocator;\n"
        "        array->sentinel = PRIVATE_SENTINEL_{{ array_type_name }};\n"
        "\n"
        "        int64_t target_capacity = jsl_next_power_of_two_i64(JSL_MAX(32L, initial_capacity));\n"
        "        res = {{ function_prefix }}__ensure_capacity(array, target_capacity);\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_insert(\n"
        "    {{ array_type_name }}* array,\n"
        "    {{ value_type_name }} value\n"
        ")\n"
        "{\n"
        "    bool res = (\n"
        "        array != NULL\n"
        "        && array->sentinel == PRIVATE_SENTINEL_{{ array_type_name }}\n"
        "    );\n"
        "\n"
        "    if (res)\n"
        "    {\n"
        "        res = {{ function_prefix }}__ensure_capacity(array, array->length + 1);\n"
        "    }\n"
        "\n"
        "    if (res)\n"
        "    {\n"
        "        array->data[array->length] = value;\n"
        "        ++array->length;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_insert_multiple(\n"
        "    {{ array_type_name }}* array,\n"
        "    {{ value_type_name }}* values,\n"
        "    int64_t value_count\n"
        ")\n"
        "{\n"
        "    bool res = (\n"
        "        array != NULL\n"
        "        && array->sentinel == PRIVATE_SENTINEL_{{ array_type_name }}\n"
        "    );\n"
        "\n"
        "    if (res)\n"
        "    {\n"
        "        res = {{ function_prefix }}__ensure_capacity(array, array->length + value_count);\n"
        "    }\n"
        "\n"
        "    if (res)\n"
        "    {\n"
        "        for (int64_t i = 0; i < value_count; ++i)\n"
        "        {\n"
        "            array->data[array->length] = values[i];\n"
        "            ++array->length;    \n"
        "        }\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_insert_at(\n"
        "    {{ array_type_name }}* array,\n"
        "    {{ value_type_name }} value,\n"
        "    int64_t index\n"
        ")\n"
        "{\n"
        "    bool res = (\n"
        "        array != NULL\n"
        "        && array->sentinel == PRIVATE_SENTINEL_{{ array_type_name }}\n"
        "        && index > -1\n"
        "        && index <= array->length\n"
        "    );\n"
        "\n"
        "    if (res)\n"
        "        res = {{ function_prefix }}__ensure_capacity(array, array->length + 1);\n"
        "\n"
        "    int64_t items_to_move = res ? array->length - index : -1;\n"
        "\n"
        "    if (items_to_move > 0)\n"
        "    {\n"
        "        size_t move_bytes = (size_t) items_to_move * sizeof({{ value_type_name }});\n"
        "        JSL_MEMMOVE(\n"
        "            array->data + index + 1,\n"
        "            array->data + index,\n"
        "            move_bytes\n"
        "        );\n"
        "\n"
        "        array->data[index] = value;\n"
        "        ++array->length;\n"
        "    }\n"
        "    else if (items_to_move == 0)\n"
        "    {\n"
        "        array->data[array->length] = value;\n"
        "        ++array->length;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "bool {{ function_prefix }}_delete_at(\n"
        "    {{ array_type_name }}* array,\n"
        "    int64_t index\n"
        ")\n"
        "{\n"
        "    bool res = (\n"
        "        array != NULL\n"
        "        && array->sentinel == PRIVATE_SENTINEL_{{ array_type_name }}\n"
        "        && index > -1\n"
        "        && index < array->length\n"
        "    );\n"
        "\n"
        "    int64_t items_to_move = res ? array->length - index - 1 : -1;\n"
        "\n"
        "    if (items_to_move > 0)\n"
        "    {\n"
        "        size_t move_bytes = (size_t) items_to_move * sizeof({{ value_type_name }});\n"
        "        JSL_MEMMOVE(\n"
        "            array->data + index,\n"
        "            array->data + index + 1,\n"
        "            move_bytes\n"
        "        );\n"
        "        --array->length;\n"
        "    }\n"
        "    else if (items_to_move == 0)\n"
        "    {\n"
        "        --array->length;\n"
        "    }\n"
        "\n"
        "    return res;\n"
        "}\n"
        "\n"
        "void {{ function_prefix }}_clear(\n"
        "    {{ array_type_name }}* array\n"
        ")\n"
        "{\n"
        "    if (\n"
        "        array != NULL\n"
        "        && array->sentinel == PRIVATE_SENTINEL_{{ array_type_name }}\n"
        "    )\n"
        "    {\n"
        "        array->length = 0;\n"
        "    }\n"
        "}\n"
        "\n"
        "void {{ function_prefix }}_free(\n"
        "    {{ array_type_name }}* array\n"
        ")\n"
        "{\n"
        "    if (\n"
        "        array != NULL\n"
        "        && array->sentinel == PRIVATE_SENTINEL_{{ array_type_name }}\n"
        "    )\n"
        "    {\n"
        "        jsl_allocator_interface_free(\n"
        "            array->allocator,\n"
        "            array->data\n"
        "        );\n"
        "        array->length = 0;\n"
        "        array->capacity = 0;\n"
        "        array->sentinel = 0;\n"
        "    }\n"
        "}\n"
    );

    static JSLImmutableMemory array_type_name_key = JSL_CSTR_INITIALIZER("array_type_name");
    static JSLImmutableMemory value_type_name_key = JSL_CSTR_INITIALIZER("value_type_name");
    static JSLImmutableMemory function_prefix_key = JSL_CSTR_INITIALIZER("function_prefix");

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
        JSLOutputSink sink,
        JSLImmutableMemory template,
        JSLStrToStrMap* variables
    )
    {
        static JSLImmutableMemory open_param = JSL_CSTR_INITIALIZER("{{");
        static JSLImmutableMemory close_param = JSL_CSTR_INITIALIZER("}}");
        JSLImmutableMemory template_reader = template;
        
        while (template_reader.length > 0)
        {
            int64_t index_of_open = jsl_substring_search(template_reader, open_param);

            // No more variables, write everything
            if (index_of_open == -1)
            {
                jsl_output_sink_write(sink, template_reader);
                break;
            }

            if (index_of_open > 0)
            {
                JSLImmutableMemory slice = jsl_slice(template_reader, 0, index_of_open);
                jsl_output_sink_write(sink, slice);
            }

            JSL_MEMORY_ADVANCE(template_reader, index_of_open + open_param.length);

            int64_t index_of_close = jsl_substring_search(template_reader, close_param);

            // Improperly closed template param, write everything including the open marker
            if (index_of_close == -1)
            {
                jsl_output_sink_write(sink, open_param);
                jsl_output_sink_write(sink, template_reader);
                break;
            }

            JSLImmutableMemory var_name = jsl_slice(template_reader, 0, index_of_close);
            jsl_strip_whitespace(&var_name);

            JSLImmutableMemory var_value;
            if (jsl_str_to_str_map_get(variables, var_name, &var_value))
            {
                jsl_output_sink_write(sink, var_value);
            }

            JSL_MEMORY_ADVANCE(template_reader, index_of_close + close_param.length);
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
    GENERATE_ARRAY_DEF void write_array_header(
        JSLAllocatorInterface* allocator,
        JSLOutputSink sink,
        ArrayImplementation impl,
        JSLImmutableMemory array_type_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory value_type_name,
        JSLImmutableMemory* include_header_array,
        int32_t include_header_count
    )
    {
        (void) impl;
        srand((uint32_t) (time(NULL) % UINT32_MAX));

        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("#pragma once\n\n"));

        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("// DEFAULT INCLUDED HEADERS\n")
        );
        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("#include <stdint.h>\n"));
        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("#include \"jsl/hash_map_common.h\"\n\n"));

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
            JSL_CSTR_EXPRESSION("#define PRIVATE_SENTINEL_%y %" PRIu64 "U \n"),
            array_type_name,
            rand_u64()
        );

        jsl_output_sink_write(sink, JSL_CSTR_EXPRESSION("\n"));

        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, allocator, 0x123456789);

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
        //     render_template(sink, fixed_header_template, &map);
        // else if (impl == IMPL_DYNAMIC)
            render_template(sink, dynamic_header_template, &map);
        // else
        //     assert(0);
    }

    GENERATE_ARRAY_DEF void write_array_source(
        JSLAllocatorInterface* allocator,
        JSLOutputSink sink,
        ArrayImplementation impl,
        JSLImmutableMemory array_type_name,
        JSLImmutableMemory function_prefix,
        JSLImmutableMemory value_type_name,
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

        jsl_output_sink_write(
            sink,
            JSL_CSTR_EXPRESSION("// USER INCLUDED HEADERS\n")
        );

        for (int32_t i = 0; i < include_header_count; ++i)
        {
            jsl_format_sink(sink, JSL_CSTR_EXPRESSION("#include \"%y\"\n"), include_header_array[i]);
        }

        jsl_format_sink(sink, JSL_CSTR_EXPRESSION("\n"));

        JSLStrToStrMap map;
        jsl_str_to_str_map_init(&map, allocator, 0x123456789);

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
        //     render_template(sink, fixed_header_template, &map);
        // else if (impl == IMPL_DYNAMIC)
            render_template(sink, dynamic_source_template, &map);
        // else
        //     assert(0);
    }

#endif /* GENERATE_ARRAY_IMPLEMENTATION */

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

#include "../src/jsl_core.h"
#include "../src/jsl_string_builder.h"
#include "../src/jsl_os.h"

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
        fixed_hash_map_docstring,
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
        fixed_map_type_typedef,
        key_type_name,
        value_type_name,
        hash_map_name,
        key_type_name,
        value_type_name,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        fixed_map_iterator_typedef,
        hash_map_name,
        hash_map_name,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        fixed_init_function_signature,
        function_prefix,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        fixed_insert_function_signature,
        function_prefix,
        hash_map_name,
        key_type_name,
        value_type_name
    );

    jsl_string_builder_format(
        builder,
        fixed_get_function_signature,
        value_type_name,
        function_prefix,
        hash_map_name,
        key_type_name
    );

    jsl_string_builder_format(
        builder,
        fixed_delete_function_signature,
        function_prefix,
        hash_map_name,
        key_type_name
    );

    jsl_string_builder_format(
        builder,
        fixed_iterator_start_function_signature,
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
        fixed_iterator_next_function_signature,
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
        fixed_hash_map_docstring,
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
        fixed_init_function_code,
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
                JSL_FATPTR_EXPRESSION("*out_hash = murmur3_fmix_u64((uint64_t) key, hash_map->seed)"),
                key_type_name
            );
        }
        else
        {
            resolved_hash_function_call = jsl_format(
                &hash_function_scratch_arena,
                JSL_FATPTR_EXPRESSION("*out_hash = jsl__rapidhash_withSeed(&key, sizeof(%y), hash_map->seed)"),
                key_type_name
            );
        }

        jsl_string_builder_format(
            builder,
            fixed_hash_function_code,
            function_prefix,
            hash_map_name,
            key_type_name,
            resolved_hash_function_call,
            key_type_name
        );
    }

    jsl_string_builder_format(
        builder,
        fixed_insert_function_code,
        function_prefix,
        hash_map_name,
        key_type_name,
        value_type_name,
        hash_map_name,
        function_prefix
    );

    jsl_string_builder_format(
        builder,
        fixed_get_function_code,
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
        fixed_delete_function_code,
        function_prefix,
        hash_map_name,
        key_type_name,
        hash_map_name,
        function_prefix
    );

    jsl_string_builder_format(
        builder,
        fixed_iterator_start_function_code,
        function_prefix,
        hash_map_name,
        hash_map_name
    );

    jsl_string_builder_format(
        builder,
        fixed_iterator_next_function_code,
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
        else if (jsl_fatptr_memory_compare(arg, JSL_FATPTR_EXPRESSION("--fixed")))
        {
            impl = IMPL_FIXED;
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

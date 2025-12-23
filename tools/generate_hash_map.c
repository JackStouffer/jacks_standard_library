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

#include "../src/jsl_core.c"
#include "../src/jsl_string_builder.c"
#include "../src/jsl_str_to_str_map.c"
#include "../src/jsl_os.c"
#include "../src/jsl_cmd_line.c"

#include "templates/static_hash_map_header.h"
#include "templates/static_hash_map_source.h"

#define SIMPLE_TEMPLATE_IMPLEMENTATION
#include "simple_template.h"

#define GENERATE_HASH_MAP_IMPLEMENTATION
#include "generate_hash_map.h"


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
            jsl_format_to_file(stderr, JSL_FATPTR_EXPRESSION("Error: Unknown argument: %y\n"), arg);
            return EXIT_FAILURE;
        }
    }

    if (show_help)
    {
        jsl_format_to_file(stdout, help_message);
        return EXIT_SUCCESS;
    }
    // Check that all required parameters are provided
    else if (name.data == NULL)
    {
        jsl_format_to_file(stderr, JSL_FATPTR_EXPRESSION("Error: --name is required\n"));
        return EXIT_FAILURE;
    }

    if (function_prefix.data == NULL)
    {
        jsl_format_to_file(stderr, JSL_FATPTR_EXPRESSION("Error: --function_prefix is required\n"));
        return EXIT_FAILURE;
    }

    if (key_type.data == NULL)
    {
        jsl_format_to_file(stderr, JSL_FATPTR_EXPRESSION("Error: --key_type is required\n"));
        return EXIT_FAILURE;
    }

    if (value_type.data == NULL)
    {
        jsl_format_to_file(stderr, JSL_FATPTR_EXPRESSION("Error: --value_type is required\n"));
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
            arena,
            &builder,
            impl,
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
            arena,
            &builder,
            impl,
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

    JSLFatPtr slice;
    while (jsl_string_builder_iterator_next(&iterator, &slice))
    {
        jsl_format_to_file(stdout, slice);
    }


    return EXIT_SUCCESS;
}


// #if JSL_IS_WINDOWS

// // annoyingly, clang does not special case wmain like main
// // for missing prototypes.
// int32_t wmain(int32_t argc, wchar_t** argv);

// int32_t wmain(int32_t argc, wchar_t** argv)
// {
    
//     SetConsoleOutputCP(CP_UTF8);
//     SetConsoleCP(CP_UTF8);
//     _setmode(_fileno(stdout), _O_BINARY);
//     _setmode(_fileno(stderr), _O_BINARY);

// #else

int32_t main(int32_t argc, char** argv)
{

// #endif

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

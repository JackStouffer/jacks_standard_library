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
#include "../src/jsl_str_to_str_multimap.c"
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
    "\tgenerate_hash_map --name TYPE_NAME --function-prefix PREFIX --key-type TYPE --value-type TYPE [--static | --dynamic] [--header | --source] [--add-header=FILE]...\n\n"
    "Required arguments:\n"
    "\t--name\t\t\tThe name to give the hash map container type\n"
    "\t--function-prefix\tThe prefix added to each of the functions for the hash map\n"
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

static int32_t entrypoint(JSLArena* arena, JSLCmdLine* cmd)
{
    bool show_help = false;
    JSLFatPtr name = {0};
    JSLFatPtr function_prefix = {0};
    JSLFatPtr key_type = {0};
    JSLFatPtr value_type = {0};
    JSLFatPtr hash_function_name = {0};
    HashMapImplementation impl = IMPL_ERROR;
    JSLFatPtr* header_includes = NULL;
    int32_t header_includes_count = 0;

    static JSLFatPtr help_flag_str = JSL_FATPTR_INITIALIZER("help");
    static JSLFatPtr name_flag_str = JSL_FATPTR_INITIALIZER("name");
    static JSLFatPtr function_prefix_flag_str = JSL_FATPTR_INITIALIZER("function-prefix");
    static JSLFatPtr key_type_flag_str = JSL_FATPTR_INITIALIZER("key-type");
    static JSLFatPtr value_type_flag_str = JSL_FATPTR_INITIALIZER("value-type");
    static JSLFatPtr fixed_flag_str = JSL_FATPTR_INITIALIZER("fixed");
    static JSLFatPtr dynamic_flag_str = JSL_FATPTR_INITIALIZER("dynamic");
    static JSLFatPtr header_flag_str = JSL_FATPTR_INITIALIZER("header");
    static JSLFatPtr source_flag_str = JSL_FATPTR_INITIALIZER("source");
    static JSLFatPtr add_header_flag_str = JSL_FATPTR_INITIALIZER("add-header");
    static JSLFatPtr custom_hash_flag_str = JSL_FATPTR_INITIALIZER("custom-hash");
    
    //
    // Parsing command line
    //

    show_help = jsl_cmd_line_has_short_flag(cmd, 'h')
        || jsl_cmd_line_has_flag(cmd, help_flag_str);

    jsl_cmd_line_pop_flag_with_value(cmd, name_flag_str, &name);
    jsl_cmd_line_pop_flag_with_value(cmd, function_prefix_flag_str, &function_prefix);
    jsl_cmd_line_pop_flag_with_value(cmd, key_type_flag_str, &key_type);
    jsl_cmd_line_pop_flag_with_value(cmd, value_type_flag_str, &value_type);
    jsl_cmd_line_pop_flag_with_value(cmd, custom_hash_flag_str, &hash_function_name);

    JSLFatPtr custom_header = {0};
    while (jsl_cmd_line_pop_flag_with_value(cmd, add_header_flag_str, &custom_header))
    {
        ++header_includes_count;
        header_includes = realloc(
            header_includes,
            sizeof(JSLFatPtr) * (size_t) header_includes_count
        );
        header_includes[header_includes_count - 1] = custom_header;
    }

    bool fixed_flag_set = jsl_cmd_line_has_flag(cmd, fixed_flag_str);
    bool dynamic_flag_set = jsl_cmd_line_has_flag(cmd, dynamic_flag_str);
    bool header_flag_set = jsl_cmd_line_has_flag(cmd, header_flag_str);
    bool source_flag_set = jsl_cmd_line_has_flag(cmd, source_flag_str);

    if (show_help)
    {
        jsl_write_to_c_file(stdout, help_message);
        return EXIT_SUCCESS;
    }
    
    //
    // Check that all required parameters are provided
    //

    if (name.data == NULL)
    {
        jsl_format_to_c_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Error: --%y is required\n"),
            name_flag_str
        );
        return EXIT_FAILURE;
    }
    if (key_type.data == NULL)
    {
        jsl_format_to_c_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Error: --%y is required\n"),
            key_type_flag_str
        );
        return EXIT_FAILURE;
    }
    if (value_type.data == NULL)
    {
        jsl_format_to_c_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Error: --%y is required\n"),
            value_type_flag_str
        );
        return EXIT_FAILURE;
    }
    if (function_prefix.data == NULL)
    {
        jsl_format_to_c_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Error: --%y is required\n"),
            function_prefix_flag_str
        );
        return EXIT_FAILURE;
    }
    if (fixed_flag_set && dynamic_flag_set)
    {
        jsl_format_to_c_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Error: cannot set both --%y and --%y\n"),
            fixed_flag_str,
            dynamic_flag_str
        );
        return EXIT_FAILURE;
    }
    if (!fixed_flag_set && !dynamic_flag_set)
    {
        jsl_format_to_c_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Error: you must provide either --%y or --%y\n"),
            fixed_flag_str,
            dynamic_flag_str
        );
        return EXIT_FAILURE;
    }
    if (header_flag_set && source_flag_set)
    {
        jsl_format_to_c_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Error: cannot set both --%y and --%y\n"),
            header_flag_str,
            source_flag_str
        );
        return EXIT_FAILURE;
    }
    if (!header_flag_set && !source_flag_set)
    {
        jsl_format_to_c_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Error: you must provide either --%y or --%y\n"),
            header_flag_str,
            source_flag_str
        );
        return EXIT_FAILURE;
    }

    if (fixed_flag_set) impl = IMPL_FIXED;
    if (fixed_flag_set) impl = IMPL_DYNAMIC;

    JSLStringBuilder builder;
    jsl_string_builder_init(&builder, arena);

    if (header_flag_set)
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
        jsl_write_to_c_file(stdout, slice);
    }


    return EXIT_SUCCESS;
}



#if JSL_IS_WINDOWS

    #include <windows.h>
    #include <shellapi.h>
    #include <fcntl.h>
    #include <io.h>
    #pragma comment(lib, "shell32.lib")

    int32_t wmain(int32_t argc, wchar_t** argv);

    int32_t wmain(int32_t argc, wchar_t** argv)
    {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        _setmode(_fileno(stdout), _O_BINARY);
        _setmode(_fileno(stderr), _O_BINARY);

        int64_t arena_size = JSL_MEGABYTES(32);
        JSLArena arena;
        void* backing_data = malloc((size_t) arena_size);
        if (backing_data == NULL)
            return EXIT_FAILURE;

        jsl_arena_init(&arena, backing_data, arena_size);

        JSLCmdLine cmd;
        jsl_cmd_line_init(&cmd, &arena);
        JSLFatPtr error_message = {0};
        if (!jsl_cmd_line_parse_wide(&cmd, argc, argv, &error_message))
        {
            if (error_message.data != NULL)
            {
                jsl_format_to_c_file(stderr, error_message);
            }
            else
            {
                jsl_format_to_c_file(stderr, JSL_FATPTR_EXPRESSION("Parsing failure"));
            }
            jsl_format_to_c_file(stderr, JSL_FATPTR_EXPRESSION("\n"));
            return EXIT_FAILURE;
        }

        return entrypoint(&arena, &cmd);
    }

#elif JSL_IS_POSIX

    int32_t main(int32_t argc, char **argv)
    {
        int64_t arena_size = JSL_MEGABYTES(32);
        JSLArena arena;
        void* backing_data = malloc((size_t) arena_size);
        if (backing_data == NULL)
            return EXIT_FAILURE;

        jsl_arena_init(&arena, backing_data, arena_size);

        JSLCmdLine cmd;
        if (!jsl_cmd_line_init(&cmd, &arena))
        {
            jsl_format_to_c_file(stderr, JSL_FATPTR_EXPRESSION("Command line input exceeds memory limit"));
            return EXIT_FAILURE;
        }

        JSLFatPtr error_message = {0};
        if (!jsl_cmd_line_parse(&cmd, argc, argv, &error_message))
        {
            if (error_message.data != NULL)
            {
                jsl_format_to_c_file(stderr, error_message);
            }
            else
            {
                jsl_format_to_c_file(stderr, JSL_FATPTR_EXPRESSION("Parsing failure"));
            }
            jsl_format_to_c_file(stderr, JSL_FATPTR_EXPRESSION("\n"));
            return EXIT_FAILURE;
        }

        return entrypoint(&arena, &cmd);
    }

#else

    #error "Unknown platform. Only Windows and POSIX systems are supported."
    
#endif

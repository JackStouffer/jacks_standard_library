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

static int32_t entrypoint(JSLArena* arena, JSLCmdLine* cmd)
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

    JSLFatPtr help_flag_str = JSL_FATPTR_INITIALIZER("help");
    JSLFatPtr name_flag_str = JSL_FATPTR_INITIALIZER("name");
    JSLFatPtr function_prefix_flag_str = JSL_FATPTR_INITIALIZER("function-prefix");
    JSLFatPtr key_type_flag_str = JSL_FATPTR_INITIALIZER("key-type");
    JSLFatPtr value_type_flag_str = JSL_FATPTR_INITIALIZER("value-type");
    JSLFatPtr fixed_flag_str = JSL_FATPTR_INITIALIZER("fixed");
    JSLFatPtr dynamic_flag_str = JSL_FATPTR_INITIALIZER("dynamic");
    JSLFatPtr header_flag_str = JSL_FATPTR_INITIALIZER("header");
    JSLFatPtr source_flag_str = JSL_FATPTR_INITIALIZER("source");
    JSLFatPtr add_header_flag_str = JSL_FATPTR_INITIALIZER("add-header");
    JSLFatPtr custom_hash_flag_str = JSL_FATPTR_INITIALIZER("custom-hash");
    

    if (jsl_cmd_line_has_short_flag(cmd, 'h') || jsl_cmd_line_has_flag(cmd, help_flag_str))
    {
        show_help = true;
    }

    jsl_cmd_line_pop_flag_with_value(cmd, name_flag_str, &name);
    jsl_cmd_line_pop_flag_with_value(cmd, function_prefix_flag_str, &function_prefix);
    jsl_cmd_line_pop_flag_with_value(cmd, key_type_flag_str, &key_type);
    jsl_cmd_line_pop_flag_with_value(cmd, value_type_flag_str, &function_prefix);
    jsl_cmd_line_pop_flag_with_value(cmd, add_header_flag_str, &function_prefix);
    jsl_cmd_line_pop_flag_with_value(cmd, custom_hash_flag_str, &hash_function_name);
    
    bool has = jsl_cmd_line_has_flag(cmd, fixed_flag_str);
    bool has = jsl_cmd_line_has_flag(cmd, dynamic_flag_str);
    bool has = jsl_cmd_line_has_flag(cmd, header_flag_str);
    bool has = jsl_cmd_line_has_flag(cmd, source_flag_str);

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
        jsl_cmd_line_parse_wide(&cmd, argc, argv);

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
            jsl_format_to_file(stderr, JSL_FATPTR_EXPRESSION("Command line input exceeds memory limit"));
            return EXIT_FAILURE;
        }

        if (!jsl_cmd_line_parse(&cmd, argc, argv))
        {
            jsl_format_to_file(stderr, JSL_FATPTR_EXPRESSION("Parsing failure"));
            return EXIT_FAILURE;
        }

        return entrypoint(&arena, &cmd);
    }

#else

    #error "Unknown platform. Only Windows and POSIX systems are supported."
    
#endif

/**
 * TODO: docs
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

#ifdef INCLUDE_MAIN
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "jsl/everything.c"

#define GENERATE_ARRAY_IMPLEMENTATION
#include "generate_array.h"


#if JSL_IS_WINDOWS
    #include <windows.h>
    #include <shellapi.h>
    #pragma comment(lib, "shell32.lib")
#endif

JSLImmutableMemory help_message = JSL_CSTR_INITIALIZER(
    "OVERVIEW:\n\n"
    "Array C code generation utility\n\n"
    "This program generates both a C source and header file for an array with the given\n"
    "element type. More documentation is included in the source file.\n\n"
    "USAGE:\n\n"
    "\tgenerate_array --name TYPE_NAME --function-prefix PREFIX --value-type TYPE [--static | --dynamic] [--header | --source] [--add-header=FILE]...\n\n"
    "Required arguments:\n"
    "\t--name\t\t\tThe name to give the hash map container type\n"
    "\t--function-prefix\tThe prefix added to each of the functions for the hash map\n"
    "\t--value_type\t\tThe C type name for the value\n\n"
    "Optional arguments:\n"
    "\t--header\t\tWrite the header file to stdout\n"
    "\t--source\t\tWrite the source file to stdout\n"
    "\t--dynamic\t\tGenerate a hash map which grows dynamically\n"
    "\t--static\t\tGenerate a statically sized hash map\n"
    "\t--add-header\t\tPath to a C header which will be added with a #include directive at the top of the generated file\n"
    "\t--custom-hash\t\tOverride the included hash call with the given function name\n"
);

static int32_t entrypoint(JSLAllocatorInterface allocator, JSLCmdLineArgs* cmd)
{
    bool show_help = false;
    JSLImmutableMemory name = {0};
    JSLImmutableMemory function_prefix = {0};
    JSLImmutableMemory value_type = {0};
    ArrayImplementation impl = IMPL_ERROR;
    JSLImmutableMemory* header_includes = NULL;
    int32_t header_includes_count = 0;

    JSLOutputSink stdout_sink = jsl_c_file_output_sink(stdout);
    JSLOutputSink stderr_sink = jsl_c_file_output_sink(stderr);

    static JSLImmutableMemory help_flag_str = JSL_CSTR_INITIALIZER("help");
    static JSLImmutableMemory name_flag_str = JSL_CSTR_INITIALIZER("name");
    static JSLImmutableMemory function_prefix_flag_str = JSL_CSTR_INITIALIZER("function-prefix");
    static JSLImmutableMemory value_type_flag_str = JSL_CSTR_INITIALIZER("value-type");
    static JSLImmutableMemory fixed_flag_str = JSL_CSTR_INITIALIZER("fixed");
    static JSLImmutableMemory dynamic_flag_str = JSL_CSTR_INITIALIZER("dynamic");
    static JSLImmutableMemory header_flag_str = JSL_CSTR_INITIALIZER("header");
    static JSLImmutableMemory source_flag_str = JSL_CSTR_INITIALIZER("source");
    static JSLImmutableMemory add_header_flag_str = JSL_CSTR_INITIALIZER("add-header");

    //
    // Parsing command line
    //

    show_help = jsl_cmd_line_args_has_short_flag(cmd, 'h')
        || jsl_cmd_line_args_has_flag(cmd, help_flag_str);

    jsl_cmd_line_args_pop_flag_with_value(cmd, name_flag_str, &name);
    jsl_cmd_line_args_pop_flag_with_value(cmd, function_prefix_flag_str, &function_prefix);
    jsl_cmd_line_args_pop_flag_with_value(cmd, value_type_flag_str, &value_type);

    JSLImmutableMemory custom_header = {0};
    while (jsl_cmd_line_args_pop_flag_with_value(cmd, add_header_flag_str, &custom_header))
    {
        ++header_includes_count;
        header_includes = realloc(
            header_includes,
            sizeof(JSLImmutableMemory) * (size_t) header_includes_count
        );
        header_includes[header_includes_count - 1] = custom_header;
    }

    bool fixed_flag_set = jsl_cmd_line_args_has_flag(cmd, fixed_flag_str);
    bool dynamic_flag_set = jsl_cmd_line_args_has_flag(cmd, dynamic_flag_str);
    bool header_flag_set = jsl_cmd_line_args_has_flag(cmd, header_flag_str);
    bool source_flag_set = jsl_cmd_line_args_has_flag(cmd, source_flag_str);

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
        jsl_format_sink(
            stderr_sink,
            JSL_CSTR_EXPRESSION("Error: --%y is required\n"),
            name_flag_str
        );
        return EXIT_FAILURE;
    }
    if (value_type.data == NULL)
    {
        jsl_format_sink(
            stderr_sink,
            JSL_CSTR_EXPRESSION("Error: --%y is required\n"),
            value_type_flag_str
        );
        return EXIT_FAILURE;
    }
    if (function_prefix.data == NULL)
    {
        jsl_format_sink(
            stderr_sink,
            JSL_CSTR_EXPRESSION("Error: --%y is required\n"),
            function_prefix_flag_str
        );
        return EXIT_FAILURE;
    }
    if (fixed_flag_set && dynamic_flag_set)
    {
        jsl_format_sink(
            stderr_sink,
            JSL_CSTR_EXPRESSION("Error: cannot set both --%y and --%y\n"),
            fixed_flag_str,
            dynamic_flag_str
        );
        return EXIT_FAILURE;
    }
    if (!fixed_flag_set && !dynamic_flag_set)
    {
        jsl_format_sink(
            stderr_sink,
            JSL_CSTR_EXPRESSION("Error: you must provide either --%y or --%y\n"),
            fixed_flag_str,
            dynamic_flag_str
        );
        return EXIT_FAILURE;
    }
    if (header_flag_set && source_flag_set)
    {
        jsl_format_sink(
            stderr_sink,
            JSL_CSTR_EXPRESSION("Error: cannot set both --%y and --%y\n"),
            header_flag_str,
            source_flag_str
        );
        return EXIT_FAILURE;
    }
    if (!header_flag_set && !source_flag_set)
    {
        jsl_format_sink(
            stderr_sink,
            JSL_CSTR_EXPRESSION("Error: you must provide either --%y or --%y\n"),
            header_flag_str,
            source_flag_str
        );
        return EXIT_FAILURE;
    }

    if (fixed_flag_set) impl = IMPL_FIXED;
    if (dynamic_flag_set) impl = IMPL_DYNAMIC;

    if (header_flag_set)
    {
        write_array_header(
            allocator,
            stdout_sink,
            impl,
            name,
            function_prefix,
            value_type,
            header_includes,
            header_includes_count
        );
    }
    else
    {
        write_array_source(
            allocator,
            stdout_sink,
            impl,
            name,
            function_prefix,
            value_type,
            header_includes,
            header_includes_count
        );
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

        JSLInfiniteArena arena;
        bool arena_init = jsl_infinite_arena_init(&arena);
        assert(arena_init);

        JSLAllocatorInterface allocator;
        jsl_infinite_arena_get_allocator_interface(&allocator, &arena);

        JSLCmdLineArgs cmd;
        jsl_cmd_line_args_init(&cmd, &allocator);
        JSLImmutableMemory error_message = {0};
        if (!jsl_cmd_line_args_parse_wide(&cmd, argc, argv, &error_message))
        {
            if (error_message.data != NULL)
            {
                jsl_write_to_c_file(stderr, error_message);
            }
            else
            {
                jsl_write_to_c_file(stderr, JSL_CSTR_EXPRESSION("Parsing failure"));
            }
            jsl_write_to_c_file(stderr, JSL_CSTR_EXPRESSION("\n"));
            return EXIT_FAILURE;
        }

        return entrypoint(allocator, &cmd);
    }

#elif JSL_IS_POSIX

    #include <sys/mman.h>

    int32_t main(int32_t argc, char **argv)
    {
        JSLInfiniteArena arena;
        bool arena_init = jsl_infinite_arena_init(&arena);
        assert(arena_init);

        JSLAllocatorInterface allocator;
        jsl_infinite_arena_get_allocator_interface(&allocator, &arena);

        JSLCmdLineArgs cmd;
        if (!jsl_cmd_line_args_init(&cmd, allocator))
        {
            jsl_write_to_c_file(stderr, JSL_CSTR_EXPRESSION("Command line input exceeds memory limit"));
            return EXIT_FAILURE;
        }

        JSLImmutableMemory error_message = {0};
        if (!jsl_cmd_line_args_parse(&cmd, argc, argv, &error_message))
        {
            if (error_message.data != NULL)
            {
                jsl_write_to_c_file(stderr, error_message);
            }
            else
            {
                jsl_write_to_c_file(stderr, JSL_CSTR_EXPRESSION("Parsing failure"));
            }
            jsl_write_to_c_file(stderr, JSL_CSTR_EXPRESSION("\n"));
            return EXIT_FAILURE;
        }

        return entrypoint(allocator, &cmd);
    }

#else

    #error "Unknown platform. Only Windows and POSIX systems are supported."
    
#endif

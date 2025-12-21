/**
 * # Embed File Data
 * 
 * Every large, serious C program wants to take file data and include
 * it in binary. Unfortunately it took until C23 for this to be standardized
 * in the language. This program takes the binary representation of a
 * file and generates the text of a C header with that info as a fat pointer.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#define EMBED_IMPLEMENTATION
#include "embed.h"

#include "../../src/jsl_core.c"
#include "../../src/jsl_string_builder.c"
#include "../../src/jsl_os.c"
#include "../../src/jsl_str_to_str_map.c"
#include "../../src/jsl_str_to_str_multimap.c"
#include "../../src/jsl_cmd_line.c"

static int32_t entrypoint(JSLCmdLine* cmd, JSLArena* arena)
{
    JSLFatPtr file_path = {0};
    JSLFatPtr file_contents = {0};
    int32_t load_errno = 0;

    jsl_cmd_line_pop_arg_list(cmd, &file_path);

    if (jsl_cmd_line_pop_arg_list(cmd, &file_path))
    {
        jsl_format_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Only provide zero or one file path\n")
        );
        return EXIT_FAILURE;
    }

    if (file_path.data != NULL)
    {
        JSLLoadFileResultEnum load_result = jsl_load_file_contents(
            arena,
            file_path,
            &file_contents,
            &load_errno
        );

        if (load_result != JSL_FILE_LOAD_SUCCESS)
        {
            jsl_format_file(
                stderr,
                JSL_FATPTR_EXPRESSION("Failed to load template file %y (errno %d)\n"),
                file_contents,
                load_errno
            );
            return EXIT_FAILURE;
        }

        JSLStringBuilder builder;
        jsl_string_builder_init(&builder, arena);

        generate_embed_header(
            &builder,
            JSL_FATPTR_EXPRESSION("test"),
            file_contents
        );
        
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
    else
    {
        JSLFatPtr stdin_buffer = jsl_arena_allocate(arena, 4096, false);
        JSLFatPtr stdin_buffer_read_buffer = stdin_buffer;

        size_t n;
        while ((n = fread(stdin_buffer_read_buffer.data, 1, stdin_buffer_read_buffer.length, stdin)) > 0)
        {
            printf("n %lu\n", n);
            jsl_format_file(stdout, stdin_buffer);
            JSL_FATPTR_ADVANCE(stdin_buffer_read_buffer, (int64_t) n);

            if (stdin_buffer_read_buffer.length < 32)
            {
                stdin_buffer = jsl_arena_reallocate(arena, stdin_buffer, stdin_buffer.length + 4096);
                stdin_buffer_read_buffer.length += 4096;
            }
        }

        JSLFatPtr written_to = jsl_fatptr_auto_slice(stdin_buffer, stdin_buffer_read_buffer);
        jsl_format_file(stdout, written_to);
    }

    return EXIT_SUCCESS;
}

#if JSL_IS_WINDOWS

    #include <windows.h>
    #include <shellapi.h>
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

        return entrypoint(&cmd, &arena);
    }

#else

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
            jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Command line input exceeds memory limit"));
            return EXIT_FAILURE;
        }

        if (!jsl_cmd_line_parse(&cmd, argc, argv))
        {
            jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Parsing failure"));
            return EXIT_FAILURE;
        }

        return entrypoint(&cmd, &arena);
    }

#endif

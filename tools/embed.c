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

#include "../src/jsl_core.c"
#include "../src/jsl_string_builder.c"
#include "../src/jsl_os.c"
#include "../src/jsl_str_set.c"
#include "../src/jsl_str_to_str_map.c"
#include "../src/jsl_str_to_str_multimap.c"
#include "../src/jsl_cmd_line.c"

static JSLFatPtr help_message = JSL_FATPTR_INITIALIZER(
    "OVERVIEW:\n\n"
    "Embed a file into a C program by generating a header file with the binary data.\n"
    "Pass in the file data as an argument or from stdin.\n\n"
    "USAGE:\n"
    "\tembed --var-name=VAR-NAME [file]\n\n"
    "Optional arguments:\n"
    "\t--var-name\t\tSet the name of the exported variable containing the binary data.\n"
);

static JSLFatPtr default_var_name = JSL_FATPTR_EXPRESSION("data");

static int32_t entrypoint(
    JSLCmdLine* cmd,
    JSLArena* arena,
    bool stdin_has_data
)
{
    JSLFatPtr file_path = {0};
    JSLFatPtr file_contents = {0};
    int32_t load_errno = 0;

    jsl_cmd_line_pop_arg_list(cmd, &file_path);

    if (file_path.data != NULL)
    {
        if (jsl_cmd_line_pop_arg_list(cmd, &file_path))
        {
            jsl_format_to_c_file(
                stderr,
                JSL_FATPTR_EXPRESSION("Only provide zero or one file path\n")
            );
            return EXIT_FAILURE;
        }

        JSLLoadFileResultEnum load_result = jsl_load_file_contents(
            arena,
            file_path,
            &file_contents,
            &load_errno
        );

        if (load_result != JSL_FILE_LOAD_SUCCESS)
        {
            jsl_format_to_c_file(
                stderr,
                JSL_FATPTR_EXPRESSION("Failed to load template file %y (errno %d)\n"),
                file_contents,
                load_errno
            );
            return EXIT_FAILURE;
        }
    }
    else if (stdin_has_data)
    {
        JSLFatPtr stdin_buffer = jsl_arena_allocate(arena, 4096, false);
        JSLFatPtr stdin_buffer_read_buffer = stdin_buffer;

        size_t n;
        while ((n = fread(stdin_buffer_read_buffer.data, 1, stdin_buffer_read_buffer.length, stdin)) > 0)
        {
            JSL_FATPTR_ADVANCE(stdin_buffer_read_buffer, (int64_t) n);

            if (stdin_buffer_read_buffer.length < 32)
            {
                stdin_buffer = jsl_arena_reallocate(
                    arena,
                    stdin_buffer,
                    stdin_buffer.length + 4096
                );
                stdin_buffer_read_buffer.length += 4096;
            }
        }

        file_contents = jsl_fatptr_auto_slice(stdin_buffer, stdin_buffer_read_buffer);
    }
    else
    {
        jsl_format_to_c_file(stdout, help_message);
        return EXIT_FAILURE;
    }

    if (file_contents.length > 0)
    {
        JSLStringBuilder builder;
        jsl_string_builder_init(&builder, arena);

        JSLFatPtr var_name = default_var_name;
        jsl_cmd_line_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("var-name"), &var_name);

        generate_embed_header(
            &builder,
            var_name,
            file_contents
        );
        
        JSLStringBuilderIterator iterator;
        jsl_string_builder_iterator_init(&builder, &iterator);

        JSLFatPtr slice;
        while (jsl_string_builder_iterator_next(&iterator, &slice))
        {
            jsl_write_to_c_file(stdout, slice);
        }

        return EXIT_SUCCESS;
    }
    else
    {
        jsl_format_to_c_file(stdout, help_message);
        return EXIT_FAILURE;
    }
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

        SYSTEM_INFO si;
        GetSystemInfo(&si);
        int64_t ps = (int64_t) si.dwPageSize;
        int64_t page_size = ps > 0 ? ps : 4096;

        JSLArena arena;

        int64_t arena_size = jsl_round_up_i64(JSL_MEGABYTES(64), page_size);
        void* backing_data = VirtualAlloc(0, arena_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (backing_data == NULL)
        {
            jsl_write_to_c_file(stderr, JSL_FATPTR_EXPRESSION("Failed to allocate memory"));
            return EXIT_FAILURE;
        }

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

            jsl_write_to_c_file(stderr, JSL_FATPTR_EXPRESSION("\n"));
            return EXIT_FAILURE;
        }

        HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
        DWORD stdin_type = GetFileType(stdin_handle);
        bool stdin_has_data = false;

        if (stdin_type == FILE_TYPE_PIPE)
        {
            DWORD bytes_available = 0;
            if (PeekNamedPipe(stdin_handle, NULL, 0, NULL, &bytes_available, NULL) && bytes_available > 0)
                stdin_has_data = true;
        }
        else if (stdin_type == FILE_TYPE_DISK)
        {
            stdin_has_data = true;
        }

        return entrypoint(&cmd, &arena, stdin_has_data);
    }

#elif JSL_IS_POSIX

    #include <poll.h>
    #include <unistd.h>
    #include <sys/mman.h>

    int32_t main(int32_t argc, char **argv)
    {
        int64_t ps = (int64_t) sysconf(_SC_PAGESIZE);
        int64_t page_size = ps > 0 ? ps : 4096;

        int64_t arena_size = jsl_round_up_nearest_pow2_i64(JSL_MEGABYTES(64), page_size);
        JSLArena arena;

        void* backing_data = mmap(NULL, (size_t) arena_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (backing_data == NULL)
        {
            jsl_write_to_c_file(stderr, JSL_FATPTR_EXPRESSION("Failed to allocate memory"));
            return EXIT_FAILURE;
        }

        jsl_arena_init(&arena, backing_data, arena_size);

        JSLCmdLine cmd;
        if (!jsl_cmd_line_init(&cmd, &arena))
        {
            jsl_write_to_c_file(stderr, JSL_FATPTR_EXPRESSION("Command line input exceeds memory limit"));
            return EXIT_FAILURE;
        }

        JSLFatPtr error_message = {0};
        if (!jsl_cmd_line_parse(&cmd, argc, argv, &error_message))
        {
            if (error_message.data != NULL)
            {
                jsl_write_to_c_file(stderr, error_message);
            }
            else
            {
                jsl_write_to_c_file(stderr, JSL_FATPTR_EXPRESSION("Parsing failure"));
            }

            jsl_write_to_c_file(stderr, JSL_FATPTR_EXPRESSION("\n"));
            return EXIT_FAILURE;
        }

        struct pollfd stdin_poll = {
            .fd = STDIN_FILENO,
            .events = POLLIN
        };
        int stdin_poll_result = poll(&stdin_poll, 1, 0);
        bool stdin_has_data = stdin_poll_result > 0 && (stdin_poll.revents & POLLIN);

        return entrypoint(&cmd, &arena, stdin_has_data);
    }

#else

    #error "Unknown platform. Only Windows and POSIX systems are supported."
    
#endif

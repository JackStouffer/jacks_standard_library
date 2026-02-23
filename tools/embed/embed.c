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

#include "../../src/jsl_everything.c"

#define EMBED_IMPLEMENTATION
#include "embed.h"

static JSLImmutableMemory help_message = JSL_CSTR_INITIALIZER(
    "OVERVIEW:\n\n"
    "Embed a file into a C program by generating a header file.\n"
    "Pass in the file path as an argument or pass in the file data from stdin.\n\n"
    "USAGE:\n"
    "\tembed --var-name=VAR-NAME [--binary | --text] [file]\n\n"
    "Optional arguments:\n"
    "\t--var-name\t\tSet the name of the exported variable containing the binary data.\n"
    "\t--binary\t\tThe output will be in bytes of hex data.\n"
    "\t--text\t\tThe output will be a multiline C str with length. Expects text file input\n"
);
static JSLImmutableMemory default_var_name = JSL_CSTR_INITIALIZER("data");
static JSLImmutableMemory help_flag_str = JSL_CSTR_INITIALIZER("help");
static JSLImmutableMemory binary_flag_str = JSL_CSTR_INITIALIZER("binary");
static JSLImmutableMemory text_flag_str = JSL_CSTR_INITIALIZER("text");
static JSLImmutableMemory var_name_flag_str = JSL_CSTR_INITIALIZER("var-name");

static int32_t entrypoint(
    JSLCmdLineArgs* cmd,
    JSLAllocatorInterface* allocator,
    bool stdin_has_data
)
{
    JSLOutputSink stdout_sink = jsl_c_file_output_sink(stdout);
    JSLOutputSink stderr_sink = jsl_c_file_output_sink(stderr);

    JSLImmutableMemory file_path = {0};
    JSLImmutableMemory file_contents = {0};
    JSLImmutableMemory var_name = default_var_name;
    int32_t load_errno = 0;
    EmbedOuputTypeEnum output_type = OUTPUT_TYPE_INVALID;

    bool show_help = jsl_cmd_line_args_has_short_flag(cmd, 'h')
        || jsl_cmd_line_args_has_flag(cmd, help_flag_str);

    bool output_binary = jsl_cmd_line_args_has_flag(cmd, binary_flag_str);
    bool output_text = jsl_cmd_line_args_has_flag(cmd, text_flag_str);

    jsl_cmd_line_args_pop_flag_with_value(cmd, var_name_flag_str, &var_name);

    jsl_cmd_line_args_pop_arg_list(cmd, &file_path);

    //
    // check params
    //
    if (output_text && output_binary)
    {
        jsl_format_sink(
            stderr_sink,
            JSL_CSTR_EXPRESSION("Error: cannot specify both --%y and --%y"),
            binary_flag_str,
            text_flag_str
        );
        return EXIT_FAILURE;
    }
    else if (!output_text && !output_binary)
    {
        output_type = OUTPUT_TYPE_BINARY;
    }
    else if (output_text)
    {
        output_type = OUTPUT_TYPE_TEXT;
    }
    else if (output_binary)
    {
        output_type = OUTPUT_TYPE_BINARY;
    }

    if (show_help)
    {
        jsl_write_to_c_file(stdout, help_message);
        return EXIT_SUCCESS;
    }
    else if (file_path.data != NULL)
    {
        if (jsl_cmd_line_args_pop_arg_list(cmd, &file_path))
        {
            jsl_format_sink(
                stderr_sink,
                JSL_CSTR_EXPRESSION("Only provide zero or one file path\n")
            );
            return EXIT_FAILURE;
        }

        JSLLoadFileResultEnum load_result = jsl_load_file_contents(
            allocator,
            file_path,
            &file_contents,
            &load_errno
        );

        if (load_result != JSL_FILE_LOAD_SUCCESS)
        {
            jsl_format_sink(
                stderr_sink,
                JSL_CSTR_EXPRESSION("Failed to load template file %y (errno %d)\n"),
                file_contents,
                load_errno
            );
            return EXIT_FAILURE;
        }
    }
    else if (stdin_has_data)
    {
        // TODO: refactor to dynamic array

        uint8_t* stdin_buffer = malloc(4096);
        int64_t buffer_length = 4096;
        int64_t cursor = 0;

        size_t n;
        while (true)
        {
            size_t remaining = (size_t) (buffer_length - cursor);
            if (remaining == 0)
            {
                stdin_buffer = realloc(stdin_buffer, (size_t) (buffer_length + 4096));
                buffer_length += 4096;
                remaining = (size_t) (buffer_length - cursor);
            }

            n = fread(stdin_buffer + cursor, 1, remaining, stdin);
            if (n == 0)
                break;

            cursor += (int64_t) n;
        }

        file_contents = jsl_immutable_memory(stdin_buffer, cursor);
    }
    else
    {
        jsl_format_sink(stdout_sink, help_message);
        return EXIT_FAILURE;
    }

    if (file_contents.length > 0)
    {
        generate_embed_header(
            stdout_sink,
            var_name,
            file_contents,
            output_type
        );

        return EXIT_SUCCESS;
    }
    else
    {
        jsl_write_to_c_file(stderr, JSL_CSTR_EXPRESSION("Error: no input data"));
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

        return entrypoint(&cmd, &allocator, stdin_has_data);
    }

#elif JSL_IS_POSIX

    #include <poll.h>
    #include <unistd.h>
    #include <sys/mman.h>

    int32_t main(int32_t argc, char **argv)
    {
        JSLInfiniteArena arena;
        bool arena_init = jsl_infinite_arena_init(&arena);
        assert(arena_init);

        JSLAllocatorInterface allocator;
        jsl_infinite_arena_get_allocator_interface(&allocator, &arena);

        JSLCmdLineArgs cmd;
        if (!jsl_cmd_line_args_init(&cmd, &allocator))
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

        struct pollfd stdin_poll = {
            .fd = STDIN_FILENO,
            .events = POLLIN
        };
        int stdin_poll_result = poll(&stdin_poll, 1, 0);
        bool stdin_has_data = stdin_poll_result > 0 && (stdin_poll.revents & POLLIN);

        return entrypoint(&cmd, &allocator, stdin_has_data);
    }

#else

    #error "Unknown platform. Only Windows and POSIX systems are supported."
    
#endif

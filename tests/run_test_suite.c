/**
 * This file runs the test suite using a meta-program style of build system.
 * 
 * Each test file is compiled and run four times. On POSIX systems it's once
 * with gcc unoptimized and with address sanitizer, once with gcc full optimization
 * and native CPU code gen, and then the same thing again with clang. On
 * Windows it's done with MSVC and clang.
 * 
 * This may seem excessive but I've caught bugs with this before.
 * 
 * ## Running
 * 
 * The program needs a one time bootstrap from your chosen C compiler.
 * 
 * On POSIX
 * 
 * ```
 * cc -o run_test_suite ./tests/bin/run_test_suite.c 
 * ```
 * 
 * On Windows
 * 
 * ```
 * cl /Fe"run_test_suite.exe" tests\\bin\\run_test_suite.c 
 * ```
 * 
 * Then run your executable. Every time afterwards when you run the test
 * program it will check if there have been changes to the test file. If
 * there have been it will rebuild itself using the program you used to
 * build it in the first place. if there have been changes to the test
 * file, so no need to 
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define JSL_IMPLEMENTATION
#include "../src/jacks_standard_library.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

char* test_file_paths[] = {
    "tests/test_fatptr.c",
    "tests/test_format.c",
    "tests/test_string_builder.c",
};

char* test_file_names[] = {
    "test_fatptr",
    "test_format",
    "test_string_builder",
};

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);


    int32_t test_file_count = sizeof(test_file_paths) / sizeof(char*);
    for (int32_t i = 0; i < test_file_count; i++)
    {
        char* test_file_path = test_file_paths[i];
        char* test_file_name = test_file_names[i];

        {
            char exe_name[128] = "tests/bin/debug_clang_";
            strncat(exe_name, test_file_name, strlen(test_file_name));

            #if defined(_WIN32)
                strncat(exe_name, ".exe", 4);
                char exe_run_command[128] = exe_name;
            #else
                strncat(exe_name, ".out", 4);
                char exe_run_command[128] = "./";
                strncat(exe_run_command, exe_name, strlen(exe_name));
            #endif

            Nob_Cmd clang_debug_compile_command = {0};
            nob_cmd_append(
                &clang_debug_compile_command,
                "clang",
                "-O0",
                "-g",
                "-fsanitize=address",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-pedantic",
                "-o", exe_name,
                test_file_path
            );
            if (!nob_cmd_run(&clang_debug_compile_command)) return 1;

            Nob_Cmd clang_debug_run_command = {0};
            nob_cmd_append(&clang_debug_run_command, exe_name);
            if (!nob_cmd_run(&clang_debug_run_command)) return 1;
        }

        {
            char exe_name[128] = "tests/bin/opt_clang_";
            strncat(exe_name, test_file_name, strlen(test_file_name));

            #if defined(_WIN32)
                strncat(exe_name, ".exe", 4);
                char exe_run_command[128] = exe_name;
            #else
                strncat(exe_name, ".out", 4);
                char exe_run_command[128] = "./";
                strncat(exe_run_command, exe_name, strlen(exe_name));
            #endif

            Nob_Cmd clang_debug_compile_command = {0};
            nob_cmd_append(
                &clang_debug_compile_command,
                "clang",
                "-O3",
                "-march=native",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-pedantic",
                "-o", exe_name,
                test_file_path
            );
            if (!nob_cmd_run(&clang_debug_compile_command)) return 1;

            Nob_Cmd clang_debug_run_command = {0};
            nob_cmd_append(&clang_debug_run_command, exe_name);
            if (!nob_cmd_run(&clang_debug_run_command)) return 1;
        }

        #if defined(_WIN32)

            {
                char exe_name[128] = "tests/bin/debug_msvc_";

                strncat(exe_name, test_file_name, strlen(test_file_name));
                strncat(exe_name, ".exe", 4);

                Nob_Cmd gcc_debug_compile_command = {0};
                nob_cmd_append(
                    &gcc_debug_compile_command,
                    "cl.exe",
                    "/nologo",
                    "/TC",
                    "/Od",
                    "/Zi",
                    "/W4",
                    "/WX",
                    "-fsanitize=address",
                    "-o", exe_name,
                    test_file_path
                );
                if (!nob_cmd_run(&gcc_debug_compile_command)) return 1;

                Nob_Cmd gcc_debug_run_command = {0};
                nob_cmd_append(&gcc_debug_run_command, "./tests/bin/debug_gcc");
                if (!nob_cmd_run(&gcc_debug_run_command)) return 1;
            }

            {
                char exe_name[128] = "tests/bin/debug_gcc_";

                strncat(exe_name, test_file_name, strlen(test_file_name));
                strncat(exe_name, ".exe", 4);

                Nob_Cmd gcc_optimized_compile_command = {0};
                nob_cmd_append(
                    &gcc_optimized_compile_command,
                    "gcc",
                    "-O3",
                    "-march=native",
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-pedantic",
                    "-o", exe_name,
                    test_file_path
                );
                if (!nob_cmd_run(&gcc_optimized_compile_command)) return 1;

                Nob_Cmd gcc_optimized_run_command = {0};
                nob_cmd_append(&gcc_optimized_run_command, "./tests/bin/opt_gcc");
                if (!nob_cmd_run(&gcc_optimized_run_command)) return 1;
            }

        #elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

            {
                char exe_name[128] = "tests/bin/debug_gcc_";

                strncat(exe_name, test_file_name, strlen(test_file_name));
                strncat(exe_name, ".out", 4);

                Nob_Cmd gcc_debug_compile_command = {0};
                nob_cmd_append(
                    &gcc_debug_compile_command,
                    "gcc",
                    "-O0",
                    "-g",
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-pedantic",
                    "-fsanitize=address",
                    "-o", exe_name,
                    test_file_path
                );
                if (!nob_cmd_run(&gcc_debug_compile_command)) return 1;

                Nob_Cmd gcc_debug_run_command = {0};
                nob_cmd_append(&gcc_debug_run_command, "./tests/bin/debug_gcc");
                if (!nob_cmd_run(&gcc_debug_run_command)) return 1;
            }

            {
                char exe_name[128] = "tests/bin/debug_gcc_";

                strncat(exe_name, test_file_name, strlen(test_file_name));
                strncat(exe_name, ".out", 4);

                Nob_Cmd gcc_optimized_compile_command = {0};
                nob_cmd_append(
                    &gcc_optimized_compile_command,
                    "gcc",
                    "-O3",
                    "-march=native",
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-pedantic",
                    "-o", exe_name,
                    test_file_path
                );
                if (!nob_cmd_run(&gcc_optimized_compile_command)) return 1;

                Nob_Cmd gcc_optimized_run_command = {0};
                nob_cmd_append(&gcc_optimized_run_command, "./tests/bin/opt_gcc");
                if (!nob_cmd_run(&gcc_optimized_run_command)) return 1;
            }
        
        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif
    }
    
    return 0;
}

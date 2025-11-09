/**
 * # Test Suite
 * 
 * This file runs the test suite using a meta-program style of build system.
 * 
 * Each test file is compiled and run four times. On POSIX systems it's once
 * with gcc unoptimized and with address sanitizer, once with gcc full optimization
 * and native CPU code gen, and then the same thing again with clang. On
 * Windows it's done with MSVC and clang.
 * 
 * This may seem excessive, but I've caught bugs with this before. Especially
 * with bugs around pointer alignment.
 * 
 * ## Running
 * 
 * The program needs a one time bootstrap from your chosen C compiler.
 * 
 * On POSIX
 * 
 * ```bash
 * cc -o run_test_suite ./tests/bin/run_test_suite.c 
 * ```
 * 
 * On Windows
 * 
 * ```
 * cl /Ferun_test_suite.exe tests\bin\run_test_suite.c 
 * ```
 * 
 * Then run your executable. Every time afterwards when you run the test
 * program it will check if there have been changes to the test file. If
 * there have been it will rebuild itself using the program you used to
 * build it in the first place. if there have been changes to the test
 * file, so no need to 
 */

#define NOB_IMPLEMENTATION
#include "nob.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define JSL_IMPLEMENTATION
#include "../src/jacks_standard_library.h"

typedef struct HashMapDecl { 
    char *name, *prefix, *key_type, *value_type, *impl_type;
    char** headers;
} HashMapDecl;

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

int32_t main(int32_t argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists("tests/bin")) return 1;
    if (!nob_mkdir_if_not_exists("tests/hash_maps")) return 1;

    nob_log(NOB_INFO, "Running unit test suite");

    /**
     *
     * 
     *              UNIT TESTS
     * 
     *  
     */

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
                char exe_run_command[128] = {0};
                strncat(exe_run_command, exe_name, strlen(exe_name));
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
                "-glldb",
                "-fno-omit-frame-pointer",
                "-fno-optimize-sibling-calls",
                "-DJSL_DEBUG",
                "-fsanitize=address",
                "-fsanitize=undefined",
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
                char exe_run_command[128] = {0};
                strncat(exe_run_command, exe_name, strlen(exe_name));
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
                "-glldb",
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
                char exe_output_param[128] = "/Fe";
                char exe_name[128] = "tests\\bin\\opt_msvc_";
                char obj_output_param[128] = "/Fo";
                char obj_name[128] = "tests\\bin\\opt_msvc_debug_";

                strncat(exe_name, test_file_name, strlen(test_file_name));
                strncat(exe_name, ".exe", 4);
                strncat(exe_output_param, exe_name, strlen(exe_name));
                strncat(obj_name, test_file_name, strlen(test_file_name));
                strncat(obj_name, ".obj", 4);
                strncat(obj_output_param, obj_name, strlen(obj_name));

                Nob_Cmd gcc_debug_compile_command = {0};
                nob_cmd_append(
                    &gcc_debug_compile_command,
                    "cl.exe",
                    "/nologo",
                    "/DJSL_DEBUG",
                    "/TC",
                    "/Od",
                    "/Zi",
                    "/W4",
                    "/WX",
                    "/std:c11",
                    exe_output_param,
                    obj_output_param,
                    test_file_path
                );
                if (!nob_cmd_run(&gcc_debug_compile_command)) return 1;

                Nob_Cmd msvc_debug_run_command = {0};
                nob_cmd_append(&msvc_debug_run_command, exe_name);
                if (!nob_cmd_run(&msvc_debug_run_command)) return 1;
            }

            {
                char exe_output_param[128] = "/Fe";
                char exe_name[128] = "tests\\bin\\opt_msvc_";
                char obj_output_param[128] = "/Fo";
                char obj_name[128] = "tests\\bin\\opt_msvc_release_";

                strncat(exe_name, test_file_name, strlen(test_file_name));
                strncat(exe_name, ".exe", 4);
                strncat(exe_output_param, exe_name, strlen(exe_name));
                strncat(obj_name, test_file_name, strlen(test_file_name));
                strncat(obj_name, ".obj", 4);
                strncat(obj_output_param, obj_name, strlen(obj_name));

                Nob_Cmd gcc_optimized_compile_command = {0};
                nob_cmd_append(
                    &gcc_optimized_compile_command,
                    "cl.exe",
                    "/nologo",
                    "/TC",
                    "/O2",
                    "/W4",
                    "/WX",
                    "/std:c11",
                    exe_output_param,
                    obj_output_param,
                    test_file_path
                );
                if (!nob_cmd_run(&gcc_optimized_compile_command)) return 1;

                Nob_Cmd msvc_optimized_run_command = {0};
                nob_cmd_append(&msvc_optimized_run_command, exe_name);
                if (!nob_cmd_run(&msvc_optimized_run_command)) return 1;
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

                char run_command[128] = "./";
                strncat(run_command, exe_name, strlen(exe_name));

                Nob_Cmd gcc_debug_run_command = {0};
                nob_cmd_append(&gcc_debug_run_command, run_command);
                if (!nob_cmd_run(&gcc_debug_run_command)) return 1;
            }

            {
                char exe_name[128] = "tests/bin/opt_gcc_";

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

                char run_command[128] = "./";
                strncat(run_command, exe_name, strlen(exe_name));

                Nob_Cmd gcc_optimized_run_command = {0};
                nob_cmd_append(&gcc_optimized_run_command, run_command);
                if (!nob_cmd_run(&gcc_optimized_run_command)) return 1;
            }
        
        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif
    }

    /**
     *
     * 
     *              HASH MAPS
     * 
     *  
     */

    nob_log(NOB_INFO, "Compiling generate hash map program");

    #if JSL_IS_WIN32
        char generate_hash_map_exe_name[128] = "tests\\bin\\generate_hash_map.exe";
        char generate_hash_map_run_exe_command[128] = ".\tests\\bin\\generate_hash_map.exe";
    #elif JSL_IS_POSIX
        char generate_hash_map_exe_name[128] = "tests/bin/generate_hash_map";
        char generate_hash_map_run_exe_command[128] = "./tests/bin/generate_hash_map";
    #else
        #error "Unrecognized platform. Only windows and POSIX platforms are supported."
    #endif

    Nob_Cmd generate_hash_map_compile_command = {0};
    nob_cmd_append(
        &generate_hash_map_compile_command,
        "clang",
        "-DINCLUDE_MAIN",
        "-O0",
        "-glldb",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-pedantic",
        "-o", generate_hash_map_exe_name,
        "src/generate_hash_map.c"
    );
    if (!nob_cmd_run(&generate_hash_map_compile_command)) return 1;
    
    HashMapDecl hash_map_declarations[] = {
        { "IntToIntMap", "int32_to_int32_map", "int32_t", "int32_t", "--static", (char*[]) {"../tests/test_hash_map_types.h", NULL} },
        { "FloatToFloatMap", "float_to_float_map", "float", "float", "--static", (char*[]) {"../tests/test_hash_map_types.h", NULL} }
    };

    int32_t hash_map_test_count = sizeof(hash_map_declarations) / sizeof(HashMapDecl);

    nob_log(NOB_INFO, "Generating Hash Map Files");

    for (int32_t i = 0; i < hash_map_test_count; ++i)
    {
        HashMapDecl* decl = &hash_map_declarations[i];

        Nob_Cmd write_hash_map_header = {0};
        nob_cmd_append(
            &write_hash_map_header,
            generate_hash_map_run_exe_command,
            "--name", decl->name,
            "--function_prefix", decl->prefix,
            "--key_type", decl->key_type,
            "--value_type", decl->value_type,
            decl->impl_type,
            "--header"
        );

        for (int32_t header_idx = 0;;++header_idx)
        {
            if (decl->headers == NULL)
                break;
            if (decl->headers[header_idx] == NULL)
                break;
            
            nob_cmd_append(
                &write_hash_map_header,
                "--add-header",
                decl->headers[header_idx]
            );
        }

        char out_path_header[128] = "tests/hash_maps/";
        strcat(out_path_header, decl->prefix);
        strcat(out_path_header, ".h");

        if (!nob_cmd_run(&write_hash_map_header, .stdout_path = out_path_header)) return 1;

        Nob_Cmd write_hash_map_source = {0};
        nob_cmd_append(
            &write_hash_map_source,
            generate_hash_map_run_exe_command,
            "--name", decl->name,
            "--function_prefix", decl->prefix,
            "--key_type", decl->key_type,
            "--value_type", decl->value_type,
            decl->impl_type,
            "--source"
        );

        for (int32_t header_idx = 0;;++header_idx)
        {
            if (decl->headers == NULL)
                break;
            if (decl->headers[header_idx] == NULL)
                break;
            
            nob_cmd_append(
                &write_hash_map_source,
                "--add-header",
                decl->headers[header_idx]
            );
        }

        char out_path_source[128] = "tests/hash_maps/";
        strcat(out_path_source, decl->prefix);
        strcat(out_path_source, ".c");

        if (!nob_cmd_run(&write_hash_map_source, .stdout_path = out_path_source)) return 1;
    }

    nob_log(NOB_INFO, "Running hash map test suite");
    
    return 0;
}

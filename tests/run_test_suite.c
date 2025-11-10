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
 * cc -o run_test_suite ./tests/run_test_suite.c 
 * ```
 * 
 * On Windows
 * 
 * ```
 * cl /Ferun_test_suite.exe tests\run_test_suite.c 
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

typedef struct UnitTestDecl { 
    char* executable_name;
    char** files;
} UnitTestDecl;

static UnitTestDecl unit_test_declarations[] = {
    { "test_fatptr", (char*[]) {"tests/test_fatptr.c", NULL} },
    { "test_format", (char*[]) {"tests/test_format.c", NULL} },
    { "test_string_builder", (char*[]) {"tests/test_string_builder.c", NULL} },
    { 
        "test_hash_map", 
        (char*[]) {
            "tests/test_hash_map.c",
            "tests/hash_maps/comp2_to_int_map.c",
            "tests/hash_maps/comp3_to_comp2_map.c",
            "tests/hash_maps/int32_to_comp1_map.c",
            "tests/hash_maps/int32_to_int32_map.c",
            NULL
        }
    }
};

static HashMapDecl hash_map_declarations[] = {
    { "IntToIntMap", "int32_to_int32_map", "int32_t", "int32_t", "--static", (char*[]) {"../tests/test_hash_map_types.h", NULL} },
    { "IntToCompositeType1Map", "int32_to_comp1_map", "int32_t", "CompositeType1", "--static", (char*[]) {"../tests/test_hash_map_types.h", NULL} },
    { "CompositeType2ToIntMap", "comp2_to_int_map", "CompositeType2", "int32_t", "--static", (char*[]) {"../tests/test_hash_map_types.h", NULL} },
    { "CompositeTyp3ToCompositeType2Map", "comp3_to_comp2_map", "CompositeType3", "CompositeType2", "--static", (char*[]) {"../tests/test_hash_map_types.h", NULL} }
};

int32_t main(int32_t argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists("tests/bin")) return 1;
    if (!nob_mkdir_if_not_exists("tests/hash_maps")) return 1;

    /**
     *
     * 
     *              HASH MAPS
     * 
     *  
     */

    nob_log(NOB_INFO, "Compiling generate hash map program");

    #if JSL_IS_WIN32
        char generate_hash_map_exe_name[256] = "tests\\bin\\generate_hash_map.exe";
        char generate_hash_map_run_exe_command[256] = ".\\tests\\bin\\generate_hash_map.exe";
    #elif JSL_IS_POSIX
        char generate_hash_map_exe_name[256] = "tests/bin/generate_hash_map";
        char generate_hash_map_run_exe_command[256] = "./tests/bin/generate_hash_map";
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

        char out_path_header[256] = "tests/hash_maps/";
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

        char out_path_source[256] = "tests/hash_maps/";
        strcat(out_path_source, decl->prefix);
        strcat(out_path_source, ".c");

        if (!nob_cmd_run(&write_hash_map_source, .stdout_path = out_path_source)) return 1;
    }

    nob_log(NOB_INFO, "Running unit test suite");

    /**
     *
     * 
     *              UNIT TESTS
     * 
     *  
     */

    int32_t test_count = sizeof(unit_test_declarations) / sizeof(UnitTestDecl);
    for (int32_t i = 0; i < test_count; i++)
    {
        UnitTestDecl* unit_test = &unit_test_declarations[i];

        {
            char exe_name[256] = "tests/bin/debug_clang_";
            strcat(exe_name, unit_test->executable_name);

            #if defined(_WIN32)
                strcat(exe_name, ".exe");
                char exe_run_command[256] = {0};
                strcat(exe_run_command, exe_name);
            #else
                strcat(exe_name, ".out");
                char exe_run_command[256] = "./";
                strcat(exe_run_command, exe_name);
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
                "-Isrc/",
                "-Wall",
                "-Wextra",
                "-pedantic",
                "-o", exe_name
            );

            for (int32_t source_file_idx = 0;; ++source_file_idx)
            {
                char* source_file = unit_test->files[source_file_idx];
                if (source_file == NULL)
                    break;
                    
                nob_cmd_append(&clang_debug_compile_command, source_file);
            }

            if (!nob_cmd_run(&clang_debug_compile_command)) return 1;

            Nob_Cmd clang_debug_run_command = {0};
            nob_cmd_append(&clang_debug_run_command, exe_name);
            if (!nob_cmd_run(&clang_debug_run_command)) return 1;
        }

        {
            char exe_name[256] = "tests/bin/opt_clang_";
            strcat(exe_name, unit_test->executable_name);

            #if defined(_WIN32)
                strcat(exe_name, ".exe");
                char exe_run_command[256] = {0};
                strncat(exe_run_command, exe_name, strlen(exe_name));
            #else
                strcat(exe_name, ".out");
                char exe_run_command[256] = "./";
                strcat(exe_run_command, exe_name);
            #endif

            Nob_Cmd clang_optimized_compile_command = {0};
            nob_cmd_append(
                &clang_optimized_compile_command,
                "clang",
                "-O3",
                "-glldb",
                "-march=native",
                "-Isrc/",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-pedantic",
                "-o", exe_name
            );

            for (int32_t source_file_idx = 0;; ++source_file_idx)
            {
                char* source_file = unit_test->files[source_file_idx];
                if (source_file == NULL)
                    break;
                    
                nob_cmd_append(&clang_optimized_compile_command, source_file);
            }

            if (!nob_cmd_run(&clang_optimized_compile_command)) return 1;

            Nob_Cmd clang_optimized_run_command = {0};
            nob_cmd_append(&clang_optimized_run_command, exe_name);
            if (!nob_cmd_run(&clang_optimized_run_command)) return 1;
        }

        #if JSL_IS_WIN32

            {
                char exe_output_param[256] = "/Fe";
                char exe_name[256] = "tests\\bin\\opt_msvc_";
                char obj_output_param[256] = "/Fo";
                char obj_name[256] = "tests\\bin\\opt_msvc_debug_";

                strcat(exe_name, unit_test->executable_name);
                strcat(exe_name, ".exe");
                strcat(exe_output_param, exe_name);
                strcat(obj_name, unit_test->executable_name);
                strcat(obj_name, ".obj");
                strcat(obj_output_param, obj_name);

                Nob_Cmd msvc_debug_compile_command = {0};
                nob_cmd_append(
                    &msvc_debug_compile_command,
                    "cl.exe",
                    "/nologo",
                    "/DJSL_DEBUG",
                    "/I\"src\\\"",
                    "/TC",
                    "/Od",
                    "/Zi",
                    "/W4",
                    "/WX",
                    "/std:c11",
                    exe_output_param,
                    obj_output_param,
                );

                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;
                        
                    nob_cmd_append(&msvc_debug_compile_command, source_file);
                }

                if (!nob_cmd_run(&msvc_debug_compile_command)) return 1;

                Nob_Cmd msvc_debug_run_command = {0};
                nob_cmd_append(&msvc_debug_run_command, exe_name);
                if (!nob_cmd_run(&msvc_debug_run_command)) return 1;
            }

            {
                char exe_output_param[256] = "/Fe";
                char exe_name[256] = "tests\\bin\\opt_msvc_";
                char obj_output_param[256] = "/Fo";
                char obj_name[256] = "tests\\bin\\opt_msvc_release_";

                strcat(exe_name, unit_test->executable_name);
                strcat(exe_name, ".exe");
                strcat(exe_output_param, exe_name);
                strcat(obj_name, unit_test->executable_name);
                strcat(obj_name, ".obj");
                strcat(obj_output_param, obj_name);

                Nob_Cmd msvc_optimized_compile_command = {0};
                nob_cmd_append(
                    &msvc_optimized_compile_command,
                    "cl.exe",
                    "/nologo",
                    "/I\"src\\\"",
                    "/TC",
                    "/O2",
                    "/W4",
                    "/WX",
                    "/std:c11",
                    exe_output_param,
                    obj_output_param
                );

                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;
                        
                    nob_cmd_append(&msvc_optimized_compile_command, source_file);
                }

                if (!nob_cmd_run(&msvc_optimized_compile_command)) return 1;

                Nob_Cmd msvc_optimized_run_command = {0};
                nob_cmd_append(&msvc_optimized_run_command, exe_name);
                if (!nob_cmd_run(&msvc_optimized_run_command)) return 1;
            }

        #elif JSL_IS_POSIX

            {
                char exe_name[256] = "tests/bin/debug_gcc_";

                strcat(exe_name, test_file_name);
                strcat(exe_name, ".out");

                Nob_Cmd gcc_debug_compile_command = {0};
                nob_cmd_append(
                    &gcc_debug_compile_command,
                    "gcc",
                    "-O0",
                    "-g",
                    "-std=c11",
                    "-Isrc/",
                    "-Wall",
                    "-Wextra",
                    "-pedantic",
                    "-fsanitize=address",
                    "-o", exe_name
                );

                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;
                        
                    nob_cmd_append(&gcc_debug_compile_command, source_file);
                }

                if (!nob_cmd_run(&gcc_debug_compile_command)) return 1;

                char run_command[256] = "./";
                strcat(run_command, exe_name);

                Nob_Cmd gcc_debug_run_command = {0};
                nob_cmd_append(&gcc_debug_run_command, run_command);
                if (!nob_cmd_run(&gcc_debug_run_command)) return 1;
            }

            {
                char exe_name[256] = "tests/bin/opt_gcc_";

                strcat(exe_name, test_file_name);
                strcat(exe_name, ".out");

                Nob_Cmd gcc_optimized_compile_command = {0};
                nob_cmd_append(
                    &gcc_optimized_compile_command,
                    "gcc",
                    "-O3",
                    "-march=native",
                    "-std=c11",
                    "-Isrc/",
                    "-Wall",
                    "-Wextra",
                    "-pedantic",
                    "-o", exe_name
                );

                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;
                        
                    nob_cmd_append(&gcc_optimized_compile_command, source_file);
                }

                if (!nob_cmd_run(&gcc_optimized_compile_command)) return 1;

                char run_command[256] = "./";
                strncat(run_command, exe_name, strlen(exe_name));

                Nob_Cmd gcc_optimized_run_command = {0};
                nob_cmd_append(&gcc_optimized_run_command, run_command);
                if (!nob_cmd_run(&gcc_optimized_run_command)) return 1;
            }
        
        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif
    }
    
    return 0;
}

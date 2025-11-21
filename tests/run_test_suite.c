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
    { "test_intrinsics", (char*[]) {"tests/test_intrinsics.c", NULL} },
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
    {
        "IntToIntMap",
        "int32_to_int32_map",
        "int32_t",
        "int32_t",
        "--static",
        (char*[]) {
            "../tests/hash_maps/int32_to_int32_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    },
    {
        "IntToCompositeType1Map",
        "int32_to_comp1_map",
        "int32_t",
        "CompositeType1",
        "--static",
        (char*[]) {
            "../tests/hash_maps/int32_to_comp1_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    },
    {
        "CompositeType2ToIntMap",
        "comp2_to_int_map",
        "CompositeType2",
        "int32_t",
        "--static",
        (char*[]) {
            "../tests/hash_maps/comp2_to_int_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    },
    {
        "CompositeType3ToCompositeType2Map",
        "comp3_to_comp2_map",
        "CompositeType3",
        "CompositeType2",
        "--static",
        (char*[]) {
            "../tests/hash_maps/comp3_to_comp2_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    }
};

typedef struct CStringArray {
    char** array;
    int32_t length;
    int32_t capacity;
} CStringArray;

char* my_strdup(char* str)
{
    size_t len = strlen(str);
    char* ret = calloc(len, sizeof(char));
    assert(ret != NULL);
    memcpy(ret, str, len);
    return ret;
}

void cstring_array_init(CStringArray* array)
{
    memset(array, 0, sizeof(CStringArray));
    array->capacity = 32;
    array->array = malloc(sizeof(CStringArray) * array->capacity);
    assert(array->array != NULL);
}

void cstring_array_insert(CStringArray* array, char* string)
{
    if (array->length == array->capacity)
    {
        array->capacity = jsl_next_power_of_two_u32(array->capacity + 1);
        array->array = realloc(array->array, sizeof(CStringArray) * array->capacity);
        assert(array->array != NULL);
    }

    array->array[array->length] = my_strdup(string);
    ++array->length;
}

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

    #if JSL_IS_WINDOWS
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

    Nob_Procs hash_map_procs = {0};

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

        if (!nob_cmd_run(
            &write_hash_map_header,
            .stdout_path = out_path_header,
            .async = &hash_map_procs
        )) return 1;

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

        if (!nob_cmd_run(
            &write_hash_map_source,
            .stdout_path = out_path_source,
            .async = &hash_map_procs
        )) return 1;
    }

    if (!nob_procs_wait(hash_map_procs)) return 1;
    nob_da_free(hash_map_procs);

    nob_log(NOB_INFO, "Running unit test suite");

    /**
     *
     * 
     *              UNIT TESTS
     * 
     *  
     */

    CStringArray executables;
    cstring_array_init(&executables);

    int32_t test_count = sizeof(unit_test_declarations) / sizeof(UnitTestDecl);
    Nob_Procs compile_procs = {0};

    for (int32_t i = 0; i < test_count; i++)
    {
        UnitTestDecl* unit_test = &unit_test_declarations[i];

        {
            char exe_name[256] = "tests/bin/debug_clang_";
            strcat(exe_name, unit_test->executable_name);

            #if JSL_IS_WINDOWS
                strcat(exe_name, ".exe");
            #else
                strcat(exe_name, ".out");
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

            if (!nob_cmd_run(&clang_debug_compile_command, .async = &compile_procs)) return 1;

            cstring_array_insert(&executables, exe_name);
        }

        {
            char exe_name[256] = "tests/bin/opt_clang_";
            strcat(exe_name, unit_test->executable_name);

            #if JSL_IS_WINDOWS
                strcat(exe_name, ".exe");
            #else
                strcat(exe_name, ".out");
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

            if (!nob_cmd_run(&clang_optimized_compile_command, .async = &compile_procs)) return 1;

            cstring_array_insert(&executables, exe_name);
        }

        #if JSL_IS_WINDOWS

            {
                char exe_output_param[512] = "/Fe";
                char exe_name[512] = "tests\\bin\\debug_msvc_";
                char obj_output_param[512] = "/Fo";
                char obj_name[512] = "tests\\bin\\debug_msvc_";

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
                    "/Isrc",
                    "/TC",
                    "/Od",
                    "/Zi",
                    "/W4",
                    "/WX",
                    "/std:c11",
                    exe_output_param
                    // obj_output_param,
                );

                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;
                        
                    nob_cmd_append(&msvc_debug_compile_command, source_file);
                }

                // TODO, speed: Add .async = &compile_procs for MSVC, right now
                // pdbs over write each other causing errors
                if (!nob_cmd_run(&msvc_debug_compile_command)) return 1;

                cstring_array_insert(&executables, exe_name);
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
                    "/Isrc",
                    "/TC",
                    "/O2",
                    "/W4",
                    "/WX",
                    "/std:c11",
                    exe_output_param
                    // obj_output_param
                );

                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;
                        
                    nob_cmd_append(&msvc_optimized_compile_command, source_file);
                }

                // TODO, speed: Add .async = &compile_procs for MSVC, right now
                // pdbs over write each other causing errors
                if (!nob_cmd_run(&msvc_optimized_compile_command)) return 1;
        
                cstring_array_insert(&executables, exe_name);
            }

        #elif JSL_IS_POSIX

            const char* test_file_name = unit_test->executable_name;

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

                if (!nob_cmd_run(&gcc_debug_compile_command, .async = &compile_procs)) return 1;
        
                cstring_array_insert(&executables, exe_name);
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

                if (!nob_cmd_run(&gcc_optimized_compile_command, .async = &compile_procs)) return 1;
        
                cstring_array_insert(&executables, exe_name);
            }
        
        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif
    }

    if (!nob_procs_wait(compile_procs)) return 1;
    nob_da_free(compile_procs);

    for (int32_t i = 0; i < executables.length; ++i)
    {
        #if JSL_IS_WINDOWS

            Nob_Cmd run_command = {0};
            nob_cmd_append(&run_command, executables.array[i]);
            if (!nob_cmd_run(&run_command)) return 1;

        #else

            char execute_command[256] = "./";
            strcat(execute_command, executables.array[i]);
            Nob_Cmd run_command = {0};
            nob_cmd_append(&run_command, execute_command);
            if (!nob_cmd_run(&run_command)) return 1;

        #endif
    }
    
    return 0;
}

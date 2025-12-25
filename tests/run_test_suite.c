/**
 * # Test Suite
 *
 * This file runs the test suite using a meta-program style of build system.
 *
 * Each test file is compiled and many, many times, with a list of different
 * configurations for each compiler. On Windows the compilers are MSVC and
 * clang, on everything else it's gcc and clang. This means each test file
 * is compiled and run upwards of a dozen times.
 *
 * This may seem excessive but this is the trade off of C being so versatile
 * for so many platforms/use-cases: there is a combinatoric explosion in
 * possible command line configurations. If you want to make a library which
 * is broadly usable you need to test that it actually works in all of these
 * scenarios.
 *
 * ## Running
 *
 * The program needs a one time bootstrap from your chosen C compiler.
 *
 * On POSIX
 *
 * ```bash
 * cc -o run_test_suite tests/run_test_suite.c
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
#include "../vendor/nob.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../src/jsl_core.c"

#if JSL_IS_POSIX
    #include <sys/types.h>
    #include <sys/sysctl.h>
#endif

typedef struct HashMapDecl {
    char *name, *prefix, *key_type, *value_type, *impl_type;
    char** headers;
} HashMapDecl;

typedef struct UnitTestDecl {
    char* executable_name;
    char** files;
} UnitTestDecl;

typedef struct CompilerConfig {
    char* prefix;
    char** flags;
} CompilerConfig;

static char* clang_warning_flags[] = {
    "-Wall",
    "-Wextra",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wshadow",
    "-Wconditional-uninitialized",
    "-Wcomma",
    "-Widiomatic-parentheses",
    "-Wpointer-arith",
    "-Wassign-enum",
    "-Wswitch-enum",
    "-Wimplicit-fallthrough",
    "-Wnull-dereference",
    "-Wmissing-prototypes",
    "-Wundef",
    "-pedantic",
    NULL
};

static CompilerConfig clang_configs[] = {
    {
        "clang_debug_c11_",
        (char*[]) {
            "-O0",
            "-glldb",
            "-fno-omit-frame-pointer",
            "-fno-optimize-sibling-calls",
            "-DJSL_DEBUG",
            "-fsanitize=address",
            "-fsanitize-address-use-after-scope",
            "-fsanitize=undefined",
            "-fsanitize=pointer-compare,pointer-subtract",
            "-fsanitize=alignment",
            "-fsanitize=unreachable,return",
            "-fsanitize=signed-integer-overflow,shift,shift-base,shift-exponent",
            "-fno-sanitize-recover=all",
            "-std=c11",
            "-Isrc/",
            NULL
        }
    },
    {
        "clang_debug_c23_",
        (char*[]) {
            "-O0",
            "-glldb",
            "-fno-omit-frame-pointer",
            "-fno-optimize-sibling-calls",
            "-DJSL_DEBUG",
            "-fsanitize=address",
            "-fsanitize-address-use-after-scope",
            "-fsanitize=undefined",
            "-fsanitize=pointer-compare,pointer-subtract",
            "-fsanitize=alignment",
            "-fsanitize=unreachable,return",
            "-fsanitize=signed-integer-overflow,shift,shift-base,shift-exponent",
            "-fno-sanitize-recover=all",
            "-std=c23",
            "-Isrc/",
            NULL
        }
    },
    {
        "clang_opt_level3_native_c11_",
        (char*[]) {
            "-O3",
            "-glldb",
            "-march=native",
            "-Isrc/",
            "-std=c11",
            NULL
        }
    },
    {
        "clang_opt_level3_native_c23_",
        (char*[]) {
            "-O3",
            "-glldb",
            "-march=native",
            "-Isrc/",
            "-std=c23",
            NULL
        }
    },
    {
        "clang_hardended_c11_",
        (char*[]) {
            "-O2",
            "-D_FORTIFY_SOURCE=2",
            "-fstack-protector-strong",

            #if !defined(__APPLE__)
                "-fsanitize=shadow-call-stack",
            #endif

            #if JSL_IS_POSIX
                "-fPIE",
            #endif

            "-glldb",
            "-Isrc/",
            "-std=c11",
            NULL
        }
    },
    {
        "clang_hardended_c23_",
        (char*[]) {
            "-O2",
            "-D_FORTIFY_SOURCE=2",
            "-fstack-protector-strong",

            #if !defined(__APPLE__)
                "-fsanitize=shadow-call-stack",
            #endif

            #if JSL_IS_POSIX
                "-fPIE",
            #endif

            "-glldb",
            "-Isrc/",
            "-std=c23",
            NULL
        }
    }
};

static CompilerConfig msvc_configs[] = {
    {
        "msvc_debug_c11_",
        (char*[]) {
            "/nologo",
            "/utf-8",
            "/DJSL_DEBUG",
            "/Isrc",
            "/Od",
            "/Zi", // debug info
            "/W4",
            "/WX", // warnings as errors
            "/std:c11",
            "/FS", // allow concurrent PDB writes
            NULL
        }
    },
    {
        "msvc_debug_c17_",
        (char*[]) {
            "/nologo",
            "/utf-8",
            "/DJSL_DEBUG",
            "/Isrc",
            "/Od",
            "/Zi", // debug info
            "/W4",
            "/WX", // warnings as errors
            "/std:c17",
            "/FS", // allow concurrent PDB writes
            NULL
        }
    },
    {
        "msvc_opt_c11_",
        (char*[]) {
            "/nologo",
            "/utf-8",
            "/Isrc",
            "/O2",
            "/W4",
            "/WX", // warnings as errors
            "/std:c11",
            "/FS", // allow concurrent PDB writes
            NULL
        }
    },
    {
        "msvc_opt_c17_",
        (char*[]) {
            "/nologo",
            "/utf-8",
            "/Isrc",
            "/O2",
            "/W4",
            "/WX", // warnings as errors
            "/std:c17",
            "/FS", // allow concurrent PDB writes
            NULL
        }
    },
    {
        "msvc_debug_error_checks_c11_",
        (char*[]) {
            "/nologo",
            "/utf-8",
            "/Isrc",
            "/Od",
            "/W4",
            "/WX", // warnings as errors
            "/std:c11",
            "/RTC1", // run time error checks
            "/sdl", // extra compile time error checks
            "/guard:cf",
            "/Qspectre",
            "/DYNAMICBASE",
            "/FS", // allow concurrent PDB writes
            NULL
        }
    },
    {
        "msvc_debug_error_checks_c17_",
        (char*[]) {
            "/nologo",
            "/utf-8",
            "/Isrc",
            "/Od",
            "/W4",
            "/WX", // warnings as errors
            "/std:c17",
            "/RTC1", // run time error checks
            "/sdl", // extra compile time error checks
            "/guard:cf",
            "/Qspectre",
            "/DYNAMICBASE",
            "/FS", // allow concurrent PDB writes
            NULL
        }
    }
};

static UnitTestDecl unit_test_declarations[] = {
    { "test_fatptr", (char*[]) {
        "tests/test_fatptr.c",
        "src/jsl_core.c",
        NULL
    } },
    { "test_format", (char*[]) {
        "tests/test_format.c",
        "src/jsl_core.c",
        NULL
    } },
    { "test_string_builder", (char*[]) {
        "tests/test_string_builder.c",
        "src/jsl_core.c",
        "src/jsl_string_builder.c",
        NULL
    } },
    { "test_cmd_line", (char*[]) {
        "tests/test_cmd_line.c",
        "src/jsl_core.c",
        "src/jsl_str_to_str_map.c",
        "src/jsl_str_to_str_multimap.c",
        "src/jsl_cmd_line.c",
        NULL
    } },
    { "test_intrinsics", (char*[]) {
        "tests/test_intrinsics.c",
        "src/jsl_core.c",
        NULL
    } },
    { "test_file_utils", (char*[]) {
        "tests/test_file_utils.c",
        "src/jsl_core.c",
        "src/jsl_os.c",
        NULL
    } },
    { "test_str_to_str_multimap", (char*[]) {
        "tests/test_str_to_str_multimap.c",
        "src/jsl_core.c",
        "src/jsl_str_to_str_multimap.c",
        NULL
    } },
    {
        "test_hash_map",
        (char*[]) {
            "tests/test_hash_map.c",
            "src/jsl_str_to_str_map.c",
            "tests/hash_maps/fixed_comp2_to_int_map.c",
            "tests/hash_maps/fixed_comp3_to_comp2_map.c",
            "tests/hash_maps/fixed_int32_to_comp1_map.c",
            "tests/hash_maps/fixed_int32_to_int32_map.c",
            NULL
        }
    }
};

static HashMapDecl hash_map_declarations[] = {
    {
        "FixedIntToIntMap",
        "fixed_int32_to_int32_map",
        "int32_t",
        "int32_t",
        "--fixed",
        (char*[]) {
            "../tests/hash_maps/fixed_int32_to_int32_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    },
    {
        "FixedIntToCompositeType1Map",
        "fixed_int32_to_comp1_map",
        "int32_t",
        "CompositeType1",
        "--fixed",
        (char*[]) {
            "../tests/hash_maps/fixed_int32_to_comp1_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    },
    {
        "FixedCompositeType2ToIntMap",
        "fixed_comp2_to_int_map",
        "CompositeType2",
        "int32_t",
        "--fixed",
        (char*[]) {
            "../tests/hash_maps/fixed_comp2_to_int_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    },
    {
        "FixedCompositeType3ToCompositeType2Map",
        "fixed_comp3_to_comp2_map",
        "CompositeType3",
        "CompositeType2",
        "--fixed",
        (char*[]) {
            "../tests/hash_maps/fixed_comp3_to_comp2_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    }
};

typedef struct CStringArray {
    char** array;
    int32_t length;
    int32_t capacity;
} CStringArray;

static char* my_strdup(char* str)
{
    size_t len = strlen(str) + 1;
    char* ret = calloc(len, sizeof(char));
    assert(ret != NULL);
    memcpy(ret, str, len);
    return ret;
}

static void cstring_array_init(CStringArray* array)
{
    memset(array, 0, sizeof(CStringArray));
    array->capacity = 32;
    array->array = malloc(sizeof(char*) * array->capacity);
    assert(array->array != NULL);
}

static void cstring_array_insert(CStringArray* array, char* string)
{
    if (array->length == array->capacity)
    {
        array->capacity = jsl_next_power_of_two_u32(array->capacity + 1);
        array->array = realloc(array->array, sizeof(char*) * array->capacity);
        assert(array->array != NULL);
    }

    array->array[array->length] = my_strdup(string);
    ++array->length;
}

static int32_t get_logical_processor_count(void)
{
    // Returns the number of logical (including SMT) processors available.
    // Falls back to 1 if the platform APIs cannot provide a value.
    #if JSL_IS_WINDOWS
        DWORD logical_processors = 0;

        #ifdef ALL_PROCESSOR_GROUPS
            logical_processors = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
        #endif

        if (logical_processors == 0)
        {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            logical_processors = info.dwNumberOfProcessors;
        }

    #elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
        long logical_processors = sysconf(_SC_NPROCESSORS_ONLN);

        if (logical_processors < 1)
        {
            int mib[2] = { CTL_HW, 0 };
            int count = 0;
            size_t size = sizeof(count);

            #ifdef HW_AVAILCPU
                mib[1] = HW_AVAILCPU;
                if (sysctl(mib, 2, &count, &size, NULL, 0) == 0 && count > 0)
                {
                    logical_processors = count;
                }
            #endif

            #ifdef HW_NCPU
                if (logical_processors < 1)
                {
                    mib[1] = HW_NCPU;
                    size = sizeof(count);
                    if (sysctl(mib, 2, &count, &size, NULL, 0) == 0 && count > 0)
                    {
                        logical_processors = count;
                    }
                }
            #endif
        }

    #endif

    if (logical_processors < 1)
        logical_processors = 1;
    if (logical_processors > INT32_MAX)
        logical_processors = 1;

    return (int32_t) logical_processors;
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
        "-DJSL_DEBUG",
        "-fno-omit-frame-pointer",
        "-fno-optimize-sibling-calls",
        "-O0",
        "-glldb",
        "-std=c11"
    );

    // add clang warnings
    for (int32_t flag_idx = 0;; ++flag_idx)
    {
        char* flag = clang_warning_flags[flag_idx];
        if (flag == NULL)
            break;

        nob_cmd_append(&generate_hash_map_compile_command, flag);
    }

    nob_cmd_append(
        &generate_hash_map_compile_command,
        "-o", generate_hash_map_exe_name,
        "-Isrc/",
        "tools/generate_hash_map.c"
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
            "--function-prefix", decl->prefix,
            "--key-type", decl->key_type,
            "--value-type", decl->value_type,
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
            "--function-prefix", decl->prefix,
            "--key-type", decl->key_type,
            "--value-type", decl->value_type,
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
    

    /**
     *
     *
     *              UNIT TESTS
     *
     *
     */

    nob_log(NOB_INFO, "Running unit test suite");

    CStringArray executables;
    cstring_array_init(&executables);

    int32_t test_count = sizeof(unit_test_declarations) / sizeof(UnitTestDecl);
    Nob_Procs compile_procs = {0};
    int32_t logical_processors = get_logical_processor_count();
    nob_log(NOB_INFO, "Compiling unit tests with up to %d parallel jobs", logical_processors);

    for (int32_t i = 0; i < test_count; i++)
    {
        UnitTestDecl* unit_test = &unit_test_declarations[i];

        /**
         * Loop over the clang configs creating compile commands for each.
         *
         * Also insert the exe name into the list of executables to be run
         * later.
         */

        int32_t clang_config_count = sizeof(clang_configs) / sizeof(CompilerConfig);
        for (int32_t i = 0; i < clang_config_count; ++i)
        {
            CompilerConfig* compiler_config = &clang_configs[i];

            char exe_name[256] = "tests/bin/";
            strcat(exe_name, compiler_config->prefix);
            strcat(exe_name, unit_test->executable_name);

            #if JSL_IS_WINDOWS
                strcat(exe_name, ".exe");
            #else
                strcat(exe_name, ".out");
            #endif

            Nob_Cmd compile_command = {0};
            nob_cmd_append(
                &compile_command,
                "clang",
                "-o", exe_name
            );

            // add compiler flags from config
            for (int32_t flag_idx = 0;; ++flag_idx)
            {
                char* flag = compiler_config->flags[flag_idx];
                if (flag == NULL)
                    break;

                nob_cmd_append(&compile_command, flag);
            }

            // add clang warnings
            for (int32_t flag_idx = 0;; ++flag_idx)
            {
                char* flag = clang_warning_flags[flag_idx];
                if (flag == NULL)
                    break;

                nob_cmd_append(&compile_command, flag);
            }

            for (int32_t source_file_idx = 0;; ++source_file_idx)
            {
                char* source_file = unit_test->files[source_file_idx];
                if (source_file == NULL)
                    break;

                nob_cmd_append(&compile_command, source_file);
            }

            if (!nob_cmd_run(&compile_command)) return 1;
            // if (!nob_cmd_run(&compile_command, .async = &compile_procs, .max_procs = (size_t)logical_processors)) return 1;

            cstring_array_insert(&executables, exe_name);
        }

        #if JSL_IS_WINDOWS

            /**
             * Loop over the MSVC configs creating compile commands for each.
             *
             * Also insert the exe name into the list of executables to be run
             * later.
             */

            int32_t msvc_config_count = sizeof(msvc_configs) / sizeof(CompilerConfig);
            for (int32_t i = 0; i < msvc_config_count; ++i)
            {
                CompilerConfig* compiler_config = &msvc_configs[i];

                char exe_output_param[256] = "/Fe";
                char exe_name[256] = "tests\\bin\\";
                char obj_output_param[256] = "/Fo";
                char obj_dir[256] = "tests\\bin\\";
                char pdb_output_param[256] = "/Fd";
                char pdb_name[256] = "tests\\bin\\";

                strcat(exe_name, compiler_config->prefix);
                strcat(exe_name, unit_test->executable_name);
                strcat(exe_name, ".exe");

                strcat(exe_output_param, exe_name);

                strcat(obj_dir, compiler_config->prefix);
                strcat(obj_dir, unit_test->executable_name);
                strcat(obj_dir, "_obj\\");

                if (!nob_mkdir_if_not_exists(obj_dir)) return 1;

                strcat(obj_output_param, obj_dir);

                strcat(pdb_name, compiler_config->prefix);
                strcat(pdb_name, unit_test->executable_name);
                strcat(pdb_name, ".pdb");
                strcat(pdb_output_param, pdb_name);

                Nob_Cmd compile_command = {0};
                nob_cmd_append(&compile_command, "cl.exe");

                // add compiler flags from config
                for (int32_t flag_idx = 0;; ++flag_idx)
                {
                    char* flag = compiler_config->flags[flag_idx];
                    if (flag == NULL)
                        break;

                    nob_cmd_append(&compile_command, flag);
                }

                nob_cmd_append(
                    &compile_command,
                    pdb_output_param,
                    obj_output_param,
                    exe_output_param
                );

                // add source files
                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;

                    nob_cmd_append(&compile_command, source_file);
                }

                if (!nob_cmd_run(&compile_command)) return 1;
                // if (!nob_cmd_run(&compile_command, .async = &compile_procs, .max_procs = (size_t)logical_processors)) return 1;

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

                if (!nob_cmd_run(&gcc_debug_compile_command, .async = &compile_procs, .max_procs = (size_t)logical_processors)) return 1;

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

                if (!nob_cmd_run(&gcc_optimized_compile_command, .async = &compile_procs, .max_procs = (size_t)logical_processors)) return 1;

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

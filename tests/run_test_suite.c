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

// nftw(3) and its flags require _XOPEN_SOURCE >= 500; must be defined before
// any system header (including those pulled in by nob.h below).
#if !defined(_WIN32) && !defined(__wasm__)
    #define _XOPEN_SOURCE 700
#endif

// On macOS/BSDs, _XOPEN_SOURCE hides non-POSIX extensions such as
// _SC_NPROCESSORS_ONLN and MAP_ANONYMOUS/MAP_NORESERVE. _DARWIN_C_SOURCE
// (and _BSD_SOURCE / _GNU_SOURCE on Linux/BSDs) re-exposes them.
#if defined(__APPLE__)
    #define _DARWIN_C_SOURCE 1
#endif
#if defined(__linux__) || defined(__linux)
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE 1
    #endif
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    #ifndef _BSD_SOURCE
        #define _BSD_SOURCE 1
    #endif
    #ifndef _DEFAULT_SOURCE
        #define _DEFAULT_SOURCE 1
    #endif
#endif

#define JSL_BUILDER_IMPLEMENTATION
#include "../tools/builder/builder.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../src/jsl/everything.c"

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
#endif

typedef struct HashMapDecl {
    char *name, *prefix, *key_type, *value_type, *impl_type;
    bool key_is_str;
    bool value_is_str;
    char** headers;
} HashMapDecl;

typedef struct ArrayDecl {
    char *name, *prefix, *value_type, *impl_type;
    char** headers;
} ArrayDecl;

typedef struct UnitTestDecl {
    char* executable_name;
    char** files;
} UnitTestDecl;

typedef struct CompilerConfig {
    char* prefix;
    char** flags;
} CompilerConfig;

static char* clang_warning_flags[] = {
    "-Werror",
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

    // Because the 4095 limit is DUMB
    "-Wno-overlength-strings",

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
    {
        "test_main",
        (char*[])
        {
            "tests/test_main.c",
            "tests/test_core.c",
            "tests/test_allocator_arena.c",
            "tests/test_allocator_libc.c",
            "tests/test_allocator_pool.c",
            "tests/test_array.c",
            "tests/test_cmd_line.c",
            "tests/test_file_utils.c",
            "tests/test_format.c",
            "tests/test_hash_map.c",
            "tests/test_hash_set.c",
            "tests/test_intrinsics.c",
            "tests/test_str_to_str_multimap.c",
            "tests/test_string_builder.c",
            "tests/test_subprocess.c",
            "src/jsl/everything.c",
            "tests/arrays/dynamic_comp1_array.c",
            "tests/arrays/dynamic_comp2_array.c",
            "tests/arrays/dynamic_comp3_array.c",
            "tests/arrays/dynamic_int32_array.c",
            "tests/hash_maps/fixed_comp2_to_int_map.c",
            "tests/hash_maps/fixed_comp3_to_comp2_map.c",
            "tests/hash_maps/fixed_int32_to_comp1_map.c",
            "tests/hash_maps/fixed_int32_to_int32_map.c",
            "tests/hash_maps/fixed_int32_to_str_map.c",
            "tests/hash_maps/fixed_str_to_int32_map.c",
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
        false,
        false,
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
        false,
        false,
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
        false,
        false,
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
        false,
        false,
        (char*[]) {
            "../tests/hash_maps/fixed_comp3_to_comp2_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    },
    {
        "FixedStrToIntMap",
        "fixed_str_to_int32_map",
        NULL,
        "int32_t",
        "--fixed",
        true,
        false,
        (char*[]) {
            "../tests/hash_maps/fixed_str_to_int32_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    },
    {
        "FixedIntToStrMap",
        "fixed_int32_to_str_map",
        "int32_t",
        NULL,
        "--fixed",
        false,
        true,
        (char*[]) {
            "../tests/hash_maps/fixed_int32_to_str_map.h",
            "../tests/test_hash_map_types.h", NULL
        }
    }
};

static ArrayDecl array_declarations[] = {
    {
        "DynamicInt32Array",
        "dynamic_int32_array",
        "int32_t",
        "--dynamic",
        (char*[]) {
            "../tests/hash_maps/dynamic_int32_array.h",
            "",
            NULL
        }
    },
    {
        "DynamicCompositeType1Map",
        "dynamic_comp1_array",
        "CompositeType1",
        "--dynamic",
        (char*[]) {
            "../tests/hash_maps/dynamic_comp1_array.h",
            "../tests/test_hash_map_types.h",
            NULL
        }
    },
    {
        "DynamicCompositeType2ToIntMap",
        "dynamic_comp2_array",
        "CompositeType2",
        "--dynamic",
        (char*[]) {
            "../tests/hash_maps/dynamic_comp2_array.h",
            "../tests/test_hash_map_types.h",
            NULL
        }
    },
    {
        "DynamicCompositeType3ToCompositeType2Map",
        "dynamic_comp3_array",
        "CompositeType3",
        "--dynamic",
        (char*[]) {
            "../tests/hash_maps/dynamic_comp3_array.h",
            "../tests/test_hash_map_types.h",
            NULL
        }
    }
};

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

    #elif defined(__linux__) || defined(__linux)
        long logical_processors = sysconf(_SC_NPROCESSORS_ONLN);

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
    static JSLImmutableMemory clang_command = JSL_CSTR_INITIALIZER("clang");
    static JSLImmutableMemory bin_path = JSL_CSTR_INITIALIZER("tests/bin");
    static JSLImmutableMemory test_array_path = JSL_CSTR_INITIALIZER("tests/arrays");
    static JSLImmutableMemory test_hash_map_path = JSL_CSTR_INITIALIZER("tests/hash_maps");

    /**
     *
     *
     *                          SETUP
     *
     *
     */

    BUILDER_AUTO_BOOTSTRAP(argc, argv);

    JSLOutputSink stdout_sink = jsl_c_file_output_sink(stdout);
    JSLOutputSink stderr_sink = jsl_c_file_output_sink(stderr);
    
    int32_t last_errno = 0;

    JSLInfiniteArena build_memory;
    JSLAllocatorInterface build_memory_interface;
    jsl_infinite_arena_init(&build_memory);
    jsl_infinite_arena_get_allocator_interface(&build_memory_interface, &build_memory);

    jsl_make_directory(bin_path, NULL);
    jsl_make_directory(test_array_path, NULL);
    jsl_make_directory(test_hash_map_path, NULL);

    int32_t max_parallelism = JSL_MAX(jsl_get_logical_processor_count(NULL), 1);
    jsl_format_sink(
        stdout_sink,
        JSL_CSTR_EXPRESSION("Running with %d max parallel processes\n"),
        max_parallelism
    );

    /**
     *
     *
     *                       EMBED TOOL
     *
     *
     */

    {
        jsl_format_sink(stdout_sink, JSL_CSTR_EXPRESSION("Compiling embed program\n"));

        #if JSL_IS_WINDOWS
            char embed_exe_name[256] = "tests\\bin\\embed.exe";
        #elif JSL_IS_POSIX
            char embed_exe_name[256] = "tests/bin/embed";
        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif

        JSLSubprocess embed_compile_command;
        JSL_ZERO_STRUCT(embed_compile_command);

        jsl_subprocess_init(&embed_compile_command, build_memory_interface, clang_command);
        jsl_subprocess_arg_cstr(
            &embed_compile_command,
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

            jsl_subprocess_arg_cstr(&embed_compile_command, flag);
        }

        jsl_subprocess_arg_cstr(
            &embed_compile_command,
            "-o", embed_exe_name,
            "-Isrc/",
            "tools/embed/embed.c"
        );

        jsl_format_sink(stdout_sink, JSL_CSTR_EXPRESSION("CMD: "));
        jsl_subprocess_debug_print_command(&embed_compile_command, stdout_sink);
        jsl_format_sink(stdout_sink, JSL_CSTR_EXPRESSION("\n"));

        JSLSubProcessResultEnum run_res = jsl_subprocess_run_blocking(
            &embed_compile_command,
            1,
            build_memory_interface,
            &last_errno
        );
        if (run_res != JSL_SUBPROCESS_SUCCESS || embed_compile_command.exit_code != 0)
        {
            jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Building embed program failed with result %d exit code %d errno %d\n"), run_res, embed_compile_command.exit_code, last_errno);
            return EXIT_FAILURE;
        }
    }

    /**
     *
     *
     *                 SUBPROCESS TEST HELPER
     *
     *
     */

    {
        jsl_format_sink(stdout_sink, JSL_CSTR_EXPRESSION("Compiling subprocess test helper program\n"));

        #if JSL_IS_WINDOWS
            char helper_exe_name[256] = "tests\\bin\\subprocess_helper.exe";
        #elif JSL_IS_POSIX
            char helper_exe_name[256] = "tests/bin/subprocess_helper";
        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif

        JSLSubprocess helper_compile_command = {0};
        jsl_subprocess_init(&helper_compile_command, build_memory_interface, clang_command);
        jsl_subprocess_arg_cstr(
            &helper_compile_command,
            "-O0",
            "-glldb",
            "-std=c11",
            "-o", helper_exe_name,
            "tests/subprocess_helper.c"
        );

        JSLSubProcessResultEnum run_res = jsl_subprocess_run_blocking(
            &helper_compile_command,
            1,
            build_memory_interface,
            &last_errno
        );
        if (run_res != JSL_SUBPROCESS_SUCCESS || helper_compile_command.exit_code != 0)
        {
            jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Building subprocess test program failed with result %d exit code %d errno %d\n"), run_res, helper_compile_command.exit_code, last_errno);
            return EXIT_FAILURE;
        }
    }

    /**
     *
     *
     *                         ARRAYS
     *
     *
     */

    {
        jsl_format_sink(stdout_sink, JSL_CSTR_EXPRESSION("Compiling generate array program\n"));

        #if JSL_IS_WINDOWS
            char generate_array_exe_name[256] = "tests\\bin\\generate_array.exe";
            static JSLImmutableMemory generate_array_run_exe_command = JSL_CSTR_INITIALIZER(".\\tests\\bin\\generate_array.exe");
        #elif JSL_IS_POSIX
            char generate_array_exe_name[256] = "tests/bin/generate_array";
            static JSLImmutableMemory generate_array_run_exe_command = JSL_CSTR_INITIALIZER("./tests/bin/generate_array");
        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif

        JSLSubprocess generate_array_compile_command;
        JSL_ZERO_STRUCT(generate_array_compile_command);

        jsl_subprocess_init(&generate_array_compile_command, build_memory_interface, clang_command);
        jsl_subprocess_arg_cstr(
            &generate_array_compile_command,
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

            jsl_subprocess_arg_cstr(&generate_array_compile_command, flag);
        }

        jsl_subprocess_arg_cstr(
            &generate_array_compile_command,
            "-o", generate_array_exe_name,
            "-Isrc/",
            "tools/generate_array/generate_array.c"
        );

        JSLSubProcessResultEnum run_res = jsl_subprocess_run_blocking(
            &generate_array_compile_command,
            1,
            build_memory_interface,
            NULL
        );
        if (run_res != JSL_SUBPROCESS_SUCCESS || generate_array_compile_command.exit_code != 0)
        {
            jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Building array generation program failed with result %d exit code %d errno %d\n"), run_res, generate_array_compile_command.exit_code, last_errno);
            return EXIT_FAILURE;
        }

        int32_t array_test_count = sizeof(array_declarations) / sizeof(ArrayDecl);

        jsl_format_sink(stdout_sink, JSL_CSTR_EXPRESSION("Generating Array Files\n"));

        int64_t array_procs_length = array_test_count * 2;
        JSLSubprocess* array_procs = jsl_allocator_interface_alloc(
            build_memory_interface,
            sizeof(JSLSubprocess) * array_procs_length,
            JSL_DEFAULT_ALLOCATION_ALIGNMENT,
            true
        );

        int32_t array_decl_idx = 0;
        int32_t array_proc_idx = 0;

        while (array_decl_idx < array_test_count)
        {
            ArrayDecl* decl = &array_declarations[array_decl_idx];

            JSLSubprocess* write_array_header = &array_procs[array_proc_idx];
            jsl_subprocess_init(write_array_header, build_memory_interface, generate_array_run_exe_command);
            ++array_proc_idx;

            jsl_subprocess_arg_cstr(
                write_array_header,
                "--name", decl->name,
                "--function-prefix", decl->prefix,
                "--value-type", decl->value_type,
                decl->impl_type,
                "--header",
                "--add-header",
                "../test_hash_map_types.h"
            );

            JSLImmutableMemory header_out_file_name = jsl_format(
                build_memory_interface,
                JSL_CSTR_EXPRESSION("tests/arrays/%s.h"),
                decl->prefix
            );
            jsl_subprocess_set_stdout_file_name(write_array_header, header_out_file_name);

            JSLSubprocess* write_array_source = &array_procs[array_proc_idx];
            jsl_subprocess_init(
                write_array_source,
                build_memory_interface,
                generate_array_run_exe_command
            );
            ++array_proc_idx;

            jsl_subprocess_arg_cstr(
                write_array_source,
                "--name", decl->name,
                "--function-prefix", decl->prefix,
                "--value-type", decl->value_type,
                decl->impl_type,
                "--source"
            );

            char header_name[256] = {0};
            strcat(header_name, decl->prefix);
            strcat(header_name, ".h");

            jsl_subprocess_arg_cstr(
                write_array_source,
                "--add-header",
                "../test_hash_map_types.h"
            );
            jsl_subprocess_arg_cstr(
                write_array_source,
                "--add-header",
                header_name
            );

            JSLImmutableMemory source_out_file_name = jsl_format(
                build_memory_interface,
                JSL_CSTR_EXPRESSION("tests/arrays/%s.c"),
                decl->prefix
            );
            jsl_subprocess_set_stdout_file_name(write_array_source, source_out_file_name);

            ++array_decl_idx;
        }

        run_res = jsl_subprocess_run_blocking(
            array_procs,
            array_procs_length,
            build_memory_interface,
            &last_errno
        );
        if (run_res != JSL_SUBPROCESS_SUCCESS)
        {
            jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Generating arrays failed with exit code %d errno %d\n"), run_res, last_errno);
            return EXIT_FAILURE;
        }
    }

    /**
     *
     *
     *                    HASH MAPS
     *
     *
     */

    {
        jsl_format_sink(stdout_sink, JSL_CSTR_EXPRESSION("Compiling generate hash map program\n"));

        #if JSL_IS_WINDOWS
            char generate_hash_map_exe_name[256] = "tests\\bin\\generate_hash_map.exe";
            static JSLImmutableMemory generate_hash_map_run_exe_command = JSL_CSTR_INITIALIZER(".\\tests\\bin\\generate_hash_map.exe");
        #elif JSL_IS_POSIX
            char generate_hash_map_exe_name[256] = "tests/bin/generate_hash_map";
            static JSLImmutableMemory generate_hash_map_run_exe_command = JSL_CSTR_INITIALIZER("./tests/bin/generate_hash_map");
        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif

        JSLSubprocess generate_hash_map_compile_command;
        JSL_ZERO_STRUCT(generate_hash_map_compile_command);

        jsl_subprocess_init(&generate_hash_map_compile_command, build_memory_interface, clang_command);
        jsl_subprocess_arg_cstr(
            &generate_hash_map_compile_command,
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

            jsl_subprocess_arg_cstr(&generate_hash_map_compile_command, flag);
        }

        jsl_subprocess_arg_cstr(
            &generate_hash_map_compile_command,
            "-o", generate_hash_map_exe_name,
            "-Isrc/",
            "tools/generate_hash_map/generate_hash_map.c"
        );

        JSLSubProcessResultEnum run_compile_res = jsl_subprocess_run_blocking(
            &generate_hash_map_compile_command,
            1,
            build_memory_interface,
            &last_errno
        );
        if (run_compile_res != JSL_SUBPROCESS_SUCCESS || generate_hash_map_compile_command.exit_code != 0)
        {
            jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Building hash map generation program failed with result %d exit code %d errno %d\n"), run_compile_res, generate_hash_map_compile_command.exit_code, last_errno);
            return EXIT_FAILURE;
        }

        printf("Generating Hash Map Files\n");

        int32_t hash_map_test_count = sizeof(hash_map_declarations) / sizeof(HashMapDecl);
        int32_t hash_map_procs_length = hash_map_test_count * 2;
        JSLSubprocess* hash_map_procs = jsl_allocator_interface_alloc(
            build_memory_interface,
            sizeof(JSLSubprocess) * hash_map_procs_length,
            JSL_DEFAULT_ALLOCATION_ALIGNMENT,
            true
        );

        int32_t hash_decl_idx = 0;
        int32_t hash_proc_idx = 0;

        while (hash_decl_idx < hash_map_test_count)
        {
            HashMapDecl* decl = &hash_map_declarations[hash_decl_idx];
            JSLSubprocess* write_hash_map_header = &hash_map_procs[hash_proc_idx];
            ++hash_proc_idx;

            jsl_subprocess_init(
                write_hash_map_header,
                build_memory_interface,
                generate_hash_map_run_exe_command
            );

            if (decl->key_is_str)
            {
                jsl_subprocess_arg_cstr(
                    write_hash_map_header,
                    "--name", decl->name,
                    "--function-prefix", decl->prefix,
                    "--key-is-string",
                    "--value-type", decl->value_type,
                    decl->impl_type,
                    "--header"
                );
            }
            else if (decl->value_is_str)
            {
                jsl_subprocess_arg_cstr(
                    write_hash_map_header,
                    "--name", decl->name,
                    "--function-prefix", decl->prefix,
                    "--key-type", decl->key_type,
                    "--value-is-string",
                    decl->impl_type,
                    "--header"
                );
            }
            else
            {
                jsl_subprocess_arg_cstr(
                    write_hash_map_header,
                    "--name", decl->name,
                    "--function-prefix", decl->prefix,
                    "--key-type", decl->key_type,
                    "--value-type", decl->value_type,
                    decl->impl_type,
                    "--header"
                );
            }

            for (int32_t header_idx = 0;; ++header_idx)
            {
                if (decl->headers == NULL)
                    break;
                if (decl->headers[header_idx] == NULL)
                    break;

                jsl_subprocess_arg_cstr(
                    write_hash_map_header,
                    "--add-header",
                    decl->headers[header_idx]
                );
            }

            JSLImmutableMemory out_file_name = jsl_format(
                build_memory_interface,
                JSL_CSTR_EXPRESSION("tests/hash_maps/%s.h"),
                decl->prefix
            );
            jsl_subprocess_set_stdout_file_name(write_hash_map_header, out_file_name);

            JSLSubprocess* write_hash_map_source = &hash_map_procs[hash_proc_idx];
            ++hash_proc_idx;

            jsl_subprocess_init(
                write_hash_map_source,
                build_memory_interface,
                generate_hash_map_run_exe_command
            );

            if (decl->key_is_str)
                jsl_subprocess_arg_cstr(
                    write_hash_map_source,
                    "--name", decl->name,
                    "--function-prefix", decl->prefix,
                    "--key-is-string",
                    "--value-type", decl->value_type,
                    decl->impl_type,
                    "--source"
                );
            else if (decl->value_is_str)
                jsl_subprocess_arg_cstr(
                    write_hash_map_source,
                    "--name", decl->name,
                    "--function-prefix", decl->prefix,
                    "--key-type", decl->key_type,
                    "--value-is-string",
                    decl->impl_type,
                    "--source"
                );
            else
                jsl_subprocess_arg_cstr(
                    write_hash_map_source,
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

                jsl_subprocess_arg_cstr(
                    write_hash_map_source,
                    "--add-header",
                    decl->headers[header_idx]
                );
            }

            out_file_name = jsl_format(
                build_memory_interface,
                JSL_CSTR_EXPRESSION("tests/hash_maps/%s.c"),
                decl->prefix
            );
            jsl_subprocess_set_stdout_file_name(write_hash_map_source, out_file_name);

            ++hash_decl_idx;
        }

        JSLSubProcessResultEnum run_res = jsl_subprocess_run_blocking(
            hash_map_procs,
            hash_map_procs_length,
            build_memory_interface,
            &last_errno
        );
        if (run_res != JSL_SUBPROCESS_SUCCESS)
        {
            jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Generating hash maps failed with result %d errno %d\n"), run_res, last_errno);
            return EXIT_FAILURE;
        }
    }
    

    /**
     *
     *
     *              UNIT TESTS
     *
     *
     */

    printf("Running unit test suite\n");

    int32_t test_count = (int32_t) (sizeof(unit_test_declarations) / sizeof(UnitTestDecl));
    int32_t clang_config_count = (int32_t) (sizeof(clang_configs) / sizeof(CompilerConfig));

    #if JSL_IS_WINDOWS
        int32_t msvc_config_count = (int32_t) (sizeof(msvc_configs) / sizeof(CompilerConfig));
        int32_t configs_per_test = clang_config_count + msvc_config_count;
    #elif JSL_IS_POSIX
        // clang configs plus the two gcc configs (debug + optimized).
        int32_t configs_per_test = clang_config_count + 2;
    #else
        #error "Unrecognized platform. Only windows and POSIX platforms are supported."
    #endif

    int32_t total_compile_count = test_count * configs_per_test;

    JSLSubprocess* test_compile_procs = jsl_allocator_interface_alloc(
        build_memory_interface,
        sizeof(JSLSubprocess) * total_compile_count,
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        true
    );

    // Executable paths, filled in lockstep with `test_compile_procs` so each
    // compiled binary can be run after the compile batch finishes.
    JSLImmutableMemory* test_executables = jsl_allocator_interface_alloc(
        build_memory_interface,
        sizeof(JSLImmutableMemory) * total_compile_count,
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        true
    );

    int32_t logical_processors = get_logical_processor_count();
    printf("Compiling unit tests with up to %d parallel jobs\n", logical_processors);

    int32_t compile_idx = 0;

    for (int32_t test_idx = 0; test_idx < test_count; ++test_idx)
    {
        UnitTestDecl* unit_test = &unit_test_declarations[test_idx];

        /**
         * Loop over the clang configs creating compile commands for each.
         *
         * Also record the exe name in the list of executables to be run
         * later.
         */

        for (int32_t config_idx = 0; config_idx < clang_config_count; ++config_idx)
        {
            CompilerConfig* compiler_config = &clang_configs[config_idx];

            #if JSL_IS_WINDOWS
                JSLImmutableMemory exe_name = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("tests/bin/%s%s.exe"),
                    compiler_config->prefix,
                    unit_test->executable_name
                );
            #else
                JSLImmutableMemory exe_name = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("tests/bin/%s%s.out"),
                    compiler_config->prefix,
                    unit_test->executable_name
                );
            #endif

            JSLSubprocess* compile_command = &test_compile_procs[compile_idx];
            jsl_subprocess_init(compile_command, build_memory_interface, clang_command);

            jsl_subprocess_arg(
                compile_command,
                JSL_CSTR_EXPRESSION("-o"),
                exe_name
            );

            // add compiler flags from config
            for (int32_t flag_idx = 0;; ++flag_idx)
            {
                char* flag = compiler_config->flags[flag_idx];
                if (flag == NULL)
                    break;

                jsl_subprocess_arg_cstr(compile_command, flag);
            }

            // nftw(3) and its flags require _XOPEN_SOURCE >= 500 on Linux
            #if JSL_IS_LINUX
                jsl_subprocess_arg_cstr(compile_command, "-D_XOPEN_SOURCE=700");
            #endif

            // add clang warnings
            for (int32_t flag_idx = 0;; ++flag_idx)
            {
                char* flag = clang_warning_flags[flag_idx];
                if (flag == NULL)
                    break;

                jsl_subprocess_arg_cstr(compile_command, flag);
            }

            for (int32_t source_file_idx = 0;; ++source_file_idx)
            {
                char* source_file = unit_test->files[source_file_idx];
                if (source_file == NULL)
                    break;

                jsl_subprocess_arg_cstr(compile_command, source_file);
            }

            test_executables[compile_idx] = exe_name;
            ++compile_idx;
        }

        #if JSL_IS_WINDOWS

            /**
             * Loop over the MSVC configs creating compile commands for each.
             *
             * Also record the exe name in the list of executables to be run
             * later.
             */

            for (int32_t config_idx = 0; config_idx < msvc_config_count; ++config_idx)
            {
                CompilerConfig* compiler_config = &msvc_configs[config_idx];

                JSLImmutableMemory exe_name = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("tests\\bin\\%s%s.exe"),
                    compiler_config->prefix,
                    unit_test->executable_name
                );

                JSLImmutableMemory obj_dir = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("tests\\bin\\%s%s_obj\\"),
                    compiler_config->prefix,
                    unit_test->executable_name
                );

                jsl_make_directory(obj_dir, NULL);

                JSLImmutableMemory exe_output_param = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("/Fe%y"),
                    exe_name
                );
                JSLImmutableMemory obj_output_param = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("/Fo%y"),
                    obj_dir
                );
                JSLImmutableMemory pdb_output_param = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("/Fdtests\\bin\\%s%s.pdb"),
                    compiler_config->prefix,
                    unit_test->executable_name
                );

                JSLSubprocess* compile_command = &test_compile_procs[compile_idx];
                jsl_subprocess_init(
                    compile_command,
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("cl.exe")
                );

                // add compiler flags from config
                for (int32_t flag_idx = 0;; ++flag_idx)
                {
                    char* flag = compiler_config->flags[flag_idx];
                    if (flag == NULL)
                        break;

                    jsl_subprocess_arg_cstr(compile_command, flag);
                }

                jsl_subprocess_arg(
                    compile_command,
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

                    jsl_subprocess_arg_cstr(compile_command, source_file);
                }

                test_executables[compile_idx] = exe_name;
                ++compile_idx;
            }

        #elif JSL_IS_POSIX

            {
                JSLImmutableMemory exe_name = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("tests/bin/debug_gcc_%s.out"),
                    unit_test->executable_name
                );

                JSLSubprocess* compile_command = &test_compile_procs[compile_idx];
                jsl_subprocess_init(
                    compile_command,
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("gcc")
                );

                jsl_subprocess_arg_cstr(
                    compile_command,
                    "-O0",
                    "-g",
                    "-std=c11",
                    "-Isrc/",
                    "-Wall",
                    "-Wextra",
                    "-pedantic",
                    "-fsanitize=address"
                );

                #if JSL_IS_LINUX
                    jsl_subprocess_arg_cstr(compile_command, "-D_XOPEN_SOURCE=700");
                #endif

                jsl_subprocess_arg(
                    compile_command,
                    JSL_CSTR_EXPRESSION("-o"),
                    exe_name
                );

                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;

                    jsl_subprocess_arg_cstr(compile_command, source_file);
                }

                test_executables[compile_idx] = exe_name;
                ++compile_idx;
            }

            {
                JSLImmutableMemory exe_name = jsl_format(
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("tests/bin/opt_gcc_%s.out"),
                    unit_test->executable_name
                );

                JSLSubprocess* compile_command = &test_compile_procs[compile_idx];
                jsl_subprocess_init(
                    compile_command,
                    build_memory_interface,
                    JSL_CSTR_EXPRESSION("gcc")
                );

                jsl_subprocess_arg_cstr(
                    compile_command,
                    "-O3",
                    "-march=native",
                    "-std=c11",
                    "-Isrc/",
                    "-Wall",
                    "-Wextra",
                    "-pedantic"
                );

                #if JSL_IS_LINUX
                    jsl_subprocess_arg_cstr(compile_command, "-D_XOPEN_SOURCE=700");
                #endif

                jsl_subprocess_arg(
                    compile_command,
                    JSL_CSTR_EXPRESSION("-o"),
                    exe_name
                );

                for (int32_t source_file_idx = 0;; ++source_file_idx)
                {
                    char* source_file = unit_test->files[source_file_idx];
                    if (source_file == NULL)
                        break;

                    jsl_subprocess_arg_cstr(compile_command, source_file);
                }

                test_executables[compile_idx] = exe_name;
                ++compile_idx;
            }

        #else
            #error "Unrecognized platform. Only windows and POSIX platforms are supported."
        #endif
    }

    JSLSubProcessResultEnum compile_run_res = jsl_subprocess_run_blocking(
        test_compile_procs,
        total_compile_count,
        build_memory_interface,
        &last_errno
    );
    if (compile_run_res != JSL_SUBPROCESS_SUCCESS)
    {
        jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Compiling unit tests failed with result %d errno %d"), compile_run_res, last_errno);
        return EXIT_FAILURE;
    }

    // A compiler that ran but reported errors exits non-zero. The batch still
    // "succeeds" from the runner's point of view, so check each proc's exit
    // code and fail the suite if any compile failed.
    for (int32_t proc_idx = 0; proc_idx < total_compile_count; ++proc_idx)
    {
        if (test_compile_procs[proc_idx].exit_code != 0)
        {
            jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Compiling %y failed with exit code %d"), test_executables[proc_idx], test_compile_procs[proc_idx].exit_code);
            return EXIT_FAILURE;
        }
    }

    for (int32_t exe_idx = 0; exe_idx < total_compile_count; ++exe_idx)
    {
        #if JSL_IS_WINDOWS
            JSLImmutableMemory run_path = test_executables[exe_idx];
        #else
            JSLImmutableMemory run_path = jsl_format(
                build_memory_interface,
                JSL_CSTR_EXPRESSION("./%y"),
                test_executables[exe_idx]
            );
        #endif

        JSLSubprocess run_command;
        JSL_ZERO_STRUCT(run_command);
        jsl_subprocess_init(&run_command, build_memory_interface, run_path);

        JSLSubProcessResultEnum run_res = jsl_subprocess_run_blocking(
            &run_command,
            1,
            build_memory_interface,
            &last_errno
        );
        if (run_res != JSL_SUBPROCESS_SUCCESS || run_command.exit_code != 0)
        {
            jsl_format_sink(stderr_sink, JSL_CSTR_EXPRESSION("Test %y failed with result %d exit code %d errno %d"), test_executables[exe_idx], run_res, run_command.exit_code, last_errno);
            return EXIT_FAILURE;
        }
    }

    return 0;
}

/**
 * # Build The CLI Tools
 * 
 * TODO: docs
 */

#define NOB_IMPLEMENTATION
#include "../vendor/nob.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSL_CORE_IMPLEMENTATION
#include "../src/jsl_core.h"

#define JSL_FILES_IMPLEMENTATION
#include "../src/jsl_files.h"

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

static JSLFatPtr help_message = JSL_FATPTR_INITIALIZER(""
    "OVERVIEW:\n\n"
    "Build program for the tool programs.\n\n"
    "This program builds the tooling programs as either command line programs\n"
    "or static library files (.a or .lib).\n\n"
    "USAGE:\n\n"
    "\tbuild [--library | --program]\n\n"
    "Required arguments:\n"
    "\t--library\t\tBuild static library files for the host OS\n"
    "\t--program\t\tBuild the command line programs\n"
);

static JSLFatPtr h_flag_str = JSL_FATPTR_INITIALIZER("-h");
static JSLFatPtr help_flag_str = JSL_FATPTR_INITIALIZER("--help");
static JSLFatPtr library_flag_str = JSL_FATPTR_INITIALIZER("--library");
static JSLFatPtr program_flag_str = JSL_FATPTR_INITIALIZER("--program");

void write_template_files(JSLArena* arena)
{
    JSLFatPtr header_template_path = JSL_FATPTR_EXPRESSION("tools/src/templates/static_hash_map_header.txt");
    JSLFatPtr header_output_path = JSL_FATPTR_EXPRESSION("tools/src/templates/static_hash_map_header.h");


    const char* header_template[] = {"tools/src/templates/static_hash_map_header.txt"};
    bool build_header_data = nob_needs_rebuild("tools/src/templates/static_hash_map_header.h", header_template, 1U);
    if (build_header_data)
    {
        
    }
}

int32_t build_library_files(JSLArena* arena)
{
    write_template_files(arena);

    Nob_Cmd generate_hash_map_compile_command = {0};
    nob_cmd_append(
        &generate_hash_map_compile_command,
        "clang",
        "-O3",
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
        "-o", "tools/dist/generate_hash_map.o",
        "-Isrc/",
        "tools/src/generate_hash_map.c"
    );

    if (!nob_cmd_run(&generate_hash_map_compile_command)) return EXIT_FAILURE;

    // Generate archive file
    Nob_Cmd generate_hash_map_build_library = {0};
    nob_cmd_append(
        &generate_hash_map_build_library,
        "ar",
        "rcs",
        "tools/dist/generate_hash_map.a",
        "tools/dist/generate_hash_map.o"
    );

    return EXIT_SUCCESS;
}

int32_t build_program_files(JSLArena* arena)
{
    write_template_files(arena);

    Nob_Cmd generate_hash_map_compile_command = {0};
    nob_cmd_append(
        &generate_hash_map_compile_command,
        "clang",
        "-O3",
        "-DINCLUDE_MAIN",
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
        "-o", "tools/dist/generate_hash_map",
        "-Isrc/",
        "tools/src/generate_hash_map.c"
    );

    if (!nob_cmd_run(&generate_hash_map_compile_command)) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

int32_t main(int32_t argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!nob_mkdir_if_not_exists("tools/dist")) return EXIT_FAILURE;

    JSLArena arena;
    jsl_arena_init(&arena, malloc(JSL_MEGABYTES(32)), JSL_MEGABYTES(32));

    bool show_help = false;
    bool build_libraries = false;
    bool build_programs = false;

    for (int32_t i = 1; i < argc; ++i)
    {
        JSLFatPtr arg = jsl_fatptr_from_cstr(argv[i]); 

        if (
            jsl_fatptr_memory_compare(arg, h_flag_str)
            || jsl_fatptr_memory_compare(arg, help_flag_str)
        )
        {
            show_help = true;
        }
        else if (jsl_fatptr_memory_compare(arg, library_flag_str))
        {
            build_libraries = true;
        }
        else if (jsl_fatptr_memory_compare(arg, program_flag_str))
        {
            build_programs = true;
        }
        else
        {
            jsl_format_file(stdout, JSL_FATPTR_EXPRESSION("Unknown argument %y\n"), arg);
        }
    }

    if (show_help)
    {
        jsl_format_file(stdout, help_message);
        return EXIT_SUCCESS;
    }
    else if (build_libraries && build_programs)
    {
        jsl_format_file(stdout, JSL_FATPTR_EXPRESSION("Cannot specify both --library and --program\n"));
        return EXIT_FAILURE;
    }
    else if (build_libraries)
    {
        return build_library_files(&arena);
    }
    else if (build_programs)
    {
        return build_program_files(&arena);
    }
    else
    {
        jsl_format_file(stdout, JSL_FATPTR_EXPRESSION("Must specify either --library or --program\n"));
        return EXIT_FAILURE;
    }
}

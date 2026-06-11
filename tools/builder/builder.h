/**
 * # JSL Builder
 * 
 * Tools you need to write your build program (a.k.a metaprogram) in C.
 * 
 * This is a fork of nob.h by Alexey Kutepov
 *
 * ## Why
 *
 * C and C++ programs are not buildable from their source files. They require some
 * external definition of how the build will happen in order to produce a
 * runnable program. This is actually a huge problem in practice, as in real
 * world projects the build is never simple. Debug vs release builds,
 * pre-processor defines, include paths, OS specific source files, incremental
 * builds, are all examples of things that normal projects will all have to
 * handle at some point.
 *
 * This means that the external build definition must be defined in a turing complete
 * programming language. There's no way around it. Build systems that don't 
 * have complete programming languages (e.g. GNUMake) inevitably need to call
 * out to platform specific batch scripting languages or do a pre-build configure
 * step (e.g. Autoconf) to build the build definition which is fed into the build
 * system which then actually builds the program.
 *
 * The logic of this system is thus: if you're going to need to write your
 * build definition in a programming language, just write it in the
 * programming language you already use. You define the logic that's needed
 * to build an executable of your program in C. You build and run the C file,
 * that program then calls what ever you want in order to build your program.
 * Ideally you'd just invoke the C compiler directly, but you can do what ever
 * ill advised thing you want, like generating a Makefile and then calling
 * make.
 * 
 * The alternative is to use some other language that is not nearly as simple
 * and you're not nearly as familiar with. CMake asks you to learn and remember a
 * CMake specific programming language that generates build definitions for other
 * build systems. So when the build breaks you have to edit this language that
 * you barely know. Now you have two dependencies: the specific version of CMake
 * and then the specific version of the underlying build system. Awesome. And
 * imagine how much fun future you will have 15 years from now trying to find, build,
 * and install a specific version of CMake on a machine that it wasn't designed
 * to run on.
 * 
 * C on the other hand is supported everywhere. The tool chain is easy to install
 * on every system. Every serious C compiler has C89, C99, and C11 modes and there's
 * no reason to suspect that future C versions won't also get the same backwards
 * compatibility treatment. All this to say: getting a single C file to build on some
 * future computer will almost certainly be a lot easier than any of the alternatives.  
 * 
 * ## How
 * 
 * First you write your build program. This is just a series of steps that the computer
 * has to follow to build your program; just treat it like any other problem, builds
 * aren't special.
 * 
 * JSL already provides most of the tools you need:
 * 
 *      * Host OS detection
 *      * File reads/writes
 *      * Creating/Deleting directories (recursively if needed)
 *      * Directory iteration
 *      * Command line parsing
 *      * Subprocesses
 * 
 * What this file provides is,
 *      
 *      * Detection if something needs a rebuild
 *      * Auto bootstrapping your build file when it changes
 * 
 * ### Example
 * 
 * ```
 * #include <stdlib.h>
 * #include "jsl/everything.c"
 * #define JSL_BUILDER_IMPLEMENTATION
 * #include "builder.h"
 * 
 * int main(int argc, char** argv)
 * {
 *     BUILDER_AUTO_BOOTSTRAP();
 * 
 *     static char* source_paths[] = {
 *         "main.c",
 *         "logic.c",
 *         "display.c"
 *     };
 *     static int source_file_count = sizeof(source_paths) / sizeof(source_paths[0]);
 * 
 *     bool needs_rebuild = is_build_stale("my_program", source_paths, source_file_count);
 *     if (needs_rebuild)
 *     {
 *         JSLInfiniteArena build_memory;
 *         JSLAllocatorInterface build_memory_interface;
 *         jsl_infinite_arena_init(&build_memory);
 *         jsl_infinite_arena_get_allocator_interface(&build_memory_interface, &build_memory);
 * 
 *         JSLSubprocess compile_proc;
 *         JSL_ZERO_STRUCT(compile_proc);
 * 
 *         jsl_subprocess_init(&compile_proc, build_memory_interface, JSL_CSTR_EXPRESSION("clang"));
 *         jsl_subprocess_arg_cstr(
 *             &compile_proc,
 *             "-O0",
 *             "-g",
 *             "-std=c11",
 *             "-o", "my_program"
 *         );
 * 
 * 
 *         for (int i = 0; i < source_file_count; ++i)
 *         {
 *             jsl_subprocess_arg_cstr(&compile_proc, source_paths[i]);
 *         }
 * 
 *         JSLSubProcessResultEnum run_res = jsl_subprocess_run_blocking(build_memory_interface, &compile_proc, 1, NULL);
 *         if (run_res != JSL_SUBPROCESS_SUCCESS)
 *         {
 *             return EXIT_FAILURE;
 *         }
 *     }
 * 
 *     return EXIT_SUCCESS;
 * }
 * ```
 * 
 * You might think to yourself, "this is quite a bit of code for something that would
 * take five lines in a Makefile". But I'd encourage you to think about the rationals
 * given above. As soon as you do anything slightly outside of "compile these c files
 * into an executable" you'll be back to writing build logic in scripts.
 * 
 * ## Original Copyright
 * 
 * Copyright (c) 2024 Alexey Kutepov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef JSL_BUILDER_H_
    #define JSL_BUILDER_H_

    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include <stdint.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <stdarg.h>
    #include <string.h>
    #include <errno.h>
    #include <ctype.h>
    #include <limits.h>
    #include <time.h>

    #include "jsl/core.h"
    #include "jsl/os.h"
    #include "jsl/allocator_libc.h"

    /**
     * Is the given output file older than any of the given input files.
     * 
     * This function is very pessimistic. It will return true not only based
     * on age but if there is any error from the OS when grabbing all of the
     * needed info.
     */
    bool is_build_stale(const char* output_path, const char** input_paths, size_t input_paths_count);

    /**
     *
     * How to use it:
     *     int32_t main(int32_t argc, char** argv) {
     *         BUILDER_AUTO_BOOTSTRAP(argc, argv);
     *         // actual work
     *         return 0;
     *     }
     *
     * After you added this macro every time you run ./nob it will detect
     * that you modified its original source code and will try to rebuild itself
     * before doing any actual work. So you only need to bootstrap your build system
     * once.
     *
     * The modification is detected by comparing the last modified times of the executable
     * and its source code. The same way the make utility usually does it.
     *
     * The rebuilding is done by using the JSL_BUILDER_REBUILD_URSELF macro which you can redefine
     * if you need a special way of bootstraping your build system. (which I personally
     * do not recommend since the whole idea of NoBuild is to keep the process of bootstrapping
     * as simple as possible and doing all of the actual work inside of ./nob)
     */
    void builder__auto_bootstrap(int32_t argc, char **argv, const char *source_path, ...);
    #define BUILDER_AUTO_BOOTSTRAP(argc, argv) builder__auto_bootstrap(argc, argv, __FILE__, NULL)
    
    // Sometimes your nob.c includes additional files, so you want the Go Rebuild Urself™ Technology to check
    // if they also were modified and rebuild nob.c accordingly. For that we have BUILDER_AUTO_BOOTSTRAP_FILES():
    // ```c
    // #define JSL_BUILDER_IMPLEMENTATION
    // #include "nob.h"
    //
    // #include "foo.c"
    // #include "bar.c"
    //
    // int32_t main(int32_t argc, char **argv)
    // {
    //     BUILDER_AUTO_BOOTSTRAP_FILES(argc, argv, "foo.c", "bar.c");
    //     // ...
    //     return 0;
    // }
    #define BUILDER_AUTO_BOOTSTRAP_FILES(argc, argv, ...) JSL_BUILDER__go_rebuild_urself(argc, argv, __FILE__, __VA_ARGS__, NULL);

    #ifndef BUILDER_COMPILER
        #if defined(_WIN32)
            #if defined(__clang__)
                #define BUILDER_COMPILER JSL_CSTR_EXPRESSION("clang")
            #elif defined(__GNUC__)
                #define BUILDER_COMPILER JSL_CSTR_EXPRESSION("gcc")
            #elif defined(_MSC_VER)
                #define BUILDER_COMPILER JSL_CSTR_EXPRESSION("cl.exe")
            #elif defined(__TINYC__)
                #define BUILDER_COMPILER JSL_CSTR_EXPRESSION("tcc")
            #endif
        #else
            #if defined(__cplusplus)
                #define BUILDER_COMPILER JSL_CSTR_EXPRESSION("cc")
            #else
                #define BUILDER_COMPILER JSL_CSTR_EXPRESSION("cc")
            #endif
        #endif
    #endif

    #ifndef BUILDER_SELF_REBUILD
        #if defined(_WIN32)
            #if defined(__clang__)
                #if defined(__cplusplus)
                    #define BUILDER_SELF_REBUILD(binary_path, source_path) "clang", "-x", "c++", "-o", binary_path, source_path
                #else
                    #define BUILDER_SELF_REBUILD(binary_path, source_path) "clang", "-x", "c", "-o", binary_path, source_path
                #endif
            #elif defined(__GNUC__)
                #if defined(__cplusplus)
                    #define BUILDER_SELF_REBUILD(binary_path, source_path) "gcc", "-x", "c++", "-o", binary_path, source_path
                #else
                    #define BUILDER_SELF_REBUILD(binary_path, source_path) "gcc", "-x", "c", "-o", binary_path, source_path
                #endif
            #elif defined(_MSC_VER)
                #define BUILDER_SELF_REBUILD(binary_path, source_path) "cl.exe", nob_temp_sprintf("/Fe:%s", (binary_path)), source_path
            #elif defined(__TINYC__)
                #define BUILDER_SELF_REBUILD(binary_path, source_path) "tcc", "-o", binary_path, source_path
            #endif
        #else
            #if defined(__cplusplus)
                #define BUILDER_SELF_REBUILD(binary_path, source_path) "cc", "-x", "c++", "-o", binary_path, source_path
            #else
                #define BUILDER_SELF_REBUILD(binary_path, source_path) "cc", "-x", "c", "-o", binary_path, source_path
            #endif
        #endif
    #endif

#endif // JSL_BUILDER_H_

#ifdef JSL_BUILDER_IMPLEMENTATION

    struct BuilderArrayOfStrings
    {
        const char** items;
        int64_t count;
        int64_t capacity;
        JSLAllocatorInterface allocator;
        bool failed;
    };

    static void builder_array_of_strings_init(struct BuilderArrayOfStrings* arr, JSLAllocatorInterface allocator)
    {
        arr->items = NULL;
        arr->count = 0;
        arr->capacity = 0;
        arr->allocator = allocator;
        arr->failed = false;
    }

    static void builder_array_of_strings_insert(struct BuilderArrayOfStrings* arr, const char* string)
    {
        bool need_grow = !arr->failed && arr->count >= arr->capacity;
        int64_t new_capacity = need_grow
            ? (arr->capacity == 0 ? 8 : arr->capacity * 2)
            : arr->capacity;

        const char** new_items = arr->items;
        if (need_grow)
        {
            new_items = (const char**) jsl_allocator_interface_realloc(
                arr->allocator,
                arr->items,
                new_capacity * (int64_t) sizeof(char*),
                JSL_DEFAULT_ALLOCATION_ALIGNMENT
            );
        }

        bool alloc_failed = need_grow && new_items == NULL;
        if (alloc_failed)
        {
            arr->failed = true;
        }

        bool grew_ok = need_grow && !alloc_failed;
        if (grew_ok)
        {
            arr->items = new_items;
            arr->capacity = new_capacity;
        }

        if (!arr->failed)
        {
            arr->items[arr->count] = string;
            arr->count += 1;
        }
    }

    bool is_build_stale(const char *output_path, const char **input_paths, size_t input_paths_count)
    {
        #ifdef _WIN32
            BOOL bSuccess;

            HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
            if (output_path_fd == INVALID_HANDLE_VALUE)
            {
                return true;
            }
            FILETIME output_path_time;
            bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
            CloseHandle(output_path_fd);
            if (!bSuccess)
            {
                return true;
            }

            for (size_t i = 0; i < input_paths_count; ++i)
            {
                const char *input_path = input_paths[i];
                HANDLE input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
                if (input_path_fd == INVALID_HANDLE_VALUE)
                {
                    // NOTE: non-existing input is an error cause it is needed for building in the first place
                    return true;
                }
                FILETIME input_path_time;
                bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
                CloseHandle(input_path_fd);
                if (!bSuccess)
                {
                    return true;
                }

                // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
                if (CompareFileTime(&input_path_time, &output_path_time) == 1)
                    return true;
            }

            return false;
        #else
            struct stat statbuf = {0};

            if (stat(output_path, &statbuf) < 0)
            {
                return true;
            }
            time_t output_path_time = statbuf.st_mtime;

            for (size_t i = 0; i < input_paths_count; ++i)
            {
                const char *input_path = input_paths[i];
                if (stat(input_path, &statbuf) < 0)
                {
                return true;
                }
                time_t input_path_time = statbuf.st_mtime;
                // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
                if (input_path_time > output_path_time)
                    return true;
            }

            return false;
        #endif
    }

    // The implementation idea is stolen from https://github.com/zhiayang/nabs
    void builder__auto_bootstrap(int32_t argc, char **argv, const char *source_path, ...)
    {
        static JSLImmutableMemory exe_str = JSL_CSTR_INITIALIZER(".exe");

        JSLLibcAllocator allocator;
        JSLAllocatorInterface alloc_interface;

        jsl_libc_allocator_init(&allocator);
        jsl_libc_allocator_get_allocator_interface(&alloc_interface, &allocator);

        const char* binary_path = argv[0];

        #ifdef _WIN32
            // On Windows executables almost always invoked without extension, so
            // it's ./nob, not ./nob.exe. For renaming the extension is a must.
            const bool ends_with_exe = jsl_ends_with(binary_path_cstr, exe_str);
            if (!ends_with_exe)
            {
                binary_path = jsl_format(alloc_interface, "%y.exe", binary_path);
            }
        #endif

        struct BuilderArrayOfStrings source_paths;
        JSL_ZERO_STRUCT(source_paths);

        builder_array_of_strings_init(&source_paths, alloc_interface);
        builder_array_of_strings_insert(&source_paths, source_path);

        va_list args;
        va_start(args, source_path);
        for (;;)
        {
            const char *path = va_arg(args, const char *);
            if (path == NULL)
                break;

            builder_array_of_strings_insert(&source_paths, path);
        }
        va_end(args);

        int32_t rebuild_is_needed = is_build_stale(binary_path, source_paths.items, source_paths.count);
        if (rebuild_is_needed < 0)
        {
            exit(EXIT_FAILURE);
        }

        if (!rebuild_is_needed)
        {
            jsl_libc_allocator_free_all(&allocator);
            return;
        }

        JSLImmutableMemory old_binary_path = jsl_format(alloc_interface, JSL_CSTR_EXPRESSION("%s.old"), binary_path);
        const char* old_binary_path_cstr = jsl_memory_to_cstr(alloc_interface, old_binary_path); 

        JSLRenameFileResultEnum rename_res = jsl_rename_file(jsl_cstr_to_memory(binary_path), old_binary_path, NULL);
        if (rename_res != JSL_RENAME_FILE_SUCCESS)
        {
            exit(EXIT_FAILURE);
        }

        // Self rebuild
        {
            JSLSubprocess self_rebuild_cmd;
            JSL_ZERO_STRUCT(self_rebuild_cmd);

            JSLSubProcessCreateResultEnum subprocess_create_res = jsl_subprocess_init(&self_rebuild_cmd, alloc_interface, BUILDER_COMPILER);
            if (subprocess_create_res != JSL_SUBPROCESS_CREATE_SUCCESS)
            {
                exit(EXIT_FAILURE);
            }

            jsl_subprocess_arg_cstr(&self_rebuild_cmd, BUILDER_SELF_REBUILD(binary_path, source_path));

            JSLSubProcessResultEnum run_res = jsl_subprocess_run_blocking(&self_rebuild_cmd, 1, alloc_interface, NULL);
            if (run_res != JSL_SUBPROCESS_SUCCESS)
            {
                jsl_rename_file(old_binary_path, jsl_cstr_to_memory(binary_path), NULL);
                exit(EXIT_FAILURE);
            }
        }

        // Run the rebuild
        {
            JSLSubprocess run_rebuild_cmd;
            JSL_ZERO_STRUCT(run_rebuild_cmd);

            JSLSubProcessCreateResultEnum subprocess_create_res = jsl_subprocess_init(&run_rebuild_cmd, alloc_interface, jsl_cstr_to_memory(binary_path));
            if (subprocess_create_res != JSL_SUBPROCESS_CREATE_SUCCESS)
            {
                exit(EXIT_FAILURE);
            }

            jsl_subprocess_arg_cstr(&run_rebuild_cmd, binary_path);
            for (int32_t i = 0; i < argc; i++)
            {
                jsl_subprocess_arg_cstr(&run_rebuild_cmd, argv[i]);
            }

            JSLSubProcessResultEnum run_res = jsl_subprocess_run_blocking(&run_rebuild_cmd, 1, alloc_interface, NULL);
            if (run_res != JSL_SUBPROCESS_SUCCESS)
            {
                exit(EXIT_FAILURE);
            }
        }

        exit(EXIT_SUCCESS);
    }

#endif // JSL_BUILDER_IMPLEMENTATION

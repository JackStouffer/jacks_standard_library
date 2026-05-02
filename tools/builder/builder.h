/**
 * # JSL Builder
 * 
 * Tools you need to write your build program (a.k.a metaprogram) in C.
 * 
 * This is a fork of nob.h by Alexey Kutepov
 * 
 * ## Why
 *
 * C programs are not buildable from their source files. They require some
 * external definition of how the build will happen in order to produce a
 * runnable program. This is actually a huge problem in practice, as in real
 * world projects the build is never simple. Debug vs release builds,
 * pre-processor defines, include paths, OS specific source files, incremental
 * builds, are all examples of things that normal projects will all have to
 * handle at some point.
 *
 * This means that the external build definition must become a turing complete
 * programming language. There's no way around it. Build systems that don't 
 * have complete programming languages (e.g. GNUMake) inevitably need to call
 * out to platform specific batch scripting languages or do a pre-build configure
 * step (Autoconf) to build the build definition which builds the program.
 *
 * The logic of this system is thus: if you're going to need to write your
 * build definition in a programming language, just write it in the
 * programming language you already use. You define the logic that's needed
 * to build an executable of your program in C. You build and run the C file,
 * that program then calls what ever you want in order to build your program.
 * Ideally you'd just make calls to the C compiler, but you can do what ever
 * ill advised thing you want, like generating a Makefile and then calling
 * make.
 * 
 * The alternative is to use some other language that is not nearly as simple
 * and you're not nearly as familiar with. CMake asks you to learn and remember a
 * CMake specific programming language that generates build definitions for other
 * build systems. So when the build breaks you have to edit this language that
 * you barely know and now you have two dependencies: the specific version of CMake
 * and then the specific version of the underlying build system. Awesome. And
 * imagine how much fun future you will have trying to find, build, and install a
 * specific version of CMake 20 years from now on a machine that it wasn't designed
 * to run on.
 * 
 * ## How
 * 
 * First you write your build program. 
 * 
 * JSL already provides most of the tools you need:
 * 
 *      * OS detection
 *      * File reads/writes
 *      * Creating/Deleting directories (recursively if needed)
 *      * Directory iteration
 *      * Subprocesses
 * 
 * What this file provides is,
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

    #include <stdbool.h>
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

    int32_t builder_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);

    // Go Rebuild Urself™ Technology
    //
    //   How to use it:
    //     int32_t main(int32_t argc, char** argv) {
    //         JSL_BUILDER_GO_REBUILD_URSELF(argc, argv);
    //         // actual work
    //         return 0;
    //     }
    //
    //   After you added this macro every time you run ./nob it will detect
    //   that you modified its original source code and will try to rebuild itself
    //   before doing any actual work. So you only need to bootstrap your build system
    //   once.
    //
    //   The modification is detected by comparing the last modified times of the executable
    //   and its source code. The same way the make utility usually does it.
    //
    //   The rebuilding is done by using the JSL_BUILDER_REBUILD_URSELF macro which you can redefine
    //   if you need a special way of bootstraping your build system. (which I personally
    //   do not recommend since the whole idea of NoBuild is to keep the process of bootstrapping
    //   as simple as possible and doing all of the actual work inside of ./nob)
    //
    void builder__go_rebuild_urself(int32_t argc, char **argv, const char *source_path, ...);
    #define JSL_BUILDER_GO_REBUILD_URSELF(argc, argv) builder__go_rebuild_urself(argc, argv, __FILE__, NULL)
    // Sometimes your nob.c includes additional files, so you want the Go Rebuild Urself™ Technology to check
    // if they also were modified and rebuild nob.c accordingly. For that we have JSL_BUILDER_GO_REBUILD_URSELF_PLUS():
    // ```c
    // #define JSL_BUILDER_IMPLEMENTATION
    // #include "nob.h"
    //
    // #include "foo.c"
    // #include "bar.c"
    //
    // int32_t main(int32_t argc, char **argv)
    // {
    //     JSL_BUILDER_GO_REBUILD_URSELF_PLUS(argc, argv, "foo.c", "bar.c");
    //     // ...
    //     return 0;
    // }
    #define JSL_BUILDER_GO_REBUILD_URSELF_PLUS(argc, argv, ...) JSL_BUILDER__go_rebuild_urself(argc, argv, __FILE__, __VA_ARGS__, NULL);

#endif // JSL_BUILDER_H_

#ifdef JSL_BUILDER_IMPLEMENTATION

    int32_t builder_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count)
    {
    #ifdef _WIN32
        BOOL bSuccess;

        HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (output_path_fd == INVALID_HANDLE_VALUE)
        {
            // NOTE: if output does not exist it 100% must be rebuilt
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                return 1;
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s: %s", output_path, JSL_BUILDER_win32_error_message(GetLastError()));
            return -1;
        }
        FILETIME output_path_time;
        bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
        CloseHandle(output_path_fd);
        if (!bSuccess)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not get time of %s: %s", output_path, JSL_BUILDER_win32_error_message(GetLastError()));
            return -1;
        }

        for (size_t i = 0; i < input_paths_count; ++i)
        {
            const char *input_path = input_paths[i];
            HANDLE input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
            if (input_path_fd == INVALID_HANDLE_VALUE)
            {
                // NOTE: non-existing input is an error cause it is needed for building in the first place
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s: %s", input_path, JSL_BUILDER_win32_error_message(GetLastError()));
                return -1;
            }
            FILETIME input_path_time;
            bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
            CloseHandle(input_path_fd);
            if (!bSuccess)
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not get time of %s: %s", input_path, JSL_BUILDER_win32_error_message(GetLastError()));
                return -1;
            }

            // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
            if (CompareFileTime(&input_path_time, &output_path_time) == 1)
                return 1;
        }

        return 0;
    #else
        struct stat statbuf = {0};

        if (stat(output_path, &statbuf) < 0)
        {
            // NOTE: if output does not exist it 100% must be rebuilt
            if (errno == ENOENT)
                return 1;
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not stat %s: %s", output_path, strerror(errno));
            return -1;
        }
        time_t output_path_time = statbuf.st_mtime;

        for (size_t i = 0; i < input_paths_count; ++i)
        {
            const char *input_path = input_paths[i];
            if (stat(input_path, &statbuf) < 0)
            {
                // NOTE: non-existing input is an error cause it is needed for building in the first place
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not stat %s: %s", input_path, strerror(errno));
                return -1;
            }
            time_t input_path_time = statbuf.st_mtime;
            // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
            if (input_path_time > output_path_time)
                return 1;
        }

        return 0;
    #endif
    }

    // The implementation idea is stolen from https://github.com/zhiayang/nabs
    void builder__go_rebuild_urself(int32_t argc, char **argv, const char *source_path, ...)
    {
        const char *binary_path = JSL_BUILDER_shift(argv, argc);
        #ifdef _WIN32
            // On Windows executables almost always invoked without extension, so
            // it's ./nob, not ./nob.exe. For renaming the extension is a must.
            if (!JSL_BUILDER_sv_ends_with_cstr(JSL_BUILDER_sv_from_cstr(binary_path), ".exe"))
            {
                binary_path = JSL_BUILDER_temp_sprintf("%s.exe", binary_path);
            }
        #endif

        JSL_BUILDER_File_Paths source_paths = {0};
        JSL_BUILDER_da_append(&source_paths, source_path);
        va_list args;
        va_start(args, source_path);
        for (;;)
        {
            const char *path = va_arg(args, const char *);
            if (path == NULL)
                break;
            JSL_BUILDER_da_append(&source_paths, path);
        }
        va_end(args);

        int32_t rebuild_is_needed = JSL_BUILDER_needs_rebuild(binary_path, source_paths.items, source_paths.count);
        if (rebuild_is_needed < 0)
            exit(1); // error
        if (!rebuild_is_needed)
        { // no rebuild is needed
            JSL_BUILDER_FREE(source_paths.items);
            return;
        }

        JSL_BUILDER_Cmd cmd = {0};

        const char *old_binary_path = JSL_BUILDER_temp_sprintf("%s.old", binary_path);

        if (!JSL_BUILDER_rename(binary_path, old_binary_path))
            exit(1);
        JSL_BUILDER_cmd_append(&cmd, JSL_BUILDER_REBUILD_URSELF(binary_path, source_path));
        JSL_BUILDER_Cmd_Opt opt = {0};
        if (!JSL_BUILDER_cmd_run_opt(&cmd, opt))
        {
            JSL_BUILDER_rename(old_binary_path, binary_path);
            exit(1);
        }

        JSL_BUILDER_cmd_append(&cmd, binary_path);
        JSL_BUILDER_da_append_many(&cmd, argv, argc);
        if (!JSL_BUILDER_cmd_run_opt(&cmd, opt))
            exit(1);
        exit(0);
    }

#endif // JSL_BUILDER_IMPLEMENTATION

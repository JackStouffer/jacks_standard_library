/**
 * Copyright (c) 2026 Jack Stouffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define _GNU_SOURCE
#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "jsl/core.h"
#include "jsl/allocator.h"
#include "jsl/allocator_arena.h"
#include "jsl/os.h"

#if JSL_IS_LINUX
    #include <signal.h>
    #include <sys/stat.h>
#endif

#include "minctest.h"
#include "test_file_utils.h"

void test_jsl_load_file_contents(void)
{
    #if JSL_IS_WINDOWS
        char* path = "tests\\example.txt";
    #else
        char* path = "./tests/example.txt";
    #endif

    char stack_buffer[4*1024] = {0};
    int64_t file_size;

    // Load the comparison using libc
    {
        FILE* file = fopen(path, "rb");
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        TEST_BOOL(file_size > 0);
        rewind(file);

        size_t res = fread(stack_buffer, (size_t) file_size, 1, file);
        assert(res > 0);
    }

    JSLArena arena;
    jsl_arena_init(&arena, malloc(JSL_KILOBYTES(4)), JSL_KILOBYTES(4));
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &arena);

    JSLImmutableMemory contents;
    JSLLoadFileResultEnum res = jsl_load_file_contents(
        allocator,
        jsl_cstr_to_memory(path),
        &contents,
        NULL
    );

    TEST_BOOL(res == JSL_FILE_LOAD_SUCCESS);
    TEST_BUFFERS_EQUAL(stack_buffer, contents.data, (size_t) file_size);
}

void test_jsl_get_file_size(void)
{
    #if JSL_IS_WINDOWS
        char* path = "tests\\example.txt";
    #else
        char* path = "./tests/example.txt";
    #endif

    int64_t size = -1;
    int32_t os_error = 0;
    JSLGetFileSizeResultEnum res = jsl_get_file_size(
        jsl_cstr_to_memory(NULL),
        &size,
        &os_error
    );
    TEST_INT32_EQUAL(res, JSL_GET_FILE_SIZE_BAD_PARAMETERS);

    FILE* file = fopen(path, "rb");
    TEST_BOOL(file != NULL);
    if (file == NULL)
        return;

    int32_t fseek_res = fseek(file, 0, SEEK_END);
    TEST_INT32_EQUAL(fseek_res, 0);
    int64_t expected_size = (int64_t) ftell(file);
    TEST_BOOL(expected_size > 0);
    fclose(file);

    size = -1;
    os_error = 0;
    res = jsl_get_file_size(
        jsl_cstr_to_memory(path),
        &size,
        &os_error
    );

    TEST_INT32_EQUAL(res, JSL_GET_FILE_SIZE_OK);
    TEST_INT32_EQUAL(os_error, 0);
    TEST_INT64_EQUAL(size, (int64_t) expected_size);
}

void test_jsl_load_file_contents_buffer(void)
{
    char* path = "./tests/example.txt";
    char stack_buffer[4*1024];
    int64_t file_size;

    // Load the comparison using libc
    {
        FILE* file = fopen(path, "rb");
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        TEST_BOOL(file_size > 0);
        rewind(file);

        size_t res = fread(stack_buffer, (size_t) file_size, 1, file);
        assert(res > 0);
    }

    JSLMutableMemory buffer = jsl_mutable_memory(malloc(4*1024), 4*1024);
    JSLMutableMemory writer = buffer;

    JSLLoadFileResultEnum res = jsl_load_file_contents_buffer(
        &writer,
        JSL_CSTR_EXPRESSION("./tests/example.txt"),
        NULL
    );

    TEST_BOOL(res == JSL_FILE_LOAD_SUCCESS);
    TEST_BOOL(memcmp(stack_buffer, buffer.data, (size_t) file_size) == 0);
}



#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static int jsl__funopen_write_fail(void* cookie, const char* buf, int len)
{
    (void)cookie;
    (void)buf;
    (void)len;
    errno = EIO;
    return -1;
}

static int jsl__funopen_close(void* cookie)
{
    (void)cookie;
    return 0;
}
#endif

#if defined(__linux__)
static ssize_t jsl__cookie_write_fail(void* cookie, const char* buf, size_t len)
{
    (void)cookie;
    (void)buf;
    (void)len;
    errno = EIO;
    return -1;
}

static int jsl__cookie_close(void* cookie)
{
    (void)cookie;
    return 0;
}
#endif

static FILE* jsl__open_failing_stream(void)
{
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    return funopen(NULL, NULL, jsl__funopen_write_fail, NULL, jsl__funopen_close);
#elif defined(__linux__)
    cookie_io_functions_t functions = {
        .read = NULL,
        .write = jsl__cookie_write_fail,
        .seek = NULL,
        .close = jsl__cookie_close
    };
    return fopencookie(NULL, "w", functions);
#else
    return NULL;
#endif
}


void test_jsl_format_file_formats_and_writes_output(void)
{
    FILE* file = tmpfile();
    TEST_BOOL(file != NULL);
    if (file == NULL)
        return;

    JSLOutputSink sink = jsl_c_file_output_sink(file);

    jsl_format_sink(
        sink,
        JSL_CSTR_EXPRESSION("Hello %s %d"),
        "World",
        42
    );

    TEST_BOOL(fflush(file) == 0);
    TEST_BOOL(fseek(file, 0, SEEK_SET) == 0);

    char buffer[64] = {0};
    size_t read = fread(buffer, 1, sizeof(buffer), file);
    const char* expected = "Hello World 42";
    TEST_BOOL(read == strlen(expected));
    TEST_BOOL(memcmp(buffer, expected, read) == 0);

    fclose(file);
}

void test_jsl_format_file_accepts_empty_format(void)
{
    FILE* file = tmpfile();
    TEST_BOOL(file != NULL);
    if (file == NULL)
        return;

    JSLOutputSink sink = jsl_c_file_output_sink(file);

    jsl_format_sink(sink, JSL_CSTR_EXPRESSION(""));

    TEST_BOOL(fflush(file) == 0);
    TEST_BOOL(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    TEST_BOOL(size == 0);

    fclose(file);
}

void test_jsl_format_file_null_out_parameter(void)
{
    JSLOutputSink sink = jsl_c_file_output_sink(NULL);

    jsl_format_sink(sink, JSL_CSTR_EXPRESSION("Hello"));
}

void test_jsl_format_file_null_format_pointer(void)
{
    JSLImmutableMemory fmt = {
        .data = NULL,
        .length = 5
    };

    JSLOutputSink sink = jsl_c_file_output_sink(stdout);

    jsl_format_sink(sink, fmt);
}

void test_jsl_format_file_negative_length(void)
{
    JSLImmutableMemory fmt = {
        .data = (uint8_t*)"Hello",
        .length = -1
    };
    JSLOutputSink sink = jsl_c_file_output_sink(stdout);

    jsl_format_sink(sink, fmt);
}

#if JSL_IS_WINDOWS
    #define JSL_TEST_RMDIR(p) _rmdir(p)
#else
    #define JSL_TEST_RMDIR(p) rmdir(p)
#endif

void test_jsl_make_directory_bad_parameters(void)
{
    int32_t os_err = 0;

    JSLImmutableMemory null_path = { .data = NULL, .length = 5 };
    JSLMakeDirectoryResultEnum res = jsl_make_directory(null_path, &os_err);
    TEST_INT32_EQUAL(res, JSL_MAKE_DIRECTORY_BAD_PARAMETERS);

    JSLImmutableMemory zero_path = { .data = (const uint8_t*) "x", .length = 0 };
    res = jsl_make_directory(zero_path, &os_err);
    TEST_INT32_EQUAL(res, JSL_MAKE_DIRECTORY_BAD_PARAMETERS);
}

void test_jsl_make_directory_creates_directory(void)
{
    const char* path = "./tests/tmp_make_dir_create";

    // Best-effort cleanup from any prior failed run
    JSL_TEST_RMDIR(path);

    int32_t os_err = 0;
    JSLMakeDirectoryResultEnum res = jsl_make_directory(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_MAKE_DIRECTORY_SUCCESS);

    // Verify it really exists by checking jsl_get_file_size returns
    // not-a-regular-file (directories aren't regular files).
    int64_t size = -1;
    int32_t size_err = 0;
    JSLGetFileSizeResultEnum size_res = jsl_get_file_size(
        jsl_cstr_to_memory(path),
        &size,
        &size_err
    );
    TEST_INT32_EQUAL(size_res, JSL_GET_FILE_SIZE_NOT_REGULAR_FILE);

    int rm_res = JSL_TEST_RMDIR(path);
    TEST_INT32_EQUAL(rm_res, 0);
}

void test_jsl_make_directory_already_exists(void)
{
    const char* path = "./tests/tmp_make_dir_exists";

    JSL_TEST_RMDIR(path);

    int32_t os_err = 0;
    JSLMakeDirectoryResultEnum first = jsl_make_directory(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(first, JSL_MAKE_DIRECTORY_SUCCESS);

    os_err = 0;
    JSLMakeDirectoryResultEnum second = jsl_make_directory(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(second, JSL_MAKE_DIRECTORY_ALREADY_EXISTS);
    TEST_BOOL(os_err != 0);

    int rm_res = JSL_TEST_RMDIR(path);
    TEST_INT32_EQUAL(rm_res, 0);
}

void test_jsl_make_directory_parent_not_found(void)
{
    const char* path = "./tests/tmp_make_dir_no_parent_xyz/child";

    int32_t os_err = 0;
    JSLMakeDirectoryResultEnum res = jsl_make_directory(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_MAKE_DIRECTORY_PARENT_NOT_FOUND);
    TEST_BOOL(os_err != 0);
}

void test_jsl_make_directory_path_too_long(void)
{
    char buffer[FILENAME_MAX + 16];
    for (int64_t i = 0; i < (int64_t) sizeof(buffer); i++)
    {
        buffer[i] = 'a';
    }

    JSLImmutableMemory long_path = {
        .data = (const uint8_t*) buffer,
        .length = (int64_t) FILENAME_MAX
    };

    int32_t os_err = 0;
    JSLMakeDirectoryResultEnum res = jsl_make_directory(long_path, &os_err);
    TEST_INT32_EQUAL(res, JSL_MAKE_DIRECTORY_PATH_TOO_LONG);
}

void test_jsl_get_file_type_regular_file(void)
{
    #if JSL_IS_WINDOWS
        const char* path = "tests\\example.txt";
    #else
        const char* path = "./tests/example.txt";
    #endif

    JSLFileTypeEnum t = jsl_get_file_type(jsl_cstr_to_memory(path));
    TEST_INT32_EQUAL(t, JSL_FILE_TYPE_REG);
}

void test_jsl_get_file_type_directory(void)
{
    #if JSL_IS_WINDOWS
        const char* path = "tests";
    #else
        const char* path = "./tests";
    #endif

    JSLFileTypeEnum t = jsl_get_file_type(jsl_cstr_to_memory(path));
    TEST_INT32_EQUAL(t, JSL_FILE_TYPE_DIR);
}

void test_jsl_get_file_type_bad_parameters(void)
{
    JSLImmutableMemory null_path = { .data = NULL, .length = 5 };
    JSLFileTypeEnum t1 = jsl_get_file_type(null_path);
    TEST_INT32_EQUAL(t1, JSL_FILE_TYPE_UNKNOWN);

    JSLImmutableMemory zero_path = { .data = (const uint8_t*) "x", .length = 0 };
    JSLFileTypeEnum t2 = jsl_get_file_type(zero_path);
    TEST_INT32_EQUAL(t2, JSL_FILE_TYPE_UNKNOWN);
}

void test_jsl_get_file_type_nonexistent(void)
{
    const char* path = "./tests/this_path_does_not_exist_xyz_12345";
    JSLFileTypeEnum t = jsl_get_file_type(jsl_cstr_to_memory(path));
    TEST_INT32_EQUAL(t, JSL_FILE_TYPE_NOT_FOUND);
}

void test_jsl_delete_file_bad_parameters(void)
{
    JSLImmutableMemory null_path = { .data = NULL, .length = 5 };
    JSLDeleteFileResultEnum res = jsl_delete_file(null_path, NULL);
    TEST_INT32_EQUAL(res, JSL_DELETE_FILE_BAD_PARAMETERS);

    JSLImmutableMemory zero_path = { .data = (const uint8_t*) "x", .length = 0 };
    res = jsl_delete_file(zero_path, NULL);
    TEST_INT32_EQUAL(res, JSL_DELETE_FILE_BAD_PARAMETERS);
}

void test_jsl_delete_file_success(void)
{
    const char* path = "./tests/tmp_delete_file_success.txt";

    FILE* f = fopen(path, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
        return;
    fclose(f);

    int32_t os_err = 0;
    JSLDeleteFileResultEnum res = jsl_delete_file(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_FILE_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    // Confirm the file is gone
    JSLFileTypeEnum type = jsl_get_file_type(jsl_cstr_to_memory(path));
    TEST_INT32_EQUAL(type, JSL_FILE_TYPE_NOT_FOUND);
}

void test_jsl_delete_file_not_found(void)
{
    const char* path = "./tests/tmp_delete_file_does_not_exist_xyz_12345.txt";
    int32_t os_err = 0;
    JSLDeleteFileResultEnum res = jsl_delete_file(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_FILE_NOT_FOUND);
    TEST_BOOL(os_err != 0);
}

void test_jsl_delete_file_is_directory(void)
{
    const char* path = "./tests/tmp_delete_file_dir";

    #if JSL_IS_WINDOWS
        _mkdir(path);
    #else
        mkdir(path, S_IRWXU);
    #endif

    int32_t os_err = 0;
    JSLDeleteFileResultEnum res = jsl_delete_file(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_FILE_IS_DIRECTORY);

    #if JSL_IS_WINDOWS
        _rmdir(path);
    #else
        rmdir(path);
    #endif
}

void test_jsl_delete_file_symlink(void)
{
#if JSL_IS_POSIX
    const char* target    = "./tests/example.txt";
    const char* link_path = "./tests/tmp_delete_file_symlink";

    // Best-effort cleanup from any prior failed run
    unlink(link_path);

    int sym_res = symlink(target, link_path);
    TEST_INT32_EQUAL(sym_res, 0);
    if (sym_res != 0)
        return;

    int32_t os_err = 0;
    JSLDeleteFileResultEnum res = jsl_delete_file(
        jsl_cstr_to_memory(link_path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_FILE_SUCCESS);

    // The symlink itself must be gone
    JSLFileTypeEnum type = jsl_get_file_type(jsl_cstr_to_memory(link_path));
    TEST_INT32_EQUAL(type, JSL_FILE_TYPE_NOT_FOUND);

    // The target must still exist
    JSLFileTypeEnum target_type = jsl_get_file_type(jsl_cstr_to_memory(target));
    TEST_INT32_EQUAL(target_type, JSL_FILE_TYPE_REG);
#endif
}

void test_jsl_delete_file_path_too_long(void)
{
    char buffer[FILENAME_MAX + 16];
    for (int64_t i = 0; i < (int64_t) sizeof(buffer); i++)
        buffer[i] = 'a';

    JSLImmutableMemory long_path = {
        .data   = (const uint8_t*) buffer,
        .length = (int64_t) FILENAME_MAX
    };

    JSLDeleteFileResultEnum res = jsl_delete_file(long_path, NULL);
    TEST_INT32_EQUAL(res, JSL_DELETE_FILE_BAD_PARAMETERS);
}

void test_jsl_get_file_type_symlink(void)
{
#if JSL_IS_POSIX
    const char* target = "./tests/example.txt";
    const char* link_path = "./tests/tmp_symlink_get_file_type";

    // Best-effort cleanup from any prior failed run
    unlink(link_path);

    int sym_res = symlink(target, link_path);
    TEST_INT32_EQUAL(sym_res, 0);
    if (sym_res != 0)
        return;

    JSLFileTypeEnum t = jsl_get_file_type(jsl_cstr_to_memory(link_path));
    TEST_INT32_EQUAL(t, JSL_FILE_TYPE_SYMLINK);

    int rm_res = unlink(link_path);
    TEST_INT32_EQUAL(rm_res, 0);
#endif
}

void test_jsl_delete_directory_bad_parameters(void)
{
    JSLImmutableMemory null_path = { .data = NULL, .length = 5 };
    JSLDeleteDirectoryResultEnum res = jsl_delete_directory(null_path, NULL);
    TEST_INT32_EQUAL(res, JSL_DELETE_DIRECTORY_BAD_PARAMETERS);

    JSLImmutableMemory zero_path = { .data = (const uint8_t*) "x", .length = 0 };
    res = jsl_delete_directory(zero_path, NULL);
    TEST_INT32_EQUAL(res, JSL_DELETE_DIRECTORY_BAD_PARAMETERS);
}

void test_jsl_delete_directory_not_found(void)
{
    const char* path = "./tests/tmp_delete_dir_does_not_exist_xyz_12345";
    int32_t os_err = 0;
    JSLDeleteDirectoryResultEnum res = jsl_delete_directory(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_DIRECTORY_NOT_FOUND);
    TEST_BOOL(os_err != 0);
}

void test_jsl_delete_directory_not_a_directory(void)
{
    const char* path = "./tests/tmp_delete_dir_is_file.txt";

    FILE* f = fopen(path, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
        return;
    fclose(f);

    int32_t os_err = 0;
    JSLDeleteDirectoryResultEnum res = jsl_delete_directory(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_DIRECTORY_NOT_A_DIRECTORY);

    remove(path);
}

void test_jsl_delete_directory_empty(void)
{
    const char* path = "./tests/tmp_delete_dir_empty";

    JSL_TEST_RMDIR(path);

    #if JSL_IS_WINDOWS
        _mkdir(path);
    #else
        mkdir(path, S_IRWXU);
    #endif

    int32_t os_err = 0;
    JSLDeleteDirectoryResultEnum res = jsl_delete_directory(
        jsl_cstr_to_memory(path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_DIRECTORY_SUCCESS);

    JSLFileTypeEnum type = jsl_get_file_type(jsl_cstr_to_memory(path));
    TEST_INT32_EQUAL(type, JSL_FILE_TYPE_NOT_FOUND);
}

void test_jsl_delete_directory_with_files(void)
{
    const char* dir_path   = "./tests/tmp_delete_dir_with_files";
    const char* file1_path = "./tests/tmp_delete_dir_with_files/a.txt";
    const char* file2_path = "./tests/tmp_delete_dir_with_files/b.txt";

    JSL_TEST_RMDIR(dir_path);

    #if JSL_IS_WINDOWS
        _mkdir(dir_path);
    #else
        mkdir(dir_path, S_IRWXU);
    #endif

    FILE* f = fopen(file1_path, "wb");
    TEST_BOOL(f != NULL);
    if (f != NULL)
        fclose(f);

    f = fopen(file2_path, "wb");
    TEST_BOOL(f != NULL);
    if (f != NULL)
        fclose(f);

    int32_t os_err = 0;
    JSLDeleteDirectoryResultEnum res = jsl_delete_directory(
        jsl_cstr_to_memory(dir_path),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_DIRECTORY_SUCCESS);

    JSLFileTypeEnum type = jsl_get_file_type(jsl_cstr_to_memory(dir_path));
    TEST_INT32_EQUAL(type, JSL_FILE_TYPE_NOT_FOUND);
}

void test_jsl_delete_directory_nested(void)
{
    const char* top     = "./tests/tmp_delete_dir_nested";
    const char* sub     = "./tests/tmp_delete_dir_nested/sub";
    const char* top_f   = "./tests/tmp_delete_dir_nested/top.txt";
    const char* sub_f   = "./tests/tmp_delete_dir_nested/sub/deep.txt";

    // Best-effort cleanup from any prior failed run
    remove(sub_f);
    remove(top_f);
    JSL_TEST_RMDIR(sub);
    JSL_TEST_RMDIR(top);

    #if JSL_IS_WINDOWS
        _mkdir(top);
        _mkdir(sub);
    #else
        mkdir(top, S_IRWXU);
        mkdir(sub, S_IRWXU);
    #endif

    FILE* f = fopen(top_f, "wb");
    TEST_BOOL(f != NULL);
    if (f != NULL)
        fclose(f);

    f = fopen(sub_f, "wb");
    TEST_BOOL(f != NULL);
    if (f != NULL)
        fclose(f);

    int32_t os_err = 0;
    JSLDeleteDirectoryResultEnum res = jsl_delete_directory(
        jsl_cstr_to_memory(top),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_DELETE_DIRECTORY_SUCCESS);

    JSLFileTypeEnum type = jsl_get_file_type(jsl_cstr_to_memory(top));
    TEST_INT32_EQUAL(type, JSL_FILE_TYPE_NOT_FOUND);
}

void test_jsl_delete_directory_path_too_long(void)
{
    char buffer[FILENAME_MAX + 16];
    for (int64_t i = 0; i < (int64_t) sizeof(buffer); i++)
        buffer[i] = 'a';

    JSLImmutableMemory long_path = {
        .data   = (const uint8_t*) buffer,
        .length = (int64_t) FILENAME_MAX
    };

    JSLDeleteDirectoryResultEnum res = jsl_delete_directory(long_path, NULL);
    TEST_INT32_EQUAL(res, JSL_DELETE_DIRECTORY_BAD_PARAMETERS);
}

void test_jsl_format_file_write_failure(void)
{
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    int pipe_fds[2];
    if (pipe(pipe_fds) == 0)
    {
        close(pipe_fds[0]);

        FILE* writer = fdopen(pipe_fds[1], "w");
        TEST_BOOL(writer != NULL);
        if (writer != NULL)
        {
            TEST_BOOL(setvbuf(writer, NULL, _IONBF, 0) == 0);
            void (*previous_handler)(int) = signal(SIGPIPE, SIG_IGN);

            JSLOutputSink sink = jsl_c_file_output_sink(writer);

            jsl_format_sink(sink, JSL_CSTR_EXPRESSION("Hello"));

            fclose(writer);
            if (previous_handler == SIG_ERR)
            {
                signal(SIGPIPE, SIG_DFL);
            }
            else
            {
                signal(SIGPIPE, previous_handler);
            }
            return;
        }

        close(pipe_fds[1]);
    }
#endif

    FILE* file = jsl__open_failing_stream();

    if (file != NULL)
    {
        JSLOutputSink sink = jsl_c_file_output_sink(file);
        jsl_format_sink(sink, JSL_CSTR_EXPRESSION("Hello"));

        fclose(file);
        return;
    }

    char* path = "./tests/tmp_format_file_failure.txt";
    FILE* setup = fopen(path, "wb");
    if (setup != NULL)
        fclose(setup);

    FILE* read_only = fopen(path, "rb");
    TEST_BOOL(read_only != NULL);
    if (read_only == NULL)
    {
        remove(path);
        return;
    }

    JSLOutputSink ro_sink = jsl_c_file_output_sink(read_only);
    jsl_format_sink(ro_sink, JSL_CSTR_EXPRESSION("Hello"));

    fclose(read_only);
    remove(path);
}

static void jsl__test_iter_create_file(const char* path)
{
    FILE* f = fopen(path, "wb");
    TEST_BOOL(f != NULL);
    if (f != NULL)
        fclose(f);
}

static void jsl__test_iter_make_dir(const char* path)
{
    #if JSL_IS_WINDOWS
        _mkdir(path);
    #else
        mkdir(path, S_IRWXU);
    #endif
}

// Search a recorded list of relative paths for a given expected name.
static bool jsl__test_iter_contains(
    const char (*paths)[FILENAME_MAX + 1],
    int32_t      count,
    const char*  expected
)
{
    bool found = false;
    for (int32_t i = 0; i < count && !found; i++)
    {
        if (strcmp(paths[i], expected) == 0)
            found = true;
    }
    return found;
}

void test_jsl_directory_iterator_bad_parameters(void)
{
    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res;

    // NULL path data
    JSLImmutableMemory null_path = { .data = NULL, .length = 5 };
    init_res = jsl_directory_iterator_init(null_path, &iterator, false);
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_BAD_PARAMETERS);

    // Zero-length path
    JSLImmutableMemory zero_path = { .data = (const uint8_t*) "x", .length = 0 };
    init_res = jsl_directory_iterator_init(zero_path, &iterator, false);
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_BAD_PARAMETERS);

    // NULL iterator pointer
    init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory("./tests"),
        NULL,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_BAD_PARAMETERS);

    // next() on null args
    JSLDirectoryIteratorResult result;
    bool ok = jsl_directory_iterator_next(NULL, &result, NULL);
    TEST_BOOL(!ok);
    ok = jsl_directory_iterator_next(&iterator, NULL, NULL);
    TEST_BOOL(!ok);

    // close() on null is a no-op
    jsl_directory_iterator_end(NULL);
}

void test_jsl_directory_iterator_not_found(void)
{
    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory("./tests/tmp_iter_does_not_exist_xyz_12345"),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_NOT_FOUND);

    // Iterating an init-failed iterator must report no entries.
    JSLDirectoryIteratorResult result;
    bool ok = jsl_directory_iterator_next(&iterator, &result, NULL);
    TEST_BOOL(!ok);

    jsl_directory_iterator_end(&iterator);
}

void test_jsl_directory_iterator_not_a_directory(void)
{
    const char* path = "./tests/example.txt";

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(path),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_NOT_A_DIRECTORY);

    jsl_directory_iterator_end(&iterator);
}

void test_jsl_directory_iterator_empty(void)
{
    const char* dir_path = "./tests/tmp_iter_empty";

    JSL_TEST_RMDIR(dir_path);
    jsl__test_iter_make_dir(dir_path);

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(dir_path),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);

    JSLDirectoryIteratorResult result;
    bool got = jsl_directory_iterator_next(&iterator, &result, NULL);
    TEST_BOOL(!got);

    jsl_directory_iterator_end(&iterator);
    JSL_TEST_RMDIR(dir_path);
}

void test_jsl_directory_iterator_flat(void)
{
    const char* dir_path = "./tests/tmp_iter_flat";
    const char* file_a   = "./tests/tmp_iter_flat/a.txt";
    const char* file_b   = "./tests/tmp_iter_flat/b.txt";
    const char* file_c   = "./tests/tmp_iter_flat/c.txt";

    // Best-effort cleanup from any prior failed run
    remove(file_a);
    remove(file_b);
    remove(file_c);
    JSL_TEST_RMDIR(dir_path);

    jsl__test_iter_make_dir(dir_path);
    jsl__test_iter_create_file(file_a);
    jsl__test_iter_create_file(file_b);
    jsl__test_iter_create_file(file_c);

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(dir_path),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);

    char    relative_paths[8][FILENAME_MAX + 1] = {{0}};
    int32_t count = 0;
    int32_t types[8] = {0};
    int32_t depths[8] = {0};

    JSLDirectoryIteratorResult result;
    while (jsl_directory_iterator_next(&iterator, &result, NULL))
    {
        TEST_INT32_EQUAL(result.status, JSL_DIRECTORY_ITERATOR_OK);
        TEST_BOOL(result.relative_path.length > 0);
        TEST_BOOL(result.relative_path.length < FILENAME_MAX);
        memcpy(
            relative_paths[count],
            result.relative_path.data,
            (size_t) result.relative_path.length
        );
        relative_paths[count][result.relative_path.length] = '\0';
        types[count]  = (int32_t) result.type;
        depths[count] = result.depth;
        count++;
        if (count >= 8)
            break;
    }

    TEST_INT32_EQUAL(count, 3);

    bool found_a = jsl__test_iter_contains(relative_paths, count, "a.txt");
    bool found_b = jsl__test_iter_contains(relative_paths, count, "b.txt");
    bool found_c = jsl__test_iter_contains(relative_paths, count, "c.txt");
    TEST_BOOL(found_a);
    TEST_BOOL(found_b);
    TEST_BOOL(found_c);

    for (int32_t i = 0; i < count; i++)
    {
        TEST_INT32_EQUAL(types[i], JSL_FILE_TYPE_REG);
        TEST_INT32_EQUAL(depths[i], 0);
    }

    jsl_directory_iterator_end(&iterator);

    remove(file_a);
    remove(file_b);
    remove(file_c);
    JSL_TEST_RMDIR(dir_path);
}

void test_jsl_directory_iterator_nested(void)
{
    const char* root      = "./tests/tmp_iter_nested";
    const char* sub_a     = "./tests/tmp_iter_nested/a";
    const char* sub_b     = "./tests/tmp_iter_nested/b";
    const char* sub_a_sub = "./tests/tmp_iter_nested/a/inner";
    const char* file_root = "./tests/tmp_iter_nested/root.txt";
    const char* file_a    = "./tests/tmp_iter_nested/a/a.txt";
    const char* file_b    = "./tests/tmp_iter_nested/b/b.txt";
    const char* file_deep = "./tests/tmp_iter_nested/a/inner/deep.txt";

    // Best-effort cleanup
    remove(file_deep);
    remove(file_a);
    remove(file_b);
    remove(file_root);
    JSL_TEST_RMDIR(sub_a_sub);
    JSL_TEST_RMDIR(sub_a);
    JSL_TEST_RMDIR(sub_b);
    JSL_TEST_RMDIR(root);

    jsl__test_iter_make_dir(root);
    jsl__test_iter_make_dir(sub_a);
    jsl__test_iter_make_dir(sub_b);
    jsl__test_iter_make_dir(sub_a_sub);
    jsl__test_iter_create_file(file_root);
    jsl__test_iter_create_file(file_a);
    jsl__test_iter_create_file(file_b);
    jsl__test_iter_create_file(file_deep);

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(root),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);

    char    relative_paths[16][FILENAME_MAX + 1] = {{0}};
    int32_t types[16] = {0};
    int32_t depths[16] = {0};
    int32_t count = 0;

    JSLDirectoryIteratorResult result;
    while (jsl_directory_iterator_next(&iterator, &result, NULL) && count < 16)
    {
        TEST_INT32_EQUAL(result.status, JSL_DIRECTORY_ITERATOR_OK);
        memcpy(
            relative_paths[count],
            result.relative_path.data,
            (size_t) result.relative_path.length
        );
        relative_paths[count][result.relative_path.length] = '\0';
        types[count]  = (int32_t) result.type;
        depths[count] = result.depth;
        count++;
    }

    // Expect: 4 files + 3 subdirs = 7 entries.
    TEST_INT32_EQUAL(count, 7);

    bool found_root_file = jsl__test_iter_contains(relative_paths, count, "root.txt");
    bool found_sub_a     = jsl__test_iter_contains(relative_paths, count, "a");
    bool found_sub_b     = jsl__test_iter_contains(relative_paths, count, "b");
    TEST_BOOL(found_root_file);
    TEST_BOOL(found_sub_a);
    TEST_BOOL(found_sub_b);

    // Pick a separator that matches the platform's iterator output.
    #if JSL_IS_WINDOWS
        const char* a_a    = "a\\a.txt";
        const char* b_b    = "b\\b.txt";
        const char* inner  = "a\\inner";
        const char* a_deep = "a\\inner\\deep.txt";
    #else
        const char* a_a    = "a/a.txt";
        const char* b_b    = "b/b.txt";
        const char* inner  = "a/inner";
        const char* a_deep = "a/inner/deep.txt";
    #endif

    bool found_a_a   = jsl__test_iter_contains(relative_paths, count, a_a);
    bool found_b_b   = jsl__test_iter_contains(relative_paths, count, b_b);
    bool found_inner = jsl__test_iter_contains(relative_paths, count, inner);
    bool found_deep  = jsl__test_iter_contains(relative_paths, count, a_deep);
    TEST_BOOL(found_a_a);
    TEST_BOOL(found_b_b);
    TEST_BOOL(found_inner);
    TEST_BOOL(found_deep);

    // Verify depth values per entry.
    for (int32_t i = 0; i < count; i++)
    {
        bool is_root_level = (strcmp(relative_paths[i], "root.txt") == 0
            || strcmp(relative_paths[i], "a") == 0
            || strcmp(relative_paths[i], "b") == 0);
        bool is_one_deep = (strcmp(relative_paths[i], a_a) == 0
            || strcmp(relative_paths[i], b_b) == 0
            || strcmp(relative_paths[i], inner) == 0);
        bool is_two_deep = (strcmp(relative_paths[i], a_deep) == 0);

        if (is_root_level)
            TEST_INT32_EQUAL(depths[i], 0);
        if (is_one_deep)
            TEST_INT32_EQUAL(depths[i], 1);
        if (is_two_deep)
            TEST_INT32_EQUAL(depths[i], 2);
    }

    // Subdirectories should be reported as DIR.
    for (int32_t i = 0; i < count; i++)
    {
        bool is_dir = (strcmp(relative_paths[i], "a") == 0
            || strcmp(relative_paths[i], "b") == 0
            || strcmp(relative_paths[i], inner) == 0);
        if (is_dir)
            TEST_INT32_EQUAL(types[i], JSL_FILE_TYPE_DIR);
    }

    jsl_directory_iterator_end(&iterator);

    // Cleanup
    remove(file_deep);
    remove(file_a);
    remove(file_b);
    remove(file_root);
    JSL_TEST_RMDIR(sub_a_sub);
    JSL_TEST_RMDIR(sub_a);
    JSL_TEST_RMDIR(sub_b);
    JSL_TEST_RMDIR(root);
}

void test_jsl_directory_iterator_relative_paths(void)
{
    const char* root  = "./tests/tmp_iter_rel";
    const char* sub   = "./tests/tmp_iter_rel/sub";
    const char* file1 = "./tests/tmp_iter_rel/file.txt";
    const char* file2 = "./tests/tmp_iter_rel/sub/inside.txt";

    remove(file2);
    remove(file1);
    JSL_TEST_RMDIR(sub);
    JSL_TEST_RMDIR(root);

    jsl__test_iter_make_dir(root);
    jsl__test_iter_make_dir(sub);
    jsl__test_iter_create_file(file1);
    jsl__test_iter_create_file(file2);

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(root),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);

    JSLDirectoryIteratorResult result;
    while (jsl_directory_iterator_next(&iterator, &result, NULL))
    {
        // The absolute_path must always be an absolute path.
        TEST_BOOL(result.absolute_path.length >= result.relative_path.length);

        // The relative_path must be a suffix of absolute_path.
        const uint8_t* tail = result.absolute_path.data
            + (result.absolute_path.length - result.relative_path.length);
        TEST_BUFFERS_EQUAL(tail, result.relative_path.data, result.relative_path.length);

        // On POSIX absolute paths begin with '/'.
        #if JSL_IS_POSIX
            TEST_BOOL(result.absolute_path.length > 0
                && result.absolute_path.data[0] == '/');
        #endif
    }

    jsl_directory_iterator_end(&iterator);

    remove(file2);
    remove(file1);
    JSL_TEST_RMDIR(sub);
    JSL_TEST_RMDIR(root);
}

void test_jsl_directory_iterator_close_early(void)
{
    const char* root  = "./tests/tmp_iter_close";
    const char* sub_a = "./tests/tmp_iter_close/a";
    const char* sub_b = "./tests/tmp_iter_close/a/b";
    const char* file  = "./tests/tmp_iter_close/a/b/c.txt";

    remove(file);
    JSL_TEST_RMDIR(sub_b);
    JSL_TEST_RMDIR(sub_a);
    JSL_TEST_RMDIR(root);

    jsl__test_iter_make_dir(root);
    jsl__test_iter_make_dir(sub_a);
    jsl__test_iter_make_dir(sub_b);
    jsl__test_iter_create_file(file);

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(root),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);

    // Read just one entry then close. The close call must release any open
    // handles even though we have not drained the iterator.
    JSLDirectoryIteratorResult result;
    bool got = jsl_directory_iterator_next(&iterator, &result, NULL);
    TEST_BOOL(got);

    jsl_directory_iterator_end(&iterator);

    // Calling close again must be safe.
    jsl_directory_iterator_end(&iterator);

    // After close, next() must return false without crashing.
    got = jsl_directory_iterator_next(&iterator, &result, NULL);
    TEST_BOOL(!got);

    remove(file);
    JSL_TEST_RMDIR(sub_b);
    JSL_TEST_RMDIR(sub_a);
    JSL_TEST_RMDIR(root);
}

void test_jsl_directory_iterator_symlink_default(void)
{
#if JSL_IS_POSIX
    const char* root        = "./tests/tmp_iter_sym_def";
    const char* sub         = "./tests/tmp_iter_sym_def/real";
    const char* file_inside = "./tests/tmp_iter_sym_def/real/file.txt";
    const char* link_path   = "./tests/tmp_iter_sym_def/link";

    unlink(link_path);
    remove(file_inside);
    JSL_TEST_RMDIR(sub);
    JSL_TEST_RMDIR(root);

    jsl__test_iter_make_dir(root);
    jsl__test_iter_make_dir(sub);
    jsl__test_iter_create_file(file_inside);

    int sym_res = symlink("real", link_path);
    TEST_INT32_EQUAL(sym_res, 0);
    if (sym_res != 0)
        return;

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(root),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);

    int32_t link_count = 0;
    int32_t link_type  = JSL_FILE_TYPE_UNKNOWN;
    int32_t real_visits = 0;

    JSLDirectoryIteratorResult result;
    while (jsl_directory_iterator_next(&iterator, &result, NULL))
    {
        char rel_buf[FILENAME_MAX + 1];
        memcpy(rel_buf, result.relative_path.data, (size_t) result.relative_path.length);
        rel_buf[result.relative_path.length] = '\0';

        if (strcmp(rel_buf, "link") == 0)
        {
            link_count++;
            link_type = (int32_t) result.type;
        }
        if (strcmp(rel_buf, "real/file.txt") == 0)
        {
            real_visits++;
        }
    }

    // The symlink should be reported once as a SYMLINK and not descended.
    TEST_INT32_EQUAL(link_count, 1);
    TEST_INT32_EQUAL(link_type, JSL_FILE_TYPE_SYMLINK);
    // The real directory contents are still walked exactly once.
    TEST_INT32_EQUAL(real_visits, 1);

    jsl_directory_iterator_end(&iterator);

    unlink(link_path);
    remove(file_inside);
    JSL_TEST_RMDIR(sub);
    JSL_TEST_RMDIR(root);
#endif
}

void test_jsl_directory_iterator_symlink_follow(void)
{
#if JSL_IS_POSIX
    const char* root        = "./tests/tmp_iter_sym_follow";
    const char* sub         = "./tests/tmp_iter_sym_follow/real";
    const char* file_inside = "./tests/tmp_iter_sym_follow/real/file.txt";
    const char* link_path   = "./tests/tmp_iter_sym_follow/link";

    unlink(link_path);
    remove(file_inside);
    JSL_TEST_RMDIR(sub);
    JSL_TEST_RMDIR(root);

    jsl__test_iter_make_dir(root);
    jsl__test_iter_make_dir(sub);
    jsl__test_iter_create_file(file_inside);

    int sym_res = symlink("real", link_path);
    TEST_INT32_EQUAL(sym_res, 0);
    if (sym_res != 0)
        return;

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(root),
        &iterator,
        true
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);

    int32_t real_file_visits = 0;
    int32_t link_file_visits = 0;
    int32_t link_dir_count   = 0;

    JSLDirectoryIteratorResult result;
    while (jsl_directory_iterator_next(&iterator, &result, NULL))
    {
        char rel_buf[FILENAME_MAX + 1];
        memcpy(rel_buf, result.relative_path.data, (size_t) result.relative_path.length);
        rel_buf[result.relative_path.length] = '\0';

        if (strcmp(rel_buf, "real/file.txt") == 0)
            real_file_visits++;
        if (strcmp(rel_buf, "link/file.txt") == 0)
            link_file_visits++;
        if (strcmp(rel_buf, "link") == 0 && result.type == JSL_FILE_TYPE_DIR)
            link_dir_count++;
    }

    // When following, both the real directory and the symlinked
    // directory's contents should be visited.
    TEST_INT32_EQUAL(real_file_visits, 1);
    TEST_INT32_EQUAL(link_file_visits, 1);
    TEST_INT32_EQUAL(link_dir_count, 1);

    jsl_directory_iterator_end(&iterator);

    unlink(link_path);
    remove(file_inside);
    JSL_TEST_RMDIR(sub);
    JSL_TEST_RMDIR(root);
#endif
}

void test_jsl_directory_iterator_path_too_long(void)
{
    char buffer[FILENAME_MAX + 16];
    for (int64_t i = 0; i < (int64_t) sizeof(buffer); i++)
        buffer[i] = 'a';

    JSLImmutableMemory long_path = {
        .data   = (const uint8_t*) buffer,
        .length = (int64_t)(FILENAME_MAX + 1)
    };

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        long_path,
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_PATH_TOO_LONG);

    jsl_directory_iterator_end(&iterator);
}

void test_jsl_directory_iterator_skips_dot_and_dotdot(void)
{
    const char* root  = "./tests/tmp_iter_dots";
    const char* file  = "./tests/tmp_iter_dots/only.txt";

    remove(file);
    JSL_TEST_RMDIR(root);

    jsl__test_iter_make_dir(root);
    jsl__test_iter_create_file(file);

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = jsl_directory_iterator_init(
        jsl_cstr_to_memory(root),
        &iterator,
        false
    );
    TEST_INT32_EQUAL(init_res, JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);

    int32_t saw_dot    = 0;
    int32_t saw_dotdot = 0;
    int32_t total      = 0;

    JSLDirectoryIteratorResult result;
    while (jsl_directory_iterator_next(&iterator, &result, NULL))
    {
        total++;
        char rel_buf[FILENAME_MAX + 1];
        memcpy(rel_buf, result.relative_path.data, (size_t) result.relative_path.length);
        rel_buf[result.relative_path.length] = '\0';

        if (strcmp(rel_buf, ".") == 0)
            saw_dot++;
        if (strcmp(rel_buf, "..") == 0)
            saw_dotdot++;
    }

    TEST_INT32_EQUAL(total, 1);
    TEST_INT32_EQUAL(saw_dot, 0);
    TEST_INT32_EQUAL(saw_dotdot, 0);

    jsl_directory_iterator_end(&iterator);

    remove(file);
    JSL_TEST_RMDIR(root);
}

void test_jsl_copy_file_bad_parameters(void)
{
    JSLImmutableMemory null_path = { .data = NULL, .length = 5 };
    JSLImmutableMemory valid_path = jsl_cstr_to_memory("./tests/example.txt");

    JSLCopyFileResultEnum res = jsl_copy_file(null_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_BAD_PARAMETERS);

    res = jsl_copy_file(valid_path, null_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_BAD_PARAMETERS);

    JSLImmutableMemory zero_path = { .data = (const uint8_t*) "x", .length = 0 };
    res = jsl_copy_file(zero_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_BAD_PARAMETERS);

    res = jsl_copy_file(valid_path, zero_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_BAD_PARAMETERS);
}

void test_jsl_copy_file_path_too_long(void)
{
    char buffer[FILENAME_MAX + 16];
    for (int64_t i = 0; i < (int64_t) sizeof(buffer); i++)
        buffer[i] = 'a';

    JSLImmutableMemory long_path = {
        .data   = (const uint8_t*) buffer,
        .length = (int64_t) FILENAME_MAX
    };
    JSLImmutableMemory valid_path = jsl_cstr_to_memory("./tests/example.txt");

    JSLCopyFileResultEnum res = jsl_copy_file(long_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_PATH_TOO_LONG);

    res = jsl_copy_file(valid_path, long_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_PATH_TOO_LONG);
}

void test_jsl_copy_file_source_not_found(void)
{
    const char* src  = "./tests/tmp_copy_file_no_such_source_xyz_12345.txt";
    const char* dst  = "./tests/tmp_copy_file_nf_dst.txt";

    int32_t os_err = 0;
    JSLCopyFileResultEnum res = jsl_copy_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_SOURCE_NOT_FOUND);
    TEST_BOOL(os_err != 0);
}

void test_jsl_copy_file_source_is_directory(void)
{
    const char* src = "./tests";
    const char* dst = "./tests/tmp_copy_file_dir_dst.txt";

    int32_t os_err = 0;
    JSLCopyFileResultEnum res = jsl_copy_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_SOURCE_IS_DIRECTORY);
}

void test_jsl_copy_file_success(void)
{
    #if JSL_IS_WINDOWS
        const char* src = "tests\\example.txt";
    #else
        const char* src = "./tests/example.txt";
    #endif
    const char* dst = "./tests/tmp_copy_file_success.txt";

    // Best-effort cleanup from any prior failed run
    remove(dst);

    int32_t os_err = 0;
    JSLCopyFileResultEnum res = jsl_copy_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    // Verify the destination exists and has the same contents
    FILE* src_f = fopen(src, "rb");
    TEST_BOOL(src_f != NULL);
    if (src_f == NULL)
    {
        remove(dst);
        return;
    }
    fseek(src_f, 0, SEEK_END);
    int64_t src_size = (int64_t) ftell(src_f);
    rewind(src_f);

    FILE* dst_f = fopen(dst, "rb");
    TEST_BOOL(dst_f != NULL);
    if (dst_f == NULL)
    {
        fclose(src_f);
        remove(dst);
        return;
    }
    fseek(dst_f, 0, SEEK_END);
    int64_t dst_size = (int64_t) ftell(dst_f);
    rewind(dst_f);

    TEST_INT64_EQUAL(src_size, dst_size);

    char src_buf[4*1024];
    char dst_buf[4*1024];
    size_t src_read = fread(src_buf, 1, (size_t) src_size, src_f);
    size_t dst_read = fread(dst_buf, 1, (size_t) dst_size, dst_f);
    TEST_INT64_EQUAL((int64_t) src_read, src_size);
    TEST_INT64_EQUAL((int64_t) dst_read, dst_size);
    TEST_BOOL(memcmp(src_buf, dst_buf, (size_t) src_size) == 0);

    fclose(src_f);
    fclose(dst_f);
    remove(dst);
}

void test_jsl_copy_file_overwrites_existing(void)
{
    #if JSL_IS_WINDOWS
        const char* src = "tests\\example.txt";
    #else
        const char* src = "./tests/example.txt";
    #endif
    const char* dst = "./tests/tmp_copy_file_overwrite.txt";

    // Write different content to destination first
    FILE* f = fopen(dst, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
        return;
    fprintf(f, "old content that should be overwritten");
    fclose(f);

    int32_t os_err = 0;
    JSLCopyFileResultEnum res = jsl_copy_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_FILE_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    // Verify contents match source, not the old destination content
    FILE* src_f = fopen(src, "rb");
    TEST_BOOL(src_f != NULL);
    if (src_f == NULL)
    {
        remove(dst);
        return;
    }
    fseek(src_f, 0, SEEK_END);
    int64_t src_size = (int64_t) ftell(src_f);
    rewind(src_f);

    FILE* dst_f = fopen(dst, "rb");
    TEST_BOOL(dst_f != NULL);
    if (dst_f == NULL)
    {
        fclose(src_f);
        remove(dst);
        return;
    }
    fseek(dst_f, 0, SEEK_END);
    int64_t dst_size = (int64_t) ftell(dst_f);
    rewind(dst_f);

    TEST_INT64_EQUAL(src_size, dst_size);

    fclose(src_f);
    fclose(dst_f);
    remove(dst);
}

void test_jsl_copy_file_dest_parent_not_found(void)
{
    #if JSL_IS_WINDOWS
        const char* src = "tests\\example.txt";
    #else
        const char* src = "./tests/example.txt";
    #endif
    const char* dst = "./tests/tmp_nonexistent_dir_xyz/copy.txt";

    int32_t os_err = 0;
    JSLCopyFileResultEnum res = jsl_copy_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_BOOL(res != JSL_COPY_FILE_SUCCESS);
    TEST_BOOL(os_err != 0);
}

//
// jsl_copy_directory tests
//

void test_jsl_copy_directory_bad_parameters(void)
{
    JSLImmutableMemory null_path  = { .data = NULL, .length = 5 };
    JSLImmutableMemory valid_path = jsl_cstr_to_memory("./tests");

    JSLCopyDirectoryResultEnum res = jsl_copy_directory(null_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_BAD_PARAMETERS);

    res = jsl_copy_directory(valid_path, null_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_BAD_PARAMETERS);

    JSLImmutableMemory zero_path = { .data = (const uint8_t*) "x", .length = 0 };
    res = jsl_copy_directory(zero_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_BAD_PARAMETERS);

    res = jsl_copy_directory(valid_path, zero_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_BAD_PARAMETERS);
}

void test_jsl_copy_directory_path_too_long(void)
{
    char buffer[FILENAME_MAX + 16];
    for (int64_t i = 0; i < (int64_t) sizeof(buffer); i++)
        buffer[i] = 'a';

    JSLImmutableMemory long_path = {
        .data   = (const uint8_t*) buffer,
        .length = (int64_t) FILENAME_MAX
    };
    JSLImmutableMemory valid_path = jsl_cstr_to_memory("./tests");

    JSLCopyDirectoryResultEnum res = jsl_copy_directory(long_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_PATH_TOO_LONG);

    res = jsl_copy_directory(valid_path, long_path, NULL);
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_PATH_TOO_LONG);
}

void test_jsl_copy_directory_source_not_found(void)
{
    const char* src = "./tests/tmp_copy_dir_no_such_source_xyz_12345";
    const char* dst = "./tests/tmp_copy_dir_nf_dst";

    int32_t os_err = 0;
    JSLCopyDirectoryResultEnum res = jsl_copy_directory(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_SOURCE_NOT_FOUND);
}

void test_jsl_copy_directory_source_not_a_directory(void)
{
    #if JSL_IS_WINDOWS
        const char* src = "tests\\example.txt";
    #else
        const char* src = "./tests/example.txt";
    #endif
    const char* dst = "./tests/tmp_copy_dir_src_is_file";

    int32_t os_err = 0;
    JSLCopyDirectoryResultEnum res = jsl_copy_directory(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_SOURCE_NOT_A_DIRECTORY);
}

void test_jsl_copy_directory_dest_already_exists(void)
{
    const char* src = "./tests";
    const char* dst = "./tests/tmp_copy_dir_dst_exists";

    #if JSL_IS_WINDOWS
        _mkdir(dst);
    #else
        mkdir(dst, S_IRWXU);
    #endif

    int32_t os_err = 0;
    JSLCopyDirectoryResultEnum res = jsl_copy_directory(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_DEST_ALREADY_EXISTS);

    JSL_TEST_RMDIR(dst);
}

void test_jsl_copy_directory_empty(void)
{
    const char* src = "./tests/tmp_copy_dir_empty_src";
    const char* dst = "./tests/tmp_copy_dir_empty_dst";

    // Best-effort cleanup from any prior failed run
    (void) jsl_delete_directory(jsl_cstr_to_memory(dst), NULL);
    (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);

    #if JSL_IS_WINDOWS
        _mkdir(src);
    #else
        mkdir(src, S_IRWXU);
    #endif

    int32_t os_err = 0;
    JSLCopyDirectoryResultEnum res = jsl_copy_directory(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    // Destination directory must now exist and be empty
    JSLFileTypeEnum dst_type = jsl_get_file_type(jsl_cstr_to_memory(dst));
    TEST_INT32_EQUAL(dst_type, JSL_FILE_TYPE_DIR);

    (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);
    (void) jsl_delete_directory(jsl_cstr_to_memory(dst), NULL);
}

void test_jsl_copy_directory_with_files(void)
{
    const char* src       = "./tests/tmp_copy_dir_flat_src";
    const char* src_file1 = "./tests/tmp_copy_dir_flat_src/a.txt";
    const char* src_file2 = "./tests/tmp_copy_dir_flat_src/b.txt";
    const char* dst       = "./tests/tmp_copy_dir_flat_dst";
    const char* dst_file1 = "./tests/tmp_copy_dir_flat_dst/a.txt";
    const char* dst_file2 = "./tests/tmp_copy_dir_flat_dst/b.txt";

    // Best-effort cleanup from any prior failed run
    (void) jsl_delete_directory(jsl_cstr_to_memory(dst), NULL);
    (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);

    #if JSL_IS_WINDOWS
        _mkdir(src);
    #else
        mkdir(src, S_IRWXU);
    #endif

    FILE* f = fopen(src_file1, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
    {
        (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);
        return;
    }
    fprintf(f, "hello from a");
    fclose(f);

    f = fopen(src_file2, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
    {
        (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);
        return;
    }
    fprintf(f, "hello from b");
    fclose(f);

    int32_t os_err = 0;
    JSLCopyDirectoryResultEnum res = jsl_copy_directory(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    // Both files must exist in the destination
    JSLFileTypeEnum t1 = jsl_get_file_type(jsl_cstr_to_memory(dst_file1));
    JSLFileTypeEnum t2 = jsl_get_file_type(jsl_cstr_to_memory(dst_file2));
    TEST_INT32_EQUAL(t1, JSL_FILE_TYPE_REG);
    TEST_INT32_EQUAL(t2, JSL_FILE_TYPE_REG);

    // Verify contents match source
    FILE* sf = fopen(src_file1, "rb");
    FILE* df = fopen(dst_file1, "rb");
    TEST_BOOL(sf != NULL && df != NULL);
    if (sf != NULL && df != NULL)
    {
        char src_buf[64] = {0};
        char dst_buf[64] = {0};
        size_t sr = fread(src_buf, 1, sizeof(src_buf), sf);
        size_t dr = fread(dst_buf, 1, sizeof(dst_buf), df);
        TEST_INT64_EQUAL((int64_t) sr, (int64_t) dr);
        TEST_BOOL(memcmp(src_buf, dst_buf, sr) == 0);
    }
    if (sf != NULL)
        fclose(sf);
    if (df != NULL)
        fclose(df);

    (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);
    (void) jsl_delete_directory(jsl_cstr_to_memory(dst), NULL);
}

void test_jsl_copy_directory_nested(void)
{
    const char* src         = "./tests/tmp_copy_dir_nested_src";
    const char* src_sub     = "./tests/tmp_copy_dir_nested_src/sub";
    const char* src_root_f  = "./tests/tmp_copy_dir_nested_src/root.txt";
    const char* src_sub_f   = "./tests/tmp_copy_dir_nested_src/sub/inner.txt";
    const char* dst         = "./tests/tmp_copy_dir_nested_dst";
    const char* dst_sub     = "./tests/tmp_copy_dir_nested_dst/sub";
    const char* dst_root_f  = "./tests/tmp_copy_dir_nested_dst/root.txt";
    const char* dst_sub_f   = "./tests/tmp_copy_dir_nested_dst/sub/inner.txt";

    // Best-effort cleanup from any prior failed run
    (void) jsl_delete_directory(jsl_cstr_to_memory(dst), NULL);
    (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);

    #if JSL_IS_WINDOWS
        _mkdir(src);
        _mkdir(src_sub);
    #else
        mkdir(src, S_IRWXU);
        mkdir(src_sub, S_IRWXU);
    #endif

    FILE* f = fopen(src_root_f, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
    {
        (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);
        return;
    }
    fprintf(f, "root content");
    fclose(f);

    f = fopen(src_sub_f, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
    {
        (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);
        return;
    }
    fprintf(f, "inner content");
    fclose(f);

    int32_t os_err = 0;
    JSLCopyDirectoryResultEnum res = jsl_copy_directory(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_COPY_DIRECTORY_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    // Destination structure must mirror source
    JSLFileTypeEnum dst_type = jsl_get_file_type(jsl_cstr_to_memory(dst));
    JSLFileTypeEnum sub_type = jsl_get_file_type(jsl_cstr_to_memory(dst_sub));
    JSLFileTypeEnum rf_type  = jsl_get_file_type(jsl_cstr_to_memory(dst_root_f));
    JSLFileTypeEnum sf_type  = jsl_get_file_type(jsl_cstr_to_memory(dst_sub_f));
    TEST_INT32_EQUAL(dst_type, JSL_FILE_TYPE_DIR);
    TEST_INT32_EQUAL(sub_type, JSL_FILE_TYPE_DIR);
    TEST_INT32_EQUAL(rf_type,  JSL_FILE_TYPE_REG);
    TEST_INT32_EQUAL(sf_type,  JSL_FILE_TYPE_REG);

    (void) jsl_delete_directory(jsl_cstr_to_memory(src), NULL);
    (void) jsl_delete_directory(jsl_cstr_to_memory(dst), NULL);
}

void test_jsl_rename_file_bad_parameters(void)
{
    JSLImmutableMemory null_path = { .data = NULL, .length = 5 };
    JSLImmutableMemory valid_path = jsl_cstr_to_memory("./tests/example.txt");

    JSLRenameFileResultEnum res = jsl_rename_file(null_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_BAD_PARAMETERS);

    res = jsl_rename_file(valid_path, null_path, NULL);
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_BAD_PARAMETERS);

    JSLImmutableMemory zero_path = { .data = (const uint8_t*) "x", .length = 0 };
    res = jsl_rename_file(zero_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_BAD_PARAMETERS);

    res = jsl_rename_file(valid_path, zero_path, NULL);
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_BAD_PARAMETERS);
}

void test_jsl_rename_file_path_too_long(void)
{
    char buffer[FILENAME_MAX + 16];
    for (int64_t i = 0; i < (int64_t) sizeof(buffer); i++)
        buffer[i] = 'a';

    JSLImmutableMemory long_path = {
        .data   = (const uint8_t*) buffer,
        .length = (int64_t) FILENAME_MAX
    };
    JSLImmutableMemory valid_path = jsl_cstr_to_memory("./tests/example.txt");

    JSLRenameFileResultEnum res = jsl_rename_file(long_path, valid_path, NULL);
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_PATH_TOO_LONG);

    res = jsl_rename_file(valid_path, long_path, NULL);
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_PATH_TOO_LONG);
}

void test_jsl_rename_file_source_not_found(void)
{
    const char* src = "./tests/tmp_rename_no_such_file_xyz_12345.txt";
    const char* dst = "./tests/tmp_rename_dst_xyz_12345.txt";

    int32_t os_err = 0;
    JSLRenameFileResultEnum res = jsl_rename_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_SOURCE_NOT_FOUND);
    TEST_BOOL(os_err != 0);
}

void test_jsl_rename_file_renames_file(void)
{
    const char* src = "./tests/tmp_rename_src_file.txt";
    const char* dst = "./tests/tmp_rename_dst_file.txt";

    // Best-effort cleanup from any prior failed run
    remove(src);
    remove(dst);

    FILE* f = fopen(src, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
        return;
    fwrite("hello", 1, 5, f);
    fclose(f);

    int32_t os_err = 0;
    JSLRenameFileResultEnum res = jsl_rename_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    JSLFileTypeEnum src_type = jsl_get_file_type(jsl_cstr_to_memory(src));
    TEST_INT32_EQUAL(src_type, JSL_FILE_TYPE_NOT_FOUND);

    JSLFileTypeEnum dst_type = jsl_get_file_type(jsl_cstr_to_memory(dst));
    TEST_INT32_EQUAL(dst_type, JSL_FILE_TYPE_REG);

    remove(dst);
}

void test_jsl_rename_file_renames_directory(void)
{
    const char* src = "./tests/tmp_rename_src_dir";
    const char* dst = "./tests/tmp_rename_dst_dir";

    // Best-effort cleanup from any prior failed run
    #if JSL_IS_WINDOWS
        _rmdir(src);
        _rmdir(dst);
        _mkdir(src);
    #else
        rmdir(src);
        rmdir(dst);
        mkdir(src, S_IRWXU);
    #endif

    int32_t os_err = 0;
    JSLRenameFileResultEnum res = jsl_rename_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    JSLFileTypeEnum src_type = jsl_get_file_type(jsl_cstr_to_memory(src));
    TEST_INT32_EQUAL(src_type, JSL_FILE_TYPE_NOT_FOUND);

    JSLFileTypeEnum dst_type = jsl_get_file_type(jsl_cstr_to_memory(dst));
    TEST_INT32_EQUAL(dst_type, JSL_FILE_TYPE_DIR);

    #if JSL_IS_WINDOWS
        _rmdir(dst);
    #else
        rmdir(dst);
    #endif
}

void test_jsl_rename_file_overwrites_existing_file(void)
{
    const char* src = "./tests/tmp_rename_overwrite_src.txt";
    const char* dst = "./tests/tmp_rename_overwrite_dst.txt";

    remove(src);
    remove(dst);

    FILE* f = fopen(src, "wb");
    TEST_BOOL(f != NULL);
    if (f == NULL)
        return;
    fwrite("new", 1, 3, f);
    fclose(f);

    FILE* g = fopen(dst, "wb");
    TEST_BOOL(g != NULL);
    if (g == NULL)
    {
        remove(src);
        return;
    }
    fwrite("old", 1, 3, g);
    fclose(g);

    int32_t os_err = 0;
    JSLRenameFileResultEnum res = jsl_rename_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    JSLFileTypeEnum src_type = jsl_get_file_type(jsl_cstr_to_memory(src));
    TEST_INT32_EQUAL(src_type, JSL_FILE_TYPE_NOT_FOUND);

    JSLFileTypeEnum dst_type = jsl_get_file_type(jsl_cstr_to_memory(dst));
    TEST_INT32_EQUAL(dst_type, JSL_FILE_TYPE_REG);

    remove(dst);
}

void test_jsl_rename_file_renames_symlink(void)
{
#if JSL_IS_POSIX
    const char* target    = "./tests/example.txt";
    const char* src       = "./tests/tmp_rename_symlink_src";
    const char* dst       = "./tests/tmp_rename_symlink_dst";

    unlink(src);
    unlink(dst);

    int sym_res = symlink(target, src);
    TEST_INT32_EQUAL(sym_res, 0);
    if (sym_res != 0)
        return;

    int32_t os_err = 0;
    JSLRenameFileResultEnum res = jsl_rename_file(
        jsl_cstr_to_memory(src),
        jsl_cstr_to_memory(dst),
        &os_err
    );
    TEST_INT32_EQUAL(res, JSL_RENAME_FILE_SUCCESS);
    TEST_INT32_EQUAL(os_err, 0);

    JSLFileTypeEnum src_type = jsl_get_file_type(jsl_cstr_to_memory(src));
    TEST_INT32_EQUAL(src_type, JSL_FILE_TYPE_NOT_FOUND);

    JSLFileTypeEnum dst_type = jsl_get_file_type(jsl_cstr_to_memory(dst));
    TEST_INT32_EQUAL(dst_type, JSL_FILE_TYPE_SYMLINK);

    // The original target is unaffected
    JSLFileTypeEnum target_type = jsl_get_file_type(jsl_cstr_to_memory(target));
    TEST_INT32_EQUAL(target_type, JSL_FILE_TYPE_REG);

    unlink(dst);
#endif
}

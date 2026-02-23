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

#include "minctest.h"

static void test_jsl_load_file_contents(void)
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
        &allocator,
        jsl_cstr_to_memory(path),
        &contents,
        NULL
    );

    TEST_BOOL(res == JSL_FILE_LOAD_SUCCESS);
    TEST_BUFFERS_EQUAL(stack_buffer, contents.data, (size_t) file_size);
}

static void test_jsl_get_file_size(void)
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

static void test_jsl_load_file_contents_buffer(void)
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


static void test_jsl_format_file_formats_and_writes_output(void)
{
    FILE* file = tmpfile();
    TEST_BOOL(file != NULL);
    if (file == NULL)
        return;

    JSLOutputSink sink = jsl_c_file_output_sink(file);

    bool res = jsl_format_sink(
        sink,
        JSL_CSTR_EXPRESSION("Hello %s %d"),
        "World",
        42
    );
    TEST_BOOL(res == true);

    TEST_BOOL(fflush(file) == 0);
    TEST_BOOL(fseek(file, 0, SEEK_SET) == 0);

    char buffer[64] = {0};
    size_t read = fread(buffer, 1, sizeof(buffer), file);
    const char* expected = "Hello World 42";
    TEST_BOOL(read == strlen(expected));
    TEST_BOOL(memcmp(buffer, expected, read) == 0);

    fclose(file);
}

static void test_jsl_format_file_accepts_empty_format(void)
{
    FILE* file = tmpfile();
    TEST_BOOL(file != NULL);
    if (file == NULL)
        return;

    JSLOutputSink sink = jsl_c_file_output_sink(file);

    int64_t res = jsl_format_sink(sink, JSL_CSTR_EXPRESSION(""));
    TEST_INT64_EQUAL(res, 0);

    TEST_BOOL(fflush(file) == 0);
    TEST_BOOL(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    TEST_BOOL(size == 0);

    fclose(file);
}

static void test_jsl_format_file_null_out_parameter(void)
{
    JSLOutputSink sink = jsl_c_file_output_sink(NULL);

    bool res = jsl_format_sink(sink, JSL_CSTR_EXPRESSION("Hello"));
    TEST_BOOL(!res);
}

static void test_jsl_format_file_null_format_pointer(void)
{
    JSLImmutableMemory fmt = {
        .data = NULL,
        .length = 5
    };

    JSLOutputSink sink = jsl_c_file_output_sink(stdout);

    int64_t res = jsl_format_sink(sink, fmt);
    TEST_INT64_EQUAL(res, -1);
}

static void test_jsl_format_file_negative_length(void)
{
    JSLImmutableMemory fmt = {
        .data = (uint8_t*)"Hello",
        .length = -1
    };
    JSLOutputSink sink = jsl_c_file_output_sink(stdout);

    int64_t res = jsl_format_sink(sink, fmt);
    TEST_INT64_EQUAL(res, -1);
}

static void test_jsl_format_file_write_failure(void)
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

            int64_t res = jsl_format_sink(sink, JSL_CSTR_EXPRESSION("Hello"));
            TEST_BOOL(res < 0);

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
        int64_t res1 = jsl_format_sink(sink, JSL_CSTR_EXPRESSION("Hello"));
        TEST_INT64_EQUAL(res1, -1);

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
    int64_t res2 = jsl_format_sink(ro_sink, JSL_CSTR_EXPRESSION("Hello"));
    TEST_BOOL(res2 < 0);

    fclose(read_only);
    remove(path);
}

int main(void)
{
    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    RUN_TEST_FUNCTION("Test jsl_load_file_contents", test_jsl_load_file_contents);
    RUN_TEST_FUNCTION("Test jsl_get_file_size", test_jsl_get_file_size);
    RUN_TEST_FUNCTION("Test jsl_load_file_contents_buffer", test_jsl_load_file_contents_buffer);

    RUN_TEST_FUNCTION("Test jsl_format_to_c_file formats and writes output", test_jsl_format_file_formats_and_writes_output);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file accepts empty format", test_jsl_format_file_accepts_empty_format);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file null out parameter", test_jsl_format_file_null_out_parameter);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file null format pointer", test_jsl_format_file_null_format_pointer);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file negative length", test_jsl_format_file_negative_length);
    RUN_TEST_FUNCTION("Test jsl_format_to_c_file write failure", test_jsl_format_file_write_failure);

    TEST_RESULTS();
    return lfails != 0;
}

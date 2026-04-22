/**
 * Copyright (c) 2026 Jack Stouffer
 */

#define _CRT_SECURE_NO_WARNINGS

#if !defined(_WIN32) && !defined(__wasm__)
    #define _GNU_SOURCE
    #define _XOPEN_SOURCE 700
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsl/core.h"
#include "jsl/allocator.h"
#include "jsl/allocator_libc.h"
#include "jsl/os.h"
#include "jsl/string_builder.h"

#include "minctest.h"
#include "test_subprocess.h"

#if JSL_IS_WINDOWS
    #include <direct.h>
    static const char* HELPER_PATH = "tests\\bin\\subprocess_helper.exe";
#else
    #include <unistd.h>
    static const char* HELPER_PATH = "tests/bin/subprocess_helper";
#endif

static JSLAllocatorInterface test_libc_allocator_interface(JSLLibcAllocator* backing)
{
    jsl_libc_allocator_init(backing);
    JSLAllocatorInterface iface;
    jsl_libc_allocator_get_allocator_interface(&iface, backing);
    return iface;
}

// Initialize a subprocess using the helper executable. Keeps the per-test
// boilerplate short. Caller owns the backing allocator and must call
// jsl_libc_allocator_free_all on it after cleanup.
static bool make_helper(
    JSLSubprocess* proc,
    JSLLibcAllocator* backing
)
{
    JSLAllocatorInterface iface = test_libc_allocator_interface(backing);
    JSLSubProcessCreateResultEnum r = jsl_subprocess_create(
        proc,
        iface,
        jsl_cstr_to_memory(HELPER_PATH)
    );
    return r == JSL_SUBPROCESS_CREATE_SUCCESS;
}

void test_jsl_subprocess_create_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLAllocatorInterface iface = test_libc_allocator_interface(&backing);

    JSLSubprocess proc;
    // NULL proc
    JSLSubProcessCreateResultEnum r = jsl_subprocess_create(
        NULL, iface, jsl_cstr_to_memory("a")
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_CREATE_BAD_PARAMETERS);

    // NULL executable data
    JSLImmutableMemory empty = { NULL, 0 };
    r = jsl_subprocess_create(&proc, iface, empty);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_CREATE_BAD_PARAMETERS);

    // Zero length executable
    JSLImmutableMemory zero = { (const uint8_t*) "x", 0 };
    r = jsl_subprocess_create(&proc, iface, zero);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_CREATE_BAD_PARAMETERS);

    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_create_success(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_NOT_STARTED);
    TEST_INT64_EQUAL(proc.args_count, 0);
    TEST_INT64_EQUAL(proc.env_count, 0);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_args_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    // NULL proc
    JSLImmutableMemory arg = JSL_CSTR_EXPRESSION("hello");
    JSLSubProcessArgResultEnum r = jsl_subprocess_args(NULL, &arg, 1);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ARG_BAD_PARAMETERS);

    // NULL args
    r = jsl_subprocess_args(&proc, NULL, 1);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ARG_BAD_PARAMETERS);

    // Zero count
    r = jsl_subprocess_args(&proc, &arg, 0);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ARG_BAD_PARAMETERS);

    // Bad sentinel
    JSLSubprocess bogus;
    memset(&bogus, 0, sizeof(bogus));
    r = jsl_subprocess_args(&bogus, &arg, 1);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ARG_BAD_PARAMETERS);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_arg_macro(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLSubProcessArgResultEnum r = jsl_subprocess_arg(
        &proc,
        JSL_CSTR_EXPRESSION("exit"),
        JSL_CSTR_EXPRESSION("0")
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ARG_SUCCESS);
    TEST_INT64_EQUAL(proc.args_count, 2);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_arg_cstr_macro(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLSubProcessArgResultEnum r = jsl_subprocess_arg_cstr(&proc, "echo", "hi");
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ARG_SUCCESS);
    TEST_INT64_EQUAL(proc.args_count, 2);
    TEST_INT64_EQUAL(proc.args[0].length, 4);
    TEST_BUFFERS_EQUAL(proc.args[0].data, "echo", 4);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_env_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLImmutableMemory empty = { NULL, 0 };
    JSLImmutableMemory value = JSL_CSTR_EXPRESSION("x");

    JSLSubProcessEnvResultEnum r = jsl_subprocess_env(NULL, value, value);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ENV_BAD_PARAMETERS);

    r = jsl_subprocess_env(&proc, empty, value);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ENV_BAD_PARAMETERS);

    r = jsl_subprocess_env(&proc, value, empty);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ENV_BAD_PARAMETERS);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_change_working_directory_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLImmutableMemory empty = { NULL, 0 };
    TEST_BOOL(!jsl_subprocess_change_working_directory(NULL, JSL_CSTR_EXPRESSION("/")));
    TEST_BOOL(!jsl_subprocess_change_working_directory(&proc, empty));

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_set_stdin_memory_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLImmutableMemory empty = { NULL, 0 };
    TEST_BOOL(!jsl_subprocess_set_stdin_memory(NULL, JSL_CSTR_EXPRESSION("x")));
    TEST_BOOL(!jsl_subprocess_set_stdin_memory(&proc, empty));

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_set_stdin_fd_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_BOOL(!jsl_subprocess_set_stdin_fd(NULL, 0));
    TEST_BOOL(!jsl_subprocess_set_stdin_fd(&proc, -1));

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_set_stdout_fd_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_BOOL(!jsl_subprocess_set_stdout_fd(NULL, 1));
    TEST_BOOL(!jsl_subprocess_set_stdout_fd(&proc, -1));

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_set_stdout_sink_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLOutputSink nil = { NULL, NULL };
    TEST_BOOL(!jsl_subprocess_set_stdout_sink(NULL, nil));
    TEST_BOOL(!jsl_subprocess_set_stdout_sink(&proc, nil));

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_set_stderr_fd_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_BOOL(!jsl_subprocess_set_stderr_fd(NULL, 2));
    TEST_BOOL(!jsl_subprocess_set_stderr_fd(&proc, -1));

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_set_stderr_sink_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLOutputSink nil = { NULL, NULL };
    TEST_BOOL(!jsl_subprocess_set_stderr_sink(NULL, nil));
    TEST_BOOL(!jsl_subprocess_set_stderr_sink(&proc, nil));

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    int32_t exit_code = 0;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(NULL, &exit_code, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    r = jsl_subprocess_run_blocking(&proc, NULL, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_spawn_failed(void)
{
    JSLLibcAllocator backing;
    JSLAllocatorInterface iface = test_libc_allocator_interface(&backing);

    JSLSubprocess proc;
    JSLSubProcessCreateResultEnum cr = jsl_subprocess_create(
        &proc,
        iface,
        jsl_cstr_to_memory("./tests/bin/definitely_not_a_real_binary_xyz")
    );
    TEST_INT32_EQUAL(cr, JSL_SUBPROCESS_CREATE_SUCCESS);

    int32_t exit_code = 0;
    int32_t errno_out = 0;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(
        &proc,
        &exit_code,
        &errno_out
    );

    // posix_spawnp may report the missing executable synchronously (most
    // macOS/BSD libcs) or succeed and let the child exec fail with exit
    // code 127 (some glibc versions). Accept either.
    bool ok = (r == JSL_SUBPROCESS_SPAWN_FAILED)
        || (r == JSL_SUBPROCESS_SUCCESS && exit_code != 0);
    TEST_BOOL(ok);

    if (r == JSL_SUBPROCESS_SPAWN_FAILED)
        TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_FAILED_TO_START);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_exit_code(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "exit", "7"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    int32_t exit_code = -1;
    int32_t errno_out = 0;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(
        &proc, &exit_code, &errno_out
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(exit_code, 7);
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_EXITED);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_already_started(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    int32_t exit_code = -1;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, &exit_code, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    r = jsl_subprocess_run_blocking(&proc, &exit_code, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ALREADY_STARTED);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_stdout_sink(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "echo", "hello-world"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));

    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    int32_t exit_code = -1;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, &exit_code, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, 12); // "hello-world\n"
    if (out.length == 12)
        TEST_BUFFERS_EQUAL(out.data, "hello-world\n", 12);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_stderr_sink(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "stderr", "out-msg", "err-msg"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLLibcAllocator sb_backing_out;
    JSLAllocatorInterface sb_iface_out = test_libc_allocator_interface(&sb_backing_out);

    JSLLibcAllocator sb_backing_err;
    JSLAllocatorInterface sb_iface_err = test_libc_allocator_interface(&sb_backing_err);

    JSLStringBuilder sb_out;
    JSLStringBuilder sb_err;
    TEST_BOOL(jsl_string_builder_init(&sb_out, sb_iface_out, 64));
    TEST_BOOL(jsl_string_builder_init(&sb_err, sb_iface_err, 64));

    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb_out)));
    TEST_BOOL(jsl_subprocess_set_stderr_sink(&proc, jsl_string_builder_output_sink(&sb_err)));

    int32_t exit_code = -1;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, &exit_code, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb_out);
    JSLImmutableMemory err = jsl_string_builder_get_string(&sb_err);
    TEST_INT64_EQUAL(out.length, 7);
    TEST_INT64_EQUAL(err.length, 7);
    if (out.length == 7)
        TEST_BUFFERS_EQUAL(out.data, "out-msg", 7);
    if (err.length == 7)
        TEST_BUFFERS_EQUAL(err.data, "err-msg", 7);

    jsl_string_builder_free(&sb_out);
    jsl_string_builder_free(&sb_err);
    jsl_libc_allocator_free_all(&sb_backing_out);
    jsl_libc_allocator_free_all(&sb_backing_err);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_stdin_memory(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "cat"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    const char* payload = "piped-bytes";
    int64_t payload_len = (int64_t) strlen(payload);
    JSLImmutableMemory input = jsl_cstr_to_memory(payload);
    TEST_BOOL(jsl_subprocess_set_stdin_memory(&proc, input));

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    int32_t exit_code = -1;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, &exit_code, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, payload_len);
    if (out.length == payload_len)
        TEST_BUFFERS_EQUAL(out.data, payload, (size_t) payload_len);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_env_var(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "env", "JSL_TEST_SUBPROCESS_VAR"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_env(
            &proc,
            JSL_CSTR_EXPRESSION("JSL_TEST_SUBPROCESS_VAR"),
            JSL_CSTR_EXPRESSION("env-value-42")
        ),
        JSL_SUBPROCESS_ENV_SUCCESS
    );

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    int32_t exit_code = -1;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, &exit_code, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, 12);
    if (out.length == 12)
        TEST_BUFFERS_EQUAL(out.data, "env-value-42", 12);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

// Drain a background subprocess until it exits: pump stdin + output, then
// poll with a short timeout, repeating. Final pump on output after exit to
// scoop up anything still buffered. Returns final status.
static JSLSubProcessStatusEnum drain_until_exit(JSLSubprocess* proc, int32_t* out_exit_code)
{
    JSLSubProcessStatusEnum status = JSL_SUBPROCESS_STATUS_RUNNING;
    int32_t exit_code = -1;
    for (int i = 0; i < 1000; i++)
    {
        (void) jsl_subprocess_background_send_stdin(proc, NULL);
        (void) jsl_subprocess_background_receive_output(proc, NULL);
        JSLSubProcessResultEnum r = jsl_subprocess_background_poll(
            proc, 20, &status, &exit_code, NULL
        );
        if (r != JSL_SUBPROCESS_SUCCESS)
            break;
        if (status != JSL_SUBPROCESS_STATUS_RUNNING)
            break;
    }
    (void) jsl_subprocess_background_receive_output(proc, NULL);
    if (out_exit_code != NULL)
        *out_exit_code = exit_code;
    return status;
}

void test_jsl_subprocess_background_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    // pump/poll/kill on a NOT_STARTED (non-background) proc → BAD_PARAMETERS
    JSLSubProcessStatusEnum status = JSL_SUBPROCESS_STATUS_NOT_STARTED;
    int32_t exit_code = -1;
    TEST_INT32_EQUAL(
        jsl_subprocess_background_poll(&proc, 0, &status, &exit_code, NULL),
        JSL_SUBPROCESS_BAD_PARAMETERS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_background_send_stdin(&proc, NULL),
        JSL_SUBPROCESS_BAD_PARAMETERS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_background_receive_output(&proc, NULL),
        JSL_SUBPROCESS_BAD_PARAMETERS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_background_kill(&proc, NULL),
        JSL_SUBPROCESS_BAD_PARAMETERS
    );

    // NULL proc
    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(NULL, NULL),
        JSL_SUBPROCESS_BAD_PARAMETERS
    );

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_already_started(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLSubProcessResultEnum r = jsl_subprocess_background_start(&proc, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    // A second start attempt must fail with ALREADY_STARTED.
    JSLSubProcessResultEnum r2 = jsl_subprocess_background_start(&proc, NULL);
    TEST_INT32_EQUAL(r2, JSL_SUBPROCESS_ALREADY_STARTED);

    int32_t exit_code = -1;
    JSLSubProcessStatusEnum status = drain_until_exit(&proc, &exit_code);
    TEST_INT32_EQUAL(status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(exit_code, 0);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_stdout_sink(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "echo", "hello-world"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    int32_t exit_code = -1;
    JSLSubProcessStatusEnum status = drain_until_exit(&proc, &exit_code);
    TEST_INT32_EQUAL(status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, 12);
    if (out.length == 12)
        TEST_BUFFERS_EQUAL(out.data, "hello-world\n", 12);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_stderr_sink(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "stderr", "out-msg", "err-msg"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLLibcAllocator sb_backing_out;
    JSLAllocatorInterface sb_iface_out = test_libc_allocator_interface(&sb_backing_out);
    JSLLibcAllocator sb_backing_err;
    JSLAllocatorInterface sb_iface_err = test_libc_allocator_interface(&sb_backing_err);

    JSLStringBuilder sb_out;
    JSLStringBuilder sb_err;
    TEST_BOOL(jsl_string_builder_init(&sb_out, sb_iface_out, 64));
    TEST_BOOL(jsl_string_builder_init(&sb_err, sb_iface_err, 64));

    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb_out)));
    TEST_BOOL(jsl_subprocess_set_stderr_sink(&proc, jsl_string_builder_output_sink(&sb_err)));

    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    int32_t exit_code = -1;
    JSLSubProcessStatusEnum status = drain_until_exit(&proc, &exit_code);
    TEST_INT32_EQUAL(status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb_out);
    JSLImmutableMemory err = jsl_string_builder_get_string(&sb_err);
    TEST_INT64_EQUAL(out.length, 7);
    TEST_INT64_EQUAL(err.length, 7);
    if (out.length == 7)
        TEST_BUFFERS_EQUAL(out.data, "out-msg", 7);
    if (err.length == 7)
        TEST_BUFFERS_EQUAL(err.data, "err-msg", 7);

    jsl_string_builder_free(&sb_out);
    jsl_string_builder_free(&sb_err);
    jsl_libc_allocator_free_all(&sb_backing_out);
    jsl_libc_allocator_free_all(&sb_backing_err);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_stdin_memory(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "cat"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    const char* payload = "piped-bytes";
    int64_t payload_len = (int64_t) strlen(payload);
    TEST_BOOL(jsl_subprocess_set_stdin_memory(&proc, jsl_cstr_to_memory(payload)));

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    int32_t exit_code = -1;
    JSLSubProcessStatusEnum status = drain_until_exit(&proc, &exit_code);
    TEST_INT32_EQUAL(status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, payload_len);
    if (out.length == payload_len)
        TEST_BUFFERS_EQUAL(out.data, payload, (size_t) payload_len);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_env_var(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "env", "JSL_TEST_SUBPROCESS_BG_VAR"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_env(
            &proc,
            JSL_CSTR_EXPRESSION("JSL_TEST_SUBPROCESS_BG_VAR"),
            JSL_CSTR_EXPRESSION("bg-value-7")
        ),
        JSL_SUBPROCESS_ENV_SUCCESS
    );

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);
    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    int32_t exit_code = -1;
    JSLSubProcessStatusEnum status = drain_until_exit(&proc, &exit_code);
    TEST_INT32_EQUAL(status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, 10);
    if (out.length == 10)
        TEST_BUFFERS_EQUAL(out.data, "bg-value-7", 10);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_kill(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    // "cat" reads stdin until EOF, but we never close stdin, so it runs forever.
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "cat"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    // Confirm it is running, then kill.
    JSLSubProcessStatusEnum status = JSL_SUBPROCESS_STATUS_NOT_STARTED;
    int32_t exit_code = -1;
    TEST_INT32_EQUAL(
        jsl_subprocess_background_poll(&proc, 0, &status, &exit_code, NULL),
        JSL_SUBPROCESS_SUCCESS
    );
    TEST_INT32_EQUAL(status, JSL_SUBPROCESS_STATUS_RUNNING);

    TEST_INT32_EQUAL(
        jsl_subprocess_background_kill(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    status = drain_until_exit(&proc, &exit_code);
    // On POSIX we expect KILLED_BY_SIGNAL, on Windows EXITED with code 1.
    #if JSL_IS_WINDOWS
        TEST_INT32_EQUAL(status, JSL_SUBPROCESS_STATUS_EXITED);
    #else
        TEST_BOOL(
            status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL
            || status == JSL_SUBPROCESS_STATUS_EXITED
        );
    #endif

    // kill after exit is a no-op SUCCESS
    TEST_INT32_EQUAL(
        jsl_subprocess_background_kill(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_working_directory(void)
{
    JSLLibcAllocator backing;
    JSLAllocatorInterface iface = test_libc_allocator_interface(&backing);

    // The subprocess needs to resolve the helper executable relative to
    // the *new* working directory, so give it an absolute path.
    char cwd[4096];
    #if JSL_IS_WINDOWS
        TEST_BOOL(_getcwd(cwd, sizeof(cwd)) != NULL);
    #else
        TEST_BOOL(getcwd(cwd, sizeof(cwd)) != NULL);
    #endif

    char abs_helper[4096];
    #if JSL_IS_WINDOWS
        snprintf(abs_helper, sizeof(abs_helper), "%s\\%s", cwd, HELPER_PATH);
    #else
        snprintf(abs_helper, sizeof(abs_helper), "%s/%s", cwd, HELPER_PATH);
    #endif

    JSLSubprocess proc;
    JSLSubProcessCreateResultEnum cr = jsl_subprocess_create(
        &proc, iface, jsl_cstr_to_memory(abs_helper)
    );
    TEST_INT32_EQUAL(cr, JSL_SUBPROCESS_CREATE_SUCCESS);

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "cwd"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    // The "tests" directory is guaranteed to exist from the project root.
    #if JSL_IS_WINDOWS
        const char* new_dir_rel = "tests";
    #else
        const char* new_dir_rel = "tests";
    #endif
    TEST_BOOL(jsl_subprocess_change_working_directory(
        &proc, jsl_cstr_to_memory(new_dir_rel)
    ));

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    int32_t exit_code = -1;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, &exit_code, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    // The child's cwd should end with "tests" (or "\\tests" on Windows).
    TEST_BOOL(out.length >= 5);
    if (out.length >= 5)
    {
        const uint8_t* tail = out.data + out.length - 5;
        TEST_BUFFERS_EQUAL(tail, "tests", 5);
    }

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

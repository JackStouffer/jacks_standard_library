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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(NULL, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    // Bad sentinel
    JSLSubprocess bogus;
    memset(&bogus, 0, sizeof(bogus));
    r = jsl_subprocess_run_blocking(&bogus, NULL);
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

    int32_t errno_out = 0;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(
        &proc,
        &errno_out
    );

    // posix_spawnp may report the missing executable synchronously (most
    // macOS/BSD libcs) or succeed and let the child exec fail with exit
    // code 127 (some glibc versions). Accept either.
    bool ok = (r == JSL_SUBPROCESS_SPAWN_FAILED)
        || (r == JSL_SUBPROCESS_SUCCESS && proc.exit_code != 0);
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

    int32_t errno_out = 0;
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(
        &proc, &errno_out
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 7);
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    r = jsl_subprocess_run_blocking(&proc, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

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

void test_jsl_subprocess_background_wait_bad_parameters(void)
{
    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(NULL, 0, -1, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    JSLSubprocess dummy;
    r = jsl_subprocess_background_wait(&dummy, 0, 0, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    r = jsl_subprocess_background_wait(NULL, 1, 0, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);
}

void test_jsl_subprocess_background_wait_all_ignored(void)
{
    // A proc that's NOT started + one with zeroed sentinel. The wait
    // should silently ignore both and return SUCCESS without hanging.
    JSLLibcAllocator backing;
    JSLSubprocess procs[2];
    TEST_BOOL(make_helper(&procs[0], &backing));
    memset(&procs[1], 0, sizeof(procs[1]));

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(procs, 2, 1000, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    jsl_subprocess_cleanup(&procs[0]);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_wait_pre_exited(void)
{
    // A proc that has already reached EXITED should be treated as
    // trivially satisfied. Start + drain one proc, then batch-wait.
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "exit", "3"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );
    (void) drain_until_exit(&proc, NULL);
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_EXITED);

    int32_t before_code = proc.exit_code;
    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(&proc, 1, 1000, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(proc.exit_code, before_code);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_wait_many_healthy(void)
{
    // N healthy procs with stdout sinks. They all finish normally and
    // `background_wait` returns SUCCESS with the captured output
    // intact for each one. Run enough procs to shake out accounting
    // bugs but few enough to stay well inside Windows' batching cap.
    enum { N = 5 };
    JSLLibcAllocator backing[N];
    JSLLibcAllocator sb_backing[N];
    JSLSubprocess procs[N];
    JSLStringBuilder sbs[N];

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "echo", "ok"),
            JSL_SUBPROCESS_ARG_SUCCESS
        );

        JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing[i]);
        TEST_BOOL(jsl_string_builder_init(&sbs[i], sb_iface, 32));
        TEST_BOOL(jsl_subprocess_set_stdout_sink(
            &procs[i], jsl_string_builder_output_sink(&sbs[i])
        ));

        TEST_INT32_EQUAL(
            jsl_subprocess_background_start(&procs[i], NULL),
            JSL_SUBPROCESS_SUCCESS
        );
    }

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(procs, N, 5000, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    for (int i = 0; i < N; i++)
    {
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_EXITED);
        TEST_INT32_EQUAL(procs[i].exit_code, 0);
        JSLImmutableMemory out = jsl_string_builder_get_string(&sbs[i]);
        TEST_INT64_EQUAL(out.length, 3); // "ok\n"
        if (out.length == 3)
            TEST_BUFFERS_EQUAL(out.data, "ok\n", 3);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_string_builder_free(&sbs[i]);
        jsl_libc_allocator_free_all(&sb_backing[i]);
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_background_wait_timeout(void)
{
    // One fast proc (echo) and one slow proc (sleep 3s). With a small
    // timeout we expect TIMEOUT_REACHED, the fast proc reported
    // EXITED, and the slow one still RUNNING. Then we kill and drain.
    JSLLibcAllocator backing_fast, backing_slow;
    JSLSubprocess procs[2];
    TEST_BOOL(make_helper(&procs[0], &backing_fast));
    TEST_BOOL(make_helper(&procs[1], &backing_slow));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[0], "echo", "fast"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[1], "sleep", "3000"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&procs[0], NULL),
        JSL_SUBPROCESS_SUCCESS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&procs[1], NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(procs, 2, 300, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_TIMEOUT_REACHED);
    TEST_INT32_EQUAL(procs[0].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_BOOL(procs[1].status == JSL_SUBPROCESS_STATUS_RUNNING);

    // Kill the slow one and drain it.
    TEST_INT32_EQUAL(
        jsl_subprocess_background_kill(&procs[1], NULL),
        JSL_SUBPROCESS_SUCCESS
    );
    (void) drain_until_exit(&procs[1], NULL);

    jsl_subprocess_cleanup(&procs[0]);
    jsl_subprocess_cleanup(&procs[1]);
    jsl_libc_allocator_free_all(&backing_fast);
    jsl_libc_allocator_free_all(&backing_slow);
}

void test_jsl_subprocess_background_wait_heavy_stdout(void)
{
    // A child that produces enough stdout to block on the pipe buffer
    // if we fail to pump. The auto-pumping in `background_wait` should
    // keep the child moving to completion. Uses a payload well north
    // of any realistic pipe-buffer size.
    const int payload_bytes = 200000;

    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d", payload_bytes);
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "spew", count_str),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);
    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 4096));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    TEST_INT32_EQUAL(
        jsl_subprocess_background_start(&proc, NULL),
        JSL_SUBPROCESS_SUCCESS
    );

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(&proc, 1, 10000, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(proc.exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, (int64_t) payload_bytes);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_wait_single_stdin_memory(void)
{
    // Exercise the stdin-pump path inside `background_wait`. Feed a
    // payload and expect the child (`cat`) to echo it back.
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "cat"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    const char* payload = "batch-wait-stdin";
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

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(&proc, 1, 5000, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(proc.exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, payload_len);
    if (out.length == payload_len)
        TEST_BUFFERS_EQUAL(out.data, payload, (size_t) payload_len);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_spew_large(void)
{
    // Exercises the "drain after the process handle signalled" step of
    // the overlapped run_blocking loop. 200 KB is well beyond the 64 KiB
    // pipe buffer, so completion will often race ahead of the final
    // reader and leave bytes queued on the pipe once the child exits.
    const int payload_bytes = 200000;

    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d", payload_bytes);
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "spew", count_str),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);
    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 4096));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(&proc, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, (int64_t) payload_bytes);

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_wait_17_procs_event_driven(void)
{
    // 17 procs is one more than the Windows group-size cap of 16, so
    // this shakes out group-rotation fairness: every group must make
    // forward progress, none may starve. The procs are tiny (`echo ok`)
    // so completion well inside the 5s budget is the real assertion.
    enum { N = 17 };
    JSLLibcAllocator backing[N];
    JSLLibcAllocator sb_backing[N];
    JSLSubprocess procs[N];
    JSLStringBuilder sbs[N];

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "echo", "ok"),
            JSL_SUBPROCESS_ARG_SUCCESS
        );

        JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing[i]);
        TEST_BOOL(jsl_string_builder_init(&sbs[i], sb_iface, 32));
        TEST_BOOL(jsl_subprocess_set_stdout_sink(
            &procs[i], jsl_string_builder_output_sink(&sbs[i])
        ));

        TEST_INT32_EQUAL(
            jsl_subprocess_background_start(&procs[i], NULL),
            JSL_SUBPROCESS_SUCCESS
        );
    }

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(procs, N, 5000, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    for (int i = 0; i < N; i++)
    {
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_EXITED);
        TEST_INT32_EQUAL(procs[i].exit_code, 0);
        JSLImmutableMemory out = jsl_string_builder_get_string(&sbs[i]);
        TEST_INT64_EQUAL(out.length, 3); // "ok\n"
        if (out.length == 3)
            TEST_BUFFERS_EQUAL(out.data, "ok\n", 3);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_string_builder_free(&sbs[i]);
        jsl_libc_allocator_free_all(&sb_backing[i]);
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

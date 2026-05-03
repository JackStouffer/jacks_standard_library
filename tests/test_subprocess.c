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
    #include <windows.h>
    static const char* HELPER_PATH = "tests\\bin\\subprocess_helper.exe";
#else
    #include <time.h>
    #include <unistd.h>
    static const char* HELPER_PATH = "tests/bin/subprocess_helper";
#endif

static void test_sleep_ms(int ms)
{
#if JSL_IS_WINDOWS
    Sleep((DWORD) ms);
#else
    usleep((useconds_t) ms * 1000u);
#endif
}

static int64_t test_monotonic_ms(void)
{
#if JSL_IS_WINDOWS
    return (int64_t) GetTickCount64();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (int64_t) ts.tv_sec * 1000 + (int64_t) ts.tv_nsec / 1000000;
#endif
}

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

void test_jsl_subprocess_set_env_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLImmutableMemory empty = { NULL, 0 };
    JSLImmutableMemory value = JSL_CSTR_EXPRESSION("x");

    JSLSubProcessEnvResultEnum r = jsl_subprocess_set_env(NULL, value, value);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ENV_BAD_PARAMETERS);

    r = jsl_subprocess_set_env(&proc, empty, value);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ENV_BAD_PARAMETERS);

    r = jsl_subprocess_set_env(&proc, value, empty);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ENV_BAD_PARAMETERS);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_unset_env_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLImmutableMemory empty = { NULL, 0 };
    JSLImmutableMemory key = JSL_CSTR_EXPRESSION("FOO");

    JSLSubProcessEnvResultEnum r = jsl_subprocess_unset_env(NULL, key);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ENV_BAD_PARAMETERS);

    r = jsl_subprocess_unset_env(&proc, empty);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ENV_BAD_PARAMETERS);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_env_override_last_wins(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    JSLImmutableMemory key = JSL_CSTR_EXPRESSION("FOO");
    JSLImmutableMemory v1 = JSL_CSTR_EXPRESSION("first");
    JSLImmutableMemory v2 = JSL_CSTR_EXPRESSION("second");

    TEST_INT32_EQUAL(jsl_subprocess_set_env(&proc, key, v1), JSL_SUBPROCESS_ENV_SUCCESS);
    TEST_INT32_EQUAL(jsl_subprocess_set_env(&proc, key, v2), JSL_SUBPROCESS_ENV_SUCCESS);

    TEST_INT64_EQUAL(proc.env_count, 1);
    TEST_BOOL(!proc.env_vars[0].unset);
    TEST_INT64_EQUAL(proc.env_vars[0].value.length, v2.length);
    TEST_BUFFERS_EQUAL(proc.env_vars[0].value.data, v2.data, (size_t) v2.length);

    TEST_INT32_EQUAL(jsl_subprocess_unset_env(&proc, key), JSL_SUBPROCESS_ENV_SUCCESS);
    TEST_INT64_EQUAL(proc.env_count, 1);
    TEST_BOOL(proc.env_vars[0].unset);

    TEST_INT32_EQUAL(jsl_subprocess_set_env(&proc, key, v1), JSL_SUBPROCESS_ENV_SUCCESS);
    TEST_INT64_EQUAL(proc.env_count, 1);
    TEST_BOOL(!proc.env_vars[0].unset);

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

    // NULL procs
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, NULL, 1, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    // count <= 0
    r = jsl_subprocess_run_blocking(proc.allocator, &proc, 0, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    // Bad sentinel
    JSLSubprocess bogus;
    memset(&bogus, 0, sizeof(bogus));
    r = jsl_subprocess_run_blocking(proc.allocator, &bogus, 1, NULL);
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
        iface,
        &proc,
        1,
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
        proc.allocator, &proc, 1, &errno_out
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
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

void test_jsl_subprocess_set_env_base_bad_parameters(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    TEST_BOOL(!jsl_subprocess_set_env_base(NULL, JSL_SUBPROCESS_ENV_BASE_EMPTY));
    TEST_BOOL(!jsl_subprocess_set_env_base(&proc, (JSLSubProcessEnvBaseEnum) -1));
    TEST_BOOL(!jsl_subprocess_set_env_base(&proc, JSL_SUBPROCESS_ENV_BASE_ENUM_COUNT));

    // Successful call flips the field.
    TEST_BOOL(jsl_subprocess_set_env_base(&proc, JSL_SUBPROCESS_ENV_BASE_INHERIT));
    TEST_INT32_EQUAL(proc.env_base, JSL_SUBPROCESS_ENV_BASE_INHERIT);
    TEST_BOOL(jsl_subprocess_set_env_base(&proc, JSL_SUBPROCESS_ENV_BASE_EMPTY));
    TEST_INT32_EQUAL(proc.env_base, JSL_SUBPROCESS_ENV_BASE_EMPTY);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_env_base_empty(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    // Plant a value in the parent env. With the default EMPTY base it
    // must NOT leak into the child.
#if JSL_IS_WINDOWS
    _putenv_s("JSL_TEST_SUBPROCESS_BASE_EMPTY_VAR", "leaked-value");
#else
    setenv("JSL_TEST_SUBPROCESS_BASE_EMPTY_VAR", "leaked-value", 1);
#endif

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "env", "JSL_TEST_SUBPROCESS_BASE_EMPTY_VAR"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, 6);
    if (out.length == 6)
        TEST_BUFFERS_EQUAL(out.data, "<null>", 6);

#if JSL_IS_WINDOWS
    _putenv_s("JSL_TEST_SUBPROCESS_BASE_EMPTY_VAR", "");
#else
    unsetenv("JSL_TEST_SUBPROCESS_BASE_EMPTY_VAR");
#endif

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_env_base_inherit(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

#if JSL_IS_WINDOWS
    _putenv_s("JSL_TEST_SUBPROCESS_BASE_INHERIT_VAR", "inherited-99");
#else
    setenv("JSL_TEST_SUBPROCESS_BASE_INHERIT_VAR", "inherited-99", 1);
#endif

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "env", "JSL_TEST_SUBPROCESS_BASE_INHERIT_VAR"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_BOOL(jsl_subprocess_set_env_base(&proc, JSL_SUBPROCESS_ENV_BASE_INHERIT));

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, 12);
    if (out.length == 12)
        TEST_BUFFERS_EQUAL(out.data, "inherited-99", 12);

#if JSL_IS_WINDOWS
    _putenv_s("JSL_TEST_SUBPROCESS_BASE_INHERIT_VAR", "");
#else
    unsetenv("JSL_TEST_SUBPROCESS_BASE_INHERIT_VAR");
#endif

    jsl_string_builder_free(&sb);
    jsl_libc_allocator_free_all(&sb_backing);
    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_unset_env(void)
{
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));

    // Plant a value in the parent env so we can prove unset removes it
    // from the inherited set rather than only undoing a prior set_env.
#if JSL_IS_WINDOWS
    _putenv_s("JSL_TEST_SUBPROCESS_UNSET_VAR", "leaked-value");
#else
    setenv("JSL_TEST_SUBPROCESS_UNSET_VAR", "leaked-value", 1);
#endif

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "env", "JSL_TEST_SUBPROCESS_UNSET_VAR"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    // The base must be INHERIT for unset_env to have anything to remove —
    // the default EMPTY base would mask the planted parent var on its own.
    TEST_BOOL(jsl_subprocess_set_env_base(&proc, JSL_SUBPROCESS_ENV_BASE_INHERIT));
    TEST_INT32_EQUAL(
        jsl_subprocess_unset_env(
            &proc,
            JSL_CSTR_EXPRESSION("JSL_TEST_SUBPROCESS_UNSET_VAR")
        ),
        JSL_SUBPROCESS_ENV_SUCCESS
    );

    JSLLibcAllocator sb_backing;
    JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing);

    JSLStringBuilder sb;
    TEST_BOOL(jsl_string_builder_init(&sb, sb_iface, 64));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&sb)));

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_INT32_EQUAL(proc.exit_code, 0);

    JSLImmutableMemory out = jsl_string_builder_get_string(&sb);
    TEST_INT64_EQUAL(out.length, 6);
    if (out.length == 6)
        TEST_BUFFERS_EQUAL(out.data, "<null>", 6);

#if JSL_IS_WINDOWS
    _putenv_s("JSL_TEST_SUBPROCESS_UNSET_VAR", "");
#else
    unsetenv("JSL_TEST_SUBPROCESS_UNSET_VAR");
#endif

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
        jsl_subprocess_set_env(
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
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
        jsl_subprocess_set_env(
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
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
    JSLLibcAllocator backing;
    JSLAllocatorInterface iface = test_libc_allocator_interface(&backing);

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(iface, NULL, 0, -1, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    JSLSubprocess dummy;
    r = jsl_subprocess_background_wait(iface, &dummy, 0, 0, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    r = jsl_subprocess_background_wait(iface, NULL, 1, 0, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);

    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_background_wait_all_ignored(void)
{
    // A proc that's NOT started + one with zeroed sentinel. The wait
    // should silently ignore both and return SUCCESS without hanging.
    JSLLibcAllocator backing;
    JSLSubprocess procs[2];
    TEST_BOOL(make_helper(&procs[0], &backing));
    memset(&procs[1], 0, sizeof(procs[1]));

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(procs[0].allocator, procs, 2, 1000, NULL);
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
    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(proc.allocator, &proc, 1, 1000, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(procs[0].allocator, procs, N, 5000, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(procs[0].allocator, procs, 2, 300, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(proc.allocator, &proc, 1, 10000, NULL);
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

void test_jsl_subprocess_background_wait_post_exit_drain(void)
{
    // Regression test: a child that writes more than the per-read buffer
    // (4 KiB on POSIX) and exits BEFORE the parent calls background_wait
    // hits only the pre-reap + opportunistic-drain paths — the main
    // poll/wait loop never runs because waiting_count is already 0.
    // If the drain helper does only one read per stream, anything past
    // its buffer size silently disappears. 16 KiB fits inside a typical
    // 64 KiB pipe buffer, so the child writes everything and exits
    // without blocking, leaving the bytes queued for the parent.
    const int payload_bytes = 16384;

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

    // Give the child time to write its payload and exit. The parent has
    // not pumped anything yet, so when background_wait runs the data is
    // sitting in the pipe buffer with the child already terminal.
    test_sleep_ms(200);

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(proc.allocator, &proc, 1, 5000, NULL);
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

void test_jsl_subprocess_background_wait_drain_after_poll_exited(void)
{
    // Regression test: caller polls until the proc is observed as
    // EXITED, then calls background_wait expecting it to drain any
    // remaining buffered output. On Windows the prime-pump loop used
    // `still_waiting` as its predicate, which is `false` for
    // already-terminal procs after the first-pass classifier — so the
    // drain pass never ran and the buffered bytes were silently lost.
    // POSIX correctly uses `!ignored` for its opportunistic drain, so
    // this scenario only fails on Windows pre-fix.
    const int payload_bytes = 16384;

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

    // Poll until the proc is observed terminal. This transitions
    // proc->status to EXITED before background_wait is ever called,
    // which is the exact precondition that triggers the bug. Caller
    // intentionally does NOT call receive_output here — the contract
    // under test is that background_wait performs a final drain.
    JSLSubProcessStatusEnum status = JSL_SUBPROCESS_STATUS_RUNNING;
    int32_t exit_code = -1;
    int32_t poll_deadline_ms = 5000;
    int32_t step_ms = 50;
    int32_t waited_ms = 0;
    while (status != JSL_SUBPROCESS_STATUS_EXITED
        && status != JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL
        && waited_ms < poll_deadline_ms)
    {
        JSLSubProcessResultEnum pr = jsl_subprocess_background_poll(
            &proc, step_ms, &status, &exit_code, NULL
        );
        TEST_INT32_EQUAL(pr, JSL_SUBPROCESS_SUCCESS);
        waited_ms += step_ms;
    }
    TEST_INT32_EQUAL(status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_EXITED);

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(proc.allocator, &proc, 1, 5000, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

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

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(proc.allocator, &proc, 1, 5000, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(proc.allocator, &proc, 1, NULL);
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

    JSLSubProcessResultEnum r = jsl_subprocess_background_wait(procs[0].allocator, procs, N, 5000, NULL);
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
void test_jsl_subprocess_run_blocking_multi_mixed_exits(void)
{
    // Three procs, three different exit codes. Confirms aggregated
    // SUCCESS even when individual procs exit non-zero, and that
    // each proc's exit_code is recorded independently.
    enum { N = 3 };
    JSLLibcAllocator backing[N];
    JSLSubprocess procs[N];
    const int codes[N] = { 0, 1, 7 };

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        char code_str[16];
        snprintf(code_str, sizeof(code_str), "%d", codes[i]);
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "exit", code_str),
            JSL_SUBPROCESS_ARG_SUCCESS
        );
    }

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(procs[0].allocator, procs, N, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    for (int i = 0; i < N; i++)
    {
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_EXITED);
        TEST_INT32_EQUAL(procs[i].exit_code, codes[i]);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_multi_already_started_rejects_all(void)
{
    // If any proc in the batch is non-NOT_STARTED, the whole call must
    // be rejected with ALREADY_STARTED before any spawn happens. The
    // second proc (NOT_STARTED at call time) must remain NOT_STARTED
    // after the rejected call.
    JSLLibcAllocator backing0, backing1;
    JSLSubprocess procs[2];
    TEST_BOOL(make_helper(&procs[0], &backing0));
    TEST_BOOL(make_helper(&procs[1], &backing1));

    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[0], "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[1], "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    // Run the first proc on its own so its status flips to EXITED.
    TEST_INT32_EQUAL(
        jsl_subprocess_run_blocking(procs[0].allocator, &procs[0], 1, NULL),
        JSL_SUBPROCESS_SUCCESS
    );
    TEST_INT32_EQUAL(procs[0].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(procs[1].status, JSL_SUBPROCESS_STATUS_NOT_STARTED);

    // Now batch them. procs[0] is no longer NOT_STARTED, so the whole
    // call must fail and procs[1] must NOT have been spawned.
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(procs[0].allocator, procs, 2, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_ALREADY_STARTED);
    TEST_INT32_EQUAL(procs[1].status, JSL_SUBPROCESS_STATUS_NOT_STARTED);

    jsl_subprocess_cleanup(&procs[0]);
    jsl_subprocess_cleanup(&procs[1]);
    jsl_libc_allocator_free_all(&backing0);
    jsl_libc_allocator_free_all(&backing1);
}

void test_jsl_subprocess_run_blocking_multi_partial_spawn_failure(void)
{
    // A failure to spawn one proc must not abort the batch — siblings
    // still run to completion. Top-level return is SPAWN_FAILED, the
    // failed proc has FAILED_TO_START, the others EXITED with the
    // expected codes.
    enum { N = 3 };
    JSLLibcAllocator backing[N];
    JSLAllocatorInterface ifaces[N];
    JSLSubprocess procs[N];

    // procs[0]: real, exits 0
    TEST_BOOL(make_helper(&procs[0], &backing[0]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[0], "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    // procs[1]: bogus path — spawn (or exec) fails
    ifaces[1] = test_libc_allocator_interface(&backing[1]);
    TEST_INT32_EQUAL(
        jsl_subprocess_create(
            &procs[1],
            ifaces[1],
            jsl_cstr_to_memory("./tests/bin/definitely_not_a_real_binary_xyz")
        ),
        JSL_SUBPROCESS_CREATE_SUCCESS
    );

    // procs[2]: real, exits 5
    TEST_BOOL(make_helper(&procs[2], &backing[2]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[2], "exit", "5"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(procs[0].allocator, procs, N, NULL);

    // posix_spawnp may report missing-executable synchronously
    // (macOS/BSD) or let the child exec fail with code 127 (some
    // glibc). Accept either: aggregated return is SPAWN_FAILED in
    // the synchronous case, SUCCESS in the async-exec-fail case.
    bool sync_fail = (r == JSL_SUBPROCESS_SPAWN_FAILED
        && procs[1].status == JSL_SUBPROCESS_STATUS_FAILED_TO_START);
    bool async_fail = (r == JSL_SUBPROCESS_SUCCESS
        && procs[1].status == JSL_SUBPROCESS_STATUS_EXITED
        && procs[1].exit_code != 0);
    TEST_BOOL(sync_fail || async_fail);

    // Either way, the surviving siblings must have run to completion.
    TEST_INT32_EQUAL(procs[0].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(procs[0].exit_code, 0);
    TEST_INT32_EQUAL(procs[2].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(procs[2].exit_code, 5);

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_options_multi_timeout_kills_all(void)
{
    // Mixed batch: two quick exits and one long sleep. With a tight
    // deadline the call must return TIMEOUT_REACHED, every proc must
    // be terminal on return, and the sleeper must have been killed
    // (not still RUNNING) — that is _options' contract. Parallelism
    // is N so all three are spawned up front and the sleeper is in
    // the live set when the deadline fires.
    enum { N = 3 };
    JSLLibcAllocator backing[N];
    JSLSubprocess procs[N];

    TEST_BOOL(make_helper(&procs[0], &backing[0]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[0], "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_BOOL(make_helper(&procs[1], &backing[1]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[1], "sleep", "5000"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_BOOL(make_helper(&procs[2], &backing[2]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[2], "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        procs[0].allocator, procs, N, N, 200, NULL
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_TIMEOUT_REACHED);

    // Every proc must be in a terminal state — none still RUNNING
    // and none NOT_STARTED.
    for (int i = 0; i < N; i++)
    {
        bool terminal = (procs[i].status == JSL_SUBPROCESS_STATUS_EXITED
            || procs[i].status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL);
        TEST_BOOL(terminal);
    }

    // The sleeper specifically should have been killed.
    TEST_INT32_EQUAL(procs[1].status, JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL);

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_multi_heterogeneous_io(void)
{
    // Three procs with different I/O wiring exercised in a single
    // batch: stdin_memory + stdout_sink (cat), stderr_sink only,
    // and a plain `exit 3`. Confirms each proc's pumps drive the
    // right per-proc fds inside the batch wait loop.
    enum { N = 3 };
    JSLLibcAllocator backing[N];
    JSLSubprocess procs[N];

    JSLLibcAllocator sb_cat_backing;
    JSLAllocatorInterface sb_cat_iface = test_libc_allocator_interface(&sb_cat_backing);
    JSLStringBuilder sb_cat;
    TEST_BOOL(jsl_string_builder_init(&sb_cat, sb_cat_iface, 64));

    JSLLibcAllocator sb_err_backing;
    JSLAllocatorInterface sb_err_iface = test_libc_allocator_interface(&sb_err_backing);
    JSLStringBuilder sb_err;
    TEST_BOOL(jsl_string_builder_init(&sb_err, sb_err_iface, 64));

    // 0: cat with stdin "hello-cat"
    TEST_BOOL(make_helper(&procs[0], &backing[0]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[0], "cat"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_BOOL(jsl_subprocess_set_stdin_memory(
        &procs[0], jsl_cstr_to_memory("hello-cat")
    ));
    TEST_BOOL(jsl_subprocess_set_stdout_sink(
        &procs[0], jsl_string_builder_output_sink(&sb_cat)
    ));

    // 1: stderr "ignored" "err-payload" — only the stderr side has a sink.
    TEST_BOOL(make_helper(&procs[1], &backing[1]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[1], "stderr", "ignored", "err-payload"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );
    TEST_BOOL(jsl_subprocess_set_stderr_sink(
        &procs[1], jsl_string_builder_output_sink(&sb_err)
    ));

    // 2: nothing wired, just exit 3
    TEST_BOOL(make_helper(&procs[2], &backing[2]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[2], "exit", "3"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(procs[0].allocator, procs, N, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    TEST_INT32_EQUAL(procs[0].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(procs[0].exit_code, 0);
    TEST_INT32_EQUAL(procs[1].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(procs[1].exit_code, 0);
    TEST_INT32_EQUAL(procs[2].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(procs[2].exit_code, 3);

    JSLImmutableMemory cat_out = jsl_string_builder_get_string(&sb_cat);
    TEST_INT64_EQUAL(cat_out.length, 9);
    if (cat_out.length == 9)
        TEST_BUFFERS_EQUAL(cat_out.data, "hello-cat", 9);

    JSLImmutableMemory err_out = jsl_string_builder_get_string(&sb_err);
    TEST_INT64_EQUAL(err_out.length, 11);
    if (err_out.length == 11)
        TEST_BUFFERS_EQUAL(err_out.data, "err-payload", 11);

    jsl_string_builder_free(&sb_cat);
    jsl_string_builder_free(&sb_err);
    jsl_libc_allocator_free_all(&sb_cat_backing);
    jsl_libc_allocator_free_all(&sb_err_backing);
    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_multi_17_procs(void)
{
    // 17 procs exercises wrapper pipelining: the live set is bounded
    // by `jsl_get_logical_processor_count()` (and additionally by 16
    // on Windows, since the WaitForMultipleObjectsEx ceiling is 64
    // handles / 4 per proc). All 17 must still complete, and per-proc
    // sinks must not get cross-wired.
    enum { N = 17 };
    JSLLibcAllocator backing[N];
    JSLLibcAllocator sb_backing[N];
    JSLSubprocess procs[N];
    JSLStringBuilder sbs[N];

    char expected[N][16];

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        char idx_str[16];
        snprintf(idx_str, sizeof(idx_str), "p%d", i);
        snprintf(expected[i], sizeof(expected[i]), "p%d\n", i);
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "echo", idx_str),
            JSL_SUBPROCESS_ARG_SUCCESS
        );

        JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing[i]);
        TEST_BOOL(jsl_string_builder_init(&sbs[i], sb_iface, 32));
        TEST_BOOL(jsl_subprocess_set_stdout_sink(
            &procs[i], jsl_string_builder_output_sink(&sbs[i])
        ));
    }

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking(procs[0].allocator, procs, N, NULL);
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    for (int i = 0; i < N; i++)
    {
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_EXITED);
        TEST_INT32_EQUAL(procs[i].exit_code, 0);
        JSLImmutableMemory out = jsl_string_builder_get_string(&sbs[i]);
        int64_t expected_len = (int64_t) strlen(expected[i]);
        TEST_INT64_EQUAL(out.length, expected_len);
        if (out.length == expected_len)
            TEST_BUFFERS_EQUAL(out.data, expected[i], (size_t) expected_len);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_string_builder_free(&sbs[i]);
        jsl_libc_allocator_free_all(&sb_backing[i]);
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_options_parallelism_one(void)
{
    // parallelism_count == 1 with N>1 procs: each proc spawns only
    // after the previous one has been reaped. All must complete with
    // the expected per-proc exit codes.
    enum { N = 3 };
    JSLLibcAllocator backing[N];
    JSLSubprocess procs[N];
    const int codes[N] = { 0, 4, 9 };

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        char code_str[16];
        snprintf(code_str, sizeof(code_str), "%d", codes[i]);
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "exit", code_str),
            JSL_SUBPROCESS_ARG_SUCCESS
        );
    }

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        procs[0].allocator, procs, N, 1, -1, NULL
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    for (int i = 0; i < N; i++)
    {
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_EXITED);
        TEST_INT32_EQUAL(procs[i].exit_code, codes[i]);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_options_parallelism_caps_live(void)
{
    // 4 procs each sleeping ~300 ms with parallelism=2 must take at
    // least ~600 ms wall (two waves of two). If the cap were ignored
    // and all 4 ran in parallel, total would be ~300 ms. We assert
    // > 500 ms to leave headroom for jitter.
    enum { N = 4 };
    JSLLibcAllocator backing[N];
    JSLSubprocess procs[N];

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "sleep", "300"),
            JSL_SUBPROCESS_ARG_SUCCESS
        );
    }

    int64_t start = test_monotonic_ms();
    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        procs[0].allocator, procs, N, 2, -1, NULL
    );
    int64_t elapsed = test_monotonic_ms() - start;

    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);
    TEST_BOOL(elapsed > 500);

    for (int i = 0; i < N; i++)
    {
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_EXITED);
        TEST_INT32_EQUAL(procs[i].exit_code, 0);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_options_bad_parallelism(void)
{
    // parallelism_count <= 0 is rejected up-front with BAD_PARAMETERS.
    // The proc must remain NOT_STARTED in both branches.
    JSLLibcAllocator backing;
    JSLSubprocess proc;
    TEST_BOOL(make_helper(&proc, &backing));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&proc, "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        proc.allocator, &proc, 1, 0, -1, NULL
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_NOT_STARTED);

    r = jsl_subprocess_run_blocking_options(
        proc.allocator, &proc, 1, -1, -1, NULL
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_BAD_PARAMETERS);
    TEST_INT32_EQUAL(proc.status, JSL_SUBPROCESS_STATUS_NOT_STARTED);

    jsl_subprocess_cleanup(&proc);
    jsl_libc_allocator_free_all(&backing);
}

void test_jsl_subprocess_run_blocking_options_clamp_above_count(void)
{
    // parallelism_count > count is silently clamped down to count;
    // every proc still runs to completion.
    enum { N = 3 };
    JSLLibcAllocator backing[N];
    JSLSubprocess procs[N];

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "exit", "0"),
            JSL_SUBPROCESS_ARG_SUCCESS
        );
    }

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        procs[0].allocator, procs, N, 100, -1, NULL
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    for (int i = 0; i < N; i++)
    {
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_EXITED);
        TEST_INT32_EQUAL(procs[i].exit_code, 0);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_options_timeout_unspawned_stay_not_started(void)
{
    // parallelism=1 forces strict serial spawning. With a tight
    // deadline (200 ms) and procs that sleep 5s each, the first proc
    // is alive when the deadline fires and is killed; later procs
    // never get spawned and must stay NOT_STARTED.
    enum { N = 3 };
    JSLLibcAllocator backing[N];
    JSLSubprocess procs[N];

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "sleep", "5000"),
            JSL_SUBPROCESS_ARG_SUCCESS
        );
    }

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        procs[0].allocator, procs, N, 1, 200, NULL
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_TIMEOUT_REACHED);

    // procs[0] was alive when the deadline fired and got killed.
    TEST_INT32_EQUAL(procs[0].status, JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL);
    // Later procs never got spawned and stay NOT_STARTED.
    TEST_INT32_EQUAL(procs[1].status, JSL_SUBPROCESS_STATUS_NOT_STARTED);
    TEST_INT32_EQUAL(procs[2].status, JSL_SUBPROCESS_STATUS_NOT_STARTED);

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_options_spawn_failure_backfill(void)
{
    // With parallelism=1, a failed spawn at index 1 must NOT consume
    // the live slot — the next pending proc (index 2) is launched
    // immediately, so procs[0] and procs[2] both run to completion.
    enum { N = 3 };
    JSLLibcAllocator backing[N];
    JSLAllocatorInterface ifaces[N];
    JSLSubprocess procs[N];

    TEST_BOOL(make_helper(&procs[0], &backing[0]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[0], "exit", "0"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    ifaces[1] = test_libc_allocator_interface(&backing[1]);
    TEST_INT32_EQUAL(
        jsl_subprocess_create(
            &procs[1],
            ifaces[1],
            jsl_cstr_to_memory("./tests/bin/definitely_not_a_real_binary_xyz")
        ),
        JSL_SUBPROCESS_CREATE_SUCCESS
    );

    TEST_BOOL(make_helper(&procs[2], &backing[2]));
    TEST_INT32_EQUAL(
        jsl_subprocess_arg_cstr(&procs[2], "exit", "5"),
        JSL_SUBPROCESS_ARG_SUCCESS
    );

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        procs[0].allocator, procs, N, 1, -1, NULL
    );

    // posix_spawnp may report missing-executable synchronously
    // (macOS/BSD) or let the child exec fail with code 127 (some
    // glibc). Accept either: aggregated return is SPAWN_FAILED in
    // the synchronous case, SUCCESS in the async-exec-fail case.
    bool sync_fail = (r == JSL_SUBPROCESS_SPAWN_FAILED
        && procs[1].status == JSL_SUBPROCESS_STATUS_FAILED_TO_START);
    bool async_fail = (r == JSL_SUBPROCESS_SUCCESS
        && procs[1].status == JSL_SUBPROCESS_STATUS_EXITED
        && procs[1].exit_code != 0);
    TEST_BOOL(sync_fail || async_fail);

    // Either way, the surviving siblings must have run to completion
    // — a failed spawn must not have consumed the lone parallelism
    // slot.
    TEST_INT32_EQUAL(procs[0].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(procs[0].exit_code, 0);
    TEST_INT32_EQUAL(procs[2].status, JSL_SUBPROCESS_STATUS_EXITED);
    TEST_INT32_EQUAL(procs[2].exit_code, 5);

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_options_high_parallelism_17_procs(void)
{
    // _options companion to multi_17_procs: explicitly request
    // parallelism=32. On Windows the implementation silently clamps
    // to 16 (handle ceiling), and all 17 procs must still complete
    // — observed indirectly by every proc reporting EXITED.
    enum { N = 17 };
    JSLLibcAllocator backing[N];
    JSLLibcAllocator sb_backing[N];
    JSLSubprocess procs[N];
    JSLStringBuilder sbs[N];

    char expected[N][16];

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        char idx_str[16];
        snprintf(idx_str, sizeof(idx_str), "p%d", i);
        snprintf(expected[i], sizeof(expected[i]), "p%d\n", i);
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "echo", idx_str),
            JSL_SUBPROCESS_ARG_SUCCESS
        );

        JSLAllocatorInterface sb_iface = test_libc_allocator_interface(&sb_backing[i]);
        TEST_BOOL(jsl_string_builder_init(&sbs[i], sb_iface, 32));
        TEST_BOOL(jsl_subprocess_set_stdout_sink(
            &procs[i], jsl_string_builder_output_sink(&sbs[i])
        ));
    }

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        procs[0].allocator, procs, N, 32, 5000, NULL
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_SUCCESS);

    for (int i = 0; i < N; i++)
    {
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_EXITED);
        TEST_INT32_EQUAL(procs[i].exit_code, 0);
        JSLImmutableMemory out = jsl_string_builder_get_string(&sbs[i]);
        int64_t expected_len = (int64_t) strlen(expected[i]);
        TEST_INT64_EQUAL(out.length, expected_len);
        if (out.length == expected_len)
            TEST_BUFFERS_EQUAL(out.data, expected[i], (size_t) expected_len);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_string_builder_free(&sbs[i]);
        jsl_libc_allocator_free_all(&sb_backing[i]);
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

void test_jsl_subprocess_run_blocking_options_zero_timeout_waves(void)
{
    // `timeout_ms == 0` runs procs in waves of `parallelism_count`,
    // pumping each one once and killing any survivors before spawning
    // the next wave. Every proc must reach a terminal state and the
    // call must return TIMEOUT_REACHED. With N=4 sleepers and
    // parallelism=2 the implementation runs two waves; both waves'
    // sleepers should be killed.
    enum { N = 4 };
    JSLLibcAllocator backing[N];
    JSLSubprocess procs[N];

    for (int i = 0; i < N; i++)
    {
        TEST_BOOL(make_helper(&procs[i], &backing[i]));
        TEST_INT32_EQUAL(
            jsl_subprocess_arg_cstr(&procs[i], "sleep", "5000"),
            JSL_SUBPROCESS_ARG_SUCCESS
        );
    }

    JSLSubProcessResultEnum r = jsl_subprocess_run_blocking_options(
        procs[0].allocator, procs, N, 2, 0, NULL
    );
    TEST_INT32_EQUAL(r, JSL_SUBPROCESS_TIMEOUT_REACHED);

    // Every proc must be terminal and must have actually been
    // spawned — none should remain NOT_STARTED, since the wave loop
    // is required to spawn + pump every proc before returning. With
    // sleepers, every wave's procs must be killed.
    for (int i = 0; i < N; i++)
    {
        bool terminal = (procs[i].status == JSL_SUBPROCESS_STATUS_EXITED
            || procs[i].status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL);
        TEST_BOOL(terminal);
        TEST_INT32_EQUAL(procs[i].status, JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL);
    }

    for (int i = 0; i < N; i++)
    {
        jsl_subprocess_cleanup(&procs[i]);
        jsl_libc_allocator_free_all(&backing[i]);
    }
}

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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <wchar.h>

#include "../src/jsl_cmd_line.h"
#include "../src/jsl_core.h"
#include "../src/jsl_allocator.h"
#include "../src/jsl_allocator_arena.h"

#include "minctest.h"

static void test_short_flags_grouping(void)
{
    uint8_t buffer[4096];
    JSLArena arena = JSL_ARENA_FROM_STACK(buffer);
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&arena);

    JSLCmdLineArgs cmd = {0};

    TEST_BOOL(jsl_cmd_line_args_init(&cmd, &allocator));

    char* argv[] = {
        "prog",
        "-a",
        "-bc",
        "-d",
        "--output=result.txt",
    };

    TEST_BOOL(jsl_cmd_line_args_parse(&cmd, 5, argv, NULL));

    TEST_BOOL(jsl_cmd_line_args_has_short_flag(&cmd, 'a'));
    TEST_BOOL(jsl_cmd_line_args_has_short_flag(&cmd, 'b'));
    TEST_BOOL(jsl_cmd_line_args_has_short_flag(&cmd, 'c'));
    TEST_BOOL(jsl_cmd_line_args_has_short_flag(&cmd, 'd'));
    TEST_BOOL(!jsl_cmd_line_args_has_short_flag(&cmd, 'e'));

    JSLFatPtr value = {0};
    TEST_BOOL(jsl_cmd_line_args_pop_flag_with_value(
        &cmd,
        JSL_FATPTR_EXPRESSION("output"),
        &value
    ));
    TEST_BOOL(jsl_fatptr_cstr_compare(value, "result.txt"));
    TEST_BOOL(!jsl_cmd_line_args_pop_flag_with_value(
        &cmd,
        JSL_FATPTR_EXPRESSION("output"),
        &value
    ));
}

static void test_short_flag_equals_is_invalid(void)
{
    uint8_t buffer[4096];
    JSLArena arena = JSL_ARENA_FROM_STACK(buffer);
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&arena);

    JSLCmdLineArgs cmd = {0};

    TEST_BOOL(jsl_cmd_line_args_init(&cmd, &allocator));

    char* argv[] = {
        "prog",
        "-bc=foo",
        "run"
    };

    JSLFatPtr error = {0};
    TEST_BOOL(!jsl_cmd_line_args_parse(&cmd, 3, argv, &error));
    TEST_BOOL(error.data != NULL && error.length > 0);
    TEST_BOOL(jsl_fatptr_index_of(error, '=') >= 0);
}

static void test_long_flags_and_commands(void)
{
    uint8_t buffer[4096];
    JSLArena arena = JSL_ARENA_FROM_STACK(buffer);
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&arena);

    JSLCmdLineArgs cmd = {0};

    TEST_BOOL(jsl_cmd_line_args_init(&cmd, &allocator));

    char* argv[] = {
        "prog",
        "--verbose",
        "--output=result.txt",
        "build",
        "--",
        "--not-a-flag",
    };

    TEST_BOOL(jsl_cmd_line_args_parse(&cmd, 6, argv, NULL));

    TEST_BOOL(jsl_cmd_line_args_has_flag(&cmd, JSL_FATPTR_EXPRESSION("verbose")));
    TEST_BOOL(!jsl_cmd_line_args_has_flag(&cmd, JSL_FATPTR_EXPRESSION("output")));
    TEST_BOOL(jsl_cmd_line_args_has_command(&cmd, JSL_FATPTR_EXPRESSION("build")));
    TEST_BOOL(jsl_cmd_line_args_has_command(&cmd, JSL_FATPTR_EXPRESSION("--not-a-flag")));

    JSLFatPtr value = {0};
    TEST_BOOL(jsl_cmd_line_args_pop_flag_with_value(
        &cmd,
        JSL_FATPTR_EXPRESSION("output"),
        &value
    ));
    TEST_BOOL(jsl_fatptr_cstr_compare(value, "result.txt"));

    JSLFatPtr arg = {0};
    TEST_BOOL(jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
    TEST_BOOL(jsl_fatptr_cstr_compare(arg, "build"));
    TEST_BOOL(jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
    TEST_BOOL(jsl_fatptr_cstr_compare(arg, "--not-a-flag"));
    TEST_BOOL(!jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
}

static bool contains_value(JSLFatPtr* values, int32_t length, const char* needle)
{
    for (int32_t i = 0; i < length; ++i)
    {
        if (jsl_fatptr_cstr_compare(values[i], (char*) needle))
            return true;
    }
    return false;
}

static void test_long_values_equals_and_space(void)
{
    uint8_t buffer[4096];
    JSLArena arena = JSL_ARENA_FROM_STACK(buffer);
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&arena);

    JSLCmdLineArgs cmd = {0};

    TEST_BOOL(jsl_cmd_line_args_init(&cmd, &allocator));

    char* argv[] = {
        "prog",
        "--ignore=foo",
        "--ignore", "bar",
        "--ignore=baz",
        "run",
        "clean",
    };

    TEST_BOOL(jsl_cmd_line_args_parse(&cmd, 7, argv, NULL));

    JSLFatPtr collected[3] = {0};
    int32_t collected_count = 0;
    JSLFatPtr value = {0};
    while (jsl_cmd_line_args_pop_flag_with_value(
        &cmd,
        JSL_FATPTR_EXPRESSION("ignore"),
        &value
    ))
    {
        collected[collected_count++] = value;
    }

    TEST_INT32_EQUAL(collected_count, 3);
    TEST_BOOL(contains_value(collected, collected_count, "foo"));
    TEST_BOOL(contains_value(collected, collected_count, "bar"));
    TEST_BOOL(contains_value(collected, collected_count, "baz"));

    TEST_BOOL(!jsl_cmd_line_args_has_command(&cmd, JSL_FATPTR_EXPRESSION("bar")));

    JSLFatPtr arg = {0};
    TEST_BOOL(jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
    TEST_BOOL(jsl_fatptr_cstr_compare(arg, "run"));
    TEST_BOOL(jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
    TEST_BOOL(jsl_fatptr_cstr_compare(arg, "clean"));
    TEST_BOOL(!jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
}

static void test_wide_parsing(void)
{
    uint8_t buffer[4096];
    JSLArena arena = JSL_ARENA_FROM_STACK(buffer);
    JSLAllocatorInterface allocator = jsl_arena_get_allocator_interface(&arena);

    JSLCmdLineArgs cmd = {0};

    TEST_BOOL(jsl_cmd_line_args_init(&cmd, &allocator));

    wchar_t arg0[] = L"prog";
    wchar_t arg1[] = L"--name";
    wchar_t arg2[] = L"alice";
    wchar_t arg3[] = L"deploy";

    wchar_t* argv[] = {
        arg0,
        arg1,
        arg2,
        arg3
    };

    TEST_BOOL(jsl_cmd_line_args_parse_wide(&cmd, 4, argv, NULL));

    JSLFatPtr value = {0};
    TEST_BOOL(jsl_cmd_line_args_pop_flag_with_value(
        &cmd,
        JSL_FATPTR_EXPRESSION("name"),
        &value
    ));
    TEST_BOOL(jsl_fatptr_cstr_compare(value, "alice"));

    TEST_BOOL(jsl_cmd_line_args_has_command(&cmd, JSL_FATPTR_EXPRESSION("deploy")));
    TEST_BOOL(!jsl_cmd_line_args_has_command(&cmd, JSL_FATPTR_EXPRESSION("alice")));
}

int main(void)
{
    RUN_TEST_FUNCTION("Short flags grouping", test_short_flags_grouping);
    RUN_TEST_FUNCTION("Short flag with equals fails", test_short_flag_equals_is_invalid);
    RUN_TEST_FUNCTION("Long flags, commands, and terminator", test_long_flags_and_commands);
    RUN_TEST_FUNCTION("Long flag values via equals and space", test_long_values_equals_and_space);
    RUN_TEST_FUNCTION("Wide argument parsing", test_wide_parsing);

    TEST_RESULTS();
    return lfails != 0;
}

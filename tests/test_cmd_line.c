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

#define EXPECT_SINK_OUTPUT(expected_literal, bytes_written, buffer, writer) \
    do { \
        int64_t expected_len = (int64_t)(sizeof(expected_literal) - 1); \
        int64_t actual_len = jsl_total_write_length((buffer), (writer)); \
        TEST_INT64_EQUAL((bytes_written), expected_len); \
        TEST_INT64_EQUAL(actual_len, expected_len); \
        TEST_BUFFERS_EQUAL((buffer).data, (expected_literal), expected_len); \
    } while (0)

static void test_short_flags_grouping(void)
{
    uint8_t buffer[4096];
    JSLArena arena = JSL_ARENA_FROM_STACK(buffer);
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &arena);

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

    JSLImmutableMemory value = {0};
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
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &arena);

    JSLCmdLineArgs cmd = {0};

    TEST_BOOL(jsl_cmd_line_args_init(&cmd, &allocator));

    char* argv[] = {
        "prog",
        "-bc=foo",
        "run"
    };

    JSLImmutableMemory error = {0};
    TEST_BOOL(!jsl_cmd_line_args_parse(&cmd, 3, argv, &error));
    TEST_BOOL(error.data != NULL && error.length > 0);
    TEST_BOOL(jsl_fatptr_index_of(error, '=') >= 0);
}

static void test_long_flags_and_commands(void)
{
    uint8_t buffer[4096];
    JSLArena arena = JSL_ARENA_FROM_STACK(buffer);
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &arena);

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

    JSLImmutableMemory value = {0};
    TEST_BOOL(jsl_cmd_line_args_pop_flag_with_value(
        &cmd,
        JSL_FATPTR_EXPRESSION("output"),
        &value
    ));
    TEST_BOOL(jsl_fatptr_cstr_compare(value, "result.txt"));

    JSLImmutableMemory arg = {0};
    TEST_BOOL(jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
    TEST_BOOL(jsl_fatptr_cstr_compare(arg, "build"));
    TEST_BOOL(jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
    TEST_BOOL(jsl_fatptr_cstr_compare(arg, "--not-a-flag"));
    TEST_BOOL(!jsl_cmd_line_args_pop_arg_list(&cmd, &arg));
}

static bool contains_value(JSLImmutableMemory* values, int32_t length, const char* needle)
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
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &arena);

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

    JSLImmutableMemory collected[3] = {0};
    int32_t collected_count = 0;
    JSLImmutableMemory value = {0};
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

    JSLImmutableMemory arg = {0};
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
    JSLAllocatorInterface allocator;
    jsl_arena_get_allocator_interface(&allocator, &arena);

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

    JSLImmutableMemory value = {0};
    TEST_BOOL(jsl_cmd_line_args_pop_flag_with_value(
        &cmd,
        JSL_FATPTR_EXPRESSION("name"),
        &value
    ));
    TEST_BOOL(jsl_fatptr_cstr_compare(value, "alice"));

    TEST_BOOL(jsl_cmd_line_args_has_command(&cmd, JSL_FATPTR_EXPRESSION("deploy")));
    TEST_BOOL(!jsl_cmd_line_args_has_command(&cmd, JSL_FATPTR_EXPRESSION("alice")));
}

static void test_cmd_line_color_conversions(void)
{
    TEST_UINT32_EQUAL(jsl_cmd_line_rgb_to_ansi16(0, 0, 0), 0);
    TEST_UINT32_EQUAL(jsl_cmd_line_rgb_to_ansi16(255, 0, 0), 9);
    TEST_UINT32_EQUAL(jsl_cmd_line_rgb_to_ansi16(0, 255, 0), 10);
    TEST_UINT32_EQUAL(jsl_cmd_line_rgb_to_ansi16(255, 255, 255), 15);
    TEST_UINT32_EQUAL(jsl_cmd_line_rgb_to_ansi16(0, 0, 255), 4);

    TEST_UINT32_EQUAL(jsl_cmd_line_rgb_to_ansi256(255, 0, 0), 9);
    TEST_UINT32_EQUAL(jsl_cmd_line_rgb_to_ansi256(95, 135, 175), 67);
    TEST_UINT32_EQUAL(jsl_cmd_line_rgb_to_ansi256(58, 58, 58), 237);

    TEST_UINT32_EQUAL(jsl_cmd_line_ansi256_to_ansi16(0), 0);
    TEST_UINT32_EQUAL(jsl_cmd_line_ansi256_to_ansi16(15), 15);
    TEST_UINT32_EQUAL(jsl_cmd_line_ansi256_to_ansi16(16), 0);
    TEST_UINT32_EQUAL(jsl_cmd_line_ansi256_to_ansi16(196), 9);
}

static void test_cmd_line_write_style_no_color(void)
{
    JSLTerminalInfo info = {0};
    TEST_BOOL(jsl_cmd_line_get_terminal_info(&info, JSL_GET_TERMINAL_INFO_FORCE_NO_COLOR));

    JSLCmdLineColor fg = {0};
    jsl_cmd_line_color_from_ansi16(&fg, 1);

    JSLCmdLineStyle style = {0};
    jsl_cmd_line_style_with_foreground(&style, fg, JSL_CMD_LINE_STYLE_BOLD);

    uint8_t raw[64] = {0};
    JSLImmutableMemory buffer = JSL_MEMORY_FROM_STACK(raw);
    JSLImmutableMemory writer = buffer;
    JSLOutputSink sink = jsl_fatptr_output_sink(&writer);

    int64_t result = jsl_cmd_line_write_style(sink, &info, &style);
    EXPECT_SINK_OUTPUT("", result, buffer, writer);

    JSLImmutableMemory reset_writer = buffer;
    JSLOutputSink reset_sink = jsl_fatptr_output_sink(&reset_writer);
    result = jsl_cmd_line_write_reset(reset_sink, &info);
    EXPECT_SINK_OUTPUT("", result, buffer, reset_writer);
}

static void test_cmd_line_write_style_ansi16(void)
{
    JSLTerminalInfo info = {0};
    TEST_BOOL(jsl_cmd_line_get_terminal_info(&info, JSL_GET_TERMINAL_INFO_FORCE_16_COLOR_MODE));

    JSLCmdLineColor fg = {0};
    JSLCmdLineColor bg = {0};
    jsl_cmd_line_color_from_ansi16(&fg, 1);
    jsl_cmd_line_color_from_ansi16(&bg, 12);

    JSLCmdLineStyle style = {0};
    jsl_cmd_line_style_with_foreground_and_background(
        &style,
        fg,
        bg,
        JSL_CMD_LINE_STYLE_BOLD | JSL_CMD_LINE_STYLE_UNDERLINE | JSL_CMD_LINE_STYLE_STRIKE
    );

    uint8_t raw[128] = {0};
    JSLImmutableMemory buffer = JSL_MEMORY_FROM_STACK(raw);
    JSLImmutableMemory writer = buffer;
    JSLOutputSink sink = jsl_fatptr_output_sink(&writer);

    int64_t result = jsl_cmd_line_write_style(sink, &info, &style);
    EXPECT_SINK_OUTPUT("\x1b[1m\x1b[4m\x1b[9m\x1b[31m\x1b[104m", result, buffer, writer);
}

static void test_cmd_line_write_style_ansi16_converts_color_types(void)
{
    JSLTerminalInfo info = {0};
    TEST_BOOL(jsl_cmd_line_get_terminal_info(&info, JSL_GET_TERMINAL_INFO_FORCE_16_COLOR_MODE));

    JSLCmdLineColor fg = {0};
    JSLCmdLineColor bg = {0};
    jsl_cmd_line_color_from_rgb(&fg, 0, 255, 0);
    jsl_cmd_line_color_from_ansi256(&bg, 196);

    JSLCmdLineStyle style = {0};
    jsl_cmd_line_style_with_foreground_and_background(&style, fg, bg, 0);

    uint8_t raw[128] = {0};
    JSLImmutableMemory buffer = JSL_MEMORY_FROM_STACK(raw);
    JSLImmutableMemory writer = buffer;
    JSLOutputSink sink = jsl_fatptr_output_sink(&writer);

    int64_t result = jsl_cmd_line_write_style(sink, &info, &style);
    EXPECT_SINK_OUTPUT("\x1b[92m\x1b[101m", result, buffer, writer);
}

static void test_cmd_line_write_style_ansi256(void)
{
    JSLTerminalInfo info = {0};
    TEST_BOOL(jsl_cmd_line_get_terminal_info(&info, JSL_GET_TERMINAL_INFO_FORCE_255_COLOR_MODE));

    JSLCmdLineColor fg = {0};
    JSLCmdLineColor bg = {0};
    jsl_cmd_line_color_from_rgb(&fg, 95, 135, 175);
    jsl_cmd_line_color_from_ansi16(&bg, 3);

    JSLCmdLineStyle style = {0};
    jsl_cmd_line_style_with_foreground_and_background(&style, fg, bg, JSL_CMD_LINE_STYLE_DIM);

    uint8_t raw[128] = {0};
    JSLImmutableMemory buffer = JSL_MEMORY_FROM_STACK(raw);
    JSLImmutableMemory writer = buffer;
    JSLOutputSink sink = jsl_fatptr_output_sink(&writer);

    int64_t result = jsl_cmd_line_write_style(sink, &info, &style);
    EXPECT_SINK_OUTPUT("\x1b[2m\x1b[38;5;67m\x1b[43m", result, buffer, writer);
}

static void test_cmd_line_write_style_truecolor(void)
{
    JSLTerminalInfo info = {0};
    TEST_BOOL(jsl_cmd_line_get_terminal_info(&info, JSL_GET_TERMINAL_INFO_FORCE_24_BIT_COLOR_MODE));

    JSLCmdLineColor fg = {0};
    JSLCmdLineColor bg = {0};
    jsl_cmd_line_color_from_rgb(&fg, 12, 34, 56);
    jsl_cmd_line_color_from_ansi256(&bg, 200);

    JSLCmdLineStyle style = {0};
    jsl_cmd_line_style_with_foreground_and_background(
        &style,
        fg,
        bg,
        JSL_CMD_LINE_STYLE_ITALIC | JSL_CMD_LINE_STYLE_INVERSE
    );

    uint8_t raw[128] = {0};
    JSLImmutableMemory buffer = JSL_MEMORY_FROM_STACK(raw);
    JSLImmutableMemory writer = buffer;
    JSLOutputSink sink = jsl_fatptr_output_sink(&writer);

    int64_t result = jsl_cmd_line_write_style(sink, &info, &style);
    EXPECT_SINK_OUTPUT("\x1b[3m\x1b[7m\x1b[38;2;12;34;56m\x1b[48;5;200m", result, buffer, writer);
}

static void test_cmd_line_write_style_and_reset_invalid(void)
{
    JSLTerminalInfo info = {0};
    bool ok = jsl_cmd_line_get_terminal_info(
        &info,
        JSL_GET_TERMINAL_INFO_FORCE_16_COLOR_MODE | JSL_GET_TERMINAL_INFO_FORCE_255_COLOR_MODE
    );
    TEST_BOOL(!ok);

    JSLCmdLineStyle style = {0};

    uint8_t raw[32] = {0};
    JSLImmutableMemory buffer = JSL_MEMORY_FROM_STACK(raw);
    JSLImmutableMemory writer = buffer;
    JSLOutputSink sink = jsl_fatptr_output_sink(&writer);

    TEST_INT64_EQUAL(jsl_cmd_line_write_style(sink, &info, &style), -1);
    EXPECT_SINK_OUTPUT("", 0, buffer, writer);

    JSLImmutableMemory reset_writer = buffer;
    JSLOutputSink reset_sink = jsl_fatptr_output_sink(&reset_writer);
    TEST_INT64_EQUAL(jsl_cmd_line_write_reset(reset_sink, &info), -1);
    EXPECT_SINK_OUTPUT("", 0, buffer, reset_writer);
}

static void test_cmd_line_write_reset_ansi_modes(void)
{
    JSLTerminalInfo info = {0};
    TEST_BOOL(jsl_cmd_line_get_terminal_info(&info, JSL_GET_TERMINAL_INFO_FORCE_16_COLOR_MODE));

    uint8_t raw[16] = {0};
    JSLImmutableMemory buffer = JSL_MEMORY_FROM_STACK(raw);
    JSLImmutableMemory writer = buffer;
    JSLOutputSink sink = jsl_fatptr_output_sink(&writer);

    int64_t result = jsl_cmd_line_write_reset(sink, &info);
    EXPECT_SINK_OUTPUT("\x1b[0m", result, buffer, writer);
}

int main(void)
{
    RUN_TEST_FUNCTION("Test command line arg short flags grouping", test_short_flags_grouping);
    RUN_TEST_FUNCTION("Test command line arg short flag with equals fails", test_short_flag_equals_is_invalid);
    RUN_TEST_FUNCTION("Test command line arg long flags, commands, and terminator", test_long_flags_and_commands);
    RUN_TEST_FUNCTION("Test command line arg long flag values via equals and space", test_long_values_equals_and_space);
    RUN_TEST_FUNCTION("Test command line arg wide argument parsing", test_wide_parsing);
    RUN_TEST_FUNCTION("Test command line color conversions", test_cmd_line_color_conversions);
    RUN_TEST_FUNCTION("Test command line style writes no color", test_cmd_line_write_style_no_color);
    RUN_TEST_FUNCTION("Test command line style write ANSI16", test_cmd_line_write_style_ansi16);
    RUN_TEST_FUNCTION("Test command line style converts to ANSI16", test_cmd_line_write_style_ansi16_converts_color_types);
    RUN_TEST_FUNCTION("Test command line style write ANSI256", test_cmd_line_write_style_ansi256);
    RUN_TEST_FUNCTION("Test command line style write truecolor", test_cmd_line_write_style_truecolor);
    RUN_TEST_FUNCTION("Test command line style/reset invalid", test_cmd_line_write_style_and_reset_invalid);
    RUN_TEST_FUNCTION("Test command line reset ANSI modes", test_cmd_line_write_reset_ansi_modes);

    TEST_RESULTS();
    return lfails != 0;
}

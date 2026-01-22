/**
 * TODO: docs
 * 
 * ## License
 *
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

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"
#include "jsl_str_set.h"
#include "jsl_str_to_str_map.h"
#include "jsl_str_to_str_multimap.h"

#define JSL__CMD_LINE_SHORT_FLAG_BUCKETS 4
#define JSL__CMD_LINE_UTF8_CONT_MASK 0xC0u
#define JSL__CMD_LINE_UTF8_CONT_VALUE 0x80u

#ifdef __cplusplus
extern "C" {
#endif

struct JSL__CmdLineArgs {
    JSLAllocatorInterface* allocator;

    uint64_t short_flag_bitset[JSL__CMD_LINE_SHORT_FLAG_BUCKETS];

    JSLStrToStrMap long_flags;
    JSLStrToStrMultimap flags_with_values;
    JSLStrSet commands;

    JSLFatPtr* arg_list;
    int64_t arg_list_length;
    int64_t arg_list_index;
    int64_t arg_list_capacity;
};

/**
 * State container struct 
 */
typedef struct JSL__CmdLineArgs JSLCmdLineArgs;

/**
 * Initialize an instance of the command line parser container.
 */
bool jsl_cmd_line_init(JSLCmdLineArgs* cmd_line, JSLAllocatorInterface* allocator);

/**
 * Parse the given command line arguments that are in the POSIX style.
 * 
 * The inputs must represent valid UTF-8. 
 *
 * This functions parses and stores the flags, arguments, and commands
 * from the user input for easy querying with the other functions in this
 * module. If `out_error` is not NULL it will be set to a
 * user-friendly message on failure.
 * 
 * @returns false if the arena is out of memory or the passed in strings
 * were not valid utf-8.
 */
bool jsl_cmd_line_parse(
    JSLCmdLineArgs* cmd_line,
    int32_t argc,
    char** argv,
    JSLFatPtr* out_error
);

/**
 * Parse the given command line arguments that are in the POSIX style.
 * 
 * The inputs must represent valid UTF-16. 
 *
 * This functions parses and stores the flags, arguments, and commands
 * from the user input for easy querying with the other functions in this
 * module. If `out_error` is not NULL it will be set to a
 * user-friendly message on failure.
 * 
 * @returns false if the arena is out of memory or the passed in strings
 * were not valid utf-16.
 */
bool jsl_cmd_line_parse_wide(
    JSLCmdLineArgs* cmd_line,
    int32_t argc,
    wchar_t** argv,
    JSLFatPtr* out_error
);

/**
 * Checks if the user passed in the given short flag.
 * 
 * Example:
 * 
 * ```
 * ./command -f -v
 * ```
 * 
 * ```
 * jsl_cmd_line_has_short_flag(cmd, 'f'); // true
 * jsl_cmd_line_has_short_flag(cmd, 'v'); // true
 * jsl_cmd_line_has_short_flag(cmd, 'g'); // false
 * ```
 */
bool jsl_cmd_line_has_short_flag(JSLCmdLineArgs* cmd_line, uint8_t flag);

/**
 * Checks if the user passed in the given flag with no value.
 * 
 * Example:
 * 
 * ```
 * ./command --long-flag --arg=my_value --arg2
 * ```
 * 
 * ```
 * jsl_cmd_line_has_flag(cmd, JSL_FATPTR_EXPRESSION("long-flag")); // true
 * jsl_cmd_line_has_flag(cmd, JSL_FATPTR_EXPRESSION("arg")); // false
 * jsl_cmd_line_has_flag(cmd, JSL_FATPTR_EXPRESSION("arg2")); // true
 * ```
 */
bool jsl_cmd_line_has_flag(JSLCmdLineArgs* cmd_line, JSLFatPtr flag);

/**
 * Checks if the user passed in the given command.
 * 
 * Example:
 * 
 * ```
 * ./command -v clean build 
 * ```
 * 
 * ```
 * jsl_cmd_line_has_command(cmd, JSL_FATPTR_EXPRESSION("clean")); // true
 * jsl_cmd_line_has_command(cmd, JSL_FATPTR_EXPRESSION("build")); // true
 * jsl_cmd_line_has_command(cmd, JSL_FATPTR_EXPRESSION("restart")); // false
 * ```
 */
bool jsl_cmd_line_has_command(JSLCmdLineArgs* cmd_line, JSLFatPtr flag);

/**
 * If the user passed in a argument list, pop one off returning true if
 * a value was successfully gotten.
 * 
 * Example:
 * 
 * ```
 * ./command file1 file2 file3 
 * ```
 * 
 * ```
 * JSLFatPtr arg;
 * 
 * jsl_cmd_line_pop_arg_list(cmd, &arg); // arg == file1
 * jsl_cmd_line_pop_arg_list(cmd, &arg); // arg == file2
 * jsl_cmd_line_pop_arg_list(cmd, &arg); // arg == file3
 * jsl_cmd_line_pop_arg_list(cmd, &arg); // returns false
 * ```
 * 
 * @warning This function assumes that there are no commands that can be returned
 * with `jsl_cmd_line_has_command`. There's no syntactic distinction between a single
 * command and a argument list.
 */
bool jsl_cmd_line_pop_arg_list(JSLCmdLineArgs* cmd_line, JSLFatPtr* out_value);

/**
 * If the user passed in multiple instances of flag with a value, this function
 * will grab one for each call. If there are no more flag instances this function
 * returns false.
 * 
 * Example:
 * 
 * ```
 * ./command --ignore=foo --ignore=bar
 * ```
 * 
 * ```
 * JSLFatPtr arg;
 * 
 * jsl_cmd_line_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // arg == foo
 * jsl_cmd_line_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // arg == bar
 * jsl_cmd_line_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // returns false
 * ```
 * 
 * @warning Argument ordering is not guaranteed
 */
bool jsl_cmd_line_pop_flag_with_value(
    JSLCmdLineArgs* cmd_line,
    JSLFatPtr flag,
    JSLFatPtr* out_value
);

#ifdef __cplusplus
} /* extern "C" */
#endif

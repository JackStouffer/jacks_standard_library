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
#include <stdalign.h>

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

// Private implementation, not guaranteed to be stable!
struct JSL__TerminalInfo {
    int32_t output_mode;
};

/**
 * TODO: docs
 */
typedef struct JSL__TerminalInfo JSLTerminalInfo;

// TODO: docs
enum JSL__CmdLineColorType
{
    /// @brief TODO: docs
    JSL_CMD_LINE_COLOR_DEFAULT = 0,
    /// @brief TODO: docs
    JSL_CMD_LINE_COLOR_ANSI16,      // 0..15
    /// @brief TODO: docs
    JSL_CMD_LINE_COLOR_ANSI256,     // 0..255
    /// @brief TODO: docs
    JSL_CMD_LINE_COLOR_RGB          // 24-bit
};

/**
 * TODO: docs
 */
typedef struct JSLCmdLineColor
{
    alignas(alignof(JSL_MAX_ALIGN_T)) uint8_t _buf[8];
} JSLCmdLineColor;

/**
 * TODO: docs
 * 
 * flags to be set on JSLCmdLineStyle
 */
typedef enum JSLCmdLineStyleAttribute {
    /// @brief TODO: docs
    JSL_CMD_LINE_STYLE_BOLD = JSL_MAKE_BITFLAG(0),
    /// @brief TODO: docs
    JSL_CMD_LINE_STYLE_DIM = JSL_MAKE_BITFLAG(1),
    /// @brief TODO: docs
    JSL_CMD_LINE_STYLE_ITALIC = JSL_MAKE_BITFLAG(2),
    /// @brief TODO: docs
    JSL_CMD_LINE_STYLE_UNDERLINE = JSL_MAKE_BITFLAG(3),
    /// @brief double underline (if supported)
    JSL_CMD_LINE_STYLE_DUNDERLINE = JSL_MAKE_BITFLAG(4),
    /// @brief TODO: docs
    JSL_CMD_LINE_STYLE_BLINK = JSL_MAKE_BITFLAG(5),
    /// @brief rapid blink (rare)
    JSL_CMD_LINE_STYLE_RBLINK = JSL_MAKE_BITFLAG(6),
    /// @brief TODO: docs
    JSL_CMD_LINE_STYLE_INVERSE = JSL_MAKE_BITFLAG(7),
    /// @brief TODO: docs
    JSL_CMD_LINE_STYLE_HIDDEN = JSL_MAKE_BITFLAG(8),
    /// @brief TODO: docs
    JSL_CMD_LINE_STYLE_STRIKE = JSL_MAKE_BITFLAG(9)
} JSLCmdLineStyleAttribute;

typedef struct JSLCmdLineStyle {
    alignas(alignof(JSL_MAX_ALIGN_T)) uint8_t _buf[20];
} JSLCmdLineStyle;

/**
 * TODO: docs
 * 
 * flags to be used with jsl_cmd_line_get_terminal_info
 */
enum JSLGetTerminalInfoFlags {
    JSL_GET_TERMINAL_INFO_FORCE_NO_COLOR = JSL_MAKE_BITFLAG(0),
    JSL_GET_TERMINAL_INFO_FORCE_16_COLOR_MODE = JSL_MAKE_BITFLAG(1),
    JSL_GET_TERMINAL_INFO_FORCE_255_COLOR_MODE = JSL_MAKE_BITFLAG(2),
    JSL_GET_TERMINAL_INFO_FORCE_24_BIT_COLOR_MODE = JSL_MAKE_BITFLAG(3)
};

/**
 * Gather information about this terminal for the styling functions. 
 * 
 * The flags value is a bitwise OR of the values of `enum JSLGetTerminalInfoFlags`.
 * Some of these flags are mutually exclusive.
 * 
 * With a flags value of zero, this function will autodetect the terminals
 * color display capabilities.
 * 
 * @param info The info struct to fill out
 * @param flags Bitflags to control the function's behavior
 * @returns A bool indicating the success or failure of `info`'s initialization
 */
bool jsl_cmd_line_get_terminal_info(JSLTerminalInfo* info, uint32_t flags);

/**
 * Converts the RGB color to its closest color in the ANSI 16 color space.
 * 
 * This function is used in the background when attempting to write a 
 * JSLCmdLineStyle with a RGB color to a terminal that only supports 16
 * color mode. 
 *
 * This function assumes an XTERM like color pallet for ANSI colors, as
 * such colors are user configurable.
 *
 * This function does not use a lookup table, as such a table would add many
 * tens of kilobytes to the executable for a very rare case (ANSI 16 only terminals).
 * As such, this function is constant time, but it's not cheap.
 *
 * @param r The red color channel
 * @param g The green color channel
 * @param b The blue color channel
 * @returns The ANSI 16 color code
 */
uint8_t jsl_cmd_line_rgb_to_ansi16(uint8_t r, uint8_t g, uint8_t b);

/**
 * Converts the RGB color to its closest color in the ANSI 256 color space.
 * 
 * This function is used in the background when attempting to write a 
 * JSLCmdLineStyle with a RGB color to a terminal that only supports 256
 * color mode. 
 * 
 * This function assumes an XTERM like color pallet for ANSI colors, as
 * such colors are user configurable.
 * 
 * This function does not use a lookup table, as such a table would add many
 * tens of kilobytes to the executable for a very rare case (ANSI 256 only terminals).
 * As such, this function is constant time, but it's not cheap.
 *
 * @param r The red color channel
 * @param g The green color channel
 * @param b The blue color channel
 * @returns The ANSI 256 color code
 */
uint8_t jsl_cmd_line_rgb_to_ansi256(uint8_t r, uint8_t g, uint8_t b);

/**
 * Converts the ANSI 256 color to its closest color in the ANSI
 * 16 color space.
 *
 * This function is used in the background when attempting to write a 
 * JSLCmdLineStyle with a ANSI 256 color to a terminal that only supports 16
 * color mode.
 * 
 * This function assumes an XTERM like color pallet for ANSI colors, as
 * such colors are user configurable.
 * 
 * This function does use a constant look up table and is very fast.
 *
 * @param color256 The ANSI 256 color code
 * @returns The ANSI 16 color code
 */
uint8_t jsl_cmd_line_ansi256_to_ansi16(uint8_t color256);

/**
 * TODO: docs
 */
void jsl_cmd_line_color_from_ansi16(JSLCmdLineColor* color, uint8_t color16);

/**
 * TODO: docs
 */
void jsl_cmd_line_color_from_ansi256(JSLCmdLineColor* color, uint8_t color256);

/**
 * TODO: docs
 */
void jsl_cmd_line_color_from_rgb(JSLCmdLineColor* color, uint8_t r, uint8_t g, uint8_t b);

/**
 * TODO: docs
 * 
 * The value of `flags` is set from OR-ing any of the flags in `JSLCmdLineStyleAttribute`
 */
void jsl_cmd_line_style_with_foreground(
    JSLCmdLineStyle* style,
    JSLCmdLineColor foreground,
    uint32_t style_flags
);

/**
 * TODO: docs
 * 
 * The value of `flags` is set from OR-ing any of the flags in `JSLCmdLineStyleAttribute`
 */
void jsl_cmd_line_style_with_background(
    JSLCmdLineStyle* style,
    JSLCmdLineColor background,
    uint32_t style_flags
);

/**
 * TODO: docs
 * 
 * The value of `flags` is set from OR-ing any of the flags in `JSLCmdLineStyleAttribute`
 */
void jsl_cmd_line_style_with_foreground_and_background(
    JSLCmdLineStyle* style,
    JSLCmdLineColor foreground,
    JSLCmdLineColor background,
    uint32_t style_flags
);

/** 
 * Send the ANSI escape codes to generate the given style to the output sink.
 * This does NOT automatically send a reset code. All ANSI codes sent by this
 * function are additive.
 * 
 * Example:
 * 
 * ```
 * // This is a full example of writing colored text to stdout
 * 
 * JSLOutputSink sink = jsl_c_file_output_sink(stdout);
 * 
 * JSLTerminalInfo terminal_info;
 * jsl_cmd_line_get_terminal_info(&terminal_info, 0);
 * 
 * JSLCmdLineColor red_color;
 * jsl_cmd_line_color_from_rgb(&red_color, 255, 0, 0);
 * 
 * JSLCmdLineStyle bold_red;
 * jsl_cmd_line_style_with_foreground(&bold_red, &red_color, JSL_CMD_LINE_STYLE_BOLD);
 *
 * // You write the style first, then you write your text 
 * jsl_cmd_line_reset_style(sink);
 * jsl_cmd_line_write_style(sink, &terminal_info, &bold_red);
 *
 * // this will be bold red
 * jsl_output_sink_write_cstr(sink, "ERROR: ");
 * 
 * // You have to reset, or else each style write is additive
 * jsl_cmd_line_reset_style(sink);
 *
 * // this will be default
 * jsl_output_sink_write_cstr(sink, "There's an error in the program!");
 * ```
 * 
 * @param sink Where to send the ANSI escape codes
 * @param terminal_info Information about the current terminal environment so this knows what codes to send
 * @param style The style to write
 * @returns The number of bytes written to the sink, or -1 if there was an error in the sink
 */
int64_t jsl_cmd_line_write_style(JSLOutputSink sink, JSLTerminalInfo* terminal_info, JSLCmdLineStyle* style);

/**
 * Reset all styling by sending the reset ANSI escape code to the given output sink
 * 
 * Example:
 * 
 * ```
 * // This is a full example of writing colored text to stdout
 * 
 * JSLOutputSink sink = jsl_c_file_output_sink(stdout);
 * 
 * JSLTerminalInfo terminal_info;
 * jsl_cmd_line_get_terminal_info(&terminal_info, 0);
 * 
 * JSLCmdLineColor red_color;
 * jsl_cmd_line_color_from_rgb(&red_color, 255, 0, 0);
 * 
 * JSLCmdLineStyle bold_red;
 * jsl_cmd_line_style_with_foreground(&bold_red, &red_color, JSL_CMD_LINE_STYLE_BOLD);
 *
 * // You write the style first, then you write your text 
 * jsl_cmd_line_reset_style(sink);
 * jsl_cmd_line_write_style(sink, &terminal_info, &bold_red);
 *
 * // this will be bold red
 * jsl_output_sink_write_cstr(sink, "ERROR: ");
 * 
 * // You have to reset, or else each style write is additive
 * jsl_cmd_line_reset_style(sink);
 *
 * // this will be default
 * jsl_output_sink_write_cstr(sink, "There's an error in the program!");
 * ```
 * 
 * @param sink Where to send the ANSI reset escape code
 * @param terminal_info Information about the current terminal environment so this knows what codes to send
 * @returns The number of bytes written to the sink, or -1 if there was an error in the sink
 */
int64_t jsl_cmd_line_write_reset(JSLOutputSink sink, JSLTerminalInfo* terminal_info);

/**
 * TODO: docs
 *
 * State container struct 
 */
typedef struct JSLCmdLineArgs {
    alignas(alignof(JSL_MAX_ALIGN_T)) uint8_t _buf[312];
} JSLCmdLineArgs;

/**
 * Initialize an instance of the command line parser container.
 */
bool jsl_cmd_line_args_init(JSLCmdLineArgs* args, JSLAllocatorInterface* allocator);

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
 * TODO: docs
 * @returns false if the arena is out of memory or the passed in strings
 * were not valid utf-8.
 */
bool jsl_cmd_line_args_parse(
    JSLCmdLineArgs* args,
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
 * TODO: docs
 * @returns false if the arena is out of memory or the passed in strings
 * were not valid utf-16.
 */
bool jsl_cmd_line_args_parse_wide(
    JSLCmdLineArgs* args,
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
 * jsl_cmd_line_args_has_short_flag(cmd, 'f'); // true
 * jsl_cmd_line_args_has_short_flag(cmd, 'v'); // true
 * jsl_cmd_line_args_has_short_flag(cmd, 'g'); // false
 * ```
 * TODO: docs
 */
bool jsl_cmd_line_args_has_short_flag(JSLCmdLineArgs* args, uint8_t flag);

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
 * jsl_cmd_line_args_has_flag(cmd, JSL_FATPTR_EXPRESSION("long-flag")); // true
 * jsl_cmd_line_args_has_flag(cmd, JSL_FATPTR_EXPRESSION("arg")); // false
 * jsl_cmd_line_args_has_flag(cmd, JSL_FATPTR_EXPRESSION("arg2")); // true
 * ```
 * TODO: docs
 */
bool jsl_cmd_line_args_has_flag(JSLCmdLineArgs* args, JSLFatPtr flag);

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
 * jsl_cmd_line_args_has_command(cmd, JSL_FATPTR_EXPRESSION("clean")); // true
 * jsl_cmd_line_args_has_command(cmd, JSL_FATPTR_EXPRESSION("build")); // true
 * jsl_cmd_line_args_has_command(cmd, JSL_FATPTR_EXPRESSION("restart")); // false
 * ```
 * TODO: docs
 */
bool jsl_cmd_line_args_has_command(JSLCmdLineArgs* args, JSLFatPtr flag);

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
 * jsl_cmd_line_args_pop_arg_list(cmd, &arg); // arg == file1
 * jsl_cmd_line_args_pop_arg_list(cmd, &arg); // arg == file2
 * jsl_cmd_line_args_pop_arg_list(cmd, &arg); // arg == file3
 * jsl_cmd_line_args_pop_arg_list(cmd, &arg); // returns false
 * ```
 * TODO: docs
 * 
 * @warning This function assumes that there are no commands that can be returned
 * with `jsl_cmd_line_args_has_command`. There's no syntactic distinction between a single
 * command and a argument list.
 */
bool jsl_cmd_line_args_pop_arg_list(JSLCmdLineArgs* args, JSLFatPtr* out_value);

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
 * jsl_cmd_line_args_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // arg == foo
 * jsl_cmd_line_args_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // arg == bar
 * jsl_cmd_line_args_pop_flag_with_value(cmd, JSL_FATPTR_EXPRESSION("ignore"), &arg); // returns false
 * ```
 * TODO: docs
 * 
 * @warning Argument ordering is not guaranteed
 */
bool jsl_cmd_line_args_pop_flag_with_value(
    JSLCmdLineArgs* args,
    JSLFatPtr flag,
    JSLFatPtr* out_value
);

#ifdef __cplusplus
} /* extern "C" */
#endif

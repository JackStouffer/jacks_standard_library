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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"
#include "jsl_allocator.h"
#include "jsl_cmd_line.h"
#include "jsl_str_set.h"
#include "jsl_str_to_str_map.h"
#include "jsl_str_to_str_multimap.h"

#if JSL_IS_WINDOWS
    #include <windows.h>
    #include <io.h>
#elif JSL_IS_POSIX
    #include <unistd.h>
#endif

#define JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL 10777217890826255191U

#if WCHAR_MAX <= 0xFFFFu
    #define JSL__CMD_LINE_WCHAR_IS_16_BIT 1
#elif WCHAR_MAX <= 0xFFFFFFFFu
    #define JSL__CMD_LINE_WCHAR_IS_32_BIT 1
#else
    #error "Unsupported wchar_t size"
#endif

#define JSL__CMD_LINE_OUTPUT_MODE_INVALID_STATE 0
#define JSL__CMD_LINE_OUTPUT_MODE_NONE 1
#define JSL__CMD_LINE_OUTPUT_MODE_ANSI16 2
#define JSL__CMD_LINE_OUTPUT_MODE_ANSI256 3
#define JSL__CMD_LINE_OUTPUT_MODE_TRUECOLOR 4

static const JSLImmutableMemory JSL__CMD_LINE_EMPTY_VALUE = JSL_CSTR_INITIALIZER("");

static JSL__FORCE_INLINE void jsl__cmd_line_args_set_error(struct JSLCmdLineArgs* args, JSLImmutableMemory* out_error, JSLImmutableMemory message)
{
    bool params_valid = (
        args != NULL
        && out_error != NULL
        && (message.data != NULL || message.length == 0)
    );

    if (params_valid)
    {
        *out_error = message;
    }
}

static JSL__FORCE_INLINE void jsl__cmd_line_args_set_short_flag(struct JSLCmdLineArgs* args, uint8_t flag)
{
    uint32_t bucket_index = (uint32_t) flag >> 6;
    uint32_t bit_index = (uint32_t) flag & 63u;
    args->_short_flag_bitset[bucket_index] |= (uint64_t) 1u << bit_index;
}

static bool jsl__cmd_line_args_validate_utf8(JSLImmutableMemory str)
{
    bool res = false;

    bool params_valid = (
        str.length > -1
        && (str.data != NULL || str.length == 0)
    );

    int64_t index = 0;
    bool valid = params_valid;
    while (valid && index < str.length)
    {
        uint8_t byte = str.data[index];
        int64_t remaining = str.length - index;

        if (byte <= 0x7Fu)
        {
            ++index;
            continue;
        }

        if ((byte >> 5) == 0x6)
        {
            bool length_ok = remaining >= 2;
            bool overlong = byte < 0xC2u;
            bool cont_ok = length_ok
                && (str.data[index + 1] & JSL__CMD_LINE_UTF8_CONT_MASK) == JSL__CMD_LINE_UTF8_CONT_VALUE;

            valid = length_ok && cont_ok && !overlong;
            index += valid ? 2 : 0;
            continue;
        }

        if ((byte >> 4) == 0xE)
        {
            bool length_ok = remaining >= 3;
            uint8_t b1 = length_ok ? str.data[index + 1] : 0;
            uint8_t b2 = length_ok ? str.data[index + 2] : 0;

            bool cont_ok = length_ok
                && (b1 & JSL__CMD_LINE_UTF8_CONT_MASK) == JSL__CMD_LINE_UTF8_CONT_VALUE
                && (b2 & JSL__CMD_LINE_UTF8_CONT_MASK) == JSL__CMD_LINE_UTF8_CONT_VALUE;

            bool overlong = byte == 0xE0u && b1 < 0xA0u;
            bool surrogate = byte == 0xEDu && b1 >= 0xA0u;

            valid = length_ok && cont_ok && !overlong && !surrogate;
            index += valid ? 3 : 0;
            continue;
        }

        if ((byte >> 3) == 0x1E)
        {
            bool length_ok = remaining >= 4;
            uint8_t b1 = length_ok ? str.data[index + 1] : 0;
            uint8_t b2 = length_ok ? str.data[index + 2] : 0;
            uint8_t b3 = length_ok ? str.data[index + 3] : 0;

            bool b1_ok = false;
            if (byte == 0xF0u)
            {
                b1_ok = b1 >= 0x90u && b1 <= 0xBFu;
            }
            else if (byte == 0xF4u)
            {
                b1_ok = b1 >= 0x80u && b1 <= 0x8Fu;
            }
            else
            {
                b1_ok = (b1 & JSL__CMD_LINE_UTF8_CONT_MASK) == JSL__CMD_LINE_UTF8_CONT_VALUE;
            }

            bool cont_ok = length_ok
                && b1_ok
                && (b2 & JSL__CMD_LINE_UTF8_CONT_MASK) == JSL__CMD_LINE_UTF8_CONT_VALUE
                && (b3 & JSL__CMD_LINE_UTF8_CONT_MASK) == JSL__CMD_LINE_UTF8_CONT_VALUE
                && byte <= 0xF4u;

            valid = length_ok && cont_ok;
            index += valid ? 4 : 0;
            continue;
        }

        valid = false;
    }

    bool consumed_all = params_valid && valid && index == str.length;
    if (consumed_all)
    {
        res = true;
    }

    return res;
}

static bool jsl__cmd_line_args_utf16_to_utf8(JSLAllocatorInterface* allocator, wchar_t* wide, JSLImmutableMemory* out_utf8)
{
    bool res = false;

    bool params_valid = (
        allocator != NULL
        && out_utf8 != NULL
        && wide != NULL
    );

    int64_t total_bytes = 0;
    size_t wide_length = 0;
    bool iteration_valid = params_valid;

    while (iteration_valid && wide[wide_length] != 0)
    {
        uint32_t codepoint = 0;

        #if defined(JSL__CMD_LINE_WCHAR_IS_16_BIT)
            uint16_t word = (uint16_t) wide[wide_length];

            if (word >= 0xD800u && word <= 0xDBFFu)
            {
                uint16_t second = (uint16_t) wide[wide_length + 1];
                bool low_surrogate = second >= 0xDC00u && second <= 0xDFFFu;
                if (low_surrogate)
                {
                    codepoint = 0x10000u + (((uint32_t) word - 0xD800u) << 10)
                        + ((uint32_t) second - 0xDC00u);
                    ++wide_length;
                }
                else
                {
                    iteration_valid = false;
                }
            }
            else if (word >= 0xDC00u && word <= 0xDFFFu)
            {
                iteration_valid = false;
            }
            else
            {
                codepoint = word;
            }
        #elif defined(JSL__CMD_LINE_WCHAR_IS_32_BIT)
            uint32_t value = (uint32_t) wide[wide_length];
            bool surrogate = value >= 0xD800u && value <= 0xDFFFu;
            bool too_large = value > 0x10FFFFu;
            if (!surrogate && !too_large)
            {
                codepoint = value;
            }
            else
            {
                iteration_valid = false;
            }
        #else
            iteration_valid = false;
        #endif

        if (iteration_valid)
        {
            int64_t add = 0;
            if (codepoint <= 0x7Fu)
            {
                add = 1;
            }
            else if (codepoint <= 0x7FFu)
            {
                add = 2;
            }
            else if (codepoint <= 0xFFFFu)
            {
                add = 3;
            }
            else
            {
                add = 4;
            }

            bool overflow = total_bytes > INT64_MAX - add;
            iteration_valid = !overflow;
            if (iteration_valid)
            {
                total_bytes += add;
            }
        }

        ++wide_length;
    }

    uint8_t* allocation = NULL;
    uint8_t* write_ptr = NULL;
    if (iteration_valid)
    {
        allocation = jsl_allocator_interface_alloc(allocator, total_bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT, false);
        write_ptr = allocation;
    }

    size_t index = 0;
    bool encode_ok = write_ptr != NULL;
    while (encode_ok && wide[index] != 0)
    {
        uint32_t codepoint = 0;

        #if defined(JSL__CMD_LINE_WCHAR_IS_16_BIT)
            uint16_t word = (uint16_t) wide[index];
            if (word >= 0xD800u && word <= 0xDBFFu)
            {
                uint16_t second = (uint16_t) wide[index + 1];
                codepoint = 0x10000u + (((uint32_t) word - 0xD800u) << 10)
                    + ((uint32_t) second - 0xDC00u);
                ++index;
            }
            else
            {
                codepoint = word;
            }
        #elif defined(JSL__CMD_LINE_WCHAR_IS_32_BIT)
            codepoint = (uint32_t) wide[index];
        #else
            codepoint = 0;
            encode_ok = false;
        #endif

        if (codepoint <= 0x7Fu)
        {
            *write_ptr++ = (uint8_t) codepoint;
        }
        else if (codepoint <= 0x7FFu)
        {
            *write_ptr++ = (uint8_t) (0xC0u | (codepoint >> 6));
            *write_ptr++ = (uint8_t) (JSL__CMD_LINE_UTF8_CONT_VALUE | (codepoint & 0x3Fu));
        }
        else if (codepoint <= 0xFFFFu)
        {
            *write_ptr++ = (uint8_t) (0xE0u | (codepoint >> 12));
            *write_ptr++ = (uint8_t) (JSL__CMD_LINE_UTF8_CONT_VALUE | ((codepoint >> 6) & 0x3Fu));
            *write_ptr++ = (uint8_t) (JSL__CMD_LINE_UTF8_CONT_VALUE | (codepoint & 0x3Fu));
        }
        else
        {
            *write_ptr++ = (uint8_t) (0xF0u | (codepoint >> 18));
            *write_ptr++ = (uint8_t) (JSL__CMD_LINE_UTF8_CONT_VALUE | ((codepoint >> 12) & 0x3Fu));
            *write_ptr++ = (uint8_t) (JSL__CMD_LINE_UTF8_CONT_VALUE | ((codepoint >> 6) & 0x3Fu));
            *write_ptr++ = (uint8_t) (JSL__CMD_LINE_UTF8_CONT_VALUE | (codepoint & 0x3Fu));
        }

        ++index;
    }

    if (params_valid && encode_ok)
    {
        out_utf8->data = allocation;
        out_utf8->length = total_bytes;
        res = true;
    }

    return res;
}

static void jsl__cmd_line_args_reset(struct JSLCmdLineArgs* args)
{
    bool params_valid = args != NULL;

    if (params_valid)
    {
        // clear
        for (int32_t i = 0; i < JSL__CMD_LINE_SHORT_FLAG_BUCKETS; ++i)
        {
            args->_short_flag_bitset[i] = 0;
        }

        args->_arg_list_length = 0;
        args->_arg_list_index = 0;

        jsl_str_to_str_map_clear(&args->_long_flags);
        jsl_str_set_clear(&args->_commands);
        jsl_str_to_str_multimap_clear(&args->_flags_with_values);
    }
}

static bool jsl__cmd_line_args_ensure_arg_capacity(struct JSLCmdLineArgs* args, int64_t capacity_needed)
{
    bool res = false;

    bool params_valid = (
        args != NULL
        && args->_allocator != NULL
        && capacity_needed > -1
    );

    bool enough = params_valid && capacity_needed <= args->_arg_list_capacity;
    if (enough)
    {
        res = true;
    }

    bool needs_allocation = params_valid && !enough;
    if (needs_allocation)
    {
        bool can_multiply = capacity_needed <= (INT64_MAX / (int64_t) sizeof(JSLImmutableMemory));
        if (can_multiply)
        {
            int64_t bytes = capacity_needed * (int64_t) sizeof(JSLImmutableMemory);
            JSLImmutableMemory* allocation = jsl_allocator_interface_alloc(
                args->_allocator,
                bytes,
                _Alignof(JSLImmutableMemory),
                false
            );

            if (allocation != NULL)
            {
                args->_arg_list = allocation;
                args->_arg_list_capacity = capacity_needed;
                res = true;
            }
        }
    }

    return res;
}

static bool jsl__cmd_line_args_copy_arg(struct JSLCmdLineArgs* args, JSLImmutableMemory raw, JSLImmutableMemory* out_copy)
{
    bool res = false;

    bool params_valid = (
        args != NULL
        && args->_allocator != NULL
        && out_copy != NULL
        && raw.length > -1
        && (raw.data != NULL || raw.length == 0)
    );

    void* copy = NULL;
    bool allocated = false;
    if (params_valid)
    {
        copy = jsl_allocator_interface_alloc(
            args->_allocator,
            raw.length,
            JSL_DEFAULT_ALLOCATION_ALIGNMENT,
            false
        );
        allocated = (copy != NULL || raw.length == 0);
    }

    if (params_valid && allocated)
    {
        JSL_MEMCPY(copy, raw.data, (size_t) raw.length);
        out_copy->data = copy;
        out_copy->length = raw.length;
        res = true;
    }

    return res;
}

static bool jsl__cmd_line_args_add_command(
    struct JSLCmdLineArgs* args,
    JSLImmutableMemory command,
    int32_t arg_index,
    JSLImmutableMemory* out_error
)
{
    bool res = false;

    bool params_valid = (
        args != NULL
        && args->_arg_list != NULL
        && args->_arg_list_length < args->_arg_list_capacity
        && command.data != NULL
        && command.length > -1
    );

    if (params_valid)
    {
        args->_arg_list[args->_arg_list_length] = command;
        ++args->_arg_list_length;
    }

    bool inserted = params_valid
        && jsl_str_set_insert(
            &args->_commands,
            command,
            JSL_STRING_LIFETIME_STATIC
        );

    if (params_valid && !inserted && out_error != NULL)
    {
        jsl__cmd_line_args_set_error(
            args,
            out_error,
            jsl_format(
                args->_allocator,
                JSL_CSTR_EXPRESSION("Unable to store argument %d: %y"),
                arg_index,
                command
            )
        );
    }

    if (params_valid && inserted)
    {
        res = true;
    }

    return res;
}

static bool jsl__cmd_line_args_handle_long_option(
    struct JSLCmdLineArgs* args,
    JSLImmutableMemory arg,
    JSLImmutableMemory separate_value,
    bool has_separate_value,
    bool* out_consumed_separate,
    int32_t arg_index,
    JSLImmutableMemory* out_error
)
{
    bool res = false;

    bool params_valid = (
        args != NULL
        && arg.data != NULL
        && arg.length > 2
    );

    JSLImmutableMemory flag_body = {0};
    if (params_valid)
    {
        flag_body = jsl_slice(arg, 2, arg.length);
        params_valid = flag_body.data != NULL && flag_body.length > 0;
        if (!params_valid && out_error != NULL)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Expected a flag name after \"--\" in argument %d"),
                    arg_index
                )
            );
        }
    }

    int64_t equals_index = -1;
    if (params_valid)
    {
        equals_index = jsl_index_of(flag_body, '=');
        if (equals_index == 0)
        {
            if (out_error != NULL)
            {
                jsl__cmd_line_args_set_error(
                    args,
                    out_error,
                    jsl_format(
                        args->_allocator,
                        JSL_CSTR_EXPRESSION("Expected a flag name before '=' in argument %d"),
                        arg_index
                    )
                );
            }
            params_valid = false;
        }
    }

    bool consumed_separate = false;

    bool insert_flag = params_valid && equals_index < 0;
    bool insert_flag_with_value = false;
    if (insert_flag)
    {
        if (has_separate_value)
        {
            insert_flag_with_value = true;
        }
        else
        {
            bool inserted = jsl_str_to_str_map_insert(
                &args->_long_flags,
                flag_body,
                JSL_STRING_LIFETIME_STATIC,
                JSL__CMD_LINE_EMPTY_VALUE,
                JSL_STRING_LIFETIME_STATIC
            );

            if (out_error != NULL && !inserted)
            {
                jsl__cmd_line_args_set_error(
                    args,
                    out_error,
                    jsl_format(
                        args->_allocator,
                        JSL_CSTR_EXPRESSION("Unable to record flag --%y"),
                        flag_body
                    )
                );
            }

            if (inserted)
            {
                res = true;
            }
        }
    }

    if (params_valid && equals_index > 0)
    {
        insert_flag_with_value = true;
    }

    if (insert_flag_with_value && params_valid)
    {
        JSLImmutableMemory key = equals_index > 0
            ? jsl_slice(flag_body, 0, equals_index)
            : flag_body;
        JSLImmutableMemory value = {0};

        if (equals_index > 0)
        {
            value = jsl_slice(flag_body, equals_index + 1, flag_body.length);
        }
        else if (has_separate_value)
        {
            value = separate_value;
            consumed_separate = true;
        }

        bool key_valid = key.data != NULL && key.length > -1;
        if (!key_valid && out_error != NULL)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Expected a flag name before value in argument %d"),
                    arg_index
                )
            );
        }
        bool inserted = key_valid
            && jsl_str_to_str_multimap_insert(
                &args->_flags_with_values,
                key,
                JSL_STRING_LIFETIME_STATIC,
                value,
                JSL_STRING_LIFETIME_STATIC
            );

        if (out_error != NULL && key_valid && !inserted)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Unable to store value for --%y"),
                    key
                )
            );
        }

        if (inserted)
        {
            res = true;
        }
    }

    if (params_valid && out_consumed_separate != NULL)
    {
        *out_consumed_separate = consumed_separate;
    }

    return res;
}

static bool jsl__cmd_line_args_handle_short_option(
    struct JSLCmdLineArgs* args,
    JSLImmutableMemory arg,
    int32_t arg_index,
    JSLImmutableMemory* out_error
)
{
    bool res = false;

    bool params_valid = (
        args != NULL
        && arg.data != NULL
        && arg.length > 1
    );

    JSLImmutableMemory flags = {0};
    if (params_valid)
    {
        flags = jsl_slice(arg, 1, arg.length);
        params_valid = flags.data != NULL && flags.length > 0;
    }

    int64_t equals_index = -1;
    if (params_valid)
    {
        equals_index = jsl_index_of(flags, '=');
        if (equals_index >= 0)
        {
            if (out_error != NULL)
            {
                jsl__cmd_line_args_set_error(
                    args,
                    out_error,
                    jsl_format(
                        args->_allocator,
                        JSL_CSTR_EXPRESSION("Short flags cannot use '=' (argument %d: %y)"),
                        arg_index,
                        arg
                    )
                );
            }
            return false;
        }
    }

    bool only_flags = params_valid;
    if (only_flags)
    {
        bool ascii_valid = true;
        for (int64_t i = 0; ascii_valid && i < flags.length; ++i)
        {
            uint8_t flag_char = flags.data[i];
            ascii_valid = flag_char < 0x80u;
            if (ascii_valid)
            {
                jsl__cmd_line_args_set_short_flag(args, flag_char);
            }
        }

        if (!ascii_valid && out_error != NULL)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Short flags must be ASCII (argument %d: %y)"),
                    arg_index,
                    arg
                )
            );
        }

        if (ascii_valid)
        {
            res = true;
        }
    }

    return res;
}

static bool jsl__cmd_line_args_process_arg(
    struct JSLCmdLineArgs* args,
    JSLImmutableMemory stored_arg,
    JSLImmutableMemory next_arg,
    bool has_next_arg,
    bool* stop_parsing,
    bool* consumed_next,
    int32_t arg_index,
    JSLImmutableMemory* out_error
)
{
    bool res = false;

    bool params_valid = (
        args != NULL
        && stop_parsing != NULL
        && stored_arg.data != NULL
        && stored_arg.length > -1
    );

    if (!params_valid && out_error != NULL && args != NULL && args->_allocator != NULL)
    {
        jsl__cmd_line_args_set_error(
            args,
            out_error,
            jsl_format(
                args->_allocator,
                JSL_CSTR_EXPRESSION("Invalid argument at position %d"),
                arg_index
            )
        );
    }

    bool parsing_done = params_valid && *stop_parsing;
    bool needs_stop = params_valid
        && !parsing_done
        && stored_arg.length == 2
        && stored_arg.data[0] == '-'
        && stored_arg.data[1] == '-';

    if (needs_stop)
    {
        parsing_done = true;
    }

    bool handled = params_valid && parsing_done;
    if (handled && !needs_stop)
    {
        handled = jsl__cmd_line_args_add_command(args, stored_arg, arg_index, out_error);
    }

    bool is_long_flag = params_valid
        && !parsing_done
        && stored_arg.length > 2
        && stored_arg.data[0] == '-'
        && stored_arg.data[1] == '-';

    bool consumed_next_arg = false;
    if (is_long_flag)
    {
        handled = jsl__cmd_line_args_handle_long_option(
            args,
            stored_arg,
            next_arg,
            has_next_arg,
            &consumed_next_arg,
            arg_index,
            out_error
        );
    }

    bool is_short_flag = params_valid
        && !parsing_done
        && !is_long_flag
        && stored_arg.length > 1
        && stored_arg.data[0] == '-';

    if (is_short_flag)
    {
        handled = jsl__cmd_line_args_handle_short_option(args, stored_arg, arg_index, out_error);
    }

    bool is_command = params_valid
        && !parsing_done
        && !is_long_flag
        && !is_short_flag;

    if (is_command)
    {
        handled = jsl__cmd_line_args_add_command(args, stored_arg, arg_index, out_error);
    }

    if (params_valid)
    {
        *stop_parsing = parsing_done;
        if (consumed_next != NULL)
        {
            *consumed_next = consumed_next_arg;
        }
    }

    if (params_valid && handled)
    {
        res = true;
    }
    else if (params_valid && !handled && out_error != NULL && out_error->data == NULL)
    {
        jsl__cmd_line_args_set_error(
            args,
            out_error,
            jsl_format(
                args->_allocator,
                JSL_CSTR_EXPRESSION("Could not parse argument %d: %y"),
                arg_index,
                stored_arg
            )
        );
    }

    return res;
}

typedef bool (*JSL__CmdLinePrepareArg)(
    struct JSLCmdLineArgs* args,
    void* argv,
    int32_t index,
    JSLImmutableMemory* out_arg,
    JSLImmutableMemory* out_error
);

typedef bool (*JSL__CmdLineIsFlag)(void* argv, int32_t index);

static bool jsl__cmd_line_args_is_narrow_flag(void* argv, int32_t index)
{
    bool res = false;

    if (argv != NULL)
    {
        char** args = (char**) argv;
        char* value = args[index];
        res = value != NULL && value[0] == '-' && value[1] != '\0';
    }

    return res;
}

static bool jsl__cmd_line_args_is_wide_flag(void* argv, int32_t index)
{
    bool res = false;

    if (argv != NULL)
    {
        wchar_t** args = (wchar_t**) argv;
        wchar_t* value = args[index];
        res = value != NULL && value[0] == L'-' && value[1] != 0;
    }

    return res;
}

static bool jsl__cmd_line_args_prepare_utf8_arg(
    struct JSLCmdLineArgs* args,
    void* argv,
    int32_t index,
    JSLImmutableMemory* out_arg,
    JSLImmutableMemory* out_error
)
{
    bool res = false;

    bool params_valid = (
        args != NULL
        && argv != NULL
        && out_arg != NULL
    );

    JSLImmutableMemory raw = {0};
    if (params_valid)
    {
        char** arg_array = (char**) argv;
        raw = jsl_cstr_to_memory(arg_array[index]);
        params_valid = raw.data != NULL;
        if (!params_valid && out_error != NULL)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Argument %d is missing"),
                    index
                )
            );
        }
    }

    bool utf8_ok = params_valid && jsl__cmd_line_args_validate_utf8(raw);
    if (params_valid && !utf8_ok && out_error != NULL)
    {
        jsl__cmd_line_args_set_error(
            args,
            out_error,
            jsl_format(
                args->_allocator,
                JSL_CSTR_EXPRESSION("Argument %d is not valid UTF-8"),
                index
            )
        );
    }

    JSLImmutableMemory stored = {0};
    bool copied = utf8_ok && jsl__cmd_line_args_copy_arg(args, raw, &stored);
    if (utf8_ok && !copied && out_error != NULL)
    {
        jsl__cmd_line_args_set_error(
            args,
            out_error,
            jsl_format(
                args->_allocator,
                JSL_CSTR_EXPRESSION("Unable to store argument %d"),
                index
            )
        );
    }

    if (utf8_ok && copied)
    {
        *out_arg = stored;
        res = true;
    }

    return res;
}

static bool jsl__cmd_line_args_prepare_wide_arg(
    struct JSLCmdLineArgs* args,
    void* argv,
    int32_t index,
    JSLImmutableMemory* out_arg,
    JSLImmutableMemory* out_error
)
{
    bool res = false;

    bool params_valid = (
        args != NULL
        && argv != NULL
        && out_arg != NULL
    );

    JSLImmutableMemory utf8_arg = {0};
    wchar_t* wide_arg = NULL;
    if (params_valid)
    {
        wchar_t** arg_array = (wchar_t**) argv;
        wide_arg = arg_array[index];
        params_valid = wide_arg != NULL;
        if (!params_valid && out_error != NULL)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Argument %d is missing"),
                    index
                )
            );
        }
    }

    if (params_valid)
    {
        bool converted = jsl__cmd_line_args_utf16_to_utf8(args->_allocator, wide_arg, &utf8_arg)
            && jsl__cmd_line_args_validate_utf8(utf8_arg);

        if (!converted && out_error != NULL)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Argument %d is not valid UTF-16"),
                    index
                )
            );
        }

        res = converted;
    }

    if (res)
    {
        *out_arg = utf8_arg;
    }

    return res;
}

static bool jsl__cmd_line_args_parse_common(
    struct JSLCmdLineArgs* cmd_args,
    int32_t argc,
    void* argv,
    JSL__CmdLinePrepareArg prepare_arg,
    JSL__CmdLineIsFlag is_flag_like,
    JSLImmutableMemory* out_error
)
{
    struct JSLCmdLineArgs* args = (struct JSLCmdLineArgs*) cmd_args;

    bool res = false;

    if (out_error != NULL)
    {
        *out_error = (JSLImmutableMemory) {0};
    }

    bool params_valid = (
        args != NULL
        && args->_sentinel == JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL
        && argv != NULL
        && argc > -1
        && prepare_arg != NULL
        && is_flag_like != NULL
    );

    if (!params_valid)
    {
        return false;
    }

    jsl__cmd_line_args_reset(args);

    int64_t capacity_needed = (int64_t) argc;
    bool capacity_ok = jsl__cmd_line_args_ensure_arg_capacity(args, capacity_needed);

    if (!capacity_ok)
    {
        if (out_error != NULL)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Command line input exceeds memory limit")
                )
            );
        }
        return false;
    }

    bool parse_ok = true;
    bool stop_parsing = false;

    for (int32_t index = 1; parse_ok && index < argc; ++index)
    {
        JSLImmutableMemory stored = {0};
        parse_ok = prepare_arg(args, argv, index, &stored, out_error);

        bool has_next_raw = parse_ok && (index + 1) < argc;
        bool next_is_flag = has_next_raw && is_flag_like(argv, index + 1);

        bool should_prepare_next = parse_ok
            && !stop_parsing
            && has_next_raw
            && stored.length > 2
            && stored.data[0] == '-'
            && stored.data[1] == '-'
            && jsl_index_of(stored, '=') < 0
            && !next_is_flag;

        JSLImmutableMemory next_stored = {0};
        bool next_available = false;
        if (should_prepare_next)
        {
            next_available = prepare_arg(args, argv, index + 1, &next_stored, out_error);
            parse_ok = next_available;
        }

        bool consumed_next = false;
        bool processed = parse_ok
            && jsl__cmd_line_args_process_arg(
                args,
                stored,
                next_stored,
                next_available,
                &stop_parsing,
                &consumed_next,
                index,
                out_error
            );

        if (processed && consumed_next)
        {
            ++index;
        }

        parse_ok = processed;
    }

    if (parse_ok)
    {
        res = true;
    }
    else
    {
        jsl__cmd_line_args_reset(args);
        if (out_error != NULL && out_error->data == NULL)
        {
            jsl__cmd_line_args_set_error(
                args,
                out_error,
                jsl_format(
                    args->_allocator,
                    JSL_CSTR_EXPRESSION("Failed to parse command line input")
                )
            );
        }
    }

    return res;
}

static inline uint8_t jsl__cmd_line_to_lower(uint8_t ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return ch + 32;
    }
    return ch;
}

static bool jsl__cmd_line_str_contains_ci(JSLImmutableMemory haystack, JSLImmutableMemory needle)
{
    if (
        haystack.data == NULL
        || haystack.length < 1
        || needle.data == NULL
        || needle.length < 1
        || needle.length > haystack.length
    )
    {
        return false;
    }

    uint8_t n0_lower = jsl__cmd_line_to_lower(needle.data[0]);
    uint8_t nlast = needle.data[needle.length - 1];
    uint8_t nlast_lower = jsl__cmd_line_to_lower(nlast);
    int64_t last_index = needle.length - 1;
    int64_t max_i = haystack.length - needle.length;

    for (int64_t i = 0; i <= max_i; ++i)
    {
        uint8_t h0 = jsl__cmd_line_to_lower(haystack.data[i]);
        if (h0 != n0_lower)
        {
            continue;
        }

        uint8_t hlast = jsl__cmd_line_to_lower(haystack.data[i + last_index]);
        if (hlast != nlast_lower)
        {
            continue;
        }

        int64_t j = 1;
        while (j < last_index)
        {
            uint8_t hc = jsl__cmd_line_to_lower(haystack.data[i + j]);
            uint8_t nc = jsl__cmd_line_to_lower(needle.data[j]);
            if (hc != nc)
            {
                break;
            }
            ++j;
        }

        if (j == last_index)
        {
            return true;
        }
    }

    return false;
}

static bool jsl__cmd_line_env_truthy(JSLImmutableMemory value)
{
    if (value.data == NULL || value.length < 1)
    {
        return false;
    }

    static JSLImmutableMemory zero_str = JSL_CSTR_INITIALIZER("0");
    static JSLImmutableMemory false_str = JSL_CSTR_INITIALIZER("false");
    static JSLImmutableMemory no_str = JSL_CSTR_INITIALIZER("no");
    static JSLImmutableMemory off_str = JSL_CSTR_INITIALIZER("off");

    if (jsl_memory_compare(value, zero_str)
        || jsl_memory_compare(value, false_str)
        || jsl_memory_compare(value, no_str)
        || jsl_memory_compare(value, off_str))
    {
        return false;
    }

    return true;
}

bool jsl_cmd_line_get_terminal_info(JSLTerminalInfo* info, uint32_t flags)
{
    JSL_MEMSET(info, 0, sizeof(JSLTerminalInfo));

    bool force_no_color = JSL_IS_BITFLAG_SET(flags, JSL_GET_TERMINAL_INFO_FORCE_NO_COLOR);
    bool force_16 = JSL_IS_BITFLAG_SET(flags, JSL_GET_TERMINAL_INFO_FORCE_16_COLOR_MODE);
    bool force_255 = JSL_IS_BITFLAG_SET(flags, JSL_GET_TERMINAL_INFO_FORCE_255_COLOR_MODE);
    bool force_24 = JSL_IS_BITFLAG_SET(flags, JSL_GET_TERMINAL_INFO_FORCE_24_BIT_COLOR_MODE);

    int32_t param_check = (force_no_color + force_16 + force_255 + force_24);
    if (param_check > 1)
    {
        info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_INVALID_STATE;
        return false;
    }

    if (force_no_color)
    {
        info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_NONE;
        return true;
    }

    if (force_16)
    {
        info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_ANSI16;
        return true;
    }

    if (force_255)
    {
        info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_ANSI256;
        return true;
    }

    if (force_24)
    {
        info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_TRUECOLOR;
        return true;
    }

    //
    //      Auto-detect Code
    //

    // default to none
    info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_NONE;

    #if JSL_IS_WINDOWS
        
        // Look at all this fucking code, man. getenv_s is such a terrible API.
        // Two return values you need to check
        #define GET_WIN_ENV_SAFE(VAR_NAME, ENV_NAME)                    \
            JSLImmutableMemory VAR_NAME = {0};                                   \
            char _buffer_##VAR_NAME[128];                               \
            size_t _res_##VAR_NAME = 0;                                 \
            int32_t _err_##VAR_NAME = getenv_s(                         \
                &_res_##VAR_NAME,                                       \
                _buffer_##VAR_NAME,                                     \
                128,                                                    \
                ENV_NAME                                                \
            );                                                          \
            if (_err_##VAR_NAME == 0 && _res_##VAR_NAME > 0)            \
            {                                                           \
                VAR_NAME.data = (uint8_t*) _buffer_##VAR_NAME;          \
                VAR_NAME.length = (int64_t) _res_##VAR_NAME;            \
            }

        GET_WIN_ENV_SAFE(env_no_color, "NO_COLOR")
        GET_WIN_ENV_SAFE(env_term, "TERM")
        GET_WIN_ENV_SAFE(env_colorterm, "COLORTERM")
        GET_WIN_ENV_SAFE(env_clicolor, "CLICOLOR")
        GET_WIN_ENV_SAFE(env_clicolor_force, "CLICOLOR_FORCE")
        GET_WIN_ENV_SAFE(env_force_color, "FORCE_COLOR")
        GET_WIN_ENV_SAFE(env_vte_version, "VTE_VERSION")
        GET_WIN_ENV_SAFE(env_ansicon, "ANSICON")
        GET_WIN_ENV_SAFE(env_conemu_ansi, "ConEmuANSI")
        GET_WIN_ENV_SAFE(env_wt_session, "WT_SESSION")
        
        #undef GET_WIN_ENV_SAFE

    #else

        JSLImmutableMemory env_no_color = jsl_cstr_to_memory(getenv("NO_COLOR"));
        JSLImmutableMemory env_term = jsl_cstr_to_memory(getenv("TERM"));
        JSLImmutableMemory env_colorterm = jsl_cstr_to_memory(getenv("COLORTERM"));
        JSLImmutableMemory env_clicolor = jsl_cstr_to_memory(getenv("CLICOLOR"));
        JSLImmutableMemory env_clicolor_force = jsl_cstr_to_memory(getenv("CLICOLOR_FORCE"));
        JSLImmutableMemory env_force_color = jsl_cstr_to_memory(getenv("FORCE_COLOR"));
        JSLImmutableMemory env_vte_version = jsl_cstr_to_memory(getenv("VTE_VERSION"));
        JSLImmutableMemory env_ansicon = jsl_cstr_to_memory(getenv("ANSICON"));
        JSLImmutableMemory env_conemu_ansi = jsl_cstr_to_memory(getenv("ConEmuANSI"));
        JSLImmutableMemory env_wt_session = jsl_cstr_to_memory(getenv("WT_SESSION"));

    #endif

    static JSLImmutableMemory dumb_str = JSL_CSTR_INITIALIZER("dumb");
    bool term_dumb = jsl_compare_ascii_insensitive(env_term, dumb_str);

    bool clicolor_disabled = env_clicolor.length > 0
        && !jsl__cmd_line_env_truthy(env_clicolor);

    bool color_disabled = env_no_color.length > 0
        || term_dumb
        || clicolor_disabled;

    bool color_forced = jsl__cmd_line_env_truthy(env_clicolor_force)
        || jsl__cmd_line_env_truthy(env_force_color);

    bool is_tty = false;

    #if JSL_IS_WINDOWS

        bool windows_console = false;
        bool windows_vt_enabled = false;

        // Explicitly set VT code processing for Windows 10 or greater

        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (handle != NULL && handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode))
        {
            windows_console = true;
            is_tty = true;

            if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0)
            {
                windows_vt_enabled = true;
            }
            else
            {
                DWORD new_mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                if (SetConsoleMode(handle, new_mode))
                {
                    windows_vt_enabled = true;
                }
            }
        }

        if (!is_tty)
        {
            int fd = _fileno(stdout);
            if (fd >= 0 && _isatty(fd))
            {
                is_tty = true;
            }
        }

    #elif JSL_IS_POSIX

        is_tty = isatty(fileno(stdout)) != 0;

    #else

        JSL_ASSERT(0 && "Terminal auto detection is only available on Windows or POSIX systems");

    #endif

    bool ansi_available = !color_disabled && (is_tty || color_forced);

    #if JSL_IS_WINDOWS
        bool ansi_shim = (env_ansicon.data != NULL && env_ansicon.length > 0)
            || jsl__cmd_line_env_truthy(env_conemu_ansi);

        if (windows_console && !windows_vt_enabled && !ansi_shim)
        {
            ansi_available = false;
        }
    #endif

    static JSLImmutableMemory color_str = JSL_CSTR_INITIALIZER("color");
    static JSLImmutableMemory truecolor_str = JSL_CSTR_INITIALIZER("truecolor");
    static JSLImmutableMemory bit24_str = JSL_CSTR_INITIALIZER("24bit");
    static JSLImmutableMemory color256_str = JSL_CSTR_INITIALIZER("256color");

    bool colorterm_truecolor = jsl__cmd_line_str_contains_ci(env_colorterm, truecolor_str)
        || jsl__cmd_line_str_contains_ci(env_colorterm, bit24_str);

    bool term_has_256 = jsl__cmd_line_str_contains_ci(env_term, color256_str);
    bool term_has_color = jsl__cmd_line_str_contains_ci(env_term, color_str);

    bool truecolor_hint = colorterm_truecolor
        || env_wt_session.length > 0;

    bool ansi256_hint = term_has_256
        || env_vte_version.length > 0;

    bool ansi16_hint = term_has_color
        || env_colorterm.data != NULL
        || env_ansicon.data != NULL
        || env_conemu_ansi.data != NULL;

    if (ansi_available)
    {
        if (truecolor_hint)
        {
            info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_TRUECOLOR;
        }
        else if (ansi256_hint)
        {
            info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_ANSI256;
        }
        else if (ansi16_hint)
        {
            info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_ANSI16;
        }
        else
        {
            info->_output_mode = JSL__CMD_LINE_OUTPUT_MODE_ANSI16;
        }
    }

    return true;
}

static const uint32_t jsl__ansi16[16][3] = {
    {  0,   0,   0}, {205,   0,   0}, {  0, 205,   0}, {205, 205,   0},
    {  0,   0, 238}, {205,   0, 205}, {  0, 205, 205}, {229, 229, 229},
    {127, 127, 127}, {255,   0,   0}, {  0, 255,   0}, {255, 255,   0},
    { 92,  92, 255}, {255,   0, 255}, {  0, 255, 255}, {255, 255, 255}
};

uint8_t jsl_cmd_line_rgb_to_ansi16(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t best = 0;
    uint32_t best_dist = UINT32_MAX;

    for (uint8_t i = 0; i < 16; ++i)
    {
        uint32_t dr = (uint32_t) r - jsl__ansi16[i][0];
        uint32_t dg = (uint32_t) g - jsl__ansi16[i][1];
        uint32_t db = (uint32_t) b - jsl__ansi16[i][2];

        // perceptual-ish weighting; still cheap integer math
        uint32_t red_dist = 30u * dr * dr;
        uint32_t green_dist = 59u * dg * dg;
        uint32_t blue_dist = 11u * db * db;
        
        uint32_t dist = red_dist + green_dist + blue_dist;

        if (dist < best_dist)
        {
            best_dist = dist;
            best = i;
        }
    }

    return best;
}

uint8_t jsl_cmd_line_rgb_to_ansi256(uint8_t r, uint8_t g, uint8_t b)
{
    static const uint8_t cube_levels[6] = { 0, 95, 135, 175, 215, 255 };

    uint8_t best = 0;
    uint32_t best_dist = UINT32_MAX;

    for (uint8_t i = 0; i < 16; ++i)
    {
        uint32_t dr = (uint32_t) r - jsl__ansi16[i][0];
        uint32_t dg = (uint32_t) g - jsl__ansi16[i][1];
        uint32_t db = (uint32_t) b - jsl__ansi16[i][2];

        uint32_t red_dist = 30u * dr * dr;
        uint32_t green_dist = 59u * dg * dg;
        uint32_t blue_dist = 11u * db * db;
        
        uint32_t dist = red_dist + green_dist + blue_dist;

        if (dist < best_dist)
        {
            best_dist = dist;
            best = i;
        }
    }

    for (uint8_t ri = 0; ri < 6; ++ri)
    {
        uint8_t rv = cube_levels[ri];
        uint32_t dr = (uint32_t) r - (uint32_t) rv;
        uint32_t drw = 30u*dr*dr;

        for (uint8_t gi = 0; gi < 6; ++gi)
        {
            uint8_t gv = cube_levels[gi];
            uint32_t dg = (uint32_t) g - (uint32_t) gv;
            uint32_t drdg = drw + 59u*dg*dg;

            for (uint8_t bi = 0; bi < 6; ++bi)
            {
                uint8_t bv = cube_levels[bi];
                uint32_t db = (uint32_t) b - (uint32_t) bv;
                uint32_t dist = drdg + 11u*db*db;

                if (dist < best_dist)
                {
                    best_dist = dist;
                    best = (uint8_t)(16 + 36u*ri + 6u*gi + bi);
                }
            }
        }
    }

    for (uint8_t i = 0; i < 24; ++i)
    {
        uint8_t level = (uint8_t)(8u + 10u*i);
        uint32_t dr = (uint32_t) r - (uint32_t) level;
        uint32_t dg = (uint32_t) g - (uint32_t) level;
        uint32_t db = (uint32_t) b - (uint32_t) level;

        uint32_t dist = 30u*dr*dr + 59u*dg*dg + 11u*db*db;
        if (dist < best_dist)
        {
            best_dist = dist;
            best = (uint8_t)(232 + i);
        }
    }

    return best;
}

uint8_t jsl_cmd_line_ansi256_to_ansi16(uint8_t color256)
{
    /* Python used to generate LUT:

    ```
    ansi16 = [
        (0,0,0),(205,0,0),(0,205,0),(205,205,0),
        (0,0,238),(205,0,205),(0,205,205),(229,229,229),
        (127,127,127),(255,0,0),(0,255,0),(255,255,0),
        (92,92,255),(255,0,255),(0,255,255),(255,255,255)
    ]

    cube_levels = [0, 95, 135, 175, 215, 255]

    def rgb_to_ansi16(r, g, b):
        best = 0
        best_dist = 2**32-1
        for i,(ar,ag,ab) in enumerate(ansi16):
            dr = r - ar
            dg = g - ag
            db = b - ab
            dist = 30*dr*dr + 59*dg*dg + 11*db*db
            if dist < best_dist:
                best_dist = dist
                best = i
        return best

    def ansi256_to_ansi16(c):
        if c < 16:
            return c
        if c >= 232:
            level = 8 + 10*(c - 232)
            return rgb_to_ansi16(level, level, level)
        idx = c - 16
        ri = idx // 36
        gi = (idx // 6) % 6
        bi = idx % 6
        return rgb_to_ansi16(cube_levels[ri], cube_levels[gi], cube_levels[bi])

    lut = [ansi256_to_ansi16(c) for c in range(256)]
    ```
    */

    static const uint8_t jsl__ansi256_to_ansi16_lut[256] = {
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
         0,  0,  4,  4,  4,  4,  0, 12, 12, 12, 12, 12,  2,  2,  6,  6,
         6,  6,  2,  2,  6,  6,  6,  6,  2,  2,  6,  6,  6,  6, 10, 10,
        14, 14, 14, 14,  0,  0,  4,  4,  4,  4,  8,  8,  8, 12, 12, 12,
         8,  8,  8,  8,  8, 12,  2,  8,  8,  8,  8,  8,  2,  2,  6,  6,
         6,  6, 10, 10, 14, 14, 14, 14,  1,  1,  5,  5,  5,  5,  8,  8,
         8,  8, 12, 12,  8,  8,  8,  8,  8, 12,  3,  8,  8,  8,  8,  8,
         3,  3,  3,  7,  7,  7,  3,  3,  7,  7,  7,  7,  1,  1,  5,  5,
         5,  5,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  3,  3,
         8,  8,  7,  7,  3,  3,  7,  7,  7,  7,  3,  3,  7,  7,  7,  7,
         1,  1,  5,  5,  5,  5,  8,  8,  8,  8,  8, 12,  3,  8,  8,  8,
         8,  8,  3,  3,  3,  7,  7,  7,  3,  3,  7,  7,  7,  7, 11, 11,
         7,  7,  7, 15,  9,  9,  5, 13, 13, 13,  9,  8,  8,  8, 13, 13,
         3,  3,  8,  8,  7,  7,  3,  3,  7,  7,  7,  7,  3,  3,  7,  7,
         7,  7, 11, 11,  7, 15, 15, 15,  0,  0,  0,  0,  0,  0,  8,  8,
         8,  8,  8,  8,  8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  7,  7,
    };

    return jsl__ansi256_to_ansi16_lut[color256];
}

void jsl_cmd_line_color_from_ansi16(JSLCmdLineColor* color, uint8_t color16)
{
    color->_color_type = JSL__CMD_LINE_COLOR_ANSI16;
    color->_ansi16 = color16;
}

void jsl_cmd_line_color_from_ansi256(JSLCmdLineColor* color, uint8_t color256)
{
    color->_color_type = JSL__CMD_LINE_COLOR_ANSI256;
    color->_ansi16 = color256;
}

void jsl_cmd_line_color_from_rgb(JSLCmdLineColor* color, uint8_t r, uint8_t g, uint8_t b)
{
    color->_color_type = JSL__CMD_LINE_COLOR_RGB;
    color->_rgb._r = r;
    color->_rgb._g = g;
    color->_rgb._b = b;
}

void jsl_cmd_line_style_with_foreground(
    JSLCmdLineStyle* style,
    JSLCmdLineColor foreground,
    uint32_t style_flags
)
{
    JSL_MEMCPY(&style->_foreground, &foreground, sizeof(JSLCmdLineColor));
    style->_background._color_type = JSL__CMD_LINE_COLOR_DEFAULT;
    style->_style_attributes = style_flags;
}

void jsl_cmd_line_style_with_background(
    JSLCmdLineStyle* style,
    JSLCmdLineColor background,
    uint32_t style_flags
)
{
    style->_foreground._color_type = JSL__CMD_LINE_COLOR_DEFAULT;
    JSL_MEMCPY(&style->_background, &background, sizeof(JSLCmdLineColor));
    style->_style_attributes = style_flags;
}

void jsl_cmd_line_style_with_foreground_and_background(
    JSLCmdLineStyle* style,
    JSLCmdLineColor foreground,
    JSLCmdLineColor background,
    uint32_t style_flags
)
{
    JSL_MEMCPY(&style->_foreground, &foreground, sizeof(JSLCmdLineColor));
    JSL_MEMCPY(&style->_background, &background, sizeof(JSLCmdLineColor));
    style->_style_attributes = style_flags;
}

static int64_t jsl__cmd_line_write_color(
    JSLOutputSink sink,
    int32_t output_mode,
    JSLCmdLineColor* color,
    bool is_foreground
)
{
    static const JSLImmutableMemory ansi16_fmt = JSL_CSTR_INITIALIZER("\x1b[%dm");
    static const JSLImmutableMemory fg_256_fmt = JSL_CSTR_INITIALIZER("\x1b[38;5;%dm");
    static const JSLImmutableMemory bg_256_fmt = JSL_CSTR_INITIALIZER("\x1b[48;5;%dm");
    static const JSLImmutableMemory fg_rgb_fmt = JSL_CSTR_INITIALIZER("\x1b[38;2;%d;%d;%dm");
    static const JSLImmutableMemory bg_rgb_fmt = JSL_CSTR_INITIALIZER("\x1b[48;2;%d;%d;%dm");

    if (color->_color_type == JSL__CMD_LINE_COLOR_DEFAULT)
        return 0;

    int32_t terminal_color_type = JSL__CMD_LINE_COLOR_DEFAULT;
    switch (output_mode)
    {
        case JSL__CMD_LINE_OUTPUT_MODE_ANSI16:
            terminal_color_type = JSL__CMD_LINE_COLOR_ANSI16;
            break;
        case JSL__CMD_LINE_OUTPUT_MODE_ANSI256:
            terminal_color_type = JSL__CMD_LINE_COLOR_ANSI256;
            break;
        case JSL__CMD_LINE_OUTPUT_MODE_TRUECOLOR:
            terminal_color_type = JSL__CMD_LINE_COLOR_RGB;
            break;
        default:
            return 0;
    }

    JSLCmdLineColor output_color;
    JSL_MEMCPY(&output_color, color, sizeof(JSLCmdLineColor));

    if (terminal_color_type == JSL__CMD_LINE_COLOR_ANSI16)
    {
        if (color->_color_type == JSL__CMD_LINE_COLOR_ANSI256)
        {
            output_color._color_type = JSL__CMD_LINE_COLOR_ANSI16;
            output_color._ansi16 = jsl_cmd_line_ansi256_to_ansi16(color->_ansi256);
        }
        else if (color->_color_type == JSL__CMD_LINE_COLOR_RGB)
        {
            output_color._color_type = JSL__CMD_LINE_COLOR_ANSI16;
            output_color._ansi16 = jsl_cmd_line_rgb_to_ansi16(color->_rgb._r, color->_rgb._g, color->_rgb._b);
        }
    }
    else if (terminal_color_type == JSL__CMD_LINE_COLOR_ANSI256)
    {
        if (color->_color_type == JSL__CMD_LINE_COLOR_RGB)
        {
            output_color._color_type = JSL__CMD_LINE_COLOR_ANSI256;
            output_color._ansi256 = jsl_cmd_line_rgb_to_ansi256(color->_rgb._r, color->_rgb._g, color->_rgb._b);
        }
    }

    switch (output_color._color_type)
    {
        case JSL__CMD_LINE_COLOR_ANSI16:
        {
            int32_t code = 0;
            if (output_color._ansi16 < 8)
            {
                code = (is_foreground ? 30 : 40) + output_color._ansi16;
            }
            else
            {
                code = (is_foreground ? 90 : 100) + (output_color._ansi16 - 8);
            }

            return jsl_format_sink(
                sink,
                ansi16_fmt,
                code
            );
        }
        case JSL__CMD_LINE_COLOR_ANSI256:
        {
            JSLImmutableMemory fmt = is_foreground ? fg_256_fmt : bg_256_fmt;
            return jsl_format_sink(
                sink,
                fmt,
                (int32_t) output_color._ansi256
            );
        }
        case JSL__CMD_LINE_COLOR_RGB:
        {
            JSLImmutableMemory fmt = is_foreground ? fg_rgb_fmt : bg_rgb_fmt;
            return jsl_format_sink(
                sink,
                fmt,
                (int32_t) output_color._rgb._r,
                (int32_t) output_color._rgb._g,
                (int32_t) output_color._rgb._b
            );
        }

        case JSL__CMD_LINE_COLOR_DEFAULT:
        default:
            break;
    }

    return 0;
}

int64_t jsl_cmd_line_write_style(JSLOutputSink sink, JSLTerminalInfo* terminal_info, JSLCmdLineStyle* style)
{
    if (
        terminal_info == NULL
        || style == NULL
        || terminal_info->_output_mode == JSL__CMD_LINE_OUTPUT_MODE_INVALID_STATE
    )
        return -1;
    
    if (terminal_info->_output_mode == JSL__CMD_LINE_OUTPUT_MODE_NONE)
        return 0;

    int64_t bytes_written = 0;
    int64_t result = 0;

    static const JSLImmutableMemory bold_code = JSL_CSTR_INITIALIZER("\x1b[1m");
    static const JSLImmutableMemory dim_code = JSL_CSTR_INITIALIZER("\x1b[2m");
    static const JSLImmutableMemory italic_code = JSL_CSTR_INITIALIZER("\x1b[3m");
    static const JSLImmutableMemory underline_code = JSL_CSTR_INITIALIZER("\x1b[4m");
    static const JSLImmutableMemory dunderline_code = JSL_CSTR_INITIALIZER("\x1b[21m");
    static const JSLImmutableMemory blink_code = JSL_CSTR_INITIALIZER("\x1b[5m");
    static const JSLImmutableMemory rblink_code = JSL_CSTR_INITIALIZER("\x1b[6m");
    static const JSLImmutableMemory inverse_code = JSL_CSTR_INITIALIZER("\x1b[7m");
    static const JSLImmutableMemory hidden_code = JSL_CSTR_INITIALIZER("\x1b[8m");
    static const JSLImmutableMemory strike_code = JSL_CSTR_INITIALIZER("\x1b[9m");

    uint32_t attributes = style->_style_attributes;

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_BOLD))
    {
        result = jsl_output_sink_write(sink, bold_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_DIM))
    {
        result = jsl_output_sink_write(sink, dim_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_ITALIC))
    {
        result = jsl_output_sink_write(sink, italic_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_UNDERLINE))
    {
        result = jsl_output_sink_write(sink, underline_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_DUNDERLINE))
    {
        result = jsl_output_sink_write(sink, dunderline_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_BLINK))
    {
        result = jsl_output_sink_write(sink, blink_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_RBLINK))
    {
        result = jsl_output_sink_write(sink, rblink_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_INVERSE))
    {
        result = jsl_output_sink_write(sink, inverse_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_HIDDEN))
    {
        result = jsl_output_sink_write(sink, hidden_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    if (JSL_IS_BITFLAG_SET(attributes, JSL_CMD_LINE_STYLE_STRIKE))
    {
        result = jsl_output_sink_write(sink, strike_code);
        if (result < 0) return -1;
        bytes_written += result;
    }

    result = jsl__cmd_line_write_color(
        sink,
        terminal_info->_output_mode,
        &style->_foreground,
        true
    );
    if (result < 0) return -1;
    bytes_written += result;

    result = jsl__cmd_line_write_color(
        sink,
        terminal_info->_output_mode,
        &style->_background,
        false
    );
    if (result < 0) return -1;
    bytes_written += result;

    return bytes_written;
}

int64_t jsl_cmd_line_write_reset(JSLOutputSink sink, JSLTerminalInfo* terminal_info)
{
    if (terminal_info == NULL || terminal_info->_output_mode == JSL__CMD_LINE_OUTPUT_MODE_INVALID_STATE)
        return -1;

    if (terminal_info->_output_mode == JSL__CMD_LINE_OUTPUT_MODE_ANSI16
        || terminal_info->_output_mode == JSL__CMD_LINE_OUTPUT_MODE_ANSI256
        || terminal_info->_output_mode == JSL__CMD_LINE_OUTPUT_MODE_TRUECOLOR)
    {
        static JSLImmutableMemory reset_code = JSL_CSTR_INITIALIZER("\x1b[0m");
        return jsl_output_sink_write(sink, reset_code);
    }

    return 0;
}

bool jsl_cmd_line_args_init(JSLCmdLineArgs* cmd_args, JSLAllocatorInterface* allocator)
{
    struct JSLCmdLineArgs* args = (struct JSLCmdLineArgs*) cmd_args;

    bool res = false;

    bool long_init = false;
    bool multimap_init = false;
    bool commands_init = false;

    if (args != NULL && allocator != NULL)
    {
        JSL_MEMSET(args, 0, sizeof(JSLCmdLineArgs));
        args->_allocator = allocator;

        long_init = jsl_str_to_str_map_init(&args->_long_flags, allocator, 0);
        multimap_init = long_init && jsl_str_to_str_multimap_init(&args->_flags_with_values, allocator, 0);
        commands_init = multimap_init && jsl_str_set_init(&args->_commands, allocator, 0);
    }

    if (long_init && multimap_init && commands_init)
    {
        args->_sentinel = JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL;
        res = true;
    }

    return res;
}

bool jsl_cmd_line_args_parse(JSLCmdLineArgs* cmd_args, int32_t argc, char** argv, JSLImmutableMemory* out_error)
{
    return jsl__cmd_line_args_parse_common(
        cmd_args,
        argc,
        (void*) argv,
        jsl__cmd_line_args_prepare_utf8_arg,
        jsl__cmd_line_args_is_narrow_flag,
        out_error
    );
}

bool jsl_cmd_line_args_parse_wide(JSLCmdLineArgs* cmd_args, int32_t argc, wchar_t** argv, JSLImmutableMemory* out_error)
{
    return jsl__cmd_line_args_parse_common(
        cmd_args,
        argc,
        (void*) argv,
        jsl__cmd_line_args_prepare_wide_arg,
        jsl__cmd_line_args_is_wide_flag,
        out_error
    );
}

bool jsl_cmd_line_args_has_short_flag(JSLCmdLineArgs* cmd_args, uint8_t flag)
{
    if (
        cmd_args != NULL
        && cmd_args->_sentinel == JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL
    )
    {
        uint32_t bucket_index = (uint32_t) flag >> 6;
        uint32_t bit_index = (uint32_t) flag & 63u;
        uint64_t mask = (uint64_t) 1u << bit_index;
        return (cmd_args->_short_flag_bitset[bucket_index] & mask) != 0;
    }

    return false;
}

bool jsl_cmd_line_args_has_flag(JSLCmdLineArgs* cmd_args, JSLImmutableMemory flag)
{
    bool res = false;

    bool params_valid = (
        cmd_args != NULL
        && cmd_args->_sentinel == JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL
        && flag.data != NULL
        && flag.length > -1
    );

    if (params_valid)
    {
        res = jsl_str_to_str_map_has_key(&cmd_args->_long_flags, flag);
    }

    return res;
}

bool jsl_cmd_line_args_has_command(JSLCmdLineArgs* cmd_args, JSLImmutableMemory command)
{
    bool res = false;

    bool params_valid = (
        cmd_args != NULL
        && cmd_args->_sentinel == JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL
        && command.data != NULL
        && command.length > -1
    );

    if (params_valid)
    {
        res = jsl_str_set_has(&cmd_args->_commands, command);
    }

    return res;
}

bool jsl_cmd_line_args_pop_arg_list(JSLCmdLineArgs* cmd_args, JSLImmutableMemory* out_value)
{
    bool res = false;

    bool params_valid = (
        cmd_args != NULL
        && cmd_args->_sentinel == JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL
        && out_value != NULL
        && cmd_args->_arg_list != NULL
        && cmd_args->_arg_list_index < cmd_args->_arg_list_length
    );

    if (params_valid)
    {
        *out_value = cmd_args->_arg_list[cmd_args->_arg_list_index];
        ++cmd_args->_arg_list_index;
        res = true;
    }

    return res;
}

bool jsl_cmd_line_args_pop_flag_with_value(
    JSLCmdLineArgs* cmd_args,
    JSLImmutableMemory flag,
    JSLImmutableMemory* out_value
)
{
    bool res = false;

    bool params_valid = (
        cmd_args != NULL
        && cmd_args->_sentinel == JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL
        && out_value != NULL
        && flag.data != NULL
        && flag.length > -1
    );

    JSLStrToStrMultimapValueIter iter;
    bool iterator_init = params_valid
        && jsl_str_to_str_multimap_get_values_for_key_iterator_init(
            &cmd_args->_flags_with_values,
            &iter,
            flag
        );

    JSLImmutableMemory value = {0};
    bool has_value = iterator_init
        && jsl_str_to_str_multimap_get_values_for_key_iterator_next(
            &iter,
            &value
        );

    bool copied = false;
    if (has_value)
    {
        copied = jsl__cmd_line_args_copy_arg(cmd_args, value, out_value);
    }

    if (copied)
    {
        jsl_str_to_str_multimap_delete_value(
            &cmd_args->_flags_with_values,
            flag,
            value
        );
        res = true;
    }

    return res;
}

#undef JSL__CMD_LINE_ARGS_PRIVATE_SENTINEL
#undef JSL__CMD_LINE_WCHAR_IS_16_BIT
#undef JSL__CMD_LINE_WCHAR_IS_32_BIT
#undef JSL__CMD_LINE_OUTPUT_MODE_INVALID_STATE
#undef JSL__CMD_LINE_OUTPUT_MODE_NONE
#undef JSL__CMD_LINE_OUTPUT_MODE_ANSI16
#undef JSL__CMD_LINE_OUTPUT_MODE_ANSI256
#undef JSL__CMD_LINE_OUTPUT_MODE_TRUECOLOR

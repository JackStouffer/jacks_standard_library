
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
#include <limits.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_cmd_line.h"
#include "jsl_str_set.h"
#include "jsl_str_to_str_map.h"
#include "jsl_str_to_str_multimap.h"

#if WCHAR_MAX <= 0xFFFFu
    #define JSL__CMD_LINE_WCHAR_IS_16_BIT 1
#elif WCHAR_MAX <= 0xFFFFFFFFu
    #define JSL__CMD_LINE_WCHAR_IS_32_BIT 1
#else
    #error "Unsupported wchar_t size"
#endif

static const JSLFatPtr JSL__CMD_LINE_EMPTY_VALUE = JSL_FATPTR_INITIALIZER("");

static void jsl__cmd_line_set_error(JSLCmdLine* cmd_line, JSLFatPtr* out_error, JSLFatPtr message)
{
    bool params_valid = (
        cmd_line != NULL
        && out_error != NULL
        && (message.data != NULL || message.length == 0)
    );

    if (params_valid)
    {
        *out_error = message;
    }
}

static void jsl__cmd_line_set_short_flag(JSLCmdLine* cmd_line, uint8_t flag)
{
    uint32_t bucket_index = (uint32_t) flag >> 6;
    uint32_t bit_index = (uint32_t) flag & 63u;
    cmd_line->short_flag_bitset[bucket_index] |= (uint64_t) 1u << bit_index;
}

static bool jsl__cmd_line_short_flag_present(JSLCmdLine* cmd_line, uint8_t flag)
{
    uint32_t bucket_index = (uint32_t) flag >> 6;
    uint32_t bit_index = (uint32_t) flag & 63u;
    uint64_t mask = (uint64_t) 1u << bit_index;
    return (cmd_line->short_flag_bitset[bucket_index] & mask) != 0;
}

static bool jsl__cmd_line_validate_utf8(JSLFatPtr str)
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

static bool jsl__cmd_line_utf16_to_utf8(JSLArena* arena, wchar_t* wide, JSLFatPtr* out_utf8)
{
    bool res = false;

    bool params_valid = (
        arena != NULL
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

    bool allocation_ok = iteration_valid;
    JSLFatPtr buffer = {0};
    if (allocation_ok)
    {
        buffer = jsl_arena_allocate(arena, total_bytes, false);
        allocation_ok = (buffer.data != NULL || total_bytes == 0) && buffer.length >= total_bytes;
    }

    bool encode_ok = allocation_ok;
    if (encode_ok)
    {
        uint8_t* write_ptr = buffer.data;
        size_t index = 0;
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
    }

    if (params_valid && encode_ok)
    {
        *out_utf8 = buffer;
        res = true;
    }

    return res;
}

static void jsl__cmd_line_reset(JSLCmdLine* cmd_line)
{
    bool params_valid = cmd_line != NULL;

    if (params_valid)
    {
        // clear
        for (int32_t i = 0; i < JSL__CMD_LINE_SHORT_FLAG_BUCKETS; ++i)
        {
            cmd_line->short_flag_bitset[i] = 0;
        }

        cmd_line->arg_list_length = 0;
        cmd_line->arg_list_index = 0;

        jsl_str_to_str_map_clear(&cmd_line->long_flags);
        jsl_str_set_clear(&cmd_line->commands);
        jsl_str_to_str_multimap_clear(&cmd_line->flags_with_values);
    }
}

static bool jsl__cmd_line_ensure_arg_capacity(JSLCmdLine* cmd_line, int64_t capacity_needed)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && cmd_line->arena != NULL
        && capacity_needed > -1
    );

    bool enough = params_valid && capacity_needed <= cmd_line->arg_list_capacity;
    if (enough)
    {
        res = true;
    }

    bool needs_allocation = params_valid && !enough;
    if (needs_allocation)
    {
        bool can_multiply = capacity_needed <= (INT64_MAX / (int64_t) sizeof(JSLFatPtr));
        if (can_multiply)
        {
            int64_t bytes = capacity_needed * (int64_t) sizeof(JSLFatPtr);
            JSLFatPtr allocation = jsl_arena_allocate_aligned(
                cmd_line->arena,
                bytes,
                _Alignof(JSLFatPtr),
                false
            );

            bool allocation_ok = allocation.data != NULL && allocation.length >= bytes;
            if (allocation_ok)
            {
                cmd_line->arg_list = (JSLFatPtr*) allocation.data;
                cmd_line->arg_list_capacity = capacity_needed;
                res = true;
            }
        }
    }

    return res;
}

static bool jsl__cmd_line_copy_arg(JSLCmdLine* cmd_line, JSLFatPtr raw, JSLFatPtr* out_copy)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && cmd_line->arena != NULL
        && out_copy != NULL
        && raw.length > -1
        && (raw.data != NULL || raw.length == 0)
    );

    JSLFatPtr copy = {0};
    bool allocated = false;
    if (params_valid)
    {
        copy = jsl_arena_allocate(cmd_line->arena, raw.length, false);
        allocated = (copy.data != NULL || raw.length == 0) && copy.length >= raw.length;
    }

    if (params_valid && allocated && raw.length > 0)
    {
        JSL_MEMCPY(copy.data, raw.data, (size_t) raw.length);
    }

    if (params_valid && allocated)
    {
        *out_copy = copy;
        res = true;
    }

    return res;
}

static bool jsl__cmd_line_add_command(
    JSLCmdLine* cmd_line,
    JSLFatPtr command,
    int32_t arg_index,
    JSLFatPtr* out_error
)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && cmd_line->arg_list != NULL
        && cmd_line->arg_list_length < cmd_line->arg_list_capacity
        && command.data != NULL
        && command.length > -1
    );

    if (params_valid)
    {
        cmd_line->arg_list[cmd_line->arg_list_length] = command;
        ++cmd_line->arg_list_length;
    }

    bool inserted = params_valid
        && jsl_str_set_insert(
            &cmd_line->commands,
            command,
            JSL_STRING_LIFETIME_STATIC
        );

    if (params_valid && !inserted && out_error != NULL)
    {
        jsl__cmd_line_set_error(
            cmd_line,
            out_error,
            jsl_format(
                cmd_line->arena,
                JSL_FATPTR_EXPRESSION("Unable to store argument %d: %y"),
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

static bool jsl__cmd_line_handle_long_option(
    JSLCmdLine* cmd_line,
    JSLFatPtr arg,
    JSLFatPtr separate_value,
    bool has_separate_value,
    bool* out_consumed_separate,
    int32_t arg_index,
    JSLFatPtr* out_error
)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && arg.data != NULL
        && arg.length > 2
    );

    JSLFatPtr flag_body = {0};
    if (params_valid)
    {
        flag_body = jsl_fatptr_slice(arg, 2, arg.length);
        params_valid = flag_body.data != NULL && flag_body.length > 0;
        if (!params_valid && out_error != NULL)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Expected a flag name after \"--\" in argument %d"),
                    arg_index
                )
            );
        }
    }

    int64_t equals_index = -1;
    if (params_valid)
    {
        equals_index = jsl_fatptr_index_of(flag_body, '=');
        if (equals_index == 0)
        {
            if (out_error != NULL)
            {
                jsl__cmd_line_set_error(
                    cmd_line,
                    out_error,
                    jsl_format(
                        cmd_line->arena,
                        JSL_FATPTR_EXPRESSION("Expected a flag name before '=' in argument %d"),
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
                &cmd_line->long_flags,
                flag_body,
                JSL_STRING_LIFETIME_STATIC,
                JSL__CMD_LINE_EMPTY_VALUE,
                JSL_STRING_LIFETIME_STATIC
            );

            if (out_error != NULL && !inserted)
            {
                jsl__cmd_line_set_error(
                    cmd_line,
                    out_error,
                    jsl_format(
                        cmd_line->arena,
                        JSL_FATPTR_EXPRESSION("Unable to record flag --%y"),
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
        JSLFatPtr key = equals_index > 0
            ? jsl_fatptr_slice(flag_body, 0, equals_index)
            : flag_body;
        JSLFatPtr value = {0};

        if (equals_index > 0)
        {
            value = jsl_fatptr_slice(flag_body, equals_index + 1, flag_body.length);
        }
        else if (has_separate_value)
        {
            value = separate_value;
            consumed_separate = true;
        }

        bool key_valid = key.data != NULL && key.length > -1;
        if (!key_valid && out_error != NULL)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Expected a flag name before value in argument %d"),
                    arg_index
                )
            );
        }
        bool inserted = key_valid
            && jsl_str_to_str_multimap_insert(
                &cmd_line->flags_with_values,
                key,
                JSL_STRING_LIFETIME_STATIC,
                value,
                JSL_STRING_LIFETIME_STATIC
            );

        if (out_error != NULL && key_valid && !inserted)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Unable to store value for --%y"),
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

static bool jsl__cmd_line_handle_short_option(
    JSLCmdLine* cmd_line,
    JSLFatPtr arg,
    int32_t arg_index,
    JSLFatPtr* out_error
)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && arg.data != NULL
        && arg.length > 1
    );

    JSLFatPtr flags = {0};
    if (params_valid)
    {
        flags = jsl_fatptr_slice(arg, 1, arg.length);
        params_valid = flags.data != NULL && flags.length > 0;
    }

    int64_t equals_index = -1;
    if (params_valid)
    {
        equals_index = jsl_fatptr_index_of(flags, '=');
        if (equals_index >= 0)
        {
            if (out_error != NULL)
            {
                jsl__cmd_line_set_error(
                    cmd_line,
                    out_error,
                    jsl_format(
                        cmd_line->arena,
                        JSL_FATPTR_EXPRESSION("Short flags cannot use '=' (argument %d: %y)"),
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
                jsl__cmd_line_set_short_flag(cmd_line, flag_char);
            }
        }

        if (!ascii_valid && out_error != NULL)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Short flags must be ASCII (argument %d: %y)"),
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

static bool jsl__cmd_line_process_arg(
    JSLCmdLine* cmd_line,
    JSLFatPtr stored_arg,
    JSLFatPtr next_arg,
    bool has_next_arg,
    bool* stop_parsing,
    bool* consumed_next,
    int32_t arg_index,
    JSLFatPtr* out_error
)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && stop_parsing != NULL
        && stored_arg.data != NULL
        && stored_arg.length > -1
    );

    if (!params_valid && out_error != NULL && cmd_line != NULL && cmd_line->arena != NULL)
    {
        jsl__cmd_line_set_error(
            cmd_line,
            out_error,
            jsl_format(
                cmd_line->arena,
                JSL_FATPTR_EXPRESSION("Invalid argument at position %d"),
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
        handled = jsl__cmd_line_add_command(cmd_line, stored_arg, arg_index, out_error);
    }

    bool is_long_flag = params_valid
        && !parsing_done
        && stored_arg.length > 2
        && stored_arg.data[0] == '-'
        && stored_arg.data[1] == '-';

    bool consumed_next_arg = false;
    if (is_long_flag)
    {
        handled = jsl__cmd_line_handle_long_option(
            cmd_line,
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
        handled = jsl__cmd_line_handle_short_option(cmd_line, stored_arg, arg_index, out_error);
    }

    bool is_command = params_valid
        && !parsing_done
        && !is_long_flag
        && !is_short_flag;

    if (is_command)
    {
        handled = jsl__cmd_line_add_command(cmd_line, stored_arg, arg_index, out_error);
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
        jsl__cmd_line_set_error(
            cmd_line,
            out_error,
            jsl_format(
                cmd_line->arena,
                JSL_FATPTR_EXPRESSION("Could not parse argument %d: %y"),
                arg_index,
                stored_arg
            )
        );
    }

    return res;
}

typedef bool (*JSL__CmdLinePrepareArg)(
    JSLCmdLine* cmd_line,
    void* argv,
    int32_t index,
    JSLFatPtr* out_arg,
    JSLFatPtr* out_error
);

typedef bool (*JSL__CmdLineIsFlag)(void* argv, int32_t index);

static bool jsl__cmd_line_is_narrow_flag(void* argv, int32_t index)
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

static bool jsl__cmd_line_is_wide_flag(void* argv, int32_t index)
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

static bool jsl__cmd_line_prepare_utf8_arg(
    JSLCmdLine* cmd_line,
    void* argv,
    int32_t index,
    JSLFatPtr* out_arg,
    JSLFatPtr* out_error
)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && argv != NULL
        && out_arg != NULL
    );

    JSLFatPtr raw = {0};
    if (params_valid)
    {
        char** args = (char**) argv;
        raw = jsl_fatptr_from_cstr(args[index]);
        params_valid = raw.data != NULL;
        if (!params_valid && out_error != NULL)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Argument %d is missing"),
                    index
                )
            );
        }
    }

    bool utf8_ok = params_valid && jsl__cmd_line_validate_utf8(raw);
    if (params_valid && !utf8_ok && out_error != NULL)
    {
        jsl__cmd_line_set_error(
            cmd_line,
            out_error,
            jsl_format(
                cmd_line->arena,
                JSL_FATPTR_EXPRESSION("Argument %d is not valid UTF-8"),
                index
            )
        );
    }

    JSLFatPtr stored = {0};
    bool copied = utf8_ok && jsl__cmd_line_copy_arg(cmd_line, raw, &stored);
    if (utf8_ok && !copied && out_error != NULL)
    {
        jsl__cmd_line_set_error(
            cmd_line,
            out_error,
            jsl_format(
                cmd_line->arena,
                JSL_FATPTR_EXPRESSION("Unable to store argument %d"),
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

static bool jsl__cmd_line_prepare_wide_arg(
    JSLCmdLine* cmd_line,
    void* argv,
    int32_t index,
    JSLFatPtr* out_arg,
    JSLFatPtr* out_error
)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && argv != NULL
        && out_arg != NULL
    );

    JSLFatPtr utf8_arg = {0};
    wchar_t* wide_arg = NULL;
    if (params_valid)
    {
        wchar_t** args = (wchar_t**) argv;
        wide_arg = args[index];
        params_valid = wide_arg != NULL;
        if (!params_valid && out_error != NULL)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Argument %d is missing"),
                    index
                )
            );
        }
    }

    if (params_valid)
    {
        bool converted = jsl__cmd_line_utf16_to_utf8(cmd_line->arena, wide_arg, &utf8_arg)
            && jsl__cmd_line_validate_utf8(utf8_arg);

        if (!converted && out_error != NULL)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Argument %d is not valid UTF-16"),
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

static bool jsl__cmd_line_parse_common(
    JSLCmdLine* cmd_line,
    int32_t argc,
    void* argv,
    JSL__CmdLinePrepareArg prepare_arg,
    JSL__CmdLineIsFlag is_flag_like,
    JSLFatPtr* out_error
)
{
    bool res = false;

    if (out_error != NULL)
    {
        *out_error = (JSLFatPtr) {0};
    }

    bool params_valid = (
        cmd_line != NULL
        && cmd_line->arena != NULL
        && argv != NULL
        && argc > -1
        && prepare_arg != NULL
        && is_flag_like != NULL
    );

    if (!params_valid)
    {
        return false;
    }

    jsl__cmd_line_reset(cmd_line);

    int64_t capacity_needed = (int64_t) argc;
    bool capacity_ok = jsl__cmd_line_ensure_arg_capacity(cmd_line, capacity_needed);

    if (!capacity_ok)
    {
        if (out_error != NULL)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Command line input exceeds memory limit")
                )
            );
        }
        return false;
    }

    bool parse_ok = true;
    bool stop_parsing = false;

    for (int32_t index = 1; parse_ok && index < argc; ++index)
    {
        JSLFatPtr stored = {0};
        parse_ok = prepare_arg(cmd_line, argv, index, &stored, out_error);

        bool has_next_raw = parse_ok && (index + 1) < argc;
        bool next_is_flag = has_next_raw && is_flag_like(argv, index + 1);

        bool should_prepare_next = parse_ok
            && !stop_parsing
            && has_next_raw
            && stored.length > 2
            && stored.data[0] == '-'
            && stored.data[1] == '-'
            && jsl_fatptr_index_of(stored, '=') < 0
            && !next_is_flag;

        JSLFatPtr next_stored = {0};
        bool next_available = false;
        if (should_prepare_next)
        {
            next_available = prepare_arg(cmd_line, argv, index + 1, &next_stored, out_error);
            parse_ok = next_available;
        }

        bool consumed_next = false;
        bool processed = parse_ok
            && jsl__cmd_line_process_arg(
                cmd_line,
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
        jsl__cmd_line_reset(cmd_line);
        if (out_error != NULL && out_error->data == NULL)
        {
            jsl__cmd_line_set_error(
                cmd_line,
                out_error,
                jsl_format(
                    cmd_line->arena,
                    JSL_FATPTR_EXPRESSION("Failed to parse command line input")
                )
            );
        }
    }

    return res;
}

bool jsl_cmd_line_init(JSLCmdLine* cmd_line, JSLArena* arena)
{
    bool res = false;

    bool params_valid = cmd_line != NULL && arena != NULL;

    if (params_valid)
    {
        JSL_MEMSET(cmd_line, 0, sizeof(JSLCmdLine));
        cmd_line->arena = arena;

        bool long_init = jsl_str_to_str_map_init(&cmd_line->long_flags, arena, 0);
        bool multimap_init = jsl_str_to_str_multimap_init(&cmd_line->flags_with_values, arena, 0);
        bool commands_init = jsl_str_set_init(&cmd_line->commands, arena, 0);

        if (long_init && multimap_init && commands_init)
        {
            res = true;
        }
    }

    return res;
}

bool jsl_cmd_line_parse(JSLCmdLine* cmd_line, int32_t argc, char** argv, JSLFatPtr* out_error)
{
    return jsl__cmd_line_parse_common(
        cmd_line,
        argc,
        (void*) argv,
        jsl__cmd_line_prepare_utf8_arg,
        jsl__cmd_line_is_narrow_flag,
        out_error
    );
}

bool jsl_cmd_line_parse_wide(JSLCmdLine* cmd_line, int32_t argc, wchar_t** argv, JSLFatPtr* out_error)
{
    return jsl__cmd_line_parse_common(
        cmd_line,
        argc,
        (void*) argv,
        jsl__cmd_line_prepare_wide_arg,
        jsl__cmd_line_is_wide_flag,
        out_error
    );
}

bool jsl_cmd_line_has_short_flag(JSLCmdLine* cmd_line, uint8_t flag)
{
    bool res = false;

    bool params_valid = cmd_line != NULL;
    if (params_valid)
    {
        res = jsl__cmd_line_short_flag_present(cmd_line, flag);
    }

    return res;
}

bool jsl_cmd_line_has_flag(JSLCmdLine* cmd_line, JSLFatPtr flag)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && flag.data != NULL
        && flag.length > -1
    );

    if (params_valid)
    {
        res = jsl_str_to_str_map_has_key(&cmd_line->long_flags, flag);
    }

    return res;
}

bool jsl_cmd_line_has_command(JSLCmdLine* cmd_line, JSLFatPtr flag)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && flag.data != NULL
        && flag.length > -1
    );

    if (params_valid)
    {
        res = jsl_str_set_has(&cmd_line->commands, flag);
    }

    return res;
}

bool jsl_cmd_line_pop_arg_list(JSLCmdLine* cmd_line, JSLFatPtr* out_value)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && out_value != NULL
        && cmd_line->arg_list != NULL
        && cmd_line->arg_list_index < cmd_line->arg_list_length
    );

    if (params_valid)
    {
        *out_value = cmd_line->arg_list[cmd_line->arg_list_index];
        ++cmd_line->arg_list_index;
        res = true;
    }

    return res;
}

bool jsl_cmd_line_pop_flag_with_value(
    JSLCmdLine* cmd_line,
    JSLFatPtr flag,
    JSLFatPtr* out_value
)
{
    bool res = false;

    bool params_valid = (
        cmd_line != NULL
        && out_value != NULL
        && flag.data != NULL
        && flag.length > -1
    );

    JSLStrToStrMultimapValueIter iter;
    bool iterator_init = params_valid
        && jsl_str_to_str_multimap_get_values_for_key_iterator_init(
            &cmd_line->flags_with_values,
            &iter,
            flag
        );

    JSLFatPtr value = {0};
    bool has_value = iterator_init
        && jsl_str_to_str_multimap_get_values_for_key_iterator_next(
            &iter,
            &value
        );

    bool copied = false;
    if (has_value)
    {
        copied = jsl__cmd_line_copy_arg(cmd_line, value, out_value);
    }

    if (copied)
    {
        jsl_str_to_str_multimap_delete_value(
            &cmd_line->flags_with_values,
            flag,
            value
        );
        res = true;
    }

    return res;
}

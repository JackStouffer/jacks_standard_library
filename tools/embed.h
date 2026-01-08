/**
 * TODO: docs
 * 
 * ## License
 *
 * YOUR LICENSE HERE 
 */


#ifndef EMBED_H_INCLUDED
    #define EMBED_H_INCLUDED

    #include <stdint.h>
    #include <stddef.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "../src/jsl_core.h"
    #include "../src/jsl_string_builder.h"

    /* Versioning to catch mismatches across deps */
    #ifndef EMBED_VERSION
        #define EMBED_VERSION 0x010000  /* 1.0.0 */
    #else
        #if EMBED_VERSION != 0x010000
            #error "embed.h version mismatch across includes"
        #endif
    #endif

    #ifndef EMBED_DEF
        #define EMBED_DEF
    #endif

    #ifdef __cplusplus
    extern "C" {
    #endif

    typedef enum EmbedOuputTypeEnum {
        OUTPUT_TYPE_INVALID = 0,
        OUTPUT_TYPE_BINARY = 1,
        OUTPUT_TYPE_TEXT = 2
    } EmbedOuputTypeEnum;

    EMBED_DEF bool generate_embed_header(
        JSLStringBuilder* builder,
        JSLFatPtr variable_name,
        JSLFatPtr file_data,
        EmbedOuputTypeEnum output_type
    );

    #ifdef __cplusplus
    }
    #endif
    
#endif /* EMBED_H_INCLUDED */

#ifdef EMBED_IMPLEMENTATION

    bool generate_embed_header(
        JSLStringBuilder* builder,
        JSLFatPtr variable_name,
        JSLFatPtr file_data,
        EmbedOuputTypeEnum output_type
    )
    {
        JSL_ASSERT(output_type != OUTPUT_TYPE_INVALID);

        jsl_string_builder_insert_cstr(builder, "#pragma once\n\n");
        jsl_string_builder_insert_cstr(builder, "#include <stdint.h>\n\n");
        jsl_string_builder_insert_cstr(builder, "#include \"jsl_core.h\"\n\n");

        if (output_type == OUTPUT_TYPE_BINARY)
        {
            jsl_string_builder_format(
                builder,
                JSL_FATPTR_EXPRESSION("static uint8_t __%y_data[] = {\n"),
                variable_name
            );

            const int32_t bytes_per_line = 12;
            for (int64_t i = 0; i < file_data.length; ++i)
            {
                if (i % bytes_per_line == 0)
                    jsl_string_builder_insert_cstr(builder, "    ");

                jsl_string_builder_format(
                    builder,
                    JSL_FATPTR_EXPRESSION("0x%02x"),
                    file_data.data[i]
                );

                bool is_last = (i + 1) == file_data.length;
                bool end_of_line = ((i + 1) % bytes_per_line) == 0;

                if (!is_last)
                    jsl_string_builder_insert_u8(builder, ',');

                if (end_of_line || is_last)
                    jsl_string_builder_insert_u8(builder, '\n');
                else
                    jsl_string_builder_insert_u8(builder, ' ');
            }

            jsl_string_builder_insert_cstr(builder, "};\n\n");

            jsl_string_builder_format(
                builder,
                JSL_FATPTR_EXPRESSION("static JSLFatPtr %y = { __%y_data, %lld };\n\n"),
                variable_name,
                variable_name,
                (long long) file_data.length
            );
        }
        else if (output_type == OUTPUT_TYPE_TEXT)
        {
            jsl_string_builder_format(
                builder,
                JSL_FATPTR_EXPRESSION("static JSLFatPtr %y = JSL_FATPTR_INITIALIZER(\n"),
                variable_name
            );

            bool string_open = false;
            if (file_data.length > 0)
            {
                jsl_string_builder_insert_u8(builder, '"');
                string_open = true;

                for (int64_t i = 0; i < file_data.length; ++i)
                {
                    uint8_t c = file_data.data[i];

                    if (c == '\n')
                    {
                        jsl_string_builder_insert_cstr(builder, "\\n\"");
                        string_open = false;
                        jsl_string_builder_insert_u8(builder, '\n');

                        if ((i + 1) < file_data.length)
                        {
                            jsl_string_builder_insert_u8(builder, '"');
                            string_open = true;
                        }

                        continue;
                    }

                    switch (c)
                    {
                        case '\\':
                            jsl_string_builder_insert_cstr(builder, "\\\\");
                            break;
                        case '\"':
                            jsl_string_builder_insert_cstr(builder, "\\\"");
                            break;
                        case '\r':
                            jsl_string_builder_insert_cstr(builder, "\\r");
                            break;
                        case '\t':
                            jsl_string_builder_insert_cstr(builder, "\\t");
                            break;
                        default:
                            jsl_string_builder_insert_u8(builder, c);
                            break;
                    }
                }

                if (string_open)
                {
                    jsl_string_builder_insert_u8(builder, '"');
                    jsl_string_builder_insert_u8(builder, '\n');
                }
            }

            jsl_string_builder_insert_cstr(builder, ");\n\n");

        }

        return true;
    }

#endif /* EMBED_IMPLEMENTATION */

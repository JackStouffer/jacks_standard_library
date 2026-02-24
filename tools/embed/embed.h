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

    #include "jsl/core.h"
    #include "jsl/string_builder.h"

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

    /**
     * TODO: docs
     */
    EMBED_DEF bool generate_embed_header(
        JSLOutputSink sink,
        JSLImmutableMemory variable_name,
        JSLImmutableMemory file_data,
        EmbedOuputTypeEnum output_type
    );

    #ifdef __cplusplus
    }
    #endif
    
#endif /* EMBED_H_INCLUDED */

#ifdef EMBED_IMPLEMENTATION

    bool generate_embed_header(
        JSLOutputSink sink,
        JSLImmutableMemory variable_name,
        JSLImmutableMemory file_data,
        EmbedOuputTypeEnum output_type
    )
    {
        JSL_ASSERT(output_type != OUTPUT_TYPE_INVALID);

        jsl_output_sink_write_cstr(sink, "#pragma once\n\n");
        jsl_output_sink_write_cstr(sink, "#include <stdint.h>\n\n");
        jsl_output_sink_write_cstr(sink, "#include \"jsl/core.h\"\n\n");

        if (output_type == OUTPUT_TYPE_BINARY)
        {
            jsl_format_sink(
                sink,
                JSL_CSTR_EXPRESSION("static uint8_t __%y_data[] = {\n"),
                variable_name
            );

            const int32_t bytes_per_line = 12;
            for (int64_t i = 0; i < file_data.length; ++i)
            {
                if (i % bytes_per_line == 0)
                    jsl_output_sink_write_cstr(sink, "    ");

                jsl_format_sink(
                    sink,
                    JSL_CSTR_EXPRESSION("0x%02x"),
                    file_data.data[i]
                );

                bool is_last = (i + 1) == file_data.length;
                bool end_of_line = ((i + 1) % bytes_per_line) == 0;

                if (!is_last)
                    jsl_output_sink_write_u8(sink, ',');

                if (end_of_line || is_last)
                    jsl_output_sink_write_u8(sink, '\n');
                else
                    jsl_output_sink_write_u8(sink, ' ');
            }

            jsl_output_sink_write_cstr(sink, "};\n\n");

            jsl_format_sink(
                sink,
                JSL_CSTR_EXPRESSION("static JSLImmutableMemory %y = { __%y_data, %lld };\n\n"),
                variable_name,
                variable_name,
                (long long) file_data.length
            );
        }
        else if (output_type == OUTPUT_TYPE_TEXT)
        {
            jsl_format_sink(
                sink,
                JSL_CSTR_EXPRESSION("static JSLImmutableMemory %y = JSL_CSTR_INITIALIZER(\n"),
                variable_name
            );

            bool string_open = false;
            if (file_data.length > 0)
            {
                jsl_output_sink_write_u8(sink, '"');
                string_open = true;

                for (int64_t i = 0; i < file_data.length; ++i)
                {
                    uint8_t c = file_data.data[i];

                    if (c == '\n')
                    {
                        jsl_output_sink_write_cstr(sink, "\\n\"");
                        string_open = false;
                        jsl_output_sink_write_u8(sink, '\n');

                        if ((i + 1) < file_data.length)
                        {
                            jsl_output_sink_write_u8(sink, '"');
                            string_open = true;
                        }

                        continue;
                    }

                    switch (c)
                    {
                        case '\\':
                            jsl_output_sink_write_cstr(sink, "\\\\");
                            break;
                        case '\"':
                            jsl_output_sink_write_cstr(sink, "\\\"");
                            break;
                        case '\r':
                            jsl_output_sink_write_cstr(sink, "\\r");
                            break;
                        case '\t':
                            jsl_output_sink_write_cstr(sink, "\\t");
                            break;
                        default:
                            jsl_output_sink_write_u8(sink, c);
                            break;
                    }
                }

                if (string_open)
                {
                    jsl_output_sink_write_u8(sink, '"');
                    jsl_output_sink_write_u8(sink, '\n');
                }
            }

            jsl_output_sink_write_cstr(sink, ");\n\n");

        }

        return true;
    }

#endif /* EMBED_IMPLEMENTATION */

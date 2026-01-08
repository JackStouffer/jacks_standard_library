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

    EMBED_DEF bool generate_embed_header(
        JSLStringBuilder* builder,
        JSLFatPtr variable_name,
        JSLFatPtr file_data
    );

    #ifdef __cplusplus
    }
    #endif
    
#endif /* EMBED_H_INCLUDED */

#ifdef EMBED_IMPLEMENTATION

    bool generate_embed_header(
        JSLStringBuilder* builder,
        JSLFatPtr variable_name,
        JSLFatPtr file_data
    )
    {
        jsl_string_builder_insert_cstr(builder, "#pragma once\n\n");
        jsl_string_builder_insert_cstr(builder, "#include <stdint.h>\n\n");
        jsl_string_builder_insert_cstr(builder, "#include \"jsl_core.h\"\n\n");

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

        return true;
    }

#endif /* EMBED_IMPLEMENTATION */

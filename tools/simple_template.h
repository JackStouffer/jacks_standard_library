/**
 * # My Lib
 * 
 * Simple boilerplate for a single header lib that you can copy/paste
 * to get started.
 * 
 * ## License
 *
 * YOUR LICENSE HERE 
 */


#ifndef SIMPLE_TEMPLATE_H_INCLUDED
    #define SIMPLE_TEMPLATE_H_INCLUDED

    #include <stdint.h>
    #include <stddef.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    #include "jsl_core.h"
    #include "jsl_string_builder.h"
    #include "jsl_str_to_str_map.h"


    /* Versioning to catch mismatches across deps */
    #ifndef SIMPLE_TEMPLATE_VERSION
        #define SIMPLE_TEMPLATE_VERSION 0x010000  /* 1.0.0 */
    #else
        #if SIMPLE_TEMPLATE_VERSION != 0x010000
            #error "simple_template.h version mismatch across includes"
        #endif
    #endif

    #ifndef SIMPLE_TEMPLATE_DEF
        #define SIMPLE_TEMPLATE_DEF
    #endif

    #ifdef __cplusplus
    extern "C" {
    #endif


    SIMPLE_TEMPLATE_DEF void render_template(
        JSLStringBuilder* str_builder,
        JSLFatPtr template,
        JSLStrToStrMap* variables
    );

    
    #ifdef __cplusplus
    }
    #endif
    
#endif /* SIMPLE_TEMPLATE_H_INCLUDED */

#ifdef SIMPLE_TEMPLATE_IMPLEMENTATION

    SIMPLE_TEMPLATE_DEF void render_template(
        JSLStringBuilder* str_builder,
        JSLFatPtr template,
        JSLStrToStrMap* variables
    )
    {
        static JSLFatPtr open_param = JSL_FATPTR_INITIALIZER("{{");
        static JSLFatPtr close_param = JSL_FATPTR_INITIALIZER("}}");
        JSLFatPtr template_reader = template;
        
        while (template_reader.length > 0)
        {
            int64_t index_of_open = jsl_fatptr_substring_search(template_reader, open_param);
            int64_t index_of_close = jsl_fatptr_substring_search(template_reader, close_param);

            // No more variables, write everything
            if (index_of_open == -1)
            {
                jsl_string_builder_insert_fatptr(str_builder, template_reader);
                JSL_FATPTR_ADVANCE(template_reader, template_reader.length);
            }
            // Improperly closed template param, write everything
            else if (index_of_open > -1 && index_of_close == -1)
            {
                jsl_string_builder_insert_fatptr(str_builder, template_reader);
                JSL_FATPTR_ADVANCE(template_reader, template_reader.length);
            }
            // Close before open, write everything up to the next open
            else if (index_of_open > -1 && index_of_close > -1 && index_of_close < index_of_open)
            {
                JSLFatPtr slice = jsl_fatptr_slice(template_reader, 0, index_of_open);
                jsl_string_builder_insert_fatptr(str_builder, slice);
                JSL_FATPTR_ADVANCE(template_reader, index_of_open);
            }
            // properly formed
            else if (index_of_open > -1 && index_of_close > -1 && index_of_close > index_of_open)
            {
                JSLFatPtr slice = jsl_fatptr_slice(template_reader, 0, index_of_open);
                jsl_string_builder_insert_fatptr(str_builder, slice);
                JSL_FATPTR_ADVANCE(template_reader, index_of_open + 2);

                JSLFatPtr var_name = jsl_fatptr_slice(template_reader, 0, index_of_close);
                jsl_fatptr_strip_whitespace(&var_name);

                JSLFatPtr var_value;
                if (jsl_str_to_str_map_get(variables, var_name, &var_value))
                {
                    jsl_string_builder_insert_fatptr(str_builder, var_value);
                }

                JSL_FATPTR_ADVANCE(template_reader, index_of_close + 2);
            }
        }
    }

#endif /* SIMPLE_TEMPLATE_IMPLEMENTATION */

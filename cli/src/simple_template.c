#include "jsl_core.h"
#include "jsl_string_builder.h"
#include "jsl_str_to_str_map.h"

void render_template(
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

        // No more variables, write everything then break
        if (index_of_open == -1)
        {
            jsl_string_builder_insert_fatptr(str_builder, template_reader);
            break;
        }
        // Improperly closed template param, write everything then break
        else if (index_of_open > -1 && index_of_close == -1)
        {
            jsl_string_builder_insert_fatptr(str_builder, template_reader);
            break;
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

            
        }
    }
}

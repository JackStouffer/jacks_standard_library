/**
 * # Embed File Data
 * 
 * TODO: docs
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#define EMBED_IMPLEMENTATION
#include "embed.h"

#define JSL_CORE_IMPLEMENTATION
#include "../../src/jsl_core.h"

#define JSL_STRING_BUILDER_IMPLEMENTATION
#include "../../src/jsl_string_builder.h"

#define JSL_FILES_IMPLEMENTATION
#include "../../src/jsl_files.h"

int32_t main(int32_t argc, char **argv)
{
    JSLArena arena;
    jsl_arena_init(&arena, malloc(JSL_MEGABYTES(32)), JSL_MEGABYTES(32));

    JSLFatPtr header_template_path = JSL_FATPTR_EXPRESSION("tools/src/templates/static_hash_map_header.txt");
    JSLFatPtr header_output_path = JSL_FATPTR_EXPRESSION("tools/src/templates/static_hash_map_header.h");

    JSLFatPtr header_template_contents = {0};
    int32_t load_errno = 0;
    JSLLoadFileResultEnum load_result = jsl_load_file_contents(
        &arena,
        header_template_path,
        &header_template_contents,
        &load_errno
    );

    if (load_result != JSL_FILE_LOAD_SUCCESS)
    {
        jsl_format_file(
            stderr,
            JSL_FATPTR_EXPRESSION("Failed to load template file %y (errno %d)\n"),
            header_template_path,
            load_errno
        );
        exit(EXIT_FAILURE);
    }

    JSLStringBuilder builder;
    jsl_string_builder_init(&builder, &arena);

    generate_embed_header(
        &builder,
        JSL_FATPTR_EXPRESSION("test"),
        header_template_contents
    );
    
    JSLStringBuilderIterator iterator;
    jsl_string_builder_iterator_init(&builder, &iterator);

    while (true)
    {
        JSLFatPtr slice = jsl_string_builder_iterator_next(&iterator);
        if (slice.data == NULL)
            break;

        jsl_format_file(stdout, slice);
    }

    return EXIT_SUCCESS;
}

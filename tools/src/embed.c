/**
 * # Embed File Data
 * 
 * Every large, serious C program wants to take file data and include
 * it in binary. Unfortunately it took until C23 for this to be standardized
 * in the language. This program takes the binary representation of a
 * file and generates the text of a C header with that info as a fat pointer.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#define EMBED_IMPLEMENTATION
#include "embed.h"

#include "../../src/jsl_core.c"
#include "../../src/jsl_string_builder.c"
#include "../../src/jsl_os.c"

int32_t main(int32_t argc, char **argv)
{
    JSLArena arena;
    jsl_arena_init(&arena, malloc(JSL_MEGABYTES(32)), JSL_MEGABYTES(32));

    JSLFatPtr file_path = {0};
    JSLFatPtr file_contents = {0};
    int32_t load_errno = 0;

    if (argc == 2)
    {
        file_path = jsl_fatptr_from_cstr(argv[1]);

        JSLLoadFileResultEnum load_result = jsl_load_file_contents(
            &arena,
            file_path,
            &file_contents,
            &load_errno
        );

        if (load_result != JSL_FILE_LOAD_SUCCESS)
        {
            jsl_format_file(
                stderr,
                JSL_FATPTR_EXPRESSION("Failed to load template file %y (errno %d)\n"),
                file_contents,
                load_errno
            );
            exit(EXIT_FAILURE);
        }

        JSLStringBuilder builder;
        jsl_string_builder_init(&builder, &arena);

        generate_embed_header(
            &builder,
            JSL_FATPTR_EXPRESSION("test"),
            file_contents
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
    else if (argc == 1)
    {
        jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Must provide a file"));
        return EXIT_FAILURE;
    }
    else
    {
        jsl_format_file(stderr, JSL_FATPTR_EXPRESSION("Only one file path allowed"));
        return EXIT_FAILURE;
    }
}

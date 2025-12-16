/**
 * Copyright (c) 2025 Jack Stouffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define JSL_CORE_IMPLEMENTATION
#include "../src/jsl_core.h"

#define JSL_STR_TO_STR_MAP_IMPLEMENTATION
#include "../src/jsl_str_to_str_map.h"

#include "minctest.h"

JSLArena global_arena;


int main(void)
{
    srand((unsigned) time(NULL));

    // Windows programs that crash can lose all of the terminal output.
    // Set the buffer to zero to auto flush on output.
    #if JSL_IS_WINDOWS
        setvbuf(stdout, NULL, _IONBF, 0);
    #endif

    jsl_arena_init(&global_arena, malloc(JSL_MEGABYTES(32)), JSL_MEGABYTES(32));



    TEST_RESULTS();
    return lfails != 0;
}

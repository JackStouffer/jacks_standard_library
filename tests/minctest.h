/*
 *
 * MINCTEST - Minimal C Test Library - 0.3.0
 *
 * Copyright (c) 2014-2021 Lewis Van Winkle
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */



/*
 * MINCTEST - Minimal testing library for C
 *
 *
 * Example:
 *
 *      void test1() {
 *           lok('a' == 'a');
 *      }
 *
 *      void test2() {
 *           lequal(5, 6);
 *           lfequal(5.5, 5.6);
 *      }
 *
 *      int main() {
 *           lrun("test1", test1);
 *           lrun("test2", test2);
 *           lresults();
 *           return lfails != 0;
 *      }
 *
 *
 *
 * Hints:
 *      All functions/variables start with the letter 'l'.
 *
 */


#ifndef __MINCTEST_H__
#define __MINCTEST_H__

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>


/* How far apart can floats be before we consider them unequal. */
#ifndef LTEST_FLOAT_TOLERANCE
#define LTEST_FLOAT_TOLERANCE 0.001
#endif


/* Track the number of passes, fails. */
/* NB this is made for all tests to be in one file. */
static size_t ltests = 0;
static size_t lfails = 0;


/* Display the test results. */
#define lresults() do {\
    if (lfails == 0) {\
        printf("ALL TESTS PASSED (%zu/%zu)\n", ltests, ltests);\
    } else {\
        printf("SOME TESTS FAILED (%zu/%zu)\n", ltests-lfails, ltests);\
    }\
} while (0)


/* Run a test. Name can be any string to print out, test is the function name to call. */
#define lrun(name, test) do {\
    const size_t ts = ltests;\
    const size_t fs = lfails;\
    const clock_t start = clock();\
    printf("\t%s:\n", name);\
    test();\
    printf("\t -- pass: %-20zu fail: %-20zu time: %ldms\n",\
            (ltests-ts)-(lfails-fs), lfails-fs,\
            (long)((clock() - start) * 1000 / CLOCKS_PER_SEC));\
} while (0)


/* Assert a true statement. */
#define lok(test) do {\
    ++ltests;\
    if (!(test)) {\
        ++lfails;\
        printf("%s:%d error \n", __FILE__, __LINE__);\
    }} while (0)


/* Prototype to assert equal. */
#define lequal_base(equality, a, b, format) do {\
    ++ltests;\
    if (!(equality)) {\
        ++lfails;\
        printf("%s:%d ("format " != " format")\n", __FILE__, __LINE__, (a), (b));\
    }} while (0)


/* Assert two integers are equal. */
#define lequal(a, b)\
    lequal_base((a) == (b), a, b, "%d")


/* Assert two int64 are equal. */
#define l_long_long_equal(a, b)\
    lequal_base((a) == (b), a, b, "%" PRId64)


/* Assert two floats are equal (Within LTEST_FLOAT_TOLERANCE). */
#define lfequal(a, b)\
    lequal_base(fabs((double)(a)-(double)(b)) <= LTEST_FLOAT_TOLERANCE\
     && fabs((double)(a)-(double)(b)) == fabs((double)(a)-(double)(b)), (double)(a), (double)(b), "%f")


/* Assert two strings are equal. */
#define lsequal(a, b)\
    lequal_base(strcmp(a, b) == 0, a, b, "%s")

#define lmemcmp(buf_a, buf_b, buf_len) do {\
    ++ltests;\
    const unsigned char *const _lm_a = (const unsigned char *)(buf_a);\
    const unsigned char *const _lm_b = (const unsigned char *)(buf_b);\
    const size_t _lm_len = (buf_len);\
    size_t _lm_i;\
    for (_lm_i = 0; _lm_i < _lm_len; ++_lm_i) {\
        const unsigned char _lm_va = _lm_a[_lm_i];\
        const unsigned char _lm_vb = _lm_b[_lm_i];\
        if (_lm_va != _lm_vb) {\
            ++lfails;\
            printf("%s:%d buffers differ at byte %zu/%zu (0x%02X '%c' != 0x%02X '%c')\n",\
                   __FILE__, __LINE__, _lm_i, _lm_len,\
                   (unsigned)_lm_va, (_lm_va >= 32 && _lm_va <= 126) ? _lm_va : '.',\
                   (unsigned)_lm_vb, (_lm_vb >= 32 && _lm_vb <= 126) ? _lm_vb : '.');\
            break;\
        }\
    }\
} while (0)

#endif /*__MINCTEST_H__*/

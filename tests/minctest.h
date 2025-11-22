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
 *           TEST_BOOL('a' == 'a');
 *      }
 *
 *      void test2() {
 *           TEST_INT32_EQUAL(5, 6);
 *           TEST_F32_EQUAL(5.5, 5.6);
 *      }
 *
 *      int main() {
 *           RUN_TEST_FUNCTION("test1", test1);
 *           RUN_TEST_FUNCTION("test2", test2);
 *           TEST_RESULTS();
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
#ifndef TESTING_FLOAT_TOLERANCE
#define TESTING_FLOAT_TOLERANCE 0.001
#endif


/* Track the number of passes, fails. */
/* NB this is made for all tests to be in one file. */
static size_t ltests = 0;
static size_t lfails = 0;


/* Display the test results. */
#define TEST_RESULTS() do {\
    if (lfails == 0) {\
        printf("ALL TESTS PASSED (%zu/%zu)\n", ltests, ltests);\
    } else {\
        printf("SOME TESTS FAILED (%zu/%zu)\n", ltests-lfails, ltests);\
    }\
} while (0)


/* Run a test. Name can be any string to print out, test is the function name to call. */
#define RUN_TEST_FUNCTION(name, test) do {\
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
#define TEST_BOOL(test) do {\
    ++ltests;\
    if (!(test)) {\
        ++lfails;\
        printf("%s:%d error \n", __FILE__, __LINE__);\
    }} while (0)


/* Prototype to assert equal. */
#define TEST_INT32_EQUAL_base(equality, a, b, format) do {\
    ++ltests;\
    if (!(equality)) {\
        ++lfails;\
        printf("%s:%d ("format " != " format")\n", __FILE__, __LINE__, (a), (b));\
    }} while (0)

#define TEST_INT32_EQUAL(a, b)\
    TEST_INT32_EQUAL_base((a) == (b), a, b, "%d")

#define TEST_UINT32_EQUAL(a, b)\
    TEST_INT32_EQUAL_base((a) == (b), a, b, "%u")

#define TEST_INT64_EQUAL(a, b)\
    TEST_INT32_EQUAL_base((a) == (b), a, b, "%" PRId64)

#define TEST_UINT64_EQUAL(a, b)\
    TEST_INT32_EQUAL_base((a) == (b), a, b, "%" PRIu64)

/* Assert two floats are equal (Within TESTING_FLOAT_TOLERANCE). */
#define TEST_F32_EQUAL(a, b)\
    TEST_INT32_EQUAL_base(fabs((double)(a)-(double)(b)) <= TESTING_FLOAT_TOLERANCE\
     && fabs((double)(a)-(double)(b)) == fabs((double)(a)-(double)(b)), (double)(a), (double)(b), "%f")


#define TEST_BUFFERS_EQUAL(buf_a, buf_b, buf_len) do {\
    ++ltests;\
    const unsigned char *const _lm_a = (const unsigned char *)(buf_a);\
    const unsigned char *const _lm_b = (const unsigned char *)(buf_b);\
    const size_t _lm_len = (size_t) (buf_len);\
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

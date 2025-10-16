/*
MIT License

Copyright (c) 2017 Sean Barrett

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "minctest.h"

#define JSL_IMPLEMENTATION
#include "../src/jacks_standard_library.h"

#define CHECK_END(str)                                                              \
    JSLFatPtr written = jsl_fatptr_slice(buffer, 0, ret);                           \
    lok(jsl_fatptr_cstr_compare(written, str) && ret == (int64_t) strlen(str))

#define CHECK9(str, v1, v2, v3, v4, v5, v6, v7, v8, v9) \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str, v2, v3, v4, v5, v6, v7, v8, v9);   \
    CHECK_END(str);                                                                 \
}

#define CHECK8(str, v1, v2, v3, v4, v5, v6, v7, v8)                                 \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str, v2, v3, v4, v5, v6, v7, v8);       \
    CHECK_END(str);                                                                 \
}

#define CHECK7(str, v1, v2, v3, v4, v5, v6, v7)                                     \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str, v2, v3, v4, v5, v6, v7);    \
    CHECK_END(str);                                                                 \
}

#define CHECK6(str, v1, v2, v3, v4, v5, v6)                                         \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str, v2, v3, v4, v5, v6);        \
    CHECK_END(str);                                                                 \
}

#define CHECK5(str, v1, v2, v3, v4, v5)                                             \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str, v2, v3, v4, v5);            \
    CHECK_END(str);                                                                 \
}

#define CHECK4(str, v1, v2, v3, v4)                                                 \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str, v2, v3, v4);                \
    CHECK_END(str);                                                                 \
}

#define CHECK3(str, v1, v2, v3)                                                     \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str, v2, v3);                    \
    CHECK_END(str);                                                                 \
}

#define CHECK2(str, v1, v2)                                                         \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str, v2);                        \
    CHECK_END(str);                                                                 \
}

#define CHECK1(str, v1)                                                             \
{                                                                                   \
    JSLFatPtr writer = buffer;                                                      \
    JSLFatPtr fmt_str = jsl_fatptr_from_cstr(v1);                                   \
    int64_t ret = jsl_format_buffer(&writer, fmt_str);                            \
    CHECK_END(str);                                                                 \
}


void test_integers(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    CHECK4("a b     1", "%c %s     %d", 'a', "b", 1);
    CHECK4("This is a very long string which will call SIMD code for sure a b     1", "This is a very long string which will call SIMD code for sure %c %s     %d", 'a', "b", 1);
    CHECK2("abc     ", "%-8.3s", "abcdefgh");
    CHECK2("+5", "%+2d", 5);
    CHECK2("  6", "% 3i", 6);
    CHECK2("-7  ", "%-4d", -7);
    CHECK2("+0", "%+d", 0);
    CHECK3("     00003:     00004", "%10.5d:%10.5d", 3, 4);
    CHECK2("-100006789", "%d", -100006789);
    CHECK3("20 0020", "%u %04u", 20u, 20u);
    CHECK4("12 1e 3C", "%o %x %X", 10u, 30u, 60u);
    CHECK4(" 12 1e 3C ", "%3o %2x %-3X", 10u, 30u, 60u);
    CHECK4("012 0x1e 0X3C", "%#o %#x %#X", 10u, 30u, 60u);
    CHECK2("", "%.0x", 0);
    CHECK2("0", "%.0d", 0);  // stb_sprintf gives "0"
    CHECK3("33 555", "%hi %ld", (short)33, 555l);
    CHECK2("9888777666", "%llu", 9888777666llu);
}

void test_floating_point(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    const double pow_2_85 = 38685626227668133590597632.0;

    CHECK2("-3.000000", "%f", -3.0);
    CHECK2("This is a very long string which will call SIMD code for sure -3.000000", "This is a very long string which will call SIMD code for sure %f", -3.0);
    CHECK2("-8.8888888800", "%.10f", -8.88888888);
    CHECK2("880.0888888800", "%.10f", 880.08888888);
    CHECK2("4.1", "%.1f", 4.1);
    CHECK2(" 0", "% .0f", 0.1);
    CHECK2("0.00", "%.2f", 1e-4);
    CHECK2("-5.20", "%+4.2f", -5.2);
    CHECK2("0.0       ", "%-10.1f", 0.);
    CHECK2("-0.000000", "%f", -0.);
    CHECK2("0.000001", "%f", 9.09834e-07);
    CHECK2("38685626227668133600000000.0", "%.1f", pow_2_85);
    CHECK2("0.000000499999999999999978", "%.24f", 5e-7);
    CHECK2("0.000000000000000020000000", "%.24f", 2e-17);
    CHECK3("0.0000000100 100000000", "%.10f %.0f", 1e-8, 1e+8);
    CHECK2("100056789.0", "%.1f", 100056789.0);
    CHECK4(" 1.23 %", "%*.*f %%", 5, 2, 1.23);
    CHECK2("-3.000000e+00", "%e", -3.0);
    CHECK2("4.1E+00", "%.1E", 4.1);
    CHECK2("-5.20e+00", "%+4.2e", -5.2);
    CHECK3("+0.3 -3", "%+g %+g", 0.3, -3.0);
    CHECK2("4", "%.1G", 4.1);
    CHECK2("-5.2", "%+4.2g", -5.2);
    CHECK2("3e-300", "%g", 3e-300);
    CHECK2("1", "%.0g", 1.2);
    CHECK3(" 3.7 3.71", "% .3g %.3g", 3.704, 3.706);
    CHECK3("2e-315:1e+308", "%g:%g", 2e-315, 1e+308);

    CHECK4("Inf Inf NaN", "%g %G %f", INFINITY, INFINITY, NAN);
    CHECK2("N", "%.1g", NAN);
}

void test_n(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    int n = 0;
    CHECK3("aaa ", "%.3s %n", "aaaaaaaaaaaaa", &n);
    assert(n == 4);
}

void test_hex_floats(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    CHECK2("0x1.fedcbap+98", "%a", 0x1.fedcbap+98);
    CHECK2("0x1.999999999999a0p-4", "%.14a", 0.1);
    CHECK2("0x1.0p-1022", "%.1a", 0x1.ffp-1023);
    CHECK2("0x1.009117p-1022", "%a", 2.23e-308);
    CHECK2("-0x1.AB0P-5", "%.3A", -0x1.abp-5);
}

void test_pointer(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    CHECK2("0000000000000000", "%p", (void*) NULL);
}

void test_fatptr_format(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    uint8_t hello_data[] = "hello";
    JSLFatPtr hello = jsl_fatptr_init(hello_data, 5);
    CHECK2("hello", "%y", hello);

    uint8_t world_data[] = "world";
    JSLFatPtr world = jsl_fatptr_init(world_data, 5);
    CHECK2("begin-world", "begin-%y", world);

    JSLFatPtr empty = jsl_fatptr_init(NULL, 0);
    CHECK2("ed(ERROR)ge", "ed%yge", empty);

    uint8_t beta_data[] = "beta";
    JSLFatPtr beta = jsl_fatptr_init(beta_data, 4);
    CHECK3("hello-beta", "%y-%y", hello, beta);
}

void test_quote_modifier(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    CHECK2("1,200,000", "%'d", 1200000);
    CHECK2("-100,006,789", "%'d", -100006789);
    CHECK2("9,888,777,666", "%'lld", 9888777666ll);
    CHECK2("200,000,000.000000", "%'18f", 2e8);
    CHECK2("100,056,789", "%'.0f", 100056789.0);
    CHECK2("100,056,789.0", "%'.1f", 100056789.0);
    CHECK2("000,001,200,000", "%'015d", 1200000);
}

void test_nonstandard(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    CHECK2("(ERROR)", "%s", (char*) NULL);
    CHECK2("123,4abc:", "%'x:", 0x1234ABC);
    CHECK2("100000000", "%b", 256);
    CHECK3("0b10 0B11", "%#b %#B", 2, 3);
    CHECK4("2 3 4", "%I64d %I32d %Id", 2ll, 3, 4ll);
    CHECK3("1k 2.54 M", "%$_d %$.2d", 1000, 2536000);
    CHECK3("2.42 Mi 2.4 M", "%$$.2d %$$$d", 2536000, 2536000);
}

void test_separators(void)
{
    uint8_t _buf[1024];
    JSLFatPtr buffer = jsl_fatptr_init(_buf, 1024);

    jsl_format_set_separators(' ', ',');
    CHECK2("12 345,678900", "%'f", 12345.6789);
}


int main(void)
{
    lrun("Test format ints", test_integers);
    lrun("Test format floating point", test_floating_point);
    lrun("Test format length capture", test_n);
    lrun("Test format hex floats", test_hex_floats);
    lrun("Test format pointer", test_pointer);
    lrun("Test format fat pointer", test_fatptr_format);
    lrun("Test format quote modifier", test_quote_modifier);
    lrun("Test format non-standard", test_nonstandard);
    lrun("Test format separators", test_separators);

    lresults();
    return lfails != 0;
}

/**
 * # Jack's Standard Library
 *
 * A collection of utilities which are designed to replace much of the C standard
 * library.
 *
 * See README.md for a detailed intro.
 *
 * See DESIGN.md for background on the design decisions.
 *
 * See DOCUMENTATION.md for a single markdown file containing all of the docstrings
 * from this file. It's more nicely formatted and contains hyperlinks.
 *
 * The convention of this library is that all symbols prefixed with either `jsl__`
 * or `JSL__` (with two underscores) are meant to be private to this library. They
 * are not a stable part of the API.
 *
 * ## External Preprocessor Definitions
 *
 * `JSL_DEBUG` - turns on some debugging features, like overwriting stale memory with
 * `0xfeefee`.
 *
 * ## License
 *
 * Copyright (c) 2025 Jack Stouffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif


/**
 *
 *
 *                      INTERNAL DEFINITIONS
 *
 *
 */


#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    || defined(_BIG_ENDIAN)
    || defined(__BIG_ENDIAN__)
    || defined(__ARMEB__)
    || defined(__MIPSEB__)

    #error "ERROR: JSL_CORE_H_INCLUDED.h does not support big endian targets!"

#endif

/**
 *
 *  Platform Detection Macros
 *
 */

#if defined(_WIN32)

    #define JSL__IS_WINDOWS_VAL 1
    #define JSL__IS_POSIX_VAL 0
    #define JSL__IS_WEB_ASSEMBLY_VAL 0

#elif defined(__linux__) || defined(__linux) || \
    defined(__APPLE__) || defined(__MACH__) || \
    defined(__FreeBSD__) || defined(__OpenBSD__) || \
    defined(__NetBSD__) || \
    defined(__DragonFly__) || \
    defined(__sun) || defined(sun) || \
    defined(__hpux) || \
    defined(_AIX) || \
    defined(__CYGWIN__)

    #define JSL__IS_WINDOWS_VAL 0
    #define JSL__IS_POSIX_VAL 1
    #define JSL__IS_WEB_ASSEMBLY_VAL 0

#elif defined(__wasm__) && defined(__wasm32__) && !defined(__wasm64__)

    #define JSL__IS_WINDOWS_VAL 0
    #define JSL__IS_POSIX_VAL 0
    #define JSL__IS_WEB_ASSEMBLY_VAL 1

#else

    #define JSL__IS_WINDOWS_VAL 0
    #define JSL__IS_POSIX_VAL 0
    #define JSL__IS_WEB_ASSEMBLY_VAL 0

#endif

#if defined(_M_X64) \
    || defined(_M_AMD64) \
    || defined(__x86_64__) \
    || defined(__amd64__) \
    || defined(__i386__) \
    || defined(_M_IX86)

    #define JSL__IS_X86_VAL 1
    #define JSL__IS_ARM_VAL 0

#elif defined(__aarch64__) || defined(_M_ARM64)

    #define JSL__IS_X86_VAL 0
    #define JSL__IS_ARM_VAL 1

#endif

#if defined(__GNUC__) && !defined(__clang__)

    #define JSL__IS_GCC_VAL 1
    #define JSL__IS_CLANG_VAL 0
    #define JSL__IS_MSVC_VAL 0

#elif defined(__clang__)

    #define JSL__IS_GCC_VAL 0
    #define JSL__IS_CLANG_VAL 1
    #define JSL__IS_MSVC_VAL 0

#else

    #define JSL__IS_GCC_VAL 0
    #define JSL__IS_CLANG_VAL 0
    #define JSL__IS_MSVC_VAL 0

#endif

#if defined(__SIZEOF_POINTER__)
    #if __SIZEOF_POINTER__ == 8
        #define JSL__IS_POINTER_32_BITS_VAL 0
        #define JSL__IS_POINTER_64_BITS_VAL 1
    #elif __SIZEOF_POINTER__ == 4
        #define JSL__IS_POINTER_32_BITS_VAL 1
        #define JSL__IS_POINTER_64_BITS_VAL 0
    #else
        #error "ERROR: JSL_CORE_H_INCLUDED.h can only be used with 32 or 64 bit pointers"
    #endif
#elif defined(_WIN64)
    #define JSL__IS_POINTER_32_BITS_VAL 0
    #define JSL__IS_POINTER_64_BITS_VAL 1
#elif defined(_WIN32)
    #define JSL__IS_POINTER_32_BITS_VAL 1
    #define JSL__IS_POINTER_64_BITS_VAL 0
#elif defined(__x86_64__) || defined(__aarch64__) || defined(__ppc64__)
    #define JSL__IS_POINTER_32_BITS_VAL 0
    #define JSL__IS_POINTER_64_BITS_VAL 1
#elif defined(__i386__) || defined(__arm__)
    #define JSL__IS_POINTER_32_BITS_VAL 1
    #define JSL__IS_POINTER_64_BITS_VAL 0
#else
    #error "ERROR: JSL_CORE_H_INCLUDED.h can only be used with 32 or 64 bit pointers"
#endif

#ifndef JSL_DEBUG
    #ifndef JSL__FORCE_INLINE

        #if JSL__IS_MSVC_VAL
            #define JSL__FORCE_INLINE __forceinline
        #elif JSL__IS_CLANG_VAL || JSL__IS_GCC_VAL
            #define JSL__FORCE_INLINE inline __attribute__((__always_inline__))
        #else
            #define JSL__FORCE_INLINE
        #endif

    #endif
#else
    #define JSL__FORCE_INLINE
#endif

#if JSL__IS_CLANG_VAL || JSL__IS_GCC_VAL
    #define JSL__LIKELY(x) __builtin_expect(!!(x), 1)
#else
    #define JSL__LIKELY(x) (x)
#endif

#if JSL__IS_CLANG_VAL || JSL__IS_GCC_VAL
    #define JSL__UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define JSL__UNLIKELY(x) (x)
#endif

#if defined(__SANITIZE_ADDRESS__) && __SANITIZE_ADDRESS__
    #define JSL__HAS_ASAN 1
#elif defined(__has_feature)
    #if __has_feature(address_sanitizer)
        #define JSL__HAS_ASAN 1
    #endif
#endif

#ifndef JSL__HAS_ASAN
    #define JSL__HAS_ASAN 0
#endif

#if JSL__HAS_ASAN
    #include <sanitizer/asan_interface.h>
#else
    #define ASAN_POISON_MEMORY_REGION(ptr, len)
    #define ASAN_UNPOISON_MEMORY_REGION(ptr, len)
#endif

#if JSL__IS_CLANG_VAL || JSL__IS_GCC_VAL
    #if JSL__HAS_ASAN
        #if defined(__has_attribute)
            #if __has_attribute(no_sanitize_address)
                #define JSL__ASAN_OFF __attribute__((__no_sanitize_address__))
            #endif
        #elif JSL__IS_GCC_VAL
            #define JSL__ASAN_OFF __attribute__((__no_sanitize_address__))
        #endif
    #endif
#endif

#ifndef JSL__ASAN_OFF
    #define JSL__ASAN_OFF
#endif

#define JSL__ASAN_GUARD_SIZE 8


/**
 *
 *  Built-Ins Macros For MSVC Compatibility
 *
 */

int32_t jsl__count_trailing_zeros_u32(uint32_t x);
int32_t jsl__count_trailing_zeros_u64(uint64_t x);
int32_t jsl__count_leading_zeros_u32(uint32_t x);
int32_t jsl__count_leading_zeros_u64(uint64_t x);
int32_t jsl__population_count_u32(uint32_t x);
int32_t jsl__population_count_u64(uint64_t x);
int32_t jsl__find_first_set_u32(uint32_t x);
int32_t jsl__find_first_set_u64(uint64_t x);

#if JSL__IS_GCC_VAL || JSL__IS_CLANG_VAL
    #define JSL__COUNT_TRAILING_ZEROS_IMPL(x) __builtin_ctz(x)

    #if UINT64_MAX == ULLONG_MAX
        #define JSL__COUNT_TRAILING_ZEROS_IMPL64(x) __builtin_ctzll(x)
    #elif UINT64_MAX == ULONG_MAX
        #define JSL__COUNT_TRAILING_ZEROS_IMPL64(x) __builtin_ctzl(x)
    #endif

    #define JSL__COUNT_LEADING_ZEROS_IMPL(x) __builtin_clz(x)

    #if UINT64_MAX == ULLONG_MAX
        #define JSL__COUNT_LEADING_ZEROS_IMPL64(x) __builtin_clzll(x)
    #elif UINT64_MAX == ULONG_MAX
        #define JSL__COUNT_LEADING_ZEROS_IMPL64(x) __builtin_clzl(x)
    #endif


    #define JSL__POPULATION_COUNT_IMPL(x) __builtin_popcount(x)

    #if UINT64_MAX == ULLONG_MAX
        #define JSL__POPULATION_COUNT_IMPL64(x) __builtin_popcountll(x)
    #elif UINT64_MAX == ULONG_MAX
        #define JSL__POPULATION_COUNT_IMPL64(x) __builtin_popcountl(x)
    #endif

    #define JSL__FIND_FIRST_SET_IMPL(x) __builtin_ffs(x)

    #if UINT64_MAX == ULLONG_MAX
        #define JSL__FIND_FIRST_SET_IMPL64(x) __builtin_ffsll(x)
    #elif UINT64_MAX == ULONG_MAX
        #define JSL__FIND_FIRST_SET_IMPL64(x) __builtin_ffsl(x)
    #endif

#elif JSL__IS_MSVC_VAL

    #define JSL__COUNT_TRAILING_ZEROS_IMPL(x) jsl__count_trailing_zeros_u32(x)
    #define JSL__COUNT_TRAILING_ZEROS_IMPL64(x) jsl__count_trailing_zeros_u64(x)
    #define JSL__COUNT_LEADING_ZEROS_IMPL(x) __lzcnt(x)
    #define JSL__COUNT_LEADING_ZEROS_IMPL64(x) __lzcnt64(x)
    #define JSL__POPULATION_COUNT_IMPL(x) __popcnt(x)
    #define JSL__POPULATION_COUNT_IMPL64(x) __popcnt64(x)
    #define JSL__FIND_FIRST_SET_IMPL(x) jsl__find_first_set_u32(x)
    #define JSL__FIND_FIRST_SET_IMPL64(x) jsl__find_first_set_u64(x)

#else

    #define JSL__COUNT_TRAILING_ZEROS_IMPL(x) jsl__count_trailing_zeros_u32(x)
    #define JSL__COUNT_TRAILING_ZEROS_IMPL64(x) jsl__count_trailing_zeros_u64(x)
    #define JSL__COUNT_LEADING_ZEROS_IMPL(x) jsl__count_leading_zeros_u32(x)
    #define JSL__COUNT_LEADING_ZEROS_IMPL64(x) jsl__count_leading_zeros_u64(x)
    #define JSL__POPULATION_COUNT_IMPL(x) jsl__population_count_u32(x)
    #define JSL__POPULATION_COUNT_IMPL64(x) jsl__population_count_u64(x)
    #define JSL__FIND_FIRST_SET_IMPL(x) jsl__find_first_set_u32(x)
    #define JSL__FIND_FIRST_SET_IMPL64(x) jsl__find_first_set_u64(x)

#endif


#ifdef JSL_DEBUG

    #if JSL__IS_GCC_VAL || JSL__IS_CLANG_VAL

        #define JSL__DONT_OPTIMIZE_AWAY_IMPL(x)                 \
            do                                                  \
            {                                                   \
                __asm__ __volatile__("" : : "g"(x) : "memory"); \
            }                                                   \
            while (0)

    #elif JSL__IS_MSVC_VAL

        __declspec(noinline) static inline void jsl__msvc_debug_sink(const void *p)
        {
            /* Pretend to use p in a way the optimizer can’t see through easily */
            _ReadWriteBarrier();
            (void) p;
        }

        #define JSL__DONT_OPTIMIZE_AWAY_IMPL(x)         \
            do                                          \
            {                                           \
                jsl_msvc_debug_sink(&(x));              \
            }                                           \
            while (0)

    #else

        #define JSL__DONT_OPTIMIZE_AWAY_IMPL(x) do {} while (0)

    #endif

#else

    #define JSL__DONT_OPTIMIZE_AWAY_IMPL(x) do {} while (0)

#endif

/**
 *
 *
 *                      PUBLIC API
 *
 *
 */


#ifndef JSL_VERSION
    #define JSL_VERSION 0x010000  /* 1.0.0 */
#else
    #if JSL_VERSION != 0x010000
        #error "ERROR: Conflicting versions of JSL_CORE_H_INCLUDED.h included in the same translation unit!"
    #endif
#endif

#if defined(__has_attribute)
    #if __has_attribute(fallthrough)
        #define JSL_SWITCH_FALLTHROUGH __attribute__((fallthrough))
    #endif
#endif

#ifndef JSL_SWITCH_FALLTHROUGH
    #define JSL_SWITCH_FALLTHROUGH do { } while (0)
#endif

/**
 *
 *  Libc Override Macros
 *
 */

#ifndef JSL_ASSERT
    #include <assert.h>
    void jsl__assert(int condition, char* file, int line);

    /**
     * Assertion function definition. By default this will use `assert.h`.
     * If you wish to override it, it must be a function which takes three parameters, a int
     * conditional, a char* of the filename, and an int line number. You can also provide an
     * empty function if you just want to turn off asserts altogether; this is not
     * recommended. The small speed boost you get is from avoiding a branch is generally not
     * worth the loss of memory protection.
     *
     * Define this as a macro before importing the library to override this.
     */
    #define JSL_ASSERT(condition) jsl__assert(condition, __FILE__, __LINE__)
#endif

#ifndef JSL_MEMCPY
    #include <string.h>

    /**
     * Controls memcpy calls in the library. By default this will include
     * `string.h` and be an alias to C's `memcpy`.
     *
     * Define this as a macro before importing the library to override this.
     * Your macro must follow the libc `memcpy` signature of
     *
     * ```
     * void your_memcpy(void*, const void*, size_t);
     * ```
     */
    #define JSL_MEMCPY memcpy
#endif

#ifndef JSL_MEMCMP
    #include <string.h>

    /**
     * Controls memcmp calls in the library. By default this will include
     * `string.h` and be an alias to C's `memcmp`.
     *
     * Define this as a macro before importing the library to override this.
     * Your macro must follow the libc `memcmp` signature of
     *
     * ```
     * int your_memcmp(const void*, const void*, size_t);
     * ```
     */
    #define JSL_MEMCMP memcmp
#endif

#ifndef JSL_MEMSET
    #include <string.h>

    /**
     * Controls memset calls in the library. By default this will include
     * `string.h` and be an alias to C's `memset`.
     *
     * Define this as a macro before importing the library to override this.
     * Your macro must follow the libc `memset` signature of
     *
     * ```
     * void your_memset(void*, int, size_t);
     * ```
     */
    #define JSL_MEMSET memset
#endif

#ifndef JSL_STRLEN
    #include <string.h>

    /**
     * Controls strlen calls in the library. By default this will include
     * `string.h` and be an alias to C's `strlen`.
     *
     * Define this as a macro before importing the library to override this.
     * Your macro must follow the libc `strlen` signature of
     *
     * ```
     * size_t your_strlen(const char*);
     * ```
     */
    #define JSL_STRLEN strlen
#endif

/**
 *
 *  Platform Detection Macros
 *
 */

/**
 * If the target platform is Windows OS, then 1, else 0.
 */
#define JSL_IS_WINDOWS JSL__IS_WINDOWS_VAL

/**
 * If the target platform is a POSIX, then 1, else 0.
 */
#define JSL_IS_POSIX JSL__IS_POSIX_VAL

/**
 * If the target platform is in WebAssembly, then 1, else 0.
 */
#define JSL_IS_WEB_ASSEMBLY JSL__IS_WEB_ASSEMBLY_VAL

/**
 * If the target platform is in x86, then 1, else 0.
 */
#define JSL_IS_X86 JSL__IS_X86_VAL

/**
 * If the target platform is in ARM, then 1, else 0.
 */
#define JSL_IS_ARM JSL__IS_ARM_VAL

/**
 * If the host compiler is GCC, then 1, else 0.
 */
#define JSL_IS_GCC JSL__IS_GCC_VAL

/**
 * If the host compiler is clang, then 1, else 0.
 */
#define JSL_IS_CLANG JSL__IS_CLANG_VAL

/**
 * If the host compiler is MSVC, then 1, else 0.
 */
#define JSL_IS_MSVC JSL__IS_MSVC_VAL

/**
 * If the target executable uses 32 bit pointers, then 1, else 0.
 */
#define JSL_IS_POINTER_32_BITS JSL__IS_POINTER_32_BITS_VAL

/**
 * If the target executable uses 64 bit pointers, then 1, else 0.
 */
#define JSL_IS_POINTER_64_BITS JSL__IS_POINTER_64_BITS_VAL

#ifndef JSL_WARN_UNUSED

    #if defined(__clang__) ||  defined(__GNUC__)
        /**
         * This controls the function attribute which tells the compiler to
         * issue a warning if the return value of the function is not stored in a variable, or if
         * that variable is never read. This is auto defined for clang and gcc; there's no
         * C11 compatible implementation for MSVC. If you want to turn this off, just define it as
         * empty string.
         */
        #define JSL_WARN_UNUSED __attribute__((warn_unused_result))
    #else
        #define JSL_WARN_UNUSED
    #endif

#endif

#ifndef JSL_DEF
    /**
     * Allows you to override linkage/visibility (e.g., __declspec) for all of
     * the functions defined by this library. By default this is empty, so extern.
     *
     * Define this as a macro before importing the library to override this.
     */
    #define JSL_DEF
#endif

#ifndef JSL_DEFAULT_ALLOCATION_ALIGNMENT
    /**
     * Sets the alignment of allocations that aren't explicitly set. Defaults to 8 bytes.
     *
     * Define this as a macro before importing the library to override this.
     */
    #define JSL_DEFAULT_ALLOCATION_ALIGNMENT 8
#endif

/**
 * Platform specific intrinsic for returning the count of trailing zeros for 32
 * bit signed and unsigned integers.
 *
 * In order to be as fast as possible, this does not represent a cross
 * platform abstraction over different ctz implementations.
 * On GCC and clang, this is replaced with `__builtin_ctz`. On MSVC
 * `_BitScanForward` is used in a forced inline function call. Behavior
 * with zero is undefined for GCC and clang while MSVC will give 32.
 */
#define JSL_PLATFORM_COUNT_TRAILING_ZEROS(x) JSL__COUNT_TRAILING_ZEROS_IMPL((x))

/**
 * Platform specific intrinsic for returning the count of trailing zeros for 64
 * bit signed and unsigned integers.
 *
 * In order to be as fast as possible, this does not represent a cross
 * platform abstraction over different ctz implementations.
 * On GCC and clang, this is replaced with `__builtin_ctzll`. On MSVC
 * `_BitScanForward64` is used in a forced inline function call. Behavior
 * with zero is undefined for GCC and clang while MSVC will give 32.
 */
#define JSL_PLATFORM_COUNT_TRAILING_ZEROS64(x) JSL__COUNT_TRAILING_ZEROS_IMPL64((x))

/**
 * Platform specific intrinsic for returning the count of leading zeros for 32
 * bit signed and unsigned integers.
 *
 * In order to be as fast as possible, this does not represent a cross
 * platform abstraction over different clz implementations.
 * On GCC and clang, this is replaced with `__builtin_clz`. On MSVC
 * `_BitScanReverse` is used in a forced inline function call. Behavior
 * with zero is undefined for GCC and clang while MSVC will give 32.
 */
#define JSL_PLATFORM_COUNT_LEADING_ZEROS(x) JSL__COUNT_LEADING_ZEROS_IMPL((x))

/**
 * Platform specific intrinsic for returning the count of leading zeros for 64
 * bit signed and unsigned integers.
 *
 * In order to be as fast as possible, this does not represent a cross
 * platform abstraction over different clz implementations.
 * On GCC and clang, this is replaced with `__builtin_clzll`. On MSVC
 * `_BitScanReverse64` is used in a forced inline function call. Behavior
 * with zero is undefined for GCC and clang while MSVC will give 32.
 */
#define JSL_PLATFORM_COUNT_LEADING_ZEROS64(x) JSL__COUNT_LEADING_ZEROS_IMPL64((x))

/**
 * Platform specific intrinsic for returning the count of set bits for 32
 * bit signed and unsigned integers.
 *
 * In order to be as fast as possible, this does not represent a cross
 * platform abstraction over different popcnt implementations.
 * On GCC and clang, this is replaced with `__builtin_popcount`. On MSVC
 * `__popcnt` is used.
 */
#define JSL_PLATFORM_POPULATION_COUNT(x) JSL__POPULATION_COUNT_IMPL((x))

/**
 * Platform specific intrinsic for returning the count of set bits for 64
 * bit signed and unsigned integers.
 *
 * In order to be as fast as possible, this does not represent a cross
 * platform abstraction over different popcnt implementations.
 * On GCC and clang, this is replaced with `__builtin_popcountll`. On MSVC
 * `__popcnt64` is used.
 */
#define JSL_PLATFORM_POPULATION_COUNT64(x) JSL__POPULATION_COUNT_IMPL64((x))

/**
 * Platform specific intrinsic for returning the index of the first set
 * bit for 32 bit signed and unsigned integers.
 *
 * In order to be as fast as possible, this does not represent a cross
 * platform abstraction over different ffs implementations.
 * On GCC and clang, this is replaced with `__builtin_ffs`. On MSVC
 * `_BitScanForward` is used in a forced inline function call. Behavior
 * with zero is undefined for GCC and clang while MSVC will give 32.
 */
#define JSL_PLATFORM_FIND_FIRST_SET(x) JSL__FIND_FIRST_SET_IMPL((x))

/**
 * Platform specific intrinsic for returning the index of the first set
 * bit for 64 bit signed and unsigned integers.
 *
 * In order to be as fast as possible, this does not represent a cross
 * platform abstraction over different ffs implementations.
 * On GCC and clang, this is replaced with `__builtin_ffsll`. On MSVC
 * `_BitScanForward64` is used in a forced inline function call. Behavior
 * with zero is undefined for GCC and clang while MSVC will give 32.
 */
#define JSL_PLATFORM_FIND_FIRST_SET64(x) JSL__FIND_FIRST_SET_IMPL64((x))

/**
 * Evaluates the maximum of two values.
 *
 * @warning This macro evaluates its arguments multiple times. Do not use with
 * arguments that have side effects (e.g., function calls, increment/decrement
 * operations) as they will be executed more than once.
 *
 * Example:
 * ```c
 * int max_val = JSL_MAX(10, 20);        // Returns 20
 * double max_d = JSL_MAX(3.14, 2.71);   // Returns 3.14
 *
 * // DANGER: Don't do this - increment happens twice!
 * // int bad = JSL_MAX(++x, y);
 * ```
 */
#define JSL_MAX(a,b) ((a) > (b) ? (a) : (b))

/**
 * Evaluates the minimum of two values.
 *
 * @warning This macro evaluates its arguments multiple times. Do not use with
 * arguments that have side effects (e.g., function calls, increment/decrement
 * operations) as they will be executed more than once.
 *
 * Example:
 * ```c
 * int max_val = JSL_MAX(10, 20);        // Returns 20
 * double max_d = JSL_MAX(3.14, 2.71);   // Returns 3.14
 *
 * // DANGER: Don't do this - increment happens twice!
 * // int bad = JSL_MAX(++x, y);
 * ```
 */
#define JSL_MIN(a,b) ((a) < (b) ? (a) : (b))

/**
 * Returns `x` bound between the two given values.
 *
 * @warning This macro evaluates its arguments multiple times. Do not use with
 * arguments that have side effects (e.g., function calls, increment/decrement
 * operations) as they will be executed more than once.
 *
 * Example:
 * ```c
 * int max_val = JSL_BETWEEN(10, 15, 20);        // Returns 15
 * double max_d = JSL_BETWEEN(1.2, 0.1, 3.14);   // Returns 1.2
 *
 * // DANGER: Don't do this - increment happens more than once!
 * // int bad = JSL_BETWEEN(32, ++x, 64);
 * ```
 */
#define JSL_BETWEEN(min, x, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

/**
 * Sets a specific bit flag in a bitfield by performing a bitwise OR operation.
 *
 * @param flags Pointer to the bitfield variable where the flag should be set
 * @param flag The flag value(s) to set. Can be a single flag or multiple flags OR'd together
 *
 * @note This macro evaluates its arguments multiple times. Do not use with
 * arguments that have side effects (e.g., function calls, increment/decrement
 * operations) as they will be executed more than once.
 *
 * Example:
 *
 * ```
 * #define FLAG_READ    JSL_MAKE_BITFLAG(1)
 * #define FLAG_WRITE   JSL_MAKE_BITFLAG(2)
 *
 * uint32_t permissions = 0;
 *
 * JSL_SET_BITFLAG(&permissions, FLAG_READ);
 * JSL_SET_BITFLAG(&permissions, FLAG_WRITE);
 *
 * // DANGER: Don't do this - increment happens twice!
 * // JSL_SET_BITFLAG(&array[++index], some_flag);
 * ```
 */
#define JSL_SET_BITFLAG(flags, flag) *flags |= flag

/**
 * Clears a bit flag from a value pointed to by `flags` by zeroing the bits set in `flag`.
 *
 * Example:
 *
 * ```
 * uint32_t permissions = FLAG_READ | FLAG_WRITE;
 * JSL_UNSET_BITFLAG(&permissions, FLAG_WRITE);
 * // `permissions` now only has FLAG_READ set.
 * ```
 */
#define JSL_UNSET_BITFLAG(flags, flag) *flags &= ~(flag)

/**
 * Returns non-zero when every bit in `flag` is also set within `flags`.
 *
 * Example:
 *
 * ```
 * uint32_t permissions = FLAG_READ | FLAG_WRITE;
 * if (JSL_IS_BITFLAG_SET(permissions, FLAG_READ)) {
 *     // FLAG_READ is present
 * }
 * ```
 */
#define JSL_IS_BITFLAG_SET(flags, flag) ((flags & flag) == flag)

/**
 * Returns non-zero when none of the bits in `flag` are set within `flags`.
 *
 * Example:
 *
 * ```
 * uint32_t permissions = FLAG_READ;
 * if (JSL_IS_BITFLAG_NOT_SET(permissions, FLAG_WRITE)) {
 *     // FLAG_WRITE is not present
 * }
 * ```
 */
#define JSL_IS_BITFLAG_NOT_SET(flags, flag) ((flags & flag) == 0)

/**
 * Generates a bit flag with a single bit set at the given zero-based position.
 *
 * Example:
 *
 * ```
 * enum PermissionFlags {
 *     FLAG_READ = JSL_MAKE_BITFLAG(0),
 *     FLAG_WRITE = JSL_MAKE_BITFLAG(1),
 * };
 * uint32_t permissions = FLAG_READ | FLAG_WRITE;
 * ```
 */
#define JSL_MAKE_BITFLAG(position) 1U << position

/**
 * Macro to simply mark a value as representing a size in bytes. Does nothing with the value.
 */
#define JSL_BYTES(x) x

/**
 * Converts a numeric value to megabytes by multiplying by `1024`.
 *
 * This macro is useful for specifying memory sizes in a more readable format,
 * particularly when allocating.
 *
 * Example:
 * ```c
 * uint8_t buffer[JSL_KILOBYTES(16)];
 * JSLArena arena;
 * jsl_arena_init(&arena, buffer, JSL_KILOBYTES(16));
 * ```
 */
#define JSL_KILOBYTES(x) x * 1024

/**
 * Converts a numeric value to megabytes by multiplying by `1024 ^ 2`.
 *
 * This macro is useful for specifying memory sizes in a more readable format,
 * particularly when allocating.
 *
 * Example:
 * ```c
 * uint8_t buffer[JSL_MEGABYTES(16)];
 * JSLArena arena;
 * jsl_arena_init(&arena, buffer, JSL_MEGABYTES(16));
 * ```
 */
#define JSL_MEGABYTES(x) x * 1024 * 1024


/**
 * Converts a numeric value to gigabytes by multiplying by `1024 ^ 3`.
 *
 * This macro is useful for specifying memory sizes in a more readable format,
 * particularly when allocating.
 *
 * Example:
 * ```c
 * void* buffer = malloc(JSL_GIGABYTES(2));
 * JSLArena arena;
 * jsl_arena_init(&arena, buffer, JSL_GIGABYTES(2));
 * ```
 */
#define JSL_GIGABYTES(x) x * 1024 * 1024 * 1024

/**
 * Converts a numeric value to terabytes by multiplying by `1024 ^ 4`.
 *
 * This macro is useful for specifying memory sizes in a more readable format,
 * particularly when allocating.
 *
 * Example:
 * ```c
 * // Reserve two gigabytes of virtual address space starting at 2 terabytes.
 * // If you're using static offsets this means that your objects in memory will
 * // be at the same place every time you run your program in your debugger!
 *
 * void* buffer = VirtualAlloc(JSL_TERABYTES(2), JSL_GIGABYTES(2), MEM_RESERVE, PAGE_READWRITE);
 * JSLArena arena;
 * jsl_arena_init(&arena, buffer, JSL_GIGABYTES(2));
 * ```
 */
#define JSL_TERABYTES(x) x * 1024 * 1024 * 1024 * 1024

/**
 * TODO: docs
 */
#define JSL_DEBUG_DONT_OPTIMIZE_AWAY(x) JSL__DONT_OPTIMIZE_AWAY_IMPL(x)

/**
 * Round x up to the next power of two. If x is a power of two it returns
 * the same value.
 *
 * This function is designed to be used in tight loops and other performance
 * critical areas. Therefore, both zero, one, and values greater than 2^31 not special
 * cased. The return values for these cases are compiler, OS, and ISA specific.
 * If you need consistent behavior, then you can easily call this function like
 * so:
 *
 * ```
 * jsl_next_power_of_two_u32(
 *      JSL_BETWEEN(2u, x, 0x80000000u)
 * );
 * ```
 *
 * @param x The value to round up
 * @returns the next power of two
 */
uint32_t jsl_next_power_of_two_u32(uint32_t x);

// TODO: docs
int64_t jsl_next_power_of_two_i64(int64_t x);

/**
 * Round x up to the next power of two. If x is a power of two it returns
 * the same value.
 *
 * This function is designed to be used in tight loops and other performance
 * critical areas. Therefore, both zero and values greater than 2^63 not special
 * cased. The return values for these cases are compiler, OS, and ISA specific.
 * If you need consistent behavior, then you can easily call this function like
 * so:
 *
 * ```
 * jsl_next_power_of_two_u64(
 *      JSL_BETWEEN(1ull, x, 0x8000000000000000ull)
 * );
 * ```
 *
 * @param x The value to round up
 * @returns the next power of two
 */
uint64_t jsl_next_power_of_two_u64(uint64_t x);

/**
 * Round x down to the previous power of two. If x is a power of two it returns
 * the same value.
 *
 * This function is designed to be used in tight loops and other performance
 * critical areas. Therefore, zero is not special cased. The return value is
 * compiler, OS, and ISA specific. If you need consistent behavior, then you
 * can easily call this function like so:
 *
 * ```
 * jsl_previous_power_of_two_u32(
 *      JSL_MAX(1, x)
 * );
 * ```
 *
 * @param x The value to round up
 * @returns the next power of two
 */
uint32_t jsl_previous_power_of_two_u32(uint32_t x);

/**
 * Round x down to the previous power of two. If x is a power of two it returns
 * the same value.
 *
 * This function is designed to be used in tight loops and other performance
 * critical areas. Therefore, zero is not special cased. The return value is
 * compiler, OS, and ISA specific. If you need consistent behavior, then you
 * can easily call this function like so:
 *
 * ```
 * jsl_previous_power_of_two_u64(
 *      JSL_MAX(1UL, x)
 * );
 * ```
 *
 * @param x The value to round up
 * @returns the next power of two
 */
uint64_t jsl_previous_power_of_two_u64(uint64_t x);

/**
 * A fat pointer is a representation of a chunk of memory. It **is not** a container
 * or an abstract data type.
 *
 * A fat pointer is very similar to D or Go's slices. This provides several useful
 * functions like bounds checked reads/writes.
 *
 * One very important thing to note is that the fat pointer is always defined as mutable.
 * In my opinion, const in C provides very little protection and a world a headaches during
 * refactors, especially since C does not have generics or function overloading. I find the
 * cost benefit analysis to be in the negative.
 */
typedef struct JSLFatPtr
{
    /**
     * The data pointer.
     *
     * uint8_t because, annoyingly, `void*` doesn't have a size, so you can't
     * do `ptr++`. Also, it effectively communicates that this is a range
     * of bytes.
     */
    uint8_t* data;

    /**
     * Length.
     *
     * Intentionally signed. You really don't need the high bit (you are not
     * working with chunks of memory larger than `(2^63) - 1` bytes), and it avoids
     * all sorts of nasty bugs. Any time you subtract from an unsigned value
     * is likely to be an underflow since most numbers stored are small.
     */
    int64_t length;
} JSLFatPtr;

/**
* Creates a JSLFatPtr from a string literal at compile time. The resulting fat pointer
* points directly to the string literal's memory, so no copying occurs.
*
* @warning With MSVC this will only work during variable initialization as MSVC
* still does not support compound literals.
*
* Example:
*
* ```c
* // Create fat pointers from string literals
* JSLFatPtr hello = JSL_FATPTR_INITIALIZER("Hello, World!");
* JSLFatPtr path = JSL_FATPTR_INITIALIZER("/usr/local/bin");
* JSLFatPtr empty = JSL_FATPTR_INITIALIZER("");
* ```
*/
#define JSL_FATPTR_INITIALIZER(s) { (uint8_t*)(s), (int64_t)(sizeof(s) - 1) }

#if defined(_MSC_VER) && !defined(__clang__)

    /**
     * Creates a JSLFatPtr from a string literal for usage as an rvalue. The resulting fat pointer
     * points directly to the string literal's memory, so no copying occurs.
     *
     * @warning On MSVC this will be a function call as MSVC does not support compound literals.
     * On all other compilers this will be a zero cost compound literal.
     *
     * Example:
     *
     * ```c
     * void my_function(JSLFatPtr data);
     *
     * my_function(JSL_FATPTR_EXPRESSION("my data"));
     * ```
     */
    #define JSL_FATPTR_EXPRESSION(s) jsl_fatptr_init((uint8_t*) (s), (int64_t)(sizeof(s) - 1))

#else

    /**
     * Creates a JSLFatPtr from a string literal for usage as an rvalue. The resulting fat pointer
     * points directly to the string literal's memory, so no copying occurs.
     *
     * @warning On MSVC this will be a function call as MSVC does not support compound literals.
     * On all other compilers this will be a zero cost compound literal
     *
     * Example:
     *
     * ```c
     * void my_function(JSLFatPtr data);
     *
     * my_function(JSL_FATPTR_EXPRESSION("my data"));
     * ```
     */
    #define JSL_FATPTR_EXPRESSION(s) ((JSLFatPtr){ (uint8_t*)(s), (int64_t)(sizeof(s) - 1) })

#endif


/**
 * Advances a fat pointer forward by `n`. This macro does not bounds check
 * and is intentionally tiny so it can live in hot loops without adding overhead.
 * Only use this in cases where you've already checked the length.
 */
#define JSL_FATPTR_ADVANCE(fatptr, n) do { \
    fatptr.data += n; \
    fatptr.length -= n; \
} while (0)

/**
 * Creates a `JSLFatPtr` view over a stack-allocated buffer.
 *
 * The buffer must be a real array (not a pointer) so the macro can use
 * `sizeof` to determine its capacity at compile time.
 *
 * Example:
 *
 * ```c
 * uint8_t buffer[JSL_KILOBYTES(4)];
 * JSLFatPtr ptr = JSL_FATPTR_FROM_STACK(buffer);
 * ```
 *
 * @warning This macro only works for variable initializers and cannot be used as a
 * normal rvalue.
 */
#define JSL_FATPTR_FROM_STACK(buf) { (uint8_t *)(buf), (int64_t)(sizeof(buf)) }

/**
 * A bump allocator. Designed for situations in your program when you know a
 * definite lifetime and a good upper bound on how much memory that lifetime will
 * need.
 *
 * See the DESIGN.md file for detailed notes on arena implementation, their uses,
 * and when they shouldn't be used.
 *
 * Functions and Macros:
 *
 * * jsl_arena_init
 * * jsl_arena_init2
 * * jsl_arena_allocate
 * * jsl_arena_allocate_aligned
 * * jsl_arena_reallocate
 * * jsl_arena_reallocate_aligned
 * * jsl_arena_reset
 * * jsl_arena_save_restore_point
 * * jsl_arena_load_restore_point
 * * JSL_ARENA_TYPED_ALLOCATE
 * * JSL_ARENA_FROM_STACK
 *
 * @note The arena API is not thread safe. Arena memory is assumed to live in a
 * single thread. If you want to share an arena between threads you need to lock.
 */
typedef struct JSLArena
{
    uint8_t* start;
    uint8_t* current;
    uint8_t* end;
} JSLArena;

static inline JSLArena jsl__arena_from_stack_internal(void* buf, size_t len)
{
    JSLArena arena = {
        (uint8_t*) buf,
        (uint8_t*) buf,
        ((uint8_t*) buf) + len
    };

    ASAN_POISON_MEMORY_REGION(buf, len);

    return arena;
}

/**
 * Creates an arena from stack memory.
 *
 * Example
 *
 * ```
 * uint8_t buffer[2048];
 * JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 * ```
 *
 * This incredibly useful for getting a dynamic allocator for things which will only
 * last the lifetime of the current function. For example, if the current function
 * needs a hash map, you can use this macro and then there's no cleanup at the end
 * because the stack pointer will be reset.
 *
 * ```
 * void some_func(void)
 * {
 *      uint8_t buffer[JSL_KILOBYTES(16)];
 *      JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 *
 *      // example hash map, not real
 *      IntToStrMap map = int_to_str_ctor(&arena);
 *      int_to_str_add(&map, 64, JSL_FATPTR_INITIALIZER("This is my string data!"));
 *
 *      my_hash_map_calculations(&map);
 *
 *      // All hash map memory goes out of scope automatically,
 *      // no need for any destructor
 * }
 * ```
 *
 * Fast, cheap, easy automatic memory management!
 */
#define JSL_ARENA_FROM_STACK(buf) jsl__arena_from_stack_internal((buf), sizeof(buf))

/**
 * Constructor utility function to make a fat pointer out of a pointer and a length.
 * Useful in cases where you can't use C's struct init syntax, like as a parameter
 * to a function.
 */
JSL_DEF JSLFatPtr jsl_fatptr_init(uint8_t* ptr, int64_t length);

/**
 * Create a new fat pointer that points to the given parameter's data but
 * with a view of [start, end).
 *
 * This function is bounds checked. Out of bounds slices will assert.
 */
JSL_DEF JSLFatPtr jsl_fatptr_slice(JSLFatPtr fatptr, int64_t start, int64_t end);

/**
 * Create a new fat pointer that points to the given parameter's data but
 * with a view of [start, length).
 *
 * This function is bounds checked. Out of bounds slices will assert.
 */
JSL_DEF JSLFatPtr jsl_fatptr_slice_to_end(JSLFatPtr fatptr, int64_t start);

/**
 * Utility function to get the total amount of bytes written to the original
 * fat pointer when compared to a writer fat pointer. See jsl_fatptr_auto_slice
 * to get a slice of the written portion.
 * 
 * This function asserts on the following conditions
 * 
 *      * Either fatptr has a null data field
 *      * Either fatptr has a negative length
 *      * The writer_fatptr does not point to the memory of original_fatptr
 *
 * ```
 * JSLFatPtr original = jsl_arena_allocate(arena, 128 * 1024 * 1024);
 * JSLFatPtr writer = original;
 * jsl_write_file_contents(&writer, "file_one.txt");
 * jsl_write_file_contents(&writer, "file_two.txt");
 * int64_t write_len = jsl_fatptr_total_write_length(original, writer);
 * ```
 *
 * @param original_fatptr The pointer to the originally allocated buffer
 * @param writer_fatptr The pointer that has been advanced during writing operations
 * @returns The amount of data which has been written, or -1 if there was an issue
 */
JSL_DEF int64_t jsl_fatptr_total_write_length(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr);

/**
 * Returns the slice in `original_fatptr` that represents the written to portion, given
 * the size and pointer in `writer_fatptr`.
 * 
 * This function asserts on the following conditions
 * 
 *      * Either fatptr has a null data field
 *      * Either fatptr has a negative length
 *      * The writer_fatptr does not point to the memory of original_fatptr
 *
 * Example:
 *
 * ```
 * JSLFatPtr original = jsl_arena_allocate(arena, 128 * 1024 * 1024);
 * JSLFatPtr writer = original;
 * jsl_write_file_contents(&writer, "file_one.txt");
 * jsl_write_file_contents(&writer, "file_two.txt");
 * JSLFatPtr portion_with_file_data = jsl_fatptr_auto_slice(original, writer);
 * ```
 *
 * @param original_fatptr The pointer to the originally allocated buffer
 * @param writer_fatptr The pointer that has been advanced during writing operations
 * @returns A new fat pointer pointing to the written portion of the original buffer.
 * It will be `NULL` if there was an issue.
 */
JSL_DEF JSLFatPtr jsl_fatptr_auto_slice(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr);

/**
 * Build a fat pointer from a null terminated string. **DOES NOT** copy the data.
 * It simply sets the data pointer to `str` and the length value to the result of
 * JSL_STRLEN.
 *
 * @param str the str to create the fat ptr from
 * @return A fat ptr
 */
JSL_DEF JSLFatPtr jsl_fatptr_from_cstr(const char* str);

/**
 * Copy the contents of `source` into `destination`.
 *
 * `destination` is modified to point to the remaining data in the buffer. I.E.
 * if the entire buffer was used then `destination->length` will be `0` and
 * `destination->data` will be pointing to the end of the buffer.
 *
 * This function is bounds checked, meaning a max of `destination->length` bytes
 * will be copied into `destination`. 
 * 
 * This function also checks for
 *
 *      * overlapping buffers
 *      * null pointers in either `destination` or `source`
 *      * negative lengths.
 *      * If either `destination` or `source` would overflow if their
 *        length was added to the pointer
 * 
 * In all these cases, -1 will be returned.
 *
 * @return Number of bytes written or `-1` if the above error conditions were present.
 */
JSL_DEF int64_t jsl_fatptr_memory_copy(JSLFatPtr* destination, JSLFatPtr source);

/**
 * Writes the contents of the null terminated string at `cstring` into `buffer`.
 *
 * This function is bounds checked, meaning a max of `destination->length` bytes
 * will be copied into `destination`. This function does not check for overlapping
 * pointers.
 *
 * If `cstring` is not a valid null terminated string then this function's behavior
 * is undefined, as it uses JSL_STRLEN.
 *
 * `destination` is modified to point to the remaining data in the buffer. I.E.
 * if the entire buffer was used then `destination->length` will be `0`.
 *
 * @returns Number of bytes written or `-1` if `string` or the fat pointer was null.
 */
JSL_DEF int64_t jsl_fatptr_cstr_memory_copy(
    JSLFatPtr* destination,
    const char* cstring,
    bool include_null_terminator
);

/**
 * Searches `string` for the byte sequence in `substring` and returns the index of the first
 * match or `-1` when no match exists. This is roughly equivalent to C's `strstr` for
 * fat pointers.
 *
 * This function is optimized with SIMD specific implementations when SIMD code generation
 * is enabled during compilation. When SIMD is not enabled, this function falls back to a
 * combination of BNDM and Sunday algorithms (based on substring size). These algorithms
 * are `O(n*m)` in the worst case which is generally text that is very pattern heavy and
 * contains a lot of repeated text. In the general case performance is closer to `O(n/m)`.
 *
 * In cases where any of the following are true you will want to use a different search
 * function:
 *
 *  * Your string is very long, e.g. hundreds of megabytes or more
 *  * Your string is full of small repeating patterns
 *  * Your substring is more than a couple of kilobytes
 *  * You want to search multiple different substrings on the same string
 *
 * @note The comparison operates on raw code units. In UTF encodings, multiple code units can
 * form a single grapheme cluster, so the index does not necessarily map to user-perceived
 * characters. No Unicode normalization is performed; normalize inputs first if combining mark
 * equivalence is required.
 *
 * @param string the string to search in
 * @param substring the substring to search for
 * @returns Index of the first occurrence.
 */
JSL_DEF int64_t jsl_fatptr_substring_search(JSLFatPtr string, JSLFatPtr substring);

/**
 * Locate the first byte equal to `item` in a fat pointer. This is roughly equivalent to C's
 * `strchr` function for fat pointers.
 *
 * @note The comparison operates on raw code units. In UTF encodings, multiple code units can
 * form a single grapheme cluster, so the index does not necessarily map to user-perceived
 * characters. No Unicode normalization is performed; normalize inputs first if combining mark
 * equivalence is required.
 *
 * @param data Fat pointer to inspect.
 * @param item Byte value to search for.
 * @returns index of the first match, or -1 if none is found.
 */
JSL_DEF int64_t jsl_fatptr_index_of(JSLFatPtr data, uint8_t item);

/**
 * Count the number of occurrences of `item` within a fat pointer.
 *
 * @note The comparison operates on raw code units. In UTF encodings, multiple code units can
 * form a single grapheme cluster, so the index does not necessarily map to user-perceived
 * characters. No Unicode normalization is performed; normalize inputs first if combining mark
 * equivalence is required.
 *
 * @param str Fat pointer to scan.
 * @param item Byte value to count.
 * @returns Total number of matches, or 0 when the sequence is empty.
 */
JSL_DEF int64_t jsl_fatptr_count(JSLFatPtr str, uint8_t item);

/**
 * Locate the final occurrence of `character` within a fat pointer.
 *
 * @note The comparison operates on raw code units. In UTF encodings, multiple code units can
 * form a single grapheme cluster, so the index does not necessarily map to user-perceived
 * characters. No Unicode normalization is performed; normalize inputs first if combining mark
 * equivalence is required.
 *
 * @param str Fat pointer to inspect.
 * @param character Byte value to search for.
 * @returns index of the last match, or -1 when no match exists.
 */
JSL_DEF int64_t jsl_fatptr_index_of_reverse(JSLFatPtr str, uint8_t character);

/**
 * Check whether `str` begins with the bytes stored in `prefix`.
 *
 * Returns `false` when either fat pointer is null or when `prefix` exceeds `str` in length.
 * An empty `prefix` yields `true`.
 *
 * @note The comparison operates on raw code units. In UTF encodings, multiple code units can
 * form a single grapheme cluster, so the index does not necessarily map to user-perceived
 * characters. No Unicode normalization is performed; normalize inputs first if combining mark
 * equivalence is required.
 *
 * @param str Candidate string to test.
 * @param prefix Sequence that must appear at the start of `str`.
 * @returns `true` if `str` starts with `prefix`, otherwise `false`.
 */
JSL_DEF bool jsl_fatptr_starts_with(JSLFatPtr str, JSLFatPtr prefix);

/**
 * Check whether `str` ends with the bytes stored in `postfix`.
 *
 * Returns `false` when either fat pointer is null or when `postfix` exceeds `str` in length.
 *
 * @note The comparison operates on raw code units. In UTF encodings, multiple code units can
 * form a single grapheme cluster, so the index does not necessarily map to user-perceived
 * characters. No Unicode normalization is performed; normalize inputs first if combining mark
 * equivalence is required.
 *
 * @param str Candidate string to test.
 * @param postfix Sequence that must appear at the end of `str`.
 * @returns `true` if `str` ends with `postfix`, otherwise `false`.
 */
JSL_DEF bool jsl_fatptr_ends_with(JSLFatPtr str, JSLFatPtr postfix);

/**
 * Get the file name from a filepath.
 *
 * Returns a view over the final path component that follows the last `/` byte in `filename`.
 * The resulting fat pointer aliases the original buffer; the data is neither copied nor
 * reallocated. If no `/` byte is present, or the suffix after the final `/` is fewer than two
 * code units (for example, a trailing `/` or a single-character basename), the original fat
 * pointer is returned unchanged.
 *
 * Like the other string utilities in this module, the search operates on raw code units. When
 * working with UTF encodings, code units do not necessarily correspond to grapheme clusters.
 * Normalize the input first if grapheme-aware behavior or Unicode canonical equivalence is
 * required.
 *
 * @param filename Fat pointer referencing the path or filename to inspect.
 * @returns Fat pointer referencing the basename or, in the fallback cases described above, the
 * original input pointer.
 *
 * @code
 * JSLFatPtr path = JSL_FATPTR_INITIALIZER("/tmp/example.txt");
 * JSLFatPtr base = jsl_fatptr_basename(path); // "example.txt"
 * @endcode
 */
JSL_DEF JSLFatPtr jsl_fatptr_basename(JSLFatPtr filename);

/**
 * Get the file extension from a file name or file path.
 *
 * Returns a view over the substring that follows the final `.` in `filename`.
 * The returned fat pointer reuses the original buffer; no allocations or copies
 * are performed. If `filename` does not contain a `.` byte, the result has a
 * `NULL` data pointer and a length of `0`.
 *
 * Like the other string utilities, the search operates on raw code units.
 * Paths encoded with multi-byte Unicode sequences are treated as opaque bytes,
 * and no normalization is performed. Normalize beforehand when grapheme-aware
 * behavior is required.
 *
 * @param filename Fat pointer referencing the path or filename to inspect.
 * @returns Fat pointer to the extension (excluding the dot) or an empty fat pointer
 * when no extension exists.
 *
 * @code
 * JSLFatPtr path = JSL_FATPTR_INITIALIZER("archive.tar.gz");
 * JSLFatPtr ext = jsl_fatptr_get_file_extension(path); // "gz"
 * @endcode
 */
JSL_DEF JSLFatPtr jsl_fatptr_get_file_extension(JSLFatPtr filename);

/**
 * Element by element comparison of the contents of the two fat pointers. If either
 * parameter has a null value for its data or a zero length, then this function will
 * return false.
 *
 * @note Do not use this to compare Unicode strings when grapheme based equality is
 * desired. Use this only when absolute byte equality is desired. See the note at the
 * top of the file about Unicode normalization.
 *
 * @warning This function should not be used in cryptographic contexts, like comparing
 * two password hashes. This function is vulnerable to timing attacks since it bails out
 * at the first inequality.
 *
 * @returns true if equal, false otherwise.
 */
JSL_DEF bool jsl_fatptr_memory_compare(JSLFatPtr a, JSLFatPtr b);

/**
 * Element by element comparison of the contents of a fat pointer and a null terminated
 * string. If the fat pointer has a null data value or a zero length, or if cstr is null,
 * then this function will return false. This is true even when both pointers are
 * NULL.
 *
 * @note Do not use this to compare Unicode strings when grapheme based equality is
 * desired. Use this only when absolute byte equality is desired. See the note at the
 * top of the file about Unicode normalization.
 *
 * @param a First comparator
 * @param cstr A valid null terminated string
 */
JSL_DEF bool jsl_fatptr_cstr_compare(JSLFatPtr a, char* cstr);

/**
 * Compare two fatptrs that both contain ASCII data for equality while ignoring case
 * differences. ASCII data validity is not checked.
 *
 * @returns true for equals, false for not equal
 */
JSL_DEF bool jsl_fatptr_compare_ascii_insensitive(JSLFatPtr a, JSLFatPtr b);

/**
 * Modify the ASCII data in the fatptr in place to change all capital letters to
 * lowercase. ASCII validity is not checked.
 */
JSL_DEF void jsl_fatptr_to_lowercase_ascii(JSLFatPtr str);

/**
 * Reads a 32 bit integer in base-10 from the beginning of `str`.
 * Accepted characters are 0-9, +, and -.
 *
 * Stops once it hits the first non-accepted character. This function does
 * not check for overflows or underflows. `result` is not written to if
 * there were no successfully parsed bytes.
 *
 * @param str a string with an int representation at the start
 * @param result out parameter where the parsing result will be stored
 * @return The number of bytes that were successfully read from the string
 */
JSL_DEF int32_t jsl_fatptr_to_int32(JSLFatPtr str, int32_t* result);

/**
 * Advance the fat pointer until the first non-whitespace character is
 * reached. If the fat pointer is null or has a negative length, -1 is
 * returned.
 *
 * @param str a fat pointer
 * @return The number of bytes that were advanced or -1
 */
JSL_DEF int64_t jsl_fatptr_strip_whitespace_left(JSLFatPtr* str);

/**
 * Reduce the fat pointer's length until the first non-whitespace character is
 * reached. If the fat pointer is null or has a negative length, -1 is
 * returned.
 *
 * @param str a fat pointer
 * @return The number of bytes that were advanced or -1
 */
JSL_DEF int64_t jsl_fatptr_strip_whitespace_right(JSLFatPtr* str);

/**
 * Modify the fat pointer such that it points to the part of the string
 * without any whitespace characters at the begining or the end. If the
 * fat pointer is null or has a negative length, -1 is returned.
 *
 * @param str a fat pointer
 * @return The number of bytes that were advanced or -1
 */
JSL_DEF int64_t jsl_fatptr_strip_whitespace(JSLFatPtr* str);

/**
 * Initialize an arena with the supplied buffer.
 *
 * @param arena Arena instance to initialize; must not be null.
 * @param memory Pointer to the beginning of the backing storage.
 * @param length Size of the backing storage in bytes.
 */
JSL_DEF void jsl_arena_init(JSLArena* arena, void* memory, int64_t length);

/**
 * Initialize an arena using a fat pointer as the backing buffer.
 *
 * This is a convenience overload for cases where the backing memory and its
 * length are already packaged in a `JSLFatPtr`.
 *
 * @param arena Arena to initialize; must not be null.
 * @param memory Backing storage for the arena; `memory.data` must not be null.
 */
JSL_DEF void jsl_arena_init2(JSLArena* arena, JSLFatPtr memory);

/**
 * Allocate a block of memory from the arena using the default alignment.
 *
 * The returned fat pointer contains a null data pointer if the arena does not
 * have enough capacity. When `zeroed` is true, the allocated bytes are
 * zero-initialized.
 *
 * @param arena Arena to allocate from; must not be null.
 * @param bytes Number of bytes to reserve.
 * @param zeroed When true, zero-initialize the allocation.
 * @return Fat pointer describing the allocation or `{0}` on failure.
 */
JSL_DEF JSLFatPtr jsl_arena_allocate(JSLArena* arena, int64_t bytes, bool zeroed);

/**
 * Allocate a block of memory from the arena with the provided alignment.
 *
 * @param arena Arena to allocate from; must not be null.
 * @param bytes Number of bytes to reserve.
 * @param alignment Desired alignment in bytes; must be a positive power of two.
 * @param zeroed When true, zero-initialize the allocation.
 * @return Fat pointer describing the allocation or `{0}` on failure.
 */
JSL_DEF JSLFatPtr jsl_arena_allocate_aligned(JSLArena* arena, int64_t bytes, int32_t alignment, bool zeroed);

/**
 * Macro to make it easier to allocate an instance of `T` within an arena.
 *
 * @param T Type to allocate.
 * @param arena Arena to allocate from; must be initialized.
 * @return Pointer to the allocated object or `NULL` on failure.
 *
 * ```
 * struct MyStruct { uint64_t the_data; };
 * struct MyStruct* thing = JSL_ARENA_TYPED_ALLOCATE(struct MyStruct, arena);
 * ```
 */
#define JSL_ARENA_TYPED_ALLOCATE(T, arena) (T*) jsl_arena_allocate_aligned(arena, sizeof(T), _Alignof(T), false).data

/**
 * Macro to make it easier to allocate a zero filled array of `T` within an arena.
 *
 * @param T Type to allocate.
 * @param arena Arena to allocate from; must be initialized.
 * @return Pointer to the allocated object or `NULL` on failure.
 *
 * @code
 * struct MyStruct { uint64_t the_data; };
 * struct MyStruct* thing_array = JSL_ARENA_TYPED_ALLOCATE(struct MyStruct, arena, 42);
 * @endcode
 */
#define JSL_ARENA_TYPED_ARRAY_ALLOCATE(T, arena, length) (T*) jsl_arena_allocate_aligned(arena, (int64_t) sizeof(T) * length, _Alignof(T), true).data

/**
 * Resize the allocation if it was the last allocation, otherwise, allocate a new
 * chunk of memory and copy the old allocation's contents.
 */
JSL_DEF JSLFatPtr jsl_arena_reallocate(
    JSLArena* arena,
    JSLFatPtr original_allocation,
    int64_t new_num_bytes
);

/**
 * Resize the allocation if it was the last allocation, otherwise, allocate a new
 * chunk of memory and copy the old allocation's contents.
 */
JSL_DEF JSLFatPtr jsl_arena_reallocate_aligned(
    JSLArena* arena,
    JSLFatPtr original_allocation,
    int64_t new_num_bytes,
    int32_t align
);

/**
 * Set the current pointer back to the start of the arena.
 *
 * In debug mode, this function will set all of the memory that was
 * allocated to `0xfeeefeee` to help detect use after free bugs.
 */
JSL_DEF void jsl_arena_reset(JSLArena* arena);

/**
 * The functions jsl_arena_save_restore_point and jsl_arena_load_restore_point
 * help you make temporary allocations inside an existing arena. You can think of
 * it as an "arena inside an arena". Basically the save function marks the current
 * state of the arena and the load function sets the saved state to the given arena,
 * wiping out any allocations which happened in the interim.
 *
 * This is very useful when you need memory from the arena but only for a specific
 * function.
 *
 * For example, say you have an existing one megabyte arena that has used 128 kilobytes
 * of space. You then call a function with this arena which needs a string to make an
 * operating system call, but that string is no longer needed after the function returns.
 * You can "save" and "load" a restore point at the start and end of the function
 * (respectively) and when the function returns, the arena will still only have 128
 * kilobytes used.
 *
 * In debug mode, jsl_arena_load_restore_point function will set all of the memory
 * that was allocated to `0xfeeefeee` to help detect use after free bugs.
 */
JSL_DEF uint8_t* jsl_arena_save_restore_point(JSLArena* arena);

/**
 * The functions jsl_arena_save_restore_point and jsl_arena_load_restore_point
 * help you make temporary allocations inside an existing arena. You can think of
 * it as an "arena inside an arena". Basically the save function marks the current
 * state of the arena and the load function sets the saved state to the given arena,
 * wiping out any allocations which happened in the interim.
 *
 * This is very useful when you need memory from the arena but only for a specific
 * function.
 *
 * For example, say you have an existing one megabyte arena that has used 128 kilobytes
 * of space. You then call a function with this arena which needs a string to make an
 * operating system call, but that string is no longer needed after the function returns.
 * You can "save" and "load" a restore point at the start and end of the function
 * (respectively) and when the function returns, the arena will still only have 128
 * kilobytes used.
 *
 * In debug mode, jsl_arena_load_restore_point function will set all of the memory
 * that was allocated to `0xfeeefeee` to help detect use after free bugs.
 * 
 * This function will assert if you attempt to use a different restore point from an
 * arena different that the one passed in as the parameter.
 */
JSL_DEF void jsl_arena_load_restore_point(JSLArena* arena, uint8_t* restore_point);

/**
 * Allocate a new buffer from the arena and copy the contents of a fat pointer with
 * a null terminator.
 */
JSL_DEF char* jsl_fatptr_to_cstr(JSLArena* arena, JSLFatPtr str);

/**
 * Allocate and copy the contents of a fat pointer with a null terminator.
 *
 * @note Use `jsl_fatptr_from_cstr` to make a fat pointer without copying.
 */
JSL_DEF JSLFatPtr jsl_cstr_to_fatptr(JSLArena* arena, char* str);

/**
 * Allocate space for, and copy the contents of a fat pointer.
 *
 * @note Use `jsl_cstr_to_fatptr` to copy a c string into a fatptr.
 */
JSL_DEF JSLFatPtr jsl_fatptr_duplicate(JSLArena* arena, JSLFatPtr str);

#ifndef JSL_FORMAT_MIN_BUFFER
    #define JSL_FORMAT_MIN_BUFFER 512 // how many characters per callback
#endif

/**
 * Function signature for receiving formatted output from `jsl_format_callback`.
 *
 * The formatter hands over `len` bytes starting at `buf` each time the internal
 * buffer fills up or is flushed. The callback should consume those bytes (copy,
 * write, etc.) and then return a pointer to the buffer that should be used for
 * subsequent writes. Returning `NULL` signals an error or early termination,
 * causing `jsl_format_callback` to stop producing output.
 *
 * @param buf   Pointer to `len` bytes of freshly formatted data.
 * @param user  Opaque pointer that was supplied to `jsl_format_callback`.
 * @param len   Number of valid bytes in `buf`.
 * @return Pointer to the buffer that will receive the next chunk, or `NULL` to stop.
 */
typedef uint8_t* JSL_FORMAT_CALLBACK(uint8_t* buf, void *user, int64_t len);

/**
 * This is a full snprintf replacement that supports everything that the C
 * runtime snprintf supports, including float/double, 64-bit integers, hex
 * floats, field parameters (%*.*d stuff), length reads backs, etc.
 *
 * This returns the number of bytes written.
 *
 * There are a set of different functions for different use cases
 *
 * * jsl_format
 * * jsl_format_buffer
 * * jsl_format_valist
 * * jsl_format_callback
 * * jsl_string_builder_format
 *
 * ## Fat Pointers
 *
 * Fat pointers can be written into the resulting string using the `%y`
 * format specifier. This works exactly the same way as `%.*s`. Keep in
 * mind that, much like `snprintf`, this function is limited to 32bit
 * unsigned indicies, so fat pointers over 4gb will underflow.
 *
 * ## Floating Point
 *
 * This code uses a internal float->ascii conversion method that uses
 * doubles with error correction (double-doubles, for ~105 bits of
 * precision).  This conversion is round-trip perfect - that is, an atof
 * of the values output here will give you the bit-exact double back.
 *
 * One difference is that our insignificant digits will be different than
 * with MSVC or GCC (but they don't match each other either).  We also
 * don't attempt to find the minimum length matching float (pre-MSVC15
 * doesn't either).
 *
 * ## 64 Bit ints
 *
 * This library also supports 64-bit integers and you can use MSVC style or
 * GCC style indicators (%I64d or %lld).  It supports the C99 specifiers
 * for uint64_t and ptr_diff_t (%jd %zd) as well.
 *
 * ## Extras
 *
 * Like some GCCs, for integers and floats, you can use a ' (single quote)
 * specifier and commas will be inserted on the thousands: "%'d" on 12345
 * would print 12,345.
 *
 * For integers and floats, you can use a "$" specifier and the number
 * will be converted to float and then divided to get kilo, mega, giga or
 * tera and then printed, so "%$d" 1000 is "1.0 k", "%$.2d" 2536000 is
 * "2.53 M", etc. For byte values, use two $:s, like "%$$d" to turn
 * 2536000 to "2.42 Mi". If you prefer JEDEC suffixes to SI ones, use three
 * $:s: "%$$$d" -> "2.42 M". To remove the space between the number and the
 * suffix, add "_" specifier: "%_$d" -> "2.53M".
 *
 * In addition to octal and hexadecimal conversions, you can print
 * integers in binary: "%b" for 256 would print 100.
 *
 * ## Caveat
 *
 * The internal counters are all unsigned 32 byte values, so if for some reason
 * you're using this function to print multiple gigabytes at a time, break it
 * into chunks.
 */
JSL_DEF JSLFatPtr jsl_format(JSLArena* arena, JSLFatPtr fmt, ...);

/**
 * See docs for jsl_format.
 *
 * Writes into a provided buffer, up to `buffer.length` bytes.
 */
JSL_DEF int64_t jsl_format_buffer(
    JSLFatPtr* buffer,
    JSLFatPtr fmt,
    ...
);

/**
 * See docs for jsl_format.
 *
 * Writes into a provided buffer, up to `buffer.length` bytes using a variadic
 * argument list.
 */
JSL_DEF int64_t jsl_format_valist(
    JSLFatPtr* buffer,
    JSLFatPtr fmt,
    va_list va
);

/**
 * See docs for jsl_format.
 *
 * Convert into a buffer, calling back every JSL_FORMAT_MIN_BUFFER chars.
 * Your callback can then copy the chars out, print them or whatever.
 * This function is actually the workhorse for everything else.
 * The buffer you pass in must hold at least JSL_FORMAT_MIN_BUFFER characters.
 *
 * You return the next buffer to use or 0 to stop converting
 */
JSL_DEF int64_t jsl_format_callback(
   JSL_FORMAT_CALLBACK* callback,
   void* user,
   uint8_t* buf,
   JSLFatPtr fmt,
   va_list va
);

/**
 * Set the comma and period characters to use for the current thread.
 */
// TODO: incomplete!
JSL_DEF void jsl_format_set_separators(char comma, char period);

#ifdef __cplusplus
} /* extern "C" */
#endif

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
 * `JSL_INCLUDE_FILE_UTILS` - Include the file loading and writing utilities. These
 * require linking the standard library.
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


#ifndef JACKS_STANDARD_LIBRARY

#define JACKS_STANDARD_LIBRARY

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

    #error "ERROR: jacks_standard_library.h does not support big endian targets!"

#endif

#ifndef JSL_VERSION
    #define JSL_VERSION 0x010000  /* 1.0.0 */
#else
    #if JSL_VERSION != 0x010000
        #error "ERROR: Conflicting versions of jacks_standard_library.h included in the same translation unit!"
    #endif
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
        #error "ERROR: jacks_standard_library.h can only be used with 32 or 64 bit pointers"
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
    #error "ERROR: jacks_standard_library.h can only be used with 32 or 64 bit pointers"
#endif

#ifndef JSL_DEBUG
    #ifndef JSL__FORCE_INLINE

        #if JSL_IS_MSVC
            #define JSL__FORCE_INLINE __forceinline
        #elif JSL_IS_CLANG || JSL_IS_GCC
            #define JSL__FORCE_INLINE inline __attribute__((__always_inline__))
        #else
            #define JSL__FORCE_INLINE
        #endif

    #endif
#else
    #define JSL__FORCE_INLINE
#endif

#ifndef JSL__LIKELY

    #if JSL_IS_CLANG || JSL_IS_GCC
        #define JSL__LIKELY(x) __builtin_expect(!!(x), 1)
    #else
        #define JSL__LIKELY(x) (x)
    #endif

#endif

#ifndef JSL__UNLIKELY

    #if JSL_IS_CLANG || JSL_IS_GCC
        #define JSL__UNLIKELY(x) __builtin_expect(!!(x), 0)
    #else
        #define JSL__UNLIKELY(x) (x)
    #endif

#endif

#if defined(__SANITIZE_ADDRESS__) && __SANITIZE_ADDRESS__
    #include <sanitizer/asan_interface.h>
#else
    #define ASAN_POISON_MEMORY_REGION(ptr, len)
    #define ASAN_UNPOISON_MEMORY_REGION(ptr, len)
#endif

#if JSL_IS_CLANG || JSL_IS_GCC
    #if defined(__SANITIZE_ADDRESS__) && __SANITIZE_ADDRESS__
        #define JSL__ASAN_OFF __attribute__((__no_sanitize_address__))
    #endif
#endif

#ifndef JSL__ASAN_OFF
    #define JSL__ASAN_OFF
#endif


/**
 *
 *  Built Ins Macros For MSVC Compatibility
 *
 */


#if JSL_IS_GCC || JSL_IS_CLANG
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

#elif JSL_IS_MSVC

    uint32_t jsl__count_trailing_zeros_u32(uint32_t x);
    uint32_t jsl__count_trailing_zeros_u64(uint64_t x);
    uint32_t jsl__find_first_set_u32(uint32_t x);
    uint32_t jsl__find_first_set_u64(uint64_t x);

    #define JSL__COUNT_TRAILING_ZEROS_IMPL(x) jsl__count_trailing_zeros_u32(x)
    #define JSL__COUNT_TRAILING_ZEROS_IMPL64(x) jsl__count_trailing_zeros_u64(x)
    #define JSL__COUNT_LEADING_ZEROS_IMPL(x) __lzcnt(x)
    #define JSL__COUNT_LEADING_ZEROS_IMPL64(x) __lzcnt64(x)
    #define JSL__POPULATION_COUNT_IMPL(x) __popcnt(x)
    #define JSL__POPULATION_COUNT_IMPL64(x) __popcnt64(x)
    #define JSL__FIND_FIRST_SET_IMPL(x) jsl__find_first_set_u32(x)
    #define JSL__FIND_FIRST_SET_IMPL64(x) jsl__find_first_set_u64(x)

#else

    uint32_t jsl__count_trailing_zeros_u32(uint32_t x);
    uint32_t jsl__count_trailing_zeros_u64(uint64_t x);
    uint32_t jsl__count_leading_zeros_u32(uint32_t x);
    uint32_t jsl__count_leading_zeros_u64(uint64_t x);
    uint32_t jsl__population_count_u32(uint32_t x);
    uint32_t jsl__population_count_u64(uint64_t x);
    uint32_t jsl__find_first_set_u32(uint32_t x);
    uint32_t jsl__find_first_set_u64(uint64_t x);

    #define JSL__COUNT_TRAILING_ZEROS_IMPL(x) jsl__count_trailing_zeros_u32(x)
    #define JSL__COUNT_TRAILING_ZEROS_IMPL64(x) jsl__count_trailing_zeros_u64(x)
    #define JSL__COUNT_LEADING_ZEROS_IMPL(x) jsl__count_leading_zeros_u32(x)
    #define JSL__COUNT_LEADING_ZEROS_IMPL64(x) jsl__count_leading_zeros_u64(x)
    #define JSL__POPULATION_COUNT_IMPL(x) jsl__population_count_u32(x)
    #define JSL__POPULATION_COUNT_IMPL64(x) jsl__population_count_u64(x)
    #define JSL__FIND_FIRST_SET_IMPL(x) jsl__find_first_set_u32(x)
    #define JSL__FIND_FIRST_SET_IMPL64(x) jsl__find_first_set_u64(x)

#endif


/**
 *
 *
 *                      PUBLIC API
 *
 *
 */


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
 * Round x up to the next power of two. If x is a power of two it returns
 * the same value.
 *
 * This function is designed to be used in tight loops and other performance
 * critical areas. Therefore, both zero and values greater than 2^31 not special
 * cased. The return values for these cases are compiler, OS, and ISA specific.
 * If you need consistent behavior, then you can easily call this function like
 * so:
 *
 * ```
 * jsl_next_power_of_two_u32(
 *      JSL_BETWEEN(1u, x, 0x80000000u)
 * );
 * ```
 *
 * @param x The value to round up
 * @returns the next power of two
 */
uint32_t jsl_next_power_of_two_u32(uint32_t x);

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
 * because the stack pointer will be reset at the end of the function.
 *
 * ```
 * void some_func(void)
 * {
 *      uint8_t buffer[16 * 1024];
 *      JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 *
 *      // example hash map, not real
 *      IntToStrMap map = int_to_str_ctor(&arena);
 *      int_to_str_add(&map, 64, JSL_FATPTR_INITIALIZER("This is my string data!"));
 *
 *      // hash map cleaned up automatically
 * }
 * ```
 *
 * Fast, cheap, easy automatic memory management!
 *
 * @warning This macro only works for variable initializers and cannot be used as a
 * normal rvalue.
 */
#define JSL_ARENA_FROM_STACK(buf) { (uint8_t *)(buf), (uint8_t *)(buf), (uint8_t *)(buf) + sizeof(buf) }

/**
 * A string builder is a container for building large strings. It's specialized for
 * situations where many different smaller operations result in small strings being
 * coalesced into a final result, specifically using an arena as its allocator.
 *
 * While this is called string builder, the underlying data store is just bytes, so
 * any binary data which is built in chunks can use the string builder.
 *
 * ## Implementation
 *
 * A string builder is different from a normal dynamic array in two ways. One, it
 * has specific operations for writing string data in both fat pointer form but also
 * as a `snprintf` like operation. Two, the resulting string data is not stored as a
 * contiguous range of memory, but as a series of chunks which is given to the user
 * as an iterator when the string is finished.
 *
 * This is due to the nature of arena allocations. If you have some part of your
 * program which generates string output, the most common form of that code would be:
 *
 * 1. You do some operations, these operations themselves allocate
 * 2. You generate a string from the operations
 * 3. The string is concatenated into some buffer
 * 4. Repeat
 *
 * A dynamically sized array which grows would mean throwing away the old memory when
 * the array resizes. This would be fine for your typical heap but for an arena this
 * the old memory is unavailable until the arena is reset. A separate arena that's
 * used purely for the array would work, but that sort of defeats the whole purpose
 * of an arena, which is it's supposed to make lifetime tracking easier. Having a
 * whole bunch of separate arenas for different objects makes the program more
 * complicated than it should be.
 *
 * Having the memory in chunks means that a single arena is not wasteful with its
 * available memory.
 *
 * By default, each chunk is 256 bytes and is aligned to a 8 byte address. These are
 * tuneable parameters that you can set during init. The custom alignment helps if you
 * want to use SIMD code on the consuming code.
 *
 * ## Functions
 *
 * * jsl_string_builder_init
 * * jsl_string_builder_init2
 * * jsl_string_builder_insert_char
 * * jsl_string_builder_insert_uint8_t
 * * jsl_string_builder_insert_fatptr
 * * jsl_string_builder_format
 */
typedef struct JSLStringBuilder
{
    JSLArena* arena;
    struct JSLStringBuilderChunk* head;
    struct JSLStringBuilderChunk* tail;
    int32_t alignment;
    int32_t chunk_size;
} JSLStringBuilder;

/**
 * The iterator type for a JSLStringBuilder instance. This keeps track of
 * where the iterator is over the course of calling the next function.
 *
 * @warning It is not valid to modify a string builder while iterating over it.
 *
 * Functions:
 *
 * * jsl_string_builder_iterator_init
 * * jsl_string_builder_iterator_next
 */
typedef struct JSLStringBuilderIterator
{
    struct JSLStringBuilderChunk* current;
} JSLStringBuilderIterator;


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
 * This function checks for NULL and checks that `writer_fatptr` points to data
 * in `original_fatptr`. If either of these checks fail, then `-1` is returned.
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
 * the size and pointer in `writer_fatptr`. If either parameter has a NULL data
 * field, has a negative length, or if the writer does not point to a portion
 * of the original allocation, this function will return a fat pointer with a
 * `NULL` data pointer.
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
JSL_DEF JSLFatPtr jsl_fatptr_from_cstr(char* str);

/**
 * Copy the contents of `source` into `destination`.
 *
 * This function is bounds checked, meaning a max of `destination->length` bytes
 * will be copied into `destination`. This function also checks for overlapping
 * buffers, null pointers in either `destination` or `source`, and negative lengths.
 * In all these cases, -1 will be returned.
 *
 * `destination` is modified to point to the remaining data in the buffer. I.E.
 * if the entire buffer was used then `destination->length` will be `0` and
 * `destination->data` will be pointing to the end of the buffer.
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
    char* cstring,
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
 * @code
 * struct MyStruct { uint64_t the_data; };
 * struct MyStruct* thing = JSL_ARENA_TYPED_ALLOCATE(struct MyStruct, arena);
 * @endcode
 */
#define JSL_ARENA_TYPED_ALLOCATE(T, arena) (T*) jsl_arena_allocate(arena, sizeof(T), false).data

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

/**
 * Initialize a JSLStringBuilder using the default settings. See the JSLStringBuilder
 * for more information on the container. A chunk is allocated right away and if that
 * fails this returns false.
 *
 * @param builder The builder instance to initialize; must not be NULL.
 * @param arena The arena that backs all allocations made by the builder; must not be NULL.
 * @return `true` if the builder was initialized successfully, otherwise `false`.
 */
bool jsl_string_builder_init(JSLStringBuilder* builder, JSLArena* arena);

/**
 * Initialize a JSLStringBuilder with a custom chunk size and chunk allocation alignment.
 * See the JSLStringBuilder for more information on the container. A chunk is allocated
 * right away and if that fails this returns false.
 *
 * @param builder The builder instance to initialize; must not be NULL.
 * @param arena The arena that backs all allocations made by the builder; must not be NULL.
 * @param chunk_size The number of bytes that are allocated each time the container needs to grow
 * @param alignment The allocation alignment of the chunks of data
 * @returns `true` if the builder was initialized successfully, otherwise `false`.
 */
bool jsl_string_builder_init2(JSLStringBuilder* builder, JSLArena* arena, int32_t chunk_size, int32_t alignment);

/**
 * Append a char value to the end of the string builder without interpretation. Each append
 * may result in an allocation if there's no more space. If that allocation fails then this
 * function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param c The byte to append.
 * @returns `true` if the byte was inserted successfully, otherwise `false`.
 */
bool jsl_string_builder_insert_char(JSLStringBuilder* builder, char c);

/**
 * Append a single raw byte to the end of the string builder without interpretation.
 * The value is written as-is, so it can be used for arbitrary binary data, including
 * zero bytes. Each append may result in an allocation if there's no more space. If
 * that allocation fails then this function returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param c The byte to append.
 * @returns `true` if the byte was inserted successfully, otherwise `false`.
 */
bool jsl_string_builder_insert_uint8_t(JSLStringBuilder* builder, uint8_t c);

/**
 * Append the contents of a fat pointer. Additional chunks are allocated as needed
 * while copying so if any of the allocations fail this returns false.
 *
 * @param builder The string builder to append to; must be initialized.
 * @param data A fat pointer describing the bytes to copy; its length may be zero.
 * @returns `true` if the data was appended successfully, otherwise `false`.
 */
bool jsl_string_builder_insert_fatptr(JSLStringBuilder* builder, JSLFatPtr data);

/**
 * Format a string using the jsl_format logic and write the result directly into
 * the string builder.
 *
 * @param builder The string builder that receives the formatted output; must be initialized.
 * @param fmt A fat pointer describing the format string.
 * @param ... Variadic arguments consumed by the formatter.
 * @returns `true` if formatting succeeded and the formatted bytes were appended, otherwise `false`.
 */
bool jsl_string_builder_format(JSLStringBuilder* builder, JSLFatPtr fmt, ...);

/**
 * Initialize an iterator instance so it will traverse the given string builder
 * from the begining. It's easiest to just put an empty iterator on the stack
 * and then call this function.
 *
 * ```
 * JSLStringBuilder builder = ...;
 *
 * JSLStringBuilderIterator iter;
 * jsl_string_builder_iterator_init(&builder, &iter);
 * ```
 *
 * @param builder    The string builder whose data will be traversed.
 * @param iterator   The iterator instance to initialize.
 */
void jsl_string_builder_iterator_init(JSLStringBuilder* builder, JSLStringBuilderIterator* iterator);

/**
 * Get the next chunk of data a string builder iterator. The chunk will
 * have a `NULL` data pointer when iteration is over.
 *
 * This example program prints all the data in a string builder to stdout:
 *
 * ```
 * #include <stdio.h>
 *
 * JSLStringBuilder builder = ...;
 *
 * JSLStringBuilderIterator iter;
 * jsl_string_builder_iterator_init(&builder, &iter);
 *
 * while (true)
 * {
 *      JSLFatPtr str = jsl_string_builder_iterator_next(&iter);
 *
 *      if (str.data == NULL)
 *          break;
 *
 *      jsl_format_file(stdout, str);
 * }
 * ```
 *
 * @param iterator   The iterator instance
 * @returns The next chunk of data from the string builder
 */
JSLFatPtr jsl_string_builder_iterator_next(JSLStringBuilderIterator* iterator);

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

#ifdef JSL_INCLUDE_FILE_UTILS

    #if JSL_IS_WINDOWS

        #include <errno.h>
        #include <fcntl.h>
        #include <limits.h>
        #include <fcntl.h>
        #include <stdio.h>
        #include <io.h>
        #include <share.h>
        #include <sys\stat.h>

    #elif JSL_IS_POSIX

        #include <errno.h>
        #include <limits.h>
        #include <fcntl.h>
        #include <stdio.h>
        #include <unistd.h>
        #include <sys/types.h>
        #include <sys/stat.h>

    #endif

    typedef enum
    {
        JSL_GET_FILE_SIZE_BAD_PARAMETERS = 0,
        JSL_GET_FILE_SIZE_OK,
        JSL_GET_FILE_SIZE_NOT_FOUND,
        JSL_GET_FILE_SIZE_NOT_REGULAR_FILE,

        JSL_GET_FILE_SIZE_ENUM_COUNT
    } JSLGetFileSizeResultEnum;

    typedef enum
    {
        JSL_FILE_LOAD_BAD_PARAMETERS,
        JSL_FILE_LOAD_SUCCESS,
        JSL_FILE_LOAD_COULD_NOT_OPEN,
        JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE,
        JSL_FILE_LOAD_COULD_NOT_GET_MEMORY,
        JSL_FILE_LOAD_READ_FAILED,
        JSL_FILE_LOAD_CLOSE_FAILED,
        JSL_FILE_LOAD_ERROR_UNKNOWN,

        JSL_FILE_LOAD_ENUM_COUNT
    } JSLLoadFileResultEnum;

    typedef enum
    {
        JSL_FILE_WRITE_BAD_PARAMETERS = 0,
        JSL_FILE_WRITE_SUCCESS,
        JSL_FILE_WRITE_COULD_NOT_OPEN,
        JSL_FILE_WRITE_COULD_NOT_WRITE,
        JSL_FILE_WRITE_COULD_NOT_CLOSE,

        JSL_FILE_WRITE_ENUM_COUNT
    } JSLWriteFileResultEnum;

    typedef enum {
        JSL_FILE_TYPE_UNKNOWN = 0,
        JSL_FILE_TYPE_REG,
        JSL_FILE_TYPE_DIR,
        JSL_FILE_TYPE_SYMLINK,
        JSL_FILE_TYPE_BLOCK,
        JSL_FILE_TYPE_CHAR,
        JSL_FILE_TYPE_FIFO,
        JSL_FILE_TYPE_SOCKET,

        JSL_FILE_TYPE_COUNT
    } JSLFileTypeEnum;

    /**
     * Get the file size in bytes from the file at `path`. 
     * 
     * @param path The file system path
     * @param out_size Pointer where the resulting size will be stored, must not be null
     * @param out_os_error_code Pointer where an error code will be stored when applicable. Can be null
     * @returns An enum which denotes success or failure
     */
    JSL_WARN_UNUSED JSLGetFileSizeResultEnum jsl_get_file_size(
        JSLFatPtr path,
        int64_t* out_size,
        int32_t* out_os_error_code
    );

    /**
     * Load the contents of the file at `path` into a newly allocated buffer
     * from the given arena. The buffer will be the exact size of the file contents.
     *
     * If the arena does not have enough space,
     */
    JSL_WARN_UNUSED JSL_DEF JSLLoadFileResultEnum jsl_load_file_contents(
        JSLArena* arena,
        JSLFatPtr path,
        JSLFatPtr* out_contents,
        int32_t* out_errno
    );

    /**
     * Load the contents of the file at `path` into an existing fat pointer buffer.
     *
     * Copies up to `buffer->length` bytes into `buffer->data` and advances the fat
     * pointer by the amount read so the caller can continue writing into the same
     * backing storage. Returns a `JSLLoadFileResultEnum` describing the outcome and
     * optionally stores the system `errno` in `out_errno` on failure.
     *
     * @param buffer buffer to write to
     * @param path The file system path
     * @param out_errno A pointer which will be written to with the errno on failure
     * @returns An enum which represents the result
     */
    JSL_WARN_UNUSED JSL_DEF JSLLoadFileResultEnum jsl_load_file_contents_buffer(
        JSLFatPtr* buffer,
        JSLFatPtr path,
        int32_t* out_errno
    );

    /**
     * Write the bytes in `contents` to the file located at `path`.
     *
     * Opens or creates the destination file and attempts to write the entire
     * contents buffer. Returns a `JSLWriteFileResultEnum` describing the
     * outcome, stores the number of bytes written in `bytes_written` when
     * provided, and optionally writes the failing `errno` into `out_errno`.
     *
     * @param contents Data to be written to disk
     * @param path File system path to write to
     * @param bytes_written Optional pointer that receives the bytes written on success
     * @param out_errno Optional pointer that receives the system errno on failure
     * @returns A result enum describing the write outcome
     */
    JSL_WARN_UNUSED JSL_DEF JSLWriteFileResultEnum jsl_write_file_contents(
        JSLFatPtr contents,
        JSLFatPtr path,
        int64_t* bytes_written,
        int32_t* out_errno
    );

    /**
     * Format a string using the JSL formatter and write the result to a `FILE*`,
     * most often this will be `stdout`.
     *
     * Streams that reject writes (for example, read-only streams or closed
     * pipes) cause the function to return `false`. Passing a `NULL` file handle,
     * a `NULL` format pointer, or a negative format length also causes failure.
     *
     * @param out Destination stream
     * @param fmt Format string
     * @param ... Format args
     * @returns `true` when formatting and writing succeeds, otherwise `false`
     */
    JSL__ASAN_OFF JSL_DEF bool jsl_format_file(FILE* out, JSLFatPtr fmt, ...);

#endif // JSL_INCLUDE_FILE_UTILS

#ifdef __cplusplus
} /* extern "C" */
#endif




/**
 *
 *
 *                      IMPLEMENTATION
 *
 *
 */




 #ifdef JSL_IMPLEMENTATION

    #if JSL_IS_X86
        #include <immintrin.h>
    #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
        #include <arm_neon.h>
    #elif JSL_IS_WEB_ASSEMBLY && defined(__wasm_simd128__)
        #include <wasm_simd128.h>
    #endif

    #if JSL_IS_MSVC
        #include <intrin.h>
    #endif

    #if defined(__ARM_NEON) || defined(__ARM_NEON__)
        static inline uint32_t jsl__neon_movemask(uint8x16_t v)
        {
            const uint8x8_t weights = {1,2,4,8,16,32,64,128};

            uint8x16_t msb = vshrq_n_u8(v, 7);
            uint16x8_t lo16 = vmull_u8(vget_low_u8(msb),  weights);
            uint16x8_t hi16 = vmull_u8(vget_high_u8(msb), weights);

            uint32_t lower = vaddvq_u16(lo16);   // reduce 8×u16 -> scalar
            uint32_t upper = vaddvq_u16(hi16);

            return lower | (upper << 8);
        }
    #endif

    void jsl__assert(int condition, char* file, int line)
    {
        (void) file;
        (void) line;
        assert(condition);
    }

    JSL__FORCE_INLINE uint32_t jsl__count_trailing_zeros_u32(uint32_t x)
    {
        #if JSL_IS_MSVC
            unsigned long index;
            _BitScanForward(&index, x);
            return (uint32_t) index;
        #else
            uint32_t n = 0;
            while ((x & 1u) == 0)
            {
                x >>= 1;
                ++n;
            }
            return n;
        #endif
    }

    JSL__FORCE_INLINE uint32_t jsl__count_trailing_zeros_u64(uint64_t x)
    {
        #if JSL_IS_MSVC
            unsigned long index;
            _BitScanForward64(&index, x);
            return (uint32_t) index;
        #else
            uint32_t n = 0;
            while ((x & 1u) == 0)
            {
                x >>= 1;
                ++n;
            }
            return n;
        #endif
    }

    JSL__FORCE_INLINE uint32_t jsl__count_leading_zeros_u32(uint32_t x)
    {
        if (x == 0) return 32;
        uint32_t n = 0;
        if ((x & 0xFFFF0000) == 0) { n += 16; x <<= 16; }
        if ((x & 0xFF000000) == 0) { n += 8;  x <<= 8;  }
        if ((x & 0xF0000000) == 0) { n += 4;  x <<= 4;  }
        if ((x & 0xC0000000) == 0) { n += 2;  x <<= 2;  }
        if ((x & 0x80000000) == 0) { n += 1; }
        return n;
    }

    JSL__FORCE_INLINE uint32_t jsl__count_leading_zeros_u64(uint64_t x)
    {
        if (x == 0) return 64;
        uint32_t n = 0;
        if ((x & 0xFFFFFFFF00000000ULL) == 0) { n += 32; x <<= 32; }
        if ((x & 0xFFFF000000000000ULL) == 0) { n += 16; x <<= 16; }
        if ((x & 0xFF00000000000000ULL) == 0) { n += 8;  x <<= 8;  }
        if ((x & 0xF000000000000000ULL) == 0) { n += 4;  x <<= 4;  }
        if ((x & 0xC000000000000000ULL) == 0) { n += 2;  x <<= 2;  }
        if ((x & 0x8000000000000000ULL) == 0) { n += 1; }
        return n;
    }

    JSL__FORCE_INLINE uint32_t jsl__find_first_set_u32(uint32_t x)
    {
        #if JSL_IS_MSVC
            unsigned long index;
            _BitScanForward(&index, x);
            return (uint32_t) (index + 1);
        #else
            if (x == 0) return 0;

            uint32_t n = 1;
            if ((x & 0xFFFF) == 0) { x >>= 16; n += 16; }
            if ((x & 0xFF) == 0)   { x >>= 8;  n += 8;  }
            if ((x & 0xF) == 0)    { x >>= 4;  n += 4;  }
            if ((x & 0x3) == 0)    { x >>= 2;  n += 2;  }
            if ((x & 0x1) == 0)    { n += 1; }

            return n;
        #endif
    }

    JSL__FORCE_INLINE uint32_t jsl__find_first_set_u64(uint64_t x)
    {
        #if JSL_IS_MSVC
            unsigned long index;
            _BitScanForward64(&index, x);
            return (uint32_t) (index + 1);
        #else
            if (x == 0) return 0;

            uint32_t n = 1;
            if ((x & 0xFFFFFFFFULL) == 0) { x >>= 32; n += 32; }
            if ((x & 0xFFFFULL) == 0)     { x >>= 16; n += 16; }
            if ((x & 0xFFULL) == 0)       { x >>= 8;  n += 8;  }
            if ((x & 0xFULL) == 0)        { x >>= 4;  n += 4;  }
            if ((x & 0x3ULL) == 0)        { x >>= 2;  n += 2;  }
            if ((x & 0x1ULL) == 0)        { n += 1; }

            return n;
        #endif
    }

    JSL__FORCE_INLINE uint32_t jsl__population_count_u32(uint32_t x)
    {
        // Branchless SWAR
        x = x - ((x >> 1) & 0x55555555u);
        x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
        x = (x + (x >> 4)) & 0x0F0F0F0Fu;
        x = x + (x >> 8);
        x = x + (x >> 16);
        return x & 0x3Fu;
    }

    JSL__FORCE_INLINE uint32_t jsl__population_count_u64(uint64_t x)
    {
        x = x - ((x >> 1) & 0x5555555555555555ULL);
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
        x = x + (x >> 8);
        x = x + (x >> 16);
        x = x + (x >> 32);
        return x & 0x7F;  // result fits in 7 bits (0–64)
    }

    uint32_t jsl_next_power_of_two_u32(uint32_t x)
    {
        #if JSL_IS_CLANG || JSL_IS_GCC

            return 1u << (32 - JSL_PLATFORM_COUNT_LEADING_ZEROS(x - 1));

        #else

            x--;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            x++;
            return x;

        #endif
    }

    uint64_t jsl_next_power_of_two_u64(uint64_t x)
    {
        #if JSL_IS_CLANG || JSL_IS_GCC

            return ((uint64_t) 1u) << (((uint64_t) 64u) - JSL_PLATFORM_COUNT_LEADING_ZEROS64(x - 1));

        #else

            x--;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            x |= x >> 32;
            x++;
            return x;

        #endif
    }

    uint32_t jsl_previous_power_of_two_u32(uint32_t x)
    {
        #if JSL_IS_CLANG || JSL_IS_GCC

            return 1u << (31u - JSL_PLATFORM_COUNT_LEADING_ZEROS(x));

        #else

            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            return x - (x >> 1);

        #endif
    }

    uint64_t jsl_previous_power_of_two_u64(uint64_t x)
    {
        #if JSL_IS_CLANG || JSL_IS_GCC

            return ((uint64_t) 1u) << (((uint64_t) 63u) - JSL_PLATFORM_COUNT_LEADING_ZEROS64(x));

        #else

            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            x |= x >> 32;
            return x - (x >> 1);

        #endif
    }

    JSLFatPtr jsl_fatptr_init(uint8_t* ptr, int64_t length)
    {
        JSLFatPtr buffer = {
            .data = ptr,
            .length = length
        };
        return buffer;
    }

    JSLFatPtr jsl_fatptr_slice(JSLFatPtr fatptr, int64_t start, int64_t end)
    {
        JSL_ASSERT(
            fatptr.data != NULL
            && start > -1
            && start <= end
            && end <= fatptr.length
        );

        fatptr.data += start;
        fatptr.length = end - start;
        return fatptr;
    }

    JSLFatPtr jsl_fatptr_slice_to_end(JSLFatPtr fatptr, int64_t start)
    {
        JSL_ASSERT(
            fatptr.data != NULL
            && start > -1
            && start <= fatptr.length
        );

        fatptr.data += start;
        fatptr.length -= start;
        return fatptr;
    }

    int64_t jsl_fatptr_total_write_length(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr)
    {
        int64_t res = -1;

        if (original_fatptr.data != NULL && writer_fatptr.data != NULL)
        {
            int64_t length_written = writer_fatptr.data - original_fatptr.data;
            if (length_written > -1 && length_written <= original_fatptr.length)
            {
                res = length_written;
            }
        }

        return res;
    }

    JSLFatPtr jsl_fatptr_auto_slice(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr)
    {
        int64_t write_length = jsl_fatptr_total_write_length(original_fatptr, writer_fatptr);
        return jsl_fatptr_slice(
            original_fatptr,
            0,
            write_length
        );
    }

    JSLFatPtr jsl_fatptr_from_cstr(char* str)
    {
        JSLFatPtr ret = {
            .data = (uint8_t*) str,
            .length = str == NULL ? 0 : (int64_t) JSL_STRLEN(str)
        };
        return ret;
    }

    int64_t jsl_fatptr_memory_copy(JSLFatPtr* destination, JSLFatPtr source)
    {
        if (
            source.length < 0
            || source.data == NULL
            || destination->length < 0
            || destination->data == NULL
        )
            return -1;

        // Check for overlapping buffers
        if (
            (source.data < destination->data + destination->length && source.data + source.length > destination->data)
            || (destination->data < source.data + source.length && destination->data + destination->length > source.data)
        )
        {
            return -1;
        }

        int64_t memcpy_length = JSL_MIN(source.length, destination->length);
        JSL_MEMCPY(destination->data, source.data, (size_t) memcpy_length);

        destination->data += memcpy_length;
        destination->length -= memcpy_length;

        return memcpy_length;
    }

    int64_t jsl_fatptr_cstr_memory_copy(JSLFatPtr* destination, char* cstring, bool include_null_terminator)
    {
        if (
            cstring == NULL
            || destination->length < 0
            || destination->data == NULL
        )
            return -1;

        int64_t length = JSL_MIN(
            include_null_terminator ? (int64_t) JSL_STRLEN(cstring) + 1 : (int64_t) JSL_STRLEN(cstring),
            destination->length
        );
        JSL_MEMCPY(destination->data, cstring, (size_t) length);

        destination->data += length;
        destination->length -= length;

        return length;
    }

    bool jsl_fatptr_memory_compare(JSLFatPtr a, JSLFatPtr b)
    {
        if (a.length != b.length || a.data == NULL || b.data == NULL)
            return false;

        if (a.data == b.data)
            return true;

        return JSL_MEMCMP(a.data, b.data, (size_t) a.length) == 0;
    }

    bool jsl_fatptr_cstr_compare(JSLFatPtr string, char* cstr)
    {
        if (cstr == NULL || string.data == NULL)
            return false;

        int64_t cstr_length = (int64_t) JSL_STRLEN(cstr);

        if (string.length != cstr_length)
            return false;

        if ((void*) string.data == (void*) cstr)
            return true;

        return JSL_MEMCMP(string.data, cstr, (size_t) cstr_length) == 0;
    }

    #ifdef __AVX2__

        static JSL__FORCE_INLINE int64_t jsl__avx2_substring_search(JSLFatPtr string, JSLFatPtr substring)
        {
            /**
             * From http://0x80.pl/notesen/2016-11-28-simd-strfind.html
             *
             * Copyright (c) 2008-2016, Wojciech Muła
             * All rights reserved.
             *
             * Redistribution and use in source and binary forms, with or without
             * modification, are permitted provided that the following conditions are
             * met:
             *
             * 1. Redistributions of source code must retain the above copyright
             * notice, this list of conditions and the following disclaimer.
             *
             * 2. Redistributions in binary form must reproduce the above copyright
             * notice, this list of conditions and the following disclaimer in the
             * documentation and/or other materials provided with the distribution.
             *
             * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
             * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
             * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
             * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
             * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
             * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
             * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
             * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
             * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
             * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
             * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
            */

            int64_t i = 0;
            int64_t substring_end_index = substring.length - 1;

            __m256i first = _mm256_set1_epi8(substring.data[0]);
            __m256i last  = _mm256_set1_epi8(substring.data[substring_end_index]);

            int64_t stopping_point = string.length - substring_end_index - 64;
            for (; i <= stopping_point; i += 64)
            {
                __m256i block_first1 = _mm256_loadu_si256((__m256i*) (string.data + i));
                __m256i block_last1  = _mm256_loadu_si256((__m256i*) (string.data + i + substring_end_index));

                __m256i block_first2 = _mm256_loadu_si256((__m256i*) (string.data + i + 32));
                __m256i block_last2  = _mm256_loadu_si256((__m256i*) (string.data + i + substring_end_index + 32));

                __m256i eq_first1 = _mm256_cmpeq_epi8(first, block_first1);
                __m256i eq_last1  = _mm256_cmpeq_epi8(last, block_last1);

                __m256i eq_first2 = _mm256_cmpeq_epi8(first, block_first2);
                __m256i eq_last2  = _mm256_cmpeq_epi8(last, block_last2);

                uint32_t mask1 = _mm256_movemask_epi8(_mm256_and_si256(eq_first1, eq_last1));
                uint32_t mask2 = _mm256_movemask_epi8(_mm256_and_si256(eq_first2, eq_last2));
                uint64_t mask = mask1 | ((uint64_t) mask2 << 32);

                while (mask != 0)
                {
                    int32_t bit_position = JSL_PLATFORM_COUNT_TRAILING_ZEROS64(mask);

                    if (JSL_MEMCMP(string.data + i + bit_position + 1, substring.data + 1, substring.length - 2) == 0)
                    {
                        return i + bit_position;
                    }

                    // clear the least significant bit set
                    mask &= (mask - 1);
                }
            }

            stopping_point = string.length - substring.length;
            for (; i <= stopping_point; ++i)
            {
                if (string.data[i] == substring.data[0] &&
                    string.data[i + substring_end_index] == substring.data[substring_end_index])
                {
                    if (substring.length <= 2)
                        return i;
                    if (JSL_MEMCMP(string.data + i + 1,
                                substring.data + 1,
                                substring.length - 2) == 0)
                        return i;
                }
            }

            return -1;
        }

    #elif defined(__ARM_NEON) || defined(__ARM_NEON__)


    #endif

    static JSL__FORCE_INLINE int64_t jsl__two_char_search(JSLFatPtr string, JSLFatPtr substring)
    {
        uint8_t b0 = substring.data[0];
        uint8_t b1 = substring.data[1];

        for (int64_t i = 0; i + 1 < string.length; ++i)
        {
            if (string.data[i] == b0 && string.data[i + 1] == b1)
            {
                return i;
            }
        }

        return -1;
    }

    static JSL__FORCE_INLINE int64_t jsl__bndm_search(JSLFatPtr string, JSLFatPtr substring)
    {
        uint64_t masks[256] = {0};

        // Map rightmost pattern byte to bit 0 (LSB), leftmost to bit m-1
        for (int64_t i = 0; i < substring.length; ++i)
        {
            uint32_t bit = (uint32_t)(substring.length - 1 - i);
            masks[(uint8_t) substring.data[i]] |= (1ULL << bit);
        }

        // Bitmask of the lowest m bits set to 1; careful with m==64
        uint64_t full = (substring.length == 64) ? ~0ULL : ((1ULL << substring.length) - 1ULL);
        uint64_t MSB  = (substring.length == 64) ? (1ULL << 63) : (1ULL << (substring.length - 1));

        int64_t pos = 0;
        int64_t last_start = string.length - substring.length;

        while (pos <= last_start)
        {
            uint64_t D = full;
            // how many chars left to verify in this window
            int64_t j = substring.length;
            // shift distance if mismatch
            int64_t last = substring.length;

            // Backward scan the window using masks
            while (D != 0)
            {
                uint8_t ch = string.data[pos + j - 1];
                D &= masks[ch];
                if (D != 0)
                {
                    if (j == 1)
                    {
                        return pos;
                    }

                    --j;
                    // If MSB set, a prefix of the pattern is aligned -> shorter shift
                    if (D & MSB) last = j;
                }
                /* Advance the simulated NFA: shift left one, keep to m bits */
                D <<= 1;
                if (substring.length < 64)
                    D &= full; /* for m==64, &full is a no-op */
            }

            pos += last;
        }

        return -1;
    }

    /* Sunday / Quick Search for m > 64 */
    static JSL__FORCE_INLINE int64_t jsl__sunday_search(JSLFatPtr string, JSLFatPtr substring)
    {
        // Shift table with default m+1 for all 256 bytes
        int64_t shift[256];
        for (int64_t i = 0; i < 256; ++i)
        {
            shift[i] = substring.length + 1;
        }

        // Rightmost occurrence determines shift
        for (int64_t i = 0; i < substring.length; ++i)
        {
            shift[substring.data[i]] = substring.length - i;
        }

        int64_t pos = 0;
        while (pos + substring.length <= string.length)
        {
            if (JSL_MEMCMP(string.data + pos, substring.data, (size_t) substring.length) == 0)
            {
                return pos;
            }

            int64_t next = pos + substring.length;
            if (next < string.length)
            {
                pos += shift[string.data[next]];
            }
            else
            {
                break;
            }
        }

        return -1;
    }

    static JSL__FORCE_INLINE int64_t jsl__substring_search(JSLFatPtr string, JSLFatPtr substring)
    {
        if (substring.length == 2)
        {
            return jsl__two_char_search(string, substring);
        }
        else if (substring.length <= 64)
        {
            return jsl__bndm_search(string, substring);
        }
        else
        {
            return jsl__sunday_search(string, substring);
        }
    }


    int64_t jsl_fatptr_substring_search(JSLFatPtr string, JSLFatPtr substring)
    {
        if (JSL__UNLIKELY(
            string.data == NULL
            || string.length < 1
            || substring.data == NULL
            || substring.length < 1
            || substring.length > string.length
        ))
        {
            return -1;
        }
        else if (substring.length == 1)
        {
            return jsl_fatptr_index_of(string, substring.data[0]);
        }
        else if (string.length == substring.length)
        {
            if (JSL_MEMCMP(string.data, substring.data, (size_t) string.length) == 0)
                return 0;
            else
                return -1;
        }
        else
        {
            #ifdef __AVX2__
                if (string.length >= 64)
                    return jsl__avx2_substring_search(string, substring);
                else
                    return jsl__substring_search(string, substring);
            #else
                return jsl__substring_search(string, substring);
            #endif
        }
    }

    int64_t jsl_fatptr_index_of(JSLFatPtr string, uint8_t item)
    {
        if (string.data == NULL || string.length < 1)
        {
            return -1;
        }
        else
        {
            int64_t i = 0;

            #ifdef __AVX2__
                __m256i needle = _mm256_set1_epi8(item);

                while (i < string.length - 32)
                {
                    __m256i elements = _mm256_loadu_si256((__m256i*) (string.data + i));
                    __m256i eq_needle = _mm256_cmpeq_epi8(elements, needle);

                    uint32_t mask = _mm256_movemask_epi8(eq_needle);

                    if (mask != 0)
                    {
                        int32_t bit_position = JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                        return i + bit_position;
                    }

                    i += 32;
                }
            #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
                uint8x16_t needle = vdupq_n_u8(item);

                while (i < string.length - 16)
                {
                    const uint8x16_t chunk = vld1q_u8(string.data + i);
                    const uint8x16_t cmp = vceqq_u8(chunk, needle);
                    const uint8_t horizontal_maximum = vmaxvq_u8(cmp);

                    if (horizontal_maximum == 0)
                    {
                        i += 16;
                    }
                    else
                    {
                        const uint32_t mask = jsl__neon_movemask(cmp);
                        uint32_t bit_position = JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                        return i + bit_position;
                    }
                }

            #endif

            while (i < string.length)
            {
                if (string.data[i] == item)
                    return i;

                ++i;
            }

            return -1;
        }
    }

    int64_t jsl_fatptr_count(JSLFatPtr str, uint8_t item)
    {
        int64_t count = 0;

        #ifdef JSL_IS_X86
            #ifdef __AVX2__
                __m256i item_wide_avx = _mm256_set1_epi8(item);

                while (str.length >= 32)
                {
                    __m256i chunk = _mm256_loadu_si256((__m256i*) str.data);
                    __m256i cmp = _mm256_cmpeq_epi8(chunk, item_wide_avx);
                    int mask = _mm256_movemask_epi8(cmp);

                    count += JSL_PLATFORM_POPULATION_COUNT(mask);

                    JSL_FATPTR_ADVANCE(str, 32);
                }
            #endif

            #ifdef __SSE3__
                __m128i item_wide_sse = _mm_set1_epi8(item);

                while (str.length >= 16)
                {
                    __m128i chunk = _mm_loadu_si128((__m128i*) str.data);
                    __m128i cmp = _mm_cmpeq_epi8(chunk, item_wide_sse);
                    int mask = _mm_movemask_epi8(cmp);

                    count += JSL_PLATFORM_POPULATION_COUNT(mask);

                    JSL_FATPTR_ADVANCE(str, 16);
                }
            #endif
        #elif JSL_IS_ARM
            #if defined(__ARM_NEON) || defined(__ARM_NEON__)
                uint8x16_t item_wide = vdupq_n_u8(item);

                while (str.length >= 16)
                {
                    uint8x16_t chunk = vld1q_u8(str.data);
                    uint8x16_t cmp = vceqq_u8(chunk, item_wide);
                    uint8x16_t ones = vshrq_n_u8(cmp, 7);
                    count += vaddvq_u8(ones);

                    JSL_FATPTR_ADVANCE(str, 16);
                }
            #endif
        #endif

        while (str.length > 0)
        {
            if (str.data[0] == item)
                count++;

            JSL_FATPTR_ADVANCE(str, 1);
        }

        return count;
    }

    int64_t jsl_fatptr_index_of_reverse(JSLFatPtr string, uint8_t item)
    {
        if (string.data == NULL || string.length < 1)
        {
            return -1;
        }

        #ifdef __AVX2__
            if (string.length < 32)
            {
                int64_t i = string.length - 1;

                while (i > -1)
                {
                    if (string.data[i] == item)
                        return i;

                    --i;
                }

                return -1;
            }
            else
            {
                int64_t i = string.length - 32;
                __m256i needle = _mm256_set1_epi8(item);

                while (i >= 32)
                {
                    __m256i elements = _mm256_loadu_si256((__m256i*) (string.data + i));
                    __m256i eq_needle = _mm256_cmpeq_epi8(elements, needle);

                    uint32_t mask = _mm256_movemask_epi8(eq_needle);

                    if (mask != 0)
                    {
                        int32_t bit_position = JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                        return i + bit_position;
                    }

                    i -= 32;
                }

                while (i > -1)
                {
                    if (string.data[i] == item)
                        return i;

                    --i;
                }

                return -1;
            }
        #else
            int64_t i = string.length - 1;

            while (i > -1)
            {
                if (string.data[i] == item)
                    return i;

                --i;
            }

            return -1;
        #endif
    }

    bool jsl_fatptr_starts_with(JSLFatPtr str, JSLFatPtr prefix)
    {
        // TODO, cleanup: Think of a way to refactor this to single return

        if (str.data != NULL
            && prefix.data != NULL
            && prefix.length <= str.length)
        {
            // TODO, speed: SIMD
            while (prefix.length > 0 && str.length > 0)
            {
                if (*str.data != *prefix.data)
                {
                    return false;
                }

                ++str.data;
                --str.length;
                ++prefix.data;
                --prefix.length;
            }

            return true;
        }

        return false;
    }

    bool jsl_fatptr_ends_with(JSLFatPtr str, JSLFatPtr postfix)
    {
        // TODO, cleanup: Think of a way to refactor this to single return

        if (str.data != NULL
            && postfix.data != NULL
            && postfix.length <= str.length)
        {
            // TODO, speed: SIMD
            for (int64_t i = 1; i < postfix.length; ++i)
            {
                uint8_t str_item = str.data[str.length - i];
                uint8_t postfix_item = postfix.data[postfix.length - i];

                if (str_item != postfix_item)
                {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    JSLFatPtr jsl_fatptr_get_file_extension(JSLFatPtr filename)
    {
        JSLFatPtr ret;
        int64_t index_of_dot = jsl_fatptr_index_of_reverse(filename, '.');

        if (index_of_dot > -1)
        {
            ret = jsl_fatptr_slice(filename, index_of_dot + 1, filename.length);
        }
        else
        {
            ret.data = NULL;
            ret.length = 0;
        }

        return ret;
    }

    JSLFatPtr jsl_fatptr_basename(JSLFatPtr filename)
    {
        JSLFatPtr ret;
        int64_t slash_postion = jsl_fatptr_index_of_reverse(filename, '/');

        if (filename.length - slash_postion > 2)
        {
            ret = jsl_fatptr_slice(
                filename,
                slash_postion + 1,
                filename.length
            );
        }
        else
        {
            ret = filename;
        }

        return ret;
    }

    char* jsl_fatptr_to_cstr(JSLArena* arena, JSLFatPtr str)
    {
        if (arena == NULL || str.data == NULL || str.length < 1)
            return NULL;

        int64_t allocation_size = str.length + 1;
        JSLFatPtr allocation = jsl_arena_allocate(arena, allocation_size, false);

        if (allocation.data == NULL || allocation.length < allocation_size)
            return NULL;

        JSL_MEMCPY(allocation.data, str.data, (size_t) str.length);
        allocation.data[str.length] = '\0';
        return (char*) allocation.data;
    }

    JSLFatPtr jsl_cstr_to_fatptr(JSLArena* arena, char* str)
    {
        JSLFatPtr ret = {0};
        if (arena == NULL || str == NULL)
            return ret;

        int64_t length = (int64_t) JSL_STRLEN(str);
        if (length == 0)
            return ret;

        int64_t allocation_size = length * (int64_t) sizeof(uint8_t);
        JSLFatPtr allocation = jsl_arena_allocate(
            arena,
            allocation_size,
            false
        );
        if (allocation.data == NULL || allocation.length < length)
            return ret;

        ret = allocation;
        JSL_MEMCPY(ret.data, str, (size_t) length);
        return ret;
    }

    JSLFatPtr jsl_fatptr_duplicate(JSLArena* arena, JSLFatPtr str)
    {
        JSLFatPtr res = {0};
        if (arena == NULL || str.data == NULL || str.length < 1)
            return res;

        JSLFatPtr allocation = jsl_arena_allocate(arena, str.length, false);

        if (allocation.data == NULL || allocation.length < str.length)
            return res;

        JSL_MEMCPY(allocation.data, str.data, (size_t) str.length);
        res = allocation;
        return res;
    }

    void jsl_fatptr_to_lowercase_ascii(JSLFatPtr str)
    {
        if (str.data == NULL || str.length < 1)
            return;

        #ifdef __AVX2__
            __m256i asciiA = _mm256_set1_epi8('A' - 1);
            __m256i asciiZ = _mm256_set1_epi8('Z' + 1);
            __m256i diff   = _mm256_set1_epi8('a' - 'A');

            // TODO, SIMD: add masked loads when length < 32
            while (str.length >= 32)
            {
                __m256i base_data = _mm256_loadu_si256((__m256i*) str.data);

                /* > 'A': 0xff, < 'A': 0x00 */
                __m256i is_greater_or_equal_A = _mm256_cmpgt_epi8(base_data, asciiA);

                /* <= 'Z': 0xff, > 'Z': 0x00 */
                __m256i is_less_or_equal_Z = _mm256_cmpgt_epi8(asciiZ, base_data);

                /* 'Z' >= x >= 'A': 0xFF, else 0x00 */
                __m256i mask = _mm256_and_si256(is_greater_or_equal_A, is_less_or_equal_Z);

                /* 'Z' >= x >= 'A': 'a' - 'A', else 0x00 */
                __m256i to_add = _mm256_and_si256(mask, diff);

                /* add to change to lowercase */
                __m256i added = _mm256_add_epi8(base_data, to_add);
                _mm256_storeu_si256((__m256i *) str.data, added);

                str.length -= 32;
                str.data += 32;
            }
        #endif

        for (int64_t i = 0; i < str.length; i++)
        {
            if (str.data[i] >= 'A' && str.data[i] <= 'Z')
            {
                str.data[i] += 32;
            }
        }
    }

    static inline uint8_t ascii_to_lower(uint8_t ch)
    {
        if (ch >= 'A' && ch <= 'Z')
        {
            return ch + 32;
        }
        return ch;
    }

    #if defined(__AVX2__)
        static inline __m256i ascii_to_lower_avx2(__m256i data)
        {
            __m256i upper_A = _mm256_set1_epi8('A' - 1);
            __m256i upper_Z = _mm256_set1_epi8('Z' + 1);
            __m256i case_diff = _mm256_set1_epi8(32);

            // Check if character is between 'A' and 'Z'
            __m256i is_upper = _mm256_and_si256(
                _mm256_cmpgt_epi8(data, upper_A),
                _mm256_cmpgt_epi8(upper_Z, data)
            );

            return _mm256_add_epi8(data, _mm256_and_si256(is_upper, case_diff));
        }
    #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
        static inline uint8x16_t ascii_to_lower_neon(uint8x16_t data)
        {
            const uint8x16_t upper_A = vdupq_n_u8('A' - 1);
            const uint8x16_t upper_Z = vdupq_n_u8('Z' + 1);
            const uint8x16_t case_diff = vdupq_n_u8(32);

            uint8x16_t is_upper = vandq_u8(
                vcgtq_u8(data, upper_A),
                vcgtq_u8(upper_Z, data)
            );

            return vaddq_u8(data, vandq_u8(is_upper, case_diff));
        }
    #endif

    bool jsl_fatptr_compare_ascii_insensitive(JSLFatPtr a, JSLFatPtr b)
    {
        if (JSL__UNLIKELY(a.data == NULL || b.data == NULL || a.length != b.length))
            return false;

        int64_t i = 0;

        #if defined(__AVX2__)
            for (; i <= a.length - 32; i += 32)
            {
                __m256i a_vec = _mm256_loadu_si256((__m256i*)(a.data + i));
                __m256i b_vec = _mm256_loadu_si256((__m256i*)(b.data + i));

                a_vec = ascii_to_lower_avx2(a_vec);
                b_vec = ascii_to_lower_avx2(b_vec);

                __m256i cmp = _mm256_cmpeq_epi8(a_vec, b_vec);
                if (_mm256_movemask_epi8(cmp) != -1)
                    return false;
            }
        #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
            for (; i <= a.length - 16; i += 16)
            {
                uint8x16_t a_vec = vld1q_u8(a.data + i);
                uint8x16_t b_vec = vld1q_u8(b.data + i);

                a_vec = ascii_to_lower_neon(a_vec);
                b_vec = ascii_to_lower_neon(b_vec);

                uint8x16_t cmp = vceqq_u8(a_vec, b_vec);
                if (jsl__neon_movemask(cmp) != 0xFFFF)
                    return false;
            }
        #endif

        for (; i < a.length; i++)
        {
            if (ascii_to_lower(a.data[i]) != ascii_to_lower(b.data[i]))
                return false;
        }

        return true;
    }

    int32_t jsl_fatptr_to_int32(JSLFatPtr str, int32_t* result)
    {
        if (JSL__UNLIKELY(str.data == NULL || str.length < 1))
            return 0;

        bool negative = false;
        int32_t ret = 0;
        int32_t i = 0;

        if (str.data[0] == '-')
        {
            ++i;
            negative = true;
        }
        else if (str.data[0] == '+')
        {
            ++i;
        }

        while (str.data[i] == '0' && i < str.length)
        {
            ++i;
        }

        for (; i < str.length; i++)
        {
            uint8_t digit = str.data[i] - '0';
            if (digit > 9)
                break;

            ret = (ret * 10) + digit;
        }

        if (negative)
            ret = -ret;

        if (i > 0)
            *result = ret;

        return i;
    }

    struct JSLStringBuilderChunk
    {
        JSLFatPtr buffer;
        JSLFatPtr writer;
        struct JSLStringBuilderChunk* next;
    };

    static bool jsl__string_builder_add_chunk(JSLStringBuilder* builder)
    {
        struct JSLStringBuilderChunk* chunk = JSL_ARENA_TYPED_ALLOCATE(
            struct JSLStringBuilderChunk,
            builder->arena
        );
        chunk->next = NULL;
        chunk->buffer = jsl_arena_allocate_aligned(
            builder->arena, builder->chunk_size,
            builder->alignment,
            false
        );

        if (chunk->buffer.data != NULL)
        {
            chunk->writer = chunk->buffer;

            if (builder->head == NULL)
                builder->head = chunk;

            if (builder->tail == NULL)
            {
                builder->tail = chunk;
            }
            else
            {
                builder->tail->next = chunk;
                builder->tail = chunk;
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    bool jsl_string_builder_init(JSLStringBuilder* builder, JSLArena* arena)
    {
        return jsl_string_builder_init2(
            builder,
            arena,
            256,
            8
        );
    }

    bool jsl_string_builder_init2(
        JSLStringBuilder* builder,
        JSLArena* arena,
        int32_t chunk_size,
        int32_t alignment
    )
    {
        bool res = false;

        if (builder != NULL)
        {
            JSL_MEMSET(builder, 0, sizeof(JSLStringBuilder));

            if (arena != NULL && chunk_size > 0 && alignment > 0)
            {
                builder->arena = arena;
                builder->chunk_size = chunk_size;
                builder->alignment = alignment;
                res = jsl__string_builder_add_chunk(builder);
            }
        }

        return res;
    }

    bool jsl_string_builder_insert_char(JSLStringBuilder* builder, char c)
    {
        if (builder == NULL || builder->head == NULL || builder->tail == NULL)
            return false;

        bool res = false;

        bool needs_alloc = false;
        if (builder->tail->writer.length > 0)
        {
            builder->tail->writer.data[0] = (uint8_t) c;
            JSL_FATPTR_ADVANCE(builder->tail->writer, 1);
            res = true;
        }
        else
        {
            needs_alloc = true;
        }

        bool has_new_chunk = false;
        if (needs_alloc)
        {
            has_new_chunk = jsl__string_builder_add_chunk(builder);
        }

        if (has_new_chunk)
        {
            builder->tail->writer.data[0] = (uint8_t) c;
            JSL_FATPTR_ADVANCE(builder->tail->writer, 1);
            res = true;
        }

        return res;
    }

    bool jsl_string_builder_insert_uint8_t(JSLStringBuilder* builder, uint8_t c)
    {
        if (builder == NULL || builder->head == NULL || builder->tail == NULL)
            return false;

        bool res = false;

        bool needs_alloc = false;
        if (builder->tail->writer.length > 0)
        {
            builder->tail->writer.data[0] = c;
            JSL_FATPTR_ADVANCE(builder->tail->writer, 1);
            res = true;
        }
        else
        {
            needs_alloc = true;
        }

        bool has_new_chunk = false;
        if (needs_alloc)
        {
            has_new_chunk = jsl__string_builder_add_chunk(builder);
        }

        if (has_new_chunk)
        {
            builder->tail->writer.data[0] = c;
            JSL_FATPTR_ADVANCE(builder->tail->writer, 1);
            res = true;
        }

        return res;
    }

    bool jsl_string_builder_insert_fatptr(JSLStringBuilder* builder, JSLFatPtr data)
    {
        if (builder == NULL || builder->head == NULL || builder->tail == NULL)
            return false;

        bool res = true;

        while (data.length > 0)
        {
            if (builder->tail->writer.length == 0)
            {
                res = jsl__string_builder_add_chunk(builder);
                if (!res)
                    break;
            }

            int64_t bytes_written = jsl_fatptr_memory_copy(&builder->tail->writer, data);
            JSL_FATPTR_ADVANCE(data, bytes_written);
        }

        return res;
    }

    void jsl_string_builder_iterator_init(JSLStringBuilder* builder, JSLStringBuilderIterator* iterator)
    {
        if (builder != NULL && iterator != NULL)
            iterator->current = builder->head;
    }

    JSLFatPtr jsl_string_builder_iterator_next(JSLStringBuilderIterator* iterator)
    {
        JSLFatPtr ret = {0};
        if (iterator == NULL)
            return ret;

        struct JSLStringBuilderChunk* current = iterator->current;
        if (current == NULL || current->buffer.data == NULL)
            return ret;

        iterator->current = current->next;
        ret = jsl_fatptr_auto_slice(current->buffer, current->writer);
        return ret;
    }

    struct JSL__StringBuilderContext
    {
        JSLStringBuilder* builder;
        bool failure_flag;
        uint8_t buffer[JSL_FORMAT_MIN_BUFFER];
    };

    static uint8_t* format_string_builder_callback(uint8_t *buf, void *user, int64_t len)
    {
        struct JSL__StringBuilderContext* context = (struct JSL__StringBuilderContext*) user;

        if (context->builder->head == NULL || context->builder->tail == NULL || len > JSL_FORMAT_MIN_BUFFER)
            return NULL;

        bool res = jsl_string_builder_insert_fatptr(context->builder, jsl_fatptr_init(buf, len));

        if (res)
        {
            return context->buffer;
        }
        else
        {
            context->failure_flag = true;
            return NULL;
        }
    }

    JSL__ASAN_OFF bool jsl_string_builder_format(JSLStringBuilder* builder, JSLFatPtr fmt, ...)
    {
        if (builder == NULL || builder->head == NULL || builder->tail == NULL)
            return false;

        va_list va;
        va_start(va, fmt);

        struct JSL__StringBuilderContext context;
        context.builder = builder;
        context.failure_flag = false;

        jsl_format_callback(
            format_string_builder_callback,
            &context,
            context.buffer,
            fmt,
            va
        );

        va_end(va);

        return !context.failure_flag;
    }

    void jsl_arena_init(JSLArena* arena, void* memory, int64_t length)
    {
        arena->start = memory;
        arena->current = memory;
        arena->end = (uint8_t*) memory + length;

        ASAN_POISON_MEMORY_REGION(memory, length);
    }

    void jsl_arena_init2(JSLArena* arena, JSLFatPtr memory)
    {
        arena->start = memory.data;
        arena->current = memory.data;
        arena->end = memory.data + memory.length;

        ASAN_POISON_MEMORY_REGION(memory.data, memory.length);
    }

    JSLFatPtr jsl_arena_allocate(JSLArena* arena, int64_t bytes, bool zeroed)
    {
        return jsl_arena_allocate_aligned(arena, bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT, zeroed);
    }

    static inline bool jsl__is_power_of_two(int32_t x)
    {
        return (x & (x-1)) == 0;
    }

    static inline uint8_t* align_ptr_upwards(uint8_t* ptr, int32_t align)
    {
        uintptr_t addr   = (uintptr_t)ptr;
        uintptr_t ualign = (uintptr_t)(uint32_t)align;

        uintptr_t mask = ualign - 1;
        addr = (addr + mask) & ~mask;

        return (uint8_t*)addr;
    }

    JSLFatPtr jsl_arena_allocate_aligned(JSLArena* arena, int64_t bytes, int32_t alignment, bool zeroed)
    {
        JSL_ASSERT(
            alignment > 0
            && jsl__is_power_of_two(alignment)
        );

        JSLFatPtr res = {0};
        uint8_t* aligned_current = align_ptr_upwards(arena->current, alignment);
        uint8_t* potential_end = aligned_current + bytes;

        if (potential_end <= arena->end)
        {
            res.data = aligned_current;
            res.length = bytes;

            #if defined(__SANITIZE_ADDRESS__)
                // Add 8 to leave "guard" zones between allocations
                arena->current = potential_end + 8;
                ASAN_UNPOISON_MEMORY_REGION(res.data, res.length);
            #else
                arena->current = potential_end;
            #endif

            if (zeroed)
                JSL_MEMSET((void*) res.data, 0, (size_t) res.length);
        }

        return res;
    }

    JSLFatPtr jsl_arena_reallocate(JSLArena* arena, JSLFatPtr original_allocation, int64_t new_num_bytes)
    {
        return jsl_arena_reallocate_aligned(
            arena, original_allocation, new_num_bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT
        );
    }

    JSLFatPtr jsl_arena_reallocate_aligned(
        JSLArena* arena,
        JSLFatPtr original_allocation,
        int64_t new_num_bytes,
        int32_t align
    )
    {
        JSL_ASSERT(align > 0);
        JSL_ASSERT(jsl__is_power_of_two(align));

        JSLFatPtr res = {0};
        uint8_t* aligned_current = align_ptr_upwards(arena->current, align);
        uint8_t* potential_end = aligned_current + new_num_bytes;

        // Only resize if this given allocation was the last thing alloc-ed
        bool same_pointer =
            (arena->current - original_allocation.length) == original_allocation.data;
        bool is_space_left = potential_end <= arena->end;

        if (same_pointer && is_space_left)
        {
            res.data = original_allocation.data;
            res.length = new_num_bytes;
            arena->current = original_allocation.data + new_num_bytes;

            ASAN_UNPOISON_MEMORY_REGION(res.data, res.length);
        }
        else
        {
            res = jsl_arena_allocate_aligned(arena, new_num_bytes, align, false);
            if (res.data != NULL)
            {
                JSL_MEMCPY(
                    res.data,
                    original_allocation.data,
                    (size_t) original_allocation.length
                );

                #ifdef JSL_DEBUG
                    JSL_MEMSET((void*) original_allocation.data, 0xfeeefeee, original_allocation.length);
                #endif

                ASAN_POISON_MEMORY_REGION(original_allocation.data, original_allocation.length);
            }
        }

        return res;
    }

    void jsl_arena_reset(JSLArena* arena)
    {
        ASAN_UNPOISON_MEMORY_REGION(arena->start, arena->end - arena->start);

        #ifdef JSL_DEBUG
            JSL_MEMSET((void*) arena->start, 0xfeeefeee, arena->current - arena->start);
        #endif

        arena->current = arena->start;

        ASAN_POISON_MEMORY_REGION(arena->start, arena->end - arena->start);
    }

    uint8_t* jsl_arena_save_restore_point(JSLArena* arena)
    {
        return arena->current;
    }

    void jsl_arena_load_restore_point(JSLArena* arena, uint8_t* restore_point)
    {
        JSL_ASSERT(restore_point >= arena->start);
        JSL_ASSERT(restore_point <= arena->end);

        ASAN_UNPOISON_MEMORY_REGION(restore_point, arena->current - restore_point);

        #ifdef JSL_DEBUG
            JSL_MEMSET((void*) restore_point, 0xfeeefeee, arena->current - restore_point);
        #endif

        ASAN_POISON_MEMORY_REGION(restore_point, arena->current - restore_point);

        arena->current = restore_point;
    }

    static int32_t stbsp__real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits);
    static int32_t stbsp__real_to_parts(int64_t *bits, int32_t *expo, double value);
    #define STBSP__SPECIAL 0x7000

    static char stbsp__period = '.';
    static char stbsp__comma = ',';
    static struct
    {
        short temp; // force next field to be 2-byte aligned
        char pair[201];
    } stbsp__digitpair =
    {
        0,
        "00010203040506070809101112131415161718192021222324"
        "25262728293031323334353637383940414243444546474849"
        "50515253545556575859606162636465666768697071727374"
        "75767778798081828384858687888990919293949596979899"
    };

    JSL__ASAN_OFF void jsl_format_set_separators(char pcomma, char pperiod)
    {
        stbsp__period = pperiod;
        stbsp__comma = pcomma;
    }

    #define STBSP__LEFTJUST 1
    #define STBSP__LEADINGPLUS 2
    #define STBSP__LEADINGSPACE 4
    #define STBSP__LEADING_0X 8
    #define STBSP__LEADINGZERO 16
    #define STBSP__INTMAX 32
    #define STBSP__TRIPLET_COMMA 64
    #define STBSP__NEGATIVE 128
    #define STBSP__METRIC_SUFFIX 256
    #define STBSP__HALFWIDTH 512
    #define STBSP__METRIC_NOSPACE 1024
    #define STBSP__METRIC_1024 2048
    #define STBSP__METRIC_JEDEC 4096

    static void stbsp__lead_sign(uint32_t formatting_flags, char *sign)
    {
        sign[0] = 0;
        if (formatting_flags & STBSP__NEGATIVE) {
            sign[0] = 1;
            sign[1] = '-';
        } else if (formatting_flags & STBSP__LEADINGSPACE) {
            sign[0] = 1;
            sign[1] = ' ';
        } else if (formatting_flags & STBSP__LEADINGPLUS) {
            sign[0] = 1;
            sign[1] = '+';
        }
    }

    static JSL__ASAN_OFF uint32_t stbsp__strlen_limited(char const *string, uint32_t limit)
    {
        char const* source_ptr = string;

        #if defined(__AVX2__)

            for (;;)
            {
                if (!limit || *source_ptr == 0)
                    return (uint32_t)(source_ptr - string);

                if ((((uintptr_t) source_ptr) & 31) == 0)
                    break;

                ++source_ptr;
                --limit;
            }

            __m256i zero_wide = _mm256_setzero_si256();

            while (limit >= 32)
            {
                __m256i* source_wide = (__m256i*) source_ptr;
                __m256i data = _mm256_load_si256(source_wide);
                __m256i null_terminator_mask = _mm256_cmpeq_epi8(data, zero_wide);
                int mask = _mm256_movemask_epi8(null_terminator_mask);
                if (mask == 0)
                {
                    source_ptr += 32;
                    limit -= 32;
                }
                else
                {
                    int null_pos = JSL_PLATFORM_FIND_FIRST_SET(mask) - 1;
                    source_ptr += null_pos;
                    limit -= null_pos;
                    break;
                }
            }

        #else

            // get up to 4-byte alignment
            for (;;)
            {
                if (((uintptr_t) source_ptr & 3) == 0)
                    break;

                if (!limit || *source_ptr == 0)
                    return (uint32_t)(source_ptr - string);

                ++source_ptr;
                --limit;
            }

            // scan over 4 bytes at a time to find terminating 0
            // this will intentionally scan up to 3 bytes past the end of buffers,
            // but because it works 4B aligned, it will never cross page boundaries
            // (hence the STBSP__ASAN markup; the over-read here is intentional
            // and harmless)
            while (limit >= 4)
            {
                uint32_t v = *(uint32_t *)source_ptr;
                // bit hack to find if there's a 0 byte in there
                if ((v - 0x01010101) & (~v) & 0x80808080UL)
                    break;

                source_ptr += 4;
                limit -= 4;
            }

        #endif

        // handle the last few characters to find actual size
        while (limit && *source_ptr)
        {
            ++source_ptr;
            --limit;
        }

        return (uint32_t)(source_ptr - string);
    }

    JSL__ASAN_OFF int64_t jsl_format_callback(
        JSL_FORMAT_CALLBACK* callback,
        void* user,
        uint8_t* buffer,
        JSLFatPtr fmt,
        va_list va
    )
    {
        static char hex[] = "0123456789abcdefxp";
        static char hexu[] = "0123456789ABCDEFXP";
        static JSLFatPtr err_string = JSL_FATPTR_INITIALIZER("(ERROR)");
        uint8_t* buffer_cursor;
        JSLFatPtr f;
        int32_t tlen = 0;

        buffer_cursor = buffer;
        f = fmt;

        #if defined(__AVX2__)
            const __m256i percent_wide = _mm256_set1_epi8('%');
        #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
            const uint8x16_t percent_wide = vdupq_n_u8('%');
        #endif

        while (f.length > 0)
        {
            int32_t field_width, precision, trailing_zeros;
            uint32_t formatting_flags;

            // macros for the callback buffer stuff
            #define stbsp__chk_cb_bufL(bytes)                                                   \
                {                                                                               \
                    int32_t len = (int32_t)(buffer_cursor - buffer);                            \
                    if ((len + (bytes)) >= JSL_FORMAT_MIN_BUFFER) {                             \
                        tlen += len;                                                            \
                        if (0 == (buffer_cursor = buffer = callback(buffer, user, len)))        \
                            goto done;                                                          \
                    }                                                                           \
                }

            #define stbsp__chk_cb_buf(bytes)                                    \
                {                                                               \
                    if (callback) {                                             \
                        stbsp__chk_cb_bufL(bytes);                              \
                    }                                                           \
                }

            #define stbsp__flush_cb()                                                           \
                {                                                                               \
                    stbsp__chk_cb_bufL(JSL_FORMAT_MIN_BUFFER - 1);                              \
                } // flush if there is even one byte in the buffer

            #define stbsp__cb_buf_clamp(cl, v)                                                  \
                cl = v;                                                                         \
                if (callback) {                                                                 \
                    int32_t lg = JSL_FORMAT_MIN_BUFFER - (int32_t)(buffer_cursor - buffer);     \
                    if (cl > lg)                                                                \
                    cl = lg;                                                                    \
                }

            #if defined(__AVX2__)

                // Get 32 byte aligned address instead of doing unaligned loads
                // which will give big performance wins to long format strings and
                // are not that big of a deal to short ones.
                //
                // The performance win comes from not loading across a page boundary,
                // thus requiring two loads for each load intrinsic
                while (((uintptr_t) f.data) & 31 && f.length > 0)
                {
                    schk1:
                    if (f.data[0] == '%')
                        goto L_PROCESS_PERCENT;

                    stbsp__chk_cb_buf(1);
                    *buffer_cursor = f.data[0];
                    ++buffer_cursor;
                    JSL_FATPTR_ADVANCE(f, 1);
                }

                while (f.length > 31)
                {
                    const __m256i* source_wide = (const __m256i*) f.data;
                    __m256i* wide_dest = (__m256i*) buffer_cursor;

                    __m256i data = _mm256_load_si256(source_wide);
                    __m256i percent_mask = _mm256_cmpeq_epi8(data, percent_wide);
                    int mask = _mm256_movemask_epi8(percent_mask);

                    if (mask == 0)
                    {
                        // No special characters found, store entire block
                        stbsp__chk_cb_buf(32);
                        _mm256_storeu_si256(wide_dest, data);
                        JSL_FATPTR_ADVANCE(f, 32);
                        buffer_cursor += 32;
                    }
                    else
                    {
                        int special_pos = JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                        stbsp__chk_cb_buf(special_pos);

                        JSL_MEMCPY(buffer_cursor, f.data, special_pos);
                        JSL_FATPTR_ADVANCE(f, special_pos);
                        buffer_cursor += special_pos;

                        goto L_PROCESS_PERCENT;
                    }
                }

                if (f.length == 0)
                    goto L_END_FORMAT;
                else
                    goto schk1;

            #elif defined(__ARM_NEON) || defined(__ARM_NEON__)

                // Get 16 byte aligned address instead of doing unaligned loads
                // which will give big performance wins to long format strings and
                // are not that big of a deal to short ones.
                //
                // The performance win comes from not loading across a page boundary,
                // thus requiring two loads for each load intrinsic.
                while (((uintptr_t) f.data) & 15 && f.length > 0)
                {
                    schk1:
                    if (f.data[0] == '%')
                        goto L_PROCESS_PERCENT;

                    stbsp__chk_cb_buf(1);
                    *buffer_cursor = f.data[0];
                    ++buffer_cursor;
                    JSL_FATPTR_ADVANCE(f, 1);
                }

                while (f.length > 31)
                {
                    const uint8x16_t data0 = vld1q_u8(f.data);
                    const uint8x16_t data1 = vld1q_u8(f.data + 16);

                    const uint8x16_t percent_cmp0 = vceqq_u8(data0, percent_wide);
                    const uint8x16_t percent_cmp1 = vceqq_u8(data1, percent_wide);
                    const uint8_t horizontal_maximum0 = vmaxvq_u8(percent_cmp0);
                    const uint8_t horizontal_maximum1 = vmaxvq_u8(percent_cmp1);

                    const uint32_t combined_presence = horizontal_maximum0 | horizontal_maximum1;
                    if (combined_presence == 0)
                    {
                        stbsp__chk_cb_buf(32);
                        vst1q_u8(buffer_cursor, data0);
                        vst1q_u8(buffer_cursor + 16, data1);
                        JSL_FATPTR_ADVANCE(f, 32);
                        buffer_cursor += 32;
                    }
                    else
                    {
                        const uint32_t mask0 = jsl__neon_movemask(percent_cmp0);
                        const uint32_t mask1 = jsl__neon_movemask(percent_cmp1);
                        const uint32_t mask  = mask0 | (mask1 << 16);

                        const int32_t special_pos = (int32_t) JSL_PLATFORM_COUNT_TRAILING_ZEROS(mask);
                        stbsp__chk_cb_buf(special_pos);
                        JSL_MEMCPY(buffer_cursor, f.data, special_pos);

                        JSL_FATPTR_ADVANCE(f, special_pos);
                        buffer_cursor += special_pos;

                        goto L_PROCESS_PERCENT;
                    }
                }

                if (f.length == 0)
                    goto L_END_FORMAT;
                else
                    goto schk1;

            #else

                // loop one byte at a time to get up to 4-byte alignment
                while (((uintptr_t) f.data) & 3 && f.length > 0)
                {
                    schk1:
                    if (f.data[0] == '%')
                        goto L_PROCESS_PERCENT;

                    stbsp__chk_cb_buf(1);
                    *buffer_cursor = f.data[0];
                    ++buffer_cursor;
                    JSL_FATPTR_ADVANCE(f, 1);
                }

                // fast copy everything up to the next %
                while (f.length > 3)
                {
                    // Check if the next 4 bytes contain %
                    // Using the 'hasless' trick:
                    // https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
                    uint32_t v, c;
                    v = *(uint32_t *) f.data;
                    c = (~v) & 0x80808080;

                    if (((v ^ 0x25252525) - 0x01010101) & c)
                        goto schk1;

                    if (callback)
                    {
                        if ((JSL_FORMAT_MIN_BUFFER - (int32_t)(buffer_cursor - buffer)) < 4)
                            goto schk1;
                    }

                    if(((uintptr_t) buffer_cursor) & 3)
                    {
                        buffer_cursor[0] = f.data[0];
                        buffer_cursor[1] = f.data[1];
                        buffer_cursor[2] = f.data[2];
                        buffer_cursor[3] = f.data[3];
                    }
                    else
                    {
                        *((uint32_t*) buffer_cursor) = v;
                    }

                    buffer_cursor += 4;
                    JSL_FATPTR_ADVANCE(f, 4);
                }

                if (f.length == 0)
                    goto L_END_FORMAT;
                else
                    goto schk1;

            #endif

            L_PROCESS_PERCENT:

            JSL_FATPTR_ADVANCE(f, 1);

            // ok, we have a percent, read the modifiers first
            field_width = 0;
            precision = -1;
            formatting_flags = 0;
            trailing_zeros = 0;

            // flags
            for (;;) {
                switch (f.data[0]) {
                // if we have left justify
                case '-':
                    formatting_flags |= STBSP__LEFTJUST;
                    JSL_FATPTR_ADVANCE(f, 1);
                    continue;
                // if we have leading plus
                case '+':
                    formatting_flags |= STBSP__LEADINGPLUS;
                    JSL_FATPTR_ADVANCE(f, 1);
                    continue;
                // if we have leading space
                case ' ':
                    formatting_flags |= STBSP__LEADINGSPACE;
                    JSL_FATPTR_ADVANCE(f, 1);
                    continue;
                // if we have leading 0x
                case '#':
                    formatting_flags |= STBSP__LEADING_0X;
                    JSL_FATPTR_ADVANCE(f, 1);
                    continue;
                // if we have thousand commas
                case '\'':
                    formatting_flags |= STBSP__TRIPLET_COMMA;
                    JSL_FATPTR_ADVANCE(f, 1);
                    continue;
                // if we have kilo marker (none->kilo->kibi->jedec)
                case '$':
                    if (formatting_flags & STBSP__METRIC_SUFFIX) {
                    if (formatting_flags & STBSP__METRIC_1024) {
                        formatting_flags |= STBSP__METRIC_JEDEC;
                    } else {
                        formatting_flags |= STBSP__METRIC_1024;
                    }
                    } else {
                    formatting_flags |= STBSP__METRIC_SUFFIX;
                    }
                    JSL_FATPTR_ADVANCE(f, 1);
                    continue;
                // if we don't want space between metric suffix and number
                case '_':
                    formatting_flags |= STBSP__METRIC_NOSPACE;
                    JSL_FATPTR_ADVANCE(f, 1);
                    continue;
                // if we have leading zero
                case '0':
                    formatting_flags |= STBSP__LEADINGZERO;
                    JSL_FATPTR_ADVANCE(f, 1);
                    goto flags_done;
                default: goto flags_done;
                }
            }
            flags_done:

            // get the field width
            if (f.data[0] == '*') {
                field_width = (int32_t) va_arg(va, uint32_t);
                JSL_FATPTR_ADVANCE(f, 1);
            } else {
                while ((f.data[0] >= '0') && (f.data[0] <= '9')) {
                    field_width = field_width * 10 + f.data[0] - '0';
                    JSL_FATPTR_ADVANCE(f, 1);
                }
            }
            // get the precision
            if (f.data[0] == '.') {
                JSL_FATPTR_ADVANCE(f, 1);
                if (f.data[0] == '*') {
                    precision = (int32_t) va_arg(va, uint32_t);
                    JSL_FATPTR_ADVANCE(f, 1);
                } else {
                    precision = 0;
                    while ((f.data[0] >= '0') && (f.data[0] <= '9')) {
                    precision = precision * 10 + f.data[0] - '0';
                    JSL_FATPTR_ADVANCE(f, 1);
                    }
                }
            }

            // handle integer size overrides
            switch (f.data[0])
            {
                // are we halfwidth?
                case 'h':
                    formatting_flags |= STBSP__HALFWIDTH;
                    JSL_FATPTR_ADVANCE(f, 1);
                    if (f.data[0] == 'h')
                        JSL_FATPTR_ADVANCE(f, 1);  // QUARTERWIDTH
                    break;
                // are we 64-bit (unix style)
                case 'l':
                    formatting_flags |= ((sizeof(long) == 8) ? STBSP__INTMAX : 0);
                    JSL_FATPTR_ADVANCE(f, 1);
                    if (f.data[0] == 'l') {
                        formatting_flags |= STBSP__INTMAX;
                        JSL_FATPTR_ADVANCE(f, 1);
                    }
                    break;
                // are we 64-bit on intmax? (c99)
                case 'j':
                    formatting_flags |= (sizeof(size_t) == 8) ? STBSP__INTMAX : 0;
                    JSL_FATPTR_ADVANCE(f, 1);
                    break;
                // are we 64-bit on size_t or ptrdiff_t? (c99)
                case 'z':
                    formatting_flags |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
                    JSL_FATPTR_ADVANCE(f, 1);
                    break;
                case 't':
                    formatting_flags |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
                    JSL_FATPTR_ADVANCE(f, 1);
                    break;
                // are we 64-bit (msft style)
                case 'I':
                    if ((f.data[1] == '6') && (f.data[2] == '4')) {
                        formatting_flags |= STBSP__INTMAX;
                        JSL_FATPTR_ADVANCE(f, 3);
                    } else if ((f.data[1] == '3') && (f.data[2] == '2')) {
                        JSL_FATPTR_ADVANCE(f, 3);
                    } else {
                        formatting_flags |= ((sizeof(void *) == 8) ? STBSP__INTMAX : 0);
                        JSL_FATPTR_ADVANCE(f, 1);
                    }
                    break;
                default: break;
            }

            // handle each replacement
            switch (f.data[0])
            {
                #define STBSP__NUMSZ 512 // big enough for e308 (with commas) or e-307
                char num[STBSP__NUMSZ];
                char lead[8];
                char tail[8];
                char* string;
                char const* h;
                uint32_t l, n, comma_spacing;
                uint64_t n64;
                double float_value;
                int32_t decimal_precision;
                char const* source_ptr;

                case 's':
                    string = va_arg(va, char *);

                    if (JSL__UNLIKELY(string == NULL))
                    {
                        string = (char*) err_string.data;
                        l = (uint32_t) err_string.length;
                    }
                    else
                    {
                        // get the length, limited to desired precision
                        // always limit to ~0u chars since our counts are 32b
                        l = (precision >= 0)
                            ? stbsp__strlen_limited(string, (uint32_t) precision)
                            : (uint32_t) JSL_STRLEN(string);
                    }

                    lead[0] = 0;
                    tail[0] = 0;
                    precision = 0;
                    decimal_precision = 0;
                    comma_spacing = 0;
                    // copy the string in
                    goto L_STRING_COPY;

                case 'y':
                {
                    JSLFatPtr fat_string = va_arg(va, JSLFatPtr);

                    if (JSL__UNLIKELY(formatting_flags != 0
                        || field_width != 0
                        || precision != -1
                        || fat_string.data == NULL
                        || fat_string.length < 0
                        || fat_string.length > UINT32_MAX))
                    {
                        string = (char*) err_string.data;
                        l = (uint32_t) err_string.length;
                    }
                    else
                    {
                        string = (char*) fat_string.data;
                        l = (uint32_t) fat_string.length;
                    }

                    field_width = 0;
                    formatting_flags = 0;
                    lead[0] = 0;
                    tail[0] = 0;
                    precision = 0;
                    decimal_precision = 0;
                    comma_spacing = 0;
                    goto L_STRING_COPY;
                }

                case 'c': // char
                    // get the character
                    string = num + STBSP__NUMSZ - 1;
                    *string = (char)va_arg(va, int32_t);
                    l = 1;
                    lead[0] = 0;
                    tail[0] = 0;
                    precision = 0;
                    decimal_precision = 0;
                    comma_spacing = 0;
                    goto L_STRING_COPY;

                case 'n': // weird write-bytes specifier
                {
                    int32_t *d = va_arg(va, int32_t *);
                    *d = tlen + (int32_t)(buffer_cursor - buffer);
                } break;

                case 'A': // hex float
                case 'a': // hex float
                    h = (f.data[0] == 'A') ? hexu : hex;
                    float_value = va_arg(va, double);
                    if (precision == -1)
                        precision = 6; // default is 6
                    // read the double into a string
                    if (stbsp__real_to_parts((int64_t *)&n64, &decimal_precision, float_value))
                        formatting_flags |= STBSP__NEGATIVE;

                    string = num + 64;

                    stbsp__lead_sign(formatting_flags, lead);

                    if (decimal_precision == -1023)
                        decimal_precision = (n64) ? -1022 : 0;
                    else
                        n64 |= (((uint64_t)1) << 52);
                    n64 <<= (64 - 56);
                    if (precision < 15)
                        n64 += ((((uint64_t)8) << 56) >> (precision * 4));
                    // add leading chars

                    lead[1 + lead[0]] = '0';
                    lead[2 + lead[0]] = 'x';
                    lead[0] += 2;

                    *string++ = h[(n64 >> 60) & 15];
                    n64 <<= 4;
                    if (precision)
                        *string++ = stbsp__period;
                    source_ptr = string;

                    // print the bits
                    n = (uint32_t) precision;
                    if (n > 13)
                        n = 13;
                    if (precision > (int32_t)n)
                        trailing_zeros = precision - (int32_t)n;
                    precision = 0;
                    while (n--) {
                        *string++ = h[(n64 >> 60) & 15];
                        n64 <<= 4;
                    }

                    // print the expo
                    tail[1] = h[17];
                    if (decimal_precision < 0) {
                        tail[2] = '-';
                        decimal_precision = -decimal_precision;
                    } else
                        tail[2] = '+';
                    n = (decimal_precision >= 1000) ? 6 : ((decimal_precision >= 100) ? 5 : ((decimal_precision >= 10) ? 4 : 3));
                    tail[0] = (char)n;
                    for (;;) {
                        tail[n] = '0' + decimal_precision % 10;
                        if (n <= 3)
                        break;
                        --n;
                        decimal_precision /= 10;
                    }

                    decimal_precision = (int32_t)(string - source_ptr);
                    l = (uint32_t)(string - (num + 64));
                    string = num + 64;
                    comma_spacing = 1u + (3u << 24);
                    goto L_STRING_COPY;

                case 'G': // float
                case 'g': // float
                    h = (f.data[0] == 'G') ? hexu : hex;
                    float_value = va_arg(va, double);
                    if (precision == -1)
                        precision = 6;
                    else if (precision == 0)
                        precision = 1; // default is 6
                    // read the double into a string
                    if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, (uint32_t)(((uint32_t)(precision - 1)) | 0x80000000u)))
                        formatting_flags |= STBSP__NEGATIVE;

                    // clamp the precision and delete extra zeros after clamp
                    n = (uint32_t) precision;
                    if (l > (uint32_t)precision)
                        l = (uint32_t) precision;
                    while ((l > 1) && (precision) && (source_ptr[l - 1] == '0')) {
                        --precision;
                        --l;
                    }

                    // should we use %e
                    if ((decimal_precision <= -4) || (decimal_precision > (int32_t)n)) {
                        if (precision > (int32_t)l)
                        precision = (int32_t) l - 1;
                        else if (precision)
                        --precision; // when using %e, there is one digit before the decimal
                        goto L_DO_EXP_FROMG;
                    }
                    // this is the insane action to get the precision to match %g semantics for %f
                    if (decimal_precision > 0) {
                        precision = (decimal_precision < (int32_t)l) ? (int32_t)(l - (uint32_t) decimal_precision) : 0;
                    } else {
                        precision = -decimal_precision + ((precision > (int32_t)l) ? (int32_t) l : precision);
                    }
                    goto L_DO_FLOAT_FROMG;

                case 'E': // float
                case 'e': // float
                    h = (f.data[0] == 'E') ? hexu : hex;
                    float_value = va_arg(va, double);
                    if (precision == -1)
                        precision = 6; // default is 6
                    // read the double into a string
                    if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, ((uint32_t) precision) | 0x80000000u))
                        formatting_flags |= STBSP__NEGATIVE;
                L_DO_EXP_FROMG:
                    tail[0] = 0;
                    stbsp__lead_sign(formatting_flags, lead);
                    if (decimal_precision == STBSP__SPECIAL) {
                        string = (char *)source_ptr;
                        comma_spacing = 0;
                        precision = 0;
                        goto L_STRING_COPY;
                    }
                    string = num + 64;
                    // handle leading chars
                    *string++ = source_ptr[0];

                    if (precision)
                        *string++ = stbsp__period;

                    // handle after decimal
                    if ((l - 1u) > (uint32_t)precision)
                        l = (uint32_t) precision + 1u;
                    for (n = 1; n < l; n++)
                        *string++ = source_ptr[n];
                    // trailing zeros
                    trailing_zeros = precision - (int32_t)(l - 1u);
                    precision = 0;
                    // dump expo
                    tail[1] = h[0xe];
                    decimal_precision -= 1;
                    if (decimal_precision < 0) {
                        tail[2] = '-';
                        decimal_precision = -decimal_precision;
                    } else
                        tail[2] = '+';

                    n = (decimal_precision >= 100) ? 5 : 4;

                    tail[0] = (char)n;
                    for (;;) {
                        tail[n] = '0' + decimal_precision % 10;
                        if (n <= 3)
                        break;
                        --n;
                        decimal_precision /= 10;
                    }
                    comma_spacing = 1u + (3u << 24); // how many tens
                    goto flt_lead;

                case 'f': // float
                    float_value = va_arg(va, double);
                doafloat:
                    // do kilos
                    if (formatting_flags & STBSP__METRIC_SUFFIX) {
                        double divisor;
                        divisor = 1000.0f;
                        if (formatting_flags & STBSP__METRIC_1024)
                        divisor = 1024.0;
                        while (formatting_flags < 0x4000000) {
                        if ((float_value < divisor) && (float_value > -divisor))
                            break;
                        float_value /= divisor;
                        formatting_flags += 0x1000000;
                        }
                    }
                    if (precision == -1)
                        precision = 6; // default is 6
                    // read the double into a string
                    if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, (uint32_t) precision))
                        formatting_flags |= STBSP__NEGATIVE;
                L_DO_FLOAT_FROMG:
                    tail[0] = 0;
                    stbsp__lead_sign(formatting_flags, lead);
                    if (decimal_precision == STBSP__SPECIAL) {
                        string = (char *)source_ptr;
                        comma_spacing = 0;
                        precision = 0;
                        goto L_STRING_COPY;
                    }
                    string = num + 64;

                    // handle the three decimal varieties
                    if (decimal_precision <= 0)
                    {
                        // handle 0.000*000xxxx
                        *string++ = '0';
                        if (precision)
                        *string++ = stbsp__period;
                        n = (uint32_t)(-decimal_precision);
                        if ((int32_t)n > precision)
                        n = (uint32_t) precision;

                        JSL_MEMSET(string, '0', n);
                        string += n;

                        if ((int32_t)(l + n) > precision)
                        l = (uint32_t)(precision - (int32_t) n);

                        JSL_MEMCPY(string, source_ptr, l);
                        string += l;
                        source_ptr += l;

                        trailing_zeros = precision - (int32_t)(n + l);
                        comma_spacing = 1u + (3u << 24); // how many tens did we write (for commas below)
                    }
                    else
                    {
                        comma_spacing = (formatting_flags & STBSP__TRIPLET_COMMA) ? ((600 - (uint32_t)decimal_precision) % 3) : 0;
                        if ((uint32_t)decimal_precision >= l) {
                        // handle xxxx000*000.0
                        n = 0;
                        for (;;) {
                            if ((formatting_flags & STBSP__TRIPLET_COMMA) && (++comma_spacing == 4)) {
                                comma_spacing = 0;
                                *string++ = stbsp__comma;
                            } else {
                                *string++ = source_ptr[n];
                                ++n;
                                if (n >= l)
                                    break;
                            }
                        }
                        if (n < (uint32_t)decimal_precision) {
                            n = (uint32_t)(decimal_precision - (int32_t) n);
                            if ((formatting_flags & STBSP__TRIPLET_COMMA) == 0) {
                                while (n) {
                                    if ((((uintptr_t)string) & 3) == 0)
                                    break;
                                    *string++ = '0';
                                    --n;
                                }
                                while (n >= 4) {
                                    *(uint32_t *)string = 0x30303030;
                                    string += 4;
                                    n -= 4;
                                }
                            }
                            while (n) {
                                if ((formatting_flags & STBSP__TRIPLET_COMMA) && (++comma_spacing == 4)) {
                                    comma_spacing = 0;
                                    *string++ = stbsp__comma;
                                } else {
                                    *string++ = '0';
                                    --n;
                                }
                            }
                        }
                        comma_spacing = (uint32_t)(string - (num + 64)); // comma_spacing is how many tens
                        comma_spacing += (3u << 24);
                        if (precision) {
                            *string++ = stbsp__period;
                            trailing_zeros = precision;
                        }
                        } else {
                        // handle xxxxx.xxxx000*000
                        n = 0;
                        for (;;) {
                            if ((formatting_flags & STBSP__TRIPLET_COMMA) && (++comma_spacing == 4)) {
                                comma_spacing = 0;
                                *string++ = stbsp__comma;
                            } else {
                                *string++ = source_ptr[n];
                                ++n;
                                if (n >= (uint32_t)decimal_precision)
                                    break;
                            }
                        }
                        comma_spacing = (uint32_t)(string - (num + 64)); // comma_spacing is how many tens
                        comma_spacing += (3u << 24);
                        if (precision)
                            *string++ = stbsp__period;
                        if ((l - (uint32_t) decimal_precision) > (uint32_t)precision)
                            l = (uint32_t)(precision + decimal_precision);
                        while (n < l) {
                            *string++ = source_ptr[n];
                            ++n;
                        }
                        trailing_zeros = precision - (int32_t)(l - (uint32_t) decimal_precision);
                        }
                    }
                    precision = 0;

                    // handle k,m,g,t
                    if (formatting_flags & STBSP__METRIC_SUFFIX) {
                        char idx;
                        idx = 1;
                        if (formatting_flags & STBSP__METRIC_NOSPACE)
                        idx = 0;
                        tail[0] = idx;
                        tail[1] = ' ';
                        {
                        if (formatting_flags >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                            if (formatting_flags & STBSP__METRIC_1024)
                                tail[idx + 1] = "_KMGT"[formatting_flags >> 24];
                            else
                                tail[idx + 1] = "_kMGT"[formatting_flags >> 24];
                            idx++;
                            // If printing kibits and not in jedec, add the 'i'.
                            if (formatting_flags & STBSP__METRIC_1024 && !(formatting_flags & STBSP__METRIC_JEDEC)) {
                                tail[idx + 1] = 'i';
                                idx++;
                            }
                            tail[0] = idx;
                        }
                        }
                    };

                flt_lead:
                    // get the length that we copied
                    l = (uint32_t)(string - (num + 64));
                    string = num + 64;
                    goto L_STRING_COPY;

                case 'B': // upper binary
                case 'b': // lower binary
                    h = (f.data[0] == 'B') ? hexu : hex;
                    lead[0] = 0;
                    if (formatting_flags & STBSP__LEADING_0X) {
                        lead[0] = 2;
                        lead[1] = '0';
                        lead[2] = h[0xb];
                    }
                    l = (8 << 4) | (1 << 8);
                    goto L_RADIX_NUM;

                case 'o': // octal
                    h = hexu;
                    lead[0] = 0;
                    if (formatting_flags & STBSP__LEADING_0X) {
                        lead[0] = 1;
                        lead[1] = '0';
                    }
                    l = (3 << 4) | (3 << 8);
                    goto L_RADIX_NUM;

                case 'p': // pointer
                    formatting_flags |= (sizeof(void *) == 8) ? STBSP__INTMAX : 0;
                    precision = sizeof(void *) * 2;
                    formatting_flags &= (uint32_t) ~STBSP__LEADINGZERO; // 'p' only prints the pointer with zeros
                                                // fall through - to X

                case 'X': // upper hex
                case 'x': // lower hex
                    h = (f.data[0] == 'X') ? hexu : hex;
                    l = (4 << 4) | (4 << 8);
                    lead[0] = 0;
                    if (formatting_flags & STBSP__LEADING_0X) {
                        lead[0] = 2;
                        lead[1] = '0';
                        lead[2] = h[16];
                    }

                L_RADIX_NUM:
                    // get the number
                    if (formatting_flags & STBSP__INTMAX)
                        n64 = va_arg(va, uint64_t);
                    else
                        n64 = va_arg(va, uint32_t);

                    string = num + STBSP__NUMSZ;
                    decimal_precision = 0;
                    // clear tail, and clear leading if value is zero
                    tail[0] = 0;
                    if (n64 == 0) {
                        lead[0] = 0;
                        if (precision == 0) {
                        l = 0;
                        comma_spacing = 0;
                        goto L_STRING_COPY;
                        }
                    }
                    // convert to string
                    for (;;) {
                        *--string = h[n64 & ((1 << (l >> 8)) - 1)];
                        n64 >>= (l >> 8);
                        if (!((n64) || ((int32_t)((num + STBSP__NUMSZ) - string) < precision)))
                        break;
                        if (formatting_flags & STBSP__TRIPLET_COMMA) {
                        ++l;
                        if ((l & 15) == ((l >> 4) & 15)) {
                            l &= ~UINT32_C(15);
                            *--string = stbsp__comma;
                        }
                        }
                    };
                    // get the tens and the comma pos
                    comma_spacing = (uint32_t)((num + STBSP__NUMSZ) - string) + ((((l >> 4) & 15)) << 24);
                    // get the length that we copied
                    l = (uint32_t)((num + STBSP__NUMSZ) - string);
                    // copy it
                    goto L_STRING_COPY;

                case 'u': // unsigned
                case 'i':
                case 'd': // integer
                    // get the integer and abs it
                    if (formatting_flags & STBSP__INTMAX) {
                        int64_t i64 = va_arg(va, int64_t);
                        n64 = (uint64_t)i64;
                        if ((f.data[0] != 'u') && (i64 < 0)) {
                        n64 = (uint64_t)-i64;
                        formatting_flags |= STBSP__NEGATIVE;
                        }
                    } else {
                        int32_t i = va_arg(va, int32_t);
                        n64 = (uint32_t)i;
                        if ((f.data[0] != 'u') && (i < 0)) {
                        n64 = (uint32_t)-i;
                        formatting_flags |= STBSP__NEGATIVE;
                        }
                    }

                    if (formatting_flags & STBSP__METRIC_SUFFIX) {
                        if (n64 < 1024)
                        precision = 0;
                        else if (precision == -1)
                        precision = 1;
                        float_value = (double)(int64_t)n64;
                        goto doafloat;
                    }

                    // convert to string
                    string = num + STBSP__NUMSZ;
                    l = 0;

                    for (;;) {
                        // do in 32-bit chunks (avoid lots of 64-bit divides even with constant denominators)
                        char *o = string - 8;
                        if (n64 >= 100000000) {
                        n = (uint32_t)(n64 % 100000000);
                        n64 /= 100000000;
                        } else {
                        n = (uint32_t)n64;
                        n64 = 0;
                        }
                        if ((formatting_flags & STBSP__TRIPLET_COMMA) == 0) {
                        do {
                            string -= 2;
                            *(uint16_t *)string = *(uint16_t *)&stbsp__digitpair.pair[(n % 100) * 2];
                            n /= 100;
                        } while (n);
                        }
                        while (n) {
                        if ((formatting_flags & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                            l = 0;
                            *--string = stbsp__comma;
                            --o;
                        } else {
                            *--string = (char)(n % 10) + '0';
                            n /= 10;
                        }
                        }
                        if (n64 == 0) {
                        if ((string[0] == '0') && (string != (num + STBSP__NUMSZ)))
                            ++string;
                        break;
                        }
                        while (string != o)
                        if ((formatting_flags & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                            l = 0;
                            *--string = stbsp__comma;
                            --o;
                        } else {
                            *--string = '0';
                        }
                    }

                    tail[0] = 0;
                    stbsp__lead_sign(formatting_flags, lead);

                    // get the length that we copied
                    l = (uint32_t)((num + STBSP__NUMSZ) - string);
                    if (l == 0) {
                        *--string = '0';
                        l = 1;
                    }
                    comma_spacing = l + (3u << 24);
                    if (precision < 0)
                        precision = 0;

                L_STRING_COPY:
                    // get field_width=leading/trailing space, precision=leading zeros
                    if (precision < (int32_t)l)
                        precision = (int32_t) l;
                    n = (uint32_t)(precision + lead[0] + tail[0] + trailing_zeros);
                    if (field_width < (int32_t)n)
                        field_width = (int32_t) n;
                    field_width -= (int32_t) n;
                    precision -= (int32_t) l;

                    // handle right justify and leading zeros
                    if ((formatting_flags & STBSP__LEFTJUST) == 0)
                    {
                        if (formatting_flags & STBSP__LEADINGZERO) // if leading zeros, everything is in precision
                        {
                            precision = (field_width > precision) ? field_width : precision;
                            field_width = 0;
                        }
                        else
                        {
                            formatting_flags &= (uint32_t) ~STBSP__TRIPLET_COMMA; // if no leading zeros, then no commas
                        }
                    }

                    // copy the spaces and/or zeros
                    if (field_width + precision)
                    {
                        int32_t i;
                        uint32_t c;

                        // copy leading spaces (or when doing %8.4d stuff)
                        if ((formatting_flags & STBSP__LEFTJUST) == 0)
                        {
                            while (field_width > 0)
                            {
                                stbsp__cb_buf_clamp(i, field_width);

                                field_width -= i;
                                JSL_MEMSET(buffer_cursor, ' ', (size_t) i);
                                buffer_cursor += i;

                                stbsp__chk_cb_buf(1);
                            }
                        }

                        // copy leader
                        source_ptr = lead + 1;
                        while (lead[0])
                        {
                            stbsp__cb_buf_clamp(i, lead[0]);

                            lead[0] -= (char) i;
                            JSL_MEMCPY(buffer_cursor, source_ptr, (size_t) i);
                            buffer_cursor += i;
                            source_ptr += i;

                            stbsp__chk_cb_buf(1);
                        }

                        // copy leading zeros
                        c = comma_spacing >> 24;
                        comma_spacing &= 0xffffff;
                        comma_spacing = (formatting_flags & STBSP__TRIPLET_COMMA) ? ((uint32_t)(c - ((((uint32_t) precision) + comma_spacing) % (c + 1u)))) : 0;

                        while (precision > 0)
                        {
                        stbsp__cb_buf_clamp(i, precision);
                        precision -= i;

                        if (JSL_IS_BITFLAG_NOT_SET(formatting_flags, STBSP__TRIPLET_COMMA))
                        {
                            JSL_MEMSET(buffer_cursor, '0', (size_t) i);
                            buffer_cursor += i;
                        }
                        else
                        {
                            while (i)
                            {
                                if (comma_spacing == c)
                                {
                                    comma_spacing = 0;
                                    *buffer_cursor = (uint8_t) stbsp__comma;
                                }
                                else
                                {
                                    *buffer_cursor = '0';
                                }

                                ++buffer_cursor;
                                ++comma_spacing;
                                --i;
                            }
                        }

                        stbsp__chk_cb_buf(1);
                        }

                    }

                    // copy leader if there is still one
                    source_ptr = lead + 1;
                    while (lead[0])
                    {
                        int32_t i;
                        stbsp__cb_buf_clamp(i, lead[0]);

                        lead[0] -= (char) i;
                        JSL_MEMCPY(buffer_cursor, source_ptr, (size_t) i);
                        buffer_cursor += i;
                        source_ptr += i;

                        stbsp__chk_cb_buf(1);
                    }

                    // copy the string
                    n = l;
                    while (n)
                    {
                        int32_t i;
                        stbsp__cb_buf_clamp(i, (int32_t) n);

                        JSL_MEMCPY(buffer_cursor, string, (size_t) i);

                        n -= (uint32_t) i;
                        buffer_cursor += i;
                        string += i;

                        stbsp__chk_cb_buf(1);
                    }

                    // copy trailing zeros
                    while (trailing_zeros)
                    {
                        int32_t i;
                        stbsp__cb_buf_clamp(i, trailing_zeros);

                        trailing_zeros -= i;
                        JSL_MEMSET(buffer_cursor, '0', (size_t) i);
                        buffer_cursor += i;

                        stbsp__chk_cb_buf(1);
                    }

                    // copy tail if there is one
                    source_ptr = tail + 1;
                    while (tail[0])
                    {
                        int32_t i;
                        stbsp__cb_buf_clamp(i, tail[0]);

                        tail[0] -= (char)i;
                        JSL_MEMCPY(buffer_cursor, source_ptr, (size_t) i);
                        buffer_cursor += i;
                        source_ptr += i;

                        stbsp__chk_cb_buf(1);
                    }

                    // handle the left justify
                    if (formatting_flags & STBSP__LEFTJUST)
                    {
                        if (field_width > 0)
                        {
                        while (field_width)
                        {
                            int32_t i;
                            stbsp__cb_buf_clamp(i, field_width);

                            field_width -= i;
                            JSL_MEMSET(buffer_cursor, ' ', (size_t) i);
                            buffer_cursor += i;

                            stbsp__chk_cb_buf(1);
                        }
                        }
                    }

                    break;

                default: // unknown, just copy code
                    string = num + STBSP__NUMSZ - 1;
                    *string = (char) f.data[0];
                    l = 1;
                    field_width = formatting_flags = 0;
                    lead[0] = 0;
                    tail[0] = 0;
                    precision = 0;
                    decimal_precision = 0;
                    comma_spacing = 0;
                    goto L_STRING_COPY;
            }

            JSL_FATPTR_ADVANCE(f, 1);
        }


        L_END_FORMAT:

        if (!callback)
            *buffer_cursor = 0;
        else
            stbsp__flush_cb();

        done:
        return tlen + (int32_t)(buffer_cursor - buffer);
    }

    // cleanup
    #undef STBSP__LEFTJUST
    #undef STBSP__LEADINGPLUS
    #undef STBSP__LEADINGSPACE
    #undef STBSP__LEADING_0X
    #undef STBSP__LEADINGZERO
    #undef STBSP__INTMAX
    #undef STBSP__TRIPLET_COMMA
    #undef STBSP__NEGATIVE
    #undef STBSP__METRIC_SUFFIX
    #undef STBSP__NUMSZ
    #undef stbsp__chk_cb_bufL
    #undef stbsp__chk_cb_buf
    #undef stbsp__flush_cb
    #undef stbsp__cb_buf_clamp

    // ============================================================================
    //   wrapper functions

    typedef struct stbsp__context {
        JSLFatPtr buffer;
        int64_t length;
        uint8_t tmp[JSL_FORMAT_MIN_BUFFER];
    } stbsp__context;

    static uint8_t* stbsp__clamp_callback(uint8_t* buf, void *user, int64_t len)
    {
        stbsp__context* context = (stbsp__context*) user;
        context->length += len;

        if (len > context->buffer.length)
            len = context->buffer.length;

        if (len)
        {
            if (buf != context->buffer.data)
            {
                JSL_MEMCPY(context->buffer.data, buf, (size_t) len);
            }

            context->buffer.data += len;
            context->buffer.length -= len;
        }

        if (context->buffer.length <= 0)
            return context->tmp;

        return (context->buffer.length >= JSL_FORMAT_MIN_BUFFER) ?
            context->buffer.data
            : context->tmp; // go direct into buffer if you can
    }

    static uint8_t* stbsp__count_clamp_callback(uint8_t* buf, void* user, int64_t len)
    {
        stbsp__context* context = (stbsp__context*) user;
        (void) sizeof(buf);

        context->length += len;
        return context->tmp; // go direct into buffer if you can
    }

    JSL__ASAN_OFF int64_t jsl_format_valist(JSLFatPtr* buffer, JSLFatPtr fmt, va_list va )
    {
        stbsp__context context;
        context.length = 0;

        if ((buffer->length == 0) && buffer->data == NULL)
        {
            context.buffer.data = NULL;
            context.buffer.length = 0;

            jsl_format_callback(
                stbsp__count_clamp_callback,
                &context,
                context.tmp,
                fmt,
                va
            );
        }
        else
        {
            context.buffer = *buffer;

            jsl_format_callback(
                stbsp__clamp_callback,
                &context,
                stbsp__clamp_callback(0, &context, 0),
                fmt,
                va
            );
        }

        *buffer = context.buffer;

        return context.length;
    }

    struct JSL__ArenaContext
    {
        JSLArena* arena;
        JSLFatPtr current_allocation;
        uint8_t* cursor;
        uint8_t buffer[JSL_FORMAT_MIN_BUFFER];
    };

    static uint8_t* format_arena_callback(uint8_t *buf, void *user, int64_t len)
    {
        struct JSL__ArenaContext* context = (struct JSL__ArenaContext*) user;

        // First call
        if (context->cursor == NULL)
        {
            context->current_allocation = jsl_arena_allocate(context->arena, len, false);
            if (context->current_allocation.data == NULL)
                return 0;

            context->cursor = context->current_allocation.data;
        }
        else
        {
            int64_t new_length = context->current_allocation.length + len;
            context->current_allocation = jsl_arena_reallocate(
                context->arena,
                context->current_allocation,
                new_length
            );
            if (context->current_allocation.data == NULL)
                return 0;
        }

        JSL_MEMCPY(context->cursor, buf, (size_t) len);
        context->cursor += len;

        return context->buffer;
    }

    JSL__ASAN_OFF JSLFatPtr jsl_format(JSLArena* arena, JSLFatPtr fmt, ...)
    {
        va_list va;
        va_start(va, fmt);

        struct JSL__ArenaContext context;
        context.arena = arena;
        context.current_allocation.data = NULL;
        context.current_allocation.length = 0;
        context.cursor = NULL;

        jsl_format_callback(
            format_arena_callback,
            &context,
            context.buffer,
            fmt,
            va
        );

        va_end(va);

        JSLFatPtr ret = {0};
        int64_t write_length = context.cursor - context.current_allocation.data;
        if (context.cursor != NULL && write_length > 0)
        {
            ret.data = context.current_allocation.data;
            ret.length = write_length;
        }

        return ret;
    }

    JSL__ASAN_OFF int64_t jsl_format_buffer(JSLFatPtr* buffer, JSLFatPtr fmt, ...)
    {
        int64_t result;
        va_list va;
        va_start(va, fmt);

        result = jsl_format_valist(buffer, fmt, va);
        va_end(va);

        return result;
    }

    // =======================================================================
    //   low level float utility functions

    // copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
    #define STBSP__COPYFP(dest, src)                   \
    {                                               \
        int32_t counter;                                      \
        for (counter = 0; counter < 8; counter++)                   \
            ((char *)&dest)[counter] = ((char *)&src)[counter]; \
    }

    // get float info
    static int32_t stbsp__real_to_parts(int64_t *bits, int32_t *expo, double value)
    {
    double d;
    int64_t b = 0;

    // load value and round at the frac_digits
    d = value;

    STBSP__COPYFP(b, d);

    const int64_t mantissa_mask = (int64_t)((((uint64_t)1u) << 52u) - 1u);
    *bits = b & mantissa_mask;
    *expo = (int32_t)(((b >> 52) & 2047) - 1023);

    return (int32_t)((uint64_t) b >> 63);
    }

    static double const stbsp__bot[23] = {
        1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
        1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022
    };
    static double const stbsp__negbot[22] = {
        1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
        1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022
    };
    static double const stbsp__negboterr[22] = {
        -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
        4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
        -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032, 2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
        2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,  -4.8596774326570872e-039
    };
    static double const stbsp__top[13] = {
        1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299
    };
    static double const stbsp__negtop[13] = {
        1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299
    };
    static double const stbsp__toperr[13] = {
        8388608,
        6.8601809640529717e+028,
        -7.253143638152921e+052,
        -4.3377296974619174e+075,
        -1.5559416129466825e+098,
        -3.2841562489204913e+121,
        -3.7745893248228135e+144,
        -1.7356668416969134e+167,
        -3.8893577551088374e+190,
        -9.9566444326005119e+213,
        6.3641293062232429e+236,
        -5.2069140800249813e+259,
        -5.2504760255204387e+282
    };
    static double const stbsp__negtoperr[13] = {
        3.9565301985100693e-040,  -2.299904345391321e-063,  3.6506201437945798e-086,  1.1875228833981544e-109,
        -5.0644902316928607e-132, -6.7156837247865426e-155, -2.812077463003139e-178,  -5.7778912386589953e-201,
        7.4997100559334532e-224,  -4.6439668915134491e-247, -6.3691100762962136e-270, -9.436808465446358e-293,
        8.0970921678014997e-317
    };

    static uint64_t const stbsp__powten[20] = {
        1,
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000,
        10000000000ULL,
        100000000000ULL,
        1000000000000ULL,
        10000000000000ULL,
        100000000000000ULL,
        1000000000000000ULL,
        10000000000000000ULL,
        100000000000000000ULL,
        1000000000000000000ULL,
        10000000000000000000ULL
    };
    #define stbsp__tento19th (1000000000000000000ULL)

    #define stbsp__ddmulthi(oh, ol, xh, yh)                             \
    {                                                                   \
        double ahi = 0, alo, bhi = 0, blo;                              \
        int64_t bt;                                                     \
        oh = xh * yh;                                                   \
        STBSP__COPYFP(bt, xh);                                          \
        bt &= ((~(uint64_t)0) << 27);                                   \
        STBSP__COPYFP(ahi, bt);                                         \
        alo = xh - ahi;                                                 \
        STBSP__COPYFP(bt, yh);                                          \
        bt &= ((~(uint64_t)0) << 27);                                   \
        STBSP__COPYFP(bhi, bt);                                         \
        blo = yh - bhi;                                                 \
        ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo;    \
    }

    #define stbsp__ddtoS64(ob, xh, xl)                                  \
    {                                                                   \
        double ahi = 0, alo, vh, t;                                     \
        ob = (int64_t)xh;                                               \
        vh = (double)ob;                                                \
        ahi = (xh - vh);                                                \
        t = (ahi - xh);                                                 \
        alo = (xh - (ahi - t)) - (vh + t);                              \
        ob += (int64_t)(ahi + alo + xl);                                \
    }

    #define stbsp__ddrenorm(oh, ol) \
    {                            \
        double s;                 \
        s = oh + ol;              \
        ol = ol - (s - oh);       \
        oh = s;                   \
    }

    #define stbsp__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

    #define stbsp__ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);

    static void stbsp__raise_to_power10(double *ohi, double *olo, double d, int32_t power) // power can be -323 to +350
    {
        double ph, pl;
        if ((power >= 0) && (power <= 22)) {
            stbsp__ddmulthi(ph, pl, d, stbsp__bot[power]);
        } else {
            int32_t e, et, eb;
            double p2h, p2l;

            e = power;
            if (power < 0)
                e = -e;
            et = (e * 0x2c9) >> 14; /* %23 */
            if (et > 13)
                et = 13;
            eb = e - (et * 23);

            ph = d;
            pl = 0.0;
            if (power < 0) {
                if (eb) {
                    --eb;
                    stbsp__ddmulthi(ph, pl, d, stbsp__negbot[eb]);
                    stbsp__ddmultlos(ph, pl, d, stbsp__negboterr[eb]);
                }
                if (et) {
                    stbsp__ddrenorm(ph, pl);
                    --et;
                    stbsp__ddmulthi(p2h, p2l, ph, stbsp__negtop[et]);
                    stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__negtop[et], stbsp__negtoperr[et]);
                    ph = p2h;
                    pl = p2l;
                }
            } else {
                if (eb) {
                    e = eb;
                    if (eb > 22)
                    eb = 22;
                    e -= eb;
                    stbsp__ddmulthi(ph, pl, d, stbsp__bot[eb]);
                    if (e) {
                    stbsp__ddrenorm(ph, pl);
                    stbsp__ddmulthi(p2h, p2l, ph, stbsp__bot[e]);
                    stbsp__ddmultlos(p2h, p2l, stbsp__bot[e], pl);
                    ph = p2h;
                    pl = p2l;
                    }
                }
                if (et) {
                    stbsp__ddrenorm(ph, pl);
                    --et;
                    stbsp__ddmulthi(p2h, p2l, ph, stbsp__top[et]);
                    stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__top[et], stbsp__toperr[et]);
                    ph = p2h;
                    pl = p2l;
                }
            }
        }
        stbsp__ddrenorm(ph, pl);
        *ohi = ph;
        *olo = pl;
    }

    // given a float value, returns the significant bits in bits, and the position of the
    //   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
    //   returned in the decimal_pos parameter.
    // frac_digits is absolute normally, but if you want from first significant digits (got %g and %e), or in 0x80000000
    static int32_t stbsp__real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits)
    {
        double d;
        int64_t bits = 0;
        int32_t expo, e, ng, tens;

        d = value;
        STBSP__COPYFP(bits, d);
        expo = (int32_t)((bits >> 52) & 2047);
        ng = (int32_t)((uint64_t) bits >> 63);
        if (ng)
            d = -d;

        if (expo == 2047) // is nan or inf?
        {
            *start = ((uint64_t) bits & ((((uint64_t) 1u) << 52u) - 1u)) ? "NaN" : "Inf";
            *decimal_pos = STBSP__SPECIAL;
            *len = 3;
            return ng;
        }

        if (expo == 0) // is zero or denormal
        {
            if (((uint64_t) bits << 1) == 0) // do zero
            {
                *decimal_pos = 1;
                *start = out;
                out[0] = '0';
                *len = 1;
                return ng;
            }
            // find the right expo for denormals
            {
                int64_t v = ((uint64_t)1) << 51;
                while ((bits & v) == 0) {
                    --expo;
                    v >>= 1;
                }
            }
        }

        // find the decimal exponent as well as the decimal bits of the value
        {
            double ph, pl;

            // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of log10 of all expos 1..2046
            tens = expo - 1023;
            tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

            // move the significant bits into position and stick them into an int32_t
            stbsp__raise_to_power10(&ph, &pl, d, 18 - tens);

            // get full as much precision from double-double as possible
            stbsp__ddtoS64(bits, ph, pl);

            // check if we undershot
            if (((uint64_t)bits) >= stbsp__tento19th)
                ++tens;
        }

        // now do the rounding in integer land
        frac_digits = (frac_digits & 0x80000000u) ? ((frac_digits & 0x7ffffffu) + 1u) : (frac_digits + (uint32_t) tens);
        if ((frac_digits < 24)) {
            uint32_t dg = 1;
            if ((uint64_t)bits >= stbsp__powten[9])
                dg = 10;
            while ((uint64_t)bits >= stbsp__powten[dg]) {
                ++dg;
                if (dg == 20)
                    goto L_NO_ROUND;
            }
            if (frac_digits < dg) {
                uint64_t r;
                // add 0.5 at the right position and round
                e = (int32_t) (dg - frac_digits);
                if ((uint32_t)e >= 24)
                    goto L_NO_ROUND;
                r = stbsp__powten[e];
                bits = bits + (int64_t) (r / 2);
                if ((uint64_t) bits >= stbsp__powten[dg])
                    ++tens;
                bits /= r;
            }
        L_NO_ROUND:;
        }

        // kill long trailing runs of zeros
        if (bits) {
            uint32_t n;
            for (;;) {
                if (bits <= 0xffffffff)
                    break;
                if (bits % 1000)
                    goto donez;
                bits /= 1000;
            }
            n = (uint32_t)bits;
            while ((n % 1000) == 0)
                n /= 1000;
            bits = n;
        donez:;
        }

        // convert to string
        out += 64;
        e = 0;
        for (;;) {
            uint32_t n;
            char *o = out - 8;
            // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant denomiators be damned)
            if (bits >= 100000000) {
                n = (uint32_t)(bits % 100000000);
                bits /= 100000000;
            } else {
                n = (uint32_t)bits;
                bits = 0;
            }
            while (n) {
                out -= 2;
                *(uint16_t *)out = *(uint16_t *)&stbsp__digitpair.pair[(n % 100) * 2];
                n /= 100;
                e += 2;
            }
            if (bits == 0) {
                if ((e) && (out[0] == '0')) {
                    ++out;
                    --e;
                }
                break;
            }
            while (out != o) {
                *--out = '0';
                ++e;
            }
        }

        *decimal_pos = tens;
        *start = out;
        *len = (uint32_t) e;
        return ng;
    }

    // clean up
    #undef stbsp__ddmulthi
    #undef stbsp__ddrenorm
    #undef stbsp__ddmultlo
    #undef stbsp__ddmultlos
    #undef STBSP__SPECIAL
    #undef STBSP__COPYFP
    #undef STBSP__UNALIGNED


    /**
     *
     *
     *                      FILE UTILITIES
     *
     *
     */

    #ifdef JSL_INCLUDE_FILE_UTILS

    JSLGetFileSizeResultEnum jsl_get_file_size(
        JSLFatPtr path,
        int64_t* out_size,
        int32_t* out_os_error_code
    )
    {
        char path_buffer[FILENAME_MAX + 1];

        JSLGetFileSizeResultEnum result = JSL_GET_FILE_SIZE_BAD_PARAMETERS;
        bool good_params = false;

        if (path.data != NULL && path.length > 0 && out_size != NULL)
        {
            // File system APIs require a null terminated string
            JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
            path_buffer[path.length] = '\0';
            good_params = true;
        }

        #if JSL_IS_WINDOWS

            struct _stat64 st_win = {0};
            int stat_ret = -1;

            if (good_params)
            {
                stat_ret = _stat64(path_buffer, &st_win);
            }

            bool is_regular = false;
            if (stat_ret == 0)
            {
                is_regular = JSL_IS_BITFLAG_SET(st_win.st_mode, _S_IFREG);
            }
            else
            {
                result = JSL_GET_FILE_SIZE_NOT_FOUND;
                if (out_os_error_code != NULL)
                    *out_os_error_code = errno;
            }

            if (is_regular)
            {
                *out_size = (int64_t) st_win.st_size;
                result = JSL_GET_FILE_SIZE_OK;
            }
            else if (!is_regular && stat_ret == 0)
            {
                result = JSL_GET_FILE_SIZE_NOT_REGULAR_FILE;
            }

        #elif JSL_IS_POSIX

            struct stat st_posix;
            int stat_ret = -1;

            if (good_params)
            {
                stat_ret = stat(path_buffer, &st_posix);
            }

            bool is_regular = false;
            if (stat_ret == 0)
            {
                is_regular = (sb.st_mode & S_IFMT) == S_IFREG;
            }
            else
            {
                result = JSL_GET_FILE_SIZE_NOT_FOUND;
                if (out_os_error_code != NULL)
                    *out_os_error_code = errno;
            }

            if (is_regular)
            {
                *out_size = (int64_t) st_posix.st_size;
                result = JSL_GET_FILE_SIZE_OK;
            }
            else if (stat_ret == 0 && !is_regular)
            {
                result = JSL_GET_FILE_SIZE_NOT_REGULAR_FILE;
            }

        #else
            JSL_ASSERT(0 && "File utils only work on Windows or POSIX platforms.");
        #endif

        return result;
    }

    static inline int64_t jsl__get_file_size_from_fileno(int32_t file_descriptor)
    {
        int64_t result_size = -1;
        bool stat_success = false;

        #if JSL_IS_WINDOWS

            struct _stat64 file_info;
            int stat_result = _fstat64(file_descriptor, &file_info);
            if (stat_result == 0)
            {
                stat_success = true;
                result_size = file_info.st_size;
            }

        #elif JSL_IS_POSIX
            struct stat file_info;
            int stat_result = fstat(file_descriptor, &file_info);
            if (stat_result == 0)
            {
                stat_success = true;
                result_size = (int64_t) file_info.st_size;
            }

        #endif

        if (!stat_success)
        {
            result_size = -1;
        }

        return result_size;
    }

    JSLLoadFileResultEnum jsl_load_file_contents(
        JSLArena* arena,
        JSLFatPtr path,
        JSLFatPtr* out_contents,
        int32_t* out_errno
    )
    {
        JSLLoadFileResultEnum res = JSL_FILE_LOAD_BAD_PARAMETERS;
        char path_buffer[FILENAME_MAX + 1];

        bool got_path = false;
        if (path.data != NULL
            && path.length > 0
            && path.length < FILENAME_MAX
            && arena != NULL
            && arena->current != NULL
            && arena->start != NULL
            && arena->end != NULL
            && out_contents != NULL
        )
        {
            // File system APIs require a null terminated string

            JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
            path_buffer[path.length] = '\0';
            got_path = true;
        }

        int32_t file_descriptor = -1;
        bool opened_file = false;
        if (got_path)
        {
            #if JSL_IS_WINDOWS
                errno_t open_err = _sopen_s(
                    &file_descriptor,
                    path_buffer,
                    _O_BINARY,
                    _SH_DENYNO,
                    _S_IREAD
                );
                opened_file = (open_err == 0);
            #elif JSL_IS_POSIX
                file_descriptor = open(path_buffer, 0);
                opened_file = file_descriptor > -1;
            #else
                #error "Unsupported platform"
            #endif
        }

        int64_t file_size = -1;
        bool got_file_size = false;
        if (opened_file)
        {
            file_size = jsl__get_file_size_from_fileno(file_descriptor);
            got_file_size = file_size > -1;
        }
        else
        {
            res = JSL_FILE_LOAD_COULD_NOT_OPEN;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        JSLFatPtr allocation = {0};
        bool got_memory = false;
        if (got_file_size)
        {
            allocation = jsl_arena_allocate(arena, file_size, false);
            got_memory = allocation.data != NULL && allocation.length >= file_size;
        }
        else
        {
            res = JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        int64_t read_res = -1;
        bool did_read_data = false;
        if (got_memory)
        {
            #if JSL_IS_WINDOWS
                read_res = (int64_t) _read(file_descriptor, allocation.data, (unsigned int) file_size);
            #elif JSL_IS_POSIX
                read_res = (int64_t) read(file_descriptor, allocation.data, (size_t) file_size);
            #else
                #error "Unsupported platform"
            #endif

            did_read_data = read_res > -1;
        }
        else
        {
            res = JSL_FILE_LOAD_COULD_NOT_GET_MEMORY;
        }

        if (did_read_data)
        {
            out_contents->data = allocation.data;
            out_contents->length = read_res;
        }
        else
        {
            res = JSL_FILE_LOAD_READ_FAILED;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        bool did_close = false;
        if (opened_file)
        {
            #if JSL_IS_WINDOWS
                int32_t close_res = _close(file_descriptor);
            #elif JSL_IS_POSIX
                int32_t close_res = close(file_descriptor);
            #else
                #error "Unsupported platform"
            #endif

            did_close = close_res > -1;
        }

        if (opened_file && did_close)
        {
            res = JSL_FILE_LOAD_SUCCESS;
        }
        if (opened_file && !did_close)
        {
            res = JSL_FILE_LOAD_CLOSE_FAILED;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        return res;
    }

    JSLLoadFileResultEnum jsl_load_file_contents_buffer(
        JSLFatPtr* buffer,
        JSLFatPtr path,
        int32_t* out_errno
    )
    {
        char path_buffer[FILENAME_MAX + 1];
        JSLLoadFileResultEnum res = JSL_FILE_LOAD_BAD_PARAMETERS;

        bool got_path = false;
        if (path.data != NULL
            && path.length > 0
            && path.length < FILENAME_MAX
            && buffer != NULL
            && buffer->data != NULL
            && buffer->length > 0
        )
        {
            // File system APIs require a null terminated string

            JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
            path_buffer[path.length] = '\0';
            got_path = true;
        }

        int32_t file_descriptor = -1;
        bool opened_file = false;
        if (got_path)
        {
            #if JSL_IS_WINDOWS
                errno_t open_err = _sopen_s(
                    &file_descriptor,
                    path_buffer,
                    _O_BINARY,
                    _SH_DENYNO,
                    _S_IREAD
                );
                opened_file = (open_err == 0);
            #elif JSL_IS_POSIX
                file_descriptor = open(path_buffer, 0);
                opened_file = file_descriptor > -1;
            #else
                #error "Unsupported platform"
            #endif
        }

        int64_t file_size = -1;
        bool got_file_size = false;
        if (opened_file)
        {
            file_size = jsl__get_file_size_from_fileno(file_descriptor);
            got_file_size = file_size > -1;
        }
        else
        {
            res = JSL_FILE_LOAD_COULD_NOT_OPEN;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        int64_t read_res = -1;
        bool did_read_data = false;
        if (got_file_size)
        {
            int64_t read_size = JSL_MIN(file_size, buffer->length);

            #if JSL_IS_WINDOWS
                read_res = (int64_t) _read(file_descriptor, buffer->data, (unsigned int) read_size);
            #elif JSL_IS_POSIX
                read_res = (int64_t) read(file_descriptor, buffer->data, (size_t) read_size);
            #else
                #error "Unsupported platform"
            #endif

            did_read_data = read_res > -1;
        }
        else
        {
            res = JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        if (did_read_data)
        {
            buffer->data += read_res;
            buffer->length -= read_res;
        }
        else
        {
            res = JSL_FILE_LOAD_READ_FAILED;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        bool did_close = false;
        if (opened_file)
        {
            #if JSL_IS_WINDOWS
                int32_t close_res = _close(file_descriptor);
            #elif JSL_IS_POSIX
                int32_t close_res = close(file_descriptor);
            #else
                #error "Unsupported platform"
            #endif

            did_close = close_res > -1;
        }

        if (opened_file && did_close)
        {
            res = JSL_FILE_LOAD_SUCCESS;
        }
        if (opened_file && !did_close)
        {
            res = JSL_FILE_LOAD_CLOSE_FAILED;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        return res;
    }

    JSLWriteFileResultEnum jsl_write_file_contents(
        JSLFatPtr contents,
        JSLFatPtr path,
        int64_t* out_bytes_written,
        int32_t* out_errno
    )
    {
        char path_buffer[FILENAME_MAX + 1];
        JSLWriteFileResultEnum res = JSL_FILE_WRITE_BAD_PARAMETERS;

        bool got_path = false;
        if (path.data != NULL
            && contents.data != NULL
            && contents.length > 0
        )
        {
            JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
            path_buffer[path.length] = '\0';
            got_path = true;
        }

        int32_t file_descriptor = -1;
        bool opened_file = false;
        if (got_path)
        {
            #if JSL_IS_WINDOWS
                errno_t open_err = _sopen_s(
                    &file_descriptor,
                    path_buffer,
                    _O_CREAT,
                    _SH_DENYNO,
                    _S_IREAD | _S_IWRITE
                );
                opened_file = (open_err == 0);
            #elif JSL_IS_POSIX
                file_descriptor = open(path_buffer, O_CREAT, S_IRUSR | S_IWUSR);
                opened_file = file_descriptor > -1;
            #endif
        }

        int64_t write_res = -1;
        if (opened_file)
        {
            #if JSL_IS_WINDOWS
                write_res = _write(
                    file_descriptor,
                    contents.data,
                    (unsigned int) contents.length
                );
            #elif JSL_IS_POSIX
                write_res = write(
                    file_descriptor,
                    contents.data,
                    (size_t) contents.length
                );
            #endif
        }
        else
        {
            res = JSL_FILE_WRITE_COULD_NOT_OPEN;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        if (write_res > 0)
        {
            *out_bytes_written = write_res;
        }
        else
        {
            res = JSL_FILE_WRITE_COULD_NOT_WRITE;
            if (out_errno != NULL)
                *out_errno = errno;
        }

        int32_t close_res = -1;
        if (opened_file)
        {
            #if JSL_IS_WINDOWS
                close_res = _close(file_descriptor);
            #elif JSL_IS_POSIX
                close_res = close(file_descriptor);
            #endif

            if (close_res < 0)
            {
                res = JSL_FILE_WRITE_COULD_NOT_CLOSE;
                if (out_errno != NULL)
                    *out_errno = errno;
            }
        }

        return res;
    }

    struct JSL__FormatOutContext
    {
        FILE* out;
        bool failure_flag;
        uint8_t buffer[JSL_FORMAT_MIN_BUFFER];
    };

    static uint8_t* format_out_callback(uint8_t *buf, void *user, int64_t len)
    {
        struct JSL__FormatOutContext* context = (struct JSL__FormatOutContext*) user;

        int64_t written = (int64_t) fwrite(
            buf,
            sizeof(uint8_t),
            (size_t) len,
            context->out
        );
        if (written == len)
        {
            return context->buffer;
        }
        else
        {
            context->failure_flag = true;
            return NULL;
        }
    }

    JSL__ASAN_OFF bool jsl_format_file(FILE* out, JSLFatPtr fmt, ...)
    {
        if (out == NULL || fmt.data == NULL || fmt.length < 0)
            return false;

        va_list va;
        va_start(va, fmt);

        struct JSL__FormatOutContext context;
        context.out = out;
        context.failure_flag = false;

        jsl_format_callback(
            format_out_callback,
            &context,
            context.buffer,
            fmt,
            va
        );

        va_end(va);

        return !context.failure_flag;
    }

    #endif // JSL_INCLUDE_FILE_UTILS

#endif // JSL_IMPLEMENTATION

#endif // JACKS_STANDARD_LIBRARY

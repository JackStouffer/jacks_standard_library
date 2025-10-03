/**
 * # Jack's Standard Library
 * 
 * A collection of types and functions designed to replace usage of the
 * C Standard Library.
 *
 * ## Use
 * 
 * Include the header like normally in each source file:
 * 
 * ```c
 * #include "jacks_standard_library.h"
 * ```
 * 
 * Then, in ONE AND ONLY ONE file, do this:
 * 
 * ```c
 * #define JSL_IMPLEMENTATION
 * #include "jacks_standard_library.h"
 * ```
 * 
 * This should probably be in the "main" file for your program, but it doesn't have to be.
 * 
 * This library does not depend on the C standard library to be available at link
 * time. However, it does require the "compile time" headers `stddef.h`, `stdint.h`,
 * and `stdbool.h` (if using < C23). You'll also have to define the replacement functions
 * for the C standard library functions like `assert` and `memcmp`. See the
 * "Preprocessor Switches" section for more information.
 * 
 * ## Why
 * 
 * Much of the C Standard Library is outdated, very unsafe, or poorly designed. 
 * Some bad design decisions include
 * 
 *      * Null terminated strings
 *      * A single global heap, which is called silently, and you're expected to remember to call free
 *      * An object based file interface based around seeking with tiny reads and writes
 *      * Errors get special treatment
 * 
 * And unfortunately it was decided as part of the language that arrays decay to
 * pointers, and there's no way to stop it.
 * 
 * ## What's Included
 * 
 *      * A buffer/slice type called a fat pointer
 *          * used everywhere
 *          * standardizes that pointers should carry their length
 *          * vastly simplifies writing functions like file reading]
 *      * Common string and buffer utilities for fat pointers
 *          * things like fat pointer memcmp, substring search, etc.
 *      * An arena allocator
 *          * a.k.a a monotonic, region, or bump allocator
 *          * Easy to create, use, reset-able, allocators
 *          * Great for things with known lifetimes (which is 99% of the things you allocate)
 *      * A snprintf replacement
 *          * writes directly into a fat pointer buffer
 *          * Removes all compiler specific weirdness
 *      * Type safe containers
 *          * Unlike a lot of C containers, these are built with macros. Each container
 *            instance generates code with your value types rather than using void* plus
 *            length everywhere
 *          * Built with arenas in mind
 *          * dynamic array
 *          * hash map
 *          * hash set
 *      * Really common macros
 *          * min, max
 *          * bitflag checks
 * 
 * ## What's Not Included
 * 
 *      * There's no scanf alternative
 *      * Anything with UTF-16. Just use UTF-8
 *      * Threading. Just use pthreads or win api calls
 *      * Atomics. This is really platform specific and you should just use intrinsics
 *      * Date/time utilities
 *      * Random numbers
 * 
 * This library is slow for ARM as I haven't gotten around to writing the NEON
 * versions of the SIMD code yet. glibc will be significantly faster for comparable
 * operations.
 * 
 * ## What's Supported
 * 
 * Official support for Windows, macOS, and Linux with MSVC, GCC, and clang.
 * 
 * This might work on other POSIX systems, but I have not tested it.
 * 
 * ## Preprocessor Switches
 * 
 * `JSL_DEBUG` - turns on some debugging features, like overwriting stale memory with
 * `0xfeefee`.
 * 
 * `JSL_DEF` - allows you to override linkage/visibility (e.g., __declspec(dllexport)).
 * By default this is empty.
 * 
 * `JSL_WARN_UNUSED` - this controls the function attribute which tells the compiler to
 * issue a warning if the return value of the function is not stored in a variable, or if
 * that variable is never read. This is auto defined for clang and gcc, there's no
 * C11 compatible implementation for MSVC. If you want to turn this off, just define it as
 * empty string.
 * 
 * `JSL_ASSERT` - Assertion function definition. By default this will use `assert.h`.
 * If you wish to override it, it must be a function which takes three parameters, a int
 * conditional, a char* of the filename, and an int line number. You can also provide an
 * empty function if you just want to turn off asserts altogether.
 * 
 * `JSL_MEMCPY` - Controls memcpy calls in the library. By default this will include
 * `string.h` and use `memcpy`.
 * 
 * `JSL_MEMCPY` - Controls memcmp calls in the library. By default this will include
 * `string.h` and use `memcmp`.
 * 
 * `JSL_DEFAULT_ALLOCATION_ALIGNMENT` - Sets the alignment of allocations that aren't
 * explicitly set. Defaults to 16 bytes.
 * 
 * `JSL_INCLUDE_FILE_UTILS` - Include the file loading and writing utilities. These
 * require linking the standard library.
 * 
 * `JSL_INCLUDE_HASH_MAP` - Include the hash map macros. See the `HASHMAPS` section
 * for documentation.
 * 
 * ## Unicode
 * 
 * You should have a basic knowledge of Unicode semantics before using this library.
 * For example, this library provides length based comparison functions like
 * `jsl_fatptr_memory_compare`. This function provides byte by byte comparison; if
 * you know enough about Unicode, you should know why this is error prone for
 * determining two string's equality.
 * 
 * If you don't, you should learn the following terms:
 *
 *      * Code unit
 *      * Code point
 *      * Grapheme
 *      * How those three things are completely different from each other
 *      * Normalization
 * 
 * That would be the bare minimum needed to not shoot yourself in the foot.
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
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#ifndef JSL__LIKELY

    #if defined(__clang__) ||  defined(__GNUC__)
        #define JSL__LIKELY(x) __builtin_expect(!!(x), 1)
    #else
        #define JSL__LIKELY(x) (x)
    #endif

#endif

#ifndef JSL__UNLIKELY

    #if defined(__clang__) ||  defined(__GNUC__)
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

#ifndef JSL_DEFAULT_ALLOCATION_ALIGNMENT
    #define JSL_DEFAULT_ALLOCATION_ALIGNMENT 16
#endif

/**
 * 
 * 
 *                      PUBLIC API
 * 
 * 
 */


#ifndef JSL_DEF
    #define JSL_DEF /* extern by default */
#endif

#ifndef JSL_WARN_UNUSED

    #if defined(__clang__) ||  defined(__GNUC__)
        #define JSL_WARN_UNUSED __attribute__((warn_unused_result))
    #else
        #define JSL_WARN_UNUSED
    #endif

#endif


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

// TODO: Docs
#define JSL_UNSET_BITFLAG(flags, flag) *flags &= ~(flag)

// TODO: Docs
#define JSL_IS_BITFLAG_SET(flags, flag) ((flags & flag) == flag)

// TODO: Docs
#define JSL_IS_BITFLAG_NOT_SET(flags, flag) ((flags & flag) == 0)

// TODO: Docs
#define JSL_MAKE_BITFLAG(position) 1U << position

/**
 * Macro to simply mark a value as representing bytes. Does nothing with the value.
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
 * // be at the same place everytime you run your program in your debugger!
 * 
 * void* buffer = VirtualAlloc(JSL_TERABYTES(2), JSL_GIGABYTES(2), MEM_RESERVE, PAGE_READWRITE);
 * JSLArena arena;
 * jsl_arena_init(&arena, buffer, JSL_GIGABYTES(2));
 * ```
 */
#define JSL_TERABYTES(x) x * 1024 * 1024 * 1024 * 1024

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
 * Creates a JSLFatPtr from a string literal at compile time.
 *
 * @note The resulting fat pointer points directly to the string literal's memory,
 * so no copying occurs.
 *
 * Example:
 * 
 * ```c
 * // Create fat pointers from string literals
 * JSLFatPtr hello = JSL_FATPTR_LITERAL("Hello, World!");
 * JSLFatPtr path = JSL_FATPTR_LITERAL("/usr/local/bin");
 * JSLFatPtr empty = JSL_FATPTR_LITERAL("");
 * ```
 */
#define JSL_FATPTR_LITERAL(s) \
    ((JSLFatPtr){ .data = (uint8_t*)(s), .length = (int64_t)(sizeof("" s "") - 1) })

/**
 * TODO: docs
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
 *      uint8_t buffer[16 * 1024]; // yes this breaks the standard but it's supported everywhere that matters
 *      JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
 * 
 *      IntToStrMap map = int_to_str_ctor(&arena);
 *      int_to_str_add(&map, 64, JSL_FATPTR_LITERAL("This is my string data!"));
 * }
 * ```
 *
 * Fast, cheap, easy automatic memory management!
 */
#define JSL_ARENA_FROM_STACK(buf) \
    (JSLArena){ .start = (uint8_t *)(buf), \
                .current = (uint8_t *)(buf), \
                .end = (uint8_t *)(buf) + sizeof(buf) }

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

// TODO, docs: better explanation, example. 
/**
 * Utility function to get the total amount of bytes written to the original
 * fat pointer when compared to a writer fat pointer.
 * 
 * This function checks for NULL and checks that `writer_fatptr` points to data
 * in `original_fatptr`. If either of these checks fail, then `-1` is returned.
 */
JSL_DEF int64_t jsl_fatptr_total_write_length(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr);

/**
 * Returns the slice in `original_fatptr` that represents the written to portion, given
 * the size and pointer in `writer_fatptr`.
 * 
 * @note 
 * 
 * Example:
 * 
 * ```
 * JSLFatPtr original = jsl_arena_allocate(arena, 128 * 1024 * 1024);
 * JSLFatPtr writer = original;
 * jsl_fatptr_write_file_contents(&writer, "file_one.txt");
 * jsl_fatptr_write_file_contents(&writer, "file_two.txt");
 * JSLFatPtr portion_with_file_data = jsl_fatptr_auto_slice(original, writer);
 * ```
 */
JSL_DEF JSLFatPtr jsl_fatptr_auto_slice(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr);

/**
 * Build a fat pointer from a null terminated string. **DOES NOT** copy the data.
 * It simply sets the data pointer to `str` and the length value to the result of
 * `JSL_STRLEN`.
 *
 * @param str = the str to create the fat ptr from
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
 * is undefined, as it uses `JSL_STRLEN`.
 *
 * `destination` is modified to point to the remaining data in the buffer. I.E.
 * if the entire buffer was used then `destination->length` will be `0`.
 *
 * Returns:
 *      Number of bytes written or `-1` if `string` or the fat pointer was null.
 */
JSL_DEF int64_t jsl_fatptr_cstr_memory_copy(
    JSLFatPtr* destination,
    char* cstring,
    bool include_null_terminator
);

// TODO, docs. Remember to mention Unicode normalization. Mention difference between code units and graphemes in UTF
JSL_DEF int64_t jsl_fatptr_substring_search(JSLFatPtr string, JSLFatPtr substring);

// TODO, docs. Remember to mention Unicode normalization. Mention difference between code units and graphemes in UTF
JSL_DEF int64_t jsl_fatptr_index_of(JSLFatPtr str, uint8_t item);

// TODO, docs. Remember to mention Unicode normalization. Mention difference between code units and graphemes in UTF
JSL_DEF int64_t jsl_fatptr_count(JSLFatPtr str, uint8_t item);

// TODO, docs. Remember to mention Unicode normalization. Mention difference between code units and graphemes in UTF
JSL_DEF int64_t jsl_fatptr_index_of_reverse(JSLFatPtr str, uint8_t character);

// TODO, docs. Remember to mention Unicode normalization. Mention difference between code units and graphemes in UTF
JSL_DEF bool jsl_fatptr_starts_with(JSLFatPtr str, JSLFatPtr prefix);

// TODO, docs. Remember to mention Unicode normalization. Mention difference between code units and graphemes in UTF
JSL_DEF bool jsl_fatptr_ends_with(JSLFatPtr str, JSLFatPtr postfix);

// TODO, docs. Remember to mention Unicode normalization. Mention difference between code units and graphemes in UTF 
JSL_DEF JSLFatPtr jsl_fatptr_basename(JSLFatPtr filename);

// TODO, docs. Remember to mention Unicode normalization. Mention difference between code units and graphemes in UTF
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
 * two password hashes. This function is vulnreble to timing attacks since it bails out
 * at the first inequality.
 *
 * @returns true if equal, false otherwise. 
 */
JSL_DEF bool jsl_fatptr_memory_compare(JSLFatPtr a, JSLFatPtr b);

/**
 * Element by element comparison of the contents of a fat pointer and a null terminated
 * string. If the fat pointer has a null data value or a zero length, or if cstr is null,
 * then this function will return false.
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
 * @return The number of bytes that were successfully read from the string 
 */
JSL_DEF int32_t jsl_fatptr_to_int32(JSLFatPtr str, int32_t* result);

// TODO, docs
JSL_DEF void jsl_arena_init(JSLArena* arena, void* memory, int64_t length);
// TODO, docs
JSL_DEF void jsl_arena_init2(JSLArena* arena, JSLFatPtr memory);
// TODO, docs
JSL_DEF JSLFatPtr jsl_arena_allocate(JSLArena* arena, int64_t bytes, bool zeroed);
// TODO, docs
JSL_DEF JSLFatPtr jsl_arena_allocate_aligned(JSLArena* arena, int64_t bytes, int32_t alignment, bool zeroed);

// TODO, docs
#define JSL_ARENA_TYPED_ALLOCATE(T, arena) (T*) jsl_arena_allocate(arena, sizeof(T), false).data

/**
 * Resize the allocation if it was the last allocation, otherwise, allocate a new
 * chunk of memory.
 *
 * In debug mode, this function will set `original_allocation->data` to null to
 * help detect stale pointer bugs.
 */
JSL_DEF JSLFatPtr jsl_arena_reallocate(
    JSLArena* arena,
    JSLFatPtr original_allocation,
    int64_t new_num_bytes
);

/**
 * Resize the allocation if it was the last allocation, otherwise, allocate a new
 * chunk of memory.
 *
 * In debug mode, this function will set `original_allocation->data` to null to
 * help detect stale pointer bugs. 
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
 * The functions `jsl_arena_save_restore_point` and `jsl_arena_load_restore_point`
 * help you make temporary allocations inside an existing arena. You can think of
 * it as an "arena inside an arena"
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
 * See the docs for `jsl_arena_load_restore_point`.
 */
JSL_DEF void jsl_arena_load_restore_point(JSLArena* arena, uint8_t* restore_point);

/**
 * Allocate and copy the contents of a fat pointer with a null terminator.
 */
JSL_DEF char* jsl_arena_fatptr_to_cstr(JSLArena* arena, JSLFatPtr str);

/**
 * Allocate and copy the contents of a fat pointer with a null terminator.
 *
 * @note Use `jsl_fatptr_from_cstr` to make a fat pointer without copying.
 */
JSL_DEF JSLFatPtr jsl_arena_cstr_to_fatptr(JSLArena* arena, char* str);

/**
 * Allocate space for, and copy the contents of a fat pointer.
 *
 * @note Use `jsl_arena_cstr_to_fatptr` to copy a c string into a fatptr.
 */
JSL_DEF JSLFatPtr jsl_fatptr_duplicate(JSLArena* arena, JSLFatPtr str);

#ifndef JSL_FORMAT_MIN_BUFFER
    #define JSL_FORMAT_MIN_BUFFER 512 // how many characters per callback
#endif

typedef uint8_t* JSL_FORMAT_CALLBACK(uint8_t* buf, void *user, int64_t len);

/**
 * This is a full snprintf replacement for writing into a JSLFatPtr that
 * supports everything that the C runtime snprintf supports, including
 * float/double, 64-bit integers, hex floats, field parameters (%*.*d stuff),
 * length reads backs, etc.
 *
 * This returns the number of bytes written.
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
JSL_DEF JSLFatPtr jsl_fatptr_format(JSLArena* arena, char const *fmt, ...);

/**
 * TODO: docs
 * The fat pointer is taken by pointer and will be modified to point to
 * the remaining unwritten portion of the memory. I.E. if all the memory
 * was written to, then `buffer.length == 0`.
 */
JSL_DEF int64_t jsl_fatptr_format_buffer(
    JSLFatPtr* buffer,
    char const *fmt,
    ...
);

/**
 * TODO: docs
 * See docs for jsl_fatptr_format.
 */
JSL_DEF int64_t jsl_fatptr_format_valist(
    JSLFatPtr* buffer,
    char const *fmt,
    va_list va
);

/**
 * Convert into a buffer, calling back every JSL_FORMAT_MIN_BUFFER chars.
 * Your callback can then copy the chars out, print them or whatever.
 * This function is actually the workhorse for everything else.
 * The buffer you pass in must hold at least JSL_FORMAT_MIN_BUFFER characters.
 * 
 * You return the next buffer to use or 0 to stop converting
 */
JSL_DEF int64_t jsl_fatptr_format_callback(
   JSL_FORMAT_CALLBACK* callback,
   void* user,
   uint8_t* buf,
   char const* fmt,
   va_list va
);

/**
 * Set the comma and period characters to use for the current thread.
 */
JSL_DEF void jsl_fatptr_format_set_separators(char comma, char period);

#ifdef JSL_INCLUDE_FILE_UTILS

    typedef enum
    {
        // zero should always be an error condition!
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
        // zero should always be an error condition!
        JSL_FILE_WRITE_BAD_PARAMETERS,
        JSL_FILE_WRITE_SUCCESS,
        JSL_FILE_WRITE_COULD_NOT_OPEN,
        JSL_FILE_WRITE_COULD_NOT_WRITE,
        JSL_FILE_WRITE_COULD_NOT_CLOSE,

        JSL_FILE_WRITE_ENUM_COUNT
    } JSLWriteFileResultEnum;

    /**
     * Load the contents of the file at `path` into a newly allocated buffer
     * from the given arena. The buffer will be the exact size of the file contents.
     * 
     * If the arena does not have enough space, 
     */
    JSL_WARN_UNUSED JSL_DEF JSLLoadFileResultEnum jsl_fatptr_load_file_contents(
        JSLArena* arena,
        JSLFatPtr path,
        JSLFatPtr* out_contents,
        errno_t* out_errno
    );

    // TODO, docs
    JSL_WARN_UNUSED JSL_DEF JSLLoadFileResultEnum jsl_fatptr_load_file_contents_buffer(
        JSLFatPtr* buffer,
        JSLFatPtr path,
        errno_t* out_errno
    );

    // TODO, docs
    JSL_WARN_UNUSED JSL_DEF JSLWriteFileResultEnum jsl_fatptr_write_file_contents(
        JSLFatPtr contents,
        JSLFatPtr path,
        int64_t* bytes_written,
        errno_t* out_errno
    );

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

    #if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        #include <immintrin.h>
    #endif

    #ifndef JSL_ASSERT
        #include <assert.h>

        static void jsl__assert(int condition, char* file, int line)
        {
            (void) file;
            (void) line;
            assert(condition);
        }

        #define JSL_ASSERT(condition) jsl__assert(condition, __FILE__, __LINE__)
    #endif

    #ifndef JSL_MEMCPY
        #include <string.h>
        #define JSL_MEMCPY memcpy
    #endif

    #ifndef JSL_MEMCMP
        #include <string.h>
        #define JSL_MEMCMP memcmp
    #endif

    #ifndef JSL_STRLEN
        #include <string.h>
        #define JSL_STRLEN strlen
    #endif

    JSLFatPtr jsl_fatptr_init(uint8_t* ptr, int64_t length)
    {
        JSLFatPtr buffer = {
            .data = ptr,
            .length = length
        };
        return buffer;
    }

    JSLFatPtr jsl_fatptr_slice(JSLFatPtr buffer, int64_t start, int64_t end)
    {
        JSL_ASSERT(
            buffer.data != NULL
            && start > -1
            && start <= end
            && end <= buffer.length
        );

        buffer.data += start;
        buffer.length = end - start;
        return buffer;
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
        JSL_MEMCPY(destination->data, source.data, memcpy_length);

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
        JSL_MEMCPY(destination->data, cstring, length);

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

        return JSL_MEMCMP(a.data, b.data, a.length) == 0;
    }

    bool jsl_fatptr_cstr_compare(JSLFatPtr string, char* cstr)
    {
        if (cstr == NULL || string.data == NULL)
            return false;

        size_t cstr_length = JSL_STRLEN(cstr);

        if (string.length != cstr_length)
            return false;

        if ((void*) string.data == (void*) cstr)
            return true;

        return JSL_MEMCMP(string.data, cstr, cstr_length) == 0;
    }

    static inline void kmp_build_partial_match_table(JSLFatPtr substring, int64_t* partial_match_table)
    {
        int64_t length = 0; // Length of the previous longest prefix suffix
        int64_t i = 1;

        partial_match_table[0] = 0;

        while (i < substring.length)
        {
            if (substring.data[i] == substring.data[length])
            {
                ++length;
                partial_match_table[i] = length;
                ++i;
            }
            else
            {
                if (length != 0)
                {
                    length = partial_match_table[length - 1];
                }
                else
                {
                    partial_match_table[i] = 0;
                    ++i;
                }
            }
        }
    }

    // Knuth-Morris-Pratt substring search
    // Chose KMP over Boyer-Moore because we're only using this function in cases with
    // small strings and so the preprocessing time of BM isn't worth it
    static int64_t kmp_search(JSLFatPtr string, JSLFatPtr substring)
    {
        int64_t partial_match_table[substring.length]; // C VLA

        kmp_build_partial_match_table(substring, partial_match_table);

        int64_t i = 0; // index for string
        int64_t j = 0; // index for substring

        while (i < string.length)
        {
            if (substring.data[j] == string.data[i]) 
            {
                ++i;
                ++j;
            }

            if (j == substring.length)
            {
                return i - j;
            }
            else if (i < string.length && substring.data[j] != string.data[i])
            {
                if (j != 0)
                    j = partial_match_table[j - 1];
                else
                    i++;
            }
        }

        return -1;
    }

    int64_t jsl_fatptr_substring_search(JSLFatPtr string, JSLFatPtr substring)
    {
        if (
            string.data == NULL
            || string.length < 1
            || substring.data == NULL
            || substring.length < 1
            || substring.length > string.length
        )
        {
            return -1;
        }
        else if (string.length == substring.length)
        {
            if (JSL_MEMCMP(string.data, substring.data, string.length) == 0)
                return 0;
            else
                return -1;
        }
        else if (string.length < substring.length + 32)
        {
            return kmp_search(string, substring);
        }
        else
        {

            #ifdef __AVX2__
                // From http://0x80.pl/articles/simd-strfind.html

                __m256i first = _mm256_set1_epi8(substring.data[0]);
                __m256i last  = _mm256_set1_epi8(substring.data[substring.length - 1]);

                int64_t i = 0;
                int64_t length_remaining = string.length;
                int64_t stopping_point = substring.length + 32;

                // TODO, speed: Technically it's possible to read past the end of the end of the
                // "string" buffer safely if you first align the read to a 8-byte boundary.
                // However it seems like this is not possible to ensure if the function may inline
                // and since we're using LTO this would be a big pain to ensure.
                // Also we'd need to turn off address sanitizer for this function.

                while (length_remaining >= stopping_point)
                {
                    __m256i block_first = _mm256_loadu_si256((__m256i*) (string.data + i));
                    __m256i block_last  = _mm256_loadu_si256((__m256i*) (string.data + i + substring.length - 1));

                    __m256i eq_first = _mm256_cmpeq_epi8(first, block_first);
                    __m256i eq_last  = _mm256_cmpeq_epi8(last, block_last);

                    uint32_t mask = _mm256_movemask_epi8(_mm256_and_si256(eq_first, eq_last));

                    while (mask != 0)
                    {
                        int32_t bit_position = __builtin_ctz(mask);

                        if (JSL_MEMCMP(string.data + i + bit_position + 1, substring.data + 1, substring.length - 2) == 0)
                        {
                            return i + bit_position;
                        }

                        // clear the least significant bit set
                        mask &= (mask - 1);
                    }

                    i += 32;
                    length_remaining -= 32;
                }

                JSLFatPtr data_for_kmp = { .data = string.data + i, .length = string.length - i };
                int64_t kmp_res = kmp_search(data_for_kmp, substring);
                return kmp_res == -1 ? -1 : kmp_res + i;
            #else
                // NOTE: very slow, just here to get ARM to compile

                return kmp_search(string, substring);
            #endif
        }
    }

    int64_t jsl_fatptr_index_of(JSLFatPtr string, uint8_t item)
    {
        if (string.data == NULL || string.length < 1)
        {
            return -1;
        }
        else if (string.length < 32)
        {
            int64_t i = 0;

            while (i < string.length)
            {
                if (string.data[i] == item)
                    return i;

                ++i;
            }

            return -1;
        }
        else
        {
            int64_t i = 0;

            #ifdef __AVX2__
                __m256i needle = _mm256_set1_epi8(item);

                while (string.length - i >= 32)
                {
                    __m256i elements = _mm256_loadu_si256((__m256i*) (string.data + i));
                    __m256i eq_needle = _mm256_cmpeq_epi8(elements, needle);

                    uint32_t mask = _mm256_movemask_epi8(eq_needle);

                    if (mask != 0)
                    {
                        int32_t bit_position = __builtin_ctz(mask);
                        return i + bit_position;
                    }

                    i += 32;
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

        #ifdef __AVX2__
        while (str.length >= 32)
        {
            __m256i chunk = _mm256_loadu_si256((__m256i*) str.data);
            __m256i item_wide = _mm256_set1_epi8(item);
            __m256i cmp = _mm256_cmpeq_epi8(chunk, item_wide);
            int mask = _mm256_movemask_epi8(cmp);

            // Count the number of set bits (matches) in the mask
            count += __builtin_popcount(mask);

            str.data += 32;
            str.length -= 32;
        }
        #endif

        #ifdef __SSE3__
        while (str.length >= 16)
        {
            __m128i chunk = _mm_loadu_si128((__m128i*) str.data);
            __m128i item_wide = _mm_set1_epi8(item);
            __m128i cmp = _mm_cmpeq_epi8(chunk, item_wide);
            int mask = _mm_movemask_epi8(cmp);

            // Count the number of set bits (matches) in the mask
            count += __builtin_popcount(mask);

            str.data += 16;
            str.length -= 16;
        }
        #endif

        while (str.length > 0)
        {
            if (str.data[0] == item)
                count++;

            ++str.data;
            --str.length;
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
                        int32_t bit_position = __builtin_ctz(mask);
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

    char* jsl_arena_fatptr_to_cstr(JSLArena* arena, JSLFatPtr str)
    {
        if (arena == NULL || str.data == NULL || str.length < 1)
            return NULL;

        int64_t allocation_size = str.length + 1;
        JSLFatPtr allocation = jsl_arena_allocate(arena, allocation_size, false);

        if (allocation.data == NULL || allocation.length < allocation_size)
            return NULL;

        JSL_MEMCPY(allocation.data, str.data, str.length);
        allocation.data[str.length] = '\0';
        return (char*) allocation.data;
    }

    JSLFatPtr jsl_arena_cstr_to_fatptr(JSLArena* arena, char* str)
    {
        JSL_ASSERT(arena != NULL);
        JSL_ASSERT(str != NULL);

        JSLFatPtr ret = {0};
        if (arena == NULL || str == NULL)
            return ret;

        size_t length = JSL_STRLEN(str);
        if (length == 0)
            return ret;

        JSLFatPtr allocation = jsl_arena_allocate(arena, sizeof(uint8_t) * length, false);
        if (allocation.data == NULL || allocation.length < length)
            return ret;
        
        ret = allocation;
        JSL_MEMCPY(ret.data, str, length);
        return ret;
    }

    JSLFatPtr jsl_fatptr_duplicate(JSLArena* arena, JSLFatPtr str)
    {
        JSL_ASSERT(arena != NULL);
        JSL_ASSERT(str.data != NULL && str.length > -1);

        JSLFatPtr res = {0};
        if (arena == NULL || str.data == NULL || str.length < 1)
            return res;

        JSLFatPtr allocation = jsl_arena_allocate(arena, str.length, false);

        if (allocation.data == NULL || allocation.length < str.length)
            return res;

        JSL_MEMCPY(allocation.data, str.data, str.length);
        res = allocation;
        return res;
    }

    void jsl_fatptr_to_lowercase_ascii(JSLFatPtr str)
    {
        JSL_ASSERT(str.data != NULL);
        JSL_ASSERT(str.length > -1);

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

    #ifdef __AVX2__
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
    #endif

    bool jsl_fatptr_compare_ascii_insensitive(JSLFatPtr a, JSLFatPtr b)
    {
        if (JSL__UNLIKELY(a.data == NULL || b.data == NULL || a.length != b.length))
            return false;

        int64_t i = 0;

        #ifdef __AVX2__
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
        int64_t i = 0;

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

    static bool jsl__is_power_of_two(int32_t x)
    {
        return (x & (x-1)) == 0;
    }

    static inline uint8_t* align_ptr_upwards(uint8_t* ptr, int32_t align)
    {
        uintptr_t addr = (uintptr_t) ptr;
        addr = (addr + (align - 1)) & -align;
        return (uint8_t*) addr;
    }

    JSLFatPtr jsl_arena_allocate_aligned(JSLArena* arena, int64_t bytes, int32_t alignment, bool zeroed)
    {
        JSL_ASSERT(alignment > 0 && jsl__is_power_of_two(alignment));

        JSLFatPtr res = {0};
        uint8_t* aligned_current = align_ptr_upwards(arena->current, alignment);
        uint8_t* potential_end = aligned_current + bytes;

        if (potential_end <= arena->end)
        {
            res.data = aligned_current;
            res.length = bytes;

            #if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
                // Add 8 to leave "guard" zones between allocations
                arena->current = potential_end + 8;
                ASAN_UNPOISON_MEMORY_REGION(res.data, res.length);
            #else
                arena->current = potential_end;
            #endif

            if (zeroed)
                memset((void*) res.data, 0, res.length);
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
                memcpy(res.data, original_allocation.data, original_allocation.length);

                #ifdef JSL_DEBUG
                    memset((void*) original_allocation.data, 0xfeeefeee, original_allocation.length);
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
            memset((void*) arena->start, 0xfeeefeee, arena->current - arena->start);
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
            memset((void*) restore_point, 0xfeeefeee, arena->current - restore_point);
        #endif

        ASAN_POISON_MEMORY_REGION(restore_point, arena->current - restore_point);

        arena->current = restore_point;
    }

    static int32_t stbsp__real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits);
    static int32_t stbsp__real_to_parts(int64_t *bits, int32_t *expo, double value);
    #define STBSP__SPECIAL 0x7000

    #if defined(__GNUC__) || defined(__clang__)
        #if defined(__SANITIZE_ADDRESS__) && __SANITIZE_ADDRESS__
            #define JSL_ASAN_OFF __attribute__((__no_sanitize_address__))
        #endif
    #endif

    #ifndef JSL_ASAN_OFF
        #define JSL_ASAN_OFF
    #endif

    #if defined(__GNUC__) || defined(__clang__)
        #define JSL_NO_INLINE __attribute__((noinline))
    #endif

    #ifndef JSL_NO_INLINE
        #define JSL_NO_INLINE
    #endif

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

    JSL_ASAN_OFF void jsl_fatptr_format_set_separators(char pcomma, char pperiod)
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

    static JSL_ASAN_OFF uint32_t stbsp__strlen_limited(char const *string, uint32_t limit)
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
                    int null_pos = __builtin_ffs(mask) - 1;
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

    JSL_ASAN_OFF JSL_NO_INLINE int64_t jsl_fatptr_format_callback(
        JSL_FORMAT_CALLBACK* callback,
        void* user,
        uint8_t* buffer,
        char const* fmt,
        va_list va
    )
    {
        static char hex[] = "0123456789abcdefxp";
        static char hexu[] = "0123456789ABCDEFXP";
        uint8_t* buffer_cursor;
        char const* f;
        int32_t tlen = 0;

        buffer_cursor = buffer;
        f = fmt;

        #if defined(__AVX2__)
            __m256i percent_wide = _mm256_set1_epi8('%');
            __m256i zero_wide = _mm256_setzero_si256();
            __m256i vector_of_indexes = _mm256_set_epi8(
                31, 30, 29, 28, 27, 26, 25, 24,
                23, 22, 21, 20, 19, 18, 17, 16,
                15, 14, 13, 12, 11, 10, 9, 8,
                7, 6, 5, 4, 3, 2, 1, 0
            );
        #endif

        for (;;)
        {
            int32_t field_width, precision, trailing_zeros;
            uint32_t formatting_flags;

            // macros for the callback buffer stuff
            #define stbsp__chk_cb_bufL(bytes)                                                \
                {                                                                             \
                    int32_t len = (int32_t)(buffer_cursor - buffer);                           \
                    if ((len + (bytes)) >= JSL_FORMAT_MIN_BUFFER) {                            \
                    tlen += len;                                                            \
                    if (0 == (buffer_cursor = buffer = callback(buffer, user, len)))        \
                        goto done;                                                           \
                    }                                                                          \
                }

            #define stbsp__chk_cb_buf(bytes)                                  \
                {                                                              \
                    if (callback) {                                             \
                    stbsp__chk_cb_bufL(bytes);                               \
                    }                                                           \
                }

            #define stbsp__flush_cb()                                         \
                {                                                              \
                    stbsp__chk_cb_bufL(JSL_FORMAT_MIN_BUFFER - 1);              \
                } // flush if there is even one byte in the buffer

            #define stbsp__cb_buf_clamp(cl, v)                                                  \
                cl = v;                                                                         \
                if (callback) {                                                                 \
                    int32_t lg = JSL_FORMAT_MIN_BUFFER - (int32_t)(buffer_cursor - buffer);     \
                    if (cl > lg)                                                                \
                    cl = lg;                                                                    \
                }

            #if defined(__AVX2__)

                // Copy everything up to the next % or NULL
                for (;;)
                {
                    // SAFETY CONCERN:
                    // This is safe because when reading 32 bytes from a 32 byte aligned pointer
                    // it's impossible to read past the current page boundary into unmapped
                    // memory. While technically this is a buffer overflow, as we're reading
                    // memory that isn't "ours", I don't think it's possible that this is a
                    // security hole. I can't think of a way that info past the buffer could possibly leak
                    // out of this function. Also, if you have a string without a null terminator
                    // you would result in a page fault with or without this code. Additionally,
                    // this is a very common technique that's taken directly from glibc's strlen.


                    // Get up to 32-byte alignment so that we can safely read past the
                    // end of the given format string using an aligned read.
                    while (((uintptr_t)f) & 31)
                    {
                    if (f[0] == '%')
                        goto L_PROCESS_PERCENT;

                    if (f[0] == 0)
                        goto L_END_FORMAT;

                    stbsp__chk_cb_buf(1);
                    *buffer_cursor++ = f[0];
                    ++f;
                    }

                    const __m256i* source_wide = (const __m256i*) f;
                    __m256i* wide_dest = (__m256i*) buffer_cursor;

                    __m256i data = _mm256_load_si256(source_wide);

                    __m256i percent_mask = _mm256_cmpeq_epi8(data, percent_wide);
                    __m256i null_terminator_mask = _mm256_cmpeq_epi8(data, zero_wide);

                    int mask = _mm256_movemask_epi8(percent_mask) | _mm256_movemask_epi8(null_terminator_mask);

                    if (mask == 0)
                    {
                        // No special characters found, store entire block
                        _mm256_storeu_si256(wide_dest, data);
                        f += 32;
                        buffer_cursor += 32;
                    }
                    else
                    {
                        // Handle the characters up to the special character
                        int special_pos = __builtin_ffs(mask) - 1;
                        if (special_pos > 0)
                        {
                            // Create a byte-level mask for storing up to special_pos bytes
                            __m256i mask_for_partial_store = _mm256_cmpgt_epi8(
                                _mm256_set1_epi8(special_pos),
                                vector_of_indexes
                            );

                            // Use _mm256_blendv_epi8 to apply mask and store only up to special_pos
                            __m256i partial_data = _mm256_blendv_epi8(
                                zero_wide,
                                data,
                                mask_for_partial_store
                            );
                            _mm256_storeu_si256(wide_dest, partial_data);
                        }
                        f += special_pos;
                        buffer_cursor += special_pos;

                        if (f[0] == '%')
                            goto L_PROCESS_PERCENT;
                        if (f[0] == 0)
                            goto L_END_FORMAT;
                    }
                }

            #else
                
                // fast copy everything up to the next % (or end of string)
                for (;;)
                {
                    // get up to 4-byte alignment
                    while (((uintptr_t)f) & 3)
                    {
                    schk1:
                    if (f[0] == '%')
                        goto L_PROCESS_PERCENT;
                    schk2:
                    if (f[0] == 0)
                        goto L_END_FORMAT;
                    stbsp__chk_cb_buf(1);
                    *buffer_cursor++ = f[0];
                    ++f;
                    }

                    for (;;)
                    {
                    // Check if the next 4 bytes contain % or end of string.
                    // Using the 'hasless' trick:
                    // https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
                    uint32_t v, c;
                    v = *(uint32_t *)f;
                    c = (~v) & 0x80808080;

                    if (((v ^ 0x25252525) - 0x01010101) & c)
                        goto schk1;
                    if ((v - 0x01010101) & c)
                        goto schk2;

                    if (callback)
                        if ((JSL_FORMAT_MIN_BUFFER - (int32_t)(buffer_cursor - buffer)) < 4)
                            goto schk1;

                    if(((uintptr_t)buffer_cursor) & 3)
                    {
                        buffer_cursor[0] = f[0];
                        buffer_cursor[1] = f[1];
                        buffer_cursor[2] = f[2];
                        buffer_cursor[3] = f[3];
                    }
                    else
                    {
                        *(uint32_t *)buffer_cursor = v;
                    }
                    buffer_cursor += 4;
                    f += 4;
                    }
                }

            #endif

        L_PROCESS_PERCENT:

            ++f;

            // ok, we have a percent, read the modifiers first
            field_width = 0;
            precision = -1;
            formatting_flags = 0;
            trailing_zeros = 0;

            // flags
            for (;;) {
                switch (f[0]) {
                // if we have left justify
                case '-':
                    formatting_flags |= STBSP__LEFTJUST;
                    ++f;
                    continue;
                // if we have leading plus
                case '+':
                    formatting_flags |= STBSP__LEADINGPLUS;
                    ++f;
                    continue;
                // if we have leading space
                case ' ':
                    formatting_flags |= STBSP__LEADINGSPACE;
                    ++f;
                    continue;
                // if we have leading 0x
                case '#':
                    formatting_flags |= STBSP__LEADING_0X;
                    ++f;
                    continue;
                // if we have thousand commas
                case '\'':
                    formatting_flags |= STBSP__TRIPLET_COMMA;
                    ++f;
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
                    ++f;
                    continue;
                // if we don't want space between metric suffix and number
                case '_':
                    formatting_flags |= STBSP__METRIC_NOSPACE;
                    ++f;
                    continue;
                // if we have leading zero
                case '0':
                    formatting_flags |= STBSP__LEADINGZERO;
                    ++f;
                    goto flags_done;
                default: goto flags_done;
                }
            }
        flags_done:

            // get the field width
            if (f[0] == '*') {
                field_width = va_arg(va, uint32_t);
                ++f;
            } else {
                while ((f[0] >= '0') && (f[0] <= '9')) {
                    field_width = field_width * 10 + f[0] - '0';
                    f++;
                }
            }
            // get the precision
            if (f[0] == '.') {
                ++f;
                if (f[0] == '*') {
                    precision = va_arg(va, uint32_t);
                    ++f;
                } else {
                    precision = 0;
                    while ((f[0] >= '0') && (f[0] <= '9')) {
                    precision = precision * 10 + f[0] - '0';
                    f++;
                    }
                }
            }

            // handle integer size overrides
            switch (f[0]) {
            // are we halfwidth?
            case 'h':
                formatting_flags |= STBSP__HALFWIDTH;
                ++f;
                if (f[0] == 'h')
                    ++f;  // QUARTERWIDTH
                break;
            // are we 64-bit (unix style)
            case 'l':
                formatting_flags |= ((sizeof(long) == 8) ? STBSP__INTMAX : 0);
                ++f;
                if (f[0] == 'l') {
                    formatting_flags |= STBSP__INTMAX;
                    ++f;
                }
                break;
            // are we 64-bit on intmax? (c99)
            case 'j':
                formatting_flags |= (sizeof(size_t) == 8) ? STBSP__INTMAX : 0;
                ++f;
                break;
            // are we 64-bit on size_t or ptrdiff_t? (c99)
            case 'z':
                formatting_flags |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
                ++f;
                break;
            case 't':
                formatting_flags |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
                ++f;
                break;
            // are we 64-bit (msft style)
            case 'I':
                if ((f[1] == '6') && (f[2] == '4')) {
                    formatting_flags |= STBSP__INTMAX;
                    f += 3;
                } else if ((f[1] == '3') && (f[2] == '2')) {
                    f += 3;
                } else {
                    formatting_flags |= ((sizeof(void *) == 8) ? STBSP__INTMAX : 0);
                    ++f;
                }
                break;
            default: break;
            }

            // handle each replacement
            switch (f[0]) {
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
                // get the string
                string = va_arg(va, char *);
                if (string == 0)
                    string = (char *)"null";
                // get the length, limited to desired precision
                // always limit to ~0u chars since our counts are 32b
                l = (precision >= 0) ? stbsp__strlen_limited(string, precision) : JSL_STRLEN(string);
                lead[0] = 0;
                tail[0] = 0;
                precision = 0;
                decimal_precision = 0;
                comma_spacing = 0;
                // copy the string in
                goto L_STRING_COPY;

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
                h = (f[0] == 'A') ? hexu : hex;
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
                n = precision;
                if (n > 13)
                    n = 13;
                if (precision > (int32_t)n)
                    trailing_zeros = precision - n;
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
                l = (int32_t)(string - (num + 64));
                string = num + 64;
                comma_spacing = 1 + (3 << 24);
                goto L_STRING_COPY;

            case 'G': // float
            case 'g': // float
                h = (f[0] == 'G') ? hexu : hex;
                float_value = va_arg(va, double);
                if (precision == -1)
                    precision = 6;
                else if (precision == 0)
                    precision = 1; // default is 6
                // read the double into a string
                if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, (precision - 1) | 0x80000000))
                    formatting_flags |= STBSP__NEGATIVE;

                // clamp the precision and delete extra zeros after clamp
                n = precision;
                if (l > (uint32_t)precision)
                    l = precision;
                while ((l > 1) && (precision) && (source_ptr[l - 1] == '0')) {
                    --precision;
                    --l;
                }

                // should we use %e
                if ((decimal_precision <= -4) || (decimal_precision > (int32_t)n)) {
                    if (precision > (int32_t)l)
                    precision = l - 1;
                    else if (precision)
                    --precision; // when using %e, there is one digit before the decimal
                    goto L_DO_EXP_FROMG;
                }
                // this is the insane action to get the precision to match %g semantics for %f
                if (decimal_precision > 0) {
                    precision = (decimal_precision < (int32_t)l) ? l - decimal_precision : 0;
                } else {
                    precision = -decimal_precision + ((precision > (int32_t)l) ? (int32_t) l : precision);
                }
                goto L_DO_FLOAT_FROMG;

            case 'E': // float
            case 'e': // float
                h = (f[0] == 'E') ? hexu : hex;
                float_value = va_arg(va, double);
                if (precision == -1)
                    precision = 6; // default is 6
                // read the double into a string
                if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, precision | 0x80000000))
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
                if ((l - 1) > (uint32_t)precision)
                    l = precision + 1;
                for (n = 1; n < l; n++)
                    *string++ = source_ptr[n];
                // trailing zeros
                trailing_zeros = precision - (l - 1);
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
                comma_spacing = 1 + (3 << 24); // how many tens
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
                if (stbsp__real_to_str(&source_ptr, &l, num, &decimal_precision, float_value, precision))
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
                    n = -decimal_precision;
                    if ((int32_t)n > precision)
                    n = precision;

                    memset(string, '0', n);
                    string += n;

                    if ((int32_t)(l + n) > precision)
                    l = precision - n;

                    memcpy(string, source_ptr, l);
                    string += l;
                    source_ptr += l;
                    
                    trailing_zeros = precision - (n + l);
                    comma_spacing = 1 + (3 << 24); // how many tens did we write (for commas below)
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
                        n = decimal_precision - n;
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
                    comma_spacing = (int32_t)(string - (num + 64)) + (3 << 24); // comma_spacing is how many tens
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
                    comma_spacing = (int32_t)(string - (num + 64)) + (3 << 24); // comma_spacing is how many tens
                    if (precision)
                        *string++ = stbsp__period;
                    if ((l - decimal_precision) > (uint32_t)precision)
                        l = precision + decimal_precision;
                    while (n < l) {
                        *string++ = source_ptr[n];
                        ++n;
                    }
                    trailing_zeros = precision - (l - decimal_precision);
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
                h = (f[0] == 'B') ? hexu : hex;
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
                formatting_flags &= ~STBSP__LEADINGZERO; // 'p' only prints the pointer with zeros
                                            // fall through - to X

            case 'X': // upper hex
            case 'x': // lower hex
                h = (f[0] == 'X') ? hexu : hex;
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
                        l &= ~15;
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
                    if ((f[0] != 'u') && (i64 < 0)) {
                    n64 = (uint64_t)-i64;
                    formatting_flags |= STBSP__NEGATIVE;
                    }
                } else {
                    int32_t i = va_arg(va, int32_t);
                    n64 = (uint32_t)i;
                    if ((f[0] != 'u') && (i < 0)) {
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
                comma_spacing = l + (3 << 24);
                if (precision < 0)
                    precision = 0;

            L_STRING_COPY:
                // get field_width=leading/trailing space, precision=leading zeros
                if (precision < (int32_t)l)
                    precision = l;
                n = precision + lead[0] + tail[0] + trailing_zeros;
                if (field_width < (int32_t)n)
                    field_width = n;
                field_width -= n;
                precision -= l;

                // handle right justify and leading zeros
                if ((formatting_flags & STBSP__LEFTJUST) == 0) {
                    if (formatting_flags & STBSP__LEADINGZERO) // if leading zeros, everything is in precision
                    {
                    precision = (field_width > precision) ? field_width : precision;
                    field_width = 0;
                    } else {
                    formatting_flags &= ~STBSP__TRIPLET_COMMA; // if no leading zeros, then no commas
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
                        memset(buffer_cursor, ' ', i);
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
                    memcpy(buffer_cursor, source_ptr, i);
                    buffer_cursor += i;
                    source_ptr += i;

                    stbsp__chk_cb_buf(1);
                    }

                    // copy leading zeros
                    c = comma_spacing >> 24;
                    comma_spacing &= 0xffffff;
                    comma_spacing = (formatting_flags & STBSP__TRIPLET_COMMA) ? ((uint32_t)(c - ((precision + comma_spacing) % (c + 1)))) : 0;

                    while (precision > 0)
                    {
                    stbsp__cb_buf_clamp(i, precision);
                    precision -= i;
                    
                    if (JSL_IS_BITFLAG_NOT_SET(formatting_flags, STBSP__TRIPLET_COMMA))
                    {
                        memset(buffer_cursor, '0', i);
                        buffer_cursor += i;
                    }
                    else
                    {
                        while (i)
                        {
                            if (comma_spacing == c)
                            {
                                comma_spacing = 0;
                                *buffer_cursor = stbsp__comma;
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
                    memcpy(buffer_cursor, source_ptr, i);
                    buffer_cursor += i;
                    source_ptr += i;

                    stbsp__chk_cb_buf(1);
                }

                // copy the string
                n = l;
                while (n)
                {
                    int32_t i;
                    stbsp__cb_buf_clamp(i, n);

                    n -= i;
                    memcpy(buffer_cursor, string, i);
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
                    memset(buffer_cursor, '0', i);
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
                    memcpy(buffer_cursor, source_ptr, i);
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
                        memset(buffer_cursor, ' ', i);
                        buffer_cursor += i;

                        stbsp__chk_cb_buf(1);
                    }
                    }
                }

                break;

            default: // unknown, just copy code
                string = num + STBSP__NUMSZ - 1;
                *string = f[0];
                l = 1;
                field_width = formatting_flags = 0;
                lead[0] = 0;
                tail[0] = 0;
                precision = 0;
                decimal_precision = 0;
                comma_spacing = 0;
                goto L_STRING_COPY;
            }
            ++f;
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
                uint8_t* s;
                uint8_t* se;
                uint8_t* d = context->buffer.data;
                s = buf;
                se = buf + len;
                do
                {
                    *d++ = *s++;
                } while (s < se);
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

    JSL_ASAN_OFF int64_t jsl_fatptr_format_valist(JSLFatPtr* buffer, char const* fmt, va_list va )
    {
        stbsp__context context;

        if ((buffer->length == 0) && buffer->data == NULL)
        {
            context.length = 0;

            jsl_fatptr_format_callback(
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
            context.length = 0;

            jsl_fatptr_format_callback(
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

    struct JSLArenaContext
    {
        JSLArena* arena;
        JSLFatPtr current_allocation;
        uint8_t* cursor;
        uint8_t buffer[JSL_FORMAT_MIN_BUFFER];
    };

    static uint8_t* format_arena_callback(uint8_t *buf, void *user, int64_t len)
    {
        struct JSLArenaContext* context = (struct JSLArenaContext*) user;

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

        memcpy(context->cursor, buf, len);
        context->cursor += len;

        return context->buffer;
    }

    JSL_ASAN_OFF JSLFatPtr jsl_fatptr_format(JSLArena* arena, char const *fmt, ...)
    {
        va_list va;
        va_start(va, fmt);

        struct JSLArenaContext context;
        context.arena = arena;
        context.current_allocation.data = NULL;
        context.current_allocation.length = 0;
        context.cursor = NULL;

        jsl_fatptr_format_callback(
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

    JSL_ASAN_OFF int64_t jsl_fatptr_format_buffer(JSLFatPtr* buffer, char const *fmt, ...)
    {
        int64_t result;
        va_list va;
        va_start(va, fmt);

        result = jsl_fatptr_format_valist(buffer, fmt, va);
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

    *bits = b & ((((uint64_t)1) << 52) - 1);
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
            *start = (bits & ((((uint64_t)1) << 52) - 1)) ? "NaN" : "Inf";
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
        frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1) : (tens + frac_digits);
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
                e = dg - frac_digits;
                if ((uint32_t)e >= 24)
                    goto L_NO_ROUND;
                r = stbsp__powten[e];
                bits = bits + (r / 2);
                if ((uint64_t)bits >= stbsp__powten[dg])
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
        *len = e;
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

    #if defined(_WIN32)
    
        #define JSL_WIN32

        #include <errno.h>
        #include <fcntl.h>
        #include <limits.h>
        #include <fcntl.h>
        #include <io.h>
        #include <sys\stat.h>

    #elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

        #define JSL_POSIX

        #include <errno.h>
        #include <limits.h>
        #include <fcntl.h>
        #include <unistd.h>
        #include <sys/types.h>
        #include <sys/stat.h>

    #endif


    static int64_t jsl__get_file_size_from_fileno(int32_t file_descriptor)
    {
        int64_t result_size = -1;
        bool stat_success = false;

        #if defined(JSL_WIN32)
            struct _stat64 file_info;
            int stat_result = _fstat64(file_descriptor, &file_info);
            if (stat_result == 0)
            {
                stat_success = true;
                result_size = file_info.st_size;
            }

        #elif defined(JSL_POSIX)
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

    JSLLoadFileResultEnum jsl_fatptr_load_file_contents(
        JSLArena* arena,
        JSLFatPtr path,
        JSLFatPtr* out_contents,
        errno_t* out_errno
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

            JSL_MEMCPY(path_buffer, path.data, path.length);
            path_buffer[path.length] = '\0';
            got_path = true;
        }

        int32_t file_descriptor = -1;
        bool opened_file = false;
        if (got_path)
        {
            #if defined(JSL_WIN32)
                file_descriptor = _open(path_buffer, 0);
            #elif defined(JSL_POSIX)
                file_descriptor = open(path_buffer, 0);
            #else
                #error "Unsupported platform"
            #endif
            
            opened_file = file_descriptor > -1;
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
            #if defined(JSL_WIN32)
                read_res = _read(file_descriptor, allocation.data, file_size);
            #elif defined(JSL_POSIX)
                read_res = read(file_descriptor, allocation.data, file_size);
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
            #if defined(JSL_WIN32)
                int32_t close_res = _close(file_descriptor);
            #elif defined(JSL_POSIX)
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


    JSLLoadFileResultEnum jsl_fatptr_load_file_contents_buffer(
        JSLFatPtr* buffer,
        JSLFatPtr path,
        errno_t* out_errno
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

            JSL_MEMCPY(path_buffer, path.data, path.length);
            path_buffer[path.length] = '\0';
            got_path = true;
        }

        int32_t file_descriptor = -1;
        bool opened_file = false;
        if (got_path)
        {
            #if defined(JSL_WIN32)
                file_descriptor = _open(path_buffer, 0);
            #elif defined(JSL_POSIX)
                file_descriptor = open(path_buffer, 0);
            #else
                #error "Unsupported platform"
            #endif
            
            opened_file = file_descriptor > -1;
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

            #if defined(JSL_WIN32)
                read_res = _read(file_descriptor, buffer->data, read_size);
            #elif defined(JSL_POSIX)
                read_res = read(file_descriptor, buffer->data, read_size);
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
            #if defined(JSL_WIN32)
                int32_t close_res = _close(file_descriptor);
            #elif defined(JSL_POSIX)
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

    JSLWriteFileResultEnum jsl_fatptr_write_file_contents(
        JSLFatPtr contents,
        JSLFatPtr path,
        int64_t* out_bytes_written,
        errno_t* out_errno
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
            JSL_MEMCPY(path_buffer, path.data, path.length);
            path_buffer[path.length] = '\0';
            got_path = true;
        }

        int32_t file_descriptor = -1;
        bool opened_file = false;
        if (got_path)
        {
            #if defined(JSL_WIN32)
                int32_t file_descriptor = _open(path_buffer, _O_CREAT, _S_IREAD | _S_IWRITE);
            #elif defined(JSL_POSIX)
                int32_t file_descriptor = open(path_buffer, O_CREAT, S_IRUSR | S_IWUSR);
            #endif
    
            opened_file = file_descriptor > -1;
        }

        int64_t write_res = -1;
        bool wrote_file = false;
        if (opened_file)
        {
            #if defined(JSL_WIN32)
                int64_t write_res = _write(
                    file_descriptor,
                    contents.data,
                    (size_t) contents.length
                );
            #elif defined(JSL_POSIX)
                int64_t write_res = write(
                    file_descriptor,
                    contents.data,
                    (size_t) contents.length
                );
            #endif

            wrote_file = write_res > 0;
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
            close_res = close(file_descriptor);

            if (close_res < 0)
            {
                res = JSL_FILE_WRITE_COULD_NOT_CLOSE;
                if (out_errno != NULL)
                    *out_errno = errno;
            }
        }

        return res;
    }

    #endif // JSL_INCLUDE_FILE_UTILS

#endif // JSL_IMPLEMENTATION

#endif // JACKS_STANDARD_LIBRARY

# API Documentation

## Macros

- [`JSL_SWITCH_FALLTHROUGH`](#macro-jsl_switch_fallthrough)
- [`JSL_ASSERT`](#macro-jsl_assert)
- [`JSL_MEMCPY`](#macro-jsl_memcpy)
- [`JSL_MEMCMP`](#macro-jsl_memcmp)
- [`JSL_MEMSET`](#macro-jsl_memset)
- [`JSL_MEMMOVE`](#macro-jsl_memmove)
- [`JSL_STRLEN`](#macro-jsl_strlen)
- [`JSL_IS_WINDOWS`](#macro-jsl_is_windows)
- [`JSL_IS_POSIX`](#macro-jsl_is_posix)
- [`JSL_IS_WEB_ASSEMBLY`](#macro-jsl_is_web_assembly)
- [`JSL_IS_X86`](#macro-jsl_is_x86)
- [`JSL_IS_ARM`](#macro-jsl_is_arm)
- [`JSL_IS_GCC`](#macro-jsl_is_gcc)
- [`JSL_IS_CLANG`](#macro-jsl_is_clang)
- [`JSL_IS_MSVC`](#macro-jsl_is_msvc)
- [`JSL_IS_POINTER_32_BITS`](#macro-jsl_is_pointer_32_bits)
- [`JSL_IS_POINTER_64_BITS`](#macro-jsl_is_pointer_64_bits)
- [`JSL_WARN_UNUSED`](#macro-jsl_warn_unused)
- [`JSL_DEF`](#macro-jsl_def)
- [`JSL_DEFAULT_ALLOCATION_ALIGNMENT`](#macro-jsl_default_allocation_alignment)
- [`JSL_PLATFORM_COUNT_TRAILING_ZEROS`](#macro-jsl_platform_count_trailing_zeros)
- [`JSL_PLATFORM_COUNT_TRAILING_ZEROS64`](#macro-jsl_platform_count_trailing_zeros64)
- [`JSL_PLATFORM_COUNT_LEADING_ZEROS`](#macro-jsl_platform_count_leading_zeros)
- [`JSL_PLATFORM_COUNT_LEADING_ZEROS64`](#macro-jsl_platform_count_leading_zeros64)
- [`JSL_PLATFORM_POPULATION_COUNT`](#macro-jsl_platform_population_count)
- [`JSL_PLATFORM_POPULATION_COUNT64`](#macro-jsl_platform_population_count64)
- [`JSL_PLATFORM_FIND_FIRST_SET`](#macro-jsl_platform_find_first_set)
- [`JSL_PLATFORM_FIND_FIRST_SET64`](#macro-jsl_platform_find_first_set64)
- [`JSL_MAX`](#macro-jsl_max)
- [`JSL_MIN`](#macro-jsl_min)
- [`JSL_BETWEEN`](#macro-jsl_between)
- [`JSL_SET_BITFLAG`](#macro-jsl_set_bitflag)
- [`JSL_UNSET_BITFLAG`](#macro-jsl_unset_bitflag)
- [`JSL_IS_BITFLAG_SET`](#macro-jsl_is_bitflag_set)
- [`JSL_IS_BITFLAG_NOT_SET`](#macro-jsl_is_bitflag_not_set)
- [`JSL_MAKE_BITFLAG`](#macro-jsl_make_bitflag)
- [`JSL_BYTES`](#macro-jsl_bytes)
- [`JSL_KILOBYTES`](#macro-jsl_kilobytes)
- [`JSL_MEGABYTES`](#macro-jsl_megabytes)
- [`JSL_GIGABYTES`](#macro-jsl_gigabytes)
- [`JSL_TERABYTES`](#macro-jsl_terabytes)
- [`JSL_DEBUG_DONT_OPTIMIZE_AWAY`](#macro-jsl_debug_dont_optimize_away)
- [`JSL_FATPTR_INITIALIZER`](#macro-jsl_fatptr_initializer)
- [`JSL_FATPTR_EXPRESSION`](#macro-jsl_fatptr_expression)
- [`JSL_FATPTR_ADVANCE`](#macro-jsl_fatptr_advance)
- [`JSL_FATPTR_FROM_STACK`](#macro-jsl_fatptr_from_stack)

## Types

- [`JSLAllocatorInterface`](#type-jslallocatorinterface)
- [`JSLStringLifeTime`](#type-jslstringlifetime)
- [`JSLFatPtr`](#type-jslfatptr)
- [`int64_t`](#type-typedef-int64_t)
- [`JSLOutputSink`](#type-jsloutputsink)

## Functions

- [`jsl_is_power_of_two`](#function-jsl_is_power_of_two)
- [`jsl_next_power_of_two_i32`](#function-jsl_next_power_of_two_i32)
- [`jsl_next_power_of_two_u32`](#function-jsl_next_power_of_two_u32)
- [`jsl_next_power_of_two_i64`](#function-jsl_next_power_of_two_i64)
- [`jsl_next_power_of_two_u64`](#function-jsl_next_power_of_two_u64)
- [`jsl_previous_power_of_two_u32`](#function-jsl_previous_power_of_two_u32)
- [`jsl_previous_power_of_two_u64`](#function-jsl_previous_power_of_two_u64)
- [`jsl_round_up_i32`](#function-jsl_round_up_i32)
- [`jsl_round_up_u32`](#function-jsl_round_up_u32)
- [`jsl_round_up_i64`](#function-jsl_round_up_i64)
- [`jsl_round_up_u64`](#function-jsl_round_up_u64)
- [`jsl_round_up_pow2_i64`](#function-jsl_round_up_pow2_i64)
- [`jsl_round_up_pow2_u64`](#function-jsl_round_up_pow2_u64)
- [`jsl_fatptr_init`](#function-jsl_fatptr_init)
- [`jsl_fatptr_slice`](#function-jsl_fatptr_slice)
- [`jsl_fatptr_slice_to_end`](#function-jsl_fatptr_slice_to_end)
- [`jsl_fatptr_total_write_length`](#function-jsl_fatptr_total_write_length)
- [`jsl_fatptr_auto_slice`](#function-jsl_fatptr_auto_slice)
- [`jsl_fatptr_from_cstr`](#function-jsl_fatptr_from_cstr)
- [`jsl_fatptr_memory_copy`](#function-jsl_fatptr_memory_copy)
- [`jsl_fatptr_cstr_memory_copy`](#function-jsl_fatptr_cstr_memory_copy)
- [`jsl_fatptr_substring_search`](#function-jsl_fatptr_substring_search)
- [`jsl_fatptr_index_of`](#function-jsl_fatptr_index_of)
- [`jsl_fatptr_count`](#function-jsl_fatptr_count)
- [`jsl_fatptr_index_of_reverse`](#function-jsl_fatptr_index_of_reverse)
- [`jsl_fatptr_starts_with`](#function-jsl_fatptr_starts_with)
- [`jsl_fatptr_ends_with`](#function-jsl_fatptr_ends_with)
- [`jsl_fatptr_basename`](#function-jsl_fatptr_basename)
- [`jsl_fatptr_get_file_extension`](#function-jsl_fatptr_get_file_extension)
- [`jsl_fatptr_memory_compare`](#function-jsl_fatptr_memory_compare)
- [`jsl_fatptr_cstr_compare`](#function-jsl_fatptr_cstr_compare)
- [`jsl_fatptr_compare_ascii_insensitive`](#function-jsl_fatptr_compare_ascii_insensitive)
- [`jsl_fatptr_to_lowercase_ascii`](#function-jsl_fatptr_to_lowercase_ascii)
- [`jsl_fatptr_to_int32`](#function-jsl_fatptr_to_int32)
- [`jsl_fatptr_strip_whitespace_left`](#function-jsl_fatptr_strip_whitespace_left)
- [`jsl_fatptr_strip_whitespace_right`](#function-jsl_fatptr_strip_whitespace_right)
- [`jsl_fatptr_strip_whitespace`](#function-jsl_fatptr_strip_whitespace)
- [`jsl_fatptr_to_cstr`](#function-jsl_fatptr_to_cstr)
- [`jsl_cstr_to_fatptr`](#function-jsl_cstr_to_fatptr)
- [`jsl_fatptr_duplicate`](#function-jsl_fatptr_duplicate)
- [`jsl_output_sink_write_fatptr`](#function-jsl_output_sink_write_fatptr)
- [`jsl_output_sink_write_i8`](#function-jsl_output_sink_write_i8)
- [`jsl_output_sink_write_u8`](#function-jsl_output_sink_write_u8)
- [`jsl_output_sink_write_bool`](#function-jsl_output_sink_write_bool)
- [`jsl_output_sink_write_i16`](#function-jsl_output_sink_write_i16)
- [`jsl_output_sink_write_u16`](#function-jsl_output_sink_write_u16)
- [`jsl_output_sink_write_i32`](#function-jsl_output_sink_write_i32)
- [`jsl_output_sink_write_u32`](#function-jsl_output_sink_write_u32)
- [`jsl_output_sink_write_i64`](#function-jsl_output_sink_write_i64)
- [`jsl_output_sink_write_u64`](#function-jsl_output_sink_write_u64)
- [`jsl_output_sink_write_f32`](#function-jsl_output_sink_write_f32)
- [`jsl_output_sink_write_f64`](#function-jsl_output_sink_write_f64)
- [`jsl_output_sink_write_cstr`](#function-jsl_output_sink_write_cstr)
- [`jsl_fatptr_output_sink`](#function-jsl_fatptr_output_sink)
- [`jsl_format`](#function-jsl_format)
- [`jsl_format_sink_valist`](#function-jsl_format_sink_valist)
- [`jsl_format_sink`](#function-jsl_format_sink)
- [`jsl_format_set_separators`](#function-jsl_format_set_separators)

## File: /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h

## Jack's Standard Library

A collection of utilities which are designed to replace much of the C standard
library.

See README.md for a detailed intro.

See DESIGN.md for background on the design decisions.

See DOCUMENTATION.md for a single markdown file containing all of the docstrings
from this file. It's more nicely formatted and contains hyperlinks.

The convention of this library is that all symbols prefixed with either `jsl__`
or `JSL__` (with two underscores) are meant to be private to this library. They
are not a stable part of the API.

### External Preprocessor Definitions

`JSL_DEBUG` - turns on some debugging features, like overwriting stale memory with
`0xfeefee`.

### License

Copyright (c) 2026 Jack Stouffer

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the “Software”),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

<a id="macro-jsl_switch_fallthrough"></a>
### Macro: `JSL_SWITCH_FALLTHROUGH`

```c
#define JSL_SWITCH_FALLTHROUGH JSL_SWITCH_FALLTHROUGH __attribute__ ( ( fallthrough))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:505`

---

<a id="macro-jsl_assert"></a>
### Macro: `JSL_ASSERT`

Assertion macro definition. By default this will use `assert.h`.
If you wish to override it, it must be a function which takes three parameters, a int
conditional, a char* of the filename, and an int line number. You can also provide an
empty function if you just want to turn off asserts altogether; this is not
recommended. The small speed boost you get is from avoiding a branch is generally not
worth the loss of memory protection.

Define this as a macro before importing the library to override this.

```c
#define JSL_ASSERT JSL_ASSERT ( condition) assert ( condition)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:532`

---

<a id="macro-jsl_memcpy"></a>
### Macro: `JSL_MEMCPY`

Controls memcpy calls in the library. By default this will include
`string.h` and be an alias to C's `memcpy`.

Define this as a macro before importing the library to override this.
Your macro must follow the libc `memcpy` signature of

```
void your_memcpy(void*, const void*, size_t);
```

```c
#define JSL_MEMCPY JSL_MEMCPY memcpy
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:549`

---

<a id="macro-jsl_memcmp"></a>
### Macro: `JSL_MEMCMP`

Controls memcmp calls in the library. By default this will include
`string.h` and be an alias to C's `memcmp`.

Define this as a macro before importing the library to override this.
Your macro must follow the libc `memcmp` signature of

```
int your_memcmp(const void*, const void*, size_t);
```

```c
#define JSL_MEMCMP JSL_MEMCMP memcmp
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:566`

---

<a id="macro-jsl_memset"></a>
### Macro: `JSL_MEMSET`

Controls memset calls in the library. By default this will include
`string.h` and be an alias to C's `memset`.

Define this as a macro before importing the library to override this.
Your macro must follow the libc `memset` signature of

```
void your_memset(void*, int, size_t);
```

```c
#define JSL_MEMSET JSL_MEMSET memset
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:583`

---

<a id="macro-jsl_memmove"></a>
### Macro: `JSL_MEMMOVE`

Controls memmove calls in the library. By default this will include
`string.h` and be an alias to C's `memmove`.

Define this as a macro before importing the library to override this.
Your macro must follow the libc `memcpy` signature of

```
void your_memmove(void*, const void*, size_t);
```

```c
#define JSL_MEMMOVE JSL_MEMMOVE memmove
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:600`

---

<a id="macro-jsl_strlen"></a>
### Macro: `JSL_STRLEN`

Controls strlen calls in the library. By default this will include
`string.h` and be an alias to C's `strlen`.

Define this as a macro before importing the library to override this.
Your macro must follow the libc `strlen` signature of

```
size_t your_strlen(const char*);
```

```c
#define JSL_STRLEN JSL_STRLEN strlen
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:617`

---

<a id="macro-jsl_is_windows"></a>
### Macro: `JSL_IS_WINDOWS`

If the target platform is Windows OS, then 1, else 0.

```c
#define JSL_IS_WINDOWS JSL_IS_WINDOWS JSL__IS_WINDOWS_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:629`

---

<a id="macro-jsl_is_posix"></a>
### Macro: `JSL_IS_POSIX`

If the target platform is a POSIX, then 1, else 0.

```c
#define JSL_IS_POSIX JSL_IS_POSIX JSL__IS_POSIX_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:634`

---

<a id="macro-jsl_is_web_assembly"></a>
### Macro: `JSL_IS_WEB_ASSEMBLY`

If the target platform is in WebAssembly, then 1, else 0.

```c
#define JSL_IS_WEB_ASSEMBLY JSL_IS_WEB_ASSEMBLY JSL__IS_WEB_ASSEMBLY_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:639`

---

<a id="macro-jsl_is_x86"></a>
### Macro: `JSL_IS_X86`

If the target platform is in x86, then 1, else 0.

```c
#define JSL_IS_X86 JSL_IS_X86 JSL__IS_X86_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:644`

---

<a id="macro-jsl_is_arm"></a>
### Macro: `JSL_IS_ARM`

If the target platform is in ARM, then 1, else 0.

```c
#define JSL_IS_ARM JSL_IS_ARM JSL__IS_ARM_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:649`

---

<a id="macro-jsl_is_gcc"></a>
### Macro: `JSL_IS_GCC`

If the host compiler is GCC, then 1, else 0.

```c
#define JSL_IS_GCC JSL_IS_GCC JSL__IS_GCC_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:654`

---

<a id="macro-jsl_is_clang"></a>
### Macro: `JSL_IS_CLANG`

If the host compiler is clang, then 1, else 0.

```c
#define JSL_IS_CLANG JSL_IS_CLANG JSL__IS_CLANG_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:659`

---

<a id="macro-jsl_is_msvc"></a>
### Macro: `JSL_IS_MSVC`

If the host compiler is MSVC, then 1, else 0.

```c
#define JSL_IS_MSVC JSL_IS_MSVC JSL__IS_MSVC_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:664`

---

<a id="macro-jsl_is_pointer_32_bits"></a>
### Macro: `JSL_IS_POINTER_32_BITS`

If the target executable uses 32 bit pointers, then 1, else 0.

```c
#define JSL_IS_POINTER_32_BITS JSL_IS_POINTER_32_BITS JSL__IS_POINTER_32_BITS_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:669`

---

<a id="macro-jsl_is_pointer_64_bits"></a>
### Macro: `JSL_IS_POINTER_64_BITS`

If the target executable uses 64 bit pointers, then 1, else 0.

```c
#define JSL_IS_POINTER_64_BITS JSL_IS_POINTER_64_BITS JSL__IS_POINTER_64_BITS_VAL
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:674`

---

<a id="macro-jsl_warn_unused"></a>
### Macro: `JSL_WARN_UNUSED`

This controls the function attribute which tells the compiler to
issue a warning if the return value of the function is not stored in a variable, or if
that variable is never read. This is auto defined for clang and gcc; there's no
C11 compatible implementation for MSVC. If you want to turn this off, just define it as
empty string.

```c
#define JSL_WARN_UNUSED JSL_WARN_UNUSED __attribute__ ( ( warn_unused_result))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:686`

---

<a id="macro-jsl_def"></a>
### Macro: `JSL_DEF`

Allows you to override linkage/visibility (e.g., __declspec) for all of
the functions defined by this library. By default this is empty, so extern.

Define this as a macro before importing the library to override this.

```c
#define JSL_DEF JSL_DEF
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:700`

---

<a id="macro-jsl_default_allocation_alignment"></a>
### Macro: `JSL_DEFAULT_ALLOCATION_ALIGNMENT`

Sets the alignment of allocations that aren't explicitly set. Defaults to 8 bytes.

Define this as a macro before importing the library to override this.

```c
#define JSL_DEFAULT_ALLOCATION_ALIGNMENT JSL_DEFAULT_ALLOCATION_ALIGNMENT 8
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:709`

---

<a id="macro-jsl_platform_count_trailing_zeros"></a>
### Macro: `JSL_PLATFORM_COUNT_TRAILING_ZEROS`

Platform specific intrinsic for returning the count of trailing zeros for 32
bit signed and unsigned integers.

In order to be as fast as possible, this does not represent a cross
platform abstraction over different ctz implementations.
On GCC and clang, this is replaced with `__builtin_ctz`. On MSVC
`_BitScanForward` is used in a forced inline function call. Behavior
with zero is undefined for GCC and clang while MSVC will give 32.

```c
#define JSL_PLATFORM_COUNT_TRAILING_ZEROS JSL_PLATFORM_COUNT_TRAILING_ZEROS ( x) JSL__COUNT_TRAILING_ZEROS_IMPL ( ( x))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:722`

---

<a id="macro-jsl_platform_count_trailing_zeros64"></a>
### Macro: `JSL_PLATFORM_COUNT_TRAILING_ZEROS64`

Platform specific intrinsic for returning the count of trailing zeros for 64
bit signed and unsigned integers.

In order to be as fast as possible, this does not represent a cross
platform abstraction over different ctz implementations.
On GCC and clang, this is replaced with `__builtin_ctzll`. On MSVC
`_BitScanForward64` is used in a forced inline function call. Behavior
with zero is undefined for GCC and clang while MSVC will give 32.

```c
#define JSL_PLATFORM_COUNT_TRAILING_ZEROS64 JSL_PLATFORM_COUNT_TRAILING_ZEROS64 ( x) JSL__COUNT_TRAILING_ZEROS_IMPL64 ( ( x))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:734`

---

<a id="macro-jsl_platform_count_leading_zeros"></a>
### Macro: `JSL_PLATFORM_COUNT_LEADING_ZEROS`

Platform specific intrinsic for returning the count of leading zeros for 32
bit signed and unsigned integers.

In order to be as fast as possible, this does not represent a cross
platform abstraction over different clz implementations.
On GCC and clang, this is replaced with `__builtin_clz`. On MSVC
`_BitScanReverse` is used in a forced inline function call. Behavior
with zero is undefined for GCC and clang while MSVC will give 32.

```c
#define JSL_PLATFORM_COUNT_LEADING_ZEROS JSL_PLATFORM_COUNT_LEADING_ZEROS ( x) JSL__COUNT_LEADING_ZEROS_IMPL ( ( x))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:746`

---

<a id="macro-jsl_platform_count_leading_zeros64"></a>
### Macro: `JSL_PLATFORM_COUNT_LEADING_ZEROS64`

Platform specific intrinsic for returning the count of leading zeros for 64
bit signed and unsigned integers.

In order to be as fast as possible, this does not represent a cross
platform abstraction over different clz implementations.
On GCC and clang, this is replaced with `__builtin_clzll`. On MSVC
`_BitScanReverse64` is used in a forced inline function call. Behavior
with zero is undefined for GCC and clang while MSVC will give 32.

```c
#define JSL_PLATFORM_COUNT_LEADING_ZEROS64 JSL_PLATFORM_COUNT_LEADING_ZEROS64 ( x) JSL__COUNT_LEADING_ZEROS_IMPL64 ( ( x))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:758`

---

<a id="macro-jsl_platform_population_count"></a>
### Macro: `JSL_PLATFORM_POPULATION_COUNT`

Platform specific intrinsic for returning the count of set bits for 32
bit signed and unsigned integers.

In order to be as fast as possible, this does not represent a cross
platform abstraction over different popcnt implementations.
On GCC and clang, this is replaced with `__builtin_popcount`. On MSVC
`__popcnt` is used.

```c
#define JSL_PLATFORM_POPULATION_COUNT JSL_PLATFORM_POPULATION_COUNT ( x) JSL__POPULATION_COUNT_IMPL ( ( x))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:769`

---

<a id="macro-jsl_platform_population_count64"></a>
### Macro: `JSL_PLATFORM_POPULATION_COUNT64`

Platform specific intrinsic for returning the count of set bits for 64
bit signed and unsigned integers.

In order to be as fast as possible, this does not represent a cross
platform abstraction over different popcnt implementations.
On GCC and clang, this is replaced with `__builtin_popcountll`. On MSVC
`__popcnt64` is used.

```c
#define JSL_PLATFORM_POPULATION_COUNT64 JSL_PLATFORM_POPULATION_COUNT64 ( x) JSL__POPULATION_COUNT_IMPL64 ( ( x))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:780`

---

<a id="macro-jsl_platform_find_first_set"></a>
### Macro: `JSL_PLATFORM_FIND_FIRST_SET`

Platform specific intrinsic for returning the index of the first set
bit for 32 bit signed and unsigned integers.

In order to be as fast as possible, this does not represent a cross
platform abstraction over different ffs implementations.
On GCC and clang, this is replaced with `__builtin_ffs`. On MSVC
`_BitScanForward` is used in a forced inline function call. Behavior
with zero is undefined for GCC and clang while MSVC will give 32.

```c
#define JSL_PLATFORM_FIND_FIRST_SET JSL_PLATFORM_FIND_FIRST_SET ( x) JSL__FIND_FIRST_SET_IMPL ( ( x))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:792`

---

<a id="macro-jsl_platform_find_first_set64"></a>
### Macro: `JSL_PLATFORM_FIND_FIRST_SET64`

Platform specific intrinsic for returning the index of the first set
bit for 64 bit signed and unsigned integers.

In order to be as fast as possible, this does not represent a cross
platform abstraction over different ffs implementations.
On GCC and clang, this is replaced with `__builtin_ffsll`. On MSVC
`_BitScanForward64` is used in a forced inline function call. Behavior
with zero is undefined for GCC and clang while MSVC will give 32.

```c
#define JSL_PLATFORM_FIND_FIRST_SET64 JSL_PLATFORM_FIND_FIRST_SET64 ( x) JSL__FIND_FIRST_SET_IMPL64 ( ( x))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:804`

---

<a id="macro-jsl_max"></a>
### Macro: `JSL_MAX`

Evaluates the maximum of two values.

#### Warning

This macro evaluates its arguments multiple times. Do not use with
arguments that have side effects (e.g., function calls, increment/decrement
operations) as they will be executed more than once.

Example:
```c
int max_val = JSL_MAX(10, 20);        // Returns 20
double max_d = JSL_MAX(3.14, 2.71);   // Returns 3.14

// DANGER: Don't do this - increment happens twice!
// int bad = JSL_MAX(++x, y);
```

```c
#define JSL_MAX JSL_MAX ( a, b) ( ( a)> ( b) ? ( a) : ( b))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:822`

---

<a id="macro-jsl_min"></a>
### Macro: `JSL_MIN`

Evaluates the minimum of two values.

#### Warning

This macro evaluates its arguments multiple times. Do not use with
arguments that have side effects (e.g., function calls, increment/decrement
operations) as they will be executed more than once.

Example:
```c
int max_val = JSL_MAX(10, 20);        // Returns 20
double max_d = JSL_MAX(3.14, 2.71);   // Returns 3.14

// DANGER: Don't do this - increment happens twice!
// int bad = JSL_MAX(++x, y);
```

```c
#define JSL_MIN JSL_MIN ( a, b) ( ( a) < ( b) ? ( a) : ( b))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:840`

---

<a id="macro-jsl_between"></a>
### Macro: `JSL_BETWEEN`

Returns `x` bound between the two given values.

#### Warning

This macro evaluates its arguments multiple times. Do not use with
arguments that have side effects (e.g., function calls, increment/decrement
operations) as they will be executed more than once.

Example:
```c
int max_val = JSL_BETWEEN(10, 15, 20);        // Returns 15
double max_d = JSL_BETWEEN(1.2, 0.1, 3.14);   // Returns 1.2

// DANGER: Don't do this - increment happens more than once!
// int bad = JSL_BETWEEN(32, ++x, 64);
```

```c
#define JSL_BETWEEN JSL_BETWEEN ( min, x, max) ( ( x) < ( min) ? ( min) : ( ( x)> ( max) ? ( max) : ( x)))
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:858`

---

<a id="macro-jsl_set_bitflag"></a>
### Macro: `JSL_SET_BITFLAG`

Sets a specific bit flag in a bitfield by performing a bitwise OR operation.

#### Parameters

**flags** — Pointer to the bitfield variable where the flag should be set

**flag** — The flag value(s) to set. Can be a single flag or multiple flags OR'd together



#### Note

This macro evaluates its arguments multiple times. Do not use with
arguments that have side effects (e.g., function calls, increment/decrement
operations) as they will be executed more than once.

Example:

```
#define FLAG_READ    JSL_MAKE_BITFLAG(1)
#define FLAG_WRITE   JSL_MAKE_BITFLAG(2)

uint32_t permissions = 0;

JSL_SET_BITFLAG(&permissions, FLAG_READ);
JSL_SET_BITFLAG(&permissions, FLAG_WRITE);

// DANGER: Don't do this - increment happens twice!
// JSL_SET_BITFLAG(&array[++index], some_flag);
```

```c
#define JSL_SET_BITFLAG JSL_SET_BITFLAG ( flags, flag) * flags |= flag
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:885`

---

<a id="macro-jsl_unset_bitflag"></a>
### Macro: `JSL_UNSET_BITFLAG`

Clears a bit flag from a value pointed to by `flags` by zeroing the bits set in `flag`.

Example:

```
uint32_t permissions = FLAG_READ | FLAG_WRITE;
JSL_UNSET_BITFLAG(&permissions, FLAG_WRITE);
// `permissions` now only has FLAG_READ set.
```

```c
#define JSL_UNSET_BITFLAG JSL_UNSET_BITFLAG ( flags, flag) * flags &= ~ ( flag)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:898`

---

<a id="macro-jsl_is_bitflag_set"></a>
### Macro: `JSL_IS_BITFLAG_SET`

Returns non-zero when every bit in `flag` is also set within `flags`.

Example:

```
uint32_t permissions = FLAG_READ | FLAG_WRITE;
if (JSL_IS_BITFLAG_SET(permissions, FLAG_READ)) {
// FLAG_READ is present
}
```

```c
#define JSL_IS_BITFLAG_SET JSL_IS_BITFLAG_SET ( flags, flag) ( ( flags & flag) == flag)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:912`

---

<a id="macro-jsl_is_bitflag_not_set"></a>
### Macro: `JSL_IS_BITFLAG_NOT_SET`

Returns non-zero when none of the bits in `flag` are set within `flags`.

Example:

```
uint32_t permissions = FLAG_READ;
if (JSL_IS_BITFLAG_NOT_SET(permissions, FLAG_WRITE)) {
// FLAG_WRITE is not present
}
```

```c
#define JSL_IS_BITFLAG_NOT_SET JSL_IS_BITFLAG_NOT_SET ( flags, flag) ( ( flags & flag) == 0)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:926`

---

<a id="macro-jsl_make_bitflag"></a>
### Macro: `JSL_MAKE_BITFLAG`

Generates a bit flag with a single bit set at the given zero-based position.

Example:

```
enum PermissionFlags {
FLAG_READ = JSL_MAKE_BITFLAG(0),
FLAG_WRITE = JSL_MAKE_BITFLAG(1),
};
uint32_t permissions = FLAG_READ | FLAG_WRITE;
```

```c
#define JSL_MAKE_BITFLAG JSL_MAKE_BITFLAG ( position) 1U << position
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:941`

---

<a id="macro-jsl_bytes"></a>
### Macro: `JSL_BYTES`

Macro to simply mark a value as representing a size in bytes. Does nothing with the value.

```c
#define JSL_BYTES JSL_BYTES ( x) x
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:946`

---

<a id="macro-jsl_kilobytes"></a>
### Macro: `JSL_KILOBYTES`

Converts a numeric value to megabytes by multiplying by `1024`.

This macro is useful for specifying memory sizes in a more readable format,
particularly when allocating.

Example:
```c
uint8_t buffer[JSL_KILOBYTES(16)];
JSLArena arena;
jsl_arena_init(&arena, buffer, JSL_KILOBYTES(16));
```

```c
#define JSL_KILOBYTES JSL_KILOBYTES ( x) x * 1024
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:961`

---

<a id="macro-jsl_megabytes"></a>
### Macro: `JSL_MEGABYTES`

Converts a numeric value to megabytes by multiplying by `1024 ^ 2`.

This macro is useful for specifying memory sizes in a more readable format,
particularly when allocating.

Example:
```c
uint8_t buffer[JSL_MEGABYTES(16)];
JSLArena arena;
jsl_arena_init(&arena, buffer, JSL_MEGABYTES(16));
```

```c
#define JSL_MEGABYTES JSL_MEGABYTES ( x) x * 1024 * 1024
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:976`

---

<a id="macro-jsl_gigabytes"></a>
### Macro: `JSL_GIGABYTES`

Converts a numeric value to gigabytes by multiplying by `1024 ^ 3`.

This macro is useful for specifying memory sizes in a more readable format,
particularly when allocating.

Example:
```c
void* buffer = malloc(JSL_GIGABYTES(2));
JSLArena arena;
jsl_arena_init(&arena, buffer, JSL_GIGABYTES(2));
```

```c
#define JSL_GIGABYTES JSL_GIGABYTES ( x) ( ( int64_t) x * 1024L * 1024L * 1024L)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:992`

---

<a id="macro-jsl_terabytes"></a>
### Macro: `JSL_TERABYTES`

Converts a numeric value to terabytes by multiplying by `1024 ^ 4`.

This macro is useful for specifying memory sizes in a more readable format,
particularly when allocating.

Example:
```c
// Reserve two gigabytes of virtual address space starting at 2 terabytes.
// If you're using static offsets this means that your objects in memory will
// be at the same place every time you run your program in your debugger!

void* buffer = VirtualAlloc(JSL_TERABYTES(2), JSL_GIGABYTES(2), MEM_RESERVE, PAGE_READWRITE);
JSLArena arena;
jsl_arena_init(&arena, buffer, JSL_GIGABYTES(2));
```

```c
#define JSL_TERABYTES JSL_TERABYTES ( x) ( ( int64_t) x * 1024L * 1024L * 1024L * 1024L)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1011`

---

<a id="macro-jsl_debug_dont_optimize_away"></a>
### Macro: `JSL_DEBUG_DONT_OPTIMIZE_AWAY`

TODO: docs

```c
#define JSL_DEBUG_DONT_OPTIMIZE_AWAY JSL_DEBUG_DONT_OPTIMIZE_AWAY ( x) JSL__DONT_OPTIMIZE_AWAY_IMPL ( x)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1026`

---

<a id="macro-jsl_fatptr_initializer"></a>
### Macro: `JSL_FATPTR_INITIALIZER`

Creates a [JSLFatPtr](#type-jslfatptr) from a string literal at compile time. The resulting fat pointer
points directly to the string literal's memory, so no copying occurs.

#### Warning

With MSVC this will only work during variable initialization as MSVC
still does not support compound literals.

Example:

```c
// Create fat pointers from string literals
JSLFatPtr hello = JSL_FATPTR_INITIALIZER("Hello, World!");
JSLFatPtr path = JSL_FATPTR_INITIALIZER("/usr/local/bin");
JSLFatPtr empty = JSL_FATPTR_INITIALIZER("");
```

```c
#define JSL_FATPTR_INITIALIZER JSL_FATPTR_INITIALIZER ( s) { ( uint8_t *) ( s), ( int64_t) ( sizeof ( s) - 1) }
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1253`

---

<a id="macro-jsl_fatptr_expression"></a>
### Macro: `JSL_FATPTR_EXPRESSION`

Creates a [JSLFatPtr](#type-jslfatptr) from a string literal for usage as an rvalue. The resulting fat pointer
points directly to the string literal's memory, so no copying occurs.

#### Warning

On MSVC this will be a function call as MSVC does not support compound literals.
On all other compilers this will be a zero cost compound literal

Example:

```c
void my_function(JSLFatPtr data);

my_function(JSL_FATPTR_EXPRESSION("my data"));
```

```c
#define JSL_FATPTR_EXPRESSION JSL_FATPTR_EXPRESSION ( s) ( ( JSLFatPtr) { ( uint8_t *) ( s), ( int64_t) ( sizeof ( s) - 1) })
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1291`

---

<a id="macro-jsl_fatptr_advance"></a>
### Macro: `JSL_FATPTR_ADVANCE`

Advances a fat pointer forward by `n`. This macro does not bounds check
and is intentionally tiny so it can live in hot loops without adding overhead.
Only use this in cases where you've already checked the length.

```c
#define JSL_FATPTR_ADVANCE JSL_FATPTR_ADVANCE ( fatptr, n) do { fatptr . data += n; fatptr . length -= n; \ } while ( 0)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1301`

---

<a id="macro-jsl_fatptr_from_stack"></a>
### Macro: `JSL_FATPTR_FROM_STACK`

Creates a `JSLFatPtr` view over a stack-allocated buffer.

The buffer must be a real array (not a pointer) so the macro can use
`sizeof` to determine its capacity at compile time.

Example:

```c
uint8_t buffer[JSL_KILOBYTES(4)];
JSLFatPtr ptr = JSL_FATPTR_FROM_STACK(buffer);
```

#### Warning

This macro only works for variable initializers and cannot be used as a
normal rvalue.

```c
#define JSL_FATPTR_FROM_STACK JSL_FATPTR_FROM_STACK ( buf) { ( uint8_t *) ( buf), ( int64_t) ( sizeof ( buf)) }
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1322`

---

<a id="type-jslallocatorinterface"></a>
### : `JSLAllocatorInterface`



*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:72`

---

<a id="type-jslstringlifetime"></a>
### : `JSLStringLifeTime`

TODO: docs

- `JSL_STRING_LIFETIME_TRANSIENT = 0`
- `JSL_STRING_LIFETIME_STATIC = 1`


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1016`

---

<a id="function-jsl_is_power_of_two"></a>
### Function: `jsl_is_power_of_two`

TODO: docs

```c
int jsl_is_power_of_two(int x);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1031`

---

<a id="function-jsl_next_power_of_two_i32"></a>
### Function: `jsl_next_power_of_two_i32`

Round x up to the next power of two. If x is a power of two it returns
the same value.

This function is designed to be used in tight loops and other performance
critical areas. Therefore, both zero, one, and values greater than 2^31 not special
cased. The return values for these cases are compiler, OS, and ISA specific.
If you need consistent behavior, then you can easily call this function like
so:

```
jsl_next_power_of_two_i32(
JSL_BETWEEN(2, x, 0x8000000u)
);
```

#### Parameters

**x** — The value to round up



#### Returns

the next power of two

```c
int jsl_next_power_of_two_i32(int x);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1052`

---

<a id="function-jsl_next_power_of_two_u32"></a>
### Function: `jsl_next_power_of_two_u32`

Round x up to the next power of two. If x is a power of two it returns
the same value.

This function is designed to be used in tight loops and other performance
critical areas. Therefore, both zero, one, and values greater than 2^31 not special
cased. The return values for these cases are compiler, OS, and ISA specific.
If you need consistent behavior, then you can easily call this function like
so:

```
jsl_next_power_of_two_u32(
JSL_BETWEEN(2u, x, 0x80000000u)
);
```

#### Parameters

**x** — The value to round up



#### Returns

the next power of two

```c
int jsl_next_power_of_two_u32(int x);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1073`

---

<a id="function-jsl_next_power_of_two_i64"></a>
### Function: `jsl_next_power_of_two_i64`

```c
int jsl_next_power_of_two_i64(int x);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1076`

---

<a id="function-jsl_next_power_of_two_u64"></a>
### Function: `jsl_next_power_of_two_u64`

Round x up to the next power of two. If x is a power of two it returns
the same value.

This function is designed to be used in tight loops and other performance
critical areas. Therefore, both zero and values greater than 2^63 not special
cased. The return values for these cases are compiler, OS, and ISA specific.
If you need consistent behavior, then you can easily call this function like
so:

```
jsl_next_power_of_two_u64(
JSL_BETWEEN(1ull, x, 0x8000000000000000ull)
);
```

#### Parameters

**x** — The value to round up



#### Returns

the next power of two

```c
int jsl_next_power_of_two_u64(int x);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1097`

---

<a id="function-jsl_previous_power_of_two_u32"></a>
### Function: `jsl_previous_power_of_two_u32`

Round x down to the previous power of two. If x is a power of two it returns
the same value.

This function is designed to be used in tight loops and other performance
critical areas. Therefore, zero is not special cased. The return value is
compiler, OS, and ISA specific. If you need consistent behavior, then you
can easily call this function like so:

```
jsl_previous_power_of_two_u32(
JSL_MAX(1, x)
);
```

#### Parameters

**x** — The value to round up



#### Returns

the next power of two

```c
int jsl_previous_power_of_two_u32(int x);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1117`

---

<a id="function-jsl_previous_power_of_two_u64"></a>
### Function: `jsl_previous_power_of_two_u64`

Round x down to the previous power of two. If x is a power of two it returns
the same value.

This function is designed to be used in tight loops and other performance
critical areas. Therefore, zero is not special cased. The return value is
compiler, OS, and ISA specific. If you need consistent behavior, then you
can easily call this function like so:

```
jsl_previous_power_of_two_u64(
JSL_MAX(1UL, x)
);
```

#### Parameters

**x** — The value to round up



#### Returns

the next power of two

```c
int jsl_previous_power_of_two_u64(int x);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1137`

---

<a id="function-jsl_round_up_i32"></a>
### Function: `jsl_round_up_i32`

Round num up to the nearest multiple of multiple_of.

#### Parameters

**num** — The value to round

**multiple_of** — A power-of-two multiple to round to



#### Returns

num rounded up to the given multiple

```c
int jsl_round_up_i32(int num, int multiple_of);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1146`

---

<a id="function-jsl_round_up_u32"></a>
### Function: `jsl_round_up_u32`

Round num up to the nearest multiple of multiple_of.

#### Parameters

**num** — The value to round

**multiple_of** — A power-of-two multiple to round to



#### Returns

num rounded up to the given multiple

```c
int jsl_round_up_u32(int num, int multiple_of);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1155`

---

<a id="function-jsl_round_up_i64"></a>
### Function: `jsl_round_up_i64`

Round num up to the nearest multiple of multiple_of.

#### Parameters

**num** — The value to round

**multiple_of** — A power-of-two multiple to round to



#### Returns

num rounded up to the given multiple

```c
int jsl_round_up_i64(int num, int multiple_of);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1164`

---

<a id="function-jsl_round_up_u64"></a>
### Function: `jsl_round_up_u64`

Round num up to the nearest multiple of multiple_of.

#### Parameters

**num** — The value to round

**multiple_of** — A power-of-two multiple to round to



#### Returns

num rounded up to the given multiple

```c
int jsl_round_up_u64(int num, int multiple_of);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1173`

---

<a id="function-jsl_round_up_pow2_i64"></a>
### Function: `jsl_round_up_pow2_i64`

Round num up to the nearest multiple of multiple_of assuming multiple_of is
a power of two.

This function is optimized for speed and does not validate its arguments.
Passing a non-power-of-two, zero, negative value, or values that would
overflow will result in undefined behavior.

#### Parameters

**num** — The value to round

**multiple_of** — A power-of-two multiple to round to



#### Returns

num rounded up to the given multiple

```c
int jsl_round_up_pow2_i64(int num, int pow2);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1187`

---

<a id="function-jsl_round_up_pow2_u64"></a>
### Function: `jsl_round_up_pow2_u64`

Round num up to the nearest multiple of multiple_of assuming multiple_of is
a power of two.

This function is optimized for speed and does not validate its arguments.
Passing a non-power-of-two, zero, negative value, or values that would
overflow will result in undefined behavior.

#### Parameters

**num** — The value to round

**multiple_of** — A power-of-two multiple to round to



#### Returns

num rounded up to the given multiple

```c
int jsl_round_up_pow2_u64(int num, int pow2);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1201`

---

<a id="type-jslfatptr"></a>
### : `JSLFatPtr`

A fat pointer is a representation of a chunk of memory. It **is not** a container
or an abstract data type.

A fat pointer is very similar to D or Go's slices. This provides several useful
functions like bounds checked reads/writes.

One very important thing to note is that the fat pointer is always defined as mutable.
In my opinion, const in C provides very little protection and a world a headaches during
refactors, especially since C does not have generics or function overloading. I find the
cost benefit analysis to be in the negative.

- `int * data;`
- `int length;`


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1215`

---

<a id="function-jsl_fatptr_init"></a>
### Function: `jsl_fatptr_init`

Constructor utility function to make a fat pointer out of a pointer and a length.
Useful in cases where you can't use C's struct init syntax, like as a parameter
to a function.

```c
JSLFatPtr jsl_fatptr_init(int *ptr, int length);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1329`

---

<a id="function-jsl_fatptr_slice"></a>
### Function: `jsl_fatptr_slice`

Create a new fat pointer that points to the given parameter's data but
with a view of [start, end).

This function is bounds checked. Out of bounds slices will assert.

```c
JSLFatPtr jsl_fatptr_slice(JSLFatPtr fatptr, int start, int end);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1337`

---

<a id="function-jsl_fatptr_slice_to_end"></a>
### Function: `jsl_fatptr_slice_to_end`

Create a new fat pointer that points to the given parameter's data but
with a view of [start, length).

This function is bounds checked. Out of bounds slices will assert.

```c
JSLFatPtr jsl_fatptr_slice_to_end(JSLFatPtr fatptr, int start);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1345`

---

<a id="function-jsl_fatptr_total_write_length"></a>
### Function: `jsl_fatptr_total_write_length`

Utility function to get the total amount of bytes written to the original
fat pointer when compared to a writer fat pointer. See [jsl_fatptr_auto_slice](#function-jsl_fatptr_auto_slice)
to get a slice of the written portion.

This function asserts on the following conditions

* Either fatptr has a null data field
* Either fatptr has a negative length
* The writer_fatptr does not point to the memory of original_fatptr

```
JSLFatPtr original = jsl_arena_allocate(arena, 128 * 1024 * 1024);
JSLFatPtr writer = original;
jsl_write_file_contents(&writer, "file_one.txt");
jsl_write_file_contents(&writer, "file_two.txt");
int64_t write_len = jsl_fatptr_total_write_length(original, writer);
```

#### Parameters

**original_fatptr** — The pointer to the originally allocated buffer

**writer_fatptr** — The pointer that has been advanced during writing operations



#### Returns

The amount of data which has been written, or -1 if there was an issue

```c
int jsl_fatptr_total_write_length(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1370`

---

<a id="function-jsl_fatptr_auto_slice"></a>
### Function: `jsl_fatptr_auto_slice`

Returns the slice in `original_fatptr` that represents the written to portion, given
the size and pointer in `writer_fatptr`.

This function asserts on the following conditions

* Either fatptr has a null data field
* Either fatptr has a negative length
* The writer_fatptr does not point to the memory of original_fatptr

Example:

```
JSLFatPtr original = jsl_arena_allocate(arena, 128 * 1024 * 1024);
JSLFatPtr writer = original;
jsl_write_file_contents(&writer, "file_one.txt");
jsl_write_file_contents(&writer, "file_two.txt");
JSLFatPtr portion_with_file_data = jsl_fatptr_auto_slice(original, writer);
```

#### Parameters

**original_fatptr** — The pointer to the originally allocated buffer

**writer_fatptr** — The pointer that has been advanced during writing operations



#### Returns

A new fat pointer pointing to the written portion of the original buffer.
It will be `NULL` if there was an issue.

```c
JSLFatPtr jsl_fatptr_auto_slice(JSLFatPtr original_fatptr, JSLFatPtr writer_fatptr);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1397`

---

<a id="function-jsl_fatptr_from_cstr"></a>
### Function: `jsl_fatptr_from_cstr`

Build a fat pointer from a null terminated string. **DOES NOT** copy the data.
It simply sets the data pointer to `str` and the length value to the result of
[JSL_STRLEN](#macro-jsl_strlen).

#### Parameters

**str** — the str to create the fat ptr from



#### Returns

A fat ptr

```c
JSLFatPtr jsl_fatptr_from_cstr(const char *str);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1407`

---

<a id="function-jsl_fatptr_memory_copy"></a>
### Function: `jsl_fatptr_memory_copy`

Copy the contents of `source` into `destination`.

`destination` is modified to point to the remaining data in the buffer. I.E.
if the entire buffer was used then `destination->length` will be `0` and
`destination->data` will be pointing to the end of the buffer.

This function is bounds checked, meaning a max of `destination->length` bytes
will be copied into `destination`. 

This function also checks for

* overlapping buffers
* null pointers in either `destination` or `source`
* negative lengths.
* If either `destination` or `source` would overflow if their
length was added to the pointer

In all these cases, -1 will be returned.

#### Returns

Number of bytes written or `-1` if the above error conditions were present.

```c
int jsl_fatptr_memory_copy(JSLFatPtr *destination, JSLFatPtr source);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1431`

---

<a id="function-jsl_fatptr_cstr_memory_copy"></a>
### Function: `jsl_fatptr_cstr_memory_copy`

Writes the contents of the null terminated string at `cstring` into `buffer`.

This function is bounds checked, meaning a max of `destination->length` bytes
will be copied into `destination`. This function does not check for overlapping
pointers.

If `cstring` is not a valid null terminated string then this function's behavior
is undefined, as it uses [JSL_STRLEN](#macro-jsl_strlen).

`destination` is modified to point to the remaining data in the buffer. I.E.
if the entire buffer was used then `destination->length` will be `0`.

#### Returns

Number of bytes written or `-1` if `string` or the fat pointer was null.

```c
int jsl_fatptr_cstr_memory_copy(JSLFatPtr *destination, const char *cstring, int include_null_terminator);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1448`

---

<a id="function-jsl_fatptr_substring_search"></a>
### Function: `jsl_fatptr_substring_search`

Searches `string` for the byte sequence in `substring` and returns the index of the first
match or `-1` when no match exists. This is roughly equivalent to C's `strstr` for
fat pointers.

This function is optimized with SIMD specific implementations when SIMD code generation
is enabled during compilation. When SIMD is not enabled, this function falls back to a
combination of BNDM and Sunday algorithms (based on substring size). These algorithms
are `O(n*m)` in the worst case which is generally text that is very pattern heavy and
contains a lot of repeated text. In the general case performance is closer to `O(n/m)`.

In cases where any of the following are true you will want to use a different search
function:

* Your string is very long, e.g. hundreds of megabytes or more
* Your string is full of small repeating patterns
* Your substring is more than a couple of kilobytes
* You want to search multiple different substrings on the same string

#### Parameters

**string** — the string to search in

**substring** — the substring to search for



#### Returns

Index of the first occurrence.

#### Note

The comparison operates on raw code units. In UTF encodings, multiple code units can
form a single grapheme cluster, so the index does not necessarily map to user-perceived
characters. No Unicode normalization is performed; normalize inputs first if combining mark
equivalence is required.

```c
int jsl_fatptr_substring_search(JSLFatPtr string, JSLFatPtr substring);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1482`

---

<a id="function-jsl_fatptr_index_of"></a>
### Function: `jsl_fatptr_index_of`

Locate the first byte equal to `item` in a fat pointer. This is roughly equivalent to C's
`strchr` function for fat pointers.

#### Parameters

**data** — Fat pointer to inspect.

**item** — Byte value to search for.



#### Returns

index of the first match, or -1 if none is found.

#### Note

The comparison operates on raw code units. In UTF encodings, multiple code units can
form a single grapheme cluster, so the index does not necessarily map to user-perceived
characters. No Unicode normalization is performed; normalize inputs first if combining mark
equivalence is required.

```c
int jsl_fatptr_index_of(JSLFatPtr data, int item);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1497`

---

<a id="function-jsl_fatptr_count"></a>
### Function: `jsl_fatptr_count`

Count the number of occurrences of `item` within a fat pointer.

#### Parameters

**str** — Fat pointer to scan.

**item** — Byte value to count.



#### Returns

Total number of matches, or 0 when the sequence is empty.

#### Note

The comparison operates on raw code units. In UTF encodings, multiple code units can
form a single grapheme cluster, so the index does not necessarily map to user-perceived
characters. No Unicode normalization is performed; normalize inputs first if combining mark
equivalence is required.

```c
int jsl_fatptr_count(JSLFatPtr str, int item);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1511`

---

<a id="function-jsl_fatptr_index_of_reverse"></a>
### Function: `jsl_fatptr_index_of_reverse`

Locate the final occurrence of `character` within a fat pointer.

#### Parameters

**str** — Fat pointer to inspect.

**character** — Byte value to search for.



#### Returns

index of the last match, or -1 when no match exists.

#### Note

The comparison operates on raw code units. In UTF encodings, multiple code units can
form a single grapheme cluster, so the index does not necessarily map to user-perceived
characters. No Unicode normalization is performed; normalize inputs first if combining mark
equivalence is required.

```c
int jsl_fatptr_index_of_reverse(JSLFatPtr str, int character);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1525`

---

<a id="function-jsl_fatptr_starts_with"></a>
### Function: `jsl_fatptr_starts_with`

Check whether `str` begins with the bytes stored in `prefix`.

Returns `false` when either fat pointer is null or when `prefix` exceeds `str` in length.
An empty `prefix` yields `true`.

#### Parameters

**str** — Candidate string to test.

**prefix** — Sequence that must appear at the start of `str`.



#### Returns

`true` if `str` starts with `prefix`, otherwise `false`.

#### Note

The comparison operates on raw code units. In UTF encodings, multiple code units can
form a single grapheme cluster, so the index does not necessarily map to user-perceived
characters. No Unicode normalization is performed; normalize inputs first if combining mark
equivalence is required.

```c
int jsl_fatptr_starts_with(JSLFatPtr str, JSLFatPtr prefix);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1542`

---

<a id="function-jsl_fatptr_ends_with"></a>
### Function: `jsl_fatptr_ends_with`

Check whether `str` ends with the bytes stored in `postfix`.

Returns `false` when either fat pointer is null or when `postfix` exceeds `str` in length.

#### Parameters

**str** — Candidate string to test.

**postfix** — Sequence that must appear at the end of `str`.



#### Returns

`true` if `str` ends with `postfix`, otherwise `false`.

#### Note

The comparison operates on raw code units. In UTF encodings, multiple code units can
form a single grapheme cluster, so the index does not necessarily map to user-perceived
characters. No Unicode normalization is performed; normalize inputs first if combining mark
equivalence is required.

```c
int jsl_fatptr_ends_with(JSLFatPtr str, JSLFatPtr postfix);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1558`

---

<a id="function-jsl_fatptr_basename"></a>
### Function: `jsl_fatptr_basename`

Get the file name from a filepath.

Returns a view over the final path component that follows the last `/` byte in `filename`.
The resulting fat pointer aliases the original buffer; the data is neither copied nor
reallocated. If no `/` byte is present, or the suffix after the final `/` is fewer than two
code units (for example, a trailing `/` or a single-character basename), the original fat
pointer is returned unchanged.

Like the other string utilities in this module, the search operates on raw code units. When
working with UTF encodings, code units do not necessarily correspond to grapheme clusters.
Normalize the input first if grapheme-aware behavior or Unicode canonical equivalence is
required.

#### Parameters

**filename** — Fat pointer referencing the path or filename to inspect.



#### Returns

Fat pointer referencing the basename or, in the fallback cases described above, the
original input pointer.

```
JSLFatPtr path = JSL_FATPTR_INITIALIZER("/tmp/example.txt");
JSLFatPtr base = jsl_fatptr_basename(path); // "example.txt"
```

```c
JSLFatPtr jsl_fatptr_basename(JSLFatPtr filename);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1583`

---

<a id="function-jsl_fatptr_get_file_extension"></a>
### Function: `jsl_fatptr_get_file_extension`

Get the file extension from a file name or file path.

Returns a view over the substring that follows the final `.` in `filename`.
The returned fat pointer reuses the original buffer; no allocations or copies
are performed. If `filename` does not contain a `.` byte, the result has a
`NULL` data pointer and a length of `0`.

Like the other string utilities, the search operates on raw code units.
Paths encoded with multi-byte Unicode sequences are treated as opaque bytes,
and no normalization is performed. Normalize beforehand when grapheme-aware
behavior is required.

#### Parameters

**filename** — Fat pointer referencing the path or filename to inspect.



#### Returns

Fat pointer to the extension (excluding the dot) or an empty fat pointer
when no extension exists.

```
JSLFatPtr path = JSL_FATPTR_INITIALIZER("archive.tar.gz");
JSLFatPtr ext = jsl_fatptr_get_file_extension(path); // "gz"
```

```c
JSLFatPtr jsl_fatptr_get_file_extension(JSLFatPtr filename);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1607`

---

<a id="function-jsl_fatptr_memory_compare"></a>
### Function: `jsl_fatptr_memory_compare`

Element by element comparison of the contents of the two fat pointers. If either
parameter has a null value for its data or a zero length, then this function will
return false.

#### Returns

true if equal, false otherwise.

#### Note

Do not use this to compare Unicode strings when grapheme based equality is
desired. Use this only when absolute byte equality is desired. See the note at the
top of the file about Unicode normalization.

#### Warning

This function should not be used in cryptographic contexts, like comparing
two password hashes. This function is vulnerable to timing attacks since it bails out
at the first inequality.

```c
int jsl_fatptr_memory_compare(JSLFatPtr a, JSLFatPtr b);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1624`

---

<a id="function-jsl_fatptr_cstr_compare"></a>
### Function: `jsl_fatptr_cstr_compare`

Element by element comparison of the contents of a fat pointer and a null terminated
string. If the fat pointer has a null data value or a zero length, or if cstr is null,
then this function will return false. This is true even when both pointers are
NULL.

#### Parameters

**a** — First comparator

**cstr** — A valid null terminated string



#### Note

Do not use this to compare Unicode strings when grapheme based equality is
desired. Use this only when absolute byte equality is desired. See the note at the
top of the file about Unicode normalization.

```c
int jsl_fatptr_cstr_compare(JSLFatPtr a, char *cstr);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1639`

---

<a id="function-jsl_fatptr_compare_ascii_insensitive"></a>
### Function: `jsl_fatptr_compare_ascii_insensitive`

Compare two fatptrs that both contain ASCII data for equality while ignoring case
differences. ASCII data validity is not checked.

#### Returns

true for equals, false for not equal

```c
int jsl_fatptr_compare_ascii_insensitive(JSLFatPtr a, JSLFatPtr b);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1647`

---

<a id="function-jsl_fatptr_to_lowercase_ascii"></a>
### Function: `jsl_fatptr_to_lowercase_ascii`

Modify the ASCII data in the fatptr in place to change all capital letters to
lowercase. ASCII validity is not checked.

```c
void jsl_fatptr_to_lowercase_ascii(JSLFatPtr str);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1653`

---

<a id="function-jsl_fatptr_to_int32"></a>
### Function: `jsl_fatptr_to_int32`

Reads a 32 bit integer in base-10 from the beginning of `str`.
Accepted characters are 0-9, +, and -.

Stops once it hits the first non-accepted character. This function does
not check for overflows or underflows. `result` is not written to if
there were no successfully parsed bytes.

#### Parameters

**str** — a string with an int representation at the start

**result** — out parameter where the parsing result will be stored



#### Returns

The number of bytes that were successfully read from the string

```c
int jsl_fatptr_to_int32(JSLFatPtr str, int *result);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1667`

---

<a id="function-jsl_fatptr_strip_whitespace_left"></a>
### Function: `jsl_fatptr_strip_whitespace_left`

Advance the fat pointer until the first non-whitespace character is
reached. If the fat pointer is null or has a negative length, -1 is
returned.

#### Parameters

**str** — a fat pointer



#### Returns

The number of bytes that were advanced or -1

```c
int jsl_fatptr_strip_whitespace_left(JSLFatPtr *str);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1677`

---

<a id="function-jsl_fatptr_strip_whitespace_right"></a>
### Function: `jsl_fatptr_strip_whitespace_right`

Reduce the fat pointer's length until the first non-whitespace character is
reached. If the fat pointer is null or has a negative length, -1 is
returned.

#### Parameters

**str** — a fat pointer



#### Returns

The number of bytes that were advanced or -1

```c
int jsl_fatptr_strip_whitespace_right(JSLFatPtr *str);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1687`

---

<a id="function-jsl_fatptr_strip_whitespace"></a>
### Function: `jsl_fatptr_strip_whitespace`

Modify the fat pointer such that it points to the part of the string
without any whitespace characters at the begining or the end. If the
fat pointer is null or has a negative length, -1 is returned.

#### Parameters

**str** — a fat pointer



#### Returns

The number of bytes that were advanced or -1

```c
int jsl_fatptr_strip_whitespace(JSLFatPtr *str);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1697`

---

<a id="function-jsl_fatptr_to_cstr"></a>
### Function: `jsl_fatptr_to_cstr`

Allocate a new buffer from the arena and copy the contents of a fat pointer with
a null terminator.

```c
char * jsl_fatptr_to_cstr(JSLAllocatorInterface *allocator, JSLFatPtr str);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1703`

---

<a id="function-jsl_cstr_to_fatptr"></a>
### Function: `jsl_cstr_to_fatptr`

Allocate and copy the contents of a fat pointer with a null terminator.

#### Note

Use `jsl_fatptr_from_cstr` to make a fat pointer without copying.

```c
JSLFatPtr jsl_cstr_to_fatptr(JSLAllocatorInterface *allocator, char *str);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1710`

---

<a id="function-jsl_fatptr_duplicate"></a>
### Function: `jsl_fatptr_duplicate`

Allocate space for, and copy the contents of a fat pointer.

#### Note

Use `jsl_cstr_to_fatptr` to copy a c string into a fatptr.

```c
JSLFatPtr jsl_fatptr_duplicate(JSLAllocatorInterface *allocator, JSLFatPtr str);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1717`

---

<a id="type-typedef-int64_t"></a>
### Typedef: `int64_t`

TODO: docs

There's no run time constraints on how large a chunk of data a user might give
this function.

Handling,

- blocking or non-blocking behavior
- Retries
- Partial success
- Chunking writes
- Backpressure
- Error reporting/codes

Are all your responsibility and should be in the logic of this function.

Also flushing and/or closing the sink once its lifetime is over is also your responsibilty

```c
typedef int (int *) int64_t;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1738`

---

<a id="type-jsloutputsink"></a>
### : `JSLOutputSink`

TODO: docs

Add advice on writing data generation functions with this, like buffering the output,
don't do single char

The benefits are ...

No abstraction is without it's downsides. Not all output situations will fit cleanly into
this format.

- `int write_fp;`
- `void * user_data;`


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1751`

---

<a id="function-jsl_output_sink_write_fatptr"></a>
### Function: `jsl_output_sink_write_fatptr`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_fatptr(JSLOutputSink sink, JSLFatPtr data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1761`

---

<a id="function-jsl_output_sink_write_i8"></a>
### Function: `jsl_output_sink_write_i8`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_i8(JSLOutputSink builder, int data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1772`

---

<a id="function-jsl_output_sink_write_u8"></a>
### Function: `jsl_output_sink_write_u8`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_u8(JSLOutputSink builder, int data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1783`

---

<a id="function-jsl_output_sink_write_bool"></a>
### Function: `jsl_output_sink_write_bool`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_bool(JSLOutputSink builder, int data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1794`

---

<a id="function-jsl_output_sink_write_i16"></a>
### Function: `jsl_output_sink_write_i16`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_i16(JSLOutputSink builder, int data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1804`

---

<a id="function-jsl_output_sink_write_u16"></a>
### Function: `jsl_output_sink_write_u16`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_u16(JSLOutputSink builder, int data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1815`

---

<a id="function-jsl_output_sink_write_i32"></a>
### Function: `jsl_output_sink_write_i32`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_i32(JSLOutputSink builder, int data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1826`

---

<a id="function-jsl_output_sink_write_u32"></a>
### Function: `jsl_output_sink_write_u32`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_u32(JSLOutputSink builder, int data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1837`

---

<a id="function-jsl_output_sink_write_i64"></a>
### Function: `jsl_output_sink_write_i64`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_i64(JSLOutputSink builder, int64_t data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1848`

---

<a id="function-jsl_output_sink_write_u64"></a>
### Function: `jsl_output_sink_write_u64`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_u64(JSLOutputSink builder, int data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1859`

---

<a id="function-jsl_output_sink_write_f32"></a>
### Function: `jsl_output_sink_write_f32`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_f32(JSLOutputSink builder, float data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1870`

---

<a id="function-jsl_output_sink_write_f64"></a>
### Function: `jsl_output_sink_write_f64`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_f64(JSLOutputSink builder, double data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1881`

---

<a id="function-jsl_output_sink_write_cstr"></a>
### Function: `jsl_output_sink_write_cstr`

TODO: docs

one line convenience function

```c
int jsl_output_sink_write_cstr(JSLOutputSink builder, const char *data);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1892`

---

<a id="function-jsl_fatptr_output_sink"></a>
### Function: `jsl_fatptr_output_sink`

TODO: docs

```c
JSLOutputSink jsl_fatptr_output_sink(JSLFatPtr *buffer);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1901`

---

<a id="function-jsl_format"></a>
### Function: `jsl_format`

This is a full snprintf replacement that supports everything that the C
runtime snprintf supports, including float/double, 64-bit integers, hex
floats, field parameters (%*.*d stuff), length reads backs, etc.

This returns the number of bytes written.

There are a set of different functions for different use cases

* TODO: function list

## Fat Pointers

Fat pointers can be written into the resulting string using the `%y`
format specifier. This works exactly the same way as `%.*s`. Keep in
mind that, much like `snprintf`, this function is limited to 32bit
unsigned indicies, so fat pointers over 4gb will underflow.

## Floating Point

This code uses a internal float->ascii conversion method that uses
doubles with error correction (double-doubles, for ~105 bits of
precision).  This conversion is round-trip perfect - that is, an atof
of the values output here will give you the bit-exact double back.

One difference is that our insignificant digits will be different than
with MSVC or GCC (but they don't match each other either).  We also
don't attempt to find the minimum length matching float (pre-MSVC15
doesn't either).

## 64 Bit ints

This library also supports 64-bit integers and you can use MSVC style or
GCC style indicators (%I64d or %lld).  It supports the C99 specifiers
for uint64_t and ptr_diff_t (%jd %zd) as well.

## Extras

Like some GCCs, for integers and floats, you can use a ' (single quote)
specifier and commas will be inserted on the thousands: "%'d" on 12345
would print 12,345.

For integers and floats, you can use a "$" specifier and the number
will be converted to float and then divided to get kilo, mega, giga or
tera and then printed, so "%$d" 1000 is "1.0 k", "%$.2d" 2536000 is
"2.53 M", etc. For byte values, use two $:s, like "%$$d" to turn
2536000 to "2.42 Mi". If you prefer JEDEC suffixes to SI ones, use three
$:s: "%$$$d" -> "2.42 M". To remove the space between the number and the
suffix, add "_" specifier: "%_$d" -> "2.53M".

In addition to octal and hexadecimal conversions, you can print
integers in binary: "%b" for 256 would print 100.

## Caveat

The internal counters are all unsigned 32 byte values, so if for some reason
you're using this function to print multiple gigabytes at a time, break it
into chunks.

```c
JSLFatPtr jsl_format(JSLAllocatorInterface *allocator, JSLFatPtr fmt, ...);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1962`

---

<a id="function-jsl_format_sink_valist"></a>
### Function: `jsl_format_sink_valist`

See docs for [jsl_format](#function-jsl_format).

TODO: docs

```c
int jsl_format_sink_valist(JSLOutputSink sink, JSLFatPtr fmt, int va);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1969`

---

<a id="function-jsl_format_sink"></a>
### Function: `jsl_format_sink`

See docs for [jsl_format](#function-jsl_format).

TODO: docs

```c
int jsl_format_sink(JSLOutputSink sink, JSLFatPtr fmt, ...);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1980`

---

<a id="function-jsl_format_set_separators"></a>
### Function: `jsl_format_set_separators`

Set the comma and period characters to use for the current thread.

```c
void jsl_format_set_separators(char comma, char period);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_core.h:1990`

---


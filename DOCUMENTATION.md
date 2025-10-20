# API Documentation

## Macros

- [`JSL_DEF`](#macro-jsl_def)
- [`JSL_DEFAULT_ALLOCATION_ALIGNMENT`](#macro-jsl_default_allocation_alignment)
- [`JSL_WARN_UNUSED`](#macro-jsl_warn_unused)
- [`JSL_MAX`](#macro-jsl_max)
- [`JSL_MIN`](#macro-jsl_min)
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
- [`JSL_FATPTR_LITERAL`](#macro-jsl_fatptr_literal)
- [`JSL_FATPTR_ADVANCE`](#macro-jsl_fatptr_advance)
- [`JSL_FATPTR_FROM_STACK`](#macro-jsl_fatptr_from_stack)
- [`JSL_ARENA_FROM_STACK`](#macro-jsl_arena_from_stack)
- [`JSL_ARENA_TYPED_ALLOCATE`](#macro-jsl_arena_typed_allocate)
- [`JSL_FORMAT_MIN_BUFFER`](#macro-jsl_format_min_buffer)

## Types

- [`JSLFatPtr`](#type-jslfatptr)
- [`JSLArena`](#type-jslarena)
- [`JSLStringBuilder`](#type-jslstringbuilder)
- [`JSLStringBuilderChunk`](#type-jslstringbuilderchunk)
- [`JSLStringBuilderIterator`](#type-jslstringbuilderiterator)
- [`JSL_FORMAT_CALLBACK`](#type-typedef-jsl_format_callback)
- [`JSLLoadFileResultEnum`](#type-jslloadfileresultenum)
- [`JSLWriteFileResultEnum`](#type-jslwritefileresultenum)
- [`JSLFileTypeEnum`](#type-jslfiletypeenum)

## Functions

- [`jsl_fatptr_init`](#function-jsl_fatptr_init)
- [`jsl_fatptr_slice`](#function-jsl_fatptr_slice)
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
- [`jsl_arena_init`](#function-jsl_arena_init)
- [`jsl_arena_init2`](#function-jsl_arena_init2)
- [`jsl_arena_allocate`](#function-jsl_arena_allocate)
- [`jsl_arena_allocate_aligned`](#function-jsl_arena_allocate_aligned)
- [`jsl_arena_reallocate`](#function-jsl_arena_reallocate)
- [`jsl_arena_reallocate_aligned`](#function-jsl_arena_reallocate_aligned)
- [`jsl_arena_reset`](#function-jsl_arena_reset)
- [`jsl_arena_save_restore_point`](#function-jsl_arena_save_restore_point)
- [`jsl_arena_load_restore_point`](#function-jsl_arena_load_restore_point)
- [`jsl_arena_fatptr_to_cstr`](#function-jsl_arena_fatptr_to_cstr)
- [`jsl_arena_cstr_to_fatptr`](#function-jsl_arena_cstr_to_fatptr)
- [`jsl_fatptr_duplicate`](#function-jsl_fatptr_duplicate)
- [`jsl_string_builder_init`](#function-jsl_string_builder_init)
- [`jsl_string_builder_init2`](#function-jsl_string_builder_init2)
- [`jsl_string_builder_insert_char`](#function-jsl_string_builder_insert_char)
- [`jsl_string_builder_insert_uint8_t`](#function-jsl_string_builder_insert_uint8_t)
- [`jsl_string_builder_insert_fatptr`](#function-jsl_string_builder_insert_fatptr)
- [`jsl_string_builder_format`](#function-jsl_string_builder_format)
- [`jsl_string_builder_iterator_init`](#function-jsl_string_builder_iterator_init)
- [`jsl_string_builder_iterator_next`](#function-jsl_string_builder_iterator_next)
- [`jsl_format`](#function-jsl_format)
- [`jsl_format_buffer`](#function-jsl_format_buffer)
- [`jsl_format_valist`](#function-jsl_format_valist)
- [`jsl_format_callback`](#function-jsl_format_callback)
- [`jsl_format_set_separators`](#function-jsl_format_set_separators)
- [`jsl_load_file_contents`](#function-jsl_load_file_contents)
- [`jsl_load_file_contents_buffer`](#function-jsl_load_file_contents_buffer)
- [`jsl_write_file_contents`](#function-jsl_write_file_contents)
- [`jsl_format_file`](#function-jsl_format_file)

## File: src/jacks_standard_library.h

## Jack's Standard Library

A collection of utilities which are designed to replace much of the C standard
library.

See README.md for a detailed intro.

See DESIGN.md for background on the design decisions.

See DOCUMENTATION.md for a single markdown file containing all of the docstrings
from this file. It's more nicely formatted and contains hyperlinks.

### Preprocessor Switches

`JSL_DEBUG` - turns on some debugging features, like overwriting stale memory with
`0xfeefee`.

`JSL_DEF` - allows you to override linkage/visibility (e.g., __declspec) for all of
the functions defined by this library. By default this is empty.

`JSL_WARN_UNUSED` - this controls the function attribute which tells the compiler to
issue a warning if the return value of the function is not stored in a variable, or if
that variable is never read. This is auto defined for clang and gcc, there's no
C11 compatible implementation for MSVC. If you want to turn this off, just define it as
empty string.

`JSL_ASSERT` - Assertion function definition. By default this will use `assert.h`.
If you wish to override it, it must be a function which takes three parameters, a int
conditional, a char* of the filename, and an int line number. You can also provide an
empty function if you just want to turn off asserts altogether; this is not
recommended. The small speed boost you get is from avoiding a branch is generally not
worth the loss of correctness.

`JSL_MEMCPY` - Controls memcpy calls in the library. By default this will include
`string.h` and be an alias to C's `memcpy`.

`JSL_MEMCMP` - Controls memcmp calls in the library. By default this will include
`string.h` and be an alias to C's `memcmp`.

`JSL_MEMSET` - Controls memset calls in the library. By default this will include
`string.h` and be an alias to C's `memset`.

`JSL_DEFAULT_ALLOCATION_ALIGNMENT` - Sets the alignment of allocations that aren't
explicitly set. Defaults to 16 bytes.

`JSL_INCLUDE_FILE_UTILS` - Include the file loading and writing utilities. These
require linking the standard library.

### License

Copyright (c) 2025 Jack Stouffer

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

<a id="macro-jsl_def"></a>
### Macro: `JSL_DEF`

Allows you to override linkage/visibility (e.g., __declspec) for all of
the functions defined by this library. By default this is empty.

```c
#define JSL_DEF JSL_DEF
```


*Defined at*: `src/jacks_standard_library.h:141`

---

<a id="macro-jsl_default_allocation_alignment"></a>
### Macro: `JSL_DEFAULT_ALLOCATION_ALIGNMENT`

Sets the alignment of allocations that aren't explicitly set. Defaults to 16 bytes.

```c
#define JSL_DEFAULT_ALLOCATION_ALIGNMENT JSL_DEFAULT_ALLOCATION_ALIGNMENT 16
```


*Defined at*: `src/jacks_standard_library.h:148`

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


*Defined at*: `src/jacks_standard_library.h:161`

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


*Defined at*: `src/jacks_standard_library.h:185`

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


*Defined at*: `src/jacks_standard_library.h:203`

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


*Defined at*: `src/jacks_standard_library.h:230`

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


*Defined at*: `src/jacks_standard_library.h:243`

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


*Defined at*: `src/jacks_standard_library.h:257`

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


*Defined at*: `src/jacks_standard_library.h:271`

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


*Defined at*: `src/jacks_standard_library.h:286`

---

<a id="macro-jsl_bytes"></a>
### Macro: `JSL_BYTES`

Macro to simply mark a value as representing a size in bytes. Does nothing with the value.

```c
#define JSL_BYTES JSL_BYTES ( x) x
```


*Defined at*: `src/jacks_standard_library.h:291`

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


*Defined at*: `src/jacks_standard_library.h:306`

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


*Defined at*: `src/jacks_standard_library.h:321`

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
#define JSL_GIGABYTES JSL_GIGABYTES ( x) x * 1024 * 1024 * 1024
```


*Defined at*: `src/jacks_standard_library.h:337`

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
// be at the same place everytime you run your program in your debugger!

void* buffer = VirtualAlloc(JSL_TERABYTES(2), JSL_GIGABYTES(2), MEM_RESERVE, PAGE_READWRITE);
JSLArena arena;
jsl_arena_init(&arena, buffer, JSL_GIGABYTES(2));
```

```c
#define JSL_TERABYTES JSL_TERABYTES ( x) x * 1024 * 1024 * 1024 * 1024
```


*Defined at*: `src/jacks_standard_library.h:356`

---

<a id="macro-jsl_fatptr_literal"></a>
### Macro: `JSL_FATPTR_LITERAL`

Creates a [JSLFatPtr](#type-jslfatptr) from a string literal at compile time.

#### Note

The resulting fat pointer points directly to the string literal's memory,
so no copying occurs.

Example:

```c
// Create fat pointers from string literals
JSLFatPtr hello = JSL_FATPTR_LITERAL("Hello, World!");
JSLFatPtr path = JSL_FATPTR_LITERAL("/usr/local/bin");
JSLFatPtr empty = JSL_FATPTR_LITERAL("");
```

```c
#define JSL_FATPTR_LITERAL JSL_FATPTR_LITERAL ( s) ( ( JSLFatPtr) { . data = ( uint8_t *) ( s), . length = ( int64_t) ( sizeof ( "" s "") - 1) })
```


*Defined at*: `src/jacks_standard_library.h:407`

---

<a id="macro-jsl_fatptr_advance"></a>
### Macro: `JSL_FATPTR_ADVANCE`

Advances a fat pointer forward by `n`.

#### Note

This macro does no bounds checking and is intentionally tiny so it can
live in hot loops without adding overhead.

```c
#define JSL_FATPTR_ADVANCE JSL_FATPTR_ADVANCE ( fatptr, n) fatptr . data += n; fatptr . length -= n;
```


*Defined at*: `src/jacks_standard_library.h:416`

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

```c
#define JSL_FATPTR_FROM_STACK JSL_FATPTR_FROM_STACK ( buf) ( JSLFatPtr) { . data = ( uint8_t *) ( buf), . length = ( int64_t) ( sizeof ( buf)) }
```


*Defined at*: `src/jacks_standard_library.h:432`

---

<a id="macro-jsl_arena_from_stack"></a>
### Macro: `JSL_ARENA_FROM_STACK`

Creates an arena from stack memory.

Example

```
uint8_t buffer[2048];
JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);
```

This incredibly useful for getting a dynamic allocator for things which will only
last the lifetime of the current function. For example, if the current function
needs a hash map, you can use this macro and then there's no cleanup at the end
because the stack pointer will be reset at the end of the function.

```
void some_func(void)
{
uint8_t buffer[16 * 1024];
JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);

IntToStrMap map = int_to_str_ctor(&arena);
int_to_str_add(&map, 64, JSL_FATPTR_LITERAL("This is my string data!"));
}
```

Fast, cheap, easy automatic memory management!

```c
#define JSL_ARENA_FROM_STACK JSL_ARENA_FROM_STACK ( buf) ( JSLArena) { . start = ( uint8_t *) ( buf), . current = ( uint8_t *) ( buf), . end = ( uint8_t *) ( buf) + sizeof ( buf) }
```


*Defined at*: `src/jacks_standard_library.h:496`

---

<a id="macro-jsl_arena_typed_allocate"></a>
### Macro: `JSL_ARENA_TYPED_ALLOCATE`

Macro to make it easier to allocate an instance of `T` within an arena.

#### Parameters

**T** — Type to allocate.

**arena** — Arena to allocate from; must be initialized.



#### Returns

Pointer to the allocated object or `NULL` on failure.

```
struct MyStruct { uint64_t the_data; };
uint8_t buffer[1024];
JSLArena path = JSL_ARENA_FROM_STACK(buffer);
struct MyStruct* thing = JSL_ARENA_TYPED_ALLOCATE(struct MyStruct, arena);
```

```c
#define JSL_ARENA_TYPED_ALLOCATE JSL_ARENA_TYPED_ALLOCATE ( T, arena) ( T *) jsl_arena_allocate ( arena, sizeof ( T), false) . data
```


*Defined at*: `src/jacks_standard_library.h:907`

---

<a id="macro-jsl_format_min_buffer"></a>
### Macro: `JSL_FORMAT_MIN_BUFFER`

```c
#define JSL_FORMAT_MIN_BUFFER JSL_FORMAT_MIN_BUFFER 512
```


*Defined at*: `src/jacks_standard_library.h:1124`

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


*Defined at*: `src/jacks_standard_library.h:370`

---

<a id="type-jslarena"></a>
### : `JSLArena`

A bump allocator. Designed for situations in your program when you know a
definite lifetime and a good upper bound on how much memory that lifetime will
need.

See the DESIGN.md file for detailed notes on arena implementation, their uses,
and when they shouldn't be used.

Functions and Macros:

* [jsl_arena_init](#function-jsl_arena_init)
* [jsl_arena_init2](#function-jsl_arena_init2)
* [jsl_arena_allocate](#function-jsl_arena_allocate)
* [jsl_arena_allocate_aligned](#function-jsl_arena_allocate_aligned)
* [jsl_arena_reallocate](#function-jsl_arena_reallocate)
* [jsl_arena_reallocate_aligned](#function-jsl_arena_reallocate_aligned)
* [jsl_arena_reset](#function-jsl_arena_reset)
* [jsl_arena_save_restore_point](#function-jsl_arena_save_restore_point)
* [jsl_arena_load_restore_point](#function-jsl_arena_load_restore_point)
* [JSL_ARENA_TYPED_ALLOCATE](#macro-jsl_arena_typed_allocate)
* [JSL_ARENA_FROM_STACK](#macro-jsl_arena_from_stack)

#### Note

The arena API is not thread safe. Arena memory is assumed to live in a
single thread. If you want to share an arena between threads you need to lock.

- `int * start;`
- `int * current;`
- `int * end;`


*Defined at*: `src/jacks_standard_library.h:461`

---

<a id="type-jslstringbuilder"></a>
### : `JSLStringBuilder`

A string builder is a container for building large strings. It's specialized for
situations where many different smaller operations result in small strings being
coalesced into a final result, specifically using an arena as its allocator.

While this is called string builder, the underlying data store is just bytes, so
any binary data which is built in chunks can use the string builder.

A string builder is different from a normal dynamic array in two ways. One, it
has specific operations for writing string data in both fat pointer form but also
as a `snprintf` like operation. Two, the resulting string data is not stored as a
contiguous range of memory, but as a series of chunks which is given to the user
as an iterator when the string is finished. 

This is due to the nature of arena allocations. If you're using an arena for a
life time in your program, then the most common case is 

1. You do some operations, these operations themselves allocate
2. You add generate a string from the operations
3. The string is concatenated into some buffer
4. Repeat

A dynamically sized array which grows would mean throwing away the old memory when
the array resizes. A separate arena that's used purely for the array would work, but
with the string builder allows for less lifetimes to track.

By default, each chunk is 256 bytes and is aligned to a 32 byte address. These are
tuneable parameters that you can set during init. The custom alignment helps if you
want to use SIMD code on the consuming code.

Functions:

* [jsl_string_builder_init](#function-jsl_string_builder_init)
* [jsl_string_builder_init2](#function-jsl_string_builder_init2)
* [jsl_string_builder_insert_char](#function-jsl_string_builder_insert_char)
* [jsl_string_builder_insert_uint8_t](#function-jsl_string_builder_insert_uint8_t)
* [jsl_string_builder_insert_fatptr](#function-jsl_string_builder_insert_fatptr)
* [jsl_string_builder_format](#function-jsl_string_builder_format)

- `JSLArena * arena;`
- `struct JSLStringBuilderChunk * head;`
- `struct JSLStringBuilderChunk * tail;`
- `int alignment;`
- `int chunk_size;`


*Defined at*: `src/jacks_standard_library.h:540`

---

<a id="type-jslstringbuilderchunk"></a>
### : `JSLStringBuilderChunk`



*Defined at*: `src/jacks_standard_library.h:543`

---

<a id="type-jslstringbuilderiterator"></a>
### : `JSLStringBuilderIterator`

The iterator type for a [JSLStringBuilder](#type-jslstringbuilder) instance. This keeps track of
where the iterator is over the course of calling the next function.

#### Warning

It is not valid to modify a string builder while iterating over it. 

Functions:

* [jsl_string_builder_iterator_init](#function-jsl_string_builder_iterator_init)
* [jsl_string_builder_iterator_next](#function-jsl_string_builder_iterator_next)

- `struct JSLStringBuilderChunk * current;`


*Defined at*: `src/jacks_standard_library.h:560`

---

<a id="function-jsl_fatptr_init"></a>
### Function: `jsl_fatptr_init`

Constructor utility function to make a fat pointer out of a pointer and a length.
Useful in cases where you can't use C's struct init syntax, like as a parameter
to a function.

```c
JSLFatPtr jsl_fatptr_init(int *ptr, int length);
```


*Defined at*: `src/jacks_standard_library.h:571`

---

<a id="function-jsl_fatptr_slice"></a>
### Function: `jsl_fatptr_slice`

Create a new fat pointer that points to the given parameter's data but
with a view of [start, end).

This function is bounds checked. Out of bounds slices will assert.

```c
JSLFatPtr jsl_fatptr_slice(JSLFatPtr fatptr, int start, int end);
```


*Defined at*: `src/jacks_standard_library.h:579`

---

<a id="function-jsl_fatptr_total_write_length"></a>
### Function: `jsl_fatptr_total_write_length`

Utility function to get the total amount of bytes written to the original
fat pointer when compared to a writer fat pointer. See [jsl_fatptr_auto_slice](#function-jsl_fatptr_auto_slice)
to get a slice of the written portion.

This function checks for NULL and checks that `writer_fatptr` points to data
in `original_fatptr`. If either of these checks fail, then `-1` is returned.

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


*Defined at*: `src/jacks_standard_library.h:601`

---

<a id="function-jsl_fatptr_auto_slice"></a>
### Function: `jsl_fatptr_auto_slice`

Returns the slice in `original_fatptr` that represents the written to portion, given
the size and pointer in `writer_fatptr`. If either parameter has a NULL data
field, has a negative length, or if the writer does not point to a portion
of the original allocation, this function will return a fat pointer with a 
`NULL` data pointer.

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


*Defined at*: `src/jacks_standard_library.h:625`

---

<a id="function-jsl_fatptr_from_cstr"></a>
### Function: `jsl_fatptr_from_cstr`

Build a fat pointer from a null terminated string. **DOES NOT** copy the data.
It simply sets the data pointer to `str` and the length value to the result of
JSL_STRLEN.

#### Parameters

**str** — the str to create the fat ptr from



#### Returns

A fat ptr

```c
JSLFatPtr jsl_fatptr_from_cstr(char *str);
```


*Defined at*: `src/jacks_standard_library.h:635`

---

<a id="function-jsl_fatptr_memory_copy"></a>
### Function: `jsl_fatptr_memory_copy`

Copy the contents of `source` into `destination`.

This function is bounds checked, meaning a max of `destination->length` bytes
will be copied into `destination`. This function also checks for overlapping
buffers, null pointers in either `destination` or `source`, and negative lengths.
In all these cases, -1 will be returned.

`destination` is modified to point to the remaining data in the buffer. I.E.
if the entire buffer was used then `destination->length` will be `0` and
`destination->data` will be pointing to the end of the buffer.

#### Returns

Number of bytes written or `-1` if the above error conditions were present.

```c
int jsl_fatptr_memory_copy(JSLFatPtr *destination, JSLFatPtr source);
```


*Defined at*: `src/jacks_standard_library.h:651`

---

<a id="function-jsl_fatptr_cstr_memory_copy"></a>
### Function: `jsl_fatptr_cstr_memory_copy`

Writes the contents of the null terminated string at `cstring` into `buffer`.

This function is bounds checked, meaning a max of `destination->length` bytes
will be copied into `destination`. This function does not check for overlapping
pointers.

If `cstring` is not a valid null terminated string then this function's behavior
is undefined, as it uses JSL_STRLEN.

`destination` is modified to point to the remaining data in the buffer. I.E.
if the entire buffer was used then `destination->length` will be `0`.

#### Returns

Number of bytes written or `-1` if `string` or the fat pointer was null.

```c
int jsl_fatptr_cstr_memory_copy(JSLFatPtr *destination, char *cstring, int include_null_terminator);
```


*Defined at*: `src/jacks_standard_library.h:668`

---

<a id="function-jsl_fatptr_substring_search"></a>
### Function: `jsl_fatptr_substring_search`

Searches `string` for the byte sequence in `substring` and returns the index of the first
match or `-1` when no match exists. Both fat pointers must reference valid data.

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


*Defined at*: `src/jacks_standard_library.h:687`

---

<a id="function-jsl_fatptr_index_of"></a>
### Function: `jsl_fatptr_index_of`

Locate the first byte equal to `item` in a fat pointer.

#### Parameters

**data** — Fat pointer to inspect.

**item** — Byte value to search for.



#### Returns

index of the first match, or -1 if none is found.

```c
int jsl_fatptr_index_of(JSLFatPtr data, int item);
```


*Defined at*: `src/jacks_standard_library.h:696`

---

<a id="function-jsl_fatptr_count"></a>
### Function: `jsl_fatptr_count`

Count the number of occurrences of `item` within a fat pointer.

#### Parameters

**str** — Fat pointer to scan.

**item** — Byte value to count.



#### Returns

Total number of matches, or 0 when the sequence is empty.

```c
int jsl_fatptr_count(JSLFatPtr str, int item);
```


*Defined at*: `src/jacks_standard_library.h:705`

---

<a id="function-jsl_fatptr_index_of_reverse"></a>
### Function: `jsl_fatptr_index_of_reverse`

Locate the final occurrence of `character` within a fat pointer.

#### Parameters

**str** — Fat pointer to inspect.

**character** — Byte value to search for.



#### Returns

index of the last match, or -1 when no match exists.

```c
int jsl_fatptr_index_of_reverse(JSLFatPtr str, int character);
```


*Defined at*: `src/jacks_standard_library.h:714`

---

<a id="function-jsl_fatptr_starts_with"></a>
### Function: `jsl_fatptr_starts_with`

Check whether `str` begins with the bytes stored in `prefix`.

Returns `false` when either fat pointer is null or when `prefix` exceeds `str` in length.
Comparison is bytewise; no Unicode normalization is performed. An empty `prefix` yields
`true`.

#### Parameters

**str** — Candidate string to test.

**prefix** — Sequence that must appear at the start of `str`.



#### Returns

`true` if `str` starts with `prefix`, otherwise `false`.

```c
int jsl_fatptr_starts_with(JSLFatPtr str, JSLFatPtr prefix);
```


*Defined at*: `src/jacks_standard_library.h:727`

---

<a id="function-jsl_fatptr_ends_with"></a>
### Function: `jsl_fatptr_ends_with`

Check whether `str` ends with the bytes stored in `postfix`.

Returns `false` when either fat pointer is null or when `postfix` exceeds `str` in length.
Comparison is bytewise; no Unicode normalization is performed.

#### Parameters

**str** — Candidate string to test.

**postfix** — Sequence that must appear at the end of `str`.



#### Returns

`true` if `str` ends with `postfix`, otherwise `false`.

```c
int jsl_fatptr_ends_with(JSLFatPtr str, JSLFatPtr postfix);
```


*Defined at*: `src/jacks_standard_library.h:739`

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
JSLFatPtr path = jsl_fatptr_from_cstr("/tmp/example.txt");
JSLFatPtr base = jsl_fatptr_basename(path); // "example.txt"
```

```c
JSLFatPtr jsl_fatptr_basename(JSLFatPtr filename);
```


*Defined at*: `src/jacks_standard_library.h:764`

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
JSLFatPtr path = jsl_fatptr_from_cstr("archive.tar.gz");
JSLFatPtr ext = jsl_fatptr_get_file_extension(path); // "gz"
```

```c
JSLFatPtr jsl_fatptr_get_file_extension(JSLFatPtr filename);
```


*Defined at*: `src/jacks_standard_library.h:788`

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


*Defined at*: `src/jacks_standard_library.h:806`

---

<a id="function-jsl_fatptr_cstr_compare"></a>
### Function: `jsl_fatptr_cstr_compare`

Element by element comparison of the contents of a fat pointer and a null terminated
string. If the fat pointer has a null data value or a zero length, or if cstr is null,
then this function will return false.

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


*Defined at*: `src/jacks_standard_library.h:820`

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


*Defined at*: `src/jacks_standard_library.h:828`

---

<a id="function-jsl_fatptr_to_lowercase_ascii"></a>
### Function: `jsl_fatptr_to_lowercase_ascii`

Modify the ASCII data in the fatptr in place to change all capital letters to
lowercase. ASCII validity is not checked.

```c
void jsl_fatptr_to_lowercase_ascii(JSLFatPtr str);
```


*Defined at*: `src/jacks_standard_library.h:834`

---

<a id="function-jsl_fatptr_to_int32"></a>
### Function: `jsl_fatptr_to_int32`

Reads a 32 bit integer in base-10 from the beginning of `str`.
Accepted characters are 0-9, +, and -.

Stops once it hits the first non-accepted character. This function does
not check for overflows or underflows. `result` is not written to if
there were no successfully parsed bytes.

#### Returns

The number of bytes that were successfully read from the string

```c
int jsl_fatptr_to_int32(JSLFatPtr str, int *result);
```


*Defined at*: `src/jacks_standard_library.h:846`

---

<a id="function-jsl_arena_init"></a>
### Function: `jsl_arena_init`

Initialize an arena with the supplied buffer.

#### Parameters

**arena** — Arena instance to initialize; must not be null.

**memory** — Pointer to the beginning of the backing storage.

**length** — Size of the backing storage in bytes.

```c
void jsl_arena_init(JSLArena *arena, void *memory, int length);
```


*Defined at*: `src/jacks_standard_library.h:855`

---

<a id="function-jsl_arena_init2"></a>
### Function: `jsl_arena_init2`

Initialize an arena using a fat pointer as the backing buffer.

This is a convenience overload for cases where the backing memory and its
length are already packaged in a `JSLFatPtr`.

#### Parameters

**arena** — Arena to initialize; must not be null.

**memory** — Backing storage for the arena; `memory.data` must not be null.

```c
void jsl_arena_init2(JSLArena *arena, JSLFatPtr memory);
```


*Defined at*: `src/jacks_standard_library.h:866`

---

<a id="function-jsl_arena_allocate"></a>
### Function: `jsl_arena_allocate`

Allocate a block of memory from the arena using the default alignment.

The returned fat pointer contains a null data pointer if the arena does not
have enough capacity. When `zeroed` is true, the allocated bytes are
zero-initialized.

#### Parameters

**arena** — Arena to allocate from; must not be null.

**bytes** — Number of bytes to reserve.

**zeroed** — When true, zero-initialize the allocation.



#### Returns

Fat pointer describing the allocation or `{0}` on failure.

```c
JSLFatPtr jsl_arena_allocate(JSLArena *arena, int bytes, int zeroed);
```


*Defined at*: `src/jacks_standard_library.h:880`

---

<a id="function-jsl_arena_allocate_aligned"></a>
### Function: `jsl_arena_allocate_aligned`

Allocate a block of memory from the arena with the provided alignment.

#### Parameters

**arena** — Arena to allocate from; must not be null.

**bytes** — Number of bytes to reserve.

**alignment** — Desired alignment in bytes; must be a positive power of two.

**zeroed** — When true, zero-initialize the allocation.



#### Returns

Fat pointer describing the allocation or `{0}` on failure.

```c
JSLFatPtr jsl_arena_allocate_aligned(JSLArena *arena, int bytes, int alignment, int zeroed);
```


*Defined at*: `src/jacks_standard_library.h:891`

---

<a id="function-jsl_arena_reallocate"></a>
### Function: `jsl_arena_reallocate`

Resize the allocation if it was the last allocation, otherwise, allocate a new
chunk of memory.

In debug mode, this function will set `original_allocation->data` to null to
help detect stale pointer bugs.

```c
JSLFatPtr jsl_arena_reallocate(JSLArena *arena, JSLFatPtr original_allocation, int new_num_bytes);
```


*Defined at*: `src/jacks_standard_library.h:916`

---

<a id="function-jsl_arena_reallocate_aligned"></a>
### Function: `jsl_arena_reallocate_aligned`

Resize the allocation if it was the last allocation, otherwise, allocate a new
chunk of memory.

In debug mode, this function will set `original_allocation->data` to null to
help detect stale pointer bugs.

```c
JSLFatPtr jsl_arena_reallocate_aligned(JSLArena *arena, JSLFatPtr original_allocation, int new_num_bytes, int align);
```


*Defined at*: `src/jacks_standard_library.h:929`

---

<a id="function-jsl_arena_reset"></a>
### Function: `jsl_arena_reset`

Set the current pointer back to the start of the arena.

In debug mode, this function will set all of the memory that was
allocated to `0xfeeefeee` to help detect use after free bugs.

```c
void jsl_arena_reset(JSLArena *arena);
```


*Defined at*: `src/jacks_standard_library.h:942`

---

<a id="function-jsl_arena_save_restore_point"></a>
### Function: `jsl_arena_save_restore_point`

The functions [jsl_arena_save_restore_point](#function-jsl_arena_save_restore_point) and [jsl_arena_load_restore_point](#function-jsl_arena_load_restore_point)
help you make temporary allocations inside an existing arena. You can think of
it as an "arena inside an arena". Basically the save function marks the current
state of the arena and the load function sets the saved state to the given arena,
wiping out any allocations which happened in the interim.

This is very useful when you need memory from the arena but only for a specific
function.

For example, say you have an existing one megabyte arena that has used 128 kilobytes
of space. You then call a function with this arena which needs a string to make an
operating system call, but that string is no longer needed after the function returns.
You can "save" and "load" a restore point at the start and end of the function
(respectively) and when the function returns, the arena will still only have 128
kilobytes used.

In debug mode, [jsl_arena_load_restore_point](#function-jsl_arena_load_restore_point) function will set all of the memory
that was allocated to `0xfeeefeee` to help detect use after free bugs.

```c
int * jsl_arena_save_restore_point(JSLArena *arena);
```


*Defined at*: `src/jacks_standard_library.h:964`

---

<a id="function-jsl_arena_load_restore_point"></a>
### Function: `jsl_arena_load_restore_point`

The functions [jsl_arena_save_restore_point](#function-jsl_arena_save_restore_point) and [jsl_arena_load_restore_point](#function-jsl_arena_load_restore_point)
help you make temporary allocations inside an existing arena. You can think of
it as an "arena inside an arena". Basically the save function marks the current
state of the arena and the load function sets the saved state to the given arena,
wiping out any allocations which happened in the interim.

This is very useful when you need memory from the arena but only for a specific
function.

For example, say you have an existing one megabyte arena that has used 128 kilobytes
of space. You then call a function with this arena which needs a string to make an
operating system call, but that string is no longer needed after the function returns.
You can "save" and "load" a restore point at the start and end of the function
(respectively) and when the function returns, the arena will still only have 128
kilobytes used.

In debug mode, [jsl_arena_load_restore_point](#function-jsl_arena_load_restore_point) function will set all of the memory
that was allocated to `0xfeeefeee` to help detect use after free bugs.

```c
void jsl_arena_load_restore_point(JSLArena *arena, int *restore_point);
```


*Defined at*: `src/jacks_standard_library.h:986`

---

<a id="function-jsl_arena_fatptr_to_cstr"></a>
### Function: `jsl_arena_fatptr_to_cstr`

Allocate a new buffer from the arena and copy the contents of a fat pointer with
a null terminator.

```c
char * jsl_arena_fatptr_to_cstr(JSLArena *arena, JSLFatPtr str);
```


*Defined at*: `src/jacks_standard_library.h:992`

---

<a id="function-jsl_arena_cstr_to_fatptr"></a>
### Function: `jsl_arena_cstr_to_fatptr`

Allocate and copy the contents of a fat pointer with a null terminator.

#### Note

Use `jsl_fatptr_from_cstr` to make a fat pointer without copying.

```c
JSLFatPtr jsl_arena_cstr_to_fatptr(JSLArena *arena, char *str);
```


*Defined at*: `src/jacks_standard_library.h:999`

---

<a id="function-jsl_fatptr_duplicate"></a>
### Function: `jsl_fatptr_duplicate`

Allocate space for, and copy the contents of a fat pointer.

#### Note

Use `jsl_arena_cstr_to_fatptr` to copy a c string into a fatptr.

```c
JSLFatPtr jsl_fatptr_duplicate(JSLArena *arena, JSLFatPtr str);
```


*Defined at*: `src/jacks_standard_library.h:1006`

---

<a id="function-jsl_string_builder_init"></a>
### Function: `jsl_string_builder_init`

Initialize a [JSLStringBuilder](#type-jslstringbuilder) using the default settings. See the [JSLStringBuilder](#type-jslstringbuilder)
for more information on the container. A chunk is allocated right away and if that
fails this returns false.

#### Parameters

**builder** — The builder instance to initialize; must not be NULL.

**arena** — The arena that backs all allocations made by the builder; must not be NULL.



#### Returns

`true` if the builder was initialized successfully, otherwise `false`.

```c
int jsl_string_builder_init(JSLStringBuilder *builder, JSLArena *arena);
```


*Defined at*: `src/jacks_standard_library.h:1017`

---

<a id="function-jsl_string_builder_init2"></a>
### Function: `jsl_string_builder_init2`

Initialize a [JSLStringBuilder](#type-jslstringbuilder) with a custom chunk size and chunk allocation alignment.
See the [JSLStringBuilder](#type-jslstringbuilder) for more information on the container. A chunk is allocated
right away and if that fails this returns false.

#### Parameters

**builder** — The builder instance to initialize; must not be NULL.

**arena** — The arena that backs all allocations made by the builder; must not be NULL.

**chunk_size** — The number of bytes that are allocated each time the container needs to grow

**alignment** — The allocation alignment of the chunks of data



#### Returns

`true` if the builder was initialized successfully, otherwise `false`.

```c
int jsl_string_builder_init2(JSLStringBuilder *builder, JSLArena *arena, int chunk_size, int alignment);
```


*Defined at*: `src/jacks_standard_library.h:1030`

---

<a id="function-jsl_string_builder_insert_char"></a>
### Function: `jsl_string_builder_insert_char`

Append a char value to the end of the string builder without interpretation. Each append
may result in an allocation if there's no more space. If that allocation fails then this
function returns false.

#### Parameters

**builder** — The string builder to append to; must be initialized.

**c** — The byte to append.



#### Returns

`true` if the byte was inserted successfully, otherwise `false`.

```c
int jsl_string_builder_insert_char(JSLStringBuilder *builder, char c);
```


*Defined at*: `src/jacks_standard_library.h:1041`

---

<a id="function-jsl_string_builder_insert_uint8_t"></a>
### Function: `jsl_string_builder_insert_uint8_t`

Append a single raw byte to the end of the string builder without interpretation. 
The value is written as-is, so it can be used for arbitrary binary data, including
zero bytes. Each append may result in an allocation if there's no more space. If
that allocation fails then this function returns false.

#### Parameters

**builder** — The string builder to append to; must be initialized.

**c** — The byte to append.



#### Returns

`true` if the byte was inserted successfully, otherwise `false`.

```c
int jsl_string_builder_insert_uint8_t(JSLStringBuilder *builder, int c);
```


*Defined at*: `src/jacks_standard_library.h:1053`

---

<a id="function-jsl_string_builder_insert_fatptr"></a>
### Function: `jsl_string_builder_insert_fatptr`

Append the contents of a fat pointer. Additional chunks are allocated as needed
while copying so if any of the allocations fail this returns false.

#### Parameters

**builder** — The string builder to append to; must be initialized.

**data** — A fat pointer describing the bytes to copy; its length may be zero.



#### Returns

`true` if the data was appended successfully, otherwise `false`.

```c
int jsl_string_builder_insert_fatptr(JSLStringBuilder *builder, JSLFatPtr data);
```


*Defined at*: `src/jacks_standard_library.h:1063`

---

<a id="function-jsl_string_builder_format"></a>
### Function: `jsl_string_builder_format`

Format a string using the [jsl_format](#function-jsl_format) logic and write the result directly into
the string builder.

#### Parameters

**builder** — The string builder that receives the formatted output; must be initialized.

**fmt** — A fat pointer describing the format string.

**...** — Variadic arguments consumed by the formatter.



#### Returns

`true` if formatting succeeded and the formatted bytes were appended, otherwise `false`.

```c
int jsl_string_builder_format(JSLStringBuilder *builder, JSLFatPtr fmt, ...);
```


*Defined at*: `src/jacks_standard_library.h:1074`

---

<a id="function-jsl_string_builder_iterator_init"></a>
### Function: `jsl_string_builder_iterator_init`

Initialize an iterator instance so it will traverse the given string builder
from the begining. It's easiest to just put an empty iterator on the stack
and then call this function.

```
JSLStringBuilder builder = ...;

JSLStringBuilderIterator iter;
jsl_string_builder_iterator_init(&builder, &iter);
```

#### Parameters

**builder** — The string builder whose data will be traversed.

**iterator** — The iterator instance to initialize.

```c
void jsl_string_builder_iterator_init(JSLStringBuilder *builder, JSLStringBuilderIterator *iterator);
```


*Defined at*: `src/jacks_standard_library.h:1091`

---

<a id="function-jsl_string_builder_iterator_next"></a>
### Function: `jsl_string_builder_iterator_next`

Get the next chunk of data a string builder iterator. The chunk will
have a `NULL` data pointer when iteration is over.

This example program prints all the data in a string builder to stdout:

```
#include <stdio.h>

JSLStringBuilder builder = ...;

JSLStringBuilderIterator iter;
jsl_string_builder_iterator_init(&builder, &iter);

while (true)
{
JSLFatPtr str = jsl_string_builder_iterator_next(&iter);

if (str.data == NULL)
break;

jsl_format_out(stdout, str);
}
```

#### Parameters

**iterator** — The iterator instance



#### Returns

The next chunk of data from the string builder

```c
JSLFatPtr jsl_string_builder_iterator_next(JSLStringBuilderIterator *iterator);
```


*Defined at*: `src/jacks_standard_library.h:1121`

---

<a id="type-typedef-jsl_format_callback"></a>
### Typedef: `JSL_FORMAT_CALLBACK`

Function signature for receiving formatted output from `jsl_format_callback`.

The formatter hands over `len` bytes starting at `buf` each time the internal
buffer fills up or is flushed. The callback should consume those bytes (copy,
write, etc.) and then return a pointer to the buffer that should be used for
subsequent writes. Returning `NULL` signals an error or early termination,
causing `jsl_format_callback` to stop producing output.

#### Parameters

**buf** — Pointer to `len` bytes of freshly formatted data.

**user** — Opaque pointer that was supplied to `jsl_format_callback`.

**len** — Number of valid bytes in `buf`.



#### Returns

Pointer to the buffer that will receive the next chunk, or `NULL` to stop.

```c
typedef int *(int *, void *, int) JSL_FORMAT_CALLBACK;
```


*Defined at*: `src/jacks_standard_library.h:1141`

---

<a id="function-jsl_format"></a>
### Function: `jsl_format`

This is a full snprintf replacement that supports everything that the C
runtime snprintf supports, including float/double, 64-bit integers, hex
floats, field parameters (%*.*d stuff), length reads backs, etc.

This returns the number of bytes written.

There are a set of different functions for different use cases

* [jsl_format](#function-jsl_format)
* [jsl_format_buffer](#function-jsl_format_buffer)
* [jsl_format_valist](#function-jsl_format_valist)
* [jsl_format_callback](#function-jsl_format_callback)
* [jsl_string_builder_format](#function-jsl_string_builder_format)

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
JSLFatPtr jsl_format(JSLArena *arena, JSLFatPtr fmt, ...);
```


*Defined at*: `src/jacks_standard_library.h:1206`

---

<a id="function-jsl_format_buffer"></a>
### Function: `jsl_format_buffer`

See docs for [jsl_format](#function-jsl_format).

Writes into a provided buffer, up to `buffer.length` bytes.

```c
int jsl_format_buffer(JSLFatPtr *buffer, JSLFatPtr fmt, ...);
```


*Defined at*: `src/jacks_standard_library.h:1213`

---

<a id="function-jsl_format_valist"></a>
### Function: `jsl_format_valist`

See docs for [jsl_format](#function-jsl_format).

Writes into a provided buffer, up to `buffer.length` bytes using a variadic
argument list.

```c
int jsl_format_valist(JSLFatPtr *buffer, JSLFatPtr fmt, int va);
```


*Defined at*: `src/jacks_standard_library.h:1225`

---

<a id="function-jsl_format_callback"></a>
### Function: `jsl_format_callback`

See docs for [jsl_format](#function-jsl_format).

Convert into a buffer, calling back every [JSL_FORMAT_MIN_BUFFER](#macro-jsl_format_min_buffer) chars.
Your callback can then copy the chars out, print them or whatever.
This function is actually the workhorse for everything else.
The buffer you pass in must hold at least [JSL_FORMAT_MIN_BUFFER](#macro-jsl_format_min_buffer) characters.

You return the next buffer to use or 0 to stop converting

```c
int jsl_format_callback(JSL_FORMAT_CALLBACK *callback, void *user, int *buf, JSLFatPtr fmt, int va);
```


*Defined at*: `src/jacks_standard_library.h:1241`

---

<a id="function-jsl_format_set_separators"></a>
### Function: `jsl_format_set_separators`

Set the comma and period characters to use for the current thread.

```c
void jsl_format_set_separators(char comma, char period);
```


*Defined at*: `src/jacks_standard_library.h:1253`

---

<a id="type-jslloadfileresultenum"></a>
### : `JSLLoadFileResultEnum`

- `JSL_FILE_LOAD_BAD_PARAMETERS = 0`
- `JSL_FILE_LOAD_SUCCESS = 1`
- `JSL_FILE_LOAD_COULD_NOT_OPEN = 2`
- `JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE = 3`
- `JSL_FILE_LOAD_COULD_NOT_GET_MEMORY = 4`
- `JSL_FILE_LOAD_READ_FAILED = 5`
- `JSL_FILE_LOAD_CLOSE_FAILED = 6`
- `JSL_FILE_LOAD_ERROR_UNKNOWN = 7`
- `JSL_FILE_LOAD_ENUM_COUNT = 8`


*Defined at*: `src/jacks_standard_library.h:1283`

---

<a id="type-jslwritefileresultenum"></a>
### : `JSLWriteFileResultEnum`

- `JSL_FILE_WRITE_BAD_PARAMETERS = 0`
- `JSL_FILE_WRITE_SUCCESS = 1`
- `JSL_FILE_WRITE_COULD_NOT_OPEN = 2`
- `JSL_FILE_WRITE_COULD_NOT_WRITE = 3`
- `JSL_FILE_WRITE_COULD_NOT_CLOSE = 4`
- `JSL_FILE_WRITE_ENUM_COUNT = 5`


*Defined at*: `src/jacks_standard_library.h:1298`

---

<a id="type-jslfiletypeenum"></a>
### : `JSLFileTypeEnum`

- `JSL_FILE_TYPE_UNKNOWN = 0`
- `JSL_FILE_TYPE_REG = 1`
- `JSL_FILE_TYPE_DIR = 2`
- `JSL_FILE_TYPE_SYMLINK = 3`
- `JSL_FILE_TYPE_BLOCK = 4`
- `JSL_FILE_TYPE_CHAR = 5`
- `JSL_FILE_TYPE_FIFO = 6`
- `JSL_FILE_TYPE_SOCKET = 7`
- `JSL_FILE_TYPE_COUNT = 8`


*Defined at*: `src/jacks_standard_library.h:1310`

---

<a id="function-jsl_load_file_contents"></a>
### Function: `jsl_load_file_contents`

Load the contents of the file at `path` into a newly allocated buffer
from the given arena. The buffer will be the exact size of the file contents.

If the arena does not have enough space,

```c
JSLLoadFileResultEnum jsl_load_file_contents(JSLArena *arena, JSLFatPtr path, JSLFatPtr *out_contents, int *out_errno);
```


*Defined at*: `src/jacks_standard_library.h:1329`

---

<a id="function-jsl_load_file_contents_buffer"></a>
### Function: `jsl_load_file_contents_buffer`

Load the contents of the file at `path` into an existing fat pointer buffer.

Copies up to `buffer->length` bytes into `buffer->data` and advances the fat
pointer by the amount read so the caller can continue writing into the same
backing storage. Returns a `JSLLoadFileResultEnum` describing the outcome and
optionally stores the system `errno` in `out_errno` on failure.

#### Parameters

**buffer** — buffer to write to

**path** — The file system path

**out_errno** — A pointer which will be written to with the errno on failure



#### Returns

An enum which represents the result

```c
JSLLoadFileResultEnum jsl_load_file_contents_buffer(JSLFatPtr *buffer, JSLFatPtr path, int *out_errno);
```


*Defined at*: `src/jacks_standard_library.h:1349`

---

<a id="function-jsl_write_file_contents"></a>
### Function: `jsl_write_file_contents`

Write the bytes in `contents` to the file located at `path`.

Opens or creates the destination file and attempts to write the entire
contents buffer. Returns a `JSLWriteFileResultEnum` describing the
outcome, stores the number of bytes written in `bytes_written` when
provided, and optionally writes the failing `errno` into `out_errno`.

#### Parameters

**contents** — Data to be written to disk

**path** — File system path to write to

**bytes_written** — Optional pointer that receives the bytes written on success

**out_errno** — Optional pointer that receives the system errno on failure



#### Returns

A result enum describing the write outcome

```c
JSLWriteFileResultEnum jsl_write_file_contents(JSLFatPtr contents, JSLFatPtr path, int *bytes_written, int *out_errno);
```


*Defined at*: `src/jacks_standard_library.h:1369`

---

<a id="function-jsl_format_file"></a>
### Function: `jsl_format_file`

Format a string using the JSL formatter and write the result to a `FILE*`,
most often this will be `stdout`.

Streams that reject writes (for example, read-only streams or closed
pipes) cause the function to return `false`. Passing a `NULL` file handle,
a `NULL` format pointer, or a negative format length also causes failure.

#### Parameters

**out** — Destination stream

**fmt** — Format string

**...** — Format args



#### Returns

`true` when formatting and writing succeeds, otherwise `false`

```c
int jsl_format_file(int *out, JSLFatPtr fmt, ...);
```


*Defined at*: `src/jacks_standard_library.h:1389`

---


# API Documentation

## Macros

- [`JSL_INCLUDE_FILE_UTILS`](#macro-jsl_include_file_utils)
- [`JACKS_STANDARD_LIBRARY`](#macro-jacks_standard_library)
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

- [Struct `JSLFatPtr`](#type-struct-jslfatptr)
- [Typedef `JSLFatPtr`](#type-typedef-jslfatptr)
- [Struct `JSLArena`](#type-struct-jslarena)
- [Typedef `JSLArena`](#type-typedef-jslarena)
- [Struct `JSLStringBuilder`](#type-struct-jslstringbuilder)
- [Struct `JSLStringBuilderChunk`](#type-struct-jslstringbuilderchunk)
- [Typedef `JSLStringBuilder`](#type-typedef-jslstringbuilder)
- [Struct `JSLStringBuilderIterator`](#type-struct-jslstringbuilderiterator)
- [Typedef `JSLStringBuilderIterator`](#type-typedef-jslstringbuilderiterator)
- [Typedef `JSL_FORMAT_CALLBACK`](#type-typedef-jsl_format_callback)
- [Enum `JSLLoadFileResultEnum`](#type-enum-jslloadfileresultenum)
- [Typedef `JSLLoadFileResultEnum`](#type-typedef-jslloadfileresultenum)
- [Enum `JSLWriteFileResultEnum`](#type-enum-jslwritefileresultenum)
- [Typedef `JSLWriteFileResultEnum`](#type-typedef-jslwritefileresultenum)
- [Enum `JSLFileTypeEnum`](#type-enum-jslfiletypeenum)
- [Typedef `JSLFileTypeEnum`](#type-typedef-jslfiletypeenum)

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

See the README.md for an intro

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
`string.h` and use `memcpy`.

`JSL_MEMCMP` - Controls memcmp calls in the library. By default this will include
`string.h` and use `memcmp`.

`JSL_MEMSET` - Controls memset calls in the library. By default this will include
`string.h` and use `memset`.

`JSL_DEFAULT_ALLOCATION_ALIGNMENT` - Sets the alignment of allocations that aren't
explicitly set. Defaults to 16 bytes.

`JSL_INCLUDE_FILE_UTILS` - Include the file loading and writing utilities. These
require linking the standard library.

`JSL_INCLUDE_HASH_MAP` - Include the hash map macros. See the `HASHMAPS` section
for documentation.

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

<a id="macro-jsl_include_file_utils"></a>
### Macro: `JSL_INCLUDE_FILE_UTILS`

```c
#define JSL_INCLUDE_FILE_UTILS JSL_INCLUDE_FILE_UTILS 1
```

---

<a id="macro-jacks_standard_library"></a>
### Macro: `JACKS_STANDARD_LIBRARY`

```c
#define JACKS_STANDARD_LIBRARY JACKS_STANDARD_LIBRARY
```


*Defined at*: `src/jacks_standard_library.h:70`

---

<a id="macro-jsl_def"></a>
### Macro: `JSL_DEF`

```c
#define JSL_DEF JSL_DEF
```


*Defined at*: `src/jacks_standard_library.h:140`

---

<a id="macro-jsl_default_allocation_alignment"></a>
### Macro: `JSL_DEFAULT_ALLOCATION_ALIGNMENT`

```c
#define JSL_DEFAULT_ALLOCATION_ALIGNMENT JSL_DEFAULT_ALLOCATION_ALIGNMENT 16
```


*Defined at*: `src/jacks_standard_library.h:144`

---

<a id="macro-jsl_warn_unused"></a>
### Macro: `JSL_WARN_UNUSED`

```c
#define JSL_WARN_UNUSED JSL_WARN_UNUSED __attribute__ ( ( warn_unused_result))
```


*Defined at*: `src/jacks_standard_library.h:150`

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


*Defined at*: `src/jacks_standard_library.h:174`

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


*Defined at*: `src/jacks_standard_library.h:192`

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


*Defined at*: `src/jacks_standard_library.h:219`

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


*Defined at*: `src/jacks_standard_library.h:232`

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


*Defined at*: `src/jacks_standard_library.h:246`

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


*Defined at*: `src/jacks_standard_library.h:260`

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


*Defined at*: `src/jacks_standard_library.h:275`

---

<a id="macro-jsl_bytes"></a>
### Macro: `JSL_BYTES`

Macro to simply mark a value as representing a size in bytes. Does nothing with the value.

```c
#define JSL_BYTES JSL_BYTES ( x) x
```


*Defined at*: `src/jacks_standard_library.h:280`

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


*Defined at*: `src/jacks_standard_library.h:295`

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


*Defined at*: `src/jacks_standard_library.h:310`

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


*Defined at*: `src/jacks_standard_library.h:326`

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


*Defined at*: `src/jacks_standard_library.h:345`

---

<a id="macro-jsl_fatptr_literal"></a>
### Macro: `JSL_FATPTR_LITERAL`

Creates a JSLFatPtr from a string literal at compile time.

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


*Defined at*: `src/jacks_standard_library.h:396`

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


*Defined at*: `src/jacks_standard_library.h:405`

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


*Defined at*: `src/jacks_standard_library.h:421`

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
uint8_t buffer[16 * 1024]; // yes this breaks the standard but it's supported everywhere that matters
JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);

IntToStrMap map = int_to_str_ctor(&arena);
int_to_str_add(&map, 64, JSL_FATPTR_LITERAL("This is my string data!"));
}
```

Fast, cheap, easy automatic memory management!

```c
#define JSL_ARENA_FROM_STACK JSL_ARENA_FROM_STACK ( buf) ( JSLArena) { . start = ( uint8_t *) ( buf), . current = ( uint8_t *) ( buf), . end = ( uint8_t *) ( buf) + sizeof ( buf) }
```


*Defined at*: `src/jacks_standard_library.h:466`

---

<a id="macro-jsl_arena_typed_allocate"></a>
### Macro: `JSL_ARENA_TYPED_ALLOCATE`

TODO, docs

```c
#define JSL_ARENA_TYPED_ALLOCATE JSL_ARENA_TYPED_ALLOCATE ( T, arena) ( T *) jsl_arena_allocate ( arena, sizeof ( T), false) . data
```


*Defined at*: `src/jacks_standard_library.h:714`

---

<a id="macro-jsl_format_min_buffer"></a>
### Macro: `JSL_FORMAT_MIN_BUFFER`

```c
#define JSL_FORMAT_MIN_BUFFER JSL_FORMAT_MIN_BUFFER 512
```


*Defined at*: `src/jacks_standard_library.h:817`

---

<a id="type-struct-jslfatptr"></a>
### Struct: `JSLFatPtr`

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


*Defined at*: `src/jacks_standard_library.h:359`

---

<a id="type-typedef-jslfatptr"></a>
### Typedef: `JSLFatPtr`

A fat pointer is a representation of a chunk of memory. It **is not** a container
or an abstract data type.

A fat pointer is very similar to D or Go's slices. This provides several useful
functions like bounds checked reads/writes.

One very important thing to note is that the fat pointer is always defined as mutable.
In my opinion, const in C provides very little protection and a world a headaches during
refactors, especially since C does not have generics or function overloading. I find the
cost benefit analysis to be in the negative.

```c
typedef struct JSLFatPtr JSLFatPtr;
```


*Defined at*: `src/jacks_standard_library.h:379`

---

<a id="type-struct-jslarena"></a>
### Struct: `JSLArena`

TODO: docs

#### Note

The arena API is not thread safe. Arena memory is assumed to live in a
single thread. If you want to share an arena between threads you need to lock.

- `int * start;`
- `int * current;`
- `int * end;`


*Defined at*: `src/jacks_standard_library.h:431`

---

<a id="type-typedef-jslarena"></a>
### Typedef: `JSLArena`

TODO: docs

#### Note

The arena API is not thread safe. Arena memory is assumed to live in a
single thread. If you want to share an arena between threads you need to lock.

```c
typedef struct JSLArena JSLArena;
```


*Defined at*: `src/jacks_standard_library.h:436`

---

<a id="type-struct-jslstringbuilder"></a>
### Struct: `JSLStringBuilder`

A string builder is a container for building large strings. It's specialized for
situations where many different smaller operations result in small strings being
coalesed into a final result, specifically using an arena as its allocator.

While this is called string builder, the underlying data store is just bytes, so
any binary data which is built in chunks can use the string builder.

A string builder is different from a normal dynamic array in two ways. One, it
has specific operations for writing string data in both fat pointer form but also
as a `snprintf` like operation. Two, the resulting string data is not stored as a
contigious range of memory, but as a series of chunks which is given to the user
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
* TODO

- `JSLArena * arena;`
- `struct JSLStringBuilderChunk * head;`
- `struct JSLStringBuilderChunk * tail;`
- `int alignment;`
- `int chunk_size;`


*Defined at*: `src/jacks_standard_library.h:504`

---

<a id="type-struct-jslstringbuilderchunk"></a>
### Struct: `JSLStringBuilderChunk`



*Defined at*: `src/jacks_standard_library.h:507`

---

<a id="type-typedef-jslstringbuilder"></a>
### Typedef: `JSLStringBuilder`

A string builder is a container for building large strings. It's specialized for
situations where many different smaller operations result in small strings being
coalesed into a final result, specifically using an arena as its allocator.

While this is called string builder, the underlying data store is just bytes, so
any binary data which is built in chunks can use the string builder.

A string builder is different from a normal dynamic array in two ways. One, it
has specific operations for writing string data in both fat pointer form but also
as a `snprintf` like operation. Two, the resulting string data is not stored as a
contigious range of memory, but as a series of chunks which is given to the user
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
* TODO

```c
typedef struct JSLStringBuilder JSLStringBuilder;
```


*Defined at*: `src/jacks_standard_library.h:511`

---

<a id="type-struct-jslstringbuilderiterator"></a>
### Struct: `JSLStringBuilderIterator`

- `struct JSLStringBuilderChunk * current;`


*Defined at*: `src/jacks_standard_library.h:514`

---

<a id="type-typedef-jslstringbuilderiterator"></a>
### Typedef: `JSLStringBuilderIterator`

```c
typedef struct JSLStringBuilderIterator JSLStringBuilderIterator;
```


*Defined at*: `src/jacks_standard_library.h:517`

---

<a id="function-jsl_fatptr_init"></a>
### Function: `jsl_fatptr_init`

Constructor utility function to make a fat pointer out of a pointer and a length.
Useful in cases where you can't use C's struct init syntax, like as a parameter
to a function.

```c
JSLFatPtr jsl_fatptr_init(int *, int);
```


*Defined at*: `src/jacks_standard_library.h:525`

---

<a id="function-jsl_fatptr_slice"></a>
### Function: `jsl_fatptr_slice`

Create a new fat pointer that points to the given parameter's data but
with a view of [start, end).

This function is bounds checked. Out of bounds slices will assert.

```c
JSLFatPtr jsl_fatptr_slice(JSLFatPtr, int, int);
```


*Defined at*: `src/jacks_standard_library.h:533`

---

<a id="function-jsl_fatptr_total_write_length"></a>
### Function: `jsl_fatptr_total_write_length`

Utility function to get the total amount of bytes written to the original
fat pointer when compared to a writer fat pointer.

This function checks for NULL and checks that `writer_fatptr` points to data
in `original_fatptr`. If either of these checks fail, then `-1` is returned.

```c
int jsl_fatptr_total_write_length(JSLFatPtr, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:543`

---

<a id="function-jsl_fatptr_auto_slice"></a>
### Function: `jsl_fatptr_auto_slice`

Returns the slice in `original_fatptr` that represents the written to portion, given
the size and pointer in `writer_fatptr`.

#### Note


Example:

```
JSLFatPtr original = jsl_arena_allocate(arena, 128 * 1024 * 1024);
JSLFatPtr writer = original;
jsl_write_file_contents(&writer, "file_one.txt");
jsl_write_file_contents(&writer, "file_two.txt");
JSLFatPtr portion_with_file_data = jsl_fatptr_auto_slice(original, writer);
```

```c
JSLFatPtr jsl_fatptr_auto_slice(JSLFatPtr, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:561`

---

<a id="function-jsl_fatptr_from_cstr"></a>
### Function: `jsl_fatptr_from_cstr`

Build a fat pointer from a null terminated string. **DOES NOT** copy the data.
It simply sets the data pointer to `str` and the length value to the result of
`JSL_STRLEN`.

#### Parameters

**str** — the str to create the fat ptr from


#### Returns

A fat ptr

```c
JSLFatPtr jsl_fatptr_from_cstr(char *);
```


*Defined at*: `src/jacks_standard_library.h:571`

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
int jsl_fatptr_memory_copy(JSLFatPtr *, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:587`

---

<a id="function-jsl_fatptr_cstr_memory_copy"></a>
### Function: `jsl_fatptr_cstr_memory_copy`

Writes the contents of the null terminated string at `cstring` into `buffer`.

This function is bounds checked, meaning a max of `destination->length` bytes
will be copied into `destination`. This function does not check for overlapping
pointers.

If `cstring` is not a valid null terminated string then this function's behavior
is undefined, as it uses `JSL_STRLEN`.

`destination` is modified to point to the remaining data in the buffer. I.E.
if the entire buffer was used then `destination->length` will be `0`.

#### Returns

Number of bytes written or `-1` if `string` or the fat pointer was null.

```c
int jsl_fatptr_cstr_memory_copy(JSLFatPtr *, char *, int);
```


*Defined at*: `src/jacks_standard_library.h:604`

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
int jsl_fatptr_substring_search(JSLFatPtr, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:623`

---

<a id="function-jsl_fatptr_index_of"></a>
### Function: `jsl_fatptr_index_of`

```c
int jsl_fatptr_index_of(JSLFatPtr, int);
```


*Defined at*: `src/jacks_standard_library.h:626`

---

<a id="function-jsl_fatptr_count"></a>
### Function: `jsl_fatptr_count`

```c
int jsl_fatptr_count(JSLFatPtr, int);
```


*Defined at*: `src/jacks_standard_library.h:629`

---

<a id="function-jsl_fatptr_index_of_reverse"></a>
### Function: `jsl_fatptr_index_of_reverse`

```c
int jsl_fatptr_index_of_reverse(JSLFatPtr, int);
```


*Defined at*: `src/jacks_standard_library.h:632`

---

<a id="function-jsl_fatptr_starts_with"></a>
### Function: `jsl_fatptr_starts_with`

```c
int jsl_fatptr_starts_with(JSLFatPtr, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:635`

---

<a id="function-jsl_fatptr_ends_with"></a>
### Function: `jsl_fatptr_ends_with`

```c
int jsl_fatptr_ends_with(JSLFatPtr, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:638`

---

<a id="function-jsl_fatptr_basename"></a>
### Function: `jsl_fatptr_basename`

```c
JSLFatPtr jsl_fatptr_basename(JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:641`

---

<a id="function-jsl_fatptr_get_file_extension"></a>
### Function: `jsl_fatptr_get_file_extension`

```c
JSLFatPtr jsl_fatptr_get_file_extension(JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:644`

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
two password hashes. This function is vulnreble to timing attacks since it bails out
at the first inequality.

```c
int jsl_fatptr_memory_compare(JSLFatPtr, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:662`

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
int jsl_fatptr_cstr_compare(JSLFatPtr, char *);
```


*Defined at*: `src/jacks_standard_library.h:676`

---

<a id="function-jsl_fatptr_compare_ascii_insensitive"></a>
### Function: `jsl_fatptr_compare_ascii_insensitive`

Compare two fatptrs that both contain ASCII data for equality while ignoring case
differences. ASCII data validity is not checked.

#### Returns

true for equals, false for not equal

```c
int jsl_fatptr_compare_ascii_insensitive(JSLFatPtr, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:684`

---

<a id="function-jsl_fatptr_to_lowercase_ascii"></a>
### Function: `jsl_fatptr_to_lowercase_ascii`

Modify the ASCII data in the fatptr in place to change all capital letters to
lowercase. ASCII validity is not checked.

```c
void jsl_fatptr_to_lowercase_ascii(JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:690`

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
int jsl_fatptr_to_int32(JSLFatPtr, int *);
```


*Defined at*: `src/jacks_standard_library.h:702`

---

<a id="function-jsl_arena_init"></a>
### Function: `jsl_arena_init`

```c
void jsl_arena_init(JSLArena *, void *, int);
```


*Defined at*: `src/jacks_standard_library.h:705`

---

<a id="function-jsl_arena_init2"></a>
### Function: `jsl_arena_init2`

```c
void jsl_arena_init2(JSLArena *, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:707`

---

<a id="function-jsl_arena_allocate"></a>
### Function: `jsl_arena_allocate`

```c
JSLFatPtr jsl_arena_allocate(JSLArena *, int, int);
```


*Defined at*: `src/jacks_standard_library.h:709`

---

<a id="function-jsl_arena_allocate_aligned"></a>
### Function: `jsl_arena_allocate_aligned`

```c
JSLFatPtr jsl_arena_allocate_aligned(JSLArena *, int, int, int);
```


*Defined at*: `src/jacks_standard_library.h:711`

---

<a id="function-jsl_arena_reallocate"></a>
### Function: `jsl_arena_reallocate`

Resize the allocation if it was the last allocation, otherwise, allocate a new
chunk of memory.

In debug mode, this function will set `original_allocation->data` to null to
help detect stale pointer bugs.

```c
JSLFatPtr jsl_arena_reallocate(JSLArena *, JSLFatPtr, int);
```


*Defined at*: `src/jacks_standard_library.h:723`

---

<a id="function-jsl_arena_reallocate_aligned"></a>
### Function: `jsl_arena_reallocate_aligned`

Resize the allocation if it was the last allocation, otherwise, allocate a new
chunk of memory.

In debug mode, this function will set `original_allocation->data` to null to
help detect stale pointer bugs.

```c
JSLFatPtr jsl_arena_reallocate_aligned(JSLArena *, JSLFatPtr, int, int);
```


*Defined at*: `src/jacks_standard_library.h:736`

---

<a id="function-jsl_arena_reset"></a>
### Function: `jsl_arena_reset`

Set the current pointer back to the start of the arena.

In debug mode, this function will set all of the memory that was
allocated to `0xfeeefeee` to help detect use after free bugs.

```c
void jsl_arena_reset(JSLArena *);
```


*Defined at*: `src/jacks_standard_library.h:749`

---

<a id="function-jsl_arena_save_restore_point"></a>
### Function: `jsl_arena_save_restore_point`

The functions `jsl_arena_save_restore_point` and `jsl_arena_load_restore_point`
help you make temporary allocations inside an existing arena. You can think of
it as an "arena inside an arena"

For example, say you have an existing one megabyte arena that has used 128 kilobytes
of space. You then call a function with this arena which needs a string to make an
operating system call, but that string is no longer needed after the function returns.
You can "save" and "load" a restore point at the start and end of the function
(respectively) and when the function returns, the arena will still only have 128
kilobytes used.

In debug mode, jsl_arena_load_restore_point function will set all of the memory
that was allocated to `0xfeeefeee` to help detect use after free bugs.

```c
int * jsl_arena_save_restore_point(JSLArena *);
```


*Defined at*: `src/jacks_standard_library.h:766`

---

<a id="function-jsl_arena_load_restore_point"></a>
### Function: `jsl_arena_load_restore_point`

See the docs for `jsl_arena_load_restore_point`.

```c
void jsl_arena_load_restore_point(JSLArena *, int *);
```


*Defined at*: `src/jacks_standard_library.h:771`

---

<a id="function-jsl_arena_fatptr_to_cstr"></a>
### Function: `jsl_arena_fatptr_to_cstr`

Allocate and copy the contents of a fat pointer with a null terminator.

```c
char * jsl_arena_fatptr_to_cstr(JSLArena *, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:776`

---

<a id="function-jsl_arena_cstr_to_fatptr"></a>
### Function: `jsl_arena_cstr_to_fatptr`

Allocate and copy the contents of a fat pointer with a null terminator.

#### Note

Use `jsl_fatptr_from_cstr` to make a fat pointer without copying.

```c
JSLFatPtr jsl_arena_cstr_to_fatptr(JSLArena *, char *);
```


*Defined at*: `src/jacks_standard_library.h:783`

---

<a id="function-jsl_fatptr_duplicate"></a>
### Function: `jsl_fatptr_duplicate`

Allocate space for, and copy the contents of a fat pointer.

#### Note

Use `jsl_arena_cstr_to_fatptr` to copy a c string into a fatptr.

```c
JSLFatPtr jsl_fatptr_duplicate(JSLArena *, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:790`

---

<a id="function-jsl_string_builder_init"></a>
### Function: `jsl_string_builder_init`

```c
int jsl_string_builder_init(JSLStringBuilder *, JSLArena *);
```


*Defined at*: `src/jacks_standard_library.h:793`

---

<a id="function-jsl_string_builder_init2"></a>
### Function: `jsl_string_builder_init2`

```c
int jsl_string_builder_init2(JSLStringBuilder *, JSLArena *, int, int);
```


*Defined at*: `src/jacks_standard_library.h:796`

---

<a id="function-jsl_string_builder_insert_char"></a>
### Function: `jsl_string_builder_insert_char`

```c
int jsl_string_builder_insert_char(JSLStringBuilder *, char);
```


*Defined at*: `src/jacks_standard_library.h:799`

---

<a id="function-jsl_string_builder_insert_uint8_t"></a>
### Function: `jsl_string_builder_insert_uint8_t`

```c
int jsl_string_builder_insert_uint8_t(JSLStringBuilder *, int);
```


*Defined at*: `src/jacks_standard_library.h:802`

---

<a id="function-jsl_string_builder_insert_fatptr"></a>
### Function: `jsl_string_builder_insert_fatptr`

```c
int jsl_string_builder_insert_fatptr(JSLStringBuilder *, JSLFatPtr);
```


*Defined at*: `src/jacks_standard_library.h:805`

---

<a id="function-jsl_string_builder_format"></a>
### Function: `jsl_string_builder_format`

```c
int jsl_string_builder_format(JSLStringBuilder *, JSLFatPtr, ...);
```


*Defined at*: `src/jacks_standard_library.h:808`

---

<a id="function-jsl_string_builder_iterator_init"></a>
### Function: `jsl_string_builder_iterator_init`

```c
void jsl_string_builder_iterator_init(JSLStringBuilder *, JSLStringBuilderIterator *);
```


*Defined at*: `src/jacks_standard_library.h:811`

---

<a id="function-jsl_string_builder_iterator_next"></a>
### Function: `jsl_string_builder_iterator_next`

```c
JSLFatPtr jsl_string_builder_iterator_next(JSLStringBuilderIterator *);
```


*Defined at*: `src/jacks_standard_library.h:814`

---

<a id="type-typedef-jsl_format_callback"></a>
### Typedef: `JSL_FORMAT_CALLBACK`

```c
typedef int *(int *, void *, int) JSL_FORMAT_CALLBACK;
```


*Defined at*: `src/jacks_standard_library.h:820`

---

<a id="function-jsl_format"></a>
### Function: `jsl_format`

This is a full snprintf replacement that supports everything that the C
runtime snprintf supports, including float/double, 64-bit integers, hex
floats, field parameters (%*.*d stuff), length reads backs, etc.

This returns the number of bytes written.

There are a set of different functions for different use cases

* jsl_format
* jsl_format_buffer
* jsl_format_valist
* jsl_format_callback
* jsl_string_builder_format

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
JSLFatPtr jsl_format(JSLArena *, JSLFatPtr, ...);
```


*Defined at*: `src/jacks_standard_library.h:885`

---

<a id="function-jsl_format_buffer"></a>
### Function: `jsl_format_buffer`

See docs for jsl_format.

Writes into a provided buffer, up to `buffer.length` bytes.

```c
int jsl_format_buffer(JSLFatPtr *, JSLFatPtr, ...);
```


*Defined at*: `src/jacks_standard_library.h:892`

---

<a id="function-jsl_format_valist"></a>
### Function: `jsl_format_valist`

See docs for jsl_format.

Writes into a provided buffer, up to `buffer.length` bytes using a variadic
argument list.

```c
int jsl_format_valist(JSLFatPtr *, JSLFatPtr, int);
```


*Defined at*: `src/jacks_standard_library.h:904`

---

<a id="function-jsl_format_callback"></a>
### Function: `jsl_format_callback`

See docs for jsl_format.

Convert into a buffer, calling back every JSL_FORMAT_MIN_BUFFER chars.
Your callback can then copy the chars out, print them or whatever.
This function is actually the workhorse for everything else.
The buffer you pass in must hold at least JSL_FORMAT_MIN_BUFFER characters.

You return the next buffer to use or 0 to stop converting

```c
int jsl_format_callback(JSL_FORMAT_CALLBACK *, void *, int *, JSLFatPtr, int);
```


*Defined at*: `src/jacks_standard_library.h:920`

---

<a id="function-jsl_format_set_separators"></a>
### Function: `jsl_format_set_separators`

Set the comma and period characters to use for the current thread.

```c
void jsl_format_set_separators(char, char);
```


*Defined at*: `src/jacks_standard_library.h:932`

---

<a id="type-enum-jslloadfileresultenum"></a>
### Enum: `JSLLoadFileResultEnum`

- `JSL_FILE_LOAD_BAD_PARAMETERS = 0`
- `JSL_FILE_LOAD_SUCCESS = 1`
- `JSL_FILE_LOAD_COULD_NOT_OPEN = 2`
- `JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE = 3`
- `JSL_FILE_LOAD_COULD_NOT_GET_MEMORY = 4`
- `JSL_FILE_LOAD_READ_FAILED = 5`
- `JSL_FILE_LOAD_CLOSE_FAILED = 6`
- `JSL_FILE_LOAD_ERROR_UNKNOWN = 7`
- `JSL_FILE_LOAD_ENUM_COUNT = 8`


*Defined at*: `src/jacks_standard_library.h:962`

---

<a id="type-typedef-jslloadfileresultenum"></a>
### Typedef: `JSLLoadFileResultEnum`

```c
typedef enum JSLLoadFileResultEnum JSLLoadFileResultEnum;
```


*Defined at*: `src/jacks_standard_library.h:975`

---

<a id="type-enum-jslwritefileresultenum"></a>
### Enum: `JSLWriteFileResultEnum`

- `JSL_FILE_WRITE_BAD_PARAMETERS = 0`
- `JSL_FILE_WRITE_SUCCESS = 1`
- `JSL_FILE_WRITE_COULD_NOT_OPEN = 2`
- `JSL_FILE_WRITE_COULD_NOT_WRITE = 3`
- `JSL_FILE_WRITE_COULD_NOT_CLOSE = 4`
- `JSL_FILE_WRITE_ENUM_COUNT = 5`


*Defined at*: `src/jacks_standard_library.h:977`

---

<a id="type-typedef-jslwritefileresultenum"></a>
### Typedef: `JSLWriteFileResultEnum`

```c
typedef enum JSLWriteFileResultEnum JSLWriteFileResultEnum;
```


*Defined at*: `src/jacks_standard_library.h:987`

---

<a id="type-enum-jslfiletypeenum"></a>
### Enum: `JSLFileTypeEnum`

- `JSL_FILE_TYPE_UNKNOWN = 0`
- `JSL_FILE_TYPE_REG = 1`
- `JSL_FILE_TYPE_DIR = 2`
- `JSL_FILE_TYPE_SYMLINK = 3`
- `JSL_FILE_TYPE_BLOCK = 4`
- `JSL_FILE_TYPE_CHAR = 5`
- `JSL_FILE_TYPE_FIFO = 6`
- `JSL_FILE_TYPE_SOCKET = 7`
- `JSL_FILE_TYPE_COUNT = 8`


*Defined at*: `src/jacks_standard_library.h:989`

---

<a id="type-typedef-jslfiletypeenum"></a>
### Typedef: `JSLFileTypeEnum`

```c
typedef enum JSLFileTypeEnum JSLFileTypeEnum;
```


*Defined at*: `src/jacks_standard_library.h:1000`

---

<a id="function-jsl_load_file_contents"></a>
### Function: `jsl_load_file_contents`

Load the contents of the file at `path` into a newly allocated buffer
from the given arena. The buffer will be the exact size of the file contents.

If the arena does not have enough space,

```c
JSLLoadFileResultEnum jsl_load_file_contents(JSLArena *, JSLFatPtr, JSLFatPtr *, int *);
```


*Defined at*: `src/jacks_standard_library.h:1008`

---

<a id="function-jsl_load_file_contents_buffer"></a>
### Function: `jsl_load_file_contents_buffer`

```c
JSLLoadFileResultEnum jsl_load_file_contents_buffer(JSLFatPtr *, JSLFatPtr, int *);
```


*Defined at*: `src/jacks_standard_library.h:1016`

---

<a id="function-jsl_write_file_contents"></a>
### Function: `jsl_write_file_contents`

```c
JSLWriteFileResultEnum jsl_write_file_contents(JSLFatPtr, JSLFatPtr, int *, int *);
```


*Defined at*: `src/jacks_standard_library.h:1023`

---

<a id="function-jsl_format_file"></a>
### Function: `jsl_format_file`

```c
int jsl_format_file(int *, JSLFatPtr, ...);
```


*Defined at*: `src/jacks_standard_library.h:1031`

---


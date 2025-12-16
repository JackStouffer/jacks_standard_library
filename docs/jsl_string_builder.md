# API Documentation

## Macros

- [`JSL_STRING_BUILDER_H_INCLUDED`](#macro-jsl_string_builder_h_included)
- [`JSL_STRING_BUILDER_VERSION`](#macro-jsl_string_builder_version)
- [`JSL_STRING_BUILDER_DEF`](#macro-jsl_string_builder_def)

## Types

- [`JSLStringBuilder`](#type-jslstringbuilder)
- [`JSLStringBuilderChunk`](#type-jslstringbuilderchunk)
- [`JSLStringBuilderIterator`](#type-jslstringbuilderiterator)

## Functions

- [`jsl_string_builder_init`](#function-jsl_string_builder_init)
- [`jsl_string_builder_init2`](#function-jsl_string_builder_init2)
- [`jsl_string_builder_insert_char`](#function-jsl_string_builder_insert_char)
- [`jsl_string_builder_insert_uint8_t`](#function-jsl_string_builder_insert_uint8_t)
- [`jsl_string_builder_insert_fatptr`](#function-jsl_string_builder_insert_fatptr)
- [`jsl_string_builder_format`](#function-jsl_string_builder_format)
- [`jsl_string_builder_iterator_init`](#function-jsl_string_builder_iterator_init)
- [`jsl_string_builder_iterator_next`](#function-jsl_string_builder_iterator_next)

## File: src/jsl_string_builder.h

## JSL String Builder

A string builder is a container for building large strings. It's specialized for
situations where many different smaller operations result in small strings being
coalesced into a final result, specifically using an arena as its allocator.

While this is called string builder, the underlying data store is just bytes, so
any binary data which is built in chunks can use the string builder.

### Implementation

A string builder is different from a normal dynamic array in two ways. One, it
has specific operations for writing string data in both fat pointer form but also
as a `snprintf` like operation. Two, the resulting string data is not stored as a
contiguous range of memory, but as a series of chunks which is given to the user
as an iterator when the string is finished.

This is due to the nature of arena allocations. If you have some part of your
program which generates string output, the most common form of that code would be:

1. You do some operations, these operations themselves allocate
2. You generate a string from the operations
3. The string is concatenated into some buffer
4. Repeat

A dynamically sized array which grows would mean throwing away the old memory when
the array resizes. This would be fine for your typical heap but for an arena this
the old memory is unavailable until the arena is reset. A separate arena that's
used purely for the array would work, but that sort of defeats the whole purpose
of an arena, which is it's supposed to make lifetime tracking easier. Having a
whole bunch of separate arenas for different objects makes the program more
complicated than it should be.

Having the memory in chunks means that a single arena is not wasteful with its
available memory.

By default, each chunk is 256 bytes and is aligned to a 8 byte address. These are
tuneable parameters that you can set during init. The custom alignment helps if you
want to use SIMD code on the consuming code.

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

<a id="macro-jsl_string_builder_h_included"></a>
### Macro: `JSL_STRING_BUILDER_H_INCLUDED`

```c
#define JSL_STRING_BUILDER_H_INCLUDED JSL_STRING_BUILDER_H_INCLUDED
```


*Defined at*: `src/jsl_string_builder.h:66`

---

<a id="macro-jsl_string_builder_version"></a>
### Macro: `JSL_STRING_BUILDER_VERSION`

```c
#define JSL_STRING_BUILDER_VERSION JSL_STRING_BUILDER_VERSION 0x010000
```


*Defined at*: `src/jsl_string_builder.h:79`

---

<a id="macro-jsl_string_builder_def"></a>
### Macro: `JSL_STRING_BUILDER_DEF`

```c
#define JSL_STRING_BUILDER_DEF JSL_STRING_BUILDER_DEF
```


*Defined at*: `src/jsl_string_builder.h:87`

---

<a id="type-jslstringbuilder"></a>
### : `JSLStringBuilder`

Container type for the string builder. See the top level docstring for an overview.

This container holds a reference to the arena used, so it must have a lifetime greater
or equal to the string builder.

## Functions

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


*Defined at*: `src/jsl_string_builder.h:109`

---

<a id="type-jslstringbuilderchunk"></a>
### : `JSLStringBuilderChunk`



*Defined at*: `src/jsl_string_builder.h:112`

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


*Defined at*: `src/jsl_string_builder.h:129`

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


*Defined at*: `src/jsl_string_builder.h:143`

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


*Defined at*: `src/jsl_string_builder.h:156`

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


*Defined at*: `src/jsl_string_builder.h:167`

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


*Defined at*: `src/jsl_string_builder.h:179`

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


*Defined at*: `src/jsl_string_builder.h:189`

---

<a id="function-jsl_string_builder_format"></a>
### Function: `jsl_string_builder_format`

Format a string using the jsl_format logic and write the result directly into
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


*Defined at*: `src/jsl_string_builder.h:200`

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


*Defined at*: `src/jsl_string_builder.h:217`

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

jsl_format_file(stdout, str);
}
```

#### Parameters

**iterator** — The iterator instance



#### Returns

The next chunk of data from the string builder

```c
JSLFatPtr jsl_string_builder_iterator_next(JSLStringBuilderIterator *iterator);
```


*Defined at*: `src/jsl_string_builder.h:247`

---


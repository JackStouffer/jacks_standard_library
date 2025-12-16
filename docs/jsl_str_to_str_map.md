# API Documentation

## Macros

- [`JSL_STR_TO_STR_MAP_H_INCLUDED`](#macro-jsl_str_to_str_map_h_included)
- [`JSL_STR_TO_STR_MAP_VERSION`](#macro-jsl_str_to_str_map_version)
- [`JSL_STR_TO_STR_MAP_DEF`](#macro-jsl_str_to_str_map_def)

## Types

- [`JSLStrToStrMap`](#type-typedef-jslstrtostrmap)
- [`JSLStrToStrMapKeyValueIter`](#type-typedef-jslstrtostrmapkeyvalueiter)

## Functions

- [`jsl_str_to_str_map_init`](#function-jsl_str_to_str_map_init)
- [`jsl_str_to_str_map_init2`](#function-jsl_str_to_str_map_init2)
- [`jsl_str_to_str_map_item_count`](#function-jsl_str_to_str_map_item_count)
- [`jsl_str_to_str_map_has_key`](#function-jsl_str_to_str_map_has_key)
- [`jsl_str_to_str_map_insert`](#function-jsl_str_to_str_map_insert)
- [`jsl_str_to_str_map_get`](#function-jsl_str_to_str_map_get)
- [`jsl_str_to_str_map_key_value_iterator_init`](#function-jsl_str_to_str_map_key_value_iterator_init)
- [`jsl_str_to_str_map_key_value_iterator_next`](#function-jsl_str_to_str_map_key_value_iterator_next)
- [`jsl_str_to_str_map_delete`](#function-jsl_str_to_str_map_delete)
- [`jsl_str_to_str_map_clear`](#function-jsl_str_to_str_map_clear)

## File: src/jsl_str_to_str_map.h

## JSL String to String Map

This file is a single header file library that implements a hash map data
structure, which maps length based string keys to length based string values,
and is optimized around the arena allocator design. This file is part of
the Jack's Standard Library project.

### Documentation

See `docs/jsl_str_to_str_map.md` for a formatted documentation page.

### Usage

1. Copy the `jsl_str_to_str_map.h` file into your repo
2. Include the header like normally in each source file where you use it:

```c
#include "jsl_str_to_str_map.h"
```

3. Then, in ONE AND ONLY ONE file, do this:

```c
#define JSL_STR_TO_STR_MAP_IMPLEMENTATION
#include "jsl_str_to_str_map.h"
```

**IMPORTANT**: The map also requires that the implementation of
`jsl_core.h` be available at link time.

The implementation should probably be in the same file as your entrypoint function,
but it doesn't have to be. It's also common to put this into an otherwise
empty file for easier integration to standard C/C++ build systems.

### Caveats

This map uses arenas, so some wasted memory is indeveatble. Care has
been taken to reuse as much allocated memory as possible. But if your
map is long lived it's possible to start exhausting the arena with
old memory.

Remember to

* have an initial item count guess as accurate as you can to reduce rehashes
* have the arena have as short a lifetime as possible

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

<a id="macro-jsl_str_to_str_map_h_included"></a>
### Macro: `JSL_STR_TO_STR_MAP_H_INCLUDED`

```c
#define JSL_STR_TO_STR_MAP_H_INCLUDED JSL_STR_TO_STR_MAP_H_INCLUDED
```


*Defined at*: `src/jsl_str_to_str_map.h:72`

---

<a id="macro-jsl_str_to_str_map_version"></a>
### Macro: `JSL_STR_TO_STR_MAP_VERSION`

```c
#define JSL_STR_TO_STR_MAP_VERSION JSL_STR_TO_STR_MAP_VERSION 0x010000
```


*Defined at*: `src/jsl_str_to_str_map.h:85`

---

<a id="macro-jsl_str_to_str_map_def"></a>
### Macro: `JSL_STR_TO_STR_MAP_DEF`

```c
#define JSL_STR_TO_STR_MAP_DEF JSL_STR_TO_STR_MAP_DEF
```


*Defined at*: `src/jsl_str_to_str_map.h:93`

---

<a id="type-typedef-jslstrtostrmap"></a>
### Typedef: `JSLStrToStrMap`

This is an open addressed hash map with linear probing that maps
JSLFatPtr keys to JSLFatPtr values. This map uses rapidhash, which
is a avalanche hash with a configurable seed value for protection
against hash flooding attacks.

Example:

```
uint8_t buffer[JSL_KILOBYTES(16)];
JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);

JSLStrToStrMap map;
jsl_str_to_str_map_init(&map, &stack_arena, 0);

JSLFatPtr key = JSL_FATPTR_INITIALIZER("hello-key");

jsl_str_to_str_multimap_insert(
&map,
key,
JSL_STRING_LIFETIME_STATIC,
JSL_FATPTR_EXPRESSION("hello-value"),
JSL_STRING_LIFETIME_STATIC
);

JSLFatPtr value
jsl_str_to_str_map_get(&map, key, &value);
```

## Functions

* [jsl_str_to_str_map_init](#function-jsl_str_to_str_map_init)
* [jsl_str_to_str_map_init2](#function-jsl_str_to_str_map_init2)
* [jsl_str_to_str_map_item_count](#function-jsl_str_to_str_map_item_count)
* [jsl_str_to_str_map_has_key](#function-jsl_str_to_str_map_has_key)
* [jsl_str_to_str_map_insert](#function-jsl_str_to_str_map_insert)
* [jsl_str_to_str_map_get](#function-jsl_str_to_str_map_get)
* [jsl_str_to_str_map_key_value_iterator_init](#function-jsl_str_to_str_map_key_value_iterator_init)
* [jsl_str_to_str_map_key_value_iterator_next](#function-jsl_str_to_str_map_key_value_iterator_next)
* [jsl_str_to_str_map_delete](#function-jsl_str_to_str_map_delete)
* [jsl_str_to_str_map_clear](#function-jsl_str_to_str_map_clear)

```c
typedef struct JSL__StrToStrMap JSLStrToStrMap;
```


*Defined at*: `src/jsl_str_to_str_map.h:143`

---

<a id="type-typedef-jslstrtostrmapkeyvalueiter"></a>
### Typedef: `JSLStrToStrMapKeyValueIter`

State tracking struct for iterating over all of the keys and values
in the map.

#### Note

If you mutate the map this iterator is automatically invalidated
and any operations on this iterator will terminate with failure return
values.

## Functions

* [jsl_str_to_str_map_key_value_iterator_init](#function-jsl_str_to_str_map_key_value_iterator_init)
* [jsl_str_to_str_map_key_value_iterator_next](#function-jsl_str_to_str_map_key_value_iterator_next)

```c
typedef struct JSL__StrToStrMapKeyValueIter JSLStrToStrMapKeyValueIter;
```


*Defined at*: `src/jsl_str_to_str_map.h:158`

---

<a id="function-jsl_str_to_str_map_init"></a>
### Function: `jsl_str_to_str_map_init`

Initialize a map with default sizing parameters.

This sets up internal tables in the provided arena, using a 32 entry
initial capacity guess and a 0.75 load factor. The `seed` value is to
protect against hash flooding attacks. If you're absolutely sure this
map cannot be attacked, then zero is valid seed value.

#### Parameters

**map** — Pointer to the map to initialize.

**arena** — Arena used for all allocations.

**seed** — Arbitrary seed value for hashing.



#### Returns

`true` on success, `false` if any parameter is invalid or out of memory.

```c
int jsl_str_to_str_map_init(JSLStrToStrMap *map, JSLArena *arena, int seed);
```


*Defined at*: `src/jsl_str_to_str_map.h:173`

---

<a id="function-jsl_str_to_str_map_init2"></a>
### Function: `jsl_str_to_str_map_init2`

Initialize a map with explicit sizing parameters.

This is identical to `jsl_str_to_str_map_init`, but lets callers
provide an initial `item_count_guess` and a `load_factor`. The initial
lookup table is sized to the next power of two above `item_count_guess`,
clamped to at least 32 entries. `load_factor` must be in the range
`(0.0f, 1.0f)` and controls when the table rehashes. The `seed` value
is to protect against hash flooding attacks. If you're absolutely sure 
this map cannot be attacked, then zero is valid seed value

#### Parameters

**map** — Pointer to the map to initialize.

**arena** — Arena used for all allocations.

**seed** — Arbitrary seed value for hashing.

**item_count_guess** — Expected max number of keys

**load_factor** — Desired load factor before rehashing



#### Returns

`true` on success, `false` if any parameter is invalid or out of memory.

```c
int jsl_str_to_str_map_init2(JSLStrToStrMap *map, JSLArena *arena, int seed, int item_count_guess, float load_factor);
```


*Defined at*: `src/jsl_str_to_str_map.h:197`

---

<a id="function-jsl_str_to_str_map_item_count"></a>
### Function: `jsl_str_to_str_map_item_count`

Get the number of items currently stored.

#### Parameters

**map** — Pointer to the map.



#### Returns

Key count, or `-1` on error

```c
int jsl_str_to_str_map_item_count(JSLStrToStrMap *map);
```


*Defined at*: `src/jsl_str_to_str_map.h:211`

---

<a id="function-jsl_str_to_str_map_has_key"></a>
### Function: `jsl_str_to_str_map_has_key`

Does the map have the given key.

#### Parameters

**map** — Pointer to the map.



#### Returns

`true` if yes, `false` if no or error

```c
int jsl_str_to_str_map_has_key(JSLStrToStrMap *map, JSLFatPtr key);
```


*Defined at*: `src/jsl_str_to_str_map.h:221`

---

<a id="function-jsl_str_to_str_map_insert"></a>
### Function: `jsl_str_to_str_map_insert`

Insert a key/value pair.

#### Parameters

**map** — Map to mutate.

**key** — Key to insert.

**key_lifetime** — Lifetime semantics for the key data.

**value** — Value to insert.

**value_lifetime** — Lifetime semantics for the value data.



#### Returns

`true` on success, `false` on invalid parameters or OOM.

```c
int jsl_str_to_str_map_insert(JSLStrToStrMap *map, JSLFatPtr key, JSLStringLifeTime key_lifetime, JSLFatPtr value, JSLStringLifeTime value_lifetime);
```


*Defined at*: `src/jsl_str_to_str_map.h:236`

---

<a id="function-jsl_str_to_str_map_get"></a>
### Function: `jsl_str_to_str_map_get`

Get the value of the key.

#### Parameters

**map** — Map to search.

**key** — Key to search for.

**out_value** — Output parameter that will be filled with the value if successful



#### Returns

A bool indicating success or failure

```c
int jsl_str_to_str_map_get(JSLStrToStrMap *map, JSLFatPtr key, JSLFatPtr *out_value);
```


*Defined at*: `src/jsl_str_to_str_map.h:252`

---

<a id="function-jsl_str_to_str_map_key_value_iterator_init"></a>
### Function: `jsl_str_to_str_map_key_value_iterator_init`

Initialize an iterator that visits every key/value pair in the map.

Example:

```
JSLStrToStrMapKeyValueIter iter;
jsl_str_to_str_map_key_value_iterator_init(
&map, &iter
);

JSLFatPtr key;
JSLFatPtr value;
while (jsl_str_to_str_map_key_value_iterator_next(&iter, &key, &value))
{
...
}
```

Overall traversal order is undefined. The iterator is invalidated
if the map is mutated after initialization.

#### Parameters

**map** — Map to iterate over; must be initialized.

**iterator** — Iterator instance to initialize.



#### Returns

`true` on success, `false` if parameters are invalid.

```c
int jsl_str_to_str_map_key_value_iterator_init(JSLStrToStrMap *map, JSLStrToStrMapKeyValueIter *iterator);
```


*Defined at*: `src/jsl_str_to_str_map.h:284`

---

<a id="function-jsl_str_to_str_map_key_value_iterator_next"></a>
### Function: `jsl_str_to_str_map_key_value_iterator_next`

Advance the key/value iterator.

Example:

```
JSLStrToStrMapKeyValueIter iter;
jsl_str_to_str_map_key_value_iterator_init(
&map, &iter
);

JSLFatPtr key;
JSLFatPtr value;
while (jsl_str_to_str_map_key_value_iterator_next(&iter, &key, &value))
{
...
}
```

Returns the next key/value pair for the map. The iterator must be
initialized and is invalidated if the map is mutated; iteration order
is undefined.

#### Parameters

**iterator** — Iterator to advance.

**out_key** — Output for the current key.

**out_value** — Output for the current value.



#### Returns

`true` if a pair was produced, `false` if exhausted or invalid.

```c
int jsl_str_to_str_map_key_value_iterator_next(JSLStrToStrMapKeyValueIter *iterator, JSLFatPtr *out_key, JSLFatPtr *out_value);
```


*Defined at*: `src/jsl_str_to_str_map.h:317`

---

<a id="function-jsl_str_to_str_map_delete"></a>
### Function: `jsl_str_to_str_map_delete`

Remove a key/value.

Iterators become invalid. If the key is not present or parameters are invalid,
the map is unchanged and `false` is returned.

#### Parameters

**map** — Map to mutate.

**key** — Key to remove.



#### Returns

`true` if the key existed and was removed, `false` otherwise.

```c
int jsl_str_to_str_map_delete(JSLStrToStrMap *map, JSLFatPtr key);
```


*Defined at*: `src/jsl_str_to_str_map.h:333`

---

<a id="function-jsl_str_to_str_map_clear"></a>
### Function: `jsl_str_to_str_map_clear`

Remove all keys and values from the map.  Iterators become invalid.

#### Parameters

**map** — Map to clear.

```c
void jsl_str_to_str_map_clear(JSLStrToStrMap *map);
```


*Defined at*: `src/jsl_str_to_str_map.h:343`

---


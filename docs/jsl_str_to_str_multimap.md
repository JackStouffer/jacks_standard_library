# API Documentation

## Macros

- [`JSL_STR_TO_STR_MULTIMAP_VERSION`](#macro-jsl_str_to_str_multimap_version)
- [`JSL_STR_TO_STR_MULTIMAP_DEF`](#macro-jsl_str_to_str_multimap_def)

## Types

- [`JSLStrToStrMultimapKeyState`](#type-jslstrtostrmultimapkeystate)
- [`union (unnamed at /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:86:5)`](#type-union-unnamed-at-users-jackstouffer-documents-code-jacks_standard_library-src-jsl_str_to_str_multimap-h-86-5)
- [`struct (unnamed at /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:88:9)`](#type-struct-unnamed-at-users-jackstouffer-documents-code-jacks_standard_library-src-jsl_str_to_str_multimap-h-88-9)
- [`union (unnamed at /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:101:5)`](#type-union-unnamed-at-users-jackstouffer-documents-code-jacks_standard_library-src-jsl_str_to_str_multimap-h-101-5)
- [`struct (unnamed at /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:103:9)`](#type-struct-unnamed-at-users-jackstouffer-documents-code-jacks_standard_library-src-jsl_str_to_str_multimap-h-103-9)
- [`JSLStrToStrMultimap`](#type-typedef-jslstrtostrmultimap)
- [`JSLStrToStrMultimapKeyValueIter`](#type-typedef-jslstrtostrmultimapkeyvalueiter)
- [`JSLStrToStrMultimapValueIter`](#type-typedef-jslstrtostrmultimapvalueiter)

## Functions

- [`jsl_str_to_str_multimap_init`](#function-jsl_str_to_str_multimap_init)
- [`jsl_str_to_str_multimap_init2`](#function-jsl_str_to_str_multimap_init2)
- [`jsl_str_to_str_multimap_get_key_count`](#function-jsl_str_to_str_multimap_get_key_count)
- [`jsl_str_to_str_multimap_get_value_count`](#function-jsl_str_to_str_multimap_get_value_count)
- [`jsl_str_to_str_multimap_has_key`](#function-jsl_str_to_str_multimap_has_key)
- [`jsl_str_to_str_multimap_insert`](#function-jsl_str_to_str_multimap_insert)
- [`jsl_str_to_str_multimap_get_value_count_for_key`](#function-jsl_str_to_str_multimap_get_value_count_for_key)
- [`jsl_str_to_str_multimap_key_value_iterator_init`](#function-jsl_str_to_str_multimap_key_value_iterator_init)
- [`jsl_str_to_str_multimap_key_value_iterator_next`](#function-jsl_str_to_str_multimap_key_value_iterator_next)
- [`jsl_str_to_str_multimap_get_values_for_key_iterator_init`](#function-jsl_str_to_str_multimap_get_values_for_key_iterator_init)
- [`jsl_str_to_str_multimap_get_values_for_key_iterator_next`](#function-jsl_str_to_str_multimap_get_values_for_key_iterator_next)
- [`jsl_str_to_str_multimap_delete_key`](#function-jsl_str_to_str_multimap_delete_key)
- [`jsl_str_to_str_multimap_delete_value`](#function-jsl_str_to_str_multimap_delete_value)
- [`jsl_str_to_str_multimap_clear`](#function-jsl_str_to_str_multimap_clear)

## File: /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h

## JSL String to String Multimap

This file is a single header file library that implements a multimap data
structure, which maps length based string keys to multiple length based
string values, and is optimized around the arena allocator design.
This file is part of the Jack's Standard Library project.

### Documentation

See `docs/jsl_str_to_str_multimap.md` for a formatted documentation page.

### Caveats

This multimap uses arenas, so some wasted memory is indeveatble. Care has
been taken to reuse as much allocated memory as possible. But if your
multimap is long lived it's possible to start exhausting the arena with
old memory.

Remember to

* have an initial item count guess as accurate as you can to reduce rehashes
* have the arena have as short a lifetime as possible

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

<a id="macro-jsl_str_to_str_multimap_version"></a>
### Macro: `JSL_STR_TO_STR_MULTIMAP_VERSION`

```c
#define JSL_STR_TO_STR_MULTIMAP_VERSION JSL_STR_TO_STR_MULTIMAP_VERSION 0x010000
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:61`

---

<a id="macro-jsl_str_to_str_multimap_def"></a>
### Macro: `JSL_STR_TO_STR_MULTIMAP_DEF`

```c
#define JSL_STR_TO_STR_MULTIMAP_DEF JSL_STR_TO_STR_MULTIMAP_DEF
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:69`

---

<a id="type-jslstrtostrmultimapkeystate"></a>
### : `JSLStrToStrMultimapKeyState`

- `JSL__MULTIMAP_EMPTY = 1`
- `JSL__MULTIMAP_TOMBSTONE = 2`


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:76`

---

<a id="type-union-unnamed-at-users-jackstouffer-documents-code-jacks_standard_library-src-jsl_str_to_str_multimap-h-86-5"></a>
### : `union (unnamed at /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:86:5)`

- `JSLFatPtr value;`


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:86`

---

<a id="type-struct-unnamed-at-users-jackstouffer-documents-code-jacks_standard_library-src-jsl_str_to_str_multimap-h-88-9"></a>
### : `struct (unnamed at /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:88:9)`

- `int64_t sso_len;`
- `int[32] small_string_buffer;`


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:88`

---

<a id="type-union-unnamed-at-users-jackstouffer-documents-code-jacks_standard_library-src-jsl_str_to_str_multimap-h-101-5"></a>
### : `union (unnamed at /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:101:5)`

- `JSLFatPtr key;`
- `struct JSL__StrToStrMultimapEntry * next;`


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:101`

---

<a id="type-struct-unnamed-at-users-jackstouffer-documents-code-jacks_standard_library-src-jsl_str_to_str_multimap-h-103-9"></a>
### : `struct (unnamed at /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:103:9)`

- `int[24] small_string_buffer;`
- `int64_t sso_len;`


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:103`

---

<a id="type-typedef-jslstrtostrmultimap"></a>
### Typedef: `JSLStrToStrMultimap`

This is an open addressed, hash based multimap with linear probing that maps
JSLFatPtr keys to multiple JSLFatPtr values.

Example:

```
uint8_t buffer[JSL_KILOBYTES(16)];
JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);

JSLStrToStrMultimap map;
jsl_str_to_str_multimap_init(&map, &stack_arena, 0);

JSLFatPtr key = JSL_FATPTR_INITIALIZER("hello-key");

jsl_str_to_str_multimap_insert(
&map,
key,
JSL_STRING_LIFETIME_STATIC,
JSL_FATPTR_EXPRESSION("hello-value"),
JSL_STRING_LIFETIME_STATIC
);

jsl_str_to_str_multimap_insert(
&map,
key,
JSL_STRING_LIFETIME_STATIC,
JSL_FATPTR_EXPRESSION("hello-value2"),
JSL_STRING_LIFETIME_STATIC
);

jsl_str_to_str_multimap_get_value_count_for_key(&map, key); // 2
```

## Functions

* [jsl_str_to_str_multimap_init](#function-jsl_str_to_str_multimap_init)
* [jsl_str_to_str_multimap_init2](#function-jsl_str_to_str_multimap_init2)
* [jsl_str_to_str_multimap_get_key_count](#function-jsl_str_to_str_multimap_get_key_count)
* [jsl_str_to_str_multimap_get_value_count](#function-jsl_str_to_str_multimap_get_value_count)
* [jsl_str_to_str_multimap_has_key](#function-jsl_str_to_str_multimap_has_key)
* [jsl_str_to_str_multimap_insert](#function-jsl_str_to_str_multimap_insert)
* [jsl_str_to_str_multimap_get_value_count_for_key](#function-jsl_str_to_str_multimap_get_value_count_for_key)
* [jsl_str_to_str_multimap_key_value_iterator_init](#function-jsl_str_to_str_multimap_key_value_iterator_init)
* [jsl_str_to_str_multimap_key_value_iterator_next](#function-jsl_str_to_str_multimap_key_value_iterator_next)
* [jsl_str_to_str_multimap_get_values_for_key_iterator_init](#function-jsl_str_to_str_multimap_get_values_for_key_iterator_init)
* [jsl_str_to_str_multimap_get_values_for_key_iterator_next](#function-jsl_str_to_str_multimap_get_values_for_key_iterator_next)
* [jsl_str_to_str_multimap_delete_key](#function-jsl_str_to_str_multimap_delete_key)
* [jsl_str_to_str_multimap_delete_value](#function-jsl_str_to_str_multimap_delete_value)
* [jsl_str_to_str_multimap_clear](#function-jsl_str_to_str_multimap_clear)

```c
typedef struct JSL__StrToStrMultimap JSLStrToStrMultimap;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:216`

---

<a id="type-typedef-jslstrtostrmultimapkeyvalueiter"></a>
### Typedef: `JSLStrToStrMultimapKeyValueIter`

State tracking struct for iterating over all of the keys and values
in the map.

#### Note

If you mutate the map this iterator is automatically invalidated
and any operations on this iterator will terminate with failure return
values.

## Functions

* [jsl_str_to_str_multimap_key_value_iterator_init](#function-jsl_str_to_str_multimap_key_value_iterator_init)
* [jsl_str_to_str_multimap_key_value_iterator_next](#function-jsl_str_to_str_multimap_key_value_iterator_next)

```c
typedef struct JSL__StrToStrMultimapKeyValueIter JSLStrToStrMultimapKeyValueIter;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:231`

---

<a id="type-typedef-jslstrtostrmultimapvalueiter"></a>
### Typedef: `JSLStrToStrMultimapValueIter`

State tracking struct for iterating over all of the values for a given
key.

#### Note

If you mutate the map this iterator is automatically invalidated
and any operations on this iterator will terminate with failure return
values.

## Functions

* [jsl_str_to_str_multimap_get_values_for_key_iterator_init](#function-jsl_str_to_str_multimap_get_values_for_key_iterator_init)
* [jsl_str_to_str_multimap_get_values_for_key_iterator_next](#function-jsl_str_to_str_multimap_get_values_for_key_iterator_next)

```c
typedef struct JSL__StrToStrMultimapValueIter JSLStrToStrMultimapValueIter;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:246`

---

<a id="function-jsl_str_to_str_multimap_init"></a>
### Function: `jsl_str_to_str_multimap_init`

Initialize a multimap with default sizing parameters.

This sets up internal tables in the provided arena, using a 32 entry
initial capacity guess and a 0.75 load factor. The `seed` value is to
protect against hash flooding attacks. If you're absolutely sure this
map cannot be attacked, then zero is valid seed value.

#### Parameters

**map** — Pointer to the multimap to initialize.

**arena** — Arena used for all allocations.

**seed** — Arbitrary seed value for hashing.



#### Returns

`true` on success, `false` if any parameter is invalid or out of memory.

```c
int jsl_str_to_str_multimap_init(JSLStrToStrMultimap *map, JSLAllocatorInterface *allocator, int seed);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:261`

---

<a id="function-jsl_str_to_str_multimap_init2"></a>
### Function: `jsl_str_to_str_multimap_init2`

Initialize a multimap with explicit sizing parameters.

This is identical to `jsl_str_to_str_multimap_init`, but lets callers
provide an initial `item_count_guess` and a `load_factor`. The initial
lookup table is sized to the next power of two above `item_count_guess`,
clamped to at least 32 entries. `load_factor` must be in the range
`(0.0f, 1.0f)` and controls when the table rehashes. The `seed` value
is to protect against hash flooding attacks. If you're absolutely sure 
this map cannot be attacked, then zero is valid seed value

#### Parameters

**map** — Pointer to the multimap to initialize.

**arena** — Arena used for all allocations.

**seed** — Arbitrary seed value for hashing.

**item_count_guess** — Expected max number of keys

**load_factor** — Desired load factor before rehashing



#### Returns

`true` on success, `false` if any parameter is invalid or out of memory.

```c
int jsl_str_to_str_multimap_init2(JSLStrToStrMultimap *map, JSLAllocatorInterface *allocator, int seed, int64_t item_count_guess, float load_factor);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:285`

---

<a id="function-jsl_str_to_str_multimap_get_key_count"></a>
### Function: `jsl_str_to_str_multimap_get_key_count`

Get the number of distinct keys currently stored.

#### Parameters

**map** — Pointer to the multimap.



#### Returns

Key count, or `-1` on error

```c
int jsl_str_to_str_multimap_get_key_count(JSLStrToStrMultimap *map);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:299`

---

<a id="function-jsl_str_to_str_multimap_get_value_count"></a>
### Function: `jsl_str_to_str_multimap_get_value_count`

Get the number of values currently stored.

#### Parameters

**map** — Pointer to the multimap.



#### Returns

Key count, or `-1` on error

```c
int jsl_str_to_str_multimap_get_value_count(JSLStrToStrMultimap *map);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:309`

---

<a id="function-jsl_str_to_str_multimap_has_key"></a>
### Function: `jsl_str_to_str_multimap_has_key`

Does the map have the given key.

#### Parameters

**map** — Pointer to the multimap.



#### Returns

`true` if yes, `false` if no or error

```c
int jsl_str_to_str_multimap_has_key(JSLStrToStrMultimap *map, JSLFatPtr key);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:319`

---

<a id="function-jsl_str_to_str_multimap_insert"></a>
### Function: `jsl_str_to_str_multimap_insert`

Insert a key/value pair.

If the key already exists, the value is appended to that key's value list;
otherwise a new key entry is created. Values are not deduplicated. The
`*_lifetime` parameters control whether the data is referenced directly
(`JSL_STRING_LIFETIME_STATIC`) or copied into the map
(`JSL_STRING_LIFETIME_TRANSIENT`). Use the transient lifetime if the string's
lifetime is less than that of the map.

#### Parameters

**map** — Multimap to mutate.

**key** — Key to insert.

**key_lifetime** — Lifetime semantics for the key data.

**value** — Value to insert.

**value_lifetime** — Lifetime semantics for the value data.



#### Returns

`true` on success, `false` on invalid parameters or OOM.

```c
int jsl_str_to_str_multimap_insert(JSLStrToStrMultimap *map, JSLFatPtr key, JSLStringLifeTime key_lifetime, JSLFatPtr value, JSLStringLifeTime value_lifetime);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:341`

---

<a id="function-jsl_str_to_str_multimap_get_value_count_for_key"></a>
### Function: `jsl_str_to_str_multimap_get_value_count_for_key`

Get the number of values for the given key.

#### Parameters

**map** — Pointer to the multimap.

**key** — Key to check.



#### Returns

Key count, or `-1` on error

```c
int jsl_str_to_str_multimap_get_value_count_for_key(JSLStrToStrMultimap *map, JSLFatPtr key);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:356`

---

<a id="function-jsl_str_to_str_multimap_key_value_iterator_init"></a>
### Function: `jsl_str_to_str_multimap_key_value_iterator_init`

Initialize an iterator that visits every key/value pair in the multimap.

Example:

```
JSLStrToStrMultimapKeyValueIter iter;
jsl_str_to_str_multimap_key_value_iterator_init(
&map, &iter
);

JSLFatPtr key;
JSLFatPtr value;
while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &key, &value))
{
...
}
```

Values for a key are returned before moving on to the next occupied slot,
and the overall traversal order is undefined. The iterator is invalidated
if the map is mutated after initialization.

#### Parameters

**map** — Multimap to iterate over; must be initialized.

**iterator** — Iterator instance to initialize.



#### Returns

`true` on success, `false` if parameters are invalid.

```c
int jsl_str_to_str_multimap_key_value_iterator_init(JSLStrToStrMultimap *map, JSLStrToStrMultimapKeyValueIter *iterator);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:388`

---

<a id="function-jsl_str_to_str_multimap_key_value_iterator_next"></a>
### Function: `jsl_str_to_str_multimap_key_value_iterator_next`

Advance the key/value iterator.

Example:

```
JSLStrToStrMultimapKeyValueIter iter;
jsl_str_to_str_multimap_key_value_iterator_init(
&map, &iter
);

JSLFatPtr key;
JSLFatPtr value;
while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &key, &value))
{
...
}
```

Returns the next key/value pair for the multimap, visiting all values for
a key before moving on. The iterator must be initialized and is
invalidated if the map is mutated; iteration order is undefined.

#### Parameters

**iterator** — Iterator to advance.

**out_key** — Output for the current key.

**out_value** — Output for the current value.



#### Returns

`true` if a pair was produced, `false` if exhausted or invalid.

```c
int jsl_str_to_str_multimap_key_value_iterator_next(JSLStrToStrMultimapKeyValueIter *iterator, JSLFatPtr *out_key, JSLFatPtr *out_value);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:421`

---

<a id="function-jsl_str_to_str_multimap_get_values_for_key_iterator_init"></a>
### Function: `jsl_str_to_str_multimap_get_values_for_key_iterator_init`

Initialize an iterator over all values associated with a key.

Example:

```
JSLStrToStrMultimapValueIter iter;
jsl_str_to_str_multimap_get_values_for_key_iterator_init(
&map, &iter
);

JSLFatPtr value;
while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &value))
{
...
}
```

The iterator is valid only while the map remains unchanged. If the key
is absent or has no values, initialization still succeeds but the first
call to `jsl_str_to_str_multimap_get_values_for_key_iterator_next` will
immediately return `false`.

#### Parameters

**map** — Multimap to read from; must be initialized.

**iterator** — Iterator instance to initialize.

**key** — Key whose values should be iterated.



#### Returns

`true` on success, `false` if parameters are invalid.

```c
int jsl_str_to_str_multimap_get_values_for_key_iterator_init(JSLStrToStrMultimap *map, JSLStrToStrMultimapValueIter *iterator, JSLFatPtr key);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:455`

---

<a id="function-jsl_str_to_str_multimap_get_values_for_key_iterator_next"></a>
### Function: `jsl_str_to_str_multimap_get_values_for_key_iterator_next`

Advance a value iterator for a single key.

Example:

```
JSLStrToStrMultimapValueIter iter;
jsl_str_to_str_multimap_get_values_for_key_iterator_init(
&map, &iter
);

JSLFatPtr value;
while (jsl_str_to_str_multimap_get_values_for_key_iterator_next(&iter, &value))
{
...
}
```

Returns the next value for the key supplied to
`jsl_str_to_str_multimap_get_values_for_key_iterator_init`. The iterator
must be initialized and becomes invalid if the map is mutated; iteration
order is undefined. When the values are exhausted or the iterator is
invalid, the function returns `false`.

#### Parameters

**iterator** — Iterator to advance; must be initialized.

**out_value** — Output for the current value.



#### Returns

`true` if a value was produced, `false` otherwise.

```c
int jsl_str_to_str_multimap_get_values_for_key_iterator_next(JSLStrToStrMultimapValueIter *iterator, JSLFatPtr *out_value);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:489`

---

<a id="function-jsl_str_to_str_multimap_delete_key"></a>
### Function: `jsl_str_to_str_multimap_delete_key`

Remove a key and all of its values.

Iterators become invalid. If the key is not present or parameters are invalid,
the map is unchanged and `false` is returned.

#### Parameters

**map** — Multimap to mutate.

**key** — Key to remove.



#### Returns

`true` if the key existed and was removed, `false` otherwise.

```c
int jsl_str_to_str_multimap_delete_key(JSLStrToStrMultimap *map, JSLFatPtr key);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:504`

---

<a id="function-jsl_str_to_str_multimap_delete_value"></a>
### Function: `jsl_str_to_str_multimap_delete_value`

Remove a single value for the given key.

If the value is found, it is removed from the key's list and recycled
into the value free list. When the last value is removed, the key entry
itself is recycled. No action is taken if the key or value is missing
or parameters are invalid.

#### Parameters

**map** — Multimap to mutate.

**key** — Key whose value should be removed.

**value** — Exact value to remove.



#### Returns

`true` if a value was removed, `false` otherwise.

```c
int jsl_str_to_str_multimap_delete_value(JSLStrToStrMultimap *map, JSLFatPtr key, JSLFatPtr value);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:522`

---

<a id="function-jsl_str_to_str_multimap_clear"></a>
### Function: `jsl_str_to_str_multimap_clear`

Remove all keys and values from the map. Iterators become invalid.

#### Parameters

**map** — Multimap to clear.

```c
void jsl_str_to_str_multimap_clear(JSLStrToStrMultimap *map);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_str_to_str_multimap.h:533`

---


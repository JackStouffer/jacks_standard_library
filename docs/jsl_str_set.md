# API Documentation

## Macros

- [`JSL_STR_SET_VERSION`](#macro-jsl_str_set_version)
- [`JSL_STR_SET_DEF`](#macro-jsl_str_set_def)

## Types

- [`union (unnamed at src/jsl/str_set.h:78:5)`](#type-union-unnamed-at-src-jsl_str_set-h-78-5)
- [`struct (unnamed at src/jsl/str_set.h:80:9)`](#type-struct-unnamed-at-src-jsl_str_set-h-80-9)
- [`JSLStrSet`](#type-typedef-jslstrset)
- [`JSLStrSetKeyValueIter`](#type-typedef-jslstrsetkeyvalueiter)

## Functions

- [`jsl_str_set_init`](#function-jsl_str_set_init)
- [`jsl_str_set_init2`](#function-jsl_str_set_init2)
- [`jsl_str_set_item_count`](#function-jsl_str_set_item_count)
- [`jsl_str_set_has`](#function-jsl_str_set_has)
- [`jsl_str_set_insert`](#function-jsl_str_set_insert)
- [`jsl_str_set_iterator_init`](#function-jsl_str_set_iterator_init)
- [`jsl_str_set_iterator_next`](#function-jsl_str_set_iterator_next)
- [`jsl_str_set_delete`](#function-jsl_str_set_delete)
- [`jsl_str_set_clear`](#function-jsl_str_set_clear)
- [`jsl_str_set_free`](#function-jsl_str_set_free)
- [`jsl_str_set_intersection`](#function-jsl_str_set_intersection)
- [`jsl_str_set_union`](#function-jsl_str_set_union)
- [`jsl_str_set_difference`](#function-jsl_str_set_difference)

## File: src/jsl/str_set.h

## JSL String to String Map

This file is a single header file library that implements a hash set data
structure, which 

### Documentation

See `docs/jsl_str_set.md` for a formatted documentation page.

### Caveats

This set uses arenas, so some wasted memory is indeveatble. Care has
been taken to reuse as much allocated memory as possible. But if your
set is long lived it's possible to start exhausting the arena with
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

<a id="macro-jsl_str_set_version"></a>
### Macro: `JSL_STR_SET_VERSION`

```c
#define JSL_STR_SET_VERSION JSL_STR_SET_VERSION 0x010000
```


*Defined at*: `src/jsl/str_set.h:59`

---

<a id="macro-jsl_str_set_def"></a>
### Macro: `JSL_STR_SET_DEF`

```c
#define JSL_STR_SET_DEF JSL_STR_SET_DEF
```


*Defined at*: `src/jsl/str_set.h:67`

---

<a id="type-union-unnamed-at-src-jsl_str_set-h-78-5"></a>
### : `union (unnamed at src/jsl/str_set.h:78:5)`

- `JSLFatPtr value;`
- `struct JSL__StrSetEntry * next;`


*Defined at*: `src/jsl/str_set.h:78`

---

<a id="type-struct-unnamed-at-src-jsl_str_set-h-80-9"></a>
### : `struct (unnamed at src/jsl/str_set.h:80:9)`

- `int[16] value_sso_buffer;`
- `int64_t value_sso_buffer_len;`


*Defined at*: `src/jsl/str_set.h:80`

---

<a id="type-typedef-jslstrset"></a>
### Typedef: `JSLStrSet`

This is an open addressed hash set with linear probing that maps
This set uses rapidhash, which
is a avalanche hash with a configurable seed value for protection
against hash flooding attacks.

Example:

```
uint8_t buffer[JSL_KILOBYTES(16)];
JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);

JSLStrSet set;
jsl_str_set_init(&set, &stack_arena, 0);

JSLFatPtr value = JSL_FATPTR_INITIALIZER("hello-key");

jsl_str_to_str_multimap_insert(
&set,
value,
JSL_STRING_LIFETIME_STATIC
);

jsl_str_set_get(&set, value);
```

## Functions

* [jsl_str_set_init](#function-jsl_str_set_init)
* [jsl_str_set_init2](#function-jsl_str_set_init2)
* [jsl_str_set_item_count](#function-jsl_str_set_item_count)
* [jsl_str_set_has](#function-jsl_str_set_has)
* [jsl_str_set_insert](#function-jsl_str_set_insert)
* [jsl_str_set_iterator_init](#function-jsl_str_set_iterator_init)
* [jsl_str_set_iterator_next](#function-jsl_str_set_iterator_next)
* [jsl_str_set_delete](#function-jsl_str_set_delete)
* [jsl_str_set_clear](#function-jsl_str_set_clear)

```c
typedef struct JSL__StrSet JSLStrSet;
```


*Defined at*: `src/jsl/str_set.h:164`

---

<a id="type-typedef-jslstrsetkeyvalueiter"></a>
### Typedef: `JSLStrSetKeyValueIter`

State tracking struct for iterating over all of the keys and values
in the set.

#### Note

If you mutate the set this iterator is automatically invalidated
and any operations on this iterator will terminate with failure return
values.

## Functions

* jsl_str_set_key_value_iterator_init
* jsl_str_set_key_value_iterator_next

```c
typedef struct JSL__StrSetKeyValueIter JSLStrSetKeyValueIter;
```


*Defined at*: `src/jsl/str_set.h:179`

---

<a id="function-jsl_str_set_init"></a>
### Function: `jsl_str_set_init`

Initialize a set with default sizing parameters.

This sets up internal tables in the provided arena, using a 32 entry
initial capacity guess and a 0.75 load factor. The `seed` value is to
protect against hash flooding attacks. If you're absolutely sure this
set cannot be attacked, then zero is valid seed value.

#### Parameters

**set** — Pointer to the set to initialize.

**arena** — Arena used for all allocations.

**seed** — Arbitrary seed value for hashing.



#### Returns

`true` on success, `false` if any parameter is invalid or out of memory.

```c
int jsl_str_set_init(JSLStrSet *set, JSLAllocatorInterface *allocator, int seed);
```


*Defined at*: `src/jsl/str_set.h:194`

---

<a id="function-jsl_str_set_init2"></a>
### Function: `jsl_str_set_init2`

Initialize a set with explicit sizing parameters.

This is identical to `jsl_str_set_init`, but lets callers
provide an initial `item_count_guess` and a `load_factor`. The initial
lookup table is sized to the next power of two above `item_count_guess`,
clamped to at least 32 entries. `load_factor` must be in the range
`(0.0f, 1.0f)` and controls when the table rehashes. The `seed` value
is to protect against hash flooding attacks. If you're absolutely sure 
this set cannot be attacked, then zero is valid seed value

#### Parameters

**set** — Pointer to the set to initialize.

**arena** — Arena used for all allocations.

**seed** — Arbitrary seed value for hashing.

**item_count_guess** — Expected max number of keys

**load_factor** — Desired load factor before rehashing



#### Returns

`true` on success, `false` if any parameter is invalid or out of memory.

```c
int jsl_str_set_init2(JSLStrSet *set, JSLAllocatorInterface *allocator, int seed, int64_t item_count_guess, float load_factor);
```


*Defined at*: `src/jsl/str_set.h:218`

---

<a id="function-jsl_str_set_item_count"></a>
### Function: `jsl_str_set_item_count`

Get the number of items currently stored.

#### Parameters

**set** — Pointer to the set.



#### Returns

Key count, or `-1` on error

```c
int jsl_str_set_item_count(JSLStrSet *set);
```


*Defined at*: `src/jsl/str_set.h:232`

---

<a id="function-jsl_str_set_has"></a>
### Function: `jsl_str_set_has`

Does the set have the given key.

#### Parameters

**set** — Pointer to the set.



#### Returns

`true` if yes, `false` if no or error

```c
int jsl_str_set_has(JSLStrSet *set, JSLFatPtr value);
```


*Defined at*: `src/jsl/str_set.h:242`

---

<a id="function-jsl_str_set_insert"></a>
### Function: `jsl_str_set_insert`

Insert a key/value pair.

#### Parameters

**set** — Map to mutate.

**key** — Key to insert.

**key_lifetime** — Lifetime semantics for the key data.

**value** — Value to insert.

**value_lifetime** — Lifetime semantics for the value data.



#### Returns

`true` on success, `false` on invalid parameters or OOM.

```c
int jsl_str_set_insert(JSLStrSet *set, JSLFatPtr value, JSLStringLifeTime value_lifetime);
```


*Defined at*: `src/jsl/str_set.h:257`

---

<a id="function-jsl_str_set_iterator_init"></a>
### Function: `jsl_str_set_iterator_init`

Initialize an iterator that visits every key/value pair in the set.

Example:

```
JSLStrSetKeyValueIter iter;
jsl_str_set_key_value_iterator_init(
&set, &iter
);

JSLFatPtr key;
JSLFatPtr value;
while (jsl_str_set_key_value_iterator_next(&iter, &key, &value))
{
...
}
```

Overall traversal order is undefined. The iterator is invalidated
if the set is mutated after initialization.

#### Parameters

**set** — Map to iterate over; must be initialized.

**iterator** — Iterator instance to initialize.



#### Returns

`true` on success, `false` if parameters are invalid.

```c
int jsl_str_set_iterator_init(JSLStrSet *set, JSLStrSetKeyValueIter *iterator);
```


*Defined at*: `src/jsl/str_set.h:289`

---

<a id="function-jsl_str_set_iterator_next"></a>
### Function: `jsl_str_set_iterator_next`

Advance the key/value iterator.

Example:

```
JSLStrSetKeyValueIter iter;
jsl_str_set_key_value_iterator_init(
&set, &iter
);

JSLFatPtr key;
JSLFatPtr value;
while (jsl_str_set_key_value_iterator_next(&iter, &key, &value))
{
...
}
```

Returns the next key/value pair for the set. The iterator must be
initialized and is invalidated if the set is mutated; iteration order
is undefined.

#### Parameters

**iterator** — Iterator to advance.

**out_key** — Output for the current key.

**out_value** — Output for the current value.



#### Returns

`true` if a pair was produced, `false` if exhausted or invalid.

```c
int jsl_str_set_iterator_next(JSLStrSetKeyValueIter *iterator, JSLFatPtr *out_value);
```


*Defined at*: `src/jsl/str_set.h:322`

---

<a id="function-jsl_str_set_delete"></a>
### Function: `jsl_str_set_delete`

Remove a key/value.

Iterators become invalid. If the key is not present or parameters are invalid,
the set is unchanged and `false` is returned.

#### Parameters

**set** — Map to mutate.

**key** — Key to remove.



#### Returns

`true` if the key existed and was removed, `false` otherwise.

```c
int jsl_str_set_delete(JSLStrSet *set, JSLFatPtr value);
```


*Defined at*: `src/jsl/str_set.h:337`

---

<a id="function-jsl_str_set_clear"></a>
### Function: `jsl_str_set_clear`

Remove all values from the set. Each stored value is checked and if was
stored in the set using `JSL_STRING_LIFETIME_TRANSIENT`, the  the memory
is freed. The set will keep the memory it used for it's internal value
bookkeeping and it will not shrink. Iterators become invalid.

If you want to completely free all memory for this set, use `jsl_str_set_free`.

#### Parameters

**set** — Map to clear.

```c
void jsl_str_set_clear(JSLStrSet *set);
```


*Defined at*: `src/jsl/str_set.h:352`

---

<a id="function-jsl_str_set_free"></a>
### Function: `jsl_str_set_free`

Frees all of the memory for this set and sets in an invalid state from the set.
If you wish to reuse this set after calling this function, you must call init again.

#### Parameters

**set** — Map to free.

```c
void jsl_str_set_free(JSLStrSet *set);
```


*Defined at*: `src/jsl/str_set.h:362`

---

<a id="function-jsl_str_set_intersection"></a>
### Function: `jsl_str_set_intersection`

Fill a set `out` with only the values which exist in both sets `a` and `b`.
All of the values inserted into out are copied with `JSL_STRING_LIFETIME_TRANSIENT`.

#### Parameters

**a** — A string set

**b** — A string set

**out** — The string set to fill



#### Returns

if all of the values present in both sets were successfully added to `out`

```c
int jsl_str_set_intersection(JSLStrSet *a, JSLStrSet *b, JSLStrSet *out);
```


*Defined at*: `src/jsl/str_set.h:375`

---

<a id="function-jsl_str_set_union"></a>
### Function: `jsl_str_set_union`

Fill a set `out` with all of the values from `a` and `b`.
All of the values inserted into out are copied with `JSL_STRING_LIFETIME_TRANSIENT`.

#### Parameters

**a** — A string set

**b** — A string set

**out** — The string set to fill



#### Returns

if all of the values present in both sets were successfully added to `out`

```c
int jsl_str_set_union(JSLStrSet *a, JSLStrSet *b, JSLStrSet *out);
```


*Defined at*: `src/jsl/str_set.h:390`

---

<a id="function-jsl_str_set_difference"></a>
### Function: `jsl_str_set_difference`

Fill a set `out` with all of the values in `a` that are not in `b`.
All of the values inserted into out are copied with `JSL_STRING_LIFETIME_TRANSIENT`.

#### Parameters

**a** — A string set

**b** — A string set

**out** — The string set to fill



#### Returns

if all of the values present in both sets were successfully added to `out`

```c
int jsl_str_set_difference(JSLStrSet *a, JSLStrSet *b, JSLStrSet *out);
```


*Defined at*: `src/jsl/str_set.h:405`

---


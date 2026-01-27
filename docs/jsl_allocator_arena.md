# API Documentation

## Macros

- [`JSL_ARENA_FROM_STACK`](#macro-jsl_arena_from_stack)
- [`JSL_ARENA_TYPED_ALLOCATE`](#macro-jsl_arena_typed_allocate)
- [`JSL_ARENA_TYPED_ARRAY_ALLOCATE`](#macro-jsl_arena_typed_array_allocate)

## Types

- [`JSLArena`](#type-jslarena)

## Functions

- [`jsl_arena_init`](#function-jsl_arena_init)
- [`jsl_arena_init2`](#function-jsl_arena_init2)
- [`jsl_arena_get_allocator_interface`](#function-jsl_arena_get_allocator_interface)
- [`jsl_arena_allocate`](#function-jsl_arena_allocate)
- [`jsl_arena_allocate_aligned`](#function-jsl_arena_allocate_aligned)
- [`jsl_arena_reallocate`](#function-jsl_arena_reallocate)
- [`jsl_arena_reallocate_aligned`](#function-jsl_arena_reallocate_aligned)
- [`jsl_arena_reset`](#function-jsl_arena_reset)
- [`jsl_arena_save_restore_point`](#function-jsl_arena_save_restore_point)
- [`jsl_arena_load_restore_point`](#function-jsl_arena_load_restore_point)

## File: src/jsl_allocator_arena.h

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
because the stack pointer will be reset.

```
void some_func(void)
{
uint8_t buffer[JSL_KILOBYTES(16)];
JSLArena stack_arena = JSL_ARENA_FROM_STACK(buffer);

// example hash map, not real
IntToStrMap map = int_to_str_ctor(&arena);
int_to_str_add(&map, 64, JSL_FATPTR_INITIALIZER("This is my string data!"));

my_hash_map_calculations(&map);

// All hash map memory goes out of scope automatically,
// no need for any destructor
}
```

Fast, cheap, easy automatic memory management!

```c
#define JSL_ARENA_FROM_STACK JSL_ARENA_FROM_STACK ( buf) jsl__arena_from_stack_internal ( ( buf), sizeof ( buf))
```


*Defined at*: `src/jsl_allocator_arena.h:96`

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
struct MyStruct* thing = JSL_ARENA_TYPED_ALLOCATE(struct MyStruct, arena);
```

```c
#define JSL_ARENA_TYPED_ALLOCATE JSL_ARENA_TYPED_ALLOCATE ( T, arena) ( T *) jsl_arena_allocate_aligned ( arena, sizeof ( T), _Alignof ( T), false)
```


*Defined at*: `src/jsl_allocator_arena.h:162`

---

<a id="macro-jsl_arena_typed_array_allocate"></a>
### Macro: `JSL_ARENA_TYPED_ARRAY_ALLOCATE`

Macro to make it easier to allocate a zero filled array of `T` within an arena.

#### Parameters

**T** — Type to allocate.

**arena** — Arena to allocate from; must be initialized.



#### Returns

Pointer to the allocated object or `NULL` on failure.

```
struct MyStruct { uint64_t the_data; };
struct MyStruct* thing_array = JSL_ARENA_TYPED_ALLOCATE(struct MyStruct, arena, 42);
```

```c
#define JSL_ARENA_TYPED_ARRAY_ALLOCATE JSL_ARENA_TYPED_ARRAY_ALLOCATE ( T, arena, length) ( T *) jsl_arena_allocate_aligned ( arena, ( int64_t) sizeof ( T) * length, _Alignof ( T), true)
```


*Defined at*: `src/jsl_allocator_arena.h:176`

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


*Defined at*: `src/jsl_allocator_arena.h:42`

---

<a id="function-jsl_arena_init"></a>
### Function: `jsl_arena_init`

Initialize an arena with the supplied buffer.

#### Parameters

**arena** — Arena instance to initialize; must not be null.

**memory** — Pointer to the beginning of the backing storage.

**length** — Size of the backing storage in bytes.

```c
void jsl_arena_init(JSLArena *arena, void *memory, int64_t length);
```


*Defined at*: `src/jsl_allocator_arena.h:105`

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


*Defined at*: `src/jsl_allocator_arena.h:116`

---

<a id="function-jsl_arena_get_allocator_interface"></a>
### Function: `jsl_arena_get_allocator_interface`

TODO: docs

```c
JSLAllocatorInterface jsl_arena_get_allocator_interface(JSLArena *arena);
```


*Defined at*: `src/jsl_allocator_arena.h:121`

---

<a id="function-jsl_arena_allocate"></a>
### Function: `jsl_arena_allocate`

Allocate a block of memory from the arena using the default alignment.

NULL is returned if the arena does not have enough capacity. When
`zeroed` is true, the allocated bytes are zero-initialized.

#### Parameters

**arena** — Arena to allocate from; must not be null.

**bytes** — Number of bytes to reserve.

**zeroed** — When true, zero-initialize the allocation.



#### Returns

Fat pointer describing the allocation or `{0}` on failure.

```c
void * jsl_arena_allocate(JSLArena *arena, int64_t bytes, bool zeroed);
```


*Defined at*: `src/jsl_allocator_arena.h:134`

---

<a id="function-jsl_arena_allocate_aligned"></a>
### Function: `jsl_arena_allocate_aligned`

Allocate a block of memory from the arena with the provided alignment.

NULL is returned if the arena does not have enough capacity. When
`zeroed` is true, the allocated bytes are zero-initialized.

#### Parameters

**arena** — Arena to allocate from; must not be null.

**bytes** — Number of bytes to reserve.

**alignment** — Desired alignment in bytes; must be a positive power of two.

**zeroed** — When true, zero-initialize the allocation.



#### Returns

Fat pointer describing the allocation or `{0}` on failure.

```c
void * jsl_arena_allocate_aligned(JSLArena *arena, int64_t bytes, int alignment, bool zeroed);
```


*Defined at*: `src/jsl_allocator_arena.h:148`

---

<a id="function-jsl_arena_reallocate"></a>
### Function: `jsl_arena_reallocate`

Resize the allocation if it was the last allocation, otherwise, allocate a new
chunk of memory and copy the old allocation's contents.

```c
void * jsl_arena_reallocate(JSLArena *arena, void *original_allocation, int64_t new_num_bytes);
```


*Defined at*: `src/jsl_allocator_arena.h:182`

---

<a id="function-jsl_arena_reallocate_aligned"></a>
### Function: `jsl_arena_reallocate_aligned`

Resize the allocation if it was the last allocation, otherwise, allocate a new
chunk of memory and copy the old allocation's contents.

```c
void * jsl_arena_reallocate_aligned(JSLArena *arena, void *original_allocation, int64_t new_num_bytes, int align);
```


*Defined at*: `src/jsl_allocator_arena.h:192`

---

<a id="function-jsl_arena_reset"></a>
### Function: `jsl_arena_reset`

Set the current pointer back to the start of the arena.

In debug mode, this function will set all of the memory that was
allocated to `0xfeefee` to help detect use after free bugs.

```c
void jsl_arena_reset(JSLArena *arena);
```


*Defined at*: `src/jsl_allocator_arena.h:205`

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
that was allocated to `0xfeefee` to help detect use after free bugs.

```c
int * jsl_arena_save_restore_point(JSLArena *arena);
```


*Defined at*: `src/jsl_allocator_arena.h:227`

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
that was allocated to `0xfeefee` to help detect use after free bugs.

This function will assert if you attempt to use a different restore point from an
arena different that the one passed in as the parameter.

```c
void jsl_arena_load_restore_point(JSLArena *arena, int *restore_point);
```


*Defined at*: `src/jsl_allocator_arena.h:252`

---


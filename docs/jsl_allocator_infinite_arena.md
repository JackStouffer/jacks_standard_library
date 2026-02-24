# API Documentation

## Macros

- [`JSL_INFINITE_ARENA_TYPED_ALLOCATE`](#macro-jsl_infinite_arena_typed_allocate)

## Types

- [`JSLInfiniteArena`](#type-typedef-jslinfinitearena)

## Functions

- [`jsl_infinite_arena_init`](#function-jsl_infinite_arena_init)
- [`jsl_infinite_arena_get_allocator_interface`](#function-jsl_infinite_arena_get_allocator_interface)
- [`jsl_infinite_arena_allocate`](#function-jsl_infinite_arena_allocate)
- [`jsl_infinite_arena_allocate_aligned`](#function-jsl_infinite_arena_allocate_aligned)
- [`jsl_infinite_arena_reallocate`](#function-jsl_infinite_arena_reallocate)
- [`jsl_infinite_arena_reallocate_aligned`](#function-jsl_infinite_arena_reallocate_aligned)
- [`jsl_infinite_arena_reset`](#function-jsl_infinite_arena_reset)
- [`jsl_infinite_arena_release`](#function-jsl_infinite_arena_release)

## File: src/jsl/allocator_infinite_arena.h

<a id="macro-jsl_infinite_arena_typed_allocate"></a>
### Macro: `JSL_INFINITE_ARENA_TYPED_ALLOCATE`

Macro to make it easier to allocate an instance of `T` within an arena.

#### Parameters

**T** — Type to allocate.

**arena** — Arena to allocate from; must be initialized.



#### Returns

Pointer to the allocated object or `NULL` on failure.

```
struct MyStruct { uint64_t the_data; };
struct MyStruct* thing = JSL_INFINITE_ARENA_TYPED_ALLOCATE(struct MyStruct, arena);
```

```c
#define JSL_INFINITE_ARENA_TYPED_ALLOCATE JSL_INFINITE_ARENA_TYPED_ALLOCATE ( T, arena) ( T *) jsl_infinite_arena_allocate_aligned ( arena, sizeof ( T), _Alignof ( T), false)
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:143`

---

<a id="type-typedef-jslinfinitearena"></a>
### Typedef: `JSLInfiniteArena`

A bump allocator with a (conceptually) infinite amount of memory. Memory is pulled
from the OS using `VirtualAlloc`/`mmap` with no limits.

This allocator is useful for simple programs that can one, be a little sloppy with
memory and two, have a single memory lifetime for the whole program. A couple examples
of such programs would be batch scripts, developer tooling, and daemons. For these
types of programs it's perfectly legitimate to ask for a new piece of memory every time
you need something and never free until the program exits or the process starts over.
You're not going to exhaust the memory one your dev machine when writing tooling to
process a 30kb text file, for example.

This infinite arena is more useful than a conventional arena in these situations because
you don't want the program to fail if you suddenly need way more memory than you
anticipated. In contrast, a desktop GUI program needs to be way more careful about
how much memory is used per lifetime and the reset points of those lifetimes. For
such a program, it would be a bad idea to use an infinite arena since you want to
have constraints as soon as possible in the development cycle to make sure that your
program can run performantly on the minimum tech specs you plan on supporting. You
should develop such a program with mechanisms to break work up so any problem size
fits in the memory limits so set.

## Functions and Macros

* [jsl_infinite_arena_init](#function-jsl_infinite_arena_init)
* [jsl_infinite_arena_allocate](#function-jsl_infinite_arena_allocate)
* [jsl_infinite_arena_allocate_aligned](#function-jsl_infinite_arena_allocate_aligned)
* [jsl_infinite_arena_reallocate](#function-jsl_infinite_arena_reallocate)
* [jsl_infinite_arena_reallocate_aligned](#function-jsl_infinite_arena_reallocate_aligned)
* [jsl_infinite_arena_reset](#function-jsl_infinite_arena_reset)
* [JSL_INFINITE_ARENA_TYPED_ALLOCATE](#macro-jsl_infinite_arena_typed_allocate)

#### Note

This API is not thread safe. Arena memory is assumed to live in a
single thread. If you want to share an arena between threads you need to lock.

```c
typedef struct JSL__InfiniteArena JSLInfiniteArena;
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:72`

---

<a id="function-jsl_infinite_arena_init"></a>
### Function: `jsl_infinite_arena_init`

Initialize an infinite arena to an empty state. This function
does not allocate, as allocations are on demand, so this function
cannot fail.

#### Parameters

**arena** — The arena to initialize.

```c
int jsl_infinite_arena_init(JSLInfiniteArena *arena);
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:81`

---

<a id="function-jsl_infinite_arena_get_allocator_interface"></a>
### Function: `jsl_infinite_arena_get_allocator_interface`

Create a `JSLAllocatorInterface` that routes allocations to the arena.

The returned interface is valid as long as `arena` remains alive.

#### Parameters

**arena** — The arena used for all allocator callbacks.



#### Returns

Allocator interface that uses the infinite arena for allocate/reallocate/free.

```c
JSLAllocatorInterface jsl_infinite_arena_get_allocator_interface(JSLInfiniteArena *arena);
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:91`

---

<a id="function-jsl_infinite_arena_allocate"></a>
### Function: `jsl_infinite_arena_allocate`

Allocate a block of memory from the arena using the default alignment.

NULL is returned if `VirualAlloc`/`mmap` fail. When
`zeroed` is true, the allocated bytes are zero-initialized.

#### Parameters

**arena** — Arena to allocate from; must not be null.

**bytes** — Number of bytes to reserve.

**zeroed** — When true, zero-initialize the allocation.



#### Returns

Fat pointer describing the allocation or `{0}` on failure.

```c
void * jsl_infinite_arena_allocate(JSLInfiniteArena *arena, int64_t bytes, bool zeroed);
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:106`

---

<a id="function-jsl_infinite_arena_allocate_aligned"></a>
### Function: `jsl_infinite_arena_allocate_aligned`

Allocate a block of memory from the arena with the provided alignment.

`NULL` is returned if `VirualAlloc`/`mmap` fail. When
`zeroed` is true, the allocated bytes are zero-initialized.

#### Parameters

**arena** — Arena to allocate from; must not be null.

**bytes** — Number of bytes to reserve.

**alignment** — Desired alignment in bytes; must be a positive power of two.

**zeroed** — When true, zero-initialize the allocation.



#### Returns

Fat pointer describing the allocation or `{0}` on failure.

```c
void * jsl_infinite_arena_allocate_aligned(JSLInfiniteArena *arena, int64_t bytes, int alignment, bool zeroed);
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:124`

---

<a id="function-jsl_infinite_arena_reallocate"></a>
### Function: `jsl_infinite_arena_reallocate`

Resize the current allocation if 

1. It was the last allocation
2. The new size fits in the currently used range of reserved address space
3. `original_allocation` has the default alignment 

Otherwise, allocate a new chunk of memory and copy the old allocation's contents.

If `original_allocation` is null then this is treated as a call to
`jsl_infinite_arena_allocate`.

If `new_num_bytes` is less than the size of the original allocation this is a
no-op.

```c
void * jsl_infinite_arena_reallocate(JSLInfiniteArena *arena, void *original_allocation, int64_t new_num_bytes);
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:160`

---

<a id="function-jsl_infinite_arena_reallocate_aligned"></a>
### Function: `jsl_infinite_arena_reallocate_aligned`

Resize the current allocation if 

1. It was the last allocation
2. The new size fits in the currently used range of reserved address space
3. `original_allocation` has `align` alignment 

Otherwise, allocate a new chunk of memory and copy the old allocation's contents.

If `original_allocation` is null then this is treated as a call to
`jsl_infinite_arena_allocate`.

If `new_num_bytes` is less than the size of the original allocation this is a
no-op.

```c
void * jsl_infinite_arena_reallocate_aligned(JSLInfiniteArena *arena, void *original_allocation, int64_t new_num_bytes, int align);
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:181`

---

<a id="function-jsl_infinite_arena_reset"></a>
### Function: `jsl_infinite_arena_reset`

Set the arena to have zero active memory regions. This does not return
the reserved virtual address ranges back to the OS. All memory is kept
in a free list for future use. If you wish to return the memory to the
OS you'll need to use `jsl_infinite_arena_release`.

#### Parameters

**arena** — The arena to reset

```c
void jsl_infinite_arena_reset(JSLInfiniteArena *arena);
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:196`

---

<a id="function-jsl_infinite_arena_release"></a>
### Function: `jsl_infinite_arena_release`

Release all of the virtual memory back to the OS. This invalidates
the infinite arena and it can not be reused in future operations
until init is called on it again.

Note that it is not necessary to call this function before your program
exits. All virtual memory is automatically freed by the OS when the process
ends. The OS actually does this much faster than you can, all the freeing
can be done in kernal space while this function has to be run in user space.
Manually freeing is a waste of your user's time.

#### Parameters

**arena** — The arena to release the memory from

```c
void jsl_infinite_arena_release(JSLInfiniteArena *arena);
```


*Defined at*: `src/jsl/allocator_infinite_arena.h:211`

---


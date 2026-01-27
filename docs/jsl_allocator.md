# API Documentation

## Macros

- [`JSL_TYPED_ALLOCATE`](#macro-jsl_typed_allocate)

## Types

- [`JSLAllocateFP`](#type-typedef-jslallocatefp)
- [`JSLReallocateFP`](#type-typedef-jslreallocatefp)
- [`JSLFreeAllFP`](#type-typedef-jslfreeallfp)
- [`JSLAllocatorInterface`](#type-typedef-jslallocatorinterface)

## Functions

- [`jsl_align_ptr_upwards`](#function-jsl_align_ptr_upwards)
- [`jsl_align_ptr_upwards_uintptr`](#function-jsl_align_ptr_upwards_uintptr)
- [`jsl_allocator_interface_init`](#function-jsl_allocator_interface_init)
- [`jsl_allocator_interface_alloc`](#function-jsl_allocator_interface_alloc)
- [`jsl_allocator_interface_realloc`](#function-jsl_allocator_interface_realloc)
- [`jsl_allocator_interface_free`](#function-jsl_allocator_interface_free)
- [`jsl_allocator_interface_free_all`](#function-jsl_allocator_interface_free_all)

## File: /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h

This file defines a standardized allocator abstraction to allow library
code to interact cleanly with arbitrary user code.

### Purpose and Design

The structure is `JSLAllocatorInterface` which is a user data pointer with
a set of function pointers. Also, there are a set of provided convenience
functions to call the function pointers on a given allocator instance.

The problem this abstraction is attempting to solve is that not all allocation
strategies are appropriate for all situations. A full blown general purpose
allocator is not very useful for a batch script and would just slow things down,
for example. So, you have an issue where libraries (like this library) need
to write code which allocates memory (data containers, string formatting, etc)
but the specifics of how that memory is acquired are irrelevant or unknowable. 

The downside to any abstraction is that removing knowledge about the specifics
can make code more complicated, or slower, or both. For example, with the
knowledge that you're writing your data container for an arena, you don't need
to worry about freeing individual pieces of data once they become invalid. Your
code is a lot simpler. The inverse problem is also true, in that an abstraction
can assume things that are not true about the underlying implementation. For
example, this abstraction assumes that individual pieces of memory can be freed,
which is not true for an arena allocator. Code which is written with this
assumption then ends up wasting a bunch of memory, as it's ok with allocating
small chunks of memory that it assumes can be reused by the underlying allocator.

Additionally, not all allocators can fit into this structure. Specialized pools,
for example, cannot reallocate an allocation to a different size, and therefore
cannot provide the set of function pointers that this abstraction needs.

Despite this, I believe the cost/benefit analysis comes out in this abstraction's
favor. Without the abstraction, it would not be practical to write things like data
containers that would be useful to more than a handful of people. Not everything
can be written as functions that write into user provided buffers; sometimes you
really do just need to realloc.

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

<a id="macro-jsl_typed_allocate"></a>
### Macro: `JSL_TYPED_ALLOCATE`

Macro to make it easier to allocate an instance of `T`.

#### Parameters

**T** — Type to allocate.

**allocator** — allocator.



#### Returns

Pointer to the allocated object or `NULL` on failure.

```
struct MyStruct { uint64_t the_data; };
struct MyStruct* thing = JSL_TYPED_ALLOCATE(struct MyStruct, arena);
```

```c
#define JSL_TYPED_ALLOCATE JSL_TYPED_ALLOCATE ( T, allocator) ( T *) jsl_allocator_interface_alloc ( allocator, sizeof ( T), _Alignof ( T), false)
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:195`

---

<a id="type-typedef-jslallocatefp"></a>
### Typedef: `JSLAllocateFP`

TODO: docs

Assumptions:

- The returned memory is at least `bytes` long of valid, writable memory
- The returned memory is given at least `alignment` memory alignment
- If size request by the user isn't available this returns `NULL`
- If the allocator is not ready/initialized this returns `NULL`
- If `zeroed` is true, this allocator returns the requested memory already set to zero for all `bytes` number of bytes

```c
typedef void *(*)(void *, int64_t *, int, int) JSLAllocateFP;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:82`

---

<a id="type-typedef-jslreallocatefp"></a>
### Typedef: `JSLReallocateFP`

TODO: docs

Assumptions:

- The returned memory is at least `new_bytes` long of valid, writable memory
- If size request by the user isn't available this returns `NULL`
- If the allocator is not ready/initialized this returns `NULL`
- If `zeroed` is true, this allocator returns the requested memory already set to zero for all `bytes` number of bytes

```c
typedef void *(*)(void *, void *, int64_t *, int) JSLReallocateFP;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:93`

---

<a id="type-typedef-jslfreeallfp"></a>
### Typedef: `JSLFreeAllFP`

TODO: docs

```c
typedef int (*)(void *) JSLFreeAllFP;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:101`

---

<a id="type-typedef-jslallocatorinterface"></a>
### Typedef: `JSLAllocatorInterface`

The structure makes the following assumptions:

- This structure has been initalized using `jsl_allocator_interface_init`
- A given instance of `JSLAllocatorInterface` must have the same or shorter lifetime of the underlying allocator
- Library code can freely store a pointer to this structure
- It is not valid for library code to make a copy of this structure

```c
typedef struct JSL__AllocatorInterface JSLAllocatorInterface;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:122`

---

<a id="function-jsl_align_ptr_upwards"></a>
### Function: `jsl_align_ptr_upwards`

TODO: docs

```c
void * jsl_align_ptr_upwards(void *ptr, int align);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:127`

---

<a id="function-jsl_align_ptr_upwards_uintptr"></a>
### Function: `jsl_align_ptr_upwards_uintptr`

TODO: docs

```c
int jsl_align_ptr_upwards_uintptr(int ptr, int align);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:132`

---

<a id="function-jsl_allocator_interface_init"></a>
### Function: `jsl_allocator_interface_init`

TODO: docs

The value of `ctx` need not be a valid pointer, it can even be `NULL`

```c
void jsl_allocator_interface_init(JSLAllocatorInterface *allocator, JSLAllocateFP allocate_fp, JSLReallocateFP reallocate_fp, int free_fp, JSLFreeAllFP free_all_fp, void *context);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:139`

---

<a id="function-jsl_allocator_interface_alloc"></a>
### Function: `jsl_allocator_interface_alloc`

TODO: docs

```c
void * jsl_allocator_interface_alloc(JSLAllocatorInterface *allocator, int64_t bytes, int alignment, bool zeroed);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:151`

---

<a id="function-jsl_allocator_interface_realloc"></a>
### Function: `jsl_allocator_interface_realloc`

TODO: docs

```c
void * jsl_allocator_interface_realloc(JSLAllocatorInterface *allocator, void *allocation, int64_t new_bytes, int alignment);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:161`

---

<a id="function-jsl_allocator_interface_free"></a>
### Function: `jsl_allocator_interface_free`

TODO: docs

```c
int jsl_allocator_interface_free(JSLAllocatorInterface *allocator, void *allocation);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:171`

---

<a id="function-jsl_allocator_interface_free_all"></a>
### Function: `jsl_allocator_interface_free_all`

TODO: docs

```c
int jsl_allocator_interface_free_all(JSLAllocatorInterface *allocator);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator.h:179`

---


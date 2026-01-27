# API Documentation

## Macros

- (none)

## Types

- [`JSLPoolAllocator`](#type-typedef-jslpoolallocator)

## Functions

- [`jsl_pool_init`](#function-jsl_pool_init)
- [`jsl_pool_init2`](#function-jsl_pool_init2)
- [`jsl_pool_free_allocation_count`](#function-jsl_pool_free_allocation_count)
- [`jsl_pool_total_allocation_count`](#function-jsl_pool_total_allocation_count)
- [`jsl_pool_allocate`](#function-jsl_pool_allocate)
- [`jsl_pool_free`](#function-jsl_pool_free)
- [`jsl_pool_free_all`](#function-jsl_pool_free_all)

## File: /Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h

<a id="type-typedef-jslpoolallocator"></a>
### Typedef: `JSLPoolAllocator`

A pool allocator is a specialized allocator for allocating lots of things of the
same size (or with a well defined maximum). Since every allocation returned is
the same size, allocating and freeing are very fast. The entire allocator is just
one stack of used allocations and another stack of unused allocations.

A pool allocator should not be confused with a connection pool. These are very
different tools. In fact, this allocator should not even be used for the backing
memory for such a connection pool, as the vast majority of connection pools can
be allocated statically.

This allocator is best used in situations where you'll have thousands (or more)
of tiny objects or hundreds of large objects that are all the same type and can
have a relatively short lifetime. In these sorts of situations using a general
purpose allocator can result in heap fragmentation.

Examples of situations where this allocator shines:

* Games with hundreds of short lived entities
* Very large, changing tree structures where each node carries some state
* Many input buffers that have a max size, like in an HTTP server
when you need request body buffers for each request in flight
* Event queues with thousands of events in flight

You should not use this allocator if

* If you aren't both allocating and freeing within the same lifetime.
i.e. if you're just using the free all function all the time then this
allocator isn't giving you anything.
* You cannot define a maximum for your allocation size
* The sum of bytes of valid allocated objects at any given time is low 

Since this allocator is so specialized, this allocator does not provide
the standardized allocator interface in `jsl_allocator.h`. The main reason
being that the concept of a "realloc" from a pool is nonsensical.

Functions and Macros:

* [jsl_pool_init](#function-jsl_pool_init)
* [jsl_pool_init2](#function-jsl_pool_init2)
* [jsl_pool_allocate](#function-jsl_pool_allocate)
* [jsl_pool_free](#function-jsl_pool_free)
* [jsl_pool_free_all](#function-jsl_pool_free_all)
* [jsl_pool_free_allocation_count](#function-jsl_pool_free_allocation_count)
* [jsl_pool_total_allocation_count](#function-jsl_pool_total_allocation_count)

#### Note

The pool API is not thread safe. pool memory is assumed to live in a
single thread. If you want to share an pool between threads you need to lock
when calling these functions.

```c
typedef struct JSL__PoolAllocator JSLPoolAllocator;
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h:86`

---

<a id="function-jsl_pool_init"></a>
### Function: `jsl_pool_init`

Initialize an pool with the supplied buffer.

The number of available allocations is not equal to
`memory length / allocation size`. This allocator needs more space for
bookkeeping, and each of the given allocation pointers given the user
is aligned depending on the size of the allocation chunk for performance
reasons. Larger allocation sizes are aligned to a 4kb page, for example.

#### Parameters

**pool** — pool instance to initialize; must not be null.

**memory** — Pointer to the beginning of the backing storage.

**length** — Size of the backing storage in bytes.

```c
void jsl_pool_init(JSLPoolAllocator *pool, void *memory, int64_t length, int64_t allocation_size);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h:101`

---

<a id="function-jsl_pool_init2"></a>
### Function: `jsl_pool_init2`

Initialize an pool with the supplied buffer.

The number of available chunks is not equal to `memory length / allocation_size`.
This allocator needs more space for bookkeeping and each of the given
allocation pointers given the user is aligned depending on the size of
the allocation chunk for performance reasons. Larger chunk sizes
are aligned to a 4kb page, for example.

#### Parameters

**pool** — pool instance to initialize; must not be null.

**memory** — Pointer to the beginning of the backing storage.

**length** — Size of the backing storage in bytes.

```c
void jsl_pool_init2(JSLPoolAllocator *pool, JSLFatPtr memory, int64_t allocation_size);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h:121`

---

<a id="function-jsl_pool_free_allocation_count"></a>
### Function: `jsl_pool_free_allocation_count`

Get the number of available allocations that the pool has to give out
in calls to `jsl_pool_allocate`.

#### Returns

Allocation count

```c
int jsl_pool_free_allocation_count(JSLPoolAllocator *pool);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h:133`

---

<a id="function-jsl_pool_total_allocation_count"></a>
### Function: `jsl_pool_total_allocation_count`

Get the total number of possible allocations that the pool can give out
in calls to `jsl_pool_allocate` regardless of how many allocations are
currently outstanding.

#### Returns

Allocation count

```c
int jsl_pool_total_allocation_count(JSLPoolAllocator *pool);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h:144`

---

<a id="function-jsl_pool_allocate"></a>
### Function: `jsl_pool_allocate`

Grab an allocation from the pool.

`NULL` is returned if the pool does not have any available allocations.
When `zeroed` is true, the allocated bytes are zero-initialized.

#### Parameters

**pool** — pool to allocate from; must not be null.

**zeroed** — When true, zero-initialize the allocation.



#### Returns

pointer to the allocation or `NULL` on failure.

```c
void * jsl_pool_allocate(JSLPoolAllocator *pool, bool zeroed);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h:158`

---

<a id="function-jsl_pool_free"></a>
### Function: `jsl_pool_free`

Return an allocation to the pool. When the `JSL_DEBUG` preprocessor
macro is set then the pool will overwrite the memory with `0xfeefee`.

#### Returns

Returns true if this allocation is owned by the pool and
was outstanding. Returns false if this allocation is not owned by the
pool or if the allocation was already freed.

```c
int jsl_pool_free(JSLPoolAllocator *pool, void *allocation);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h:168`

---

<a id="function-jsl_pool_free_all"></a>
### Function: `jsl_pool_free_all`

Set all outstanding allocations. When the `JSL_DEBUG` preprocessor
macro is set then the pool will overwrite memory with `0xfeefee`.

```c
void jsl_pool_free_all(JSLPoolAllocator *pool);
```


*Defined at*: `/Users/jackstouffer/Documents/code/jacks_standard_library/src/jsl_allocator_pool.h:174`

---


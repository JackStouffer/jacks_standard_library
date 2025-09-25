# Design

These are general notes on design decisions.

## Why do you use signed 64 bit ints for sizes and not `size_t`?

First of all **no one** is creating contiguous allocations larger than `2^63 - 1`
bytes. So you don't need the extra range that the high bit gives you. Same with
programs using `size_t` for alignment. You are not writing real programs that align
on a 4 gigabyte boundary.

Second of all, it avoids all sorts of bugs that just arise naturally if you're
not constantly on guard. Every time you're subtracting is a potential underflow.
And it turns out, underflows can happen quite a lot. I've gotten fed up with it.
I've switched to signed for all size storage, and I've never had an `int64_t`
overflow on me.

## Fat Pointers

A fat pointer is JSL's representation of a contiguous range of virtual address
space represented as bytes.

A fat pointer is defined as

```c
struct {
    uint8_t* data;
    int64_t length;
};
```

Also, `uint8_t` is important because it helps when using UTF-8. `char`, being
signed, cannot hold the full range of valid UTF-8 code units.

Null-terminated strings are obsolete, inefficient, and constantly frustrating.

### Writing

The fat pointer design makes continuously writing to existing buffers much
more straight forward. 

It's best to hold the fat pointer from the original allocation and when
you need to write to it, you copy it.

```c
JSLFatPtr memory = jsl_arena_allocate(arena, 256, false);
JSLFatPtr writer = memory;
```

The arena part is explained in the "Arenas" section of this document.

Since a fat pointer is not a container (just pointer + length) you now have two pointers
to the exact same piece of memory. Then, the copy becomes your "writer", passed by pointer
to writing functions which increment the pointer and decrement the length. Here's
how a file loading function would work:

```c
#include <assert.h>
#include <fcntl.h>
#include "jacks_standard_library.h"

int64_t load_file(JSLFatPtr* writer, char* filename)
{
    int32_t fd = open(filename, 0);

    struct stat st;
    fstat(fd, &st);
    int64_t file_size = (int64_t) st.st_size;

    // get the minimum of the file size and the space the writer has left
    int64_t file_read_size = (file_size < writer->length) ? file_size : writer->length;

    int64_t read_result = read(fd, writer->data, file_read_size);
    close(fd);

    // modify the writer to point to the remaining contents of the original buffer
    // that have not been written to yet
    writer->data += read_result;
    writer->length -= read_result;

    return read_result;
}


JSLFatPtr memory = jsl_arena_allocate(arena, 1024 * 1024, false);
JSLFatPtr writer = memory;

int64_t file_size = load_file(&writer, "file.txt");
JSLFatPtr file_contents = jsl_fatptr_auto_slice(memory, writer);
```

By using a fat pointer here, we're able to very easily constrain the write to the
buffer by `read` to just the space we have available.

After the function call the writer now points to the empty space remaining in
the original allocation. This makes it really easy to use multiple functions
to write to the same buffer.

### Slicing

You may have noticed the call to `jsl_fatptr_auto_slice`. In the previous example.
This function compares the original fat pointer and your writer fat pointer to make
a new one which points to just the written portion of the original allocation.

This operation, modifying a pointer plus length to point to a subsection of an existing
buffer, is called "slicing" in many languages. Slicing is a giant improvement over C
style null terminated strings, as every time you needed to refer to just a sub section
of a string you'd need to create a new allocation with the null terminator at the end.

For example, lets say I wanted to have a function which takes a file path and returns
just the name part without the file extension. It's not possible to create a function
which does not allocate if you want a null terminated string with just the name part,
as otherwise you'd have to insert a null char in the middle of the existing string.

With a slice operation, just decrement the length and you're done.

Other slicing functions include 

* `jsl_fatptr_slice`
* `jsl_fatptr_total_write_length`

## An Essay On Memory Allocation

Before the concept of an arena is explained, there are several other 
concepts which are important to understand if you want to get good
grasp on arenas, and in my opinion, write better programs.

First, the term "lifetime" will be used a bunch. It simply means the span in
your program where a chunk of memory is valid according to your program's rules.
E.g. an object checked out from an object pool and then checked back in might still
have valid data inside it, but accessing that data outside of the pool is invalid
because the object reached the end of its *lifetime*, according to the usage semantics.

The main thing to understand is that manual memory management is not that complicated
once you change your perspective.

Manual memory management is vastly overcomplicated in the minds of most programmers
thanks to the poor API in the C standard library (`malloc`/`free`/`realloc`). These
functions make two major bad design decisions

* Each allocation is treated as a completely separate lifetime. 
* All allocations pull from a single, implicitly global, heap with a
  conceptually unlimited amount of memory.

These design decisions actually make thinking about memory way more complicated than
it needs to be. In a C program which exclusively uses the standard library allocator,
you have to mentally keep track of every single separate piece of state. Since these
allocations end up referencing each other, then you have a graph structure. Every
time you access something in the graph and another node in the graph is invalid (
not allocated yet, freed, etc) that's a probable error condition. Programming becomes
a giant metal exercise of keeping this whole graph state in your mind when writing 
code. 

You can see how RAII with auto inserted constructors and destructors comes about. But
RAII + operator new/delete is still thinking about allocations as completely separate
unconnected things. Put another way, the very design of operator new believes implicitly
that a single "thing" has a completely unknown lifetime that could end at any moment and
is unconnected to a broader allocation context. This is not true for the vast majority
of the allocations in your program

### Make The Problem Simpler

To make manual memory allocation actually manageable in real world projects, we need to
reframe the way you think of your program's execution into distinct, understandable lifetimes.

For example, in an HTTP web application, there are really only three lifetimes:

1. Memory which lives for the entirety of the application. These are things
   like database connection pools. They're allocated once and never freed.
2. Memory which lives for the lifetime of the request. These are things like
   request and response buffers and everything your application needs to read
   the former and fill the later.
3. Memory which lives for the lifetime of the current function. This can be
   handled by the stack.

Note that only one of these lifetimes needs explicit clean up. If you didn't already
know, the OS automatically frees virtual memory reserved by a process when it exits.
There's no need to call free for global memory like a connection pool. Same thing for
OS resources like open sockets.

Now lets theorize that we have some API like the following:

```c
typedef struct { ... } MyHeap

void my_heap_init(MyHeap* heap, size_t max_bytes);

void* my_heap_allocate(size_t bytes);

void my_heap_free_all(MyHeap* heap);
```

All of a sudden the problem becomes much, much simpler. The program flow would look
something like this:

```c
int32_t socket_file_descriptor = accept_new_connection(http_socket);

MyHeap heap;
my_heap_init(&heap, 16 * 1024 * 1024);

void* request_buffer = my_heap_allocate(1024 * 1024);
void* response_buffer = my_heap_allocate(1024 * 1024);

read_request(request_buffer);
route_function(&heap, request_buffer, response_buffer);
send_buffer(response_buffer);

close(socket_file_descriptor);
my_heap_free_all(&heap);
```

You just keep allocating memory from this heap for all of the things you need in
your route code. You never have to think about what's going to happen to the memory,
because you know it's all going to be freed at the end of the handler!

Just to show you "nothing is up my sleave" with the HTTP example, let's use another,
this time a compiler. A very complicated problem in many people's minds, and I'm sure
great optimization is pretty hard. However memory management for a compiler is not.

A compiler can be broken down into four lifetimes based on the four main parts of
the compiler process

1. File Loading -> File text
2. Lexing -> Token stream
3. Parsing -> AST
4. Executable writing

In modern compilers it's more complicated, with multiple stages of intermediate
representation, but the basic fact remains, that:

* Each step uses the previous result as its only input
* The n-1 step can be freed once n step is done, e.g. once lexing is done you don't
  need the file data anymore, once parsing is done and you have an AST you don't need
  the token stream anymore.

ASTs are pretty complex tree structures with lots of different node types with disparate
internals. That's completely irrelevant for the memory though. When you no longer need
the AST just call `free_all` on your AST specific heap and you're done.

### Breaking Out of the Constructor/Destructor Mindset

It seems like it should be more complicated but it really doesn't have to be.

Programmers have been trained with the RAII or OOP style that every single type
needs 

### Concretely Defining The Problem Space

You've seen how reframing the problem of manual memory management can radically simplify
the problem space. Let's break it down even further by getting more specific.

Allocations can be understood in a matrix of two pieces of information:

1. Do you know the size of the allocation (or at least its max accepted size) before
   run time?
2. Do you know the lifetime of the allocation 

So in general four types of allocations

1. Allocations where you know ahead of time the size and lifetime
2. Allocations where you know ahead of time the size
3. Allocations where you know ahead of time the lifetime
4. Allocations where you don't know anything

95% of allocations in the programs I've seen fall into the first category.


memory speed

You can see how the C++ operator `new` and `delete` with the oft promoted smart pointers
fall into this same problem. They're thinking of memory in these tiny chunks that
all have separate lifetimes, which is almost always wrong.

## Arenas

Now that we've got a good primer on the broad concepts of memory we can turn to the
Arena Allocator.

Arena Allocators (also known as monotonic, region, or bump allocators) are the simplest
possible memory allocator you can write. It's defined as

```c
struct {
    uint8_t* start, end, current;
};
```


They are also extremely useful in cases where,

* You can group together many things with the same lifetime
* This lifetime has a very well understood terminus
* You're in a situation where every byte isn't precious

TODO: mention aiming for L2 cache size
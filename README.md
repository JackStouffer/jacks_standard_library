# Jack's Standard Library

A collection of types and functions designed to replace usage of the
C Standard Library.

## Usage

1. Copy the `jacks_standard_library.h` file into your repo
2. Include the header like normally in each source file:

```c
#include "jacks_standard_library.h"
```

3. Then, in ONE AND ONLY ONE file, do this:

```c
#define JSL_IMPLEMENTATION
#include "jacks_standard_library.h"
```

This should probably be in the "main" file for your program, but it doesn't have to be.

One note is that this library does not depend on the C standard library to be available
at link time if you don't want to use it. However, it does require the "compile time"
headers `stddef.h`, `stdint.h`, and `stdbool.h` (if using < C23). You'll also have to
define the replacement functions for the C standard library functions like `assert` and
`memcmp`. See the "Preprocessor Switches" section for more information.

## Testing

This file runs the test suite using a meta-program style of build system. Then bootstrap the executable once.

On POSIX

```bash
cc -o run_test_suite ./tests/bin/run_test_suite.c 
```

On Windows

```
cl /Fe"run_test_suite.exe" tests\\bin\\run_test_suite.c 
```

Then run your executable. Every time afterwards when you run the test
program it will check if there have been changes to the test file. If
there have been it will rebuild itself using the program you used to
build it in the first place. if there have been changes to the test
file, so no need to 

### Structure

Each test file is compiled and run four times. On POSIX systems it's once
with gcc unoptimized and with address sanitizer, once with gcc full optimization
and native CPU code gen, and then the same thing again with clang. On
Windows it's done with MSVC and clang.

This may seem excessive but I've caught bugs with this before.

## Why

See the DESIGN.md file for more info.

## What's Included

* Common macros
   min, max
   bitflag checks
* A buffer type called a fat pointer
   * used everywhere
   * standardizes that pointers should carry their length
   * vastly simplifies functions like file reading
* Common string and buffer utilities for fat pointers
   * things like fat pointer memcmp, memcpy, substring search, etc.
* An arena allocator
   * a.k.a a monotonic, region, or bump allocator
   * They are easy to create, use, reset-able, allocators
   * Great for things with known lifetimes (which is 99% of the things you allocate)
   * See the DESIGN.md file more information
* A snprintf replacement
   * works directly with fat pointers
   * Removes all compiler specific weirdness
* A string builder container type
* Type safe, generated containers
   * Unlike a lot of C containers, these are built with "before compile" code generation.
     Each container instance generates code with your value types rather than using `void*`
     plus length everywhere
   * Built with arenas in mind
   * dynamic array
   * hash map
   * hash set

## What's Not Included

* A scanf alternative for fat pointers
* Threading
* Atomic operations
* Date/time utilities
* Random numbers

This library is slow for ARM as I haven't gotten around to writing the NEON
versions of the SIMD code yet. glibc will be significantly faster for comparable
operations.

## File Utilities

JSL includes a couple of functions or simple, cross platform file I/O. 

These are specifically called file utils because they are intended for scripts or
getting things going at the start of the project. In serious code, you would use
I/O more tailored to your specific use case, e.g. asyncio, atomic file operations,
custom package formats, etc.

These are separated out from the main code since they require the standard
library at link time, as they use OS level calls. Unfortunately, Linux is the only
OS that has a robust syscall API, so accessing the OS on other systems is only
valid through their runtime libraries.

You can include these functions by using `#define JSL_INCLUDE_FILE_UTILS`. 

## What's Supported

Official support is for Windows, macOS, and Linux with MSVC, GCC, and clang.

This might work on other POSIX systems with other C compilers, but I have not tested
it. ARM in big endian mode is not supported.

## Caveats 

### Notes on Safety

In 99% of cases you shouldn't turn off assertions by using the `NDEBUG` macro
or by defining an empty `JSL_ASSERT` function. 

This library uses assertions for things like `NULL` checks and Automatic Bounds
Checking (ABC). These checks prevent your program from entering an invalid state
which may corrupt user data, leak memory, or allow buffer overflow attacks. If
you have good 

### Unicode

You should have a basic knowledge of Unicode semantics before using this library.
For example, this library provides length based comparison functions like
`jsl_fatptr_memory_compare`. This function provides byte by byte comparison; if
you know enough about Unicode, you should know why this is error prone for
determining two string's equality.

If you don't, you should learn the following terms:

* Code unit
* Code point
* Grapheme
* How those three things are completely different from each other
* Normalization

That would be the bare minimum needed to not shoot yourself in the foot.

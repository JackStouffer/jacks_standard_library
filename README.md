not finished yet

# Jack's Standard Library

A collection of single header file libraries designed to replace usage
of the C Standard Library with more modern, safe, and simple alternatives
for C11/C23.

## Why

See the DESIGN.md file for more info.

## What's Included

### Jack's Standard Library Core

`jsl_core.h`

* All other libraries within this project require the core
* Common macros
   * Target platform information, e.g. OS, ISA, pointer size, and host compiler
   * min, max, between
   * bitflag checks
   * Alias macros for different intrinsic names on different compilers
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

### Jack's Standard Library File Utilities

`jsl_files.h`

* Requires the standard library at link time, as they use OS level calls
    * Linux is the only OS to have a robust syscall API
* Simple, cross platform file I/O
    * Intended for scripts or getting things going at the start of the project
    * In serious code, you would use I/O more tailored to your specific use case
      e.g. atomic, package formats, async, etc.
* Contains
    * file reading, writing, and get file size
    * mkdir
    * a `fprintf` replacement

### Jack's Standard Library Unicode

`jsl_unicode.h`

**WARNING** On MSVC you must pass `/utf-8` or else MSVC will silently give you
incorrect code generation!

### Jack's Standard Library Templates

`cli/`

* Type safe, generated containers
* Unlike a lot of C containers, these are built with "before compile" code generation.
* Each container instance generates code with your value types rather than using `void*` plus length everywhere
* Built with arenas in mind
* dynamic array
* hash map
* hash set

## What's Not Included

* A scanf alternative for fat pointers
* Sorting
* Threading
* Atomic operations
* Date/time utilities
* Random numbers

This is not to say these things will never be added in the future. But everything
in the above list (other than date/time) is pretty situation or platform
dependent. One size fits all solutions are not appropriate for things like
atomics or threading.

## What's Supported

Official support:

* Standards modes C11, C17, C23
* Windows
   * MSVC and clang
   * x86_64
   * 32 bit pointer mode supported, must have 64bit registers
* macOS
   * gcc and clang
   * x86_64 and ARM little endian
   * 32 bit pointer mode supported, must have 64bit registers
* Linux
   * gcc and clang
   * x86_64 and ARM little endian
   * 32 bit pointer mode supported, must have 64bit registers
* WebAssembly
   * clang and emscripten
   * WASM32

This might work on other POSIX systems with other C compilers, but I have not tested
it. This will not work on other compilers on Windows without edits.

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
You can also define an empty `.c` file with this as the only thing in it if that would
more easily integrate into your build.

One note is that this library does not depend on the C standard library to be available
at link time if you don't want to use it. However, it does require the "compile time"
headers `stddef.h`, `stdint.h`, and `stdbool.h` (if using < C23). You'll also have to
define the replacement functions for the C standard library functions like `assert` and
`memcmp`. See the "Preprocessor Switches" section for more information.

### Note On Compile Flags

In an attempt to be as compatible with as many projects as possible, these libraries
are verified with a very strict warning subset and with many clang/gcc runtime sanitizers.

The following shows the most restrictive, verified set of flags tested for each compiler.

MSVC:

```
/W4 /WX /std:c11
```

clang:

```
-std=c11 -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wconditional-uninitialized -Wcomma -Widiomatic-parentheses -Wpointer-arith -Wassign-enum -Wswitch-enum -Wimplicit-fallthrough -Wnull-dereference -Wmissing-prototypes -Wundef -pedantic -fsanitize=address -fsanitize-address-use-after-scope -fsanitize=undefined -fsanitize=pointer-compare,pointer-subtract -fsanitize=alignment -fsanitize=unreachable,return -fsanitize=signed-integer-overflow,shift,shift-base,shift-exponent -fno-sanitize-recover=all
```

gcc:

```

```

## Testing

Each test file is compiled and run many times. For example, Clang is used to build once
with sanitizers and once optimized, same with GCC. This may seem excessive but I've caught
bugs with this before.

### Dependencies

On windows, you will need Visual Studio, clang, and wasmtime. You need to have `cl.exe`,
`clang`, and `wasmtime` available in the path.

On POSIX systems you will need gcc, clang, and wasmtime. You need to have `gcc`,
`clang`, and `wasmtime` available in the path.

### Running

This file runs the test suite using a meta-program style of build system. You need to
bootstrap the executable first.

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
there have been it will rebuild itself using the command you used to
build it in the first place.

## Caveats

### ARM

This library is slow for ARM as I haven't gotten around to writing the NEON
versions of the SIMD code for many of the functions. glibc will be significantly
faster for comparable operations.

### Notes on Safety

In 99% of cases you shouldn't turn off assertions by using the `NDEBUG` macro
or by defining an empty `JSL_ASSERT` function. 

This library uses assertions for things like Automatic Bounds Checking (ABC).
These checks prevent your program from entering an invalid state which may
corrupt user data, leak memory, or allow buffer overflow attacks.

The main purported reason for turning off assertions or ABC is performance.
This is "fake optimization" most of the time, i.e. following a dogma rather
than using a profiler to see if an assertion is a noticeable cost in a critical
path.

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

# API Documentation

## Macros

- (none)

## Types

- [`JSLGetFileSizeResultEnum`](#type-jslgetfilesizeresultenum)
- [`JSLLoadFileResultEnum`](#type-jslloadfileresultenum)
- [`JSLWriteFileResultEnum`](#type-jslwritefileresultenum)
- [`JSLFileTypeEnum`](#type-jslfiletypeenum)

## Functions

- [`jsl_get_file_size`](#function-jsl_get_file_size)
- [`jsl_load_file_contents`](#function-jsl_load_file_contents)
- [`jsl_load_file_contents_buffer`](#function-jsl_load_file_contents_buffer)
- [`jsl_write_file_contents`](#function-jsl_write_file_contents)
- [`jsl_write_to_c_file`](#function-jsl_write_to_c_file)
- [`jsl_c_file_output_sink`](#function-jsl_c_file_output_sink)

## File: src/jsl_os.h

## Jack's Standard Library File Utilities

File loading and writing utilities. These require linking the standard library.

See README.md for a detailed intro.

See DESIGN.md for background on the design decisions.

See DOCUMENTATION.md for a single markdown file containing all of the docstrings
from this file. It's more nicely formatted and contains hyperlinks.

The convention of this library is that all symbols prefixed with either `jsl__`
or `JSL__` (with two underscores) are meant to be private to this library. They
are not a stable part of the API.

### External Preprocessor Definitions

`JSL_DEBUG` - turns on some debugging features, like overwriting stale memory with
`0xfeefee`.

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

<a id="type-jslgetfilesizeresultenum"></a>
### : `JSLGetFileSizeResultEnum`

- `JSL_GET_FILE_SIZE_BAD_PARAMETERS = 0`
- `JSL_GET_FILE_SIZE_OK = 1`
- `JSL_GET_FILE_SIZE_NOT_FOUND = 2`
- `JSL_GET_FILE_SIZE_NOT_REGULAR_FILE = 3`
- `JSL_GET_FILE_SIZE_ENUM_COUNT = 4`


*Defined at*: `src/jsl_os.h:87`

---

<a id="type-jslloadfileresultenum"></a>
### : `JSLLoadFileResultEnum`

- `JSL_FILE_LOAD_BAD_PARAMETERS = 0`
- `JSL_FILE_LOAD_SUCCESS = 1`
- `JSL_FILE_LOAD_COULD_NOT_OPEN = 2`
- `JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE = 3`
- `JSL_FILE_LOAD_COULD_NOT_GET_MEMORY = 4`
- `JSL_FILE_LOAD_READ_FAILED = 5`
- `JSL_FILE_LOAD_CLOSE_FAILED = 6`
- `JSL_FILE_LOAD_ERROR_UNKNOWN = 7`
- `JSL_FILE_LOAD_ENUM_COUNT = 8`


*Defined at*: `src/jsl_os.h:97`

---

<a id="type-jslwritefileresultenum"></a>
### : `JSLWriteFileResultEnum`

- `JSL_FILE_WRITE_BAD_PARAMETERS = 0`
- `JSL_FILE_WRITE_SUCCESS = 1`
- `JSL_FILE_WRITE_COULD_NOT_OPEN = 2`
- `JSL_FILE_WRITE_COULD_NOT_WRITE = 3`
- `JSL_FILE_WRITE_COULD_NOT_CLOSE = 4`
- `JSL_FILE_WRITE_ENUM_COUNT = 5`


*Defined at*: `src/jsl_os.h:111`

---

<a id="type-jslfiletypeenum"></a>
### : `JSLFileTypeEnum`

- `JSL_FILE_TYPE_UNKNOWN = 0`
- `JSL_FILE_TYPE_REG = 1`
- `JSL_FILE_TYPE_DIR = 2`
- `JSL_FILE_TYPE_SYMLINK = 3`
- `JSL_FILE_TYPE_BLOCK = 4`
- `JSL_FILE_TYPE_CHAR = 5`
- `JSL_FILE_TYPE_FIFO = 6`
- `JSL_FILE_TYPE_SOCKET = 7`
- `JSL_FILE_TYPE_COUNT = 8`


*Defined at*: `src/jsl_os.h:122`

---

<a id="function-jsl_get_file_size"></a>
### Function: `jsl_get_file_size`

Get the file size in bytes from the file at `path`.

#### Parameters

**path** — The file system path

**out_size** — Pointer where the resulting size will be stored, must not be null

**out_os_error_code** — Pointer where an error code will be stored when applicable. Can be null



#### Returns

An enum which denotes success or failure

```c
JSLGetFileSizeResultEnum jsl_get_file_size(JSLFatPtr path, int64_t *out_size, int *out_os_error_code);
```


*Defined at*: `src/jsl_os.h:143`

---

<a id="function-jsl_load_file_contents"></a>
### Function: `jsl_load_file_contents`

Load the contents of the file at `path` into a newly allocated buffer
from the given arena. The buffer will be the exact size of the file contents.

If the arena does not have enough space,

```c
JSLLoadFileResultEnum jsl_load_file_contents(JSLAllocatorInterface *allocator, JSLFatPtr path, JSLFatPtr *out_contents, int *out_errno);
```


*Defined at*: `src/jsl_os.h:155`

---

<a id="function-jsl_load_file_contents_buffer"></a>
### Function: `jsl_load_file_contents_buffer`

Load the contents of the file at `path` into an existing fat pointer buffer.

Copies up to `buffer->length` bytes into `buffer->data` and advances the fat
pointer by the amount read so the caller can continue writing into the same
backing storage. Returns a `JSLLoadFileResultEnum` describing the outcome and
optionally stores the system `errno` in `out_errno` on failure.

#### Parameters

**buffer** — buffer to write to

**path** — The file system path

**out_errno** — A pointer which will be written to with the errno on failure



#### Returns

An enum which represents the result

```c
JSLLoadFileResultEnum jsl_load_file_contents_buffer(JSLFatPtr *buffer, JSLFatPtr path, int *out_errno);
```


*Defined at*: `src/jsl_os.h:175`

---

<a id="function-jsl_write_file_contents"></a>
### Function: `jsl_write_file_contents`

Write the bytes in `contents` to the file located at `path`.

Opens or creates the destination file and attempts to write the entire
contents buffer. Returns a `JSLWriteFileResultEnum` describing the
outcome, stores the number of bytes written in `bytes_written` when
provided, and optionally writes the failing `errno` into `out_errno`.

#### Parameters

**contents** — Data to be written to disk

**path** — File system path to write to

**bytes_written** — Optional pointer that receives the bytes written on success

**out_errno** — Optional pointer that receives the system errno on failure



#### Returns

A result enum describing the write outcome

```c
JSLWriteFileResultEnum jsl_write_file_contents(JSLFatPtr contents, JSLFatPtr path, int64_t *bytes_written, int *out_errno);
```


*Defined at*: `src/jsl_os.h:195`

---

<a id="function-jsl_write_to_c_file"></a>
### Function: `jsl_write_to_c_file`

Write the contents of a fat pointer to a `FILE*`.

This implementation uses libc's `fwrite` to write to the file stream. If
this function returns less than `data.length` then the file stream is most
likely in an error state. In that case, `-errno` will be returned and you
can get more info with `ferror`.

#### Parameters

**out** — Destination `FILE*` stream

**data** — Buffer containing the bytes to write



#### Returns

Bytes written, or `-1` when arguments are invalid, or `-errno` on error

```c
int jsl_write_to_c_file(int *out, JSLFatPtr data);
```


*Defined at*: `src/jsl_os.h:214`

---

<a id="function-jsl_c_file_output_sink"></a>
### Function: `jsl_c_file_output_sink`

TODO: docs

```c
JSLOutputSink jsl_c_file_output_sink(int *file);
```


*Defined at*: `src/jsl_os.h:219`

---


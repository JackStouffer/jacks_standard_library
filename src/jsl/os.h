/**
 * # Jack's Standard Library File Utilities
 *
 * File loading and writing utilities. These require linking the standard library.
 *
 * See README.md for a detailed intro.
 *
 * See DESIGN.md for background on the design decisions.
 *
 * See DOCUMENTATION.md for a single markdown file containing all of the docstrings
 * from this file. It's more nicely formatted and contains hyperlinks.
 *
 * The convention of this library is that all symbols prefixed with either `jsl__`
 * or `JSL__` (with two underscores) are meant to be private to this library. They
 * are not a stable part of the API.
 *
 * ## External Preprocessor Definitions
 *
 * `JSL_DEBUG` - turns on some debugging features, like overwriting stale memory with
 * `0xfeefee`.
 *
 * ## License
 *
 * Copyright (c) 2026 Jack Stouffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

#if JSL_IS_WINDOWS

    #include <errno.h>
    #include <fcntl.h>
    #include <limits.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <io.h>
    #include <share.h>
    #include <direct.h>
    #include <sys\stat.h>
    #include <windows.h>

#elif JSL_IS_POSIX

    #include <errno.h>
    #include <ftw.h>
    #include <limits.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>

#else

    #error "os.h: Unsupported OS detected. The JSL OS interface is for POSIX and Windows systems only."

#endif

/**
* TODO: docs
*/
typedef enum
{
    JSL_GET_FILE_SIZE_BAD_PARAMETERS = 0,
    JSL_GET_FILE_SIZE_OK,
    JSL_GET_FILE_SIZE_NOT_FOUND,
    JSL_GET_FILE_SIZE_NOT_REGULAR_FILE,

    JSL_GET_FILE_SIZE_ENUM_COUNT
} JSLGetFileSizeResultEnum;

/**
* TODO: docs
*/
typedef enum
{
    JSL_FILE_LOAD_BAD_PARAMETERS,
    JSL_FILE_LOAD_SUCCESS,
    JSL_FILE_LOAD_COULD_NOT_OPEN,
    JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE,
    JSL_FILE_LOAD_COULD_NOT_GET_MEMORY,
    JSL_FILE_LOAD_READ_FAILED,
    JSL_FILE_LOAD_CLOSE_FAILED,
    JSL_FILE_LOAD_ERROR_UNKNOWN,

    JSL_FILE_LOAD_ENUM_COUNT
} JSLLoadFileResultEnum;

/**
* TODO: docs
*/
typedef enum
{
    JSL_FILE_WRITE_BAD_PARAMETERS = 0,
    JSL_FILE_WRITE_SUCCESS,
    JSL_FILE_WRITE_COULD_NOT_OPEN,
    JSL_FILE_WRITE_COULD_NOT_WRITE,
    JSL_FILE_WRITE_COULD_NOT_CLOSE,

    JSL_FILE_WRITE_ENUM_COUNT
} JSLWriteFileResultEnum;

/**
* TODO: docs
*/
typedef enum
{
    JSL_MAKE_DIRECTORY_BAD_PARAMETERS = 0,
    JSL_MAKE_DIRECTORY_SUCCESS,
    JSL_MAKE_DIRECTORY_ALREADY_EXISTS,
    JSL_MAKE_DIRECTORY_PATH_TOO_LONG,
    JSL_MAKE_DIRECTORY_PERMISSION_DENIED,
    JSL_MAKE_DIRECTORY_PARENT_NOT_FOUND,
    JSL_MAKE_DIRECTORY_NO_SPACE,
    JSL_MAKE_DIRECTORY_READ_ONLY_FS,
    JSL_MAKE_DIRECTORY_ERROR_UNKNOWN,

    JSL_MAKE_DIRECTORY_ENUM_COUNT
} JSLMakeDirectoryResultEnum;

/**
* TODO: docs
*/
typedef enum
{
    JSL_DELETE_FILE_BAD_PARAMETERS = 0,
    JSL_DELETE_FILE_SUCCESS,
    JSL_DELETE_FILE_NOT_FOUND,
    JSL_DELETE_FILE_IS_DIRECTORY,
    JSL_DELETE_FILE_PERMISSION_DENIED,
    JSL_DELETE_FILE_PATH_TOO_LONG,
    JSL_DELETE_FILE_ERROR_UNKNOWN,

    JSL_DELETE_FILE_ENUM_COUNT
} JSLDeleteFileResultEnum;

/**
* TODO: docs
*/
typedef enum
{
    JSL_DELETE_DIRECTORY_BAD_PARAMETERS = 0,
    JSL_DELETE_DIRECTORY_SUCCESS,
    JSL_DELETE_DIRECTORY_NOT_FOUND,
    JSL_DELETE_DIRECTORY_NOT_A_DIRECTORY,
    JSL_DELETE_DIRECTORY_PERMISSION_DENIED,
    JSL_DELETE_DIRECTORY_PATH_TOO_LONG,
    JSL_DELETE_DIRECTORY_ERROR_UNKNOWN,

    JSL_DELETE_DIRECTORY_ENUM_COUNT
} JSLDeleteDirectoryResultEnum;

/**
* TODO: docs
*/
typedef enum {
    JSL_FILE_TYPE_UNKNOWN = 0,
    JSL_FILE_TYPE_NOT_FOUND,
    JSL_FILE_TYPE_REG,
    JSL_FILE_TYPE_DIR,
    JSL_FILE_TYPE_SYMLINK,
    JSL_FILE_TYPE_BLOCK,
    JSL_FILE_TYPE_CHAR,
    JSL_FILE_TYPE_FIFO,
    JSL_FILE_TYPE_SOCKET,

    JSL_FILE_TYPE_COUNT
} JSLFileTypeEnum;

/**
* Get the file size in bytes from the file at `path`.
*
* @param path The file system path
* @param out_size Pointer where the resulting size will be stored, must not be null
* @param out_os_error_code Pointer where an error code will be stored when applicable. Can be null
* @returns An enum which denotes success or failure
*/
JSL_WARN_UNUSED JSLGetFileSizeResultEnum jsl_get_file_size(
    JSLImmutableMemory path,
    int64_t* out_size,
    int32_t* out_os_error_code
);

/**
* Create a new directory at `path`.
*
* The path may be relative or absolute. On POSIX systems the directory is
* created with permissions `0755` (subject to the process umask).
*
* @param path File system path of the directory to create
* @param out_errno Optional pointer that receives the system errno on failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLMakeDirectoryResultEnum jsl_make_directory(
    JSLImmutableMemory path,
    int32_t* out_errno
);

/**
* Delete the file system entry at `path`.
*
* The path may be relative or absolute. The path is copied into a stack
* buffer so no heap allocation is performed. Follows `unlink()` semantics:
* symbolic links, FIFOs, sockets, and other non-regular files are removed
* by unlinking the filesystem entry rather than following it. Directories
* are explicitly rejected and cause `JSL_DELETE_FILE_IS_DIRECTORY` to be
* returned.
*
* @param path The file system path to delete
* @param out_errno Optional pointer that receives the system errno on failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLDeleteFileResultEnum jsl_delete_file(
    JSLImmutableMemory path,
    int32_t* out_errno
);

/**
* Delete the directory at `path` and all of its contents recursively.
*
* The path may be relative or absolute. All files and subdirectories inside
* the target directory are removed before the directory itself is deleted.
* No heap allocation is performed; path components are built in stack buffers.
* Symlinks inside the directory are unlinked without following them.
*
* Returns `JSL_DELETE_DIRECTORY_NOT_A_DIRECTORY` when the path exists but
* does not point to a directory. Returns `JSL_DELETE_DIRECTORY_NOT_FOUND`
* when the path does not exist.
*
* @param path The file system path of the directory to delete
* @param out_errno Optional pointer that receives a system error code on failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLDeleteDirectoryResultEnum jsl_delete_directory(
    JSLImmutableMemory path,
    int32_t* out_errno
);

/**
* Determine the type of file system entry at `path`.
*
* The path may be relative or absolute. The path is copied into a stack
* buffer so no heap allocation is performed. On POSIX, `lstat` is used so
* symlinks themselves are reported as `JSL_FILE_TYPE_SYMLINK` rather than
* the target's type. On Windows, `GetFileAttributesA` is used to detect
* reparse points (symbolic links and junctions), which are reported as
* `JSL_FILE_TYPE_SYMLINK`.
*
* If the path argument is invalid or the type cannot be determined,
* `JSL_FILE_TYPE_UNKNOWN` is returned. If the path does not exist,
* `JSL_FILE_TYPE_NOT_FOUND` is returned.
*
* @param path The file system path
* @returns A `JSLFileTypeEnum` value describing the entry type
*/
JSL_WARN_UNUSED JSLFileTypeEnum jsl_get_file_type(JSLImmutableMemory path);

/**
    * Load the contents of the file at `path` into a newly allocated buffer
    * from the given arena. The buffer will be the exact size of the file contents.
    *
    * If the arena does not have enough space,
    */
JSL_WARN_UNUSED JSL_DEF JSLLoadFileResultEnum jsl_load_file_contents(
    JSLAllocatorInterface allocator,
    JSLImmutableMemory path,
    JSLImmutableMemory* out_contents,
    int32_t* out_errno
);

/**
* Load the contents of the file at `path` into an existing fat pointer buffer.
*
* Copies up to `buffer->length` bytes into `buffer->data` and advances the fat
* pointer by the amount read so the caller can continue writing into the same
* backing storage. Returns a `JSLLoadFileResultEnum` describing the outcome and
* optionally stores the system `errno` in `out_errno` on failure.
*
* @param buffer buffer to write to
* @param path The file system path
* @param out_errno A pointer which will be written to with the errno on failure
* @returns An enum which represents the result
*/
JSL_WARN_UNUSED JSL_DEF JSLLoadFileResultEnum jsl_load_file_contents_buffer(
    JSLMutableMemory* buffer,
    JSLImmutableMemory path,
    int32_t* out_errno
);

/**
* Write the bytes in `contents` to the file located at `path`.
*
* Opens or creates the destination file and attempts to write the entire
* contents buffer. Returns a `JSLWriteFileResultEnum` describing the
* outcome, stores the number of bytes written in `bytes_written` when
* provided, and optionally writes the failing `errno` into `out_errno`.
*
* @param contents Data to be written to disk
* @param path File system path to write to
* @param bytes_written Optional pointer that receives the bytes written on success
* @param out_errno Optional pointer that receives the system errno on failure
* @returns A result enum describing the write outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLWriteFileResultEnum jsl_write_file_contents(
    JSLImmutableMemory contents,
    JSLImmutableMemory path,
    int64_t* bytes_written,
    int32_t* out_errno
);

/**
* Write the contents of a fat pointer to a `FILE*`.
*
* This implementation uses libc's `fwrite` to write to the file stream. If
* this function returns less than `data.length` then the file stream is most
* likely in an error state. In that case, `-errno` will be returned and you
* can get more info with `ferror`.
*
* @param out Destination `FILE*` stream
* @param data Buffer containing the bytes to write
* @returns Bytes written, or `-1` when arguments are invalid, or `-errno` on error
*/
int64_t jsl_write_to_c_file(FILE* out, JSLImmutableMemory data);

/**
* TODO: docs
*/
JSLOutputSink jsl_c_file_output_sink(FILE* file);


#ifdef __cplusplus
} /* extern "C" */
#endif

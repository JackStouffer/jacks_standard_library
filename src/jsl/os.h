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

    #include <dirent.h>
    #include <errno.h>
    #include <ftw.h>
    #include <limits.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>

#else

    #error "os.h: Unsupported OS detected. The JSL OS allocator is for POSIX and Windows systems only."

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
typedef enum
{
    JSL_COPY_FILE_BAD_PARAMETERS = 0,
    JSL_COPY_FILE_SUCCESS,
    JSL_COPY_FILE_PATH_TOO_LONG,
    JSL_COPY_FILE_SOURCE_NOT_FOUND,
    JSL_COPY_FILE_SOURCE_IS_DIRECTORY,
    JSL_COPY_FILE_COULD_NOT_OPEN_SOURCE,
    JSL_COPY_FILE_COULD_NOT_OPEN_DEST,
    JSL_COPY_FILE_READ_FAILED,
    JSL_COPY_FILE_WRITE_FAILED,
    JSL_COPY_FILE_PERMISSION_DENIED,
    JSL_COPY_FILE_ERROR_UNKNOWN,

    JSL_COPY_FILE_ENUM_COUNT
} JSLCopyFileResultEnum;

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

/**
* Maximum directory depth supported by the recursive iterator. The iterator
* state is allocated inline so this puts a hard cap on the deepest tree it
* can walk. Trees deeper than this will cause `JSL_DIRECTORY_ITERATOR_MAX_DEPTH_EXCEEDED`
* to be reported on the entry that would have descended past the limit.
*/
#define JSL_DIRECTORY_ITERATOR_MAX_DEPTH 128

/**
* Result codes for `jsl_directory_iterator_init`.
*/
typedef enum
{
    JSL_DIRECTORY_ITERATOR_INIT_BAD_PARAMETERS = 0,
    JSL_DIRECTORY_ITERATOR_INIT_SUCCESS,
    JSL_DIRECTORY_ITERATOR_INIT_PATH_TOO_LONG,
    JSL_DIRECTORY_ITERATOR_INIT_NOT_FOUND,
    JSL_DIRECTORY_ITERATOR_INIT_NOT_A_DIRECTORY,
    JSL_DIRECTORY_ITERATOR_INIT_COULD_NOT_OPEN,
    JSL_DIRECTORY_ITERATOR_INIT_RESOLVE_FAILED,
    JSL_DIRECTORY_ITERATOR_INIT_ERROR_UNKNOWN,

    JSL_DIRECTORY_ITERATOR_INIT_ENUM_COUNT
} JSLDirectoryIteratorInitResultEnum;

/**
* Per-entry status reported by `jsl_directory_iterator_next` on the result
* struct. `OK` means the entry data is fully populated. `PARTIAL` means the
* path is valid but the entry's type could not be determined. The remaining
* values describe failures encountered while preparing to read the entry.
*/
typedef enum
{
    JSL_DIRECTORY_ITERATOR_PARTIAL = 0,
    JSL_DIRECTORY_ITERATOR_OK,
    JSL_DIRECTORY_ITERATOR_COULD_NOT_OPEN,
    JSL_DIRECTORY_ITERATOR_PATH_TOO_LONG,
    JSL_DIRECTORY_ITERATOR_MAX_DEPTH_EXCEEDED,

    JSL_DIRECTORY_ITERATOR_ENUM_COUNT
} JSLDirectoryIteratorResultEnum;

/**
* Information for the entry produced by `jsl_directory_iterator_next`.
*
* The `absolute_path` and `relative_path` members both point into a buffer
* owned by the `JSLDirectoryIterator`. They are only valid until the next call
* to `jsl_directory_iterator_next` or `jsl_directory_iterator_end` on the
* same iterator. If you need a copy that outlives the next iteration, copy
* the bytes out yourself.
*
* `relative_path` is rooted at the directory passed to
* `jsl_directory_iterator_init` (it does not include a leading separator).
* `depth` is `0` for entries directly inside the input directory, `1` for
* entries one level deeper, and so on.
*/
typedef struct JSLDirectoryIteratorResult
{
    JSLImmutableMemory absolute_path;
    JSLImmutableMemory relative_path;
    JSLFileTypeEnum type;
    int32_t depth;
    JSLDirectoryIteratorResultEnum status;
} JSLDirectoryIteratorResult;

/**
* Per-directory frame stored on the iterator's internal stack. This type is
* part of the public ABI only because it is embedded in `JSLDirectoryIterator`;
* its members are private to the implementation and should not be used by
* callers.
*/
typedef struct JSLDirectoryIteratorFrame
{
    int64_t prefix_length;
    #if JSL_IS_WINDOWS
        HANDLE find_handle;
        bool consumed_first;
    #elif JSL_IS_POSIX
        DIR* dir;
    #endif
} JSLDirectoryIteratorFrame;

/**
* Recursive directory iterator state. Allocate one of these on the stack and
* hand a pointer to `jsl_directory_iterator_init`. The iterator owns its own
* path buffer and per-frame state so it does not allocate from the heap.
*
* All members are private to the implementation. Treat the struct as opaque.
*/
typedef struct JSLDirectoryIterator
{
    uint64_t sentinel;
    char path_buffer[FILENAME_MAX + 1];
    int64_t current_length;
    int64_t base_length;
    int32_t depth;
    bool follow_symlinks;
    bool exhausted;

    #if JSL_IS_WINDOWS
        WIN32_FIND_DATAA find_data;
        char find_pattern[FILENAME_MAX + 3];
    #endif

    JSLDirectoryIteratorFrame frames[JSL_DIRECTORY_ITERATOR_MAX_DEPTH];
} JSLDirectoryIterator;

/**
* Begin a depth-first walk of the directory tree rooted at `path`.
*
* The iterator state is stored entirely inside `iterator`; no heap allocation
* is performed. After a successful return the caller should drive iteration
* with `jsl_directory_iterator_next` until it returns `false`, then optionally
* call `jsl_directory_iterator_end`. Calling `jsl_directory_iterator_end`
* on a fully drained iterator is safe and is also the correct way to release
* resources if iteration is abandoned early.
*
* The `.` and `..` entries are always skipped. There is no guaranteed ordering
* among siblings within a single directory; the order matches whatever the
* underlying OS returns. The walk is depth-first pre-order: a directory entry
* is reported before its contents.
*
* When `follow_symlinks` is `false` (the default behavior recommended for most
* uses) symbolic links are reported as `JSL_FILE_TYPE_SYMLINK` and their
* targets are not descended into. When `follow_symlinks` is `true` the iterator
* resolves the link target's type and will descend into directories reached
* through symlinks. Symlink loop detection is *not* performed.
*
* @param path Directory path to walk
* @param iterator State block to initialize, must not be null
* @param follow_symlinks Whether to follow symbolic links into directories
* @returns A result enum describing the outcome of the init step
*/
JSL_WARN_UNUSED JSL_DEF JSLDirectoryIteratorInitResultEnum jsl_directory_iterator_init(
    JSLImmutableMemory path,
    JSLDirectoryIterator* iterator,
    bool follow_symlinks
);

/**
* Advance to the next entry in the walk.
*
* Fills `result` with information about the entry. Returns `true` while the
* iterator has more entries to report. When iteration completes (either
* because the tree is exhausted or because the iterator was never successfully
* initialized) returns `false` and `result->status` is left unchanged.
*
* If the iterator could not enter a subdirectory it does not stop. The error
* is recorded on `result->status` (e.g., `JSL_DIRECTORY_ITERATOR_COULD_NOT_OPEN`)
* on the entry that represents the directory itself, and iteration continues
* with the next sibling on the next call. If a single entry's metadata could
* not be determined the entry is still returned with `result->type` set to
* `JSL_FILE_TYPE_UNKNOWN` and `result->status` set to `JSL_DIRECTORY_ITERATOR_PARTIAL`.
*
* @param iterator The iterator state, must not be null
* @param result The result struct to fill, must not be null
* @param out_errno Optional pointer that receives the system errno when a
*                  failure code is reported on this entry
* @returns `true` if a new entry was produced, `false` when iteration is done
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_directory_iterator_next(
    JSLDirectoryIterator* iterator,
    JSLDirectoryIteratorResult* result,
    int32_t* out_errno
);

/**
* Release any OS handles still held by the iterator.
*
* Safe to call any number of times and on a fully drained iterator. Calling
* this is only required when iteration is abandoned early - draining the
* iterator with `jsl_directory_iterator_next` until it returns `false`
* releases all resources automatically.
*
* @param iterator The iterator state, may be null
*/
JSL_DEF void jsl_directory_iterator_end(JSLDirectoryIterator* iterator);

/**
* Copy the file at `src_path` to `dst_path`.
*
* The paths may be relative or absolute. Both paths are copied into stack
* buffers so no heap allocation is performed. On POSIX, the copy is
* performed using `open`/`read`/`write` with an 8 KiB stack buffer and
* the destination is created with permissions `0600` (subject to the
* process umask). On Windows, `CopyFileA` is used and any existing
* destination file is overwritten.
*
* Only regular files may be used as the source. Attempting to copy a
* directory returns `JSL_COPY_FILE_SOURCE_IS_DIRECTORY`.
*
* @param src_path File system path of the source file
* @param dst_path File system path for the destination file
* @param out_errno Optional pointer that receives the system error code on failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLCopyFileResultEnum jsl_copy_file(
    JSLImmutableMemory src_path,
    JSLImmutableMemory dst_path,
    int32_t* out_errno
);

/**
* TODO: docs
*/
typedef enum
{
    JSL_RENAME_FILE_BAD_PARAMETERS = 0,
    JSL_RENAME_FILE_SUCCESS,
    JSL_RENAME_FILE_SOURCE_NOT_FOUND,
    JSL_RENAME_FILE_DEST_ALREADY_EXISTS,
    JSL_RENAME_FILE_PATH_TOO_LONG,
    JSL_RENAME_FILE_PERMISSION_DENIED,
    JSL_RENAME_FILE_CROSS_DEVICE,
    JSL_RENAME_FILE_READ_ONLY_FS,
    JSL_RENAME_FILE_ERROR_UNKNOWN,

    JSL_RENAME_FILE_ENUM_COUNT
} JSLRenameFileResultEnum;

/**
* Rename or move the file system entry at `src_path` to `dst_path`.
*
* Works on files, directories, and symbolic links. The paths may be relative
* or absolute. Both paths are copied into stack buffers so no heap allocation
* is performed.
*
* On POSIX, `rename()` is used; it atomically replaces the destination if
* it names an existing file of compatible type. On Windows, `MoveFileExA`
* is used with `MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED`.
*
* Cross-device moves (source and destination on different filesystems)
* return `JSL_RENAME_FILE_CROSS_DEVICE`.
*
* @param src_path File system path of the source entry
* @param dst_path File system path for the destination entry
* @param out_errno Optional pointer that receives the system error code on failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLRenameFileResultEnum jsl_rename_file(
    JSLImmutableMemory src_path,
    JSLImmutableMemory dst_path,
    int32_t* out_errno
);

/**
* TODO: docs
*/
typedef enum
{
    JSL_COPY_DIRECTORY_BAD_PARAMETERS = 0,
    JSL_COPY_DIRECTORY_SUCCESS,
    JSL_COPY_DIRECTORY_PATH_TOO_LONG,
    JSL_COPY_DIRECTORY_SOURCE_NOT_FOUND,
    JSL_COPY_DIRECTORY_SOURCE_NOT_A_DIRECTORY,
    JSL_COPY_DIRECTORY_DEST_ALREADY_EXISTS,
    JSL_COPY_DIRECTORY_COULD_NOT_CREATE_DEST,
    JSL_COPY_DIRECTORY_COULD_NOT_OPEN_SOURCE,
    JSL_COPY_DIRECTORY_COPY_FILE_FAILED,
    JSL_COPY_DIRECTORY_MAKE_SUBDIR_FAILED,
    JSL_COPY_DIRECTORY_PERMISSION_DENIED,
    JSL_COPY_DIRECTORY_ERROR_UNKNOWN,

    JSL_COPY_DIRECTORY_ENUM_COUNT
} JSLCopyDirectoryResultEnum;

/**
* Copy the directory at `src_path` to `dst_path` recursively.
*
* The paths may be relative or absolute. Both paths are copied into stack
* buffers so no heap allocation is performed. The destination directory must
* not already exist; if it does, `JSL_COPY_DIRECTORY_DEST_ALREADY_EXISTS` is
* returned. Symbolic links encountered during the walk are not followed and
* are not copied to the destination. Only regular files and subdirectories
* are reproduced in the destination tree.
*
* The copy is not atomic. If an error occurs mid-copy the destination may
* already contain a partial copy of the source tree.
*
* @param src_path File system path of the source directory
* @param dst_path File system path for the destination directory
* @param out_errno Optional pointer that receives the system error code on failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLCopyDirectoryResultEnum jsl_copy_directory(
    JSLImmutableMemory src_path,
    JSLImmutableMemory dst_path,
    int32_t* out_errno
);

/**
* TODO: docs
*/
typedef enum
{
    JSL_SUBPROCESS_CREATE_BAD_PARAMETERS = 0,
    JSL_SUBPROCESS_CREATE_SUCCESS,
    JSL_SUBPROCESS_CREATE_COULD_NOT_ALLOCATE,

    JSL_SUBPROCESS_CREATE_ENUM_COUNT
} JSLSubProcessCreateResultEnum;

/**
* TODO: docs
*/
typedef enum
{
    JSL_SUBPROCESS_ARG_BAD_PARAMETERS = 0,
    JSL_SUBPROCESS_ARG_SUCCESS,
    JSL_SUBPROCESS_ARG_COULD_NOT_ALLOCATE,

    JSL_SUBPROCESS_ARG_ENUM_COUNT
} JSLSubProcessArgResultEnum;

/**
* TODO: docs
*/
typedef enum
{
    JSL_SUBPROCESS_ENV_BAD_PARAMETERS = 0,
    JSL_SUBPROCESS_ENV_SUCCESS,
    JSL_SUBPROCESS_ENV_COULD_NOT_ALLOCATE,

    JSL_SUBPROCESS_ENV_ENUM_COUNT
} JSLSubProcessEnvResultEnum;

/**
* How the child process's standard input stream will be supplied.
*
* `INHERIT` (the default) means the child reads from the parent's stdin.
* `MEMORY` feeds a fixed byte buffer to the child's stdin and closes it
* once the buffer is exhausted. `FD` wires up a caller-provided file
* descriptor as the child's stdin. On Windows, `fd` is a CRT file
* descriptor from `_open`/`_sopen_s`/`_fileno`, which can be converted to
* a `HANDLE` with `_get_osfhandle` when the child is spawned.
*/
typedef enum
{
    JSL_SUBPROCESS_STDIN_INHERIT = 0,
    JSL_SUBPROCESS_STDIN_MEMORY,
    JSL_SUBPROCESS_STDIN_FD,
    JSL_SUBPROCESS_STDIN_NULL,

    JSL_SUBPROCESS_STDIN_ENUM_COUNT
} JSLSubProcessStdinKindEnum;

/**
* How the child process's stdout or stderr stream will be directed.
*
* `INHERIT` (the default) forwards writes to the parent's corresponding
* standard stream. `FD` redirects writes into a caller-provided file
* descriptor. `SINK` captures writes at run time and delivers them to a
* `JSLOutputSink`. On Windows, `fd` is a CRT file descriptor (see the
* notes on `JSLSubProcessStdinKindEnum`).
*/
typedef enum
{
    JSL_SUBPROCESS_OUTPUT_INHERIT = 0,
    JSL_SUBPROCESS_OUTPUT_FD,
    JSL_SUBPROCESS_OUTPUT_SINK,
    JSL_SUBPROCESS_OUTPUT_NULL,

    JSL_SUBPROCESS_OUTPUT_ENUM_COUNT
} JSLSubProcessOutputKindEnum;

/**
* A key-value pair representing an environment variable to be set on a
* subprocess before it is started. Both `key` and `value` point into memory
* owned by the `JSLSubprocess` allocator and are valid for the lifetime of
* the subprocess handle.
*/
typedef struct JSLSubProcessEnvVar
{
    JSLImmutableMemory key;
    JSLImmutableMemory value;
} JSLSubProcessEnvVar;

/**
* Lifecycle status for a `JSLSubprocess`.
*
* `NOT_STARTED` is the state immediately after `jsl_subprocess_create`.
* `RUNNING` is set while a child is alive — during
* `jsl_subprocess_run_blocking`'s wait loop, or after
* `jsl_subprocess_background_start` returns and before the child is
* observed to have terminated. `EXITED` means the child ran to completion and returned an exit
* code (available via `proc->exit_code`). `KILLED_BY_SIGNAL` means the
* child was terminated by a signal on POSIX and `proc->exit_code` holds
* the negated signal number. `FAILED_TO_START` means the run function
* could not launch the child (pipe/fork/CreateProcess/allocation
* failure); in that case no exit code is meaningful.
*/
typedef enum
{
    JSL_SUBPROCESS_STATUS_NOT_STARTED = 0,
    JSL_SUBPROCESS_STATUS_RUNNING,
    JSL_SUBPROCESS_STATUS_EXITED,
    JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL,
    JSL_SUBPROCESS_STATUS_FAILED_TO_START,

    JSL_SUBPROCESS_STATUS_ENUM_COUNT
} JSLSubProcessStatusEnum;

/**
* Handle for a subprocess that has been configured but not yet started.
*
* Allocate one of these on the stack and pass a pointer to
* `jsl_subprocess_create`. After creation, use `jsl_subprocess_arg` and
* `jsl_subprocess_env` to configure command-line arguments and environment
* variables before running.
*
* All dynamically-sized internal arrays are backed by the allocator provided
* at creation time. The strings passed to `jsl_subprocess_arg` and
* `jsl_subprocess_env` are duplicated into that allocator, so the caller
* does not need to keep the originals alive.
*/
typedef struct JSLSubprocess
{
    uint64_t sentinel;
    JSLAllocatorInterface allocator;

    JSLSubProcessStatusEnum status;

    // Exit code of the child process. Only meaningful once `status` is
    // `EXITED` (exit code) or `KILLED_BY_SIGNAL` (negated signal number on
    // POSIX). Populated by any API that observes the child's termination:
    // `jsl_subprocess_run_blocking`, `jsl_subprocess_background_poll`,
    // and `jsl_subprocess_background_wait`.
    int32_t exit_code;

    // Last OS errno observed on any pump/wait operation affecting this
    // proc. Zero if none. Written by `jsl_subprocess_background_*`.
    int32_t last_errno;

    JSLImmutableMemory executable;

    JSLImmutableMemory* args;
    int64_t args_count;
    int64_t args_capacity;

    JSLSubProcessEnvVar* env_vars;
    int64_t env_count;
    int64_t env_capacity;

    JSLImmutableMemory working_directory;

    JSLSubProcessStdinKindEnum stdin_kind;
    JSLImmutableMemory stdin_memory;
    int stdin_fd;

    JSLSubProcessOutputKindEnum stdout_kind;
    int stdout_fd;
    JSLOutputSink stdout_sink;

    JSLSubProcessOutputKindEnum stderr_kind;
    int stderr_fd;
    JSLOutputSink stderr_sink;

    // Background-mode runtime state. Zero / -1 / INVALID_HANDLE_VALUE when
    // the process was not started via `jsl_subprocess_background_start`.
    bool is_background;
    int64_t stdin_write_offset;
    bool stdout_eof_seen;
    bool stderr_eof_seen;
#if JSL_IS_WINDOWS
    HANDLE process_handle;
    HANDLE stdin_write_handle;
    HANDLE stdout_read_handle;
    HANDLE stderr_read_handle;

    // Overlapped-I/O state for the three parent-side pipe ends. The
    // parent ends are opened with FILE_FLAG_OVERLAPPED so that readiness
    // can participate in WaitForMultipleObjectsEx. Each pipe has its own
    // OVERLAPPED struct, manual-reset event, pending flag, and a fixed
    // scratch buffer whose lifetime is the subprocess lifetime.
    OVERLAPPED stdin_write_overlapped;
    HANDLE stdin_write_event;
    bool stdin_write_pending;
    DWORD stdin_write_buffer_len;
    uint8_t stdin_write_buffer[4096];

    OVERLAPPED stdout_read_overlapped;
    HANDLE stdout_read_event;
    bool stdout_read_pending;
    uint8_t stdout_read_buffer[4096];

    OVERLAPPED stderr_read_overlapped;
    HANDLE stderr_read_event;
    bool stderr_read_pending;
    uint8_t stderr_read_buffer[4096];
#elif JSL_IS_POSIX
    int32_t pid;
    int stdin_write_fd;
    int stdout_read_fd;
    int stderr_read_fd;
#endif
} JSLSubprocess;

/**
* Create a new subprocess handle for the given executable.
*
* The handle is not started; use `jsl_subprocess_arg` and
* `jsl_subprocess_env` to configure it.
*
* The `executable` string is duplicated into `allocator`, so the caller
* does not need to keep the original alive.
*
* @param proc       Pointer to the subprocess handle to initialize
* @param allocator  Allocator used for all internal dynamic storage
* @param executable Path or name of the executable to run
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessCreateResultEnum jsl_subprocess_create(
    JSLSubprocess* proc,
    JSLAllocatorInterface allocator,
    JSLImmutableMemory executable
);

/**
* Append one or more command-line arguments from an array of
* `JSLImmutableMemory`.
*
* Arguments are stored in order and will be passed to the child process
* in the same order they were added. Each argument string is duplicated
* into the subprocess's allocator.
*
* For literal argument lists prefer the `jsl_subprocess_arg` or
* `jsl_subprocess_arg_cstr` macros.
*
* @param proc  Pointer to an initialized subprocess handle
* @param args  Array of arguments to append
* @param count Number of elements in `args`
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessArgResultEnum jsl_subprocess_args(
    JSLSubprocess* proc,
    const JSLImmutableMemory* args,
    int64_t count
);

// Private
//
// Append a sentinel-terminated variadic list of `JSLImmutableMemory`
// arguments passed by value. The trailing sentinel (data == NULL,
// length < 0) marks the end of the list and must be present. Prefer
// the `jsl_subprocess_arg` macro, which supplies the sentinel
// automatically.
//
// Arguments are passed by value through varargs. This is portable
// across MSVC, gcc, and clang for small standard-layout structs.
JSLSubProcessArgResultEnum jsl__subprocess_args_va(
    JSLSubprocess* proc,
    ...
);

// Private
//
// Append a NULL-terminated variadic list of `const char*` C-string
// arguments. The trailing NULL marks the end of the list and must be
// present. Prefer the `jsl_subprocess_arg_cstr` macro, which supplies
// the sentinel automatically.
JSLSubProcessArgResultEnum jsl__subprocess_args_cstr_va(
    JSLSubprocess* proc,
    ...
);

// Private
//
// Sentinel `JSLImmutableMemory` value used to terminate the variadic
// argument list of `jsl_subprocess_arg`. A legitimate argument always
// has non-NULL `data`, so this pattern is unambiguous.
#if JSL_IS_MSVC
    #define JSL__SUBPROCESS_ARG_SENTINEL jsl_immutable_memory(NULL, -1)
#else
    #define JSL__SUBPROCESS_ARG_SENTINEL ((JSLImmutableMemory){ NULL, -1 })
#endif

/**
* Append one or more command-line arguments given as `JSLImmutableMemory`
* values. Works on all supported compilers (including MSVC) when the
* arguments are produced by `JSL_CSTR_EXPRESSION` or any other
* expression that yields a `JSLImmutableMemory`.
*
* ```c
* JSLImmutableMemory my_flag_value = my_function();
* jsl_subprocess_arg(&proc, JSL_CSTR_EXPRESSION("-o"), my_flag_value);
* ```
*
* @param proc  Pointer to an initialized subprocess handle
* @param ...   One or more `JSLImmutableMemory` values
*/
#define jsl_subprocess_arg(proc, ...) \
    jsl__subprocess_args_va((proc), __VA_ARGS__, JSL__SUBPROCESS_ARG_SENTINEL)

/**
* Append one or more command-line arguments given as C-string literals.
*
* ```c
* jsl_subprocess_arg_cstr(&proc, "-o", "output.txt");
* ```
*
* @param proc  Pointer to an initialized subprocess handle
* @param ...   One or more `const char*` arguments
*/
#define jsl_subprocess_arg_cstr(proc, ...) \
    jsl__subprocess_args_cstr_va((proc), __VA_ARGS__, (const char*)NULL)

/**
* Set an environment variable on the subprocess.
*
* The variable will be added to the child process's environment when it is
* started. Both `key` and `value` are duplicated into the subprocess's
* allocator. Calling this more than once with the same key appends a
* duplicate entry; no deduplication is performed.
*
* @param proc  Pointer to an initialized subprocess handle
* @param key   Environment variable name
* @param value Environment variable value
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessEnvResultEnum jsl_subprocess_env(
    JSLSubprocess* proc,
    JSLImmutableMemory key,
    JSLImmutableMemory value
);

/**
* Set the working directory the subprocess will be started in.
*
* The `path` string is duplicated into the subprocess's allocator, so the
* caller does not need to keep the original alive. Calling this more than
* once replaces the previously stored working directory. If this is never
* called, the subprocess inherits the parent's working directory.
*
* @param proc Pointer to an initialized subprocess handle
* @param path Working directory path to use when the process is started
* @returns `true` on success, `false` if parameters were invalid or the
*          path could not be duplicated into the allocator
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_change_working_directory(
    JSLSubprocess* proc,
    JSLImmutableMemory path
);

/**
* Configure the subprocess to read its standard input from a byte buffer.
*
* At run time the buffer is fed to the child's stdin and the stream is
* closed once fully written. The buffer is duplicated into the subprocess's
* allocator, so the caller does not need to keep the original memory alive.
* Replaces any previously configured stdin source.
*
* @param proc Pointer to an initialized subprocess handle
* @param data Bytes to feed to the child's stdin; must have non-null `data`
* @returns `true` on success, `false` if parameters were invalid or the
*          buffer could not be duplicated into the allocator
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stdin_memory(
    JSLSubprocess* proc,
    JSLImmutableMemory data
);

/**
* Configure the subprocess to read its standard input from the given file
* descriptor.
*
* The descriptor is used as-is when the child is spawned. JSL does not
* close or duplicate the descriptor; the caller is responsible for its
* lifetime. On Windows, `fd` must be a CRT file descriptor (see
* `JSLSubProcessStdinKindEnum`). Replaces any previously configured stdin
* source.
*
* @param proc Pointer to an initialized subprocess handle
* @param fd   Caller-owned file descriptor to use as the child's stdin
* @returns `true` on success, `false` if parameters were invalid
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stdin_fd(
    JSLSubprocess* proc,
    int fd
);

/**
* Configure the subprocess to read its standard input from the platform's
* null device (`/dev/null` on POSIX, `NUL` on Windows).
*
* The child observes an immediately-closed, empty stdin: any read returns
* end-of-file. Replaces any previously configured stdin source.
*
* @param proc Pointer to an initialized subprocess handle
* @returns `true` on success, `false` if parameters were invalid
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stdin_null(
    JSLSubprocess* proc
);

/**
* Redirect the subprocess's standard output into the given file descriptor.
*
* The descriptor is used as-is when the child is spawned. JSL does not
* close or duplicate the descriptor; the caller is responsible for its
* lifetime. On Windows, `fd` must be a CRT file descriptor. Replaces any
* previously configured stdout destination.
*
* @param proc Pointer to an initialized subprocess handle
* @param fd   Caller-owned file descriptor to receive the child's stdout
* @returns `true` on success, `false` if parameters were invalid
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stdout_fd(
    JSLSubprocess* proc,
    int fd
);

/**
* Capture the subprocess's standard output into the given output sink.
*
* Writes produced by the child are forwarded to `sink.write_fp` while the
* process runs. The sink's `write_fp` must be non-null. Replaces any
* previously configured stdout destination.
*
* @param proc Pointer to an initialized subprocess handle
* @param sink Output sink that receives the child's stdout
* @returns `true` on success, `false` if parameters were invalid
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stdout_sink(
    JSLSubprocess* proc,
    JSLOutputSink sink
);

/**
* Redirect the subprocess's standard error into the given file descriptor.
*
* The descriptor is used as-is when the child is spawned. JSL does not
* close or duplicate the descriptor; the caller is responsible for its
* lifetime. On Windows, `fd` must be a CRT file descriptor. Replaces any
* previously configured stderr destination.
*
* @param proc Pointer to an initialized subprocess handle
* @param fd   Caller-owned file descriptor to receive the child's stderr
* @returns `true` on success, `false` if parameters were invalid
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stderr_fd(
    JSLSubprocess* proc,
    int fd
);

/**
* Capture the subprocess's standard error into the given output sink.
*
* Writes produced by the child are forwarded to `sink.write_fp` while the
* process runs. The sink's `write_fp` must be non-null. Replaces any
* previously configured stderr destination.
*
* @param proc Pointer to an initialized subprocess handle
* @param sink Output sink that receives the child's stderr
* @returns `true` on success, `false` if parameters were invalid
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stderr_sink(
    JSLSubprocess* proc,
    JSLOutputSink sink
);

/**
* Discard the subprocess's standard output.
*
* The child's stdout is redirected to the platform's null device
* (`/dev/null` on POSIX, `NUL` on Windows). Replaces any previously
* configured stdout destination.
*
* @param proc Pointer to an initialized subprocess handle
* @returns `true` on success, `false` if parameters were invalid
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stdout_null(
    JSLSubprocess* proc
);

/**
* Discard the subprocess's standard error.
*
* The child's stderr is redirected to the platform's null device
* (`/dev/null` on POSIX, `NUL` on Windows). Replaces any previously
* configured stderr destination.
*
* @param proc Pointer to an initialized subprocess handle
* @returns `true` on success, `false` if parameters were invalid
*/
JSL_WARN_UNUSED JSL_DEF bool jsl_subprocess_set_stderr_null(
    JSLSubprocess* proc
);

/**
* TODO: docs
*/
typedef enum
{
    JSL_SUBPROCESS_BAD_PARAMETERS = 0,
    JSL_SUBPROCESS_SUCCESS,
    JSL_SUBPROCESS_ALREADY_STARTED,
    JSL_SUBPROCESS_ALLOCATION_FAILED,
    JSL_SUBPROCESS_PIPE_FAILED,
    JSL_SUBPROCESS_SPAWN_FAILED,
    JSL_SUBPROCESS_IO_FAILED,
    JSL_SUBPROCESS_WAIT_FAILED,
    JSL_SUBPROCESS_KILLED_BY_SIGNAL,
    JSL_SUBPROCESS_TIMEOUT_REACHED,
    JSL_SUBPROCESS_PROBE_FAILED,

    JSL_SUBPROCESS_ENUM_COUNT
} JSLSubProcessResultEnum;

/**
* Spawn every proc in `procs[0 .. count-1]` and block until they all
* terminate, the optional timeout elapses, or a fatal wait error
* occurs. Each proc must have been configured via `jsl_subprocess_create`
* and the usual setters; none may have been started yet.
*
* This is the full-control variant. At most `parallelism_count` procs
* are alive at any time — the call pipelines spawning, so the next
* pending proc launches immediately when one of the live procs is
* reaped. Procs are spawned in strict array order. For the convenience
* default (parallelism = logical CPU count, infinite timeout), see
* `jsl_subprocess_run_blocking`.
*
* On success, every proc's `status` is `EXITED` (or `KILLED_BY_SIGNAL`
* when the child died from a signal on POSIX, or when this call's
* timeout-kill terminated the child on Windows) and `exit_code` carries
* the child's exit code (negated signal number on POSIX kill; -1 on
* Windows abnormal termination). Per-proc pump errors are recorded on
* `procs[i].last_errno`.
*
* Pre-flight rejection (no children are spawned):
*   - `procs == NULL` or `count <= 0` → `BAD_PARAMETERS`.
*   - `parallelism_count <= 0` → `BAD_PARAMETERS`.
*   - any `procs[i].sentinel` is invalid → `BAD_PARAMETERS`.
*   - any `procs[i].status` is not `NOT_STARTED` → `ALREADY_STARTED`.
*
* `parallelism_count` is silently clamped to `count`. On Windows it is
* additionally clamped to 16 so the live set fits within
* `WaitForMultipleObjectsEx`'s 64-handle ceiling (4 handles per proc).
*
* If the pre-flight passes, the call attempts to spawn every proc
* (subject to the timeout). A per-proc spawn or pre-spawn failure
* (pipe creation, allocation, posix_spawnp / CreateProcess error)
* marks that proc as `FAILED_TO_START` with `last_errno` set, but
* does *not* abort the batch — siblings still run to completion and
* the failed proc's parallelism slot is immediately reused for the
* next pending proc. If at least one proc failed this way, the call
* returns `SPAWN_FAILED` (unless a higher-priority code wins, see
* below).
*
* Not thread safe: only one thread may operate on a given `JSLSubprocess`
* at a time. None of the procs in `procs` may be touched by any other
* thread (including via other subprocess APIs) for the duration of this
* call.
*
* `timeout_ms` semantics (single shared deadline across the whole batch):
*   - < 0: wait forever for every child to terminate.
*   -   0: single-shot per wave — spawn up to `parallelism_count` procs,
*           pump once, kill any that are still running, then move on
*           to the next wave. Every proc in `procs[]` is spawned and
*           pumped at least once before this call returns.
*   - > 0: wait up to that many milliseconds for the entire pipeline.
*           Procs that were never spawned because the deadline expired
*           remain `NOT_STARTED`.
*
* When the deadline expires while at least one child is still running,
* every still-running child is forcefully killed (SIGKILL on POSIX,
* TerminateProcess on Windows), its I/O is drained, and it is reaped
* before this function returns. So on `TIMEOUT_REACHED`, every proc
* the call has spawned is in a terminal state — the caller does not
* need to follow up with a kill or another wait. Procs the call never
* got to spawn stay `NOT_STARTED`.
*
* Return value (in priority order, highest first):
*   - `BAD_PARAMETERS` / `ALREADY_STARTED` / `ALLOCATION_FAILED` —
*     pre-flight rejection; no children are spawned.
*   - `WAIT_FAILED` — the platform wait primitive itself failed
*     (`*out_errno` carries the OS error). Per-proc state is
*     indeterminate.
*   - `TIMEOUT_REACHED` — at least one proc was still running when the
*     deadline expired; those procs have been killed and reaped.
*   - `SPAWN_FAILED` — at least one proc failed to launch; surviving
*     procs ran to completion. Inspect each proc's `status` and
*     `last_errno` to find which failed and why.
*   - `IO_FAILED` — at least one proc's stdin/stdout/stderr pump
*     failed during the wait; affected procs have `last_errno` set.
*   - `SUCCESS` — every proc reached a terminal status without any
*     of the above. Inspect each `procs[i].status` (`EXITED` /
*     `KILLED_BY_SIGNAL`) and `exit_code` per proc — a child that
*     exited with a non-zero code or died from a signal still yields
*     `SUCCESS` here.
*
* `out_errno` may be NULL. It only receives top-level wait-primitive
* errnos; per-proc pump errnos land on `procs[i].last_errno`.
*
* The caller remains responsible for calling `jsl_subprocess_cleanup`
* on each proc after this function returns.
*
* Note: this is not equivalent to a series of `background_start` +
* `background_wait` calls. Differences:
*   - `run_blocking_options` spawns each child itself; `background_wait`
*     requires procs already started via `background_start`.
*   - On timeout, `run_blocking_options` kills, drains, and reaps every
*     still-running child; `background_wait` leaves them running
*     and returns `TIMEOUT_REACHED` for the caller to handle.
*
* `allocator` is used for short-lived helper allocations whose lifetime
* is bounded by this call (poll/handle-table scratch buffers and similar).
* It is independent of any `procs[i].allocator` and is not used for
* per-proc resources. Callers with mixed-allocator procs should pass a
* dedicated scratch allocator with enough headroom for the batch rather
* than sharing one of the per-proc allocators, which may be near-full.
*
* @param allocator         Allocator for short-lived helper buffers shared
*                          across the whole batch
* @param procs             Pointer to an array of configured subprocess
*                          handles
* @param count             Number of elements in `procs`
* @param parallelism_count Maximum number of procs alive at one time;
*                          must be > 0
* @param timeout_ms        Maximum time to wait for the entire batch,
*                          in milliseconds
* @param out_errno         Optional pointer that receives the system
*                          errno on a top-level wait failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessResultEnum jsl_subprocess_run_blocking_options(
    JSLAllocatorInterface allocator,
    JSLSubprocess* procs,
    int32_t count,
    int32_t parallelism_count,
    int32_t timeout_ms,
    int32_t* out_errno
);

/**
* Convenience wrapper around `jsl_subprocess_run_blocking_options` that
* picks sensible defaults: parallelism is set to
* `jsl_get_logical_processor_count()` and the timeout is infinite. All
* other semantics (including pre-flight rejection, per-proc spawn
* failure handling, and return-code priority) are identical.
*
* If the logical-processor probe itself fails, this function returns
* `JSL_SUBPROCESS_PROBE_FAILED` with `*out_errno` set to the probe's
* errno; no procs are spawned.
*
* Example — feed stdin, capture stdout/stderr for a single proc:
*
* ```c
* JSLStringBuilder stdout_sb, stderr_sb;
* jsl_string_builder_init(&stdout_sb, out_alloc, 4096);
* jsl_string_builder_init(&stderr_sb, err_alloc, 4096);
*
* JSLSubprocess proc = {0};
* jsl_subprocess_create(&proc, alloc, jsl_cstr_to_memory("my-tool"));
* jsl_subprocess_set_stdin_memory(&proc, jsl_cstr_to_memory("input data\n"));
* jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&stdout_sb));
* jsl_subprocess_set_stderr_sink(&proc, jsl_string_builder_output_sink(&stderr_sb));
*
* jsl_subprocess_run_blocking(alloc, &proc, 1, NULL);
*
* JSLImmutableMemory out = jsl_string_builder_get_string(&stdout_sb);
* JSLImmutableMemory err = jsl_string_builder_get_string(&stderr_sb);
*
* // use out, err, proc.exit_code ...
*
* jsl_subprocess_cleanup(&proc);
* ```
*
* @param allocator   Allocator for short-lived helper buffers shared
*                    across the whole batch
* @param procs       Pointer to an array of configured subprocess handles
* @param count       Number of elements in `procs`
* @param out_errno   Optional pointer that receives the system errno on
*                    a top-level wait failure or probe failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessResultEnum jsl_subprocess_run_blocking(
    JSLAllocatorInterface allocator,
    JSLSubprocess* procs,
    int32_t count,
    int32_t* out_errno
);

/**
* Spawn the previously-configured child and return immediately.
*
* On success, the process is running and the subprocess handle carries the
* OS-level pid/HANDLE and parent-side pipe fds/handles needed for the other
* background-mode functions. On failure, the subprocess status is set to
* `JSL_SUBPROCESS_STATUS_FAILED_TO_START`, `is_background` stays `false`,
* and no OS resources are leaked.
*
* A subprocess may only be started once. Passing a handle whose status is
* not `JSL_SUBPROCESS_STATUS_NOT_STARTED` returns
* `JSL_SUBPROCESS_ALREADY_STARTED`.
*
* Example — feed stdin, capture stdout/stderr, poll with a 20 ms timeout:
*
* ```c
* JSLStringBuilder stdout_sb, stderr_sb;
* jsl_string_builder_init(&stdout_sb, out_alloc, 4096);
* jsl_string_builder_init(&stderr_sb, err_alloc, 4096);
*
* JSLSubprocess proc;
* jsl_subprocess_create(&proc, alloc, jsl_cstr_to_memory("my-tool"));
* jsl_subprocess_set_stdin_memory(&proc, jsl_cstr_to_memory("input data\n"));
* jsl_subprocess_set_stdout_sink(&proc, jsl_string_builder_output_sink(&stdout_sb));
* jsl_subprocess_set_stderr_sink(&proc, jsl_string_builder_output_sink(&stderr_sb));
*
* jsl_subprocess_background_start(&proc, NULL);
*
* JSLSubProcessStatusEnum status = JSL_SUBPROCESS_STATUS_RUNNING;
* int32_t exit_code = -1;
* bool running = true;
* while (running)
* {
*     jsl_subprocess_background_send_stdin(&proc, NULL);
*     jsl_subprocess_background_receive_output(&proc, NULL);
*     jsl_subprocess_background_poll(&proc, 20, &status, &exit_code, NULL);
*     running = status == JSL_SUBPROCESS_STATUS_RUNNING;
* }
* // Final drain: scoop up any output buffered after the child exited.
* jsl_subprocess_background_receive_output(&proc, NULL);
*
* JSLImmutableMemory out = jsl_string_builder_get_string(&stdout_sb);
* JSLImmutableMemory err = jsl_string_builder_get_string(&stderr_sb);
* // use out, err, exit_code ...
*
* jsl_subprocess_cleanup(&proc);
* ```
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessResultEnum jsl_subprocess_background_start(
    JSLSubprocess* proc,
    int32_t* out_errno
);

/**
* Non-blocking lifecycle status check for a background subprocess.
*
* Writes the current lifecycle state to `*out_status`. When the status is
* `EXITED` or `KILLED_BY_SIGNAL`, also writes `*out_exit_code` (exit code
* on EXITED, negated signal number on KILLED_BY_SIGNAL; -1 on Windows for
* abnormal termination).
*
* `timeout_ms` semantics:
*   - 0: purely non-blocking; returns immediately with current status.
*   - > 0: block up to that many milliseconds waiting for a state change.
*   - < 0: block until the child exits.
*
* Returns `JSL_SUBPROCESS_BAD_PARAMETERS` if the subprocess was not started
* via `jsl_subprocess_background_start`. Idempotent once the child has
* exited: subsequent calls re-write the same status and exit code.
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessResultEnum jsl_subprocess_background_poll(
    JSLSubprocess* proc,
    int32_t timeout_ms,
    JSLSubProcessStatusEnum* out_status,
    int32_t* out_exit_code,
    int32_t* out_errno
);

/**
* Non-blocking write of the next chunk of `stdin_memory` to the child.
*
* No-op if stdin_kind != `STDIN_MEMORY`, if the buffer is already drained,
* or if the write end has already been closed. When the buffer is fully
* written OR the child has closed its read end, closes the parent-side
* write end. Safe to call repeatedly. Returns `BAD_PARAMETERS` if the
* subprocess is not a background one.
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessResultEnum jsl_subprocess_background_send_stdin(
    JSLSubprocess* proc,
    int32_t* out_errno
);

/**
* Non-blocking drain of stdout and stderr pipes into their configured
* `JSLOutputSink`s. Reads whatever is currently buffered, up to an internal
* 4 KiB scratch buffer per stream per call. On EOF the parent-side read
* end is closed and the corresponding `*_eof_seen` flag is set.
*
* Returns `BAD_PARAMETERS` if the subprocess is not a background one.
* Returns `IO_FAILED` if a non-retryable read error occurs.
*
* The caller is responsible for calling this often enough that a chatty
* child does not fill the kernel pipe buffer and block.
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessResultEnum jsl_subprocess_background_receive_output(
    JSLSubprocess* proc,
    int32_t* out_errno
);

/**
* Forcefully terminate the child: SIGKILL on POSIX, TerminateProcess on
* Windows. The signal is sent only — the caller must continue polling
* until the status transitions to `EXITED` / `KILLED_BY_SIGNAL`, and must
* continue pumping output to drain the pipes.
*
* Returns `SUCCESS` if the process has already exited (no-op). Returns
* `BAD_PARAMETERS` if the subprocess is not a background one.
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessResultEnum jsl_subprocess_background_kill(
    JSLSubprocess* proc,
    int32_t* out_errno
);

/**
* Block the calling thread until every valid running background
* subprocess in `procs[0 .. count-1]` has terminated, the optional
* timeout elapses, or a fatal wait error occurs. While waiting, this
* function continuously pumps each proc's stdin and drains stdout/
* stderr into their configured sinks, so callers do not need to spin
* their own loop.
*
* A proc is *ignored* (not counted toward completion, not touched) if
* any of:
*   - its sentinel check fails (uninitialized / already cleaned up),
*   - it is not a background proc (`is_background == false`),
*   - its status is not `RUNNING` AND not already terminal.
*
* Procs already in `EXITED` / `KILLED_BY_SIGNAL` are considered
* trivially satisfied and skipped silently. If every proc is ignored
* the function returns `SUCCESS` immediately.
*
* On return:
*   - SUCCESS: every valid proc reached a terminal status; inspect
*     `procs[i].status` and `procs[i].exit_code` per proc.
*   - TIMEOUT_REACHED: at least one valid proc was still RUNNING when
*     the timeout expired; those procs are left running and the caller
*     is responsible for killing or continuing to wait.
*   - IO_FAILED: at least one proc's stdin/stdout/stderr pump errored
*     during the loop; the errored proc(s) have `last_errno` set and
*     are removed from the wait set (but the loop continues for the
*     others). Inspect each proc to find which.
*   - WAIT_FAILED: the platform wait primitive itself failed. `*out_errno`
*     carries the OS error; per-proc state is indeterminate.
*   - BAD_PARAMETERS: `procs == NULL` or `count <= 0`.
*
* `timeout_ms`:
*   -  0: poll once and return (essentially non-blocking).
*   - >0: wait up to that many milliseconds.
*   - <0: wait forever.
*
* `out_errno` may be NULL. It only receives top-level wait-primitive
* errnos; per-proc pump errnos land on `procs[i].last_errno`.
*
* There is no caller-visible cap on `count`. On Windows the
* implementation batches internally into groups of at most 16 procs at
* a time to stay under the `WaitForMultipleObjects` 64-handle ceiling;
* callers do not observe group boundaries.
*
* The caller remains responsible for calling `jsl_subprocess_cleanup`
* on each proc after this function returns.
*
* Not thread safe: only one thread may operate on a given `JSLSubprocess`
* at a time. Each proc in `procs` must not be touched by any other thread
* (including via other subprocess APIs) for the duration of this call.
*
* `allocator` is used for short-lived helper allocations whose lifetime
* is bounded by this call (poll/handle-table scratch buffers and similar).
* It is independent of any `procs[i].allocator` and is not used for
* per-proc resources. Callers with mixed-allocator procs should pass a
* dedicated scratch allocator with enough headroom for the batch rather
* than sharing one of the per-proc allocators, which may be near-full.
*
* @param allocator   Allocator for short-lived helper buffers shared
*                    across the whole batch
* @param procs       Pointer to an array of background subprocess handles
* @param count       Number of elements in `procs`
* @param timeout_ms  Maximum time to wait for the children, in milliseconds
* @param out_errno   Optional pointer that receives the system errno on
*                    a top-level wait failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLSubProcessResultEnum jsl_subprocess_background_wait(
    JSLAllocatorInterface allocator,
    JSLSubprocess* procs,
    int32_t count,
    int32_t timeout_ms,
    int32_t* out_errno
);

/**
* Release all memory allocated for the subprocess's configuration (arguments,
* environment variables, working directory, duplicated executable path, and
* any captured stdin buffer) and invalidate the handle by zeroing its
* sentinel.
*
* Must only be called on a subprocess that is no longer running, i.e. one
* whose status is not `JSL_SUBPROCESS_STATUS_RUNNING`. Calling this on a
* subprocess whose `jsl_subprocess_create` call failed, or one that has
* already been cleaned up, is a no-op.
*
* After this returns, the handle's sentinel is zero and all other subprocess
* functions will reject the handle with `BAD_PARAMETERS`.
*
* @param proc Pointer to the subprocess handle to clean up, may be null
*/
JSL_DEF void jsl_subprocess_cleanup(JSLSubprocess* proc);

/**
* Get the number of logical processors available on the host machine.
*
* On Windows this returns the count across all processor groups, so systems
* with more than 64 logical processors are reported accurately. On POSIX
* systems this uses `sysconf(_SC_NPROCESSORS_ONLN)`, which reports the
* number of processors currently online.
*
* @param out_errno Optional pointer that receives the system errno (POSIX) or
*                  the result of `GetLastError` (Windows) on failure
* @returns The number of logical processors, or -1 on error
*/
JSL_WARN_UNUSED JSL_DEF int32_t jsl_get_logical_processor_count(int32_t* out_errno);


#ifdef __cplusplus
} /* extern "C" */
#endif

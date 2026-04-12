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
* Result codes for `jsl_spawn_subprocess`.
*/
typedef enum
{
    JSL_SPAWN_SUBPROCESS_BAD_PARAMETERS = 0,
    JSL_SPAWN_SUBPROCESS_SUCCESS,
    JSL_SPAWN_SUBPROCESS_PATH_TOO_LONG,
    JSL_SPAWN_SUBPROCESS_EXECUTABLE_NOT_FOUND,
    JSL_SPAWN_SUBPROCESS_PERMISSION_DENIED,
    JSL_SPAWN_SUBPROCESS_PIPE_FAILED,
    JSL_SPAWN_SUBPROCESS_FORK_FAILED,
    JSL_SPAWN_SUBPROCESS_ERROR_UNKNOWN,

    JSL_SPAWN_SUBPROCESS_ENUM_COUNT
} JSLSpawnSubprocessResultEnum;

/**
* The current state of a spawned subprocess. After a successful call to
* `jsl_spawn_subprocess` the status is `RUNNING`. Future wait/poll
* functions transition it to `EXITED`, `SIGNALED`, or `FAILED`.
*/
typedef enum
{
    JSL_SUBPROCESS_STATUS_NOT_STARTED = 0,
    JSL_SUBPROCESS_STATUS_RUNNING,
    JSL_SUBPROCESS_STATUS_EXITED,
    JSL_SUBPROCESS_STATUS_SIGNALED,
    JSL_SUBPROCESS_STATUS_FAILED,

    JSL_SUBPROCESS_STATUS_ENUM_COUNT
} JSLSubprocessStatusEnum;

/**
* How the child process's stdin is wired up.
*/
typedef enum
{
    /** Inherit the parent process's stdin (the default). */
    JSL_SUBPROCESS_STDIN_INHERIT = 0,
    /** Connect stdin to /dev/null on POSIX or NUL on Windows. */
    JSL_SUBPROCESS_STDIN_NONE,
    /**
    * Provide stdin contents as an in-memory buffer. The buffer is not
    * written by `jsl_spawn_subprocess` itself; the spawn call sets up a
    * pipe and stores the pending data on `JSLSubprocess` so a future
    * wait/communicate function can stream it to the child. The buffer
    * pointed to by `stdin_memory` must remain valid until that drain
    * step has completed.
    */
    JSL_SUBPROCESS_STDIN_MEMORY,
    /** Use an existing file descriptor as the child's stdin. */
    JSL_SUBPROCESS_STDIN_FD,

    JSL_SUBPROCESS_STDIN_ENUM_COUNT
} JSLSubprocessStdinKind;

/**
* How the child process's stdout or stderr is wired up.
*/
typedef enum
{
    /** Inherit the parent process's stream (the default). */
    JSL_SUBPROCESS_OUTPUT_INHERIT = 0,
    /** Discard the output by routing to /dev/null or NUL. */
    JSL_SUBPROCESS_OUTPUT_NONE,
    /** Redirect the output to an existing file descriptor. */
    JSL_SUBPROCESS_OUTPUT_FD,
    /**
    * Capture the output to a `JSLOutputSink`. The spawn call sets up a
    * pipe and stores the sink on `JSLSubprocess`; a future
    * wait/communicate function reads from the pipe and forwards bytes
    * into the sink.
    */
    JSL_SUBPROCESS_OUTPUT_SINK,

    JSL_SUBPROCESS_OUTPUT_ENUM_COUNT
} JSLSubprocessOutputKind;

/**
* Configuration options for `jsl_spawn_subprocess`. Zero-initialize this
* struct and then fill in the fields you care about - any field left at
* its zero value selects the most "do nothing special" default
* (inheriting the parent's working directory, stdin, stdout, and stderr).
*
* On Windows, file descriptors are the C runtime descriptors returned by
* functions like `_open` / `_pipe` from `<io.h>`, not raw `HANDLE`s. The
* implementation translates them to `HANDLE`s via `_get_osfhandle` when
* invoking `CreateProcess`.
*/
typedef struct JSLSpawnSubprocessOptions
{
    /** Path to the executable to run. Required. */
    JSLImmutableMemory executable_path;

    /** When `true`, change to `working_directory` in the child process before exec. */
    bool change_working_directory;
    JSLImmutableMemory working_directory;

    JSLSubprocessStdinKind stdin_kind;
    /** Used when `stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY`. */
    JSLImmutableMemory stdin_memory;
    /** Used when `stdin_kind == JSL_SUBPROCESS_STDIN_FD`. */
    int32_t stdin_fd;

    JSLSubprocessOutputKind stdout_kind;
    /** Used when `stdout_kind == JSL_SUBPROCESS_OUTPUT_FD`. */
    int32_t stdout_fd;
    /** Used when `stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK`. */
    JSLOutputSink stdout_sink;

    JSLSubprocessOutputKind stderr_kind;
    /** Used when `stderr_kind == JSL_SUBPROCESS_OUTPUT_FD`. */
    int32_t stderr_fd;
    /** Used when `stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK`. */
    JSLOutputSink stderr_sink;
} JSLSpawnSubprocessOptions;

/**
* A handle to a spawned child process. Populate by passing a pointer to
* `jsl_spawn_subprocess`. The struct keeps both the OS-level identifiers
* (process id, and on Windows the process `HANDLE`) and the internal
* state needed by the future wait/communicate APIs - pipe ends for any
* captured streams, the pending stdin buffer, and the user-provided
* sinks.
*
* The fields below the public `status` / `process_id` / `exit_code`
* members are private to the implementation. Treat them as opaque.
*/
typedef struct JSLSubprocess
{
    JSLSubprocessStatusEnum status;

    #if JSL_IS_WINDOWS
        HANDLE process_handle;
        DWORD process_id;
    #elif JSL_IS_POSIX
        pid_t process_id;
    #endif

    /** The child's exit status once it has exited. `-1` while still running. */
    int32_t exit_code;

    /** The signal number that terminated the child if `status == SIGNALED`. POSIX only. */
    int32_t terminating_signal;

    /* --- private state used by future wait/communicate functions --- */
    int32_t stdin_write_fd;
    int32_t stdout_read_fd;
    int32_t stderr_read_fd;
    JSLImmutableMemory pending_stdin;
    JSLOutputSink stdout_sink;
    JSLOutputSink stderr_sink;
    bool has_stdin_pipe;
    bool capture_stdout;
    bool capture_stderr;
} JSLSubprocess;

/**
* Spawn a new child process described by `options`.
*
* On success the child is running and `out_subprocess` is populated with
* the process id and any state needed to interact with it later. The
* function never blocks waiting for the child - it only sets up the
* requested I/O wiring and starts the program.
*
* When the options request capturing stdout or stderr to a
* `JSLOutputSink`, this call sets up the pipe but does not drain it; a
* future wait/communicate function reads from the pipe into the sink.
* Likewise, when stdin is provided as a `JSLImmutableMemory` buffer, the
* spawn call sets up the pipe and stores the pending buffer on
* `out_subprocess` for a future drain step. The buffer must remain valid
* until that drain step has completed.
*
* Arguments and environment variables are not yet supported. The child
* is invoked with `argv = { executable_path, NULL }` and inherits the
* parent's environment.
*
* @param options How to spawn the child
* @param out_subprocess Output handle, must not be null
* @param out_errno Optional pointer that receives the system error code on failure
* @returns A result enum describing the outcome
*/
JSL_WARN_UNUSED JSL_DEF JSLSpawnSubprocessResultEnum jsl_spawn_subprocess(
    JSLSpawnSubprocessOptions options,
    JSLSubprocess* out_subprocess,
    int32_t* out_errno
);


#ifdef __cplusplus
} /* extern "C" */
#endif

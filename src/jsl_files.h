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
 * Copyright (c) 2025 Jack Stouffer
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

#ifndef JSL_FILES_H_INCLUDED

#define JSL_FILES_H_INCLUDED

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "jsl_core.h"

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
    #include <sys\stat.h>

#elif JSL_IS_POSIX

    #include <errno.h>
    #include <limits.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>

#endif

typedef enum
{
    JSL_GET_FILE_SIZE_BAD_PARAMETERS = 0,
    JSL_GET_FILE_SIZE_OK,
    JSL_GET_FILE_SIZE_NOT_FOUND,
    JSL_GET_FILE_SIZE_NOT_REGULAR_FILE,

    JSL_GET_FILE_SIZE_ENUM_COUNT
} JSLGetFileSizeResultEnum;

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

typedef enum
{
    JSL_FILE_WRITE_BAD_PARAMETERS = 0,
    JSL_FILE_WRITE_SUCCESS,
    JSL_FILE_WRITE_COULD_NOT_OPEN,
    JSL_FILE_WRITE_COULD_NOT_WRITE,
    JSL_FILE_WRITE_COULD_NOT_CLOSE,

    JSL_FILE_WRITE_ENUM_COUNT
} JSLWriteFileResultEnum;

typedef enum {
    JSL_FILE_TYPE_UNKNOWN = 0,
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
    JSLFatPtr path,
    int64_t* out_size,
    int32_t* out_os_error_code
);

/**
    * Load the contents of the file at `path` into a newly allocated buffer
    * from the given arena. The buffer will be the exact size of the file contents.
    *
    * If the arena does not have enough space,
    */
JSL_WARN_UNUSED JSL_DEF JSLLoadFileResultEnum jsl_load_file_contents(
    JSLArena* arena,
    JSLFatPtr path,
    JSLFatPtr* out_contents,
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
    JSLFatPtr* buffer,
    JSLFatPtr path,
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
    JSLFatPtr contents,
    JSLFatPtr path,
    int64_t* bytes_written,
    int32_t* out_errno
);

/**
* Format a string using the JSL formatter and write the result to a `FILE*`,
* most often this will be `stdout`.
*
* Streams that reject writes (for example, read-only streams or closed
* pipes) cause the function to return `false`. Passing a `NULL` file handle,
* a `NULL` format pointer, or a negative format length also causes failure.
*
* @param out Destination stream
* @param fmt Format string
* @param ... Format args
* @returns `true` when formatting and writing succeeds, otherwise `false`
*/
JSL__ASAN_OFF JSL_DEF bool jsl_format_file(FILE* out, JSLFatPtr fmt, ...);


#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef JSL_FILES_IMPLEMENTATION

JSLGetFileSizeResultEnum jsl_get_file_size(
    JSLFatPtr path,
    int64_t* out_size,
    int32_t* out_os_error_code
)
{
    char path_buffer[FILENAME_MAX + 1];
    int64_t temp_size = 0;
    bool proceed = false;

    proceed = (path.data != NULL && path.length > 0 && out_size != NULL);
    JSLGetFileSizeResultEnum result = proceed
        ? JSL_GET_FILE_SIZE_OK : JSL_GET_FILE_SIZE_BAD_PARAMETERS;

    #if JSL_IS_WINDOWS

        struct _stat64 st_win = {0};
        int32_t stat_ret = -1;
        bool stat_ok = false;
        bool is_regular = false;

        if (proceed)
        {
            JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
            path_buffer[path.length] = '\0';

            stat_ret = _stat64(path_buffer, &st_win);
            stat_ok = (stat_ret == 0);
            proceed = stat_ok;
            result = stat_ok ? JSL_GET_FILE_SIZE_OK : JSL_GET_FILE_SIZE_NOT_FOUND;
        }

        if (!proceed && out_os_error_code != NULL)
        {
            *out_os_error_code = errno;
        }

        if (proceed)
        {
            is_regular = (st_win.st_mode & _S_IFREG) != 0;
            temp_size = (int64_t) st_win.st_size;
            result = is_regular ? JSL_GET_FILE_SIZE_OK : JSL_GET_FILE_SIZE_NOT_REGULAR_FILE;
        }

    #elif JSL_IS_POSIX

        struct stat st_posix;
        int32_t stat_ret = -1;
        bool stat_ok = false;
        bool is_regular = false;

        if (proceed)
        {
            JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
            path_buffer[path.length] = '\0';

            stat_ret = stat(path_buffer, &st_posix);
            stat_ok = (stat_ret == 0);
            proceed = stat_ok;
            result = stat_ok ? JSL_GET_FILE_SIZE_OK : JSL_GET_FILE_SIZE_NOT_FOUND;
        }

        if (!proceed && out_os_error_code != NULL)
        {
            *out_os_error_code = errno;
        }

        if (proceed)
        {
            is_regular = (st_posix.st_mode & S_IFMT) == S_IFREG;
            temp_size = (int64_t) st_posix.st_size;
            result = is_regular ? JSL_GET_FILE_SIZE_OK : JSL_GET_FILE_SIZE_NOT_REGULAR_FILE;
        }

    #else
        JSL_ASSERT(0 && "File utils only work on Windows or POSIX platforms.");
    #endif

    if (result == JSL_GET_FILE_SIZE_OK)
    {
        *out_size = temp_size;
    }

    return result;
}

static inline int64_t jsl__get_file_size_from_fileno(int32_t file_descriptor)
{
    int64_t result_size = -1;
    bool stat_success = false;

    #if JSL_IS_WINDOWS

        struct _stat64 file_info;
        int stat_result = _fstat64(file_descriptor, &file_info);
        if (stat_result == 0)
        {
            stat_success = true;
            result_size = file_info.st_size;
        }

    #elif JSL_IS_POSIX
        struct stat file_info;
        int stat_result = fstat(file_descriptor, &file_info);
        if (stat_result == 0)
        {
            stat_success = true;
            result_size = (int64_t) file_info.st_size;
        }

    #endif

    if (!stat_success)
    {
        result_size = -1;
    }

    return result_size;
}

JSLLoadFileResultEnum jsl_load_file_contents(
    JSLArena* arena,
    JSLFatPtr path,
    JSLFatPtr* out_contents,
    int32_t* out_errno
)
{
    JSLLoadFileResultEnum res = JSL_FILE_LOAD_BAD_PARAMETERS;
    char path_buffer[FILENAME_MAX + 1];

    bool got_path = false;
    if (path.data != NULL
        && path.length > 0
        && path.length < FILENAME_MAX
        && arena != NULL
        && arena->current != NULL
        && arena->start != NULL
        && arena->end != NULL
        && out_contents != NULL
    )
    {
        // File system APIs require a null terminated string

        JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
        path_buffer[path.length] = '\0';
        got_path = true;
    }

    int32_t file_descriptor = -1;
    bool opened_file = false;
    if (got_path)
    {
        #if JSL_IS_WINDOWS
            errno_t open_err = _sopen_s(
                &file_descriptor,
                path_buffer,
                _O_BINARY,
                _SH_DENYNO,
                _S_IREAD
            );
            opened_file = (open_err == 0);
        #elif JSL_IS_POSIX
            file_descriptor = open(path_buffer, 0);
            opened_file = file_descriptor > -1;
        #else
            #error "Unsupported platform"
        #endif
    }

    int64_t file_size = -1;
    bool got_file_size = false;
    if (opened_file)
    {
        file_size = jsl__get_file_size_from_fileno(file_descriptor);
        got_file_size = file_size > -1;
    }
    else
    {
        res = JSL_FILE_LOAD_COULD_NOT_OPEN;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    JSLFatPtr allocation = {0};
    bool got_memory = false;
    if (got_file_size)
    {
        allocation = jsl_arena_allocate(arena, file_size, false);
        got_memory = allocation.data != NULL && allocation.length >= file_size;
    }
    else
    {
        res = JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    int64_t read_res = -1;
    bool did_read_data = false;
    if (got_memory)
    {
        #if JSL_IS_WINDOWS
            read_res = (int64_t) _read(file_descriptor, allocation.data, (unsigned int) file_size);
        #elif JSL_IS_POSIX
            read_res = (int64_t) read(file_descriptor, allocation.data, (size_t) file_size);
        #else
            #error "Unsupported platform"
        #endif

        did_read_data = read_res > -1;
    }
    else
    {
        res = JSL_FILE_LOAD_COULD_NOT_GET_MEMORY;
    }

    if (did_read_data)
    {
        out_contents->data = allocation.data;
        out_contents->length = read_res;
    }
    else
    {
        res = JSL_FILE_LOAD_READ_FAILED;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    bool did_close = false;
    if (opened_file)
    {
        #if JSL_IS_WINDOWS
            int32_t close_res = _close(file_descriptor);
        #elif JSL_IS_POSIX
            int32_t close_res = close(file_descriptor);
        #else
            #error "Unsupported platform"
        #endif

        did_close = close_res > -1;
    }

    if (opened_file && did_close)
    {
        res = JSL_FILE_LOAD_SUCCESS;
    }
    if (opened_file && !did_close)
    {
        res = JSL_FILE_LOAD_CLOSE_FAILED;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    return res;
}

JSLLoadFileResultEnum jsl_load_file_contents_buffer(
    JSLFatPtr* buffer,
    JSLFatPtr path,
    int32_t* out_errno
)
{
    char path_buffer[FILENAME_MAX + 1];
    JSLLoadFileResultEnum res = JSL_FILE_LOAD_BAD_PARAMETERS;

    bool got_path = false;
    if (path.data != NULL
        && path.length > 0
        && path.length < FILENAME_MAX
        && buffer != NULL
        && buffer->data != NULL
        && buffer->length > 0
    )
    {
        // File system APIs require a null terminated string

        JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
        path_buffer[path.length] = '\0';
        got_path = true;
    }

    int32_t file_descriptor = -1;
    bool opened_file = false;
    if (got_path)
    {
        #if JSL_IS_WINDOWS
            errno_t open_err = _sopen_s(
                &file_descriptor,
                path_buffer,
                _O_BINARY,
                _SH_DENYNO,
                _S_IREAD
            );
            opened_file = (open_err == 0);
        #elif JSL_IS_POSIX
            file_descriptor = open(path_buffer, 0);
            opened_file = file_descriptor > -1;
        #else
            #error "Unsupported platform"
        #endif
    }

    int64_t file_size = -1;
    bool got_file_size = false;
    if (opened_file)
    {
        file_size = jsl__get_file_size_from_fileno(file_descriptor);
        got_file_size = file_size > -1;
    }
    else
    {
        res = JSL_FILE_LOAD_COULD_NOT_OPEN;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    int64_t read_res = -1;
    bool did_read_data = false;
    if (got_file_size)
    {
        int64_t read_size = JSL_MIN(file_size, buffer->length);

        #if JSL_IS_WINDOWS
            read_res = (int64_t) _read(file_descriptor, buffer->data, (unsigned int) read_size);
        #elif JSL_IS_POSIX
            read_res = (int64_t) read(file_descriptor, buffer->data, (size_t) read_size);
        #else
            #error "Unsupported platform"
        #endif

        did_read_data = read_res > -1;
    }
    else
    {
        res = JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    if (did_read_data)
    {
        buffer->data += read_res;
        buffer->length -= read_res;
    }
    else
    {
        res = JSL_FILE_LOAD_READ_FAILED;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    bool did_close = false;
    if (opened_file)
    {
        #if JSL_IS_WINDOWS
            int32_t close_res = _close(file_descriptor);
        #elif JSL_IS_POSIX
            int32_t close_res = close(file_descriptor);
        #else
            #error "Unsupported platform"
        #endif

        did_close = close_res > -1;
    }

    if (opened_file && did_close)
    {
        res = JSL_FILE_LOAD_SUCCESS;
    }
    if (opened_file && !did_close)
    {
        res = JSL_FILE_LOAD_CLOSE_FAILED;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    return res;
}

JSLWriteFileResultEnum jsl_write_file_contents(
    JSLFatPtr contents,
    JSLFatPtr path,
    int64_t* out_bytes_written,
    int32_t* out_errno
)
{
    char path_buffer[FILENAME_MAX + 1];
    JSLWriteFileResultEnum res = JSL_FILE_WRITE_BAD_PARAMETERS;

    bool got_path = false;
    if (path.data != NULL
        && contents.data != NULL
        && contents.length > 0
    )
    {
        JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
        path_buffer[path.length] = '\0';
        got_path = true;
    }

    int32_t file_descriptor = -1;
    bool opened_file = false;
    if (got_path)
    {
        #if JSL_IS_WINDOWS
            errno_t open_err = _sopen_s(
                &file_descriptor,
                path_buffer,
                _O_CREAT,
                _SH_DENYNO,
                _S_IREAD | _S_IWRITE
            );
            opened_file = (open_err == 0);
        #elif JSL_IS_POSIX
            file_descriptor = open(path_buffer, O_CREAT, S_IRUSR | S_IWUSR);
            opened_file = file_descriptor > -1;
        #endif
    }

    int64_t write_res = -1;
    if (opened_file)
    {
        #if JSL_IS_WINDOWS
            write_res = _write(
                file_descriptor,
                contents.data,
                (unsigned int) contents.length
            );
        #elif JSL_IS_POSIX
            write_res = write(
                file_descriptor,
                contents.data,
                (size_t) contents.length
            );
        #endif
    }
    else
    {
        res = JSL_FILE_WRITE_COULD_NOT_OPEN;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    if (write_res > 0)
    {
        *out_bytes_written = write_res;
    }
    else
    {
        res = JSL_FILE_WRITE_COULD_NOT_WRITE;
        if (out_errno != NULL)
            *out_errno = errno;
    }

    int32_t close_res = -1;
    if (opened_file)
    {
        #if JSL_IS_WINDOWS
            close_res = _close(file_descriptor);
        #elif JSL_IS_POSIX
            close_res = close(file_descriptor);
        #endif

        if (close_res < 0)
        {
            res = JSL_FILE_WRITE_COULD_NOT_CLOSE;
            if (out_errno != NULL)
                *out_errno = errno;
        }
    }

    return res;
}

struct JSL__FormatOutContext
{
    FILE* out;
    bool failure_flag;
    uint8_t buffer[JSL_FORMAT_MIN_BUFFER];
};

static uint8_t* format_out_callback(uint8_t *buf, void *user, int64_t len)
{
    struct JSL__FormatOutContext* context = (struct JSL__FormatOutContext*) user;

    int64_t written = (int64_t) fwrite(
        buf,
        sizeof(uint8_t),
        (size_t) len,
        context->out
    );
    if (written == len)
    {
        return context->buffer;
    }
    else
    {
        context->failure_flag = true;
        return NULL;
    }
}

JSL__ASAN_OFF bool jsl_format_file(FILE* out, JSLFatPtr fmt, ...)
{
    if (out == NULL || fmt.data == NULL || fmt.length < 0)
        return false;

    va_list va;
    va_start(va, fmt);

    struct JSL__FormatOutContext context;
    context.out = out;
    context.failure_flag = false;

    jsl_format_callback(
        format_out_callback,
        &context,
        context.buffer,
        fmt,
        va
    );

    va_end(va);

    return !context.failure_flag;
}

#endif // JSL_FILES_IMPLEMENTATION

#endif // JSL_FILES_H_INCLUDED

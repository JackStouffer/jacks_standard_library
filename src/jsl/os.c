/**
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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
    #include <stdbool.h>
#endif

#include "core.h"
#include "allocator.h"

// nftw(3) and its flags (FTW_DEPTH, FTW_PHYS, FTW_DP) require _XOPEN_SOURCE >= 500
#if JSL_IS_LINUX
    #if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 500
        #error "JSL's os.c requires a _XOPEN_SOURCE > 500 to be defined before including system headers when using Linux"
    #endif
#endif

// <signal.h> on macOS uses NSIG in public extern declarations, which is
// only exposed when _DARWIN_C_SOURCE is defined before any system header
// is included. JSL is a source library so we cannot set this macro
// ourselves without risk of conflicting with the host program.
#if defined(__APPLE__)
    #if !defined(_DARWIN_C_SOURCE)
        #error "JSL's os.c requires _DARWIN_C_SOURCE to be defined before including system headers on macOS"
    #endif
#endif

#include "os.h"

#if JSL_IS_POSIX
    #include <sys/wait.h>
    #include <poll.h>
    #include <signal.h>
    #include <spawn.h>
    #include <string.h>
    #include <time.h>

// `environ` is a POSIX global. `<unistd.h>` only declares it when the
// program's feature-test macros ask for it, so declare it ourselves to
// avoid depending on the surrounding program's macro setup.
extern char** environ;
#endif

#define JSL__DIR_ITERATOR_PRIVATE_SENTINEL 9523783263672821879U

JSLGetFileSizeResultEnum jsl_get_file_size(
    JSLImmutableMemory path,
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
    JSLAllocatorInterface allocator,
    JSLImmutableMemory path,
    JSLImmutableMemory* out_contents,
    int32_t* out_errno
)
{
    char path_buffer[FILENAME_MAX + 1];
    bool proceed = false;

    proceed = (path.data != NULL
        && path.length > 0
        && path.length < FILENAME_MAX
        && out_contents != NULL);
    JSLLoadFileResultEnum res = proceed ? JSL_FILE_LOAD_SUCCESS : JSL_FILE_LOAD_BAD_PARAMETERS;

    int32_t file_descriptor = -1;
    bool opened_file = false;

    if (proceed)
    {
        // File system APIs require a null terminated string
        JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
        path_buffer[path.length] = '\0';

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

        proceed = opened_file;
        res = proceed ? JSL_FILE_LOAD_SUCCESS : JSL_FILE_LOAD_COULD_NOT_OPEN;
        if (!proceed && out_errno != NULL)
            *out_errno = errno;
    }

    int64_t file_size = -1;
    bool got_file_size = false;

    if (proceed)
    {
        file_size = jsl__get_file_size_from_fileno(file_descriptor);
        got_file_size = file_size > -1;
        proceed = got_file_size;
        res = proceed ? JSL_FILE_LOAD_SUCCESS : JSL_FILE_LOAD_COULD_NOT_GET_FILE_SIZE;
        if (!proceed && out_errno != NULL)
            *out_errno = errno;
    }

    JSLMutableMemory allocation = {0};
    bool got_memory = false;

    if (proceed)
    {
        if (file_size == 0)
        {
            got_memory = true;
        }
        else
        {
            allocation.data = jsl_allocator_interface_alloc(
                allocator,
                file_size,
                JSL_DEFAULT_ALLOCATION_ALIGNMENT,
                false
            );
            allocation.length = file_size;
            got_memory = allocation.data != NULL && allocation.length >= file_size;
        }
        proceed = got_memory;
        res = proceed ? JSL_FILE_LOAD_SUCCESS : JSL_FILE_LOAD_COULD_NOT_GET_MEMORY;
    }

    int64_t read_res = -1;
    bool did_read_data = false;

    if (proceed)
    {
        if (file_size == 0)
        {
            did_read_data = true;
            out_contents->data = NULL;
            out_contents->length = 0;
        }
        else
        {
            #if JSL_IS_WINDOWS
                read_res = (int64_t) _read(file_descriptor, allocation.data, (unsigned int) file_size);
            #elif JSL_IS_POSIX
                read_res = (int64_t) read(file_descriptor, allocation.data, (size_t) file_size);
            #else
                #error "Unsupported platform"
            #endif

            did_read_data = read_res > -1;
            if (did_read_data)
            {
                out_contents->data = allocation.data;
                out_contents->length = read_res;
            }
        }
        proceed = did_read_data;
        res = proceed ? JSL_FILE_LOAD_SUCCESS : JSL_FILE_LOAD_READ_FAILED;
        if (!proceed && out_errno != NULL)
            *out_errno = errno;
    }

    if (opened_file)
    {
        #if JSL_IS_WINDOWS
            int32_t close_res = _close(file_descriptor);
        #elif JSL_IS_POSIX
            int32_t close_res = close(file_descriptor);
        #else
            #error "Unsupported platform"
        #endif

        bool did_close = close_res > -1;
        if (res == JSL_FILE_LOAD_SUCCESS && !did_close)
        {
            res = JSL_FILE_LOAD_CLOSE_FAILED;
            if (out_errno != NULL)
                *out_errno = errno;
        }
    }

    return res;
}

JSLLoadFileResultEnum jsl_load_file_contents_buffer(
    JSLMutableMemory* buffer,
    JSLImmutableMemory path,
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
    JSLImmutableMemory contents,
    JSLImmutableMemory path,
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
                _O_CREAT | _O_WRONLY | _O_TRUNC,
                _SH_DENYNO,
                _S_IREAD | _S_IWRITE
            );
            opened_file = (open_err == 0);
        #elif JSL_IS_POSIX
            file_descriptor = open(path_buffer, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
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
        res = JSL_FILE_WRITE_SUCCESS;
        if (out_bytes_written != NULL)
            *out_bytes_written = write_res;
    }
    else if (opened_file)
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

JSLMakeDirectoryResultEnum jsl_make_directory(
    JSLImmutableMemory path,
    int32_t* out_errno
)
{
    char path_buffer[FILENAME_MAX + 1];
    JSLMakeDirectoryResultEnum res = JSL_MAKE_DIRECTORY_BAD_PARAMETERS;
    bool proceed = false;

    proceed = (path.data != NULL && path.length > 0);

    bool path_too_long = proceed && path.length >= (int64_t) FILENAME_MAX;
    if (path_too_long)
    {
        res = JSL_MAKE_DIRECTORY_PATH_TOO_LONG;
        proceed = false;
    }

    int mkdir_res = -1;
    if (proceed)
    {
        // File system APIs require a null terminated string
        JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
        path_buffer[path.length] = '\0';

        #if JSL_IS_WINDOWS
            mkdir_res = _mkdir(path_buffer);
        #elif JSL_IS_POSIX
            mkdir_res = mkdir(path_buffer, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        #else
            #error "Unsupported platform"
        #endif
    }

    bool mkdir_ok = proceed && (mkdir_res == 0);
    if (mkdir_ok)
    {
        res = JSL_MAKE_DIRECTORY_SUCCESS;
    }

    bool mkdir_failed = proceed && (mkdir_res != 0);
    if (mkdir_failed)
    {
        int32_t err = errno;
        if (out_errno != NULL)
            *out_errno = err;

        switch (err)
        {
            case EEXIST:
                res = JSL_MAKE_DIRECTORY_ALREADY_EXISTS;
                break;
            case EACCES:
                res = JSL_MAKE_DIRECTORY_PERMISSION_DENIED;
                break;
            case ENOENT:
                res = JSL_MAKE_DIRECTORY_PARENT_NOT_FOUND;
                break;
            case ENOSPC:
                res = JSL_MAKE_DIRECTORY_NO_SPACE;
                break;
            case EROFS:
                res = JSL_MAKE_DIRECTORY_READ_ONLY_FS;
                break;
            case ENAMETOOLONG:
                res = JSL_MAKE_DIRECTORY_PATH_TOO_LONG;
                break;
            default:
                res = JSL_MAKE_DIRECTORY_ERROR_UNKNOWN;
                break;
        }
    }

    return res;
}

JSLDeleteFileResultEnum jsl_delete_file(
    JSLImmutableMemory path,
    int32_t* out_errno
)
{
    char path_buffer[FILENAME_MAX + 1];
    JSLDeleteFileResultEnum res = JSL_DELETE_FILE_BAD_PARAMETERS;
    bool proceed = (path.data != NULL
        && path.length > 0
        && path.length < (int64_t) FILENAME_MAX);
    bool had_valid_params = proceed;

    if (proceed)
    {
        JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
        path_buffer[path.length] = '\0';
    }

    #if JSL_IS_WINDOWS

        DWORD attrs = INVALID_FILE_ATTRIBUTES;
        bool got_attrs = false;
        bool is_directory = false;

        if (proceed)
        {
            attrs = GetFileAttributesA(path_buffer);
            got_attrs = (attrs != INVALID_FILE_ATTRIBUTES);
        }

        bool attrs_not_found = (had_valid_params && !got_attrs);
        if (attrs_not_found)
        {
            DWORD last_err = GetLastError();
            if (out_errno != NULL)
                *out_errno = (int32_t) last_err;
            bool is_not_found = (last_err == ERROR_FILE_NOT_FOUND
                || last_err == ERROR_PATH_NOT_FOUND);
            res = is_not_found ? JSL_DELETE_FILE_NOT_FOUND : JSL_DELETE_FILE_ERROR_UNKNOWN;
            proceed = false;
        }

        if (got_attrs)
        {
            // Treat any directory attribute (including junctions) as a directory
            is_directory = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }

        if (proceed && is_directory)
        {
            res = JSL_DELETE_FILE_IS_DIRECTORY;
            proceed = false;
        }

        BOOL delete_ok = FALSE;
        if (proceed)
        {
            delete_ok = DeleteFileA(path_buffer);
        }

        bool delete_success = (proceed && delete_ok != FALSE);
        if (delete_success)
        {
            res = JSL_DELETE_FILE_SUCCESS;
        }

        bool delete_failed = (proceed && delete_ok == FALSE);
        if (delete_failed)
        {
            DWORD last_err = GetLastError();
            if (out_errno != NULL)
                *out_errno = (int32_t) last_err;

            if (last_err == ERROR_ACCESS_DENIED)
                res = JSL_DELETE_FILE_PERMISSION_DENIED;
            else if (last_err == ERROR_FILE_NOT_FOUND || last_err == ERROR_PATH_NOT_FOUND)
                res = JSL_DELETE_FILE_NOT_FOUND;
            else
                res = JSL_DELETE_FILE_ERROR_UNKNOWN;
        }

    #elif JSL_IS_POSIX

        struct stat st_posix;
        int32_t stat_ret = -1;
        bool stat_ok = false;
        bool stat_not_found = false;
        bool stat_other_error = false;
        bool is_directory = false;

        if (proceed)
        {
            errno = 0;
            stat_ret = lstat(path_buffer, &st_posix);
            stat_ok = (stat_ret == 0);
            stat_not_found = (!stat_ok && (errno == ENOENT || errno == ENOTDIR));
            stat_other_error = (!stat_ok && !stat_not_found);
            proceed = stat_ok;
        }

        if (had_valid_params && stat_not_found)
        {
            if (out_errno != NULL)
                *out_errno = errno;
            res = JSL_DELETE_FILE_NOT_FOUND;
        }

        if (had_valid_params && stat_other_error)
        {
            int32_t err = errno;
            if (out_errno != NULL)
                *out_errno = err;
            res = (err == EACCES) ? JSL_DELETE_FILE_PERMISSION_DENIED : JSL_DELETE_FILE_ERROR_UNKNOWN;
        }

        if (stat_ok)
        {
            is_directory = ((st_posix.st_mode & (mode_t) S_IFMT) == S_IFDIR);
        }

        if (proceed && is_directory)
        {
            res = JSL_DELETE_FILE_IS_DIRECTORY;
            proceed = false;
        }

        int32_t unlink_ret = -1;
        if (proceed)
        {
            errno = 0;
            unlink_ret = unlink(path_buffer);
        }

        bool unlink_ok = (proceed && unlink_ret == 0);
        if (unlink_ok)
        {
            res = JSL_DELETE_FILE_SUCCESS;
        }

        bool unlink_failed = (proceed && unlink_ret != 0);
        if (unlink_failed)
        {
            int32_t err = errno;
            if (out_errno != NULL)
                *out_errno = err;

            switch (err)
            {
                case EACCES:
                case EPERM:
                    res = JSL_DELETE_FILE_PERMISSION_DENIED;
                    break;
                case ENOENT:
                    res = JSL_DELETE_FILE_NOT_FOUND;
                    break;
                case ENAMETOOLONG:
                    res = JSL_DELETE_FILE_PATH_TOO_LONG;
                    break;
                default:
                    res = JSL_DELETE_FILE_ERROR_UNKNOWN;
                    break;
            }
        }

    #else
        JSL_ASSERT(0 && "File utils only work on Windows or POSIX platforms.");
        (void) had_valid_params;
    #endif

    return res;
}

#if JSL_IS_POSIX
static int jsl__nftw_delete_callback(
    const char* fpath,
    const struct stat* sb,
    int typeflag,
    struct FTW* ftwbuf
)
{
    (void) sb;
    (void) ftwbuf;

    int res = 0;
    bool is_dir = (typeflag == FTW_DP || typeflag == FTW_DNR);

    if (is_dir)
        res = rmdir(fpath);
    else
        res = unlink(fpath);

    return res == 0 ? 0 : errno;
}
#endif

#if JSL_IS_WINDOWS
static JSLDeleteDirectoryResultEnum jsl__delete_directory_windows(
    const char* path,
    size_t path_len
)
{
    char child_path[FILENAME_MAX + 1];
    char search_pattern[FILENAME_MAX + 3];
    JSLDeleteDirectoryResultEnum result = JSL_DELETE_DIRECTORY_SUCCESS;
    bool entry_fits = false;

    bool pattern_fits = (path_len + 3 <= (size_t) FILENAME_MAX);
    if (!pattern_fits)
        result = JSL_DELETE_DIRECTORY_PATH_TOO_LONG;

    WIN32_FIND_DATAA find_data;
    HANDLE find_handle = INVALID_HANDLE_VALUE;
    bool found = false;
    if (pattern_fits)
    {
        JSL_MEMCPY(search_pattern, path, path_len);
        search_pattern[path_len]     = '\\';
        search_pattern[path_len + 1] = '*';
        search_pattern[path_len + 2] = '\0';
        find_handle = FindFirstFileA(search_pattern, &find_data);
        found = (find_handle != INVALID_HANDLE_VALUE);
    }

    bool find_failed = pattern_fits && !found;
    if (find_failed && result == JSL_DELETE_DIRECTORY_SUCCESS)
    {
        DWORD err = GetLastError();
        bool not_found = (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND);
        result = not_found ? JSL_DELETE_DIRECTORY_NOT_FOUND
                           : JSL_DELETE_DIRECTORY_ERROR_UNKNOWN;
    }

    JSL_MEMCPY(child_path, path, path_len);
    child_path[path_len] = '\\';

    bool iterate = found;
    while (iterate)
    {
        bool is_dot    = find_data.cFileName[0] == '.' && find_data.cFileName[1] == '\0';
        bool is_dotdot = find_data.cFileName[0] == '.' && find_data.cFileName[1] == '.'
                                                        && find_data.cFileName[2] == '\0';
        bool skip = is_dot || is_dotdot;

        size_t name_len = 0;
        entry_fits = false;
        if (!skip)
        {
            name_len   = JSL_STRLEN(find_data.cFileName);
            entry_fits = (path_len + 1 + name_len < (size_t) FILENAME_MAX);
        }

        bool path_too_long = !skip && !entry_fits;
        if (path_too_long && result == JSL_DELETE_DIRECTORY_SUCCESS)
            result = JSL_DELETE_DIRECTORY_PATH_TOO_LONG;

        bool is_directory = false;
        if (entry_fits)
        {
            JSL_MEMCPY(child_path + path_len + 1, find_data.cFileName, name_len + 1);
            is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }

        JSLDeleteDirectoryResultEnum sub = JSL_DELETE_DIRECTORY_SUCCESS;
        if (is_directory)
            sub = jsl__delete_directory_windows(child_path, path_len + 1 + name_len);

        bool sub_failed = is_directory && (sub != JSL_DELETE_DIRECTORY_SUCCESS);
        if (sub_failed && result == JSL_DELETE_DIRECTORY_SUCCESS)
            result = sub;

        bool is_file  = entry_fits && !is_directory;
        BOOL del_ok   = TRUE;
        if (is_file)
            del_ok = DeleteFileA(child_path);

        bool del_failed = is_file && (del_ok == FALSE);
        if (del_failed && result == JSL_DELETE_DIRECTORY_SUCCESS)
        {
            DWORD err = GetLastError();
            bool perm = (err == ERROR_ACCESS_DENIED);
            result = perm ? JSL_DELETE_DIRECTORY_PERMISSION_DENIED
                          : JSL_DELETE_DIRECTORY_ERROR_UNKNOWN;
        }

        BOOL next = FindNextFileA(find_handle, &find_data);
        iterate   = (next != FALSE);
    }

    if (found)
        FindClose(find_handle);

    BOOL rm_ok     = TRUE;
    bool should_rm = found && (result == JSL_DELETE_DIRECTORY_SUCCESS);
    if (should_rm)
        rm_ok = RemoveDirectoryA(path);

    bool rm_failed = should_rm && (rm_ok == FALSE);
    if (rm_failed)
    {
        DWORD err = GetLastError();
        bool perm = (err == ERROR_ACCESS_DENIED);
        result = perm ? JSL_DELETE_DIRECTORY_PERMISSION_DENIED
                      : JSL_DELETE_DIRECTORY_ERROR_UNKNOWN;
    }

    return result;
}
#endif

JSLDeleteDirectoryResultEnum jsl_delete_directory(
    JSLImmutableMemory path,
    int32_t* out_errno
)
{
    char path_buffer[FILENAME_MAX + 1];
    JSLDeleteDirectoryResultEnum res = JSL_DELETE_DIRECTORY_BAD_PARAMETERS;
    bool proceed = false;

    proceed = (path.data != NULL
        && path.length > 0
        && path.length < (int64_t) FILENAME_MAX);

    if (proceed)
    {
        JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
        path_buffer[path.length] = '\0';
    }

    #if JSL_IS_WINDOWS

        DWORD attrs = INVALID_FILE_ATTRIBUTES;
        bool got_attrs = false;

        if (proceed)
        {
            attrs      = GetFileAttributesA(path_buffer);
            got_attrs  = (attrs != INVALID_FILE_ATTRIBUTES);
        }

        bool attrs_not_found = proceed && !got_attrs;
        if (attrs_not_found)
        {
            DWORD last_err = GetLastError();
            if (out_errno != NULL)
                *out_errno = (int32_t) last_err;
            bool is_not_found = (last_err == ERROR_FILE_NOT_FOUND
                || last_err == ERROR_PATH_NOT_FOUND);
            res     = is_not_found ? JSL_DELETE_DIRECTORY_NOT_FOUND
                                   : JSL_DELETE_DIRECTORY_ERROR_UNKNOWN;
            proceed = false;
        }

        bool not_a_directory = proceed && got_attrs && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
        if (not_a_directory)
        {
            res     = JSL_DELETE_DIRECTORY_NOT_A_DIRECTORY;
            proceed = false;
        }

        if (proceed)
        {
            res = jsl__delete_directory_windows(path_buffer, (size_t) path.length);
            if (res != JSL_DELETE_DIRECTORY_SUCCESS && out_errno != NULL)
                *out_errno = (int32_t) GetLastError();
        }

    #elif JSL_IS_POSIX

        struct stat st_posix;
        int32_t stat_ret = -1;
        bool stat_ok        = false;
        bool stat_not_found = false;
        bool stat_error     = false;

        if (proceed)
        {
            errno           = 0;
            stat_ret        = (int32_t) lstat(path_buffer, &st_posix);
            stat_ok         = (stat_ret == 0);
            stat_not_found  = (!stat_ok && (errno == ENOENT || errno == ENOTDIR));
            stat_error      = (!stat_ok && !stat_not_found);
        }

        bool is_not_found = proceed && stat_not_found;
        if (is_not_found)
        {
            if (out_errno != NULL)
                *out_errno = errno;
            res     = JSL_DELETE_DIRECTORY_NOT_FOUND;
            proceed = false;
        }

        bool had_stat_error = proceed && stat_error;
        if (had_stat_error)
        {
            if (out_errno != NULL)
                *out_errno = errno;
            res     = JSL_DELETE_DIRECTORY_ERROR_UNKNOWN;
            proceed = false;
        }

        bool not_a_directory = proceed && stat_ok
            && ((st_posix.st_mode & (mode_t) S_IFMT) != S_IFDIR);
        if (not_a_directory)
        {
            res     = JSL_DELETE_DIRECTORY_NOT_A_DIRECTORY;
            proceed = false;
        }

        if (proceed)
        {
            int32_t nftw_res = (int32_t) nftw(
                path_buffer,
                jsl__nftw_delete_callback,
                64,
                FTW_DEPTH | FTW_PHYS
            );
            bool nftw_ok = (nftw_res == 0);

            if (nftw_ok)
                res = JSL_DELETE_DIRECTORY_SUCCESS;

            bool nftw_failed = !nftw_ok;
            if (nftw_failed)
            {
                int32_t err = (nftw_res == -1) ? errno : nftw_res;
                if (out_errno != NULL)
                    *out_errno = err;
                bool perm = (err == EACCES || err == EPERM);
                res = perm ? JSL_DELETE_DIRECTORY_PERMISSION_DENIED
                           : JSL_DELETE_DIRECTORY_ERROR_UNKNOWN;
            }
        }

    #else
        JSL_ASSERT(0 && "File utils only work on Windows or POSIX platforms.");
        (void) proceed;
    #endif

    return res;
}

JSLFileTypeEnum jsl_get_file_type(JSLImmutableMemory path)
{
    char path_buffer[FILENAME_MAX + 1];
    JSLFileTypeEnum result = JSL_FILE_TYPE_UNKNOWN;
    bool proceed = false;

    proceed = (path.data != NULL
        && path.length > 0
        && path.length < (int64_t) FILENAME_MAX);

    if (proceed)
    {
        // File system APIs require a null terminated string
        JSL_MEMCPY(path_buffer, path.data, (size_t) path.length);
        path_buffer[path.length] = '\0';
    }

    #if JSL_IS_WINDOWS

        DWORD attrs = INVALID_FILE_ATTRIBUTES;
        bool got_attrs = false;
        bool is_reparse = false;

        if (proceed)
        {
            attrs = GetFileAttributesA(path_buffer);
            got_attrs = (attrs != INVALID_FILE_ATTRIBUTES);
            proceed = got_attrs;
        }

        bool attrs_not_found = false;
        if (!got_attrs && path.data != NULL && path.length > 0
            && path.length < (int64_t) FILENAME_MAX)
        {
            DWORD last_err = GetLastError();
            attrs_not_found = (last_err == ERROR_FILE_NOT_FOUND
                || last_err == ERROR_PATH_NOT_FOUND);
        }

        if (attrs_not_found)
        {
            result = JSL_FILE_TYPE_NOT_FOUND;
        }

        if (proceed)
        {
            is_reparse = (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        }

        // Reparse points include symbolic links and junctions. We report
        // them as symlinks here without resolving to the target.
        if (proceed && is_reparse)
        {
            result = JSL_FILE_TYPE_SYMLINK;
            proceed = false;
        }

        struct _stat64 st_win = {0};
        int32_t stat_ret = -1;
        bool stat_ok = false;

        if (proceed)
        {
            stat_ret = _stat64(path_buffer, &st_win);
            stat_ok = (stat_ret == 0);
            proceed = stat_ok;
        }

        if (proceed)
        {
            uint32_t mode = (uint32_t) (st_win.st_mode & _S_IFMT);
            if (mode == _S_IFREG)
                result = JSL_FILE_TYPE_REG;
            else if (mode == _S_IFDIR)
                result = JSL_FILE_TYPE_DIR;
            else if (mode == _S_IFCHR)
                result = JSL_FILE_TYPE_CHAR;
            else if (mode == _S_IFIFO)
                result = JSL_FILE_TYPE_FIFO;
            else
                result = JSL_FILE_TYPE_UNKNOWN;
        }

    #elif JSL_IS_POSIX

        struct stat st_posix;
        int32_t stat_ret = -1;
        bool stat_ok = false;
        bool stat_not_found = false;

        if (proceed)
        {
            errno = 0;
            stat_ret = lstat(path_buffer, &st_posix);
            stat_ok = (stat_ret == 0);
            stat_not_found = (!stat_ok && (errno == ENOENT || errno == ENOTDIR));
            proceed = stat_ok;
        }

        if (stat_not_found)
        {
            result = JSL_FILE_TYPE_NOT_FOUND;
        }

        if (proceed)
        {
            mode_t mode = st_posix.st_mode & (mode_t) S_IFMT;
            if (mode == S_IFREG)
                result = JSL_FILE_TYPE_REG;
            else if (mode == S_IFDIR)
                result = JSL_FILE_TYPE_DIR;
            else if (mode == S_IFLNK)
                result = JSL_FILE_TYPE_SYMLINK;
            else if (mode == S_IFBLK)
                result = JSL_FILE_TYPE_BLOCK;
            else if (mode == S_IFCHR)
                result = JSL_FILE_TYPE_CHAR;
            else if (mode == S_IFIFO)
                result = JSL_FILE_TYPE_FIFO;
            else if (mode == S_IFSOCK)
                result = JSL_FILE_TYPE_SOCKET;
            else
                result = JSL_FILE_TYPE_UNKNOWN;
        }

    #else
        JSL_ASSERT(0 && "File utils only work on Windows or POSIX platforms.");
    #endif

    return result;
}

int64_t jsl_write_to_c_file(FILE* out, JSLImmutableMemory data)
{
    if (out == NULL || data.data == NULL || data.length < 0)
        return -1;

    int64_t written = (int64_t) fwrite(
        data.data,
        sizeof(uint8_t),
        (size_t) data.length,
        out
    );

    if (ferror(out) == 0)
        return written;
    else
        return -errno;
}

static void jsl__c_file_sink_out(void* user, JSLImmutableMemory data)
{
    FILE* file = (FILE*) user;
    if (file == NULL)
        return;

    fwrite(
        data.data,
        sizeof(uint8_t),
        (size_t) data.length,
        file
    );
}

JSLOutputSink jsl_c_file_output_sink(FILE* out)
{
    JSLOutputSink sink;
    sink.write_fp = jsl__c_file_sink_out;
    sink.user_data = out;
    return sink;
}

JSLDirectoryIteratorInitResultEnum jsl_directory_iterator_init(
    JSLImmutableMemory path,
    JSLDirectoryIterator* iterator,
    bool follow_symlinks
)
{
    JSLDirectoryIteratorInitResultEnum res = JSL_DIRECTORY_ITERATOR_INIT_BAD_PARAMETERS;
    bool proceed = (path.data != NULL && path.length > 0 && iterator != NULL);

    // Initialize iterator state to a known-empty configuration regardless of
    // outcome so the caller can safely call close() and so next() will
    // immediately return false on a failed init.
    if (iterator != NULL)
    {
        iterator->path_buffer[0]  = '\0';
        iterator->base_length     = 0;
        iterator->current_length  = 0;
        iterator->depth           = 0;
        iterator->follow_symlinks = follow_symlinks;
        iterator->exhausted       = true;
        #if JSL_IS_WINDOWS
            iterator->find_pattern[0] = '\0';
        #endif
    }

    bool path_too_long_input = proceed && (path.length > (int64_t) FILENAME_MAX);
    if (path_too_long_input)
    {
        res = JSL_DIRECTORY_ITERATOR_INIT_PATH_TOO_LONG;
        proceed = false;
    }

    char input_buffer[FILENAME_MAX + 1];
    if (proceed)
    {
        JSL_MEMCPY(input_buffer, path.data, (size_t) path.length);
        input_buffer[path.length] = '\0';
    }

    #if JSL_IS_POSIX

        char    resolved_buffer[FILENAME_MAX + 1];
        bool    resolved_ok   = false;
        int32_t resolve_errno = 0;
        int64_t resolved_len  = 0;

        if (proceed)
        {
            errno = 0;
            char* r       = realpath(input_buffer, resolved_buffer);
            resolved_ok   = (r != NULL);
            resolve_errno = errno;
        }

        bool resolve_failed = proceed && !resolved_ok;
        if (resolve_failed)
        {
            bool not_found = (resolve_errno == ENOENT || resolve_errno == ENOTDIR);
            res = not_found ? JSL_DIRECTORY_ITERATOR_INIT_NOT_FOUND
                            : JSL_DIRECTORY_ITERATOR_INIT_RESOLVE_FAILED;
            proceed = false;
        }

        if (proceed)
        {
            resolved_len = (int64_t) JSL_STRLEN(resolved_buffer);
        }

        // We need room for the resolved path plus a trailing separator and a
        // NUL terminator: resolved_len + 1 (sep) + 1 (NUL) <= FILENAME_MAX + 1.
        bool resolved_too_long = proceed
            && (resolved_len + 1 > (int64_t) FILENAME_MAX);
        if (resolved_too_long)
        {
            res = JSL_DIRECTORY_ITERATOR_INIT_PATH_TOO_LONG;
            proceed = false;
        }

        if (proceed)
        {
            JSL_MEMCPY(iterator->path_buffer, resolved_buffer, (size_t) resolved_len);
            iterator->path_buffer[resolved_len] = '\0';

            // Append a trailing separator unless realpath already gave us one
            // (which only happens for the filesystem root "/").
            bool needs_sep = (resolved_len > 0
                && iterator->path_buffer[resolved_len - 1] != '/');
            if (needs_sep)
            {
                iterator->path_buffer[resolved_len]     = '/';
                iterator->path_buffer[resolved_len + 1] = '\0';
                resolved_len++;
            }

            iterator->base_length    = resolved_len;
            iterator->current_length = resolved_len;
        }

        DIR*    root_dir    = NULL;
        bool    opened_root = false;
        int32_t open_errno  = 0;
        if (proceed)
        {
            errno       = 0;
            root_dir    = opendir(iterator->path_buffer);
            opened_root = (root_dir != NULL);
            open_errno  = errno;
        }

        bool open_failed = proceed && !opened_root;
        if (open_failed)
        {
            if (open_errno == ENOENT)
                res = JSL_DIRECTORY_ITERATOR_INIT_NOT_FOUND;
            else if (open_errno == ENOTDIR)
                res = JSL_DIRECTORY_ITERATOR_INIT_NOT_A_DIRECTORY;
            else
                res = JSL_DIRECTORY_ITERATOR_INIT_COULD_NOT_OPEN;
            proceed = false;
        }

        if (proceed)
        {
            iterator->frames[0].dir = root_dir;
            iterator->frames[0].prefix_length = iterator->base_length;
            iterator->depth = 1;
            iterator->exhausted = false;
            iterator->sentinel = JSL__DIR_ITERATOR_PRIVATE_SENTINEL;
            res = JSL_DIRECTORY_ITERATOR_INIT_SUCCESS;
        }

    #elif JSL_IS_WINDOWS

        char  resolved_buffer[FILENAME_MAX + 1];
        DWORD resolve_res = 0;
        if (proceed)
        {
            resolve_res = GetFullPathNameA(
                input_buffer,
                (DWORD)(FILENAME_MAX + 1),
                resolved_buffer,
                NULL
            );
        }

        bool resolve_too_long = proceed && (resolve_res > (DWORD) FILENAME_MAX);
        if (resolve_too_long)
        {
            res = JSL_DIRECTORY_ITERATOR_INIT_PATH_TOO_LONG;
            proceed = false;
        }

        bool resolve_failed = proceed && (resolve_res == 0);
        if (resolve_failed)
        {
            DWORD err = GetLastError();
            bool not_found = (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND);
            res = not_found ? JSL_DIRECTORY_ITERATOR_INIT_NOT_FOUND
                            : JSL_DIRECTORY_ITERATOR_INIT_RESOLVE_FAILED;
            proceed = false;
        }

        int64_t resolved_len = 0;
        if (proceed)
        {
            resolved_len = (int64_t) resolve_res;
        }

        bool needs_sep = proceed && resolved_len > 0
            && resolved_buffer[resolved_len - 1] != '\\'
            && resolved_buffer[resolved_len - 1] != '/';

        bool resolved_too_long_with_sep = proceed && needs_sep
            && (resolved_len + 1 > (int64_t) FILENAME_MAX);
        if (resolved_too_long_with_sep)
        {
            res = JSL_DIRECTORY_ITERATOR_INIT_PATH_TOO_LONG;
            proceed = false;
        }

        if (proceed)
        {
            JSL_MEMCPY(iterator->path_buffer, resolved_buffer, (size_t) resolved_len);
            iterator->path_buffer[resolved_len] = '\0';

            if (needs_sep)
            {
                iterator->path_buffer[resolved_len]     = '\\';
                iterator->path_buffer[resolved_len + 1] = '\0';
                resolved_len++;
            }

            iterator->base_length    = resolved_len;
            iterator->current_length = resolved_len;

            // Build search pattern: path + "*"
            JSL_MEMCPY(
                iterator->find_pattern,
                iterator->path_buffer,
                (size_t) resolved_len
            );
            iterator->find_pattern[resolved_len]     = '*';
            iterator->find_pattern[resolved_len + 1] = '\0';
        }

        HANDLE root_handle = INVALID_HANDLE_VALUE;
        bool   found_root  = false;
        DWORD  find_err    = 0;
        if (proceed)
        {
            root_handle = FindFirstFileA(iterator->find_pattern, &iterator->find_data);
            found_root  = (root_handle != INVALID_HANDLE_VALUE);
            if (!found_root)
                find_err = GetLastError();
        }

        bool find_failed = proceed && !found_root;
        if (find_failed)
        {
            if (find_err == ERROR_FILE_NOT_FOUND || find_err == ERROR_PATH_NOT_FOUND)
                res = JSL_DIRECTORY_ITERATOR_INIT_NOT_FOUND;
            else if (find_err == ERROR_DIRECTORY)
                res = JSL_DIRECTORY_ITERATOR_INIT_NOT_A_DIRECTORY;
            else
                res = JSL_DIRECTORY_ITERATOR_INIT_COULD_NOT_OPEN;
            proceed = false;
        }

        if (proceed)
        {
            iterator->frames[0].find_handle = root_handle;
            iterator->frames[0].prefix_length = iterator->base_length;
            iterator->frames[0].consumed_first = false;
            iterator->depth = 1;
            iterator->exhausted = false;
            iterator->sentinel = JSL__DIR_ITERATOR_PRIVATE_SENTINEL;
            res = JSL_DIRECTORY_ITERATOR_INIT_SUCCESS;
        }

    #else
        JSL_ASSERT(0 && "Directory iterator only supported on POSIX or Windows.");
    #endif

    return res;
}

bool jsl_directory_iterator_next(
    JSLDirectoryIterator* iterator,
    JSLDirectoryIteratorResult* result,
    int32_t* out_errno
)
{
    bool produced    = false;
    bool valid_args  = (
        iterator != NULL
        && iterator->sentinel == JSL__DIR_ITERATOR_PRIVATE_SENTINEL
        && !iterator->exhausted
        && result != NULL
    );

    if (out_errno != NULL && valid_args)
        *out_errno = 0;

    while (valid_args && !produced && iterator->depth > 0)
    {
        int32_t                    frame_idx = iterator->depth - 1;
        JSLDirectoryIteratorFrame* frame     = &iterator->frames[frame_idx];

        const char* entry_name = NULL;
        bool        got_name   = false;

        // Read the next entry from the current frame.
        #if JSL_IS_POSIX
            errno = 0;
            struct dirent* de = readdir(frame->dir);
            if (de != NULL)
            {
                entry_name = de->d_name;
                got_name   = true;
            }
        #elif JSL_IS_WINDOWS
            if (!frame->consumed_first)
            {
                entry_name            = iterator->find_data.cFileName;
                got_name              = true;
                frame->consumed_first = true;
            }
            else
            {
                BOOL ok = FindNextFileA(frame->find_handle, &iterator->find_data);
                if (ok != FALSE)
                {
                    entry_name = iterator->find_data.cFileName;
                    got_name   = true;
                }
            }
        #endif

        // No more entries in this frame -> close handle, pop, retry parent.
        if (!got_name)
        {
            #if JSL_IS_POSIX
                if (frame->dir != NULL)
                {
                    closedir(frame->dir);
                    frame->dir = NULL;
                }
            #elif JSL_IS_WINDOWS
                if (frame->find_handle != INVALID_HANDLE_VALUE)
                {
                    FindClose(frame->find_handle);
                    frame->find_handle = INVALID_HANDLE_VALUE;
                }
                frame->consumed_first = false;
            #endif
            iterator->depth--;
            continue;
        }

        // Skip "." and "..".
        bool is_dot    = (entry_name[0] == '.' && entry_name[1] == '\0');
        bool is_dotdot = (entry_name[0] == '.'
            && entry_name[1] == '.'
            && entry_name[2] == '\0');
        if (is_dot || is_dotdot)
            continue;

        int64_t name_len       = (int64_t) JSL_STRLEN(entry_name);
        int64_t entry_path_len = frame->prefix_length + name_len;

        // The entry path needs to fit in path_buffer along with a NUL byte.
        bool entry_too_long = (entry_path_len > (int64_t) FILENAME_MAX);
        if (entry_too_long)
        {
            // We cannot represent this entry's path. Report a degenerate
            // result so the caller learns about it and continue iterating.
            result->absolute_path.data   = (const uint8_t*) iterator->path_buffer;
            result->absolute_path.length = 0;
            result->relative_path.data   = (const uint8_t*) iterator->path_buffer;
            result->relative_path.length = 0;
            result->type                 = JSL_FILE_TYPE_UNKNOWN;
            result->depth                = iterator->depth - 1;
            result->status               = JSL_DIRECTORY_ITERATOR_PATH_TOO_LONG;
            produced                     = true;
            continue;
        }

        // Write the entry name into path_buffer (NUL-terminated for the OS calls).
        JSL_MEMCPY(
            iterator->path_buffer + frame->prefix_length,
            entry_name,
            (size_t) name_len
        );
        iterator->path_buffer[entry_path_len] = '\0';
        iterator->current_length              = entry_path_len;

        // Determine the entry type.
        JSLFileTypeEnum entry_type = JSL_FILE_TYPE_UNKNOWN;
        bool            stat_ok    = false;

        #if JSL_IS_POSIX
            int32_t     stat_errno = 0;
            struct stat lst;
            errno = 0;
            int lr = lstat(iterator->path_buffer, &lst);
            if (lr == 0)
            {
                stat_ok    = true;
                mode_t m   = lst.st_mode & (mode_t) S_IFMT;
                if      (m == S_IFREG)  entry_type = JSL_FILE_TYPE_REG;
                else if (m == S_IFDIR)  entry_type = JSL_FILE_TYPE_DIR;
                else if (m == S_IFLNK)  entry_type = JSL_FILE_TYPE_SYMLINK;
                else if (m == S_IFBLK)  entry_type = JSL_FILE_TYPE_BLOCK;
                else if (m == S_IFCHR)  entry_type = JSL_FILE_TYPE_CHAR;
                else if (m == S_IFIFO)  entry_type = JSL_FILE_TYPE_FIFO;
                else if (m == S_IFSOCK) entry_type = JSL_FILE_TYPE_SOCKET;
            }
            else
            {
                stat_errno = errno;
            }

            // If following symlinks, resolve the link target's type.
            bool follow_this = stat_ok
                && (entry_type == JSL_FILE_TYPE_SYMLINK)
                && iterator->follow_symlinks;
            if (follow_this)
            {
                struct stat tst;
                int tr = stat(iterator->path_buffer, &tst);
                if (tr == 0)
                {
                    mode_t m = tst.st_mode & (mode_t) S_IFMT;
                    if      (m == S_IFREG)  entry_type = JSL_FILE_TYPE_REG;
                    else if (m == S_IFDIR)  entry_type = JSL_FILE_TYPE_DIR;
                    else if (m == S_IFLNK)  entry_type = JSL_FILE_TYPE_SYMLINK;
                    else if (m == S_IFBLK)  entry_type = JSL_FILE_TYPE_BLOCK;
                    else if (m == S_IFCHR)  entry_type = JSL_FILE_TYPE_CHAR;
                    else if (m == S_IFIFO)  entry_type = JSL_FILE_TYPE_FIFO;
                    else if (m == S_IFSOCK) entry_type = JSL_FILE_TYPE_SOCKET;
                }
                // If stat() failed, leave entry_type as SYMLINK and don't descend.
            }
        #elif JSL_IS_WINDOWS
            DWORD attrs       = iterator->find_data.dwFileAttributes;
            bool  is_reparse  = (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            bool  is_dir_attr = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;

            stat_ok = true;

            if (is_reparse)
            {
                entry_type = JSL_FILE_TYPE_SYMLINK;
                if (iterator->follow_symlinks)
                {
                    struct _stat64 st_target = {0};
                    int sr = _stat64(iterator->path_buffer, &st_target);
                    if (sr == 0)
                    {
                        uint32_t m = (uint32_t)(st_target.st_mode & _S_IFMT);
                        if      (m == _S_IFREG) entry_type = JSL_FILE_TYPE_REG;
                        else if (m == _S_IFDIR) entry_type = JSL_FILE_TYPE_DIR;
                        else if (m == _S_IFCHR) entry_type = JSL_FILE_TYPE_CHAR;
                        else if (m == _S_IFIFO) entry_type = JSL_FILE_TYPE_FIFO;
                    }
                }
            }
            else if (is_dir_attr)
            {
                entry_type = JSL_FILE_TYPE_DIR;
            }
            else
            {
                entry_type = JSL_FILE_TYPE_REG;
            }
        #endif

        // Fill the result. The status may be overridden below if descent fails.
        result->absolute_path.data   = (const uint8_t*) iterator->path_buffer;
        result->absolute_path.length = entry_path_len;
        result->relative_path.data   = (const uint8_t*)
            (iterator->path_buffer + iterator->base_length);
        result->relative_path.length = entry_path_len - iterator->base_length;
        result->type                 = entry_type;
        result->depth                = iterator->depth - 1;
        result->status               = stat_ok ? JSL_DIRECTORY_ITERATOR_OK
                                               : JSL_DIRECTORY_ITERATOR_PARTIAL;

        #if JSL_IS_POSIX
            if (!stat_ok && out_errno != NULL)
                *out_errno = stat_errno;
        #endif

        produced = true;

        // Try to descend if this entry is a directory we should enter.
        bool should_descend = (entry_type == JSL_FILE_TYPE_DIR);

        bool depth_exceeded = should_descend
            && (iterator->depth >= JSL_DIRECTORY_ITERATOR_MAX_DEPTH);
        if (depth_exceeded)
            result->status = JSL_DIRECTORY_ITERATOR_MAX_DEPTH_EXCEEDED;

        int64_t new_prefix_length  = entry_path_len + 1;
        bool    sep_fits_in_buffer = should_descend && !depth_exceeded
            && (new_prefix_length <= (int64_t) FILENAME_MAX);

        bool descent_path_too_long = should_descend
            && !depth_exceeded && !sep_fits_in_buffer;
        if (descent_path_too_long)
            result->status = JSL_DIRECTORY_ITERATOR_PATH_TOO_LONG;

        bool ok_to_open = should_descend && !depth_exceeded && sep_fits_in_buffer;

        #if JSL_IS_POSIX
            if (ok_to_open)
            {
                iterator->path_buffer[entry_path_len]    = '/';
                iterator->path_buffer[new_prefix_length] = '\0';

                errno    = 0;
                DIR* sub = opendir(iterator->path_buffer);
                if (sub != NULL)
                {
                    iterator->frames[iterator->depth].dir           = sub;
                    iterator->frames[iterator->depth].prefix_length = new_prefix_length;
                    iterator->depth++;
                }
                else
                {
                    int32_t open_err = errno;
                    // Restore path_buffer (drop the appended separator).
                    iterator->path_buffer[entry_path_len] = '\0';
                    result->status = JSL_DIRECTORY_ITERATOR_COULD_NOT_OPEN;
                    if (out_errno != NULL)
                        *out_errno = open_err;
                }
            }
        #elif JSL_IS_WINDOWS
            if (ok_to_open)
            {
                iterator->path_buffer[entry_path_len]    = '\\';
                iterator->path_buffer[new_prefix_length] = '\0';

                JSL_MEMCPY(
                    iterator->find_pattern,
                    iterator->path_buffer,
                    (size_t) new_prefix_length
                );
                iterator->find_pattern[new_prefix_length]     = '*';
                iterator->find_pattern[new_prefix_length + 1] = '\0';

                HANDLE sub_handle = FindFirstFileA(
                    iterator->find_pattern,
                    &iterator->find_data
                );
                if (sub_handle != INVALID_HANDLE_VALUE)
                {
                    iterator->frames[iterator->depth].find_handle    = sub_handle;
                    iterator->frames[iterator->depth].prefix_length  = new_prefix_length;
                    iterator->frames[iterator->depth].consumed_first = false;
                    iterator->depth++;
                }
                else
                {
                    DWORD open_err = GetLastError();
                    iterator->path_buffer[entry_path_len] = '\0';
                    result->status = JSL_DIRECTORY_ITERATOR_COULD_NOT_OPEN;
                    if (out_errno != NULL)
                        *out_errno = (int32_t) open_err;
                }
            }
        #endif
    }

    if (valid_args && !produced)
    {
        iterator->exhausted = true;
    }

    return produced;
}

void jsl_directory_iterator_end(JSLDirectoryIterator* iterator)
{
    if (
        iterator != NULL 
        && iterator->sentinel == JSL__DIR_ITERATOR_PRIVATE_SENTINEL
    )
    {
        while (iterator->depth > 0)
        {
            int32_t                    frame_idx = iterator->depth - 1;
            JSLDirectoryIteratorFrame* frame     = &iterator->frames[frame_idx];

            #if JSL_IS_POSIX
                if (frame->dir != NULL)
                {
                    closedir(frame->dir);
                    frame->dir = NULL;
                }
            #elif JSL_IS_WINDOWS
                if (frame->find_handle != INVALID_HANDLE_VALUE)
                {
                    FindClose(frame->find_handle);
                    frame->find_handle = INVALID_HANDLE_VALUE;
                }
                frame->consumed_first = false;
            #endif

            iterator->depth--;
        }

        iterator->sentinel = 0;
        iterator->exhausted = true;
    }
}

JSLCopyFileResultEnum jsl_copy_file(
    JSLImmutableMemory src_path,
    JSLImmutableMemory dst_path,
    int32_t* out_errno
)
{
    char src_buffer[FILENAME_MAX + 1];
    char dst_buffer[FILENAME_MAX + 1];
    JSLCopyFileResultEnum res = JSL_COPY_FILE_BAD_PARAMETERS;
    bool proceed = false;

    proceed = (src_path.data != NULL
        && src_path.length > 0
        && dst_path.data != NULL
        && dst_path.length > 0);

    bool path_too_long = proceed && (src_path.length >= (int64_t) FILENAME_MAX
        || dst_path.length >= (int64_t) FILENAME_MAX);
    if (path_too_long)
    {
        res = JSL_COPY_FILE_PATH_TOO_LONG;
        proceed = false;
    }

    if (proceed)
    {
        JSL_MEMCPY(src_buffer, src_path.data, (size_t) src_path.length);
        src_buffer[src_path.length] = '\0';
        JSL_MEMCPY(dst_buffer, dst_path.data, (size_t) dst_path.length);
        dst_buffer[dst_path.length] = '\0';
        res = JSL_COPY_FILE_SOURCE_NOT_FOUND;
    }

    #if JSL_IS_WINDOWS

        DWORD src_attrs = INVALID_FILE_ATTRIBUTES;
        bool got_src_attrs = false;

        if (proceed)
        {
            src_attrs = GetFileAttributesA(src_buffer);
            got_src_attrs = (src_attrs != INVALID_FILE_ATTRIBUTES);
        }

        bool src_attrs_failed = proceed && !got_src_attrs;
        if (src_attrs_failed)
        {
            DWORD last_err = GetLastError();
            if (out_errno != NULL)
                *out_errno = (int32_t) last_err;
            bool is_not_found = (last_err == ERROR_FILE_NOT_FOUND
                || last_err == ERROR_PATH_NOT_FOUND);
            res = is_not_found ? JSL_COPY_FILE_SOURCE_NOT_FOUND : JSL_COPY_FILE_ERROR_UNKNOWN;
            proceed = false;
        }

        bool src_is_dir = got_src_attrs && ((src_attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
        if (src_is_dir)
        {
            res = JSL_COPY_FILE_SOURCE_IS_DIRECTORY;
            proceed = false;
        }

        BOOL copy_ok = FALSE;
        if (proceed)
        {
            copy_ok = CopyFileA(src_buffer, dst_buffer, FALSE);
        }

        bool copy_success = (proceed && copy_ok != FALSE);
        if (copy_success)
        {
            res = JSL_COPY_FILE_SUCCESS;
        }

        bool copy_failed = (proceed && copy_ok == FALSE);
        if (copy_failed)
        {
            DWORD last_err = GetLastError();
            if (out_errno != NULL)
                *out_errno = (int32_t) last_err;

            if (last_err == ERROR_FILE_NOT_FOUND || last_err == ERROR_PATH_NOT_FOUND)
                res = JSL_COPY_FILE_SOURCE_NOT_FOUND;
            else if (last_err == ERROR_ACCESS_DENIED)
                res = JSL_COPY_FILE_PERMISSION_DENIED;
            else if (last_err == ERROR_DISK_FULL || last_err == ERROR_HANDLE_DISK_FULL)
                res = JSL_COPY_FILE_WRITE_FAILED;
            else
                res = JSL_COPY_FILE_ERROR_UNKNOWN;
        }

    #elif JSL_IS_POSIX

        struct stat src_stat;
        int32_t stat_ret = -1;
        bool stat_ok = false;
        bool is_source_reg = false;

        if (proceed)
        {
            errno = 0;
            stat_ret = lstat(src_buffer, &src_stat);
            stat_ok = (stat_ret == 0);
            proceed = stat_ok;
        }

        bool stat_failed = !stat_ok && (res == JSL_COPY_FILE_SOURCE_NOT_FOUND);
        if (stat_failed)
        {
            int32_t err = errno;
            if (out_errno != NULL)
                *out_errno = err;
            bool is_not_found = (err == ENOENT || err == ENOTDIR);
            res = is_not_found ? JSL_COPY_FILE_SOURCE_NOT_FOUND : JSL_COPY_FILE_ERROR_UNKNOWN;
        }

        if (stat_ok)
        {
            is_source_reg = ((src_stat.st_mode & (mode_t) S_IFMT) == S_IFREG);
        }

        bool source_is_dir = stat_ok && !is_source_reg;
        if (source_is_dir)
        {
            res = JSL_COPY_FILE_SOURCE_IS_DIRECTORY;
            proceed = false;
        }

        int32_t src_fd = -1;
        bool opened_src = false;

        if (proceed)
        {
            errno = 0;
            src_fd = open(src_buffer, O_RDONLY);
            opened_src = (src_fd > -1);
            proceed = opened_src;
        }

        bool open_src_failed = !opened_src && stat_ok && is_source_reg;
        if (open_src_failed)
        {
            int32_t err = errno;
            if (out_errno != NULL)
                *out_errno = err;
            res = (err == EACCES) ? JSL_COPY_FILE_PERMISSION_DENIED : JSL_COPY_FILE_COULD_NOT_OPEN_SOURCE;
        }

        int32_t dst_fd = -1;
        bool opened_dst = false;

        if (proceed)
        {
            errno = 0;
            dst_fd = open(dst_buffer, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            opened_dst = (dst_fd > -1);
            proceed = opened_dst;
        }

        bool open_dst_failed = opened_src && !opened_dst;
        if (open_dst_failed)
        {
            int32_t err = errno;
            if (out_errno != NULL)
                *out_errno = err;
            res = (err == EACCES) ? JSL_COPY_FILE_PERMISSION_DENIED : JSL_COPY_FILE_COULD_NOT_OPEN_DEST;
        }

        bool copy_ok = false;
        if (proceed)
        {
            uint8_t copy_buf[8192];
            bool loop_ok = true;
            bool reached_eof = false;

            while (loop_ok)
            {
                int64_t bytes_read = (int64_t) read(src_fd, copy_buf, sizeof(copy_buf));
                bool read_failed = (bytes_read < 0);
                reached_eof = (bytes_read == 0);

                if (read_failed)
                {
                    res = JSL_COPY_FILE_READ_FAILED;
                    if (out_errno != NULL)
                        *out_errno = errno;
                }

                bool should_write = !read_failed && !reached_eof;
                int64_t bytes_written = -1;
                if (should_write)
                {
                    bytes_written = (int64_t) write(dst_fd, copy_buf, (size_t) bytes_read);
                }
                bool write_failed = should_write && (bytes_written != bytes_read);

                if (write_failed)
                {
                    res = JSL_COPY_FILE_WRITE_FAILED;
                    if (out_errno != NULL)
                        *out_errno = errno;
                }

                copy_ok = reached_eof;
                loop_ok = !read_failed && !write_failed && !reached_eof;
            }
        }

        if (opened_dst)
        {
            close(dst_fd);
        }
        if (opened_src)
        {
            close(src_fd);
        }

        if (copy_ok)
        {
            res = JSL_COPY_FILE_SUCCESS;
        }

    #else
        #error "Unsupported platform"
    #endif

    return res;
}

JSLCopyDirectoryResultEnum jsl_copy_directory(
    JSLImmutableMemory src_path,
    JSLImmutableMemory dst_path,
    int32_t* out_errno
)
{
    char dst_buffer[FILENAME_MAX + 1];
    char dst_entry_buffer[FILENAME_MAX + 1];
    JSLCopyDirectoryResultEnum res = JSL_COPY_DIRECTORY_BAD_PARAMETERS;

    bool proceed = (src_path.data != NULL
        && src_path.length > 0
        && dst_path.data != NULL
        && dst_path.length > 0);

    bool src_too_long = proceed && (src_path.length >= (int64_t) FILENAME_MAX);
    bool dst_too_long = proceed && !src_too_long && (dst_path.length >= (int64_t) FILENAME_MAX);

    if (src_too_long || dst_too_long)
    {
        res = JSL_COPY_DIRECTORY_PATH_TOO_LONG;
        proceed = false;
    }

    if (proceed)
    {
        JSL_MEMCPY(dst_buffer, dst_path.data, (size_t) dst_path.length);
        dst_buffer[dst_path.length] = '\0';
    }

    JSLFileTypeEnum src_type = JSL_FILE_TYPE_UNKNOWN;
    if (proceed)
        src_type = jsl_get_file_type(src_path);

    bool src_not_found = proceed && (src_type == JSL_FILE_TYPE_NOT_FOUND);
    bool src_not_dir   = proceed && !src_not_found && (src_type != JSL_FILE_TYPE_DIR);

    if (src_not_found)
    {
        res = JSL_COPY_DIRECTORY_SOURCE_NOT_FOUND;
        proceed = false;
    }

    if (src_not_dir)
    {
        res = JSL_COPY_DIRECTORY_SOURCE_NOT_A_DIRECTORY;
        proceed = false;
    }

    JSLMakeDirectoryResultEnum top_mkdir_res = JSL_MAKE_DIRECTORY_BAD_PARAMETERS;
    if (proceed)
        top_mkdir_res = jsl_make_directory(dst_path, out_errno);

    bool top_mkdir_ok     = proceed && (top_mkdir_res == JSL_MAKE_DIRECTORY_SUCCESS);
    bool top_mkdir_exists = proceed && (top_mkdir_res == JSL_MAKE_DIRECTORY_ALREADY_EXISTS);
    bool top_mkdir_perm   = proceed && (top_mkdir_res == JSL_MAKE_DIRECTORY_PERMISSION_DENIED);
    bool top_mkdir_fail   = proceed && !top_mkdir_ok && !top_mkdir_exists && !top_mkdir_perm;

    if (top_mkdir_exists)
    {
        res = JSL_COPY_DIRECTORY_DEST_ALREADY_EXISTS;
        proceed = false;
    }

    if (top_mkdir_perm)
    {
        res = JSL_COPY_DIRECTORY_PERMISSION_DENIED;
        proceed = false;
    }

    if (top_mkdir_fail)
    {
        res = JSL_COPY_DIRECTORY_COULD_NOT_CREATE_DEST;
        proceed = false;
    }

    JSLDirectoryIterator iterator;
    JSLDirectoryIteratorInitResultEnum init_res = JSL_DIRECTORY_ITERATOR_INIT_BAD_PARAMETERS;
    if (proceed)
        init_res = jsl_directory_iterator_init(src_path, &iterator, false);

    bool init_ok        = proceed && (init_res == JSL_DIRECTORY_ITERATOR_INIT_SUCCESS);
    bool init_not_found = proceed && !init_ok && (init_res == JSL_DIRECTORY_ITERATOR_INIT_NOT_FOUND);
    bool init_not_dir   = proceed && !init_ok && (init_res == JSL_DIRECTORY_ITERATOR_INIT_NOT_A_DIRECTORY);
    bool init_other     = proceed && !init_ok && !init_not_found && !init_not_dir;

    if (init_not_found)
    {
        res = JSL_COPY_DIRECTORY_SOURCE_NOT_FOUND;
        proceed = false;
    }

    if (init_not_dir)
    {
        res = JSL_COPY_DIRECTORY_SOURCE_NOT_A_DIRECTORY;
        proceed = false;
    }

    if (init_other)
    {
        res = JSL_COPY_DIRECTORY_COULD_NOT_OPEN_SOURCE;
        proceed = false;
    }

    if (init_ok)
    {
        res = JSL_COPY_DIRECTORY_SUCCESS;
    }

    #if JSL_IS_WINDOWS
        char sep = '\\';
    #else
        char sep = '/';
    #endif

    JSLDirectoryIteratorResult iter_result = {0};
    bool keep_going = init_ok;
    while (keep_going && jsl_directory_iterator_next(&iterator, &iter_result, out_errno))
    {
        int64_t dst_base_len = dst_path.length;
        int64_t rel_len      = iter_result.relative_path.length;
        int64_t total_len    = dst_base_len + 1 + rel_len;

        bool entry_too_long = (total_len >= (int64_t) FILENAME_MAX);
        bool iter_error = !entry_too_long
            && (iter_result.status != JSL_DIRECTORY_ITERATOR_OK)
            && (iter_result.status != JSL_DIRECTORY_ITERATOR_PARTIAL);
        bool is_dir  = !entry_too_long && !iter_error
            && (iter_result.type == JSL_FILE_TYPE_DIR);
        bool is_file = !entry_too_long && !iter_error
            && (iter_result.type == JSL_FILE_TYPE_REG);

        if (!entry_too_long && !iter_error)
        {
            JSL_MEMCPY(dst_entry_buffer, dst_buffer, (size_t) dst_base_len);
            dst_entry_buffer[dst_base_len] = sep;
            JSL_MEMCPY(
                dst_entry_buffer + dst_base_len + 1,
                iter_result.relative_path.data,
                (size_t) rel_len
            );
            dst_entry_buffer[total_len] = '\0';
        }

        if (entry_too_long)
        {
            res = JSL_COPY_DIRECTORY_PATH_TOO_LONG;
            keep_going = false;
        }

        if (iter_error)
        {
            res = JSL_COPY_DIRECTORY_ERROR_UNKNOWN;
            keep_going = false;
        }

        JSLImmutableMemory dst_entry_mem;
        dst_entry_mem.data   = (const uint8_t*) dst_entry_buffer;
        dst_entry_mem.length = total_len;

        JSLMakeDirectoryResultEnum sub_mkdir_res = JSL_MAKE_DIRECTORY_BAD_PARAMETERS;
        bool sub_mkdir_ok = false;
        bool sub_mkdir_perm = false;

        if (is_dir)
        {
            sub_mkdir_res = jsl_make_directory(dst_entry_mem, out_errno);
            sub_mkdir_ok = sub_mkdir_res == JSL_MAKE_DIRECTORY_SUCCESS;
            sub_mkdir_perm = sub_mkdir_res == JSL_MAKE_DIRECTORY_PERMISSION_DENIED;
        }

        if (is_dir && sub_mkdir_perm)
        {
            res = JSL_COPY_DIRECTORY_PERMISSION_DENIED;
            keep_going = false;
        }

        if (is_dir && !sub_mkdir_ok && !sub_mkdir_perm)
        {
            res = JSL_COPY_DIRECTORY_MAKE_SUBDIR_FAILED;
            keep_going = false;
        }

        JSLCopyFileResultEnum copy_file_res = JSL_COPY_FILE_BAD_PARAMETERS;
        bool copy_ok = false;
        bool copy_perm = false;
        if (is_file)
        {
            copy_file_res = jsl_copy_file(iter_result.absolute_path, dst_entry_mem, out_errno);
            copy_ok = (copy_file_res == JSL_COPY_FILE_SUCCESS);
            copy_perm = (copy_file_res == JSL_COPY_FILE_PERMISSION_DENIED);
        }

        if (is_file && copy_perm)
        {
            res = JSL_COPY_DIRECTORY_PERMISSION_DENIED;
            keep_going = false;
        }

        if (is_file && !copy_ok && !copy_perm)
        {
            res = JSL_COPY_DIRECTORY_COPY_FILE_FAILED;
            keep_going = false;
        }
    }

    jsl_directory_iterator_end(&iterator);

    return res;
}

JSLRenameFileResultEnum jsl_rename_file(
    JSLImmutableMemory src_path,
    JSLImmutableMemory dst_path,
    int32_t* out_errno
)
{
    char src_buffer[FILENAME_MAX + 1];
    char dst_buffer[FILENAME_MAX + 1];
    JSLRenameFileResultEnum res = JSL_RENAME_FILE_BAD_PARAMETERS;
    bool proceed = false;

    proceed = (src_path.data != NULL
        && src_path.length > 0
        && dst_path.data != NULL
        && dst_path.length > 0);

    bool path_too_long = proceed && (src_path.length >= (int64_t) FILENAME_MAX
        || dst_path.length >= (int64_t) FILENAME_MAX);
    if (path_too_long)
    {
        res = JSL_RENAME_FILE_PATH_TOO_LONG;
        proceed = false;
    }

    if (proceed)
    {
        JSL_MEMCPY(src_buffer, src_path.data, (size_t) src_path.length);
        src_buffer[src_path.length] = '\0';
        JSL_MEMCPY(dst_buffer, dst_path.data, (size_t) dst_path.length);
        dst_buffer[dst_path.length] = '\0';
    }

    #if JSL_IS_WINDOWS

        BOOL move_ok = FALSE;
        if (proceed)
        {
            move_ok = MoveFileExA(
                src_buffer,
                dst_buffer,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED
            );
        }

        bool move_success = proceed && (move_ok != FALSE);
        if (move_success)
        {
            res = JSL_RENAME_FILE_SUCCESS;
        }

        bool move_failed = proceed && (move_ok == FALSE);
        if (move_failed)
        {
            DWORD last_err = GetLastError();
            if (out_errno != NULL)
                *out_errno = (int32_t) last_err;

            bool is_not_found = (last_err == ERROR_FILE_NOT_FOUND
                || last_err == ERROR_PATH_NOT_FOUND);
            bool is_access_denied = (last_err == ERROR_ACCESS_DENIED);
            bool is_already_exists = (last_err == ERROR_ALREADY_EXISTS);
            bool is_cross_device = (last_err == ERROR_NOT_SAME_DEVICE);

            if (is_not_found)
                res = JSL_RENAME_FILE_SOURCE_NOT_FOUND;
            if (is_access_denied)
                res = JSL_RENAME_FILE_PERMISSION_DENIED;
            if (is_already_exists)
                res = JSL_RENAME_FILE_DEST_ALREADY_EXISTS;
            if (is_cross_device)
                res = JSL_RENAME_FILE_CROSS_DEVICE;
            if (!is_not_found && !is_access_denied && !is_already_exists && !is_cross_device)
                res = JSL_RENAME_FILE_ERROR_UNKNOWN;
        }

    #elif JSL_IS_POSIX

        int32_t rename_ret = -1;
        if (proceed)
        {
            rename_ret = rename(src_buffer, dst_buffer);
        }

        bool rename_ok = proceed && (rename_ret == 0);
        if (rename_ok)
        {
            res = JSL_RENAME_FILE_SUCCESS;
        }

        bool rename_failed = proceed && (rename_ret != 0);
        if (rename_failed)
        {
            int32_t err = errno;
            if (out_errno != NULL)
                *out_errno = err;

            switch (err)
            {
                case ENOENT:
                    res = JSL_RENAME_FILE_SOURCE_NOT_FOUND;
                    break;
                case EACCES:
                case EPERM:
                    res = JSL_RENAME_FILE_PERMISSION_DENIED;
                    break;
                case EEXIST:
                case ENOTEMPTY:
                    res = JSL_RENAME_FILE_DEST_ALREADY_EXISTS;
                    break;
                case ENAMETOOLONG:
                    res = JSL_RENAME_FILE_PATH_TOO_LONG;
                    break;
                case EXDEV:
                    res = JSL_RENAME_FILE_CROSS_DEVICE;
                    break;
                case EROFS:
                    res = JSL_RENAME_FILE_READ_ONLY_FS;
                    break;
                default:
                    res = JSL_RENAME_FILE_ERROR_UNKNOWN;
                    break;
            }
        }

    #else
        #error "Unsupported platform"
    #endif

    return res;
}

#define JSL__SUBPROCESS_PRIVATE_SENTINEL 4729185036281947563U
#define JSL__SUBPROCESS_INITIAL_CAPACITY 8

JSLSubProcessCreateResultEnum jsl_subprocess_create(
    JSLSubprocess* proc,
    JSLAllocatorInterface allocator,
    JSLImmutableMemory executable
)
{
    bool proceed = (proc != NULL
        && executable.data != NULL
        && executable.length > 0);

    if (!proceed)
        return JSL_SUBPROCESS_CREATE_BAD_PARAMETERS;

    JSLImmutableMemory exe_dup = jsl_duplicate(allocator, executable);
    proceed = (exe_dup.data != NULL);

    if (!proceed)
        return JSL_SUBPROCESS_CREATE_COULD_NOT_ALLOCATE;

    JSLImmutableMemory* args = (JSLImmutableMemory*) jsl_allocator_interface_alloc(
        allocator,
        (int64_t)(JSL__SUBPROCESS_INITIAL_CAPACITY * sizeof(JSLImmutableMemory)),
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        true
    );

    proceed = (args != NULL);

    if (!proceed)
        return JSL_SUBPROCESS_CREATE_COULD_NOT_ALLOCATE;

    JSLSubProcessEnvVar* env_vars = (JSLSubProcessEnvVar*) jsl_allocator_interface_alloc(
        allocator,
        (int64_t)(JSL__SUBPROCESS_INITIAL_CAPACITY * sizeof(JSLSubProcessEnvVar)),
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        true
    );

    proceed = (env_vars != NULL);

    if (!proceed)
        return JSL_SUBPROCESS_CREATE_COULD_NOT_ALLOCATE;

    proc->sentinel = JSL__SUBPROCESS_PRIVATE_SENTINEL;
    proc->allocator = allocator;
    proc->status = JSL_SUBPROCESS_STATUS_NOT_STARTED;
    proc->exit_code = -1;
    proc->last_errno = 0;
    proc->executable = exe_dup;
    proc->args = args;
    proc->args_count = 0;
    proc->args_capacity = JSL__SUBPROCESS_INITIAL_CAPACITY;
    proc->env_vars = env_vars;
    proc->env_count = 0;
    proc->env_capacity = JSL__SUBPROCESS_INITIAL_CAPACITY;
    proc->working_directory = jsl_immutable_memory(NULL, 0);

    proc->stdin_kind = JSL_SUBPROCESS_STDIN_INHERIT;
    proc->stdin_memory = jsl_immutable_memory(NULL, 0);
    proc->stdin_fd = -1;

    proc->stdout_kind = JSL_SUBPROCESS_OUTPUT_INHERIT;
    proc->stdout_fd = -1;
    proc->stdout_sink.write_fp = NULL;
    proc->stdout_sink.user_data = NULL;

    proc->stderr_kind = JSL_SUBPROCESS_OUTPUT_INHERIT;
    proc->stderr_fd = -1;
    proc->stderr_sink.write_fp = NULL;
    proc->stderr_sink.user_data = NULL;

    proc->is_background = false;
    proc->stdin_write_offset = 0;
    proc->stdout_eof_seen = false;
    proc->stderr_eof_seen = false;
#if JSL_IS_WINDOWS
    proc->process_handle = INVALID_HANDLE_VALUE;
    proc->stdin_write_handle = INVALID_HANDLE_VALUE;
    proc->stdout_read_handle = INVALID_HANDLE_VALUE;
    proc->stderr_read_handle = INVALID_HANDLE_VALUE;
    ZeroMemory(&proc->stdin_write_overlapped, sizeof(proc->stdin_write_overlapped));
    ZeroMemory(&proc->stdout_read_overlapped, sizeof(proc->stdout_read_overlapped));
    ZeroMemory(&proc->stderr_read_overlapped, sizeof(proc->stderr_read_overlapped));
    proc->stdin_write_event = NULL;
    proc->stdout_read_event = NULL;
    proc->stderr_read_event = NULL;
    proc->stdin_write_pending = false;
    proc->stdout_read_pending = false;
    proc->stderr_read_pending = false;
    proc->stdin_write_buffer_len = 0;
#elif JSL_IS_POSIX
    proc->pid = 0;
    proc->stdin_write_fd = -1;
    proc->stdout_read_fd = -1;
    proc->stderr_read_fd = -1;
#endif

    return JSL_SUBPROCESS_CREATE_SUCCESS;
}

JSLSubProcessArgResultEnum jsl_subprocess_args(
    JSLSubprocess* proc,
    const JSLImmutableMemory* args,
    int64_t count
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && args != NULL
        && count > 0);

    if (!proceed)
        return JSL_SUBPROCESS_ARG_BAD_PARAMETERS;

    int64_t needed = proc->args_count + count;

    if (needed > proc->args_capacity)
    {
        int64_t new_capacity = proc->args_capacity;
        while (new_capacity < needed)
            new_capacity *= 2;

        JSLImmutableMemory* new_args = (JSLImmutableMemory*) jsl_allocator_interface_realloc(
            proc->allocator,
            proc->args,
            new_capacity * (int64_t) sizeof(JSLImmutableMemory),
            JSL_DEFAULT_ALLOCATION_ALIGNMENT
        );

        proceed = (new_args != NULL);

        if (!proceed)
            return JSL_SUBPROCESS_ARG_COULD_NOT_ALLOCATE;

        proc->args = new_args;
        proc->args_capacity = new_capacity;
    }

    for (int64_t i = 0; i < count; i++)
    {
        JSLImmutableMemory arg_dup = jsl_duplicate(proc->allocator, args[i]);
        proceed = (arg_dup.data != NULL);

        if (!proceed)
            return JSL_SUBPROCESS_ARG_COULD_NOT_ALLOCATE;

        proc->args[proc->args_count] = arg_dup;
        proc->args_count++;
    }

    return JSL_SUBPROCESS_ARG_SUCCESS;
}

JSLSubProcessArgResultEnum jsl__subprocess_args_va(
    JSLSubprocess* proc,
    ...
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);

    if (!proceed)
        return JSL_SUBPROCESS_ARG_BAD_PARAMETERS;

    JSLSubProcessArgResultEnum result = JSL_SUBPROCESS_ARG_SUCCESS;
    va_list va;
    va_start(va, proc);

    while (proceed)
    {
        JSLImmutableMemory arg = va_arg(va, JSLImmutableMemory);
        bool is_sentinel = (arg.data == NULL && arg.length < 0);
        if (is_sentinel)
            break;

        result = jsl_subprocess_args(proc, &arg, 1);
        proceed = (result == JSL_SUBPROCESS_ARG_SUCCESS);
    }

    va_end(va);
    return result;
}

JSLSubProcessArgResultEnum jsl__subprocess_args_cstr_va(
    JSLSubprocess* proc,
    ...
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);

    if (!proceed)
        return JSL_SUBPROCESS_ARG_BAD_PARAMETERS;

    JSLSubProcessArgResultEnum result = JSL_SUBPROCESS_ARG_SUCCESS;
    va_list va;
    va_start(va, proc);

    while (proceed)
    {
        const char* arg_cstr = va_arg(va, const char*);
        if (arg_cstr == NULL)
            break;

        JSLImmutableMemory arg = jsl_cstr_to_memory(arg_cstr);
        result = jsl_subprocess_args(proc, &arg, 1);
        proceed = (result == JSL_SUBPROCESS_ARG_SUCCESS);
    }

    va_end(va);
    return result;
}

JSLSubProcessEnvResultEnum jsl_subprocess_env(
    JSLSubprocess* proc,
    JSLImmutableMemory key,
    JSLImmutableMemory value
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && key.data != NULL
        && key.length > 0
        && value.data != NULL);

    if (!proceed)
        return JSL_SUBPROCESS_ENV_BAD_PARAMETERS;

    if (proc->env_count >= proc->env_capacity)
    {
        int64_t new_capacity = proc->env_capacity * 2;
        JSLSubProcessEnvVar* new_env = (JSLSubProcessEnvVar*) jsl_allocator_interface_realloc(
            proc->allocator,
            proc->env_vars,
            new_capacity * (int64_t) sizeof(JSLSubProcessEnvVar),
            JSL_DEFAULT_ALLOCATION_ALIGNMENT
        );

        proceed = (new_env != NULL);

        if (!proceed)
            return JSL_SUBPROCESS_ENV_COULD_NOT_ALLOCATE;

        proc->env_vars = new_env;
        proc->env_capacity = new_capacity;
    }

    JSLImmutableMemory key_dup = jsl_duplicate(proc->allocator, key);
    proceed = (key_dup.data != NULL);

    if (!proceed)
        return JSL_SUBPROCESS_ENV_COULD_NOT_ALLOCATE;

    JSLImmutableMemory value_dup = jsl_duplicate(proc->allocator, value);
    proceed = (value_dup.data != NULL);

    if (!proceed)
        return JSL_SUBPROCESS_ENV_COULD_NOT_ALLOCATE;

    proc->env_vars[proc->env_count].key = key_dup;
    proc->env_vars[proc->env_count].value = value_dup;
    proc->env_count++;

    return JSL_SUBPROCESS_ENV_SUCCESS;
}

bool jsl_subprocess_change_working_directory(
    JSLSubprocess* proc,
    JSLImmutableMemory path
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && path.data != NULL
        && path.length > 0);

    JSLImmutableMemory path_dup = {0};

    if (proceed)
    {
        path_dup = jsl_duplicate(proc->allocator, path);
        proceed = (path_dup.data != NULL);
    }

    if (proceed)
        proc->working_directory = path_dup;

    return proceed;
}

bool jsl_subprocess_set_stdin_memory(
    JSLSubprocess* proc,
    JSLImmutableMemory data
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && data.data != NULL
        && data.length >= 0);

    JSLImmutableMemory data_dup = jsl_immutable_memory(NULL, 0);

    if (proceed && data.length > 0)
    {
        data_dup = jsl_duplicate(proc->allocator, data);
        proceed = (data_dup.data != NULL);
    }

    if (proceed)
    {
        proc->stdin_kind = JSL_SUBPROCESS_STDIN_MEMORY;
        proc->stdin_memory = data_dup;
        proc->stdin_fd = -1;
    }

    return proceed;
}

bool jsl_subprocess_set_stdin_fd(
    JSLSubprocess* proc,
    int fd
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && fd >= 0);

    if (proceed)
    {
        proc->stdin_kind = JSL_SUBPROCESS_STDIN_FD;
        proc->stdin_fd = fd;
        proc->stdin_memory = jsl_immutable_memory(NULL, 0);
    }

    return proceed;
}

bool jsl_subprocess_set_stdin_null(JSLSubprocess* proc)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);

    if (proceed)
    {
        proc->stdin_kind = JSL_SUBPROCESS_STDIN_NULL;
        proc->stdin_memory = jsl_immutable_memory(NULL, 0);
        proc->stdin_fd = -1;
    }

    return proceed;
}

bool jsl_subprocess_set_stdout_fd(
    JSLSubprocess* proc,
    int fd
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && fd >= 0);

    if (proceed)
    {
        proc->stdout_kind = JSL_SUBPROCESS_OUTPUT_FD;
        proc->stdout_fd = fd;
        proc->stdout_sink.write_fp = NULL;
        proc->stdout_sink.user_data = NULL;
    }

    return proceed;
}

bool jsl_subprocess_set_stdout_sink(
    JSLSubprocess* proc,
    JSLOutputSink sink
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && sink.write_fp != NULL);

    if (proceed)
    {
        proc->stdout_kind = JSL_SUBPROCESS_OUTPUT_SINK;
        proc->stdout_sink = sink;
        proc->stdout_fd = -1;
    }

    return proceed;
}

bool jsl_subprocess_set_stderr_fd(
    JSLSubprocess* proc,
    int fd
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && fd >= 0);

    if (proceed)
    {
        proc->stderr_kind = JSL_SUBPROCESS_OUTPUT_FD;
        proc->stderr_fd = fd;
        proc->stderr_sink.write_fp = NULL;
        proc->stderr_sink.user_data = NULL;
    }

    return proceed;
}

bool jsl_subprocess_set_stderr_sink(
    JSLSubprocess* proc,
    JSLOutputSink sink
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && sink.write_fp != NULL);

    if (proceed)
    {
        proc->stderr_kind = JSL_SUBPROCESS_OUTPUT_SINK;
        proc->stderr_sink = sink;
        proc->stderr_fd = -1;
    }

    return proceed;
}

bool jsl_subprocess_set_stdout_null(JSLSubprocess* proc)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);

    if (proceed)
    {
        proc->stdout_kind = JSL_SUBPROCESS_OUTPUT_NULL;
        proc->stdout_fd = -1;
        proc->stdout_sink.write_fp = NULL;
        proc->stdout_sink.user_data = NULL;
    }

    return proceed;
}

bool jsl_subprocess_set_stderr_null(JSLSubprocess* proc)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);

    if (proceed)
    {
        proc->stderr_kind = JSL_SUBPROCESS_OUTPUT_NULL;
        proc->stderr_fd = -1;
        proc->stderr_sink.write_fp = NULL;
        proc->stderr_sink.user_data = NULL;
    }

    return proceed;
}

// Current monotonic time in milliseconds. Used to implement the
// finite-timeout deadline arithmetic shared by `jsl_subprocess_run_blocking`
// and `jsl_subprocess_background_wait`.
static int64_t jsl__monotonic_ms(void)
{
#if JSL_IS_POSIX
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return ((int64_t) ts.tv_sec) * 1000 + ((int64_t) ts.tv_nsec) / 1000000;
#elif JSL_IS_WINDOWS
    return (int64_t) GetTickCount64();
#else
    return 0;
#endif
}

#if JSL_IS_POSIX

// Everything needed to spawn a child process and pump its I/O. Populated
// by `jsl__subprocess_posix_prepare`, consumed by
// `jsl__subprocess_posix_spawn` and `jsl__subprocess_posix_pump_io`.
//
// Designed to be reusable by a future non-blocking API: the same struct
// can back an async start (prepare + spawn + close_child_pipe_ends,
// returning to the caller) and a later poll/wait step.
typedef struct JSL__SubProcessPosixLaunch
{
    // argv passed to posix_spawnp. NULL-terminated. All the string slots
    // were allocated from proc->allocator.
    char** argv;
    int64_t argv_count; // excludes the trailing NULL

    // envp passed to posix_spawnp. May alias `environ` when no custom env
    // vars are set, or be a freshly allocated array that references both
    // environ entries and our owned "key=value" strings.
    char** envp;
    bool envp_is_owned;
    // When envp_is_owned, the strings at envp[envp_owned_start ..
    // envp_owned_start + envp_owned_count) were allocated from
    // proc->allocator. Entries before that range are borrowed from environ.
    int64_t envp_owned_start;
    int64_t envp_owned_count;

    // Working directory C string, NULL to inherit the parent's cwd.
    char* cwd;

    // Pipes: [0] = read end, [1] = write end. -1 where unused.
    int stdin_pipe[2];
    int stdout_pipe[2];
    int stderr_pipe[2];
} JSL__SubProcessPosixLaunch;

static void jsl__subprocess_close_fd(int* fd)
{
    if (*fd != -1)
    {
        close(*fd);
        *fd = -1;
    }
}

static void jsl__subprocess_posix_launch_zero(JSL__SubProcessPosixLaunch* ctx)
{
    ctx->argv = NULL;
    ctx->argv_count = 0;
    ctx->envp = NULL;
    ctx->envp_is_owned = false;
    ctx->envp_owned_start = 0;
    ctx->envp_owned_count = 0;
    ctx->cwd = NULL;
    ctx->stdin_pipe[0] = -1;
    ctx->stdin_pipe[1] = -1;
    ctx->stdout_pipe[0] = -1;
    ctx->stdout_pipe[1] = -1;
    ctx->stderr_pipe[0] = -1;
    ctx->stderr_pipe[1] = -1;
}

// Close every pipe end in the context. Idempotent.
static void jsl__subprocess_posix_close_all_pipes(JSL__SubProcessPosixLaunch* ctx)
{
    jsl__subprocess_close_fd(&ctx->stdin_pipe[0]);
    jsl__subprocess_close_fd(&ctx->stdin_pipe[1]);
    jsl__subprocess_close_fd(&ctx->stdout_pipe[0]);
    jsl__subprocess_close_fd(&ctx->stdout_pipe[1]);
    jsl__subprocess_close_fd(&ctx->stderr_pipe[0]);
    jsl__subprocess_close_fd(&ctx->stderr_pipe[1]);
}

// Close the pipe ends that belong to the child after a successful spawn.
// Call this exactly once after `posix_spawnp` returns success.
static void jsl__subprocess_posix_close_child_pipe_ends(JSL__SubProcessPosixLaunch* ctx)
{
    jsl__subprocess_close_fd(&ctx->stdin_pipe[0]);
    jsl__subprocess_close_fd(&ctx->stdout_pipe[1]);
    jsl__subprocess_close_fd(&ctx->stderr_pipe[1]);
}

// Free every allocation made by `jsl__subprocess_posix_prepare`. The
// kernel has already copied argv/envp/cwd once posix_spawnp returns, so
// these strings are no longer needed and can be released regardless of
// whether the spawn succeeded.
static void jsl__subprocess_posix_free_launch(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx
)
{
    if (ctx->argv != NULL)
    {
        for (int64_t i = 0; i < ctx->argv_count; i++)
        {
            if (ctx->argv[i] != NULL)
                (void) jsl_allocator_interface_free(proc->allocator, ctx->argv[i]);
        }
        (void) jsl_allocator_interface_free(proc->allocator, ctx->argv);
        ctx->argv = NULL;
        ctx->argv_count = 0;
    }

    if (ctx->envp_is_owned && ctx->envp != NULL)
    {
        for (int64_t i = 0; i < ctx->envp_owned_count; i++)
        {
            char* s = ctx->envp[ctx->envp_owned_start + i];
            if (s != NULL)
                (void) jsl_allocator_interface_free(proc->allocator, s);
        }
        (void) jsl_allocator_interface_free(proc->allocator, ctx->envp);
    }
    ctx->envp = NULL;
    ctx->envp_is_owned = false;
    ctx->envp_owned_start = 0;
    ctx->envp_owned_count = 0;

    if (ctx->cwd != NULL)
    {
        (void) jsl_allocator_interface_free(proc->allocator, ctx->cwd);
        ctx->cwd = NULL;
    }
}

// Build the argv array (executable + each configured arg) into ctx->argv.
// Returns false on allocation failure. On failure the partial argv is
// left in ctx so the caller can hand the whole context to
// `jsl__subprocess_posix_free_launch`.
static bool jsl__subprocess_posix_build_argv(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx
)
{
    int64_t slots = proc->args_count + 2;
    ctx->argv = (char**) jsl_allocator_interface_alloc(
        proc->allocator,
        slots * (int64_t) sizeof(char*),
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        true
    );
    ctx->argv_count = slots - 1; // record early so cleanup knows how many cells to inspect
    bool ok = (ctx->argv != NULL);

    if (ok)
    {
        ctx->argv[0] = (char*) jsl_memory_to_cstr(proc->allocator, proc->executable);
        ok = (ctx->argv[0] != NULL);
    }

    for (int64_t i = 0; ok && i < proc->args_count; i++)
    {
        ctx->argv[i + 1] = (char*) jsl_memory_to_cstr(proc->allocator, proc->args[i]);
        ok = (ctx->argv[i + 1] != NULL);
    }

    if (ok)
        ctx->argv[slots - 1] = NULL;

    return ok;
}

// Build the envp array. If no custom env vars are configured, ctx->envp
// aliases `environ` directly with no allocation. Otherwise we allocate an
// array large enough for every entry in environ plus one "key=value"
// string per configured env var, in that order.
static bool jsl__subprocess_posix_build_envp(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx
)
{
    if (proc->env_count == 0)
    {
        ctx->envp = environ;
        ctx->envp_is_owned = false;
        return true;
    }

    int64_t parent_count = 0;
    if (environ != NULL)
    {
        while (environ[parent_count] != NULL)
            parent_count++;
    }

    int64_t total = parent_count + proc->env_count + 1;
    ctx->envp = (char**) jsl_allocator_interface_alloc(
        proc->allocator,
        total * (int64_t) sizeof(char*),
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        true
    );
    ctx->envp_is_owned = true;
    ctx->envp_owned_start = parent_count;
    ctx->envp_owned_count = proc->env_count;

    bool ok = (ctx->envp != NULL);
    if (!ok)
        return false;

    for (int64_t i = 0; i < parent_count; i++)
        ctx->envp[i] = environ[i];

    for (int64_t i = 0; ok && i < proc->env_count; i++)
    {
        int64_t klen = proc->env_vars[i].key.length;
        int64_t vlen = proc->env_vars[i].value.length;
        int64_t total_len = klen + 1 + vlen + 1;
        char* kv = (char*) jsl_allocator_interface_alloc(
            proc->allocator,
            total_len,
            JSL_DEFAULT_ALLOCATION_ALIGNMENT,
            false
        );
        ok = (kv != NULL);
        if (ok)
        {
            JSL_MEMCPY(kv, proc->env_vars[i].key.data, (size_t) klen);
            kv[klen] = '=';
            if (vlen > 0)
                JSL_MEMCPY(kv + klen + 1, proc->env_vars[i].value.data, (size_t) vlen);
            kv[klen + 1 + vlen] = '\0';
            ctx->envp[parent_count + i] = kv;
        }
    }

    ctx->envp[total - 1] = NULL;
    return ok;
}

static bool jsl__subprocess_posix_build_cwd(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx
)
{
    bool has_cwd = (proc->working_directory.data != NULL
        && proc->working_directory.length > 0);
    bool ok = true;
    if (has_cwd)
    {
        ctx->cwd = (char*) jsl_memory_to_cstr(proc->allocator, proc->working_directory);
        ok = (ctx->cwd != NULL);
    }
    return ok;
}

static bool jsl__subprocess_posix_create_pipes(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx
)
{
    bool ok = true;
    if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY)
        ok = (pipe(ctx->stdin_pipe) == 0);
    if (ok && proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
        ok = (pipe(ctx->stdout_pipe) == 0);
    if (ok && proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
        ok = (pipe(ctx->stderr_pipe) == 0);
    return ok;
}

// Build argv/envp/cwd and create pipes. On any failure the context is
// fully populated as far as it got and the caller is responsible for
// calling `jsl__subprocess_posix_free_launch` to release partial
// allocations.
static JSLSubProcessResultEnum jsl__subprocess_posix_prepare(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx,
    int32_t* out_errno
)
{
    jsl__subprocess_posix_launch_zero(ctx);

    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;

    if (!jsl__subprocess_posix_build_argv(proc, ctx))
        result = JSL_SUBPROCESS_ALLOCATION_FAILED;

    if (result == JSL_SUBPROCESS_SUCCESS
        && !jsl__subprocess_posix_build_envp(proc, ctx))
        result = JSL_SUBPROCESS_ALLOCATION_FAILED;

    if (result == JSL_SUBPROCESS_SUCCESS
        && !jsl__subprocess_posix_build_cwd(proc, ctx))
        result = JSL_SUBPROCESS_ALLOCATION_FAILED;

    if (result == JSL_SUBPROCESS_SUCCESS
        && !jsl__subprocess_posix_create_pipes(proc, ctx))
    {
        int32_t saved = errno;
        if (out_errno != NULL)
            *out_errno = saved;
        jsl__subprocess_posix_close_all_pipes(ctx);
        result = JSL_SUBPROCESS_PIPE_FAILED;
    }

    return result;
}

// Invoke posix_spawnp using the prepared context. On success, the child
// is running; the caller must close the child-side pipe ends (via
// `jsl__subprocess_posix_close_child_pipe_ends`) and is responsible for
// the pid. On failure every pipe is closed and the ctx no longer owns
// any fds.
static JSLSubProcessResultEnum jsl__subprocess_posix_spawn(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx,
    pid_t* out_pid,
    int32_t* out_errno
)
{
    posix_spawn_file_actions_t file_actions;
    int err = posix_spawn_file_actions_init(&file_actions);
    if (err != 0)
    {
        if (out_errno != NULL)
            *out_errno = err;
        jsl__subprocess_posix_close_all_pipes(ctx);
        return JSL_SUBPROCESS_ALLOCATION_FAILED;
    }

    // Stdin
    if (err == 0 && proc->stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY)
    {
        err = posix_spawn_file_actions_adddup2(&file_actions, ctx->stdin_pipe[0], STDIN_FILENO);
        if (err == 0)
            err = posix_spawn_file_actions_addclose(&file_actions, ctx->stdin_pipe[0]);
        if (err == 0)
            err = posix_spawn_file_actions_addclose(&file_actions, ctx->stdin_pipe[1]);
    }
    else if (err == 0 && proc->stdin_kind == JSL_SUBPROCESS_STDIN_FD)
    {
        err = posix_spawn_file_actions_adddup2(&file_actions, proc->stdin_fd, STDIN_FILENO);
    }
    else if (err == 0 && proc->stdin_kind == JSL_SUBPROCESS_STDIN_NULL)
    {
        err = posix_spawn_file_actions_addopen(&file_actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    }

    // Stdout
    if (err == 0 && proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        err = posix_spawn_file_actions_adddup2(&file_actions, ctx->stdout_pipe[1], STDOUT_FILENO);
        if (err == 0)
            err = posix_spawn_file_actions_addclose(&file_actions, ctx->stdout_pipe[0]);
        if (err == 0)
            err = posix_spawn_file_actions_addclose(&file_actions, ctx->stdout_pipe[1]);
    }
    else if (err == 0 && proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_FD)
    {
        err = posix_spawn_file_actions_adddup2(&file_actions, proc->stdout_fd, STDOUT_FILENO);
    }
    else if (err == 0 && proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_NULL)
    {
        err = posix_spawn_file_actions_addopen(&file_actions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    }

    // Stderr
    if (err == 0 && proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        err = posix_spawn_file_actions_adddup2(&file_actions, ctx->stderr_pipe[1], STDERR_FILENO);
        if (err == 0)
            err = posix_spawn_file_actions_addclose(&file_actions, ctx->stderr_pipe[0]);
        if (err == 0)
            err = posix_spawn_file_actions_addclose(&file_actions, ctx->stderr_pipe[1]);
    }
    else if (err == 0 && proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_FD)
    {
        err = posix_spawn_file_actions_adddup2(&file_actions, proc->stderr_fd, STDERR_FILENO);
    }
    else if (err == 0 && proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_NULL)
    {
        err = posix_spawn_file_actions_addopen(&file_actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    }

    // chdir in the child before exec. Requires glibc >= 2.29, musl >=
    // 1.2.3, macOS >= 10.15, or a comparably recent BSD libc.
    if (err == 0 && ctx->cwd != NULL)
        err = posix_spawn_file_actions_addchdir_np(&file_actions, ctx->cwd);

    pid_t pid = 0;
    if (err == 0)
    {
        err = posix_spawnp(
            &pid,
            ctx->argv[0],
            &file_actions,
            NULL,
            ctx->argv,
            ctx->envp
        );
    }

    (void) posix_spawn_file_actions_destroy(&file_actions);

    if (err != 0)
    {
        if (out_errno != NULL)
            *out_errno = err;
        jsl__subprocess_posix_close_all_pipes(ctx);
        return JSL_SUBPROCESS_SPAWN_FAILED;
    }

    *out_pid = pid;
    return JSL_SUBPROCESS_SUCCESS;
}

// Blocking poll() loop that feeds MEMORY stdin and drains SINK
// stdout/stderr. Returns true when every parent-side pipe end has been
// closed cleanly, false on a poll() error (in which case out_errno is
// set). Closes every pipe end it owns before returning.
// `deadline_ms` is an absolute monotonic-clock deadline (from
// `jsl__monotonic_ms()`). Pass < 0 for "no deadline". When the deadline
// expires before all pipes are closed, `*out_timed_out` is set to true
// and the loop returns early; the caller is expected to kill the child
// and reap it.
static bool jsl__subprocess_posix_pump_io(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx,
    int64_t deadline_ms,
    bool* out_timed_out,
    int32_t* out_errno
)
{
    int stdin_write = ctx->stdin_pipe[1];
    int stdout_read = ctx->stdout_pipe[0];
    int stderr_read = ctx->stderr_pipe[0];

    if (out_timed_out != NULL)
        *out_timed_out = false;

    bool infinite = (deadline_ms < 0);

    // Non-blocking stdin so a slow child can't deadlock our writes.
    if (stdin_write != -1)
    {
        int flags = fcntl(stdin_write, F_GETFL, 0);
        if (flags != -1)
            (void) fcntl(stdin_write, F_SETFL, flags | O_NONBLOCK);
    }

    // Ignore SIGPIPE during writes so a child that closes stdin early
    // surfaces as EPIPE instead of killing us.
    void (*prev_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);

    int64_t stdin_offset = 0;
    bool ok = true;
    bool timed_out = false;

    while (stdin_write != -1 || stdout_read != -1 || stderr_read != -1)
    {
        struct pollfd pfds[3];
        int nfds = 0;
        int stdin_idx = -1;
        int stdout_idx = -1;
        int stderr_idx = -1;

        if (stdin_write != -1)
        {
            pfds[nfds].fd = stdin_write;
            pfds[nfds].events = POLLOUT;
            pfds[nfds].revents = 0;
            stdin_idx = nfds++;
        }
        if (stdout_read != -1)
        {
            pfds[nfds].fd = stdout_read;
            pfds[nfds].events = POLLIN;
            pfds[nfds].revents = 0;
            stdout_idx = nfds++;
        }
        if (stderr_read != -1)
        {
            pfds[nfds].fd = stderr_read;
            pfds[nfds].events = POLLIN;
            pfds[nfds].revents = 0;
            stderr_idx = nfds++;
        }

        int wait_ms;
        if (infinite)
            wait_ms = -1;
        else
        {
            int64_t remaining = deadline_ms - jsl__monotonic_ms();
            if (remaining <= 0)
                wait_ms = 0;
            else if (remaining > (int64_t) INT_MAX)
                wait_ms = INT_MAX;
            else
                wait_ms = (int) remaining;
        }

        int pret = poll(pfds, (nfds_t) nfds, wait_ms);
        if (pret == -1)
        {
            if (errno == EINTR)
                continue;
            if (out_errno != NULL)
                *out_errno = errno;
            ok = false;
            break;
        }

        if (pret == 0 && !infinite)
        {
            // Deadline reached before any fd became ready.
            timed_out = true;
            break;
        }

        if (stdin_idx != -1
            && (pfds[stdin_idx].revents & (POLLOUT | POLLERR | POLLHUP)) != 0)
        {
            int64_t remaining = proc->stdin_memory.length - stdin_offset;
            bool done = (remaining <= 0);
            if (!done && (pfds[stdin_idx].revents & POLLOUT) != 0)
            {
                ssize_t w = write(
                    stdin_write,
                    proc->stdin_memory.data + stdin_offset,
                    (size_t) remaining
                );
                if (w > 0)
                {
                    stdin_offset += (int64_t) w;
                    done = (stdin_offset >= proc->stdin_memory.length);
                }
                else if (w == -1
                    && errno != EAGAIN
                    && errno != EWOULDBLOCK
                    && errno != EINTR)
                {
                    done = true;
                }
            }
            else if ((pfds[stdin_idx].revents & (POLLERR | POLLHUP)) != 0)
            {
                done = true;
            }

            if (done)
            {
                close(stdin_write);
                stdin_write = -1;
                ctx->stdin_pipe[1] = -1;
            }
        }

        uint8_t io_buf[4096];

        if (stdout_idx != -1
            && (pfds[stdout_idx].revents & (POLLIN | POLLERR | POLLHUP)) != 0)
        {
            ssize_t r = read(stdout_read, io_buf, sizeof(io_buf));
            if (r > 0)
            {
                jsl_output_sink_write(
                    proc->stdout_sink,
                    jsl_immutable_memory(io_buf, (int64_t) r)
                );
            }
            else if (r == 0 || (r == -1 && errno != EINTR))
            {
                close(stdout_read);
                stdout_read = -1;
                ctx->stdout_pipe[0] = -1;
            }
        }

        if (stderr_idx != -1
            && (pfds[stderr_idx].revents & (POLLIN | POLLERR | POLLHUP)) != 0)
        {
            ssize_t r = read(stderr_read, io_buf, sizeof(io_buf));
            if (r > 0)
            {
                jsl_output_sink_write(
                    proc->stderr_sink,
                    jsl_immutable_memory(io_buf, (int64_t) r)
                );
            }
            else if (r == 0 || (r == -1 && errno != EINTR))
            {
                close(stderr_read);
                stderr_read = -1;
                ctx->stderr_pipe[0] = -1;
            }
        }
    }

    // On a timeout, leave parent-side pipe ends open so the caller can
    // do a final post-kill drain. They are closed by the next pump pass
    // or by the close-on-exit logic in run_blocking.
    if (!timed_out)
    {
        if (stdin_write != -1) { close(stdin_write); ctx->stdin_pipe[1] = -1; }
        if (stdout_read != -1) { close(stdout_read); ctx->stdout_pipe[0] = -1; }
        if (stderr_read != -1) { close(stderr_read); ctx->stderr_pipe[0] = -1; }
    }

    signal(SIGPIPE, prev_sigpipe);

    if (out_timed_out != NULL)
        *out_timed_out = timed_out;
    return ok;
}

// Blocking waitpid that retries on EINTR. Translates the wait status
// into a `JSLSubProcessResultEnum` and a `JSLSubProcessStatusEnum`,
// and writes the child's exit code (or negated signal number) into
// *out_exit_code.
static JSLSubProcessResultEnum jsl__subprocess_posix_wait(
    pid_t pid,
    JSLSubProcessStatusEnum* out_status,
    int32_t* out_exit_code,
    int32_t* out_errno
)
{
    int wait_status = 0;
    pid_t wret;
    do {
        wret = waitpid(pid, &wait_status, 0);
    } while (wret == -1 && errno == EINTR);

    if (wret == -1)
    {
        if (out_errno != NULL)
            *out_errno = errno;
        *out_status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return JSL_SUBPROCESS_WAIT_FAILED;
    }

    if (WIFEXITED(wait_status))
    {
        *out_exit_code = (int32_t) WEXITSTATUS(wait_status);
        *out_status = JSL_SUBPROCESS_STATUS_EXITED;
        return JSL_SUBPROCESS_SUCCESS;
    }

    if (WIFSIGNALED(wait_status))
    {
        *out_exit_code = -(int32_t) WTERMSIG(wait_status);
        *out_status = JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL;
        return JSL_SUBPROCESS_KILLED_BY_SIGNAL;
    }

    *out_exit_code = -1;
    *out_status = JSL_SUBPROCESS_STATUS_EXITED;
    return JSL_SUBPROCESS_SUCCESS;
}

// Set O_NONBLOCK on a file descriptor. Silent on failure — the caller
// only needs best-effort non-blocking behavior here.
static void jsl__subprocess_posix_set_nonblocking(int fd)
{
    if (fd != -1)
    {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags != -1)
            (void) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

// One non-blocking write of the next chunk of stdin_memory to the child.
// Closes the parent-side write fd when the buffer is fully written or the
// child has closed its read end.
static JSLSubProcessResultEnum jsl__subprocess_posix_pump_stdin_once(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    if (proc->stdin_kind != JSL_SUBPROCESS_STDIN_MEMORY
        || proc->stdin_write_fd == -1)
        return JSL_SUBPROCESS_SUCCESS;

    int64_t remaining = proc->stdin_memory.length - proc->stdin_write_offset;
    if (remaining <= 0)
    {
        jsl__subprocess_close_fd(&proc->stdin_write_fd);
        return JSL_SUBPROCESS_SUCCESS;
    }

    void (*prev_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);

    ssize_t w = write(
        proc->stdin_write_fd,
        proc->stdin_memory.data + proc->stdin_write_offset,
        (size_t) remaining
    );
    int saved_errno = errno;

    signal(SIGPIPE, prev_sigpipe);

    bool close_pipe = false;
    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;
    if (w > 0)
    {
        proc->stdin_write_offset += (int64_t) w;
        if (proc->stdin_write_offset >= proc->stdin_memory.length)
            close_pipe = true;
    }
    else if (w == -1
        && (saved_errno == EPIPE
            || saved_errno == ENOTCONN
            || saved_errno == ECONNRESET))
    {
        // Child closed its read end of stdin. Not reported as an error.
        close_pipe = true;
    }
    else if (w == -1
        && saved_errno != EAGAIN
        && saved_errno != EWOULDBLOCK
        && saved_errno != EINTR)
    {
        // Real write failure: propagate so callers can surface it.
        if (out_errno != NULL)
            *out_errno = saved_errno;
        close_pipe = true;
        result = JSL_SUBPROCESS_IO_FAILED;
    }

    if (close_pipe)
        jsl__subprocess_close_fd(&proc->stdin_write_fd);

    return result;
}

// Drain a single read-end pipe into its sink, reading until the kernel
// buffer is empty (EAGAIN/EWOULDBLOCK), EOF, or a non-retryable error.
// Closes the fd and sets *eof_seen on EOF or non-retryable error. Returns
// IO_FAILED only on non-retryable read errors. Looping (rather than a
// single read) is required so post-reap drains don't silently truncate
// children that buffered more than 4 KiB before exiting.
static JSLSubProcessResultEnum jsl__subprocess_posix_drain_one(
    int* fd_ptr,
    JSLOutputSink sink,
    bool* eof_seen,
    int32_t* out_errno
)
{
    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;
    bool keep_reading = (*fd_ptr != -1) && !*eof_seen;

    while (keep_reading)
    {
        uint8_t io_buf[4096];
        ssize_t r = read(*fd_ptr, io_buf, sizeof(io_buf));
        int saved_errno = errno;

        if (r > 0)
        {
            jsl_output_sink_write(sink, jsl_immutable_memory(io_buf, (int64_t) r));
        }
        else if (r == 0)
        {
            jsl__subprocess_close_fd(fd_ptr);
            *eof_seen = true;
            keep_reading = false;
        }
        else if (r == -1 && saved_errno == EINTR)
        {
            // retry
        }
        else if (r == -1
            && (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK))
        {
            keep_reading = false;
        }
        else
        {
            if (out_errno != NULL)
                *out_errno = saved_errno;
            jsl__subprocess_close_fd(fd_ptr);
            *eof_seen = true;
            result = JSL_SUBPROCESS_IO_FAILED;
            keep_reading = false;
        }
    }

    return result;
}

// One non-blocking drain pass over stdout and stderr. Reads whatever is
// available right now in one pass per stream.
static JSLSubProcessResultEnum jsl__subprocess_posix_pump_output_once(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;

    if (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        JSLSubProcessResultEnum r = jsl__subprocess_posix_drain_one(
            &proc->stdout_read_fd,
            proc->stdout_sink,
            &proc->stdout_eof_seen,
            out_errno
        );
        if (r != JSL_SUBPROCESS_SUCCESS)
            result = r;
    }

    if (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        JSLSubProcessResultEnum r = jsl__subprocess_posix_drain_one(
            &proc->stderr_read_fd,
            proc->stderr_sink,
            &proc->stderr_eof_seen,
            result == JSL_SUBPROCESS_SUCCESS ? out_errno : NULL
        );
        if (r != JSL_SUBPROCESS_SUCCESS && result == JSL_SUBPROCESS_SUCCESS)
            result = r;
    }

    return result;
}

// Non-blocking / bounded-wait status check for a background subprocess.
// Returns SUCCESS and sets out_status to the current status. The status
// on the struct is also updated when the child is observed to have
// exited or been signaled.
static JSLSubProcessResultEnum jsl__subprocess_posix_poll(
    JSLSubprocess* proc,
    int32_t timeout_ms,
    JSLSubProcessStatusEnum* out_status,
    int32_t* out_exit_code,
    int32_t* out_errno
)
{
    // Already reaped — idempotent.
    if (proc->status == JSL_SUBPROCESS_STATUS_EXITED
        || proc->status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL)
    {
        *out_status = proc->status;
        return JSL_SUBPROCESS_SUCCESS;
    }

    int wait_status = 0;
    pid_t wret = 0;

    if (timeout_ms < 0)
    {
        do {
            wret = waitpid((pid_t) proc->pid, &wait_status, 0);
        } while (wret == -1 && errno == EINTR);
    }
    else
    {
        // Non-blocking or bounded poll.
        int64_t elapsed_ms = 0;
        for (;;)
        {
            do {
                wret = waitpid((pid_t) proc->pid, &wait_status, WNOHANG);
            } while (wret == -1 && errno == EINTR);

            if (wret != 0)
                break;
            if (timeout_ms == 0 || elapsed_ms >= timeout_ms)
                break;

            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000000L; // 1 ms
            (void) nanosleep(&ts, NULL);
            elapsed_ms += 1;
        }
    }

    if (wret == -1)
    {
        if (out_errno != NULL)
            *out_errno = errno;
        *out_status = proc->status;
        return JSL_SUBPROCESS_WAIT_FAILED;
    }

    if (wret == 0)
    {
        // Still running.
        *out_status = proc->status;
        return JSL_SUBPROCESS_SUCCESS;
    }

    if (WIFEXITED(wait_status))
    {
        *out_exit_code = (int32_t) WEXITSTATUS(wait_status);
        proc->status = JSL_SUBPROCESS_STATUS_EXITED;
    }
    else if (WIFSIGNALED(wait_status))
    {
        *out_exit_code = -(int32_t) WTERMSIG(wait_status);
        proc->status = JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL;
    }
    else
    {
        *out_exit_code = -1;
        proc->status = JSL_SUBPROCESS_STATUS_EXITED;
    }

    proc->pid = 0;
    *out_status = proc->status;
    return JSL_SUBPROCESS_SUCCESS;
}

// Global SIGCHLD self-pipe state. Lazy-initialized on first call to
// `jsl_subprocess_background_wait`. The previous SIGCHLD handler (if
// any) is saved so our handler can chain to it, keeping us polite to
// host programs that already listen for child termination.
static int jsl__sigchld_pipe[2] = {-1, -1};
static volatile sig_atomic_t jsl__sigchld_installed = 0;
static struct sigaction jsl__sigchld_prev_action;

static void jsl__sigchld_handler(int signum, siginfo_t* info, void* context)
{
    int saved_errno = errno;
    if (jsl__sigchld_pipe[1] != -1)
    {
        uint8_t byte = 1;
        ssize_t n;
        do {
            n = write(jsl__sigchld_pipe[1], &byte, 1);
        } while (n == -1 && errno == EINTR);
        // EAGAIN is fine: the pipe already carries a pending byte.
        (void) n;
    }

    // Chain to prior handler if the host installed one.
    if ((jsl__sigchld_prev_action.sa_flags & SA_SIGINFO) != 0)
    {
        if (jsl__sigchld_prev_action.sa_sigaction != NULL)
            jsl__sigchld_prev_action.sa_sigaction(signum, info, context);
    }
    else
    {
        void (*prev)(int) = jsl__sigchld_prev_action.sa_handler;
        if (prev != SIG_DFL && prev != SIG_IGN && prev != NULL)
            prev(signum);
    }

    errno = saved_errno;
}

// Best-effort idempotent init of the SIGCHLD self-pipe. Returns true
// when the pipe is ready. Caller is expected to still be able to
// `waitpid(pid, WNOHANG)` all children; the self-pipe only nudges the
// event loop awake and does not replace `waitpid`.
static bool jsl__sigchld_ensure_installed(int32_t* out_errno)
{
    if (jsl__sigchld_installed)
        return true;

    int pipe_fds[2] = {-1, -1};
    if (pipe(pipe_fds) != 0)
    {
        if (out_errno != NULL)
            *out_errno = errno;
        return false;
    }

    for (int i = 0; i < 2; i++)
    {
        int flags = fcntl(pipe_fds[i], F_GETFL, 0);
        if (flags != -1)
            (void) fcntl(pipe_fds[i], F_SETFL, flags | O_NONBLOCK);
        int fdflags = fcntl(pipe_fds[i], F_GETFD, 0);
        if (fdflags != -1)
            (void) fcntl(pipe_fds[i], F_SETFD, fdflags | FD_CLOEXEC);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = jsl__sigchld_handler;
    sa.sa_flags = SA_SIGINFO | SA_NOCLDSTOP | SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, &jsl__sigchld_prev_action) != 0)
    {
        if (out_errno != NULL)
            *out_errno = errno;
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return false;
    }

    jsl__sigchld_pipe[0] = pipe_fds[0];
    jsl__sigchld_pipe[1] = pipe_fds[1];
    jsl__sigchld_installed = 1;
    return true;
}

// Read everything currently buffered in the self-pipe. Used to clear
// the fd so the next SIGCHLD notification wakes us again.
static void jsl__sigchld_drain(void)
{
    if (jsl__sigchld_pipe[0] == -1)
        return;
    uint8_t scratch[64];
    for (;;)
    {
        ssize_t n = read(jsl__sigchld_pipe[0], scratch, sizeof(scratch));
        if (n == -1)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        if (n == 0)
            break;
    }
}

// One non-blocking wait on the proc's pid. Updates proc->status and
// proc->exit_code when the child is observed to have terminated.
// Returns true if the proc is (now) terminal, false if still running.
static bool jsl__subprocess_posix_reap_nohang(JSLSubprocess* proc)
{
    if (proc->status == JSL_SUBPROCESS_STATUS_EXITED
        || proc->status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL)
        return true;

    if (proc->pid <= 0)
        return false;

    int wait_status = 0;
    pid_t wret;
    do {
        wret = waitpid((pid_t) proc->pid, &wait_status, WNOHANG);
    } while (wret == -1 && errno == EINTR);

    if (wret <= 0)
        return false;

    if (WIFEXITED(wait_status))
    {
        proc->exit_code = (int32_t) WEXITSTATUS(wait_status);
        proc->status = JSL_SUBPROCESS_STATUS_EXITED;
    }
    else if (WIFSIGNALED(wait_status))
    {
        proc->exit_code = -(int32_t) WTERMSIG(wait_status);
        proc->status = JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL;
    }
    else
    {
        proc->exit_code = -1;
        proc->status = JSL_SUBPROCESS_STATUS_EXITED;
    }
    proc->pid = 0;
    return true;
}

#endif

#if JSL_IS_WINDOWS

// Quote a single argument according to the rules parsed by the Microsoft
// C runtime's CommandLineToArgvW. Writes the quoted form into `out`
// (advancing it) and returns the new end pointer. Does not null-terminate.
static char* jsl__subprocess_quote_arg_win(char* out, JSLImmutableMemory arg)
{
    // If the argument contains no whitespace, double quotes, or is empty,
    // it can be used as-is. Otherwise wrap in quotes and escape.
    bool needs_quote = (arg.length == 0);
    for (int64_t i = 0; i < arg.length && !needs_quote; i++)
    {
        uint8_t c = arg.data[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '"')
            needs_quote = true;
    }

    if (!needs_quote)
    {
        for (int64_t i = 0; i < arg.length; i++)
        {
            *out = (char) arg.data[i];
            out++;
        }
        return out;
    }

    *out = '"';
    out++;

    int64_t i = 0;
    while (i < arg.length)
    {
        int64_t backslashes = 0;
        while (i < arg.length && arg.data[i] == '\\')
        {
            backslashes++;
            i++;
        }

        if (i == arg.length)
        {
            // Trailing backslashes must be doubled before the closing quote.
            for (int64_t j = 0; j < backslashes * 2; j++)
            {
                *out = '\\';
                out++;
            }
            break;
        }
        else if (arg.data[i] == '"')
        {
            // Backslashes preceding a quote must be doubled, plus an extra
            // backslash to escape the quote.
            for (int64_t j = 0; j < backslashes * 2 + 1; j++)
            {
                *out = '\\';
                out++;
            }
            *out = '"';
            out++;
            i++;
        }
        else
        {
            for (int64_t j = 0; j < backslashes; j++)
            {
                *out = '\\';
                out++;
            }
            *out = (char) arg.data[i];
            out++;
            i++;
        }
    }

    *out = '"';
    out++;
    return out;
}

typedef struct JSL__SubProcessWindowsLaunch
{
    char* cmdline;
    char* env_block;
    char* cwd_cstr;
    HANDLE stdin_read;
    HANDLE stdin_write;
    HANDLE stdout_read;
    HANDLE stdout_write;
    HANDLE stderr_read;
    HANDLE stderr_write;
    // Manual-reset events paired with the overlapped parent ends. Only
    // the parent-side reads/writes are overlapped; the child ends are
    // opened for synchronous I/O.
    HANDLE stdin_write_event;
    HANDLE stdout_read_event;
    HANDLE stderr_read_event;
} JSL__SubProcessWindowsLaunch;

static void jsl__subprocess_win_launch_zero(JSL__SubProcessWindowsLaunch* ctx)
{
    ctx->cmdline = NULL;
    ctx->env_block = NULL;
    ctx->cwd_cstr = NULL;
    ctx->stdin_read = INVALID_HANDLE_VALUE;
    ctx->stdin_write = INVALID_HANDLE_VALUE;
    ctx->stdout_read = INVALID_HANDLE_VALUE;
    ctx->stdout_write = INVALID_HANDLE_VALUE;
    ctx->stderr_read = INVALID_HANDLE_VALUE;
    ctx->stderr_write = INVALID_HANDLE_VALUE;
    ctx->stdin_write_event = NULL;
    ctx->stdout_read_event = NULL;
    ctx->stderr_read_event = NULL;
}

static void jsl__subprocess_win_close_handle(HANDLE* h)
{
    if (*h != INVALID_HANDLE_VALUE && *h != NULL)
    {
        CloseHandle(*h);
        *h = INVALID_HANDLE_VALUE;
    }
}

static void jsl__subprocess_win_close_event(HANDLE* h)
{
    if (*h != NULL && *h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(*h);
        *h = NULL;
    }
}

static void jsl__subprocess_win_close_all_pipes(JSL__SubProcessWindowsLaunch* ctx)
{
    jsl__subprocess_win_close_handle(&ctx->stdin_read);
    jsl__subprocess_win_close_handle(&ctx->stdin_write);
    jsl__subprocess_win_close_handle(&ctx->stdout_read);
    jsl__subprocess_win_close_handle(&ctx->stdout_write);
    jsl__subprocess_win_close_handle(&ctx->stderr_read);
    jsl__subprocess_win_close_handle(&ctx->stderr_write);
    jsl__subprocess_win_close_event(&ctx->stdin_write_event);
    jsl__subprocess_win_close_event(&ctx->stdout_read_event);
    jsl__subprocess_win_close_event(&ctx->stderr_read_event);
}

// Create one named, overlapped pipe pair. The parent-end handle is
// returned non-inheritable and overlapped; the child-end handle is
// returned inheritable and synchronous.
//
// `parent_writes`: if true, parent writes / child reads (stdin pipe).
// If false, parent reads / child writes (stdout/stderr pipes).
// `dir_tag`: short string ("in"/"out"/"err") embedded in the pipe name.
//
// On success, `*out_parent`, `*out_child`, and `*out_event` hold fresh
// handles; on failure, any handle that was opened along the way is
// closed and `*out_errno` receives `GetLastError()`.
static bool jsl__subprocess_win_create_pipe(
    bool parent_writes,
    const char* dir_tag,
    HANDLE* out_parent,
    HANDLE* out_child,
    HANDLE* out_event,
    int32_t* out_errno
)
{
    static volatile LONG pipe_name_counter = 0;

    *out_parent = INVALID_HANDLE_VALUE;
    *out_child = INVALID_HANDLE_VALUE;
    *out_event = NULL;

    const DWORD pipe_buffer_bytes = 64 * 1024;
    DWORD pid = GetCurrentProcessId();

    HANDLE parent = INVALID_HANDLE_VALUE;
    char name_buf[128];

    // Retry a small bounded number of times in case someone else grabbed
    // the same name between our counter bump and CreateNamedPipeA.
    for (int attempt = 0; attempt < 3 && parent == INVALID_HANDLE_VALUE; attempt++)
    {
        LONG id = InterlockedIncrement(&pipe_name_counter);
        (void) snprintf(
            name_buf, sizeof(name_buf),
            "\\\\.\\pipe\\jsl-subprocess-%lu-%ld-%s",
            (unsigned long) pid, (long) id, dir_tag
        );

        DWORD open_mode =
            (parent_writes ? PIPE_ACCESS_OUTBOUND : PIPE_ACCESS_INBOUND)
            | FILE_FLAG_OVERLAPPED;
        DWORD pipe_mode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;

        HANDLE h = CreateNamedPipeA(
            name_buf,
            open_mode,
            pipe_mode,
            1, // nMaxInstances
            pipe_buffer_bytes, // nOutBufferSize
            pipe_buffer_bytes, // nInBufferSize
            0, // default timeout
            NULL // parent end is not inheritable
        );

        if (h == INVALID_HANDLE_VALUE)
        {
            DWORD err = GetLastError();
            if (err != ERROR_PIPE_BUSY && err != ERROR_ACCESS_DENIED)
            {
                if (out_errno != NULL)
                    *out_errno = (int32_t) err;
                return false;
            }
            continue;
        }

        parent = h;
    }

    if (parent == INVALID_HANDLE_VALUE)
    {
        if (out_errno != NULL)
            *out_errno = (int32_t) ERROR_PIPE_BUSY;
        return false;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = (DWORD) sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE child = CreateFileA(
        name_buf,
        parent_writes ? GENERIC_READ : GENERIC_WRITE,
        0,
        &sa,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (child == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        CloseHandle(parent);
        return false;
    }

    HANDLE event = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (event == NULL)
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        CloseHandle(parent);
        CloseHandle(child);
        return false;
    }

    *out_parent = parent;
    *out_child = child;
    *out_event = event;
    return true;
}

static void jsl__subprocess_win_close_child_pipe_ends(JSL__SubProcessWindowsLaunch* ctx)
{
    jsl__subprocess_win_close_handle(&ctx->stdin_read);
    jsl__subprocess_win_close_handle(&ctx->stdout_write);
    jsl__subprocess_win_close_handle(&ctx->stderr_write);
}

static void jsl__subprocess_win_free_launch(
    JSLSubprocess* proc,
    JSL__SubProcessWindowsLaunch* ctx
)
{
    if (ctx->cmdline != NULL)
    {
        (void) jsl_allocator_interface_free(proc->allocator, ctx->cmdline);
        ctx->cmdline = NULL;
    }
    if (ctx->env_block != NULL)
    {
        (void) jsl_allocator_interface_free(proc->allocator, ctx->env_block);
        ctx->env_block = NULL;
    }
    if (ctx->cwd_cstr != NULL)
    {
        (void) jsl_allocator_interface_free(proc->allocator, ctx->cwd_cstr);
        ctx->cwd_cstr = NULL;
    }
}

static bool jsl__subprocess_win_build_cmdline(
    JSLSubprocess* proc,
    JSL__SubProcessWindowsLaunch* ctx
)
{
    int64_t cmdline_len = 0;
    cmdline_len += proc->executable.length * 2 + 3;
    for (int64_t i = 0; i < proc->args_count; i++)
        cmdline_len += 1 + proc->args[i].length * 2 + 3;
    cmdline_len += 1;

    ctx->cmdline = (char*) jsl_allocator_interface_alloc(
        proc->allocator,
        cmdline_len,
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        false
    );
    if (ctx->cmdline == NULL)
        return false;

    char* cursor = jsl__subprocess_quote_arg_win(ctx->cmdline, proc->executable);
    for (int64_t i = 0; i < proc->args_count; i++)
    {
        *cursor = ' ';
        cursor++;
        cursor = jsl__subprocess_quote_arg_win(cursor, proc->args[i]);
    }
    *cursor = '\0';
    return true;
}

static bool jsl__subprocess_win_build_env_block(
    JSLSubprocess* proc,
    JSL__SubProcessWindowsLaunch* ctx
)
{
    if (proc->env_count == 0)
        return true;

    LPCH parent_env = GetEnvironmentStringsA();
    int64_t parent_len = 0;
    if (parent_env != NULL)
    {
        while (!(parent_env[parent_len] == '\0' && parent_env[parent_len + 1] == '\0'))
            parent_len++;
        parent_len++;
    }

    int64_t extra_len = 0;
    for (int64_t i = 0; i < proc->env_count; i++)
        extra_len += proc->env_vars[i].key.length + 1 + proc->env_vars[i].value.length + 1;

    ctx->env_block = (char*) jsl_allocator_interface_alloc(
        proc->allocator,
        parent_len + extra_len + 1,
        JSL_DEFAULT_ALLOCATION_ALIGNMENT,
        false
    );
    bool ok = (ctx->env_block != NULL);

    if (ok)
    {
        char* p = ctx->env_block;
        if (parent_env != NULL)
        {
            JSL_MEMCPY(p, parent_env, (size_t) parent_len);
            p += parent_len;
        }
        for (int64_t i = 0; i < proc->env_count; i++)
        {
            JSL_MEMCPY(p, proc->env_vars[i].key.data, (size_t) proc->env_vars[i].key.length);
            p += proc->env_vars[i].key.length;
            *p = '=';
            p++;
            JSL_MEMCPY(p, proc->env_vars[i].value.data, (size_t) proc->env_vars[i].value.length);
            p += proc->env_vars[i].value.length;
            *p = '\0';
            p++;
        }
        *p = '\0';
    }

    if (parent_env != NULL)
        FreeEnvironmentStringsA(parent_env);

    return ok;
}

static bool jsl__subprocess_win_build_cwd(
    JSLSubprocess* proc,
    JSL__SubProcessWindowsLaunch* ctx
)
{
    if (proc->working_directory.data == NULL || proc->working_directory.length == 0)
        return true;

    ctx->cwd_cstr = (char*) jsl_memory_to_cstr(proc->allocator, proc->working_directory);
    return ctx->cwd_cstr != NULL;
}

static bool jsl__subprocess_win_create_pipes(
    JSLSubprocess* proc,
    JSL__SubProcessWindowsLaunch* ctx,
    int32_t* out_errno
)
{
    bool ok = true;
    if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY)
    {
        ok = jsl__subprocess_win_create_pipe(
            true, "in",
            &ctx->stdin_write, &ctx->stdin_read, &ctx->stdin_write_event,
            out_errno
        );
    }
    if (ok && proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        ok = jsl__subprocess_win_create_pipe(
            false, "out",
            &ctx->stdout_read, &ctx->stdout_write, &ctx->stdout_read_event,
            out_errno
        );
    }
    if (ok && proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        ok = jsl__subprocess_win_create_pipe(
            false, "err",
            &ctx->stderr_read, &ctx->stderr_write, &ctx->stderr_read_event,
            out_errno
        );
    }
    return ok;
}

static JSLSubProcessResultEnum jsl__subprocess_win_prepare(
    JSLSubprocess* proc,
    JSL__SubProcessWindowsLaunch* ctx,
    int32_t* out_errno
)
{
    jsl__subprocess_win_launch_zero(ctx);

    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;

    if (!jsl__subprocess_win_build_cmdline(proc, ctx))
        result = JSL_SUBPROCESS_ALLOCATION_FAILED;

    if (result == JSL_SUBPROCESS_SUCCESS
        && !jsl__subprocess_win_build_env_block(proc, ctx))
        result = JSL_SUBPROCESS_ALLOCATION_FAILED;

    if (result == JSL_SUBPROCESS_SUCCESS
        && !jsl__subprocess_win_build_cwd(proc, ctx))
        result = JSL_SUBPROCESS_ALLOCATION_FAILED;

    if (result == JSL_SUBPROCESS_SUCCESS
        && !jsl__subprocess_win_create_pipes(proc, ctx, out_errno))
    {
        jsl__subprocess_win_close_all_pipes(ctx);
        result = JSL_SUBPROCESS_PIPE_FAILED;
    }

    return result;
}

// Invoke CreateProcessA using the prepared context. On success, the child
// is running and *out_pi receives the process/thread handles. The caller
// must close the child-side pipe ends via `_close_child_pipe_ends` and is
// responsible for closing pi.hThread when it is no longer needed. On
// failure, every pipe handle is closed.
static JSLSubProcessResultEnum jsl__subprocess_win_spawn(
    JSLSubprocess* proc,
    JSL__SubProcessWindowsLaunch* ctx,
    PROCESS_INFORMATION* out_pi,
    int32_t* out_errno
)
{
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = (DWORD) sizeof(si);

    bool use_stdhandles =
        proc->stdin_kind != JSL_SUBPROCESS_STDIN_INHERIT
        || proc->stdout_kind != JSL_SUBPROCESS_OUTPUT_INHERIT
        || proc->stderr_kind != JSL_SUBPROCESS_OUTPUT_INHERIT;

    HANDLE stdin_null = INVALID_HANDLE_VALUE;
    HANDLE stdout_null = INVALID_HANDLE_VALUE;
    HANDLE stderr_null = INVALID_HANDLE_VALUE;
    bool null_open_failed = false;
    DWORD null_open_err = 0;

    if (use_stdhandles)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;

        SECURITY_ATTRIBUTES null_sa;
        null_sa.nLength = (DWORD) sizeof(null_sa);
        null_sa.lpSecurityDescriptor = NULL;
        null_sa.bInheritHandle = TRUE;

        if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY)
            si.hStdInput = ctx->stdin_read;
        else if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_FD)
            si.hStdInput = (HANDLE) _get_osfhandle(proc->stdin_fd);
        else if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_NULL)
        {
            stdin_null = CreateFileA(
                "NUL",
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &null_sa,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            if (stdin_null == INVALID_HANDLE_VALUE)
            {
                null_open_failed = true;
                null_open_err = GetLastError();
            }
            si.hStdInput = stdin_null;
        }
        else
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        if (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
            si.hStdOutput = ctx->stdout_write;
        else if (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_FD)
            si.hStdOutput = (HANDLE) _get_osfhandle(proc->stdout_fd);
        else if (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_NULL)
        {
            stdout_null = CreateFileA(
                "NUL",
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &null_sa,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            if (stdout_null == INVALID_HANDLE_VALUE)
            {
                null_open_failed = true;
                null_open_err = GetLastError();
            }
            si.hStdOutput = stdout_null;
        }
        else
            si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

        if (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
            si.hStdError = ctx->stderr_write;
        else if (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_FD)
            si.hStdError = (HANDLE) _get_osfhandle(proc->stderr_fd);
        else if (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_NULL)
        {
            stderr_null = CreateFileA(
                "NUL",
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &null_sa,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            if (stderr_null == INVALID_HANDLE_VALUE)
            {
                null_open_failed = true;
                null_open_err = GetLastError();
            }
            si.hStdError = stderr_null;
        }
        else
            si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }

    ZeroMemory(out_pi, sizeof(*out_pi));

    BOOL created = FALSE;
    if (!null_open_failed)
    {
        created = CreateProcessA(
            NULL,
            ctx->cmdline,
            NULL,
            NULL,
            use_stdhandles ? TRUE : FALSE,
            0,
            ctx->env_block,
            ctx->cwd_cstr,
            &si,
            out_pi
        );
    }

    DWORD create_err = created ? 0 : GetLastError();

    jsl__subprocess_win_close_handle(&stdin_null);
    jsl__subprocess_win_close_handle(&stdout_null);
    jsl__subprocess_win_close_handle(&stderr_null);

    if (null_open_failed)
    {
        if (out_errno != NULL)
            *out_errno = (int32_t) null_open_err;
        jsl__subprocess_win_close_all_pipes(ctx);
        return JSL_SUBPROCESS_SPAWN_FAILED;
    }

    if (!created)
    {
        if (out_errno != NULL)
            *out_errno = (int32_t) create_err;
        jsl__subprocess_win_close_all_pipes(ctx);
        return JSL_SUBPROCESS_SPAWN_FAILED;
    }

    return JSL_SUBPROCESS_SUCCESS;
}

// Overlapped stdin pump. Drives the write side of the child's stdin
// pipe through a two-state machine: no op pending → kick one, op
// pending → check completion. Returns SUCCESS in the normal case and
// also when an op is still in flight. Returns IO_FAILED with errno on
// real write failures. Treats broken-pipe as "child closed stdin" and
// reports it as a normal end-of-stdin (no error).
static JSLSubProcessResultEnum jsl__subprocess_win_pump_stdin_once(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    if (proc->stdin_kind != JSL_SUBPROCESS_STDIN_MEMORY
        || proc->stdin_write_handle == INVALID_HANDLE_VALUE)
        return JSL_SUBPROCESS_SUCCESS;

    // Loop so that sync completions roll straight into the next
    // WriteFile; each iteration either returns (pending / done / EOF /
    // error) or sync-completes and tries again.
    for (;;)
    {

    // State B: an op is already in flight; see if it completed.
    if (proc->stdin_write_pending)
    {
        DWORD bytes = 0;
        BOOL ok = GetOverlappedResult(
            proc->stdin_write_handle,
            &proc->stdin_write_overlapped,
            &bytes,
            FALSE
        );
        if (!ok)
        {
            DWORD err = GetLastError();
            if (err == ERROR_IO_INCOMPLETE)
                return JSL_SUBPROCESS_SUCCESS;

            proc->stdin_write_pending = false;
            if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA)
            {
                // Child closed its stdin: end of writes.
                jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
                return JSL_SUBPROCESS_SUCCESS;
            }
            if (out_errno != NULL)
                *out_errno = (int32_t) err;
            jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
            return JSL_SUBPROCESS_IO_FAILED;
        }

        proc->stdin_write_offset += (int64_t) bytes;
        proc->stdin_write_pending = false;
        // Fall through to state A to kick the next chunk if any.
    }

    // State A: nothing pending. Either close (done) or start a new write.
    int64_t remaining = proc->stdin_memory.length - proc->stdin_write_offset;
    if (remaining <= 0)
    {
        jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
        return JSL_SUBPROCESS_SUCCESS;
    }

    DWORD chunk =
        (DWORD)(remaining > (int64_t) sizeof(proc->stdin_write_buffer)
            ? (int64_t) sizeof(proc->stdin_write_buffer)
            : remaining);

    JSL_MEMCPY(
        proc->stdin_write_buffer,
        proc->stdin_memory.data + proc->stdin_write_offset,
        chunk
    );
    proc->stdin_write_buffer_len = chunk;

    ZeroMemory(&proc->stdin_write_overlapped, sizeof(proc->stdin_write_overlapped));
    proc->stdin_write_overlapped.hEvent = proc->stdin_write_event;

    BOOL wok = WriteFile(
        proc->stdin_write_handle,
        proc->stdin_write_buffer,
        chunk,
        NULL,
        &proc->stdin_write_overlapped
    );

    if (wok)
    {
        // Completed synchronously. Advance and continue: if more data
        // remains, kick the next write this same call so we always
        // leave an op pending on the handle (or close it).
        DWORD bytes = 0;
        (void) GetOverlappedResult(
            proc->stdin_write_handle,
            &proc->stdin_write_overlapped,
            &bytes,
            FALSE
        );
        proc->stdin_write_offset += (int64_t) bytes;
        proc->stdin_write_pending = false;
        continue;
    }

    DWORD err = GetLastError();
    if (err == ERROR_IO_PENDING)
    {
        proc->stdin_write_pending = true;
        return JSL_SUBPROCESS_SUCCESS;
    }
    if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA)
    {
        jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
        return JSL_SUBPROCESS_SUCCESS;
    }

    if (out_errno != NULL)
        *out_errno = (int32_t) err;
    jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
    return JSL_SUBPROCESS_IO_FAILED;
    } // for(;;)
}

// One pump of an output stream (stdout or stderr) using overlapped I/O.
// Drains any completed read into the sink and always leaves an op
// pending on the handle when the handle is still open, so the event
// loop has something to wait on. Returns IO_FAILED on real read
// failures; EOF (ERROR_BROKEN_PIPE / ERROR_HANDLE_EOF) is a normal
// termination.
static JSLSubProcessResultEnum jsl__subprocess_win_pump_output_stream(
    HANDLE* h_ptr,
    OVERLAPPED* ov,
    HANDLE ev,
    bool* pending,
    uint8_t* buffer,
    DWORD buffer_size,
    JSLOutputSink sink,
    bool* eof_seen,
    int32_t* out_errno
)
{
    if (*h_ptr == INVALID_HANDLE_VALUE || *eof_seen)
        return JSL_SUBPROCESS_SUCCESS;

    // Loop: drain completions, then kick the next read. Bounded by
    // each iteration either returning immediately (pending/EOF/error)
    // or completing synchronously and trying again.
    for (;;)
    {
        if (*pending)
        {
            DWORD bytes = 0;
            BOOL ok = GetOverlappedResult(*h_ptr, ov, &bytes, FALSE);
            if (!ok)
            {
                DWORD err = GetLastError();
                if (err == ERROR_IO_INCOMPLETE)
                    return JSL_SUBPROCESS_SUCCESS;

                *pending = false;
                if (err == ERROR_BROKEN_PIPE || err == ERROR_HANDLE_EOF)
                {
                    jsl__subprocess_win_close_handle(h_ptr);
                    *eof_seen = true;
                    return JSL_SUBPROCESS_SUCCESS;
                }
                if (out_errno != NULL)
                    *out_errno = (int32_t) err;
                jsl__subprocess_win_close_handle(h_ptr);
                *eof_seen = true;
                return JSL_SUBPROCESS_IO_FAILED;
            }

            *pending = false;
            if (bytes == 0)
            {
                // Zero-byte completion on a byte-mode pipe means EOF.
                jsl__subprocess_win_close_handle(h_ptr);
                *eof_seen = true;
                return JSL_SUBPROCESS_SUCCESS;
            }
            jsl_output_sink_write(sink, jsl_immutable_memory(buffer, (int64_t) bytes));
            // Fall through to kick another read.
        }

        ZeroMemory(ov, sizeof(*ov));
        ov->hEvent = ev;

        BOOL rok = ReadFile(*h_ptr, buffer, buffer_size, NULL, ov);
        if (rok)
        {
            // Completed synchronously. Mark pending so the completion
            // branch above consumes it on the next iteration.
            *pending = true;
            continue;
        }

        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING)
        {
            *pending = true;
            return JSL_SUBPROCESS_SUCCESS;
        }
        if (err == ERROR_BROKEN_PIPE || err == ERROR_HANDLE_EOF)
        {
            jsl__subprocess_win_close_handle(h_ptr);
            *eof_seen = true;
            return JSL_SUBPROCESS_SUCCESS;
        }

        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        jsl__subprocess_win_close_handle(h_ptr);
        *eof_seen = true;
        return JSL_SUBPROCESS_IO_FAILED;
    }
}

static JSLSubProcessResultEnum jsl__subprocess_win_pump_output_once(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;

    if (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        JSLSubProcessResultEnum r = jsl__subprocess_win_pump_output_stream(
            &proc->stdout_read_handle,
            &proc->stdout_read_overlapped,
            proc->stdout_read_event,
            &proc->stdout_read_pending,
            proc->stdout_read_buffer,
            (DWORD) sizeof(proc->stdout_read_buffer),
            proc->stdout_sink,
            &proc->stdout_eof_seen,
            out_errno
        );
        if (r != JSL_SUBPROCESS_SUCCESS)
            result = r;
    }

    if (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        JSLSubProcessResultEnum r = jsl__subprocess_win_pump_output_stream(
            &proc->stderr_read_handle,
            &proc->stderr_read_overlapped,
            proc->stderr_read_event,
            &proc->stderr_read_pending,
            proc->stderr_read_buffer,
            (DWORD) sizeof(proc->stderr_read_buffer),
            proc->stderr_sink,
            &proc->stderr_eof_seen,
            result == JSL_SUBPROCESS_SUCCESS ? out_errno : NULL
        );
        if (r != JSL_SUBPROCESS_SUCCESS && result == JSL_SUBPROCESS_SUCCESS)
            result = r;
    }

    return result;
}

// Final drain after the child has exited. Called only from the
// process-exit branch of `jsl_subprocess_background_wait` (the
// `WIN_WAIT_KIND_PROCESS` arm of the WaitForMultipleObjectsEx loop).
//
// The bug being defended against: process termination and pending
// overlapped pipe I/O are independent kernel events. Closing the
// child's write end of the pipe (which happens during process
// termination) eventually completes the parent's pending ReadFile with
// either the buffered bytes or ERROR_BROKEN_PIPE — but Windows does
// not guarantee that completion is delivered to user mode before the
// process handle becomes signalled. So when WaitForMultipleObjectsEx
// returns the process handle, a still-in-flight `ReadFile` may answer
// `GetOverlappedResult(..., FALSE)` with ERROR_IO_INCOMPLETE for a
// short window. The plain pump treats that as "nothing to do, come
// back later" — but the multi-proc event loop will not come back: it
// drops `still_waiting` and proceeds to cleanup, where `CancelIoEx`
// then discards the bytes the child already wrote.
//
// Fix: for each pending op, call `GetOverlappedResult(..., TRUE)` to
// block until the IRP is finalized. The write end is gone, so this
// resolves promptly with either bytes or ERROR_BROKEN_PIPE (no
// further data can ever appear). Then loop `pump_output_once`, which
// will now see a real completion via its own
// `GetOverlappedResult(..., FALSE)` call (`GetOverlappedResult` is
// idempotent on a completed op), consume the bytes into the sink, and
// kick the next read — which immediately reports EOF.
//
// The `safety` counter is paranoia against an unforeseen "always
// pending" loop; under normal operation termination is reached via the
// EOF path inside one or two iterations.
//
// Note: not exercised by a regression test because reproducing the
// race deterministically would need OS-level fault injection. The
// existing `heavy_stdout` and `spew_large` tests cover the common
// (non-racy) drain path. If touching this function, re-read the
// non-blocking pump in `jsl__subprocess_win_pump_output_stream` first
// — both must stay in agreement about which OVERLAPPED states are
// transient vs terminal.
static JSLSubProcessResultEnum jsl__subprocess_win_finalize_pending_reads(
    JSLSubprocess* p,
    int32_t* out_errno
)
{
    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;
    int32_t safety = 0;
    for (;;)
    {
        bool anything_open =
            (p->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK
                && !p->stdout_eof_seen
                && p->stdout_read_handle != INVALID_HANDLE_VALUE)
            || (p->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK
                && !p->stderr_eof_seen
                && p->stderr_read_handle != INVALID_HANDLE_VALUE);
        if (!anything_open)
            break;
        if (++safety > 1024)
            break;

        int32_t e = 0;
        JSLSubProcessResultEnum pr = jsl__subprocess_win_pump_output_once(p, &e);
        if (pr == JSL_SUBPROCESS_IO_FAILED)
        {
            if (out_errno != NULL && *out_errno == 0)
                *out_errno = e;
            result = JSL_SUBPROCESS_IO_FAILED;
            break;
        }
        if (e != 0 && out_errno != NULL && *out_errno == 0)
            *out_errno = e;

        // Block on whatever the pump left pending. After the wait
        // returns, the OVERLAPPED carries a real result and the next
        // pump pass will consume it (no ERROR_IO_INCOMPLETE).
        bool any_pending = false;
        if (p->stdout_read_pending
            && p->stdout_read_handle != INVALID_HANDLE_VALUE)
        {
            DWORD bytes = 0;
            (void) GetOverlappedResult(
                p->stdout_read_handle,
                &p->stdout_read_overlapped,
                &bytes,
                TRUE
            );
            any_pending = true;
        }
        if (p->stderr_read_pending
            && p->stderr_read_handle != INVALID_HANDLE_VALUE)
        {
            DWORD bytes = 0;
            (void) GetOverlappedResult(
                p->stderr_read_handle,
                &p->stderr_read_overlapped,
                &bytes,
                TRUE
            );
            any_pending = true;
        }
        if (!any_pending)
            break;
    }
    return result;
}

static JSLSubProcessResultEnum jsl__subprocess_win_poll(
    JSLSubprocess* proc,
    int32_t timeout_ms,
    JSLSubProcessStatusEnum* out_status,
    int32_t* out_exit_code,
    int32_t* out_errno
)
{
    if (proc->status == JSL_SUBPROCESS_STATUS_EXITED
        || proc->status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL)
    {
        *out_status = proc->status;
        return JSL_SUBPROCESS_SUCCESS;
    }

    DWORD wait_ms = (timeout_ms < 0) ? INFINITE : (DWORD) timeout_ms;
    DWORD wait_res = WaitForSingleObject(proc->process_handle, wait_ms);

    if (wait_res == WAIT_TIMEOUT)
    {
        *out_status = proc->status;
        return JSL_SUBPROCESS_SUCCESS;
    }

    if (wait_res == WAIT_FAILED)
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        *out_status = proc->status;
        return JSL_SUBPROCESS_WAIT_FAILED;
    }

    DWORD exit_code = 0;
    if (!GetExitCodeProcess(proc->process_handle, &exit_code))
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        *out_status = proc->status;
        return JSL_SUBPROCESS_WAIT_FAILED;
    }

    *out_exit_code = (int32_t) exit_code;
    proc->status = JSL_SUBPROCESS_STATUS_EXITED;
    *out_status = proc->status;
    return JSL_SUBPROCESS_SUCCESS;
}

#endif

JSLSubProcessResultEnum jsl_subprocess_run_blocking(
    JSLSubprocess* proc,
    int32_t timeout_ms,
    int32_t* out_errno
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);

    if (!proceed)
        return JSL_SUBPROCESS_BAD_PARAMETERS;

    if (proc->status != JSL_SUBPROCESS_STATUS_NOT_STARTED)
        return JSL_SUBPROCESS_ALREADY_STARTED;

    proc->exit_code = -1;
    if (out_errno != NULL)
        *out_errno = 0;

    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;
    bool infinite_timeout = (timeout_ms < 0);

#if JSL_IS_POSIX

    JSL__SubProcessPosixLaunch ctx;
    JSLSubProcessResultEnum prep = jsl__subprocess_posix_prepare(proc, &ctx, out_errno);
    if (prep != JSL_SUBPROCESS_SUCCESS)
    {
        jsl__subprocess_posix_free_launch(proc, &ctx);
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return prep;
    }

    pid_t pid = 0;
    JSLSubProcessResultEnum spawn_result =
        jsl__subprocess_posix_spawn(proc, &ctx, &pid, out_errno);

    // argv/envp/cwd strings are no longer needed after posix_spawnp
    // returns, regardless of success or failure.
    jsl__subprocess_posix_free_launch(proc, &ctx);

    if (spawn_result != JSL_SUBPROCESS_SUCCESS)
    {
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return spawn_result;
    }

    proc->status = JSL_SUBPROCESS_STATUS_RUNNING;

    // Parent keeps only its ends of any pipes that were created.
    jsl__subprocess_posix_close_child_pipe_ends(&ctx);

    int64_t deadline_ms = infinite_timeout
        ? -1
        : jsl__monotonic_ms() + (int64_t) timeout_ms;

    int32_t io_errno = 0;
    bool pipes_timed_out = false;
    bool io_ok = jsl__subprocess_posix_pump_io(
        proc,
        &ctx,
        deadline_ms,
        &pipes_timed_out,
        &io_errno
    );

    bool timeout_reached = false;

    if (pipes_timed_out)
    {
        // Pipes still had work but the deadline expired: kill, drain
        // briefly, then reap.
        timeout_reached = true;
        if (pid > 0)
            (void) kill(pid, SIGKILL);
        // Best-effort final drain so any output already in the kernel
        // pipe buffers reaches the caller's sinks. SIGKILL closes the
        // child's fds so this terminates quickly.
        bool dummy = false;
        (void) jsl__subprocess_posix_pump_io(proc, &ctx, -1, &dummy, NULL);
    }
    else if (!infinite_timeout)
    {
        // Pipes closed but the child may still be alive (e.g. closed its
        // own stdout/stderr without exiting). Bound the reap by the
        // remaining time.
        int64_t remaining = deadline_ms - jsl__monotonic_ms();
        int32_t bounded = remaining <= 0
            ? 0
            : (remaining > (int64_t) INT32_MAX
                ? INT32_MAX
                : (int32_t) remaining);

        JSLSubProcessStatusEnum tmp_status = proc->status;
        int32_t tmp_exit = -1;
        JSLSubProcessResultEnum poll_r = jsl__subprocess_posix_poll(
            proc,
            bounded,
            &tmp_status,
            &tmp_exit,
            out_errno
        );
        if (poll_r != JSL_SUBPROCESS_SUCCESS
            && poll_r != JSL_SUBPROCESS_KILLED_BY_SIGNAL)
        {
            return poll_r;
        }
        if (tmp_status == JSL_SUBPROCESS_STATUS_RUNNING)
        {
            timeout_reached = true;
            (void) kill(pid, SIGKILL);
        }
        else
        {
            proc->exit_code = tmp_exit;
            if (tmp_status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL)
                result = JSL_SUBPROCESS_KILLED_BY_SIGNAL;
        }
    }

    if (timeout_reached || infinite_timeout
        || (proc->status == JSL_SUBPROCESS_STATUS_RUNNING))
    {
        // Either no deadline applied, or we just sent SIGKILL and need
        // to harvest the exit status.
        JSLSubProcessResultEnum wait_result = jsl__subprocess_posix_wait(
            pid,
            &proc->status,
            &proc->exit_code,
            out_errno
        );
        if (wait_result != JSL_SUBPROCESS_SUCCESS
            && wait_result != JSL_SUBPROCESS_KILLED_BY_SIGNAL)
            result = wait_result;
        else if (wait_result == JSL_SUBPROCESS_KILLED_BY_SIGNAL
            && !timeout_reached)
            result = JSL_SUBPROCESS_KILLED_BY_SIGNAL;
    }

    if (timeout_reached)
        result = JSL_SUBPROCESS_TIMEOUT_REACHED;
    else if (result == JSL_SUBPROCESS_SUCCESS && !io_ok)
    {
        if (out_errno != NULL)
            *out_errno = io_errno;
        result = JSL_SUBPROCESS_IO_FAILED;
    }

    return result;

#elif JSL_IS_WINDOWS

    JSL__SubProcessWindowsLaunch ctx;
    JSLSubProcessResultEnum prep = jsl__subprocess_win_prepare(proc, &ctx, out_errno);
    if (prep != JSL_SUBPROCESS_SUCCESS)
    {
        jsl__subprocess_win_free_launch(proc, &ctx);
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return prep;
    }

    PROCESS_INFORMATION pi;
    JSLSubProcessResultEnum spawn_result =
        jsl__subprocess_win_spawn(proc, &ctx, &pi, out_errno);

    jsl__subprocess_win_free_launch(proc, &ctx);

    if (spawn_result != JSL_SUBPROCESS_SUCCESS)
    {
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return spawn_result;
    }

    proc->status = JSL_SUBPROCESS_STATUS_RUNNING;

    jsl__subprocess_win_close_child_pipe_ends(&ctx);

    // Hand the parent pipe ends + overlapped events off to `proc` so the
    // shared overlapped pump helpers can drive them. `run_blocking`
    // cleans them up itself below — it does not flip `is_background`.
    proc->stdin_write_handle = ctx.stdin_write;
    proc->stdout_read_handle = ctx.stdout_read;
    proc->stderr_read_handle = ctx.stderr_read;
    proc->stdin_write_event = ctx.stdin_write_event;
    proc->stdout_read_event = ctx.stdout_read_event;
    proc->stderr_read_event = ctx.stderr_read_event;
    proc->stdin_write_offset = 0;
    proc->stdin_write_pending = false;
    proc->stdout_read_pending = false;
    proc->stderr_read_pending = false;
    proc->stdout_eof_seen = false;
    proc->stderr_eof_seen = false;

    int32_t io_errno = 0;
    bool io_error = false;
    bool process_exited = false;
    bool timeout_reached = false;

    int64_t deadline_ms = infinite_timeout
        ? 0
        : jsl__monotonic_ms() + (int64_t) timeout_ms;

    for (;;)
    {
        // Kick-or-progress each pipe. Successful calls leave a pending
        // op on each still-open stream, whose event we can wait on.
        int32_t e = 0;
        (void) jsl__subprocess_win_pump_stdin_once(proc, &e);
        if (e != 0 && !io_error) { io_error = true; io_errno = e; }

        e = 0;
        JSLSubProcessResultEnum pr = jsl__subprocess_win_pump_output_once(proc, &e);
        if (pr == JSL_SUBPROCESS_IO_FAILED && !io_error)
        {
            io_error = true;
            io_errno = e;
        }

        // Build the wait set: process handle first, then any pending
        // pipe events. Exiting the loop via "all pipes closed" is fine;
        // the final WaitForSingleObject below harvests the exit code.
        HANDLE handles[4];
        DWORD n = 0;
        handles[n++] = pi.hProcess;

        if (proc->stdin_write_handle != INVALID_HANDLE_VALUE
            && proc->stdin_write_pending)
            handles[n++] = proc->stdin_write_event;

        if (proc->stdout_read_handle != INVALID_HANDLE_VALUE
            && proc->stdout_read_pending)
            handles[n++] = proc->stdout_read_event;

        if (proc->stderr_read_handle != INVALID_HANDLE_VALUE
            && proc->stderr_read_pending)
            handles[n++] = proc->stderr_read_event;

        DWORD wait_ms;
        if (infinite_timeout)
            wait_ms = INFINITE;
        else
        {
            int64_t remaining = deadline_ms - jsl__monotonic_ms();
            if (remaining <= 0)
                wait_ms = 0;
            else if (remaining > (int64_t) (MAXDWORD - 1))
                wait_ms = MAXDWORD - 1;
            else
                wait_ms = (DWORD) remaining;
        }

        DWORD wret = WaitForMultipleObjects(n, handles, FALSE, wait_ms);

        if (wret == WAIT_FAILED)
        {
            DWORD err = GetLastError();
            if (!io_error) { io_error = true; io_errno = (int32_t) err; }
            break;
        }

        if (wret == WAIT_TIMEOUT)
        {
            timeout_reached = true;
            (void) TerminateProcess(pi.hProcess, 1);
            // Wait for the kill to land so the next pump pass can drain
            // any final output and the exit code is harvestable.
            (void) WaitForSingleObject(pi.hProcess, INFINITE);
            process_exited = true;
            break;
        }

        if (wret == WAIT_OBJECT_0)
        {
            // Process exited. Fall through to final drain.
            process_exited = true;
            break;
        }

        // Some pipe event fired. Next loop iteration's pump calls
        // consume the completion.
    }

    // Final drain: after the process is gone, whatever remained buffered
    // in the pipes is still readable until EOF. Loop until both output
    // streams report EOF and stdin is closed.
    if (process_exited)
    {
        for (;;)
        {
            bool anything_open =
                proc->stdin_write_handle != INVALID_HANDLE_VALUE
                || (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK
                    && !proc->stdout_eof_seen
                    && proc->stdout_read_handle != INVALID_HANDLE_VALUE)
                || (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK
                    && !proc->stderr_eof_seen
                    && proc->stderr_read_handle != INVALID_HANDLE_VALUE);
            if (!anything_open)
                break;

            int32_t e = 0;
            (void) jsl__subprocess_win_pump_stdin_once(proc, &e);
            if (e != 0 && !io_error) { io_error = true; io_errno = e; }

            e = 0;
            JSLSubProcessResultEnum pr = jsl__subprocess_win_pump_output_once(proc, &e);
            if (pr == JSL_SUBPROCESS_IO_FAILED && !io_error)
            {
                io_error = true;
                io_errno = e;
            }

            HANDLE handles[3];
            DWORD n = 0;
            if (proc->stdin_write_handle != INVALID_HANDLE_VALUE
                && proc->stdin_write_pending)
                handles[n++] = proc->stdin_write_event;
            if (proc->stdout_read_handle != INVALID_HANDLE_VALUE
                && proc->stdout_read_pending)
                handles[n++] = proc->stdout_read_event;
            if (proc->stderr_read_handle != INVALID_HANDLE_VALUE
                && proc->stderr_read_pending)
                handles[n++] = proc->stderr_read_event;
            if (n == 0)
                break;

            (void) WaitForMultipleObjects(n, handles, FALSE, 5000);
        }
    }

    // Harvest exit code. The process handle is the only thing we still
    // need to block on; the pumping loop may have exited because all
    // pipes closed while the process was still alive.
    DWORD wait_res = WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    JSLSubProcessResultEnum wait_status = JSL_SUBPROCESS_SUCCESS;
    if (wait_res == WAIT_FAILED)
    {
        if (out_errno != NULL)
            *out_errno = (int32_t) GetLastError();
        wait_status = JSL_SUBPROCESS_WAIT_FAILED;
    }
    else if (!GetExitCodeProcess(pi.hProcess, &exit_code))
    {
        if (out_errno != NULL)
            *out_errno = (int32_t) GetLastError();
        wait_status = JSL_SUBPROCESS_WAIT_FAILED;
    }

    // Tear down handles/events owned by run_blocking.
    if (proc->stdin_write_pending
        && proc->stdin_write_handle != INVALID_HANDLE_VALUE)
        CancelIoEx(proc->stdin_write_handle, &proc->stdin_write_overlapped);
    if (proc->stdout_read_pending
        && proc->stdout_read_handle != INVALID_HANDLE_VALUE)
        CancelIoEx(proc->stdout_read_handle, &proc->stdout_read_overlapped);
    if (proc->stderr_read_pending
        && proc->stderr_read_handle != INVALID_HANDLE_VALUE)
        CancelIoEx(proc->stderr_read_handle, &proc->stderr_read_overlapped);

    jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
    jsl__subprocess_win_close_handle(&proc->stdout_read_handle);
    jsl__subprocess_win_close_handle(&proc->stderr_read_handle);
    jsl__subprocess_win_close_event(&proc->stdin_write_event);
    jsl__subprocess_win_close_event(&proc->stdout_read_event);
    jsl__subprocess_win_close_event(&proc->stderr_read_event);
    proc->stdin_write_pending = false;
    proc->stdout_read_pending = false;
    proc->stderr_read_pending = false;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (wait_status != JSL_SUBPROCESS_SUCCESS)
    {
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return wait_status;
    }

    proc->exit_code = (int32_t) exit_code;
    if (timeout_reached)
        proc->status = JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL;
    else
        proc->status = JSL_SUBPROCESS_STATUS_EXITED;

    if (timeout_reached)
        result = JSL_SUBPROCESS_TIMEOUT_REACHED;
    else if (io_error)
    {
        if (out_errno != NULL)
            *out_errno = io_errno;
        result = JSL_SUBPROCESS_IO_FAILED;
    }

    return result;

#else
    #error "Unsupported platform"
#endif
}


JSLSubProcessResultEnum jsl_subprocess_background_start(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);

    if (!proceed)
        return JSL_SUBPROCESS_BAD_PARAMETERS;

    if (proc->status != JSL_SUBPROCESS_STATUS_NOT_STARTED)
        return JSL_SUBPROCESS_ALREADY_STARTED;

    if (out_errno != NULL)
        *out_errno = 0;

#if JSL_IS_POSIX

    JSL__SubProcessPosixLaunch ctx;
    JSLSubProcessResultEnum prep = jsl__subprocess_posix_prepare(proc, &ctx, out_errno);
    if (prep != JSL_SUBPROCESS_SUCCESS)
    {
        jsl__subprocess_posix_free_launch(proc, &ctx);
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return prep;
    }

    pid_t pid = 0;
    JSLSubProcessResultEnum spawn_result =
        jsl__subprocess_posix_spawn(proc, &ctx, &pid, out_errno);

    jsl__subprocess_posix_free_launch(proc, &ctx);

    if (spawn_result != JSL_SUBPROCESS_SUCCESS)
    {
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return spawn_result;
    }

    // Transfer parent-side pipe fds onto the subprocess struct, then
    // close the child-side ends.
    jsl__subprocess_posix_close_child_pipe_ends(&ctx);

    proc->pid = (int32_t) pid;
    proc->stdin_write_fd = ctx.stdin_pipe[1];
    proc->stdout_read_fd = ctx.stdout_pipe[0];
    proc->stderr_read_fd = ctx.stderr_pipe[0];

    // Non-blocking on all parent-side pipe ends so `pump_*` never blocks.
    jsl__subprocess_posix_set_nonblocking(proc->stdin_write_fd);
    jsl__subprocess_posix_set_nonblocking(proc->stdout_read_fd);
    jsl__subprocess_posix_set_nonblocking(proc->stderr_read_fd);

    proc->status = JSL_SUBPROCESS_STATUS_RUNNING;
    proc->is_background = true;
    return JSL_SUBPROCESS_SUCCESS;

#elif JSL_IS_WINDOWS

    JSL__SubProcessWindowsLaunch ctx;
    JSLSubProcessResultEnum prep = jsl__subprocess_win_prepare(proc, &ctx, out_errno);
    if (prep != JSL_SUBPROCESS_SUCCESS)
    {
        jsl__subprocess_win_free_launch(proc, &ctx);
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return prep;
    }

    PROCESS_INFORMATION pi;
    JSLSubProcessResultEnum spawn_result =
        jsl__subprocess_win_spawn(proc, &ctx, &pi, out_errno);

    jsl__subprocess_win_free_launch(proc, &ctx);

    if (spawn_result != JSL_SUBPROCESS_SUCCESS)
    {
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return spawn_result;
    }

    jsl__subprocess_win_close_child_pipe_ends(&ctx);

    proc->process_handle = pi.hProcess;
    CloseHandle(pi.hThread);

    proc->stdin_write_handle = ctx.stdin_write;
    proc->stdout_read_handle = ctx.stdout_read;
    proc->stderr_read_handle = ctx.stderr_read;
    proc->stdin_write_event = ctx.stdin_write_event;
    proc->stdout_read_event = ctx.stdout_read_event;
    proc->stderr_read_event = ctx.stderr_read_event;

    proc->status = JSL_SUBPROCESS_STATUS_RUNNING;
    proc->is_background = true;
    return JSL_SUBPROCESS_SUCCESS;

#else
    #error "Unsupported platform"
#endif
}

JSLSubProcessResultEnum jsl_subprocess_background_poll(
    JSLSubprocess* proc,
    int32_t timeout_ms,
    JSLSubProcessStatusEnum* out_status,
    int32_t* out_exit_code,
    int32_t* out_errno
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && out_status != NULL
        && out_exit_code != NULL
        && proc->is_background);

    if (!proceed)
        return JSL_SUBPROCESS_BAD_PARAMETERS;

    if (out_errno != NULL)
        *out_errno = 0;

    int32_t local_errno = 0;
    JSLSubProcessResultEnum r;
#if JSL_IS_POSIX
    r = jsl__subprocess_posix_poll(proc, timeout_ms, out_status, &proc->exit_code, &local_errno);
#elif JSL_IS_WINDOWS
    r = jsl__subprocess_win_poll(proc, timeout_ms, out_status, &proc->exit_code, &local_errno);
#else
    #error "Unsupported platform"
#endif

    *out_exit_code = proc->exit_code;
    if (local_errno != 0)
    {
        proc->last_errno = local_errno;
        if (out_errno != NULL)
            *out_errno = local_errno;
    }
    return r;
}

JSLSubProcessResultEnum jsl_subprocess_background_send_stdin(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && proc->is_background);

    if (!proceed)
        return JSL_SUBPROCESS_BAD_PARAMETERS;

    if (out_errno != NULL)
        *out_errno = 0;

    int32_t local_errno = 0;
    JSLSubProcessResultEnum r;
#if JSL_IS_POSIX
    r = jsl__subprocess_posix_pump_stdin_once(proc, &local_errno);
#elif JSL_IS_WINDOWS
    r = jsl__subprocess_win_pump_stdin_once(proc, &local_errno);
#else
    #error "Unsupported platform"
#endif

    if (local_errno != 0)
    {
        proc->last_errno = local_errno;
        if (out_errno != NULL)
            *out_errno = local_errno;
    }
    return r;
}

JSLSubProcessResultEnum jsl_subprocess_background_receive_output(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && proc->is_background);

    if (!proceed)
        return JSL_SUBPROCESS_BAD_PARAMETERS;

    if (out_errno != NULL)
        *out_errno = 0;

    int32_t local_errno = 0;
    JSLSubProcessResultEnum r;
#if JSL_IS_POSIX
    r = jsl__subprocess_posix_pump_output_once(proc, &local_errno);
#elif JSL_IS_WINDOWS
    r = jsl__subprocess_win_pump_output_once(proc, &local_errno);
#else
    #error "Unsupported platform"
#endif

    if (local_errno != 0)
    {
        proc->last_errno = local_errno;
        if (out_errno != NULL)
            *out_errno = local_errno;
    }
    return r;
}

JSLSubProcessResultEnum jsl_subprocess_background_kill(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && proc->is_background);

    if (!proceed)
        return JSL_SUBPROCESS_BAD_PARAMETERS;

    if (out_errno != NULL)
        *out_errno = 0;

    if (proc->status == JSL_SUBPROCESS_STATUS_EXITED
        || proc->status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL)
        return JSL_SUBPROCESS_SUCCESS;

#if JSL_IS_POSIX
    if (proc->pid > 0 && kill((pid_t) proc->pid, SIGKILL) == -1)
    {
        // ESRCH means the process has already gone; treat as success.
        if (errno != ESRCH)
        {
            if (out_errno != NULL)
                *out_errno = errno;
            return JSL_SUBPROCESS_WAIT_FAILED;
        }
    }
    return JSL_SUBPROCESS_SUCCESS;
#elif JSL_IS_WINDOWS
    if (proc->process_handle != INVALID_HANDLE_VALUE
        && !TerminateProcess(proc->process_handle, 1))
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        return JSL_SUBPROCESS_WAIT_FAILED;
    }
    return JSL_SUBPROCESS_SUCCESS;
#else
    #error "Unsupported platform"
#endif
}

// Per-proc bookkeeping used inside `jsl_subprocess_background_wait`.
// Tracks whether a given proc is still part of the wait set and, on
// POSIX, whether the stdin pump has finished. Lives on the stack inside
// the wait function so nothing here persists past the call.
typedef struct JSL__SubProcessWaitSlot
{
    // Index back into the caller's array. -1 means "slot is unused".
    int32_t index;
    // Ignored procs (bad sentinel, not background, not running) keep
    // `ignored == true`. They are never touched by the wait loop.
    bool ignored;
    // Whether this proc is still counted toward completion. Starts true
    // for valid-running procs. Cleared when the proc hits terminal or
    // is dropped due to repeated pump errors.
    bool still_waiting;
    // True when a pump operation on this proc reported IO_FAILED. The
    // proc is removed from the wait set but the loop continues for its
    // siblings.
    bool io_failed;
} JSL__SubProcessWaitSlot;

JSLSubProcessResultEnum jsl_subprocess_background_wait(
    JSLSubprocess* procs,
    int32_t count,
    int32_t timeout_ms,
    int32_t* out_errno
)
{
    if (procs == NULL || count <= 0)
        return JSL_SUBPROCESS_BAD_PARAMETERS;

    if (out_errno != NULL)
        *out_errno = 0;

    // Pick an allocator from the first proc whose sentinel is valid. If
    // every proc has a bad sentinel there is nothing to wait on, so the
    // caller gets a trivial success without any allocation.
    JSLAllocatorInterface allocator = {0};
    bool have_allocator = false;
    for (int32_t i = 0; i < count; i++)
    {
        if (procs[i].sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL)
        {
            allocator = procs[i].allocator;
            have_allocator = true;
            break;
        }
    }
    if (!have_allocator)
        return JSL_SUBPROCESS_SUCCESS;

    // First pass: classify every proc as ignored, already-terminal, or
    // still-running. Only still-running procs contribute to the wait
    // set; already-terminal is silently treated as "done".
    int64_t slot_bytes = (int64_t) sizeof(JSL__SubProcessWaitSlot) * count;
    JSL__SubProcessWaitSlot stack_slots[16];
    JSL__SubProcessWaitSlot* slots = NULL;
    bool slots_on_heap = false;

    if ((size_t) slot_bytes <= sizeof(stack_slots))
        slots = stack_slots;
    else
    {
        slots = (JSL__SubProcessWaitSlot*) jsl_allocator_interface_alloc(
            allocator, slot_bytes, JSL_DEFAULT_ALLOCATION_ALIGNMENT, false
        );
        if (slots == NULL)
            return JSL_SUBPROCESS_ALLOCATION_FAILED;
        slots_on_heap = true;
    }

    int32_t waiting_count = 0;

    for (int32_t i = 0; i < count; i++)
    {
        slots[i].index = i;
        slots[i].ignored = true;
        slots[i].still_waiting = false;
        slots[i].io_failed = false;

        JSLSubprocess* p = &procs[i];

        bool good_sentinel =
            (p != NULL && p->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);
        if (!good_sentinel || !p->is_background)
            continue;

        if (p->status == JSL_SUBPROCESS_STATUS_EXITED
            || p->status == JSL_SUBPROCESS_STATUS_KILLED_BY_SIGNAL)
        {
            // Terminal on entry: counted as trivially satisfied.
            slots[i].ignored = false;
            slots[i].still_waiting = false;
            continue;
        }

        if (p->status != JSL_SUBPROCESS_STATUS_RUNNING)
            continue;

        slots[i].ignored = false;
        slots[i].still_waiting = true;
        waiting_count++;
    }

    JSLSubProcessResultEnum final_result = JSL_SUBPROCESS_SUCCESS;
    bool any_io_failed = false;

    // No still-waiting procs *and* no terminal-but-non-ignored procs
    // means every proc was rejected by the classifier — nothing to do.
    // When `waiting_count == 0` but some procs are non-ignored
    // (already-terminal at entry), fall through so the platform-
    // specific drain pass below picks up any output the child wrote
    // before exiting. Each platform's main loop is gated on
    // `waiting_count > 0`, so this only runs the drain.
    int32_t non_ignored = 0;
    for (int32_t i = 0; i < count; i++)
    {
        if (!slots[i].ignored)
            non_ignored++;
    }
    if (non_ignored == 0)
    {
        if (slots_on_heap)
            (void) jsl_allocator_interface_free(allocator, slots);
        return JSL_SUBPROCESS_SUCCESS;
    }

#if JSL_IS_POSIX

    int32_t sigpipe_errno = 0;
    if (!jsl__sigchld_ensure_installed(&sigpipe_errno))
    {
        if (out_errno != NULL)
            *out_errno = sigpipe_errno;
        if (slots_on_heap)
            (void) jsl_allocator_interface_free(allocator, slots);
        return JSL_SUBPROCESS_WAIT_FAILED;
    }

    // Pre-reap: a SIGCHLD may have already been delivered and the
    // self-pipe byte has been consumed by a previous wait. Do a single
    // non-blocking waitpid sweep so we start from a correct view.
    for (int32_t i = 0; i < count; i++)
    {
        if (!slots[i].still_waiting)
            continue;
        if (jsl__subprocess_posix_reap_nohang(&procs[i]))
        {
            slots[i].still_waiting = false;
            waiting_count--;
        }
    }

    // Opportunistic first drain: if a child already printed something
    // before we got here, flush it so the caller's sinks are accurate
    // even if every proc was already terminal.
    for (int32_t i = 0; i < count; i++)
    {
        if (slots[i].ignored)
            continue;
        JSLSubprocess* p = &procs[i];
        int32_t pump_errno = 0;
        JSLSubProcessResultEnum pr = jsl__subprocess_posix_pump_output_once(p, &pump_errno);
        if (pr == JSL_SUBPROCESS_IO_FAILED)
        {
            p->last_errno = pump_errno;
            slots[i].io_failed = true;
            any_io_failed = true;
        }
    }

    bool infinite = (timeout_ms < 0);
    int64_t deadline = infinite ? 0 : jsl__monotonic_ms() + (int64_t) timeout_ms;
    int64_t max_fds = 3 * (int64_t) count + 1;
    struct pollfd* pfds = (struct pollfd*) jsl_allocator_interface_alloc(
        allocator, max_fds * (int64_t) sizeof(struct pollfd),
        JSL_DEFAULT_ALLOCATION_ALIGNMENT, false
    );
    // Stash per-proc fd indices so we don't re-scan the array after
    // poll returns.
    int* stdin_idx = (int*) jsl_allocator_interface_alloc(
        allocator, (int64_t) count * (int64_t) sizeof(int),
        JSL_DEFAULT_ALLOCATION_ALIGNMENT, false
    );
    int* stdout_idx = (int*) jsl_allocator_interface_alloc(
        allocator, (int64_t) count * (int64_t) sizeof(int),
        JSL_DEFAULT_ALLOCATION_ALIGNMENT, false
    );
    int* stderr_idx = (int*) jsl_allocator_interface_alloc(
        allocator, (int64_t) count * (int64_t) sizeof(int),
        JSL_DEFAULT_ALLOCATION_ALIGNMENT, false
    );
    if (pfds == NULL || stdin_idx == NULL || stdout_idx == NULL || stderr_idx == NULL)
    {
        (void) jsl_allocator_interface_free(allocator, pfds);
        (void) jsl_allocator_interface_free(allocator, stdin_idx);
        (void) jsl_allocator_interface_free(allocator, stdout_idx);
        (void) jsl_allocator_interface_free(allocator, stderr_idx);
        if (slots_on_heap)
            (void) jsl_allocator_interface_free(allocator, slots);
        return JSL_SUBPROCESS_ALLOCATION_FAILED;
    }

    // Ignore SIGPIPE for the duration of the wait so a child closing its
    // stdin early surfaces as EPIPE instead of killing us.
    void (*prev_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);

    while (waiting_count > 0)
    {
        int nfds = 0;
        for (int32_t i = 0; i < count; i++)
        {
            stdin_idx[i] = -1;
            stdout_idx[i] = -1;
            stderr_idx[i] = -1;

            if (!slots[i].still_waiting || slots[i].io_failed)
                continue;

            JSLSubprocess* p = &procs[i];
            if (p->stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY
                && p->stdin_write_fd != -1)
            {
                pfds[nfds].fd = p->stdin_write_fd;
                pfds[nfds].events = POLLOUT;
                pfds[nfds].revents = 0;
                stdin_idx[i] = nfds;
                nfds++;
            }
            if (p->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK
                && p->stdout_read_fd != -1
                && !p->stdout_eof_seen)
            {
                pfds[nfds].fd = p->stdout_read_fd;
                pfds[nfds].events = POLLIN;
                pfds[nfds].revents = 0;
                stdout_idx[i] = nfds;
                nfds++;
            }
            if (p->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK
                && p->stderr_read_fd != -1
                && !p->stderr_eof_seen)
            {
                pfds[nfds].fd = p->stderr_read_fd;
                pfds[nfds].events = POLLIN;
                pfds[nfds].revents = 0;
                stderr_idx[i] = nfds;
                nfds++;
            }
        }

        int self_pipe_idx = nfds;
        pfds[nfds].fd = jsl__sigchld_pipe[0];
        pfds[nfds].events = POLLIN;
        pfds[nfds].revents = 0;
        nfds++;

        int wait_ms;
        if (infinite)
            wait_ms = -1;
        else
        {
            int64_t remaining = deadline - jsl__monotonic_ms();
            if (remaining <= 0)
                wait_ms = 0;
            else if (remaining > (int64_t) INT_MAX)
                wait_ms = INT_MAX;
            else
                wait_ms = (int) remaining;
        }

        int pret = poll(pfds, (nfds_t) nfds, wait_ms);

        if (pret == -1)
        {
            if (errno == EINTR)
                continue;
            int32_t saved = errno;
            if (out_errno != NULL)
                *out_errno = saved;
            signal(SIGPIPE, prev_sigpipe);
            (void) jsl_allocator_interface_free(allocator, pfds);
            (void) jsl_allocator_interface_free(allocator, stdin_idx);
            (void) jsl_allocator_interface_free(allocator, stdout_idx);
            (void) jsl_allocator_interface_free(allocator, stderr_idx);
            if (slots_on_heap)
                (void) jsl_allocator_interface_free(allocator, slots);
            return JSL_SUBPROCESS_WAIT_FAILED;
        }

        bool sigchld_woke = (pret > 0
            && self_pipe_idx >= 0
            && (pfds[self_pipe_idx].revents & POLLIN) != 0);

        if (sigchld_woke)
            jsl__sigchld_drain();

        // Pump ready pipes. Every wake gets a full sweep rather than
        // only the fds that signalled, because a child that printed
        // while poll was unblocked by SIGCHLD still has readable data.
        for (int32_t i = 0; i < count; i++)
        {
            if (!slots[i].still_waiting || slots[i].io_failed)
                continue;

            JSLSubprocess* p = &procs[i];

            if (stdin_idx[i] != -1
                && (pfds[stdin_idx[i]].revents & (POLLOUT | POLLERR | POLLHUP)) != 0)
            {
                int32_t e = 0;
                (void) jsl__subprocess_posix_pump_stdin_once(p, &e);
                if (e != 0)
                    p->last_errno = e;
            }

            if (p->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK
                || p->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
            {
                int32_t e = 0;
                JSLSubProcessResultEnum pr = jsl__subprocess_posix_pump_output_once(p, &e);
                if (pr == JSL_SUBPROCESS_IO_FAILED)
                {
                    p->last_errno = e;
                    slots[i].io_failed = true;
                    any_io_failed = true;
                    slots[i].still_waiting = false;
                    waiting_count--;
                }
                else if (e != 0)
                {
                    p->last_errno = e;
                }
            }
        }

        // SIGCHLD wake or opportunistic reap: try non-blocking waitpid
        // for every still-running proc. Also reap on non-sigchld wakes
        // because EOF on a pipe often precedes our signal.
        for (int32_t i = 0; i < count; i++)
        {
            if (!slots[i].still_waiting)
                continue;
            if (jsl__subprocess_posix_reap_nohang(&procs[i]))
            {
                slots[i].still_waiting = false;
                waiting_count--;
                // One last drain so anything buffered on a now-closed
                // pipe makes it to the caller's sink.
                int32_t e = 0;
                (void) jsl__subprocess_posix_pump_output_once(&procs[i], &e);
                if (e != 0)
                    procs[i].last_errno = e;
            }
        }

        if (waiting_count == 0)
            break;

        if (!infinite && jsl__monotonic_ms() >= deadline)
        {
            final_result = JSL_SUBPROCESS_TIMEOUT_REACHED;
            break;
        }

        // `timeout_ms == 0` single-shot semantics: one cycle then
        // return. If every remaining proc is still running we report
        // TIMEOUT_REACHED so the caller can distinguish that from
        // "everything finished in one poll tick".
        if (!infinite && timeout_ms == 0 && waiting_count > 0)
        {
            final_result = JSL_SUBPROCESS_TIMEOUT_REACHED;
            break;
        }
    }

    signal(SIGPIPE, prev_sigpipe);
    (void) jsl_allocator_interface_free(allocator, pfds);
    (void) jsl_allocator_interface_free(allocator, stdin_idx);
    (void) jsl_allocator_interface_free(allocator, stdout_idx);
    (void) jsl_allocator_interface_free(allocator, stderr_idx);

#elif JSL_IS_WINDOWS

    // Windows event-driven wait: up to 16 procs per group, each
    // contributing up to 4 handles (process + stdin-write event +
    // stdout-read event + stderr-read event), for a maximum wait set
    // of 64 — the `WaitForMultipleObjectsEx` ceiling. When more than
    // one group is needed, rotate every 50 ms so no group starves.
    const int32_t group_size = 16;
    const DWORD rotation_slice_ms = 50;

    int64_t ms_deadline = 0;
    bool infinite = (timeout_ms < 0);
    if (!infinite)
        ms_deadline = jsl__monotonic_ms() + (int64_t) timeout_ms;

    // Prime: kick a pending read on each non-ignored proc so the
    // event-driven loop has something to wait on. Already-terminal
    // procs (`still_waiting == false`, `ignored == false`) also get a
    // full blocking drain here — without it, output the child wrote
    // and then exited before this call would be silently dropped
    // (e.g. when the caller polled the proc to EXITED before calling
    // wait, then the main `while (waiting_count > 0)` loop never runs
    // and cleanup `CancelIoEx`s the pending read).
    for (int32_t i = 0; i < count; i++)
    {
        if (slots[i].ignored)
            continue;
        JSLSubprocess* p = &procs[i];

        int32_t e = 0;
        (void) jsl__subprocess_win_pump_stdin_once(p, &e);
        if (e != 0) p->last_errno = e;

        if (slots[i].still_waiting)
        {
            e = 0;
            JSLSubProcessResultEnum pr = jsl__subprocess_win_pump_output_once(p, &e);
            if (pr == JSL_SUBPROCESS_IO_FAILED)
            {
                p->last_errno = e;
                slots[i].io_failed = true;
                any_io_failed = true;
            }
            else if (e != 0)
            {
                p->last_errno = e;
            }
        }
        else
        {
            // Already-terminal: the proc's write end is gone, so
            // finalize_pending_reads will resolve every outstanding
            // read promptly with either bytes or ERROR_BROKEN_PIPE.
            // Using the blocking finalizer (rather than a single
            // pump_output_once kick) is required because overlapped
            // reads on byte pipes typically return ERROR_IO_PENDING
            // even when data is available, and the main wait loop
            // won't run for these procs.
            e = 0;
            JSLSubProcessResultEnum pr =
                jsl__subprocess_win_finalize_pending_reads(p, &e);
            if (pr == JSL_SUBPROCESS_IO_FAILED)
            {
                p->last_errno = e;
                slots[i].io_failed = true;
                any_io_failed = true;
            }
            else if (e != 0)
            {
                p->last_errno = e;
            }
        }
    }

    // Classification of each wait-set slot. We keep one parallel array
    // alongside `handles[]` so that when `WaitForMultipleObjectsEx`
    // reports index `i`, we can look up (a) which proc it belongs to
    // and (b) which kind of handle it was (process exit, stdin write,
    // stdout read, stderr read).
    enum {
        JSL__WIN_WAIT_KIND_PROCESS = 0,
        JSL__WIN_WAIT_KIND_STDIN_WRITE,
        JSL__WIN_WAIT_KIND_STDOUT_READ,
        JSL__WIN_WAIT_KIND_STDERR_READ
    };

    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    int32_t handle_to_slot[MAXIMUM_WAIT_OBJECTS];
    int32_t handle_kind[MAXIMUM_WAIT_OBJECTS];

    int32_t num_groups = (waiting_count + group_size - 1) / group_size;
    if (num_groups < 1)
        num_groups = 1;
    int32_t current_group = 0;

    while (waiting_count > 0)
    {
        // Identify which procs fall into the current group: the
        // `group_size` still-waiting procs starting at the
        // `current_group`-th group boundary.
        int32_t group_start = current_group * group_size;
        int32_t group_member = 0;
        int32_t n_running_in_group = 0;
        DWORD n_handles = 0;

        for (int32_t i = 0;
            i < count && n_handles + 4 <= MAXIMUM_WAIT_OBJECTS;
            i++)
        {
            if (!slots[i].still_waiting)
                continue;

            if (group_member < group_start)
            {
                group_member++;
                continue;
            }
            if (group_member >= group_start + group_size)
                break;
            group_member++;

            JSLSubprocess* p = &procs[i];

            if (p->process_handle == NULL
                || p->process_handle == INVALID_HANDLE_VALUE)
                continue;

            handles[n_handles] = p->process_handle;
            handle_to_slot[n_handles] = i;
            handle_kind[n_handles] = JSL__WIN_WAIT_KIND_PROCESS;
            n_handles++;
            n_running_in_group++;

            if (!slots[i].io_failed
                && p->stdin_write_handle != INVALID_HANDLE_VALUE
                && p->stdin_write_pending
                && p->stdin_write_event != NULL)
            {
                handles[n_handles] = p->stdin_write_event;
                handle_to_slot[n_handles] = i;
                handle_kind[n_handles] = JSL__WIN_WAIT_KIND_STDIN_WRITE;
                n_handles++;
            }

            if (!slots[i].io_failed
                && p->stdout_read_handle != INVALID_HANDLE_VALUE
                && p->stdout_read_pending
                && p->stdout_read_event != NULL)
            {
                handles[n_handles] = p->stdout_read_event;
                handle_to_slot[n_handles] = i;
                handle_kind[n_handles] = JSL__WIN_WAIT_KIND_STDOUT_READ;
                n_handles++;
            }

            if (!slots[i].io_failed
                && p->stderr_read_handle != INVALID_HANDLE_VALUE
                && p->stderr_read_pending
                && p->stderr_read_event != NULL)
            {
                handles[n_handles] = p->stderr_read_event;
                handle_to_slot[n_handles] = i;
                handle_kind[n_handles] = JSL__WIN_WAIT_KIND_STDERR_READ;
                n_handles++;
            }
        }

        if (n_handles == 0 || n_running_in_group == 0)
        {
            // Nothing in this group (already-drained after prior
            // iterations). Advance rotation.
            current_group = (current_group + 1) % (num_groups > 0 ? num_groups : 1);
            // If every group is empty we'd loop forever — break out.
            if (waiting_count == 0)
                break;
            // Cheap guard: if there is genuinely no running proc with a
            // live process handle anywhere, bail.
            bool any = false;
            for (int32_t j = 0; j < count; j++)
            {
                if (slots[j].still_waiting
                    && procs[j].process_handle != NULL
                    && procs[j].process_handle != INVALID_HANDLE_VALUE)
                {
                    any = true;
                    break;
                }
            }
            if (!any)
                break;
            continue;
        }

        DWORD per_wait_timeout;
        if (infinite)
        {
            per_wait_timeout = (num_groups > 1) ? rotation_slice_ms : INFINITE;
        }
        else
        {
            int64_t remaining = ms_deadline - jsl__monotonic_ms();
            if (remaining <= 0)
            {
                final_result = JSL_SUBPROCESS_TIMEOUT_REACHED;
                break;
            }
            // Clamp to INFINITE - 1: passing INFINITE to
            // WaitForMultipleObjectsEx means "no timeout", which would
            // turn a finite (but very large) caller-requested wait into
            // a wait-forever. The outer deadline check at the top of
            // this loop will re-evaluate `remaining` on the next pass.
            DWORD cap = (num_groups > 1)
                ? rotation_slice_ms
                : (remaining >= (int64_t) INFINITE ? (INFINITE - 1) : (DWORD) remaining);
            if (num_groups > 1 && remaining < (int64_t) cap)
                cap = (DWORD) remaining;
            per_wait_timeout = cap;
        }

        if (timeout_ms == 0)
            per_wait_timeout = 0;

        DWORD wret = WaitForMultipleObjectsEx(
            n_handles, handles, FALSE, per_wait_timeout, FALSE
        );

        if (wret == WAIT_FAILED)
        {
            DWORD err = GetLastError();
            if (out_errno != NULL)
                *out_errno = (int32_t) err;
            if (slots_on_heap)
                (void) jsl_allocator_interface_free(allocator, slots);
            return JSL_SUBPROCESS_WAIT_FAILED;
        }

        if (wret >= WAIT_OBJECT_0 && wret < WAIT_OBJECT_0 + n_handles)
        {
            int32_t idx = (int32_t)(wret - WAIT_OBJECT_0);
            int32_t slot_idx = handle_to_slot[idx];
            int32_t kind = handle_kind[idx];
            JSLSubprocess* p = &procs[slot_idx];

            if (kind == JSL__WIN_WAIT_KIND_PROCESS)
            {
                DWORD code = 0;
                if (GetExitCodeProcess(p->process_handle, &code))
                {
                    p->exit_code = (int32_t) code;
                    p->status = JSL_SUBPROCESS_STATUS_EXITED;
                }
                else
                {
                    p->exit_code = -1;
                    p->status = JSL_SUBPROCESS_STATUS_EXITED;
                }

                // Drain any remaining output. Must use the blocking
                // finalizer, not a plain pump pass: process exit can
                // signal ahead of the pending read's IRP completion,
                // so the non-blocking
                // `GetOverlappedResult(..., FALSE)` inside the pump
                // would return ERROR_IO_INCOMPLETE and the bytes the
                // child wrote just before exit would be lost when
                // cleanup later cancels the op via `CancelIoEx`. See
                // the long comment on `finalize_pending_reads` for
                // the full rationale.
                int32_t e = 0;
                JSLSubProcessResultEnum pr =
                    jsl__subprocess_win_finalize_pending_reads(p, &e);
                if (pr == JSL_SUBPROCESS_IO_FAILED)
                {
                    p->last_errno = e;
                    slots[slot_idx].io_failed = true;
                    any_io_failed = true;
                }
                else if (e != 0)
                {
                    p->last_errno = e;
                }

                slots[slot_idx].still_waiting = false;
                waiting_count--;
            }
            else
            {
                // Pipe event fired; drive the pump which will consume
                // the completion and issue the next op.
                int32_t e = 0;
                if (kind == JSL__WIN_WAIT_KIND_STDIN_WRITE)
                {
                    (void) jsl__subprocess_win_pump_stdin_once(p, &e);
                    if (e != 0) p->last_errno = e;
                }
                else
                {
                    JSLSubProcessResultEnum pr =
                        jsl__subprocess_win_pump_output_once(p, &e);
                    if (pr == JSL_SUBPROCESS_IO_FAILED)
                    {
                        p->last_errno = e;
                        slots[slot_idx].io_failed = true;
                        any_io_failed = true;
                    }
                    else if (e != 0)
                    {
                        p->last_errno = e;
                    }
                }
            }
        }
        else if (wret == WAIT_TIMEOUT)
        {
            // No activity in this slice. Rotate to the next group.
            if (num_groups > 1)
                current_group = (current_group + 1) % num_groups;

            if (!infinite && jsl__monotonic_ms() >= ms_deadline)
            {
                if (waiting_count > 0)
                    final_result = JSL_SUBPROCESS_TIMEOUT_REACHED;
                break;
            }

            if (timeout_ms == 0)
            {
                if (waiting_count > 0)
                    final_result = JSL_SUBPROCESS_TIMEOUT_REACHED;
                break;
            }
        }
        else
        {
            // WAIT_ABANDONED_* should not happen with processes/events.
            if (out_errno != NULL)
                *out_errno = (int32_t) wret;
            if (slots_on_heap)
                (void) jsl_allocator_interface_free(allocator, slots);
            return JSL_SUBPROCESS_WAIT_FAILED;
        }

        // Recompute group count if waiting_count shrank past a boundary.
        int32_t new_num_groups = (waiting_count + group_size - 1) / group_size;
        if (new_num_groups < 1) new_num_groups = 1;
        if (new_num_groups != num_groups)
        {
            num_groups = new_num_groups;
            if (current_group >= num_groups)
                current_group = 0;
        }

        // `timeout_ms == 0` single-shot semantics: one cycle then
        // return. Mirrors the POSIX branch. Without this, a chatty
        // child whose handles keep signalling would never produce a
        // WAIT_TIMEOUT and the loop would run well past one tick.
        if (!infinite && timeout_ms == 0)
        {
            if (waiting_count > 0)
                final_result = JSL_SUBPROCESS_TIMEOUT_REACHED;
            break;
        }
    }

#else
    #error "Unsupported platform"
#endif

    if (slots_on_heap)
        (void) jsl_allocator_interface_free(allocator, slots);

    if (final_result == JSL_SUBPROCESS_SUCCESS && any_io_failed)
        final_result = JSL_SUBPROCESS_IO_FAILED;

    return final_result;
}

void jsl_subprocess_cleanup(JSLSubprocess* proc)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL);

    if (!proceed)
        return;

    if (proc->executable.data != NULL)
        (void) jsl_allocator_interface_free(proc->allocator, proc->executable.data);

    for (int64_t i = 0; i < proc->args_count; i++)
    {
        if (proc->args[i].data != NULL)
            (void) jsl_allocator_interface_free(proc->allocator, proc->args[i].data);
    }

    if (proc->args != NULL)
        (void) jsl_allocator_interface_free(proc->allocator, proc->args);

    for (int64_t i = 0; i < proc->env_count; i++)
    {
        if (proc->env_vars[i].key.data != NULL)
            (void) jsl_allocator_interface_free(proc->allocator, proc->env_vars[i].key.data);
        if (proc->env_vars[i].value.data != NULL)
            (void) jsl_allocator_interface_free(proc->allocator, proc->env_vars[i].value.data);
    }

    if (proc->env_vars != NULL)
        (void) jsl_allocator_interface_free(proc->allocator, proc->env_vars);

    if (proc->working_directory.data != NULL)
        (void) jsl_allocator_interface_free(proc->allocator, proc->working_directory.data);

    if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY && proc->stdin_memory.data != NULL)
        (void) jsl_allocator_interface_free(proc->allocator, proc->stdin_memory.data);

    if (proc->is_background)
    {
        #if JSL_IS_WINDOWS
            // Cancel any in-flight overlapped ops before closing the
            // underlying pipe handles. `CancelIoEx` may report
            // `ERROR_NOT_FOUND` if the op already completed; that's
            // fine. Close the paired event handle after the pipe.
            if (proc->stdin_write_pending
                && proc->stdin_write_handle != NULL
                && proc->stdin_write_handle != INVALID_HANDLE_VALUE)
                CancelIoEx(proc->stdin_write_handle, &proc->stdin_write_overlapped);
            if (proc->stdout_read_pending
                && proc->stdout_read_handle != NULL
                && proc->stdout_read_handle != INVALID_HANDLE_VALUE)
                CancelIoEx(proc->stdout_read_handle, &proc->stdout_read_overlapped);
            if (proc->stderr_read_pending
                && proc->stderr_read_handle != NULL
                && proc->stderr_read_handle != INVALID_HANDLE_VALUE)
                CancelIoEx(proc->stderr_read_handle, &proc->stderr_read_overlapped);

            if (proc->stdin_write_handle != NULL && proc->stdin_write_handle != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stdin_write_handle);
            if (proc->stdout_read_handle != NULL && proc->stdout_read_handle != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stdout_read_handle);
            if (proc->stderr_read_handle != NULL && proc->stderr_read_handle != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stderr_read_handle);

            if (proc->stdin_write_event != NULL && proc->stdin_write_event != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stdin_write_event);
            if (proc->stdout_read_event != NULL && proc->stdout_read_event != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stdout_read_event);
            if (proc->stderr_read_event != NULL && proc->stderr_read_event != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stderr_read_event);

            if (proc->process_handle != NULL && proc->process_handle != INVALID_HANDLE_VALUE)
            {
                if (proc->status == JSL_SUBPROCESS_STATUS_RUNNING)
                    TerminateProcess(proc->process_handle, 1);
                CloseHandle(proc->process_handle);
            }
            proc->stdin_write_handle = INVALID_HANDLE_VALUE;
            proc->stdout_read_handle = INVALID_HANDLE_VALUE;
            proc->stderr_read_handle = INVALID_HANDLE_VALUE;
            proc->stdin_write_event = NULL;
            proc->stdout_read_event = NULL;
            proc->stderr_read_event = NULL;
            proc->stdin_write_pending = false;
            proc->stdout_read_pending = false;
            proc->stderr_read_pending = false;
            proc->process_handle = INVALID_HANDLE_VALUE;
        #elif JSL_IS_POSIX
            if (proc->pid > 0 && proc->status == JSL_SUBPROCESS_STATUS_RUNNING)
            {
                kill(proc->pid, SIGKILL);
                int status_code = 0;
                (void) waitpid(proc->pid, &status_code, 0);
            }
            if (proc->stdin_write_fd >= 0)
                close(proc->stdin_write_fd);
            if (proc->stdout_read_fd >= 0)
                close(proc->stdout_read_fd);
            if (proc->stderr_read_fd >= 0)
                close(proc->stderr_read_fd);
            proc->stdin_write_fd = -1;
            proc->stdout_read_fd = -1;
            proc->stderr_read_fd = -1;
            proc->pid = 0;
        #endif
        proc->is_background = false;
    }

    proc->sentinel = 0;
    proc->executable = jsl_immutable_memory(NULL, 0);
    proc->args = NULL;
    proc->args_count = 0;
    proc->args_capacity = 0;
    proc->env_vars = NULL;
    proc->env_count = 0;
    proc->env_capacity = 0;
    proc->working_directory = jsl_immutable_memory(NULL, 0);
    proc->stdin_memory = jsl_immutable_memory(NULL, 0);
}

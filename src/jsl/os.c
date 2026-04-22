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
static bool jsl__subprocess_posix_pump_io(
    JSLSubprocess* proc,
    JSL__SubProcessPosixLaunch* ctx,
    int32_t* out_errno
)
{
    int stdin_write = ctx->stdin_pipe[1];
    int stdout_read = ctx->stdout_pipe[0];
    int stderr_read = ctx->stderr_pipe[0];

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

        int pret = poll(pfds, (nfds_t) nfds, -1);
        if (pret == -1)
        {
            if (errno == EINTR)
                continue;
            if (out_errno != NULL)
                *out_errno = errno;
            ok = false;
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

    if (stdin_write != -1) { close(stdin_write); ctx->stdin_pipe[1] = -1; }
    if (stdout_read != -1) { close(stdout_read); ctx->stdout_pipe[0] = -1; }
    if (stderr_read != -1) { close(stderr_read); ctx->stderr_pipe[0] = -1; }

    signal(SIGPIPE, prev_sigpipe);
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
    if (w > 0)
    {
        proc->stdin_write_offset += (int64_t) w;
        if (proc->stdin_write_offset >= proc->stdin_memory.length)
            close_pipe = true;
    }
    else if (w == -1
        && saved_errno != EAGAIN
        && saved_errno != EWOULDBLOCK
        && saved_errno != EINTR)
    {
        // EPIPE / other write errors: child closed stdin or pipe is broken.
        // Treat as "we're done with stdin"; not reported as an error.
        close_pipe = true;
    }

    if (close_pipe)
        jsl__subprocess_close_fd(&proc->stdin_write_fd);

    (void) out_errno;
    return JSL_SUBPROCESS_SUCCESS;
}

// Read once from a single read-end pipe into its sink. Closes the fd and
// sets *eof_seen on EOF or non-retryable error. Returns IO_FAILED only on
// non-retryable read errors.
static JSLSubProcessResultEnum jsl__subprocess_posix_drain_one(
    int* fd_ptr,
    JSLOutputSink sink,
    bool* eof_seen,
    int32_t* out_errno
)
{
    if (*fd_ptr == -1 || *eof_seen)
        return JSL_SUBPROCESS_SUCCESS;

    uint8_t io_buf[4096];
    ssize_t r = read(*fd_ptr, io_buf, sizeof(io_buf));
    int saved_errno = errno;

    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;

    if (r > 0)
    {
        jsl_output_sink_write(sink, jsl_immutable_memory(io_buf, (int64_t) r));
    }
    else if (r == 0)
    {
        jsl__subprocess_close_fd(fd_ptr);
        *eof_seen = true;
    }
    else if (r == -1
        && saved_errno != EAGAIN
        && saved_errno != EWOULDBLOCK
        && saved_errno != EINTR)
    {
        if (out_errno != NULL)
            *out_errno = saved_errno;
        jsl__subprocess_close_fd(fd_ptr);
        *eof_seen = true;
        result = JSL_SUBPROCESS_IO_FAILED;
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
}

static void jsl__subprocess_win_close_handle(HANDLE* h)
{
    if (*h != INVALID_HANDLE_VALUE && *h != NULL)
    {
        CloseHandle(*h);
        *h = INVALID_HANDLE_VALUE;
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
    JSL__SubProcessWindowsLaunch* ctx
)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = (DWORD) sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    bool ok = true;
    if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY)
    {
        ok = (CreatePipe(&ctx->stdin_read, &ctx->stdin_write, &sa, 0) != 0);
        if (ok)
            SetHandleInformation(ctx->stdin_write, HANDLE_FLAG_INHERIT, 0);
    }
    if (ok && proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        ok = (CreatePipe(&ctx->stdout_read, &ctx->stdout_write, &sa, 0) != 0);
        if (ok)
            SetHandleInformation(ctx->stdout_read, HANDLE_FLAG_INHERIT, 0);
    }
    if (ok && proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        ok = (CreatePipe(&ctx->stderr_read, &ctx->stderr_write, &sa, 0) != 0);
        if (ok)
            SetHandleInformation(ctx->stderr_read, HANDLE_FLAG_INHERIT, 0);
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
        && !jsl__subprocess_win_create_pipes(proc, ctx))
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
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

    if (use_stdhandles)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;

        if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_MEMORY)
            si.hStdInput = ctx->stdin_read;
        else if (proc->stdin_kind == JSL_SUBPROCESS_STDIN_FD)
            si.hStdInput = (HANDLE) _get_osfhandle(proc->stdin_fd);
        else
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        if (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
            si.hStdOutput = ctx->stdout_write;
        else if (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_FD)
            si.hStdOutput = (HANDLE) _get_osfhandle(proc->stdout_fd);
        else
            si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

        if (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
            si.hStdError = ctx->stderr_write;
        else if (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_FD)
            si.hStdError = (HANDLE) _get_osfhandle(proc->stderr_fd);
        else
            si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }

    ZeroMemory(out_pi, sizeof(*out_pi));

    BOOL created = CreateProcessA(
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

    if (!created)
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        jsl__subprocess_win_close_all_pipes(ctx);
        return JSL_SUBPROCESS_SPAWN_FAILED;
    }

    return JSL_SUBPROCESS_SUCCESS;
}

// One non-blocking write of up to a fixed chunk of stdin_memory to the
// child's stdin handle. Closes the parent-side write handle when the
// buffer is fully written or the write fails.
static JSLSubProcessResultEnum jsl__subprocess_win_pump_stdin_once(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    (void) out_errno;

    if (proc->stdin_kind != JSL_SUBPROCESS_STDIN_MEMORY
        || proc->stdin_write_handle == INVALID_HANDLE_VALUE)
        return JSL_SUBPROCESS_SUCCESS;

    int64_t remaining = proc->stdin_memory.length - proc->stdin_write_offset;
    if (remaining <= 0)
    {
        jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
        return JSL_SUBPROCESS_SUCCESS;
    }

    DWORD to_write = (DWORD)(remaining > 4096 ? 4096 : remaining);
    DWORD written = 0;
    BOOL wok = WriteFile(
        proc->stdin_write_handle,
        proc->stdin_memory.data + proc->stdin_write_offset,
        to_write,
        &written,
        NULL
    );

    if (!wok || written == 0)
    {
        jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
    }
    else
    {
        proc->stdin_write_offset += (int64_t) written;
        if (proc->stdin_write_offset >= proc->stdin_memory.length)
            jsl__subprocess_win_close_handle(&proc->stdin_write_handle);
    }

    return JSL_SUBPROCESS_SUCCESS;
}

// Drain whatever is currently buffered on a single pipe handle into its
// sink. Uses PeekNamedPipe to avoid blocking.
static JSLSubProcessResultEnum jsl__subprocess_win_drain_one(
    HANDLE* h_ptr,
    JSLOutputSink sink,
    bool* eof_seen,
    int32_t* out_errno
)
{
    if (*h_ptr == INVALID_HANDLE_VALUE || *eof_seen)
        return JSL_SUBPROCESS_SUCCESS;

    DWORD avail = 0;
    BOOL pok = PeekNamedPipe(*h_ptr, NULL, 0, NULL, &avail, NULL);
    if (!pok)
    {
        DWORD err = GetLastError();
        // ERROR_BROKEN_PIPE = clean EOF; other errors are real failures.
        jsl__subprocess_win_close_handle(h_ptr);
        *eof_seen = true;
        if (err != ERROR_BROKEN_PIPE && err != ERROR_HANDLE_EOF)
        {
            if (out_errno != NULL)
                *out_errno = (int32_t) err;
            return JSL_SUBPROCESS_IO_FAILED;
        }
        return JSL_SUBPROCESS_SUCCESS;
    }

    if (avail == 0)
        return JSL_SUBPROCESS_SUCCESS;

    uint8_t io_buf[4096];
    DWORD to_read = avail > (DWORD) sizeof(io_buf) ? (DWORD) sizeof(io_buf) : avail;
    DWORD read_n = 0;
    BOOL rok = ReadFile(*h_ptr, io_buf, to_read, &read_n, NULL);

    if (rok && read_n > 0)
    {
        jsl_output_sink_write(sink, jsl_immutable_memory(io_buf, (int64_t) read_n));
    }
    else
    {
        DWORD err = GetLastError();
        jsl__subprocess_win_close_handle(h_ptr);
        *eof_seen = true;
        if (!rok && err != ERROR_BROKEN_PIPE && err != ERROR_HANDLE_EOF)
        {
            if (out_errno != NULL)
                *out_errno = (int32_t) err;
            return JSL_SUBPROCESS_IO_FAILED;
        }
    }

    return JSL_SUBPROCESS_SUCCESS;
}

static JSLSubProcessResultEnum jsl__subprocess_win_pump_output_once(
    JSLSubprocess* proc,
    int32_t* out_errno
)
{
    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;

    if (proc->stdout_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        JSLSubProcessResultEnum r = jsl__subprocess_win_drain_one(
            &proc->stdout_read_handle,
            proc->stdout_sink,
            &proc->stdout_eof_seen,
            out_errno
        );
        if (r != JSL_SUBPROCESS_SUCCESS)
            result = r;
    }

    if (proc->stderr_kind == JSL_SUBPROCESS_OUTPUT_SINK)
    {
        JSLSubProcessResultEnum r = jsl__subprocess_win_drain_one(
            &proc->stderr_read_handle,
            proc->stderr_sink,
            &proc->stderr_eof_seen,
            result == JSL_SUBPROCESS_SUCCESS ? out_errno : NULL
        );
        if (r != JSL_SUBPROCESS_SUCCESS && result == JSL_SUBPROCESS_SUCCESS)
            result = r;
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
    int32_t* out_exit_code,
    int32_t* out_errno
)
{
    bool proceed = (proc != NULL
        && proc->sentinel == JSL__SUBPROCESS_PRIVATE_SENTINEL
        && out_exit_code != NULL);

    if (!proceed)
        return JSL_SUBPROCESS_BAD_PARAMETERS;

    if (proc->status != JSL_SUBPROCESS_STATUS_NOT_STARTED)
        return JSL_SUBPROCESS_ALREADY_STARTED;

    *out_exit_code = -1;
    if (out_errno != NULL)
        *out_errno = 0;

    JSLSubProcessResultEnum result = JSL_SUBPROCESS_SUCCESS;

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

    int32_t io_errno = 0;
    bool io_ok = jsl__subprocess_posix_pump_io(proc, &ctx, &io_errno);

    JSLSubProcessResultEnum wait_result = jsl__subprocess_posix_wait(
        pid,
        &proc->status,
        out_exit_code,
        out_errno
    );

    if (wait_result != JSL_SUBPROCESS_SUCCESS
        && wait_result != JSL_SUBPROCESS_KILLED_BY_SIGNAL)
        result = wait_result;
    else if (!io_ok)
    {
        if (out_errno != NULL)
            *out_errno = io_errno;
        result = JSL_SUBPROCESS_IO_FAILED;
    }
    else if (wait_result == JSL_SUBPROCESS_KILLED_BY_SIGNAL)
    {
        result = JSL_SUBPROCESS_KILLED_BY_SIGNAL;
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

    HANDLE stdin_write = ctx.stdin_write;
    HANDLE stdout_read = ctx.stdout_read;
    HANDLE stderr_read = ctx.stderr_read;

    // I/O pump. Poll each pipe with PeekNamedPipe since Windows doesn't
    // expose a portable equivalent of poll() for anonymous pipes.
    int64_t stdin_offset = 0;
    bool io_error = false;

    while (stdin_write != INVALID_HANDLE_VALUE
        || stdout_read != INVALID_HANDLE_VALUE
        || stderr_read != INVALID_HANDLE_VALUE)
    {
        bool did_work = false;

        if (stdin_write != INVALID_HANDLE_VALUE)
        {
            int64_t remaining = proc->stdin_memory.length - stdin_offset;
            if (remaining <= 0)
            {
                CloseHandle(stdin_write);
                stdin_write = INVALID_HANDLE_VALUE;
                did_work = true;
            }
            else
            {
                DWORD to_write = (DWORD)(remaining > 4096 ? 4096 : remaining);
                DWORD written = 0;
                BOOL wok = WriteFile(
                    stdin_write,
                    proc->stdin_memory.data + stdin_offset,
                    to_write,
                    &written,
                    NULL
                );
                if (!wok || written == 0)
                {
                    CloseHandle(stdin_write);
                    stdin_write = INVALID_HANDLE_VALUE;
                }
                else
                {
                    stdin_offset += (int64_t) written;
                }
                did_work = true;
            }
        }

        uint8_t io_buf[4096];

        if (stdout_read != INVALID_HANDLE_VALUE)
        {
            DWORD avail = 0;
            BOOL pok = PeekNamedPipe(stdout_read, NULL, 0, NULL, &avail, NULL);
            if (!pok)
            {
                CloseHandle(stdout_read);
                stdout_read = INVALID_HANDLE_VALUE;
                did_work = true;
            }
            else if (avail > 0)
            {
                DWORD read_n = 0;
                BOOL rok = ReadFile(stdout_read, io_buf, (DWORD) sizeof(io_buf), &read_n, NULL);
                if (rok && read_n > 0)
                {
                    jsl_output_sink_write(
                        proc->stdout_sink,
                        jsl_immutable_memory(io_buf, (int64_t) read_n)
                    );
                }
                else
                {
                    CloseHandle(stdout_read);
                    stdout_read = INVALID_HANDLE_VALUE;
                }
                did_work = true;
            }
        }

        if (stderr_read != INVALID_HANDLE_VALUE)
        {
            DWORD avail = 0;
            BOOL pok = PeekNamedPipe(stderr_read, NULL, 0, NULL, &avail, NULL);
            if (!pok)
            {
                CloseHandle(stderr_read);
                stderr_read = INVALID_HANDLE_VALUE;
                did_work = true;
            }
            else if (avail > 0)
            {
                DWORD read_n = 0;
                BOOL rok = ReadFile(stderr_read, io_buf, (DWORD) sizeof(io_buf), &read_n, NULL);
                if (rok && read_n > 0)
                {
                    jsl_output_sink_write(
                        proc->stderr_sink,
                        jsl_immutable_memory(io_buf, (int64_t) read_n)
                    );
                }
                else
                {
                    CloseHandle(stderr_read);
                    stderr_read = INVALID_HANDLE_VALUE;
                }
                did_work = true;
            }
        }

        if (!did_work)
        {
            DWORD wait_res = WaitForSingleObject(pi.hProcess, 5);
            if (wait_res == WAIT_OBJECT_0)
            {
                if (stdout_read != INVALID_HANDLE_VALUE)
                {
                    DWORD avail = 0;
                    if (!PeekNamedPipe(stdout_read, NULL, 0, NULL, &avail, NULL) || avail == 0)
                    {
                        CloseHandle(stdout_read);
                        stdout_read = INVALID_HANDLE_VALUE;
                    }
                }
                if (stderr_read != INVALID_HANDLE_VALUE)
                {
                    DWORD avail = 0;
                    if (!PeekNamedPipe(stderr_read, NULL, 0, NULL, &avail, NULL) || avail == 0)
                    {
                        CloseHandle(stderr_read);
                        stderr_read = INVALID_HANDLE_VALUE;
                    }
                }
                if (stdin_write != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(stdin_write);
                    stdin_write = INVALID_HANDLE_VALUE;
                }
            }
        }
    }

    DWORD wait_res = WaitForSingleObject(pi.hProcess, INFINITE);
    if (wait_res == WAIT_FAILED)
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return JSL_SUBPROCESS_WAIT_FAILED;
    }

    DWORD exit_code = 0;
    if (!GetExitCodeProcess(pi.hProcess, &exit_code))
    {
        DWORD err = GetLastError();
        if (out_errno != NULL)
            *out_errno = (int32_t) err;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        proc->status = JSL_SUBPROCESS_STATUS_FAILED_TO_START;
        return JSL_SUBPROCESS_WAIT_FAILED;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    *out_exit_code = (int32_t) exit_code;
    proc->status = JSL_SUBPROCESS_STATUS_EXITED;

    if (io_error)
        result = JSL_SUBPROCESS_IO_FAILED;

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

#if JSL_IS_POSIX
    return jsl__subprocess_posix_poll(proc, timeout_ms, out_status, out_exit_code, out_errno);
#elif JSL_IS_WINDOWS
    return jsl__subprocess_win_poll(proc, timeout_ms, out_status, out_exit_code, out_errno);
#else
    #error "Unsupported platform"
#endif
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

#if JSL_IS_POSIX
    return jsl__subprocess_posix_pump_stdin_once(proc, out_errno);
#elif JSL_IS_WINDOWS
    return jsl__subprocess_win_pump_stdin_once(proc, out_errno);
#else
    #error "Unsupported platform"
#endif
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

#if JSL_IS_POSIX
    return jsl__subprocess_posix_pump_output_once(proc, out_errno);
#elif JSL_IS_WINDOWS
    return jsl__subprocess_win_pump_output_once(proc, out_errno);
#else
    #error "Unsupported platform"
#endif
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
            if (proc->stdin_write_handle != NULL && proc->stdin_write_handle != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stdin_write_handle);
            if (proc->stdout_read_handle != NULL && proc->stdout_read_handle != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stdout_read_handle);
            if (proc->stderr_read_handle != NULL && proc->stderr_read_handle != INVALID_HANDLE_VALUE)
                CloseHandle(proc->stderr_read_handle);
            if (proc->process_handle != NULL && proc->process_handle != INVALID_HANDLE_VALUE)
            {
                if (proc->status == JSL_SUBPROCESS_STATUS_RUNNING)
                    TerminateProcess(proc->process_handle, 1);
                CloseHandle(proc->process_handle);
            }
            proc->stdin_write_handle = INVALID_HANDLE_VALUE;
            proc->stdout_read_handle = INVALID_HANDLE_VALUE;
            proc->stderr_read_handle = INVALID_HANDLE_VALUE;
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

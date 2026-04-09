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
#include "os.h"

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

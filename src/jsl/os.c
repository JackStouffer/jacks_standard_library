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

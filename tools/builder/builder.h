/** 
 * Copyright (c) 2024 Alexey Kutepov
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef NOB_H_
#define NOB_H_
#ifdef _WIN32
#    ifndef _CRT_SECURE_NO_WARNINGS
#        define _CRT_SECURE_NO_WARNINGS (1)
#    endif // _CRT_SECURE_NO_WARNINGS
#endif //  _WIN32

#ifndef NOBDEF
/*
   Goes before declarations and definitions of the nob functions. Useful to `#define NOBDEF static inline`
   if your source code is a single file and you want the compiler to remove unused functions.
*/
#define NOBDEF
#endif /* NOBDEF */

#ifndef NOB_ASSERT
#include <assert.h>
#define NOB_ASSERT assert
#endif /* NOB_ASSERT */

#ifndef NOB_REALLOC
#include <stdlib.h>
#define NOB_REALLOC realloc
#endif /* NOB_REALLOC */

#ifndef NOB_FREE
#include <stdlib.h>
#define NOB_FREE free
#endif /* NOB_FREE */

#ifdef NOB_WARN_DEPRECATED
#    ifndef NOB_DEPRECATED
#        if defined(__GNUC__) || defined(__clang__)
#            define NOB_DEPRECATED(message) __attribute__((deprecated(message)))
#        elif defined(_MSC_VER)
#            define NOB_DEPRECATED(message) __declspec(deprecated(message))
#        else
#            define NOB_DEPRECATED(...)
#        endif
#    endif /* NOB_DEPRECATED */
#else
#    define NOB_DEPRECATED(...)
#endif /* NOB_WARN_DEPRECATED */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define _WINUSER_
#    define _WINGDI_
#    define _IMM_
#    define _WINCON_
#    include <windows.h>
#    include <direct.h>
#    include <io.h>
#    include <shellapi.h>
#else
#    ifdef __APPLE__
#        include <mach-o/dyld.h>
#    endif
#    ifdef __FreeBSD__
#        include <sys/sysctl.h>
#    endif
#    include <sys/types.h>
#    include <sys/wait.h>
#    include <sys/stat.h>
#    include <unistd.h>
#    include <fcntl.h>
#    include <dirent.h>
#endif

#ifdef __HAIKU__
#   include <image.h>
#endif

#ifdef _WIN32
#    define NOB_LINE_END "\r\n"
#else
#    define NOB_LINE_END "\n"
#endif

#if defined(__GNUC__) || defined(__clang__)
//   https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#    ifdef __MINGW_PRINTF_FORMAT
#        define NOB_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (__MINGW_PRINTF_FORMAT, STRING_INDEX, FIRST_TO_CHECK)))
#    else
#        define NOB_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__ ((format (printf, STRING_INDEX, FIRST_TO_CHECK)))
#    endif // __MINGW_PRINTF_FORMAT
#else
//   TODO: implement NOB_PRINTF_FORMAT for MSVC
#    define NOB_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

#define NOB_UNUSED(value) (void)(value)
#define NOB_TODO(message) do { fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); abort(); } while(0)
#define NOB_UNREACHABLE(message) do { fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); abort(); } while(0)

#define NOB_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))
#define NOB_ARRAY_GET(array, index) \
    (NOB_ASSERT((size_t)index < NOB_ARRAY_LEN(array)), array[(size_t)index])

typedef enum {
    NOB_INFO,
    NOB_WARNING,
    NOB_ERROR,
    NOB_NO_LOGS,
} Nob_Log_Level;

// Any messages with the level below nob_minimal_log_level are going to be suppressed by the nob_default_log_handler.
extern Nob_Log_Level nob_minimal_log_level;

typedef void (Nob_Log_Handler)(Nob_Log_Level level, const char *fmt, va_list args);
NOB_DEPRECATED("Uncapitalized nob_log_handler type is deprecated. Use Nob_Log_Handler instead. It's just when we were releasing the log handler feature we forgot that we had a convention that all the types must be capitalized like that. Sorry about it!")
typedef Nob_Log_Handler nob_log_handler;

NOBDEF void nob_set_log_handler(Nob_Log_Handler *handler);
NOBDEF Nob_Log_Handler *nob_get_log_handler(void);

NOBDEF Nob_Log_Handler nob_default_log_handler;
NOBDEF Nob_Log_Handler nob_cancer_log_handler;
NOBDEF Nob_Log_Handler nob_null_log_handler;

NOBDEF void nob_log(Nob_Log_Level level, const char *fmt, ...) NOB_PRINTF_FORMAT(2, 3);

// It is an equivalent of shift command from bash (do `help shift` in bash). It basically
// pops an element from the beginning of a sized array.
#define nob_shift(xs, xs_sz) (NOB_ASSERT((xs_sz) > 0), (xs_sz)--, *(xs)++)
// NOTE: nob_shift_args() is an alias for an old variant of nob_shift that only worked with
// the command line arguments passed to the main() function. nob_shift() is more generic.
// So nob_shift_args() is semi-deprecated, but I don't see much reason to urgently
// remove it. This alias does not hurt anybody.
#define nob_shift_args(argc, argv) nob_shift(*argv, *argc)

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} Nob_File_Paths;

typedef enum {
    NOB_FILE_REGULAR = 0,
    NOB_FILE_DIRECTORY,
    NOB_FILE_SYMLINK,
    NOB_FILE_OTHER,
} Nob_File_Type;

NOBDEF bool nob_mkdir_if_not_exists(const char *path);
NOBDEF bool nob_copy_file(const char *src_path, const char *dst_path);
NOBDEF bool nob_copy_directory_recursively(const char *src_path, const char *dst_path);
NOBDEF bool nob_read_entire_dir(const char *parent, Nob_File_Paths *children);
NOBDEF bool nob_write_entire_file(const char *path, const void *data, size_t size);
NOBDEF Nob_File_Type nob_get_file_type(const char *path);
NOBDEF bool nob_delete_file(const char *path);

typedef enum {
    // If the current file is a directory go inside of it.
    NOB_WALK_CONT,
    // If the current file is a directory do not go inside of it.
    NOB_WALK_SKIP,
    // Stop the recursive traversal process entirely.
    NOB_WALK_STOP,
} Nob_Walk_Action;

typedef struct {
    // The path to the visited file. The lifetime of the path string is very short.
    // As soon as the execution exits the Nob_Walk_Func it's dead. Dup it somewhere
    // if you want to preserve it for longer periods of time.
    const char *path;
    // The type of the visited file.
    Nob_File_Type type;
    // How nested we currently are in the directory tree.
    size_t level;
    // User data supplied in Nob_Walk_Dir_Opt.data.
    void *data;
    // The action nob_walk_dir_opt() must perform after the Nob_Walk_Func has returned.
    // Default is NOB_WALK_CONT.
    Nob_Walk_Action *action;
} Nob_Walk_Entry;

// A function that is called by nob_walk_dir_opt() on each visited file.
// Nob_Walk_Entry provides the details about the visited file and also
// expects you to modify the `action` in case you want to alter the
// usual behavior of the recursive walking algorithm.
//
// If the function returns `false`, an error is assumed which causes the entire
// recursive walking process to exit and nob_walk_dir_opt() return `false`.
typedef bool (*Nob_Walk_Func)(Nob_Walk_Entry entry);

typedef struct {
    // User data passed to Nob_Walk_Entry.data
    void *data;
    // Walk the directory in post-order visiting the leaf files first.
    bool post_order;
} Nob_Walk_Dir_Opt;

NOBDEF bool nob_walk_dir_opt(const char *root, Nob_Walk_Func func, Nob_Walk_Dir_Opt);

#define nob_walk_dir(root, func, ...) nob_walk_dir_opt((root), (func), NOB_CLIT(Nob_Walk_Dir_Opt){__VA_ARGS__})

typedef struct {
    char *name;
    bool error;

    struct {
#ifdef _WIN32
        WIN32_FIND_DATA win32_data;
        HANDLE win32_hFind;
        bool win32_init;
#else
        DIR *posix_dir;
        struct dirent *posix_ent;
#endif // _WIN32
    } nob__private; // TODO: we don't have solid conventions regarding private struct fields
} Nob_Dir_Entry;

// nob_dir_entry_open() - open the directory entry for iteration.
// RETURN:
//   true  - Sucess.
//   false - Error. I will be logged automatically with nob_log().
NOBDEF bool nob_dir_entry_open(const char *dir_path, Nob_Dir_Entry *dir);
// nob_dir_entry_next() - acquire the next file in the directory.
// RETURN:
//   true - Successfully acquired the next file.
//   false - Either failure or no more files to iterate. In case of failure dir->error is set to true.
NOBDEF bool nob_dir_entry_next(Nob_Dir_Entry *dir);
NOBDEF void nob_dir_entry_close(Nob_Dir_Entry dir);

#define nob_return_defer(value) do { result = (value); goto defer; } while(0)

// Initial capacity of a dynamic array
#ifndef NOB_DA_INIT_CAP
#define NOB_DA_INIT_CAP 256
#endif

#ifdef __cplusplus
#define NOB_DECLTYPE_CAST(T) (decltype(T))
#else
#define NOB_DECLTYPE_CAST(T)
#endif // __cplusplus

#define nob_da_reserve(da, expected_capacity)                                              \
    do {                                                                                   \
        if ((expected_capacity) > (da)->capacity) {                                        \
            if ((da)->capacity == 0) {                                                     \
                (da)->capacity = NOB_DA_INIT_CAP;                                          \
            }                                                                              \
            while ((expected_capacity) > (da)->capacity) {                                 \
                (da)->capacity *= 2;                                                       \
            }                                                                              \
            (da)->items = NOB_DECLTYPE_CAST((da)->items)NOB_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            NOB_ASSERT((da)->items != NULL && "Buy more RAM lol");                         \
        }                                                                                  \
    } while (0)

// Append an item to a dynamic array
#define nob_da_append(da, item)                \
    do {                                       \
        nob_da_reserve((da), (da)->count + 1); \
        (da)->items[(da)->count++] = (item);   \
    } while (0)

#define nob_da_free(da) NOB_FREE((da).items)

// Append several items to a dynamic array
#define nob_da_append_many(da, new_items, new_items_count)                                      \
    do {                                                                                        \
        nob_da_reserve((da), (da)->count + (new_items_count));                                  \
        memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
        (da)->count += (new_items_count);                                                       \
    } while (0)

#define nob_da_resize(da, new_size)     \
    do {                                \
        nob_da_reserve((da), new_size); \
        (da)->count = (new_size);       \
    } while (0)

#define nob_da_pop(da) (da)->items[(NOB_ASSERT((da)->count > 0), --(da)->count)]
#define nob_da_first(da) (da)->items[(NOB_ASSERT((da)->count > 0), 0)]
#define nob_da_last(da) (da)->items[(NOB_ASSERT((da)->count > 0), (da)->count-1)]
#define nob_da_remove_unordered(da, i)               \
    do {                                             \
        size_t j = (i);                              \
        NOB_ASSERT(j < (da)->count);                 \
        (da)->items[j] = (da)->items[--(da)->count]; \
    } while(0)

// Foreach over Dynamic Arrays. Example:
// ```c
// typedef struct {
//     int *items;
//     size_t count;
//     size_t capacity;
// } Numbers;
//
// Numbers xs = {0};
//
// nob_da_append(&xs, 69);
// nob_da_append(&xs, 420);
// nob_da_append(&xs, 1337);
//
// nob_da_foreach(int, x, &xs) {
//     // `x` here is a pointer to the current element. You can get its index by taking a difference
//     // between `x` and the start of the array which is `x.items`.
//     size_t index = x - xs.items;
//     nob_log(INFO, "%zu: %d", index, *x);
// }
// ```
#define nob_da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

// The Fixed Array append. `items` fields must be a fixed size array. Its size determines the capacity.
#define nob_fa_append(fa, item) \
    (NOB_ASSERT((fa)->count < NOB_ARRAY_LEN((fa)->items)), \
     (fa)->items[(fa)->count++] = (item))

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} Nob_String_Builder;

#define nob_swap(T, a, b) do { T t = a; a = b; b = t; } while (0)

NOBDEF bool nob_read_entire_file(const char *path, Nob_String_Builder *sb);
NOBDEF int nob_sb_appendf(Nob_String_Builder *sb, const char *fmt, ...) NOB_PRINTF_FORMAT(2, 3);
// Pads the String_Builder (sb) to the desired word size boundary with 0s.
// Imagine we have sb that contains 5 `a`-s:
//
//   aaaa|a
//
// If we pad align it by size 4 it will look like this:
//
//   aaaa|a000| <- padded with 0s to the next size 4 boundary
//
// Useful when you are building some sort of binary format using String_Builder.
NOBDEF void nob_sb_pad_align(Nob_String_Builder *sb, size_t size);

// Append a sized buffer to a string builder
#define nob_sb_append_buf(sb, buf, size) nob_da_append_many(sb, buf, size)

// Append a string view to a string builder
#define nob_sb_append_sv(sb, sv) nob_sb_append_buf((sb), (sv).data, (sv).count)

// Append a NULL-terminated string to a string builder
#define nob_sb_append_cstr(sb, cstr)  \
    do {                              \
        const char *s = (cstr);       \
        size_t n = strlen(s);         \
        nob_da_append_many(sb, s, n); \
    } while (0)

// Append a single NULL character at the end of a string builder. So then you can
// use it a NULL-terminated C string
#define nob_sb_append_null(sb) nob_da_append_many(sb, "", 1)

#define nob_sb_append nob_da_append

// Free the memory allocated by a string builder
#define nob_sb_free(sb) NOB_FREE((sb).items)

// Process handle
#ifdef _WIN32
typedef HANDLE Nob_Proc;
#define NOB_INVALID_PROC INVALID_HANDLE_VALUE
typedef HANDLE Nob_Fd;
#define NOB_INVALID_FD INVALID_HANDLE_VALUE
#else
typedef int Nob_Proc;
#define NOB_INVALID_PROC (-1)
typedef int Nob_Fd;
#define NOB_INVALID_FD (-1)
#endif // _WIN32

NOBDEF Nob_Fd nob_fd_open_for_read(const char *path);
NOBDEF Nob_Fd nob_fd_open_for_write(const char *path);
NOBDEF void nob_fd_close(Nob_Fd fd);

typedef struct {
    Nob_Fd read;
    Nob_Fd write;
} Nob_Pipe;

NOBDEF bool nob_pipe_create(Nob_Pipe *pp);

typedef struct {
    Nob_Proc *items;
    size_t count;
    size_t capacity;
} Nob_Procs;

// Wait until the process has finished
NOBDEF bool nob_proc_wait(Nob_Proc proc);

// Wait until all the processes have finished
NOBDEF bool nob_procs_wait(Nob_Procs procs);

// Wait until all the processes have finished and empty the procs array.
NOBDEF bool nob_procs_flush(Nob_Procs *procs);

// Alias to nob_procs_flush
NOB_DEPRECATED("Use `nob_procs_flush(&procs)` instead.")
NOBDEF bool nob_procs_wait_and_reset(Nob_Procs *procs);

// Append a new process to procs array and if procs.count reaches max_procs_count call nob_procs_wait_and_reset() on it
NOB_DEPRECATED("Use `nob_cmd_run(&cmd, .async = &procs, .max_procs = <integer>)` instead")
NOBDEF bool nob_procs_append_with_flush(Nob_Procs *procs, Nob_Proc proc, size_t max_procs_count);

// A command - the main workhorse of Nob. Nob is all about building commands and running them
typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} Nob_Cmd;

// Options for nob_cmd_run_opt() function.
typedef struct {
    // Run the command asynchronously appending its Nob_Proc to the provided Nob_Procs array
    Nob_Procs *async;
    // Maximum processes allowed in the .async list. Zero implies nob_nprocs().
    size_t max_procs;
    // Do not reset the command after execution.
    bool dont_reset;
    // Redirect stdin to file
    const char *stdin_path;
    // Redirect stdout to file
    const char *stdout_path;
    // Redirect stderr to file
    const char *stderr_path;
} Nob_Cmd_Opt;

// Run the command with options.
NOBDEF bool nob_cmd_run_opt(Nob_Cmd *cmd, Nob_Cmd_Opt opt);

// Command Chains (in Shell Scripting they are know as Pipes)
//
// Usage:
// ```c
// Nob_Cmd cmd = {0};
// Nob_Chain chain = {0};
// if (!nob_chain_begin(&chain)) return 1;
// {
//     nob_cmd_append(&cmd, "echo", "Hello, World");
//     if (!nob_chain_cmd(&chain, &cmd)) return 1;
//
//     nob_cmd_append(&cmd, "rev");
//     if (!nob_chain_cmd(&chain, &cmd)) return 1;
//
//     nob_cmd_append(&cmd, "xxd");
//     if (!nob_chain_cmd(&chain, &cmd)) return 1;
// }
// if (!nob_chain_end(&chain)) return 1;
// ```
//
// The above is equivalent to a shell command:
//
// ```sh
// echo "Hello, World" | rev | xxd
// ```
//
// After nob_chain_end() the Nob_Chain struct can be reused again.
//
// The fields of the Nob_Chain struct contain the intermediate state of the Command
// Chain that is being built with the nob_chain_cmd() calls and generally have no
// particular use for the user.
//
// The only memory dynamically allocated within Nob_Chain belongs to the .cmd field.
// So if you want to clean it all up you can just do free(chain.cmd.items).
typedef struct {
    // The file descriptor of the output of the previous command. Will be used as the input for the next command.
    Nob_Fd fdin;
    // The command from the last nob_chain_cmd() call.
    Nob_Cmd cmd;
    // The value of the optional .err2out parameter from the last nob_chain_cmd() call.
    bool err2out;
} Nob_Chain;

typedef struct {
    const char *stdin_path;
} Nob_Chain_Begin_Opt;
#define nob_chain_begin(chain, ...) nob_chain_begin_opt((chain), NOB_CLIT(Nob_Chain_Begin_Opt) { __VA_ARGS__ })
NOBDEF bool nob_chain_begin_opt(Nob_Chain *chain, Nob_Chain_Begin_Opt opt);

typedef struct {
    bool err2out;
    bool dont_reset;
} Nob_Chain_Cmd_Opt;
#define nob_chain_cmd(chain, cmd, ...) nob_chain_cmd_opt((chain), (cmd), NOB_CLIT(Nob_Chain_Cmd_Opt) { __VA_ARGS__ })
NOBDEF bool nob_chain_cmd_opt(Nob_Chain *chain, Nob_Cmd *cmd, Nob_Chain_Cmd_Opt opt);

typedef struct {
    Nob_Procs *async;
    size_t max_procs;
    const char *stdout_path;
    const char *stderr_path;
} Nob_Chain_End_Opt;
#define nob_chain_end(chain, ...) nob_chain_end_opt((chain), NOB_CLIT(Nob_Chain_End_Opt) { __VA_ARGS__ })
NOBDEF bool nob_chain_end_opt(Nob_Chain *chain, Nob_Chain_End_Opt opt);

// Get amount of processors on the machine.
NOBDEF int nob_nprocs(void);

#define NOB_NANOS_PER_SEC (1000*1000*1000)

// The maximum time span representable is 584 years.
NOBDEF uint64_t nob_nanos_since_unspecified_epoch(void);

// Same as nob_cmd_run_opt but using cool variadic macro to set the default options.
// See https://x.com/vkrajacic/status/1749816169736073295 for more info on how to use such macros.
#define nob_cmd_run(cmd, ...) nob_cmd_run_opt((cmd), NOB_CLIT(Nob_Cmd_Opt){__VA_ARGS__})

// DEPRECATED:
//
// You were suppose to use this structure like this:
//
// ```c
// Nob_Fd fdin = nob_fd_open_for_read("input.txt");
// if (fdin == NOB_INVALID_FD) fail();
// Nob_Fd fdout = nob_fd_open_for_write("output.txt");
// if (fdout == NOB_INVALID_FD) fail();
// Nob_Cmd cmd = {0};
// nob_cmd_append(&cmd, "cat");
// if (!nob_cmd_run_sync_redirect_and_reset(&cmd, (Nob_Cmd_Redirect) {
//     .fdin = &fdin,
//     .fdout = &fdout
// })) fail();
// ```
//
// But these days you should do:
//
// ```c
// Nob_Cmd cmd = {0};
// nob_cmd_append(&cmd, "cat");
// if (!nob_cmd_run(&cmd, .stdin_path = "input.txt", .stdout_path = "output.txt")) fail();
// ```
typedef struct {
    Nob_Fd *fdin;
    Nob_Fd *fdout;
    Nob_Fd *fderr;
} Nob_Cmd_Redirect;

// Render a string representation of a command into a string builder. Keep in mind the the
// string builder is not NULL-terminated by default. Use nob_sb_append_null if you plan to
// use it as a C string.
NOBDEF void nob_cmd_render(Nob_Cmd cmd, Nob_String_Builder *render);

// Compound Literal
#if defined(__cplusplus)
    #define NOB_CLIT(type) type
#else
    #define NOB_CLIT(type) (type)
#endif

NOBDEF void nob__cmd_append(Nob_Cmd *cmd, size_t n, const char **args);
#if defined(__cplusplus)
    template <typename... Args>
    static inline void nob__cpp_cmd_append_wrapper(Nob_Cmd *cmd, Args... strs)
    {
        const char* args[] = { strs... };
        nob__cmd_append(cmd, sizeof(args)/sizeof(args[0]), args);
    }
    #define nob_cmd_append(cmd, ...) nob__cpp_cmd_append_wrapper(cmd, __VA_ARGS__)
#else
    #define nob_cmd_append(cmd, ...) \
        nob__cmd_append(cmd, sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*), (const char*[]){__VA_ARGS__})
#endif // __cplusplus

// TODO: nob_cmd_extend() evaluates other_cmd twice
// It can be fixed by turning nob_cmd_extend() call into a statement.
// But that may break backward compatibility of the API.
#define nob_cmd_extend(cmd, other_cmd) \
    nob_da_append_many(cmd, (other_cmd)->items, (other_cmd)->count)

// Free all the memory allocated by command arguments
#define nob_cmd_free(cmd) NOB_FREE(cmd.items)

// Run command asynchronously
NOB_DEPRECATED("Use `nob_cmd_run(&cmd, .async = &procs, .dont_reset = true)`.")
NOBDEF Nob_Proc nob_cmd_run_async(Nob_Cmd cmd);

// nob_cmd_run_async_and_reset() is just like nob_cmd_run_async() except it also resets cmd.count to 0
// so the Nob_Cmd instance can be seamlessly used several times in a row
NOB_DEPRECATED("Use `nob_cmd_run(&cmd, .async = &procs)` intead.")
NOBDEF Nob_Proc nob_cmd_run_async_and_reset(Nob_Cmd *cmd);

// Run redirected command asynchronously
NOB_DEPRECATED("Use `nob_cmd_run(&cmd, "
               ".async = &procs, "
               ".stdin_path = \"path/to/stdin\", "
               ".stdout_path = \"path/to/stdout\", "
               ".stderr_path = \"path/to/stderr\", "
               ".dont_reset = true"
               ")` instead.")
NOBDEF Nob_Proc nob_cmd_run_async_redirect(Nob_Cmd cmd, Nob_Cmd_Redirect redirect);

// Run redirected command asynchronously and set cmd.count to 0 and close all the opened files
NOB_DEPRECATED("Use `nob_cmd_run(&cmd, "
               ".async = &procs, "
               ".stdin_path = \"path/to/stdin\", "
               ".stdout_path = \"path/to/stdout\", "
               ".stderr_path = \"path/to/stderr\")` instead.")
NOBDEF Nob_Proc nob_cmd_run_async_redirect_and_reset(Nob_Cmd *cmd, Nob_Cmd_Redirect redirect);

// Run command synchronously
NOB_DEPRECATED("Use `nob_cmd_run(&cmd, .dont_reset = true)` instead.")
NOBDEF bool nob_cmd_run_sync(Nob_Cmd cmd);

// NOTE: nob_cmd_run_sync_and_reset() is just like nob_cmd_run_sync() except it also resets cmd.count to 0
// so the Nob_Cmd instance can be seamlessly used several times in a row
NOB_DEPRECATED("Use `nob_cmd_run(&cmd)` instead.")
NOBDEF bool nob_cmd_run_sync_and_reset(Nob_Cmd *cmd);

// Run redirected command synchronously
NOB_DEPRECATED("Use `nob_cmd_run(&cmd, "
               ".stdin_path  = \"path/to/stdin\", "
               ".stdout_path = \"path/to/stdout\", "
               ".stderr_path = \"path/to/stderr\", "
               ".dont_reset = true"
               ")` instead.")
NOBDEF bool nob_cmd_run_sync_redirect(Nob_Cmd cmd, Nob_Cmd_Redirect redirect);

// Run redirected command synchronously and set cmd.count to 0 and close all the opened files
NOB_DEPRECATED("Use `nob_cmd_run(&cmd, "
               ".stdin_path = \"path/to/stdin\", "
               ".stdout_path = \"path/to/stdout\", "
               ".stderr_path = \"path/to/stderr\")` instead.")
NOBDEF bool nob_cmd_run_sync_redirect_and_reset(Nob_Cmd *cmd, Nob_Cmd_Redirect redirect);

#ifndef NOB_TEMP_CAPACITY
#define NOB_TEMP_CAPACITY (8*1024*1024)
#endif // NOB_TEMP_CAPACITY
NOBDEF char *nob_temp_strdup(const char *cstr);
NOBDEF char *nob_temp_strndup(const char *cstr, size_t size);
NOBDEF void *nob_temp_alloc(size_t size);
NOBDEF char *nob_temp_sprintf(const char *format, ...) NOB_PRINTF_FORMAT(1, 2);
NOBDEF char *nob_temp_vsprintf(const char *format, va_list ap);
// nob_temp_reset() - Resets the entire temporary storage to 0.
//
// It is generally not recommended to call this function ever. What you usually want to do is let's say you have a loop,
// that allocates some temporary objects and cleans them up at the end of each iteration. You should use
// nob_temp_save() and nob_temp_rewind() to organize such loop like this:
//
// ```c
// char *message = nob_temp_sprintf("This message is still valid after the loop below");
// while (!quit) {
//     size_t mark = nob_temp_save();
//     nob_temp_alloc(69);
//     nob_temp_alloc(420);
//     nob_temp_alloc(1337);
//     nob_temp_rewind(mark);
// }
// printf("%s\n", message);
// ```
//
// That way all the temporary allocations created before the loop are still valid even after the loop.
// Such save/rewind blocks define lifetime boundaries of the temporary objects which also could be nested.
// This turns the temporary storage into kind of a second stack with a more manual management.
NOBDEF void nob_temp_reset(void);
NOBDEF size_t nob_temp_save(void);
NOBDEF void nob_temp_rewind(size_t checkpoint);

// Given any path returns the last part of that path.
// "/path/to/a/file.c" -> "file.c"; "/path/to/a/directory" -> "directory"
NOBDEF const char *nob_path_name(const char *path);
NOBDEF bool nob_rename(const char *old_path, const char *new_path);
NOBDEF int nob_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);
NOBDEF int nob_needs_rebuild1(const char *output_path, const char *input_path);
NOBDEF int nob_file_exists(const char *file_path);
NOBDEF const char *nob_get_current_dir_temp(void);
NOBDEF bool nob_set_current_dir(const char *path);
// Returns you the directory part of the path allocated on the temporary storage.
NOBDEF char *nob_temp_dir_name(const char *path);
NOBDEF char *nob_temp_file_name(const char *path);
NOBDEF char *nob_temp_file_ext(const char *path);
NOBDEF char *nob_temp_running_executable_path(void);

// TODO: we should probably document somewhere all the compilers we support

// The nob_cc_* macros try to abstract away the specific compiler.
// They are verify basic and not particularly flexible, but you can redefine them if you need to
// or not use them at all and create your own abstraction on top of Nob_Cmd.

#ifndef nob_cc
#  if _WIN32
#    if defined(__GNUC__)
#       define nob_cc(cmd) nob_cmd_append(cmd, "cc")
#    elif defined(__clang__)
#       define nob_cc(cmd) nob_cmd_append(cmd, "clang")
#    elif defined(_MSC_VER)
#       define nob_cc(cmd) nob_cmd_append(cmd, "cl.exe")
#    elif defined(__TINYC__)
#       define nob_cc(cmd) nob_cmd_append(cmd, "tcc")
#    endif
#  else
#    define nob_cc(cmd) nob_cmd_append(cmd, "cc")
#  endif
#endif // nob_cc

#ifndef nob_cc_flags
#  if defined(_MSC_VER) && !defined(__clang__)
#    define nob_cc_flags(cmd) nob_cmd_append(cmd, "/W4", "/nologo", "/D_CRT_SECURE_NO_WARNINGS")
#  else
#    define nob_cc_flags(cmd) nob_cmd_append(cmd, "-Wall", "-Wextra")
#  endif
#endif // nob_cc_flags

#ifndef nob_cc_output
#  if defined(_MSC_VER) && !defined(__clang__)
#    define nob_cc_output(cmd, output_path) nob_cmd_append(cmd, nob_temp_sprintf("/Fe:%s", (output_path)), nob_temp_sprintf("/Fo:%s", (output_path)))
#  else
#    define nob_cc_output(cmd, output_path) nob_cmd_append(cmd, "-o", (output_path))
#  endif
#endif // nob_cc_output

#ifndef nob_cc_inputs
#  define nob_cc_inputs(cmd, ...) nob_cmd_append(cmd, __VA_ARGS__)
#endif // nob_cc_inputs

// TODO: add MinGW support for Go Rebuild Urself™ Technology and all the nob_cc_* macros above
//   Musializer contributors came up with a pretty interesting idea of an optional prefix macro which could be useful for
//   MinGW support:
//   https://github.com/tsoding/musializer/blob/b7578cc76b9ecb573d239acc9ccf5a04d3aba2c9/src_build/nob_win64_mingw.c#L3-L9
// TODO: Maybe instead NOB_REBUILD_URSELF macro, the Go Rebuild Urself™ Technology should use the
//   user defined nob_cc_* macros instead?
#ifndef NOB_REBUILD_URSELF
#  if defined(_WIN32)
#    if defined(__clang__)
#      if defined(__cplusplus)
#        define NOB_REBUILD_URSELF(binary_path, source_path) "clang", "-x", "c++", "-o", binary_path, source_path
#      else
#        define NOB_REBUILD_URSELF(binary_path, source_path) "clang", "-x", "c", "-o", binary_path, source_path
#      endif
#    elif defined(__GNUC__)
#      if defined(__cplusplus)
#        define NOB_REBUILD_URSELF(binary_path, source_path) "gcc", "-x", "c++", "-o", binary_path, source_path
#      else
#        define NOB_REBUILD_URSELF(binary_path, source_path) "gcc", "-x", "c", "-o", binary_path, source_path
#      endif
#    elif defined(_MSC_VER)
#       define NOB_REBUILD_URSELF(binary_path, source_path) "cl.exe", nob_temp_sprintf("/Fe:%s", (binary_path)), source_path
#    elif defined(__TINYC__)
#       define NOB_REBUILD_URSELF(binary_path, source_path) "tcc", "-o", binary_path, source_path
#    endif
#  else
#    if defined(__cplusplus)
#      define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-x", "c++", "-o", binary_path, source_path
#    else
#      define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-x", "c", "-o", binary_path, source_path
#    endif
#  endif
#endif

// Go Rebuild Urself™ Technology
//
//   How to use it:
//     int main(int argc, char** argv) {
//         NOB_GO_REBUILD_URSELF(argc, argv);
//         // actual work
//         return 0;
//     }
//
//   After you added this macro every time you run ./nob it will detect
//   that you modified its original source code and will try to rebuild itself
//   before doing any actual work. So you only need to bootstrap your build system
//   once.
//
//   The modification is detected by comparing the last modified times of the executable
//   and its source code. The same way the make utility usually does it.
//
//   The rebuilding is done by using the NOB_REBUILD_URSELF macro which you can redefine
//   if you need a special way of bootstraping your build system. (which I personally
//   do not recommend since the whole idea of NoBuild is to keep the process of bootstrapping
//   as simple as possible and doing all of the actual work inside of ./nob)
//
NOBDEF void nob__go_rebuild_urself(int argc, char **argv, const char *source_path, ...);
#define NOB_GO_REBUILD_URSELF(argc, argv) nob__go_rebuild_urself(argc, argv, __FILE__, NULL)
// Sometimes your nob.c includes additional files, so you want the Go Rebuild Urself™ Technology to check
// if they also were modified and rebuild nob.c accordingly. For that we have NOB_GO_REBUILD_URSELF_PLUS():
// ```c
// #define NOB_IMPLEMENTATION
// #include "nob.h"
//
// #include "foo.c"
// #include "bar.c"
//
// int main(int argc, char **argv)
// {
//     NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "foo.c", "bar.c");
//     // ...
//     return 0;
// }
#define NOB_GO_REBUILD_URSELF_PLUS(argc, argv, ...) nob__go_rebuild_urself(argc, argv, __FILE__, __VA_ARGS__, NULL);

typedef struct {
    size_t count;
    const char *data;
} Nob_String_View;

NOBDEF const char *nob_temp_sv_to_cstr(Nob_String_View sv);

NOBDEF Nob_String_View nob_sv_chop_while(Nob_String_View *sv, int (*p)(int x));
NOBDEF Nob_String_View nob_sv_chop_by_delim(Nob_String_View *sv, char delim);
NOBDEF Nob_String_View nob_sv_chop_left(Nob_String_View *sv, size_t n);
NOBDEF Nob_String_View nob_sv_chop_right(Nob_String_View *sv, size_t n);
// If `sv` starts with `prefix` chops off the prefix and returns true.
// Otherwise, leaves `sv` unmodified and returns false.
NOBDEF bool nob_sv_chop_prefix(Nob_String_View *sv, Nob_String_View prefix);
// If `sv` ends with `suffix` chops off the suffix and returns true.
// Otherwise, leaves `sv` unmodified and returns false.
NOBDEF bool nob_sv_chop_suffix(Nob_String_View *sv, Nob_String_View suffix);
NOBDEF Nob_String_View nob_sv_trim(Nob_String_View sv);
NOBDEF Nob_String_View nob_sv_trim_left(Nob_String_View sv);
NOBDEF Nob_String_View nob_sv_trim_right(Nob_String_View sv);
NOBDEF bool nob_sv_eq(Nob_String_View a, Nob_String_View b);
NOB_DEPRECATED("Use nob_sv_ends_with_cstr(sv, suffix) instead. "
               "Pay attention to the `s` at the end of the `end`. "
               "The reason this function was deprecated is because "
               "of the typo in the name, of course, but also "
               "because the second argument was a NULL-terminated string "
               "while nob_sv_starts_with() accepted Nob_String_View as the "
               "prefix which created an inconsistency in the API.")
NOBDEF bool nob_sv_end_with(Nob_String_View sv, const char *cstr);
NOBDEF bool nob_sv_ends_with_cstr(Nob_String_View sv, const char *cstr);
NOBDEF bool nob_sv_ends_with(Nob_String_View sv, Nob_String_View suffix);
NOBDEF bool nob_sv_starts_with(Nob_String_View sv, Nob_String_View prefix);
NOBDEF Nob_String_View nob_sv_from_cstr(const char *cstr);
NOBDEF Nob_String_View nob_sv_from_parts(const char *data, size_t count);
// nob_sb_to_sv() enables you to just view Nob_String_Builder as Nob_String_View
#define nob_sb_to_sv(sb) nob_sv_from_parts((sb).items, (sb).count)

// printf macros for String_View
#ifndef SV_Fmt
#define SV_Fmt "%.*s"
#endif // SV_Fmt
#ifndef SV_Arg
#define SV_Arg(sv) (int) (sv).count, (sv).data
#endif // SV_Arg
// USAGE:
//   String_View name = ...;
//   printf("Name: "SV_Fmt"\n", SV_Arg(name));

#ifdef _WIN32

NOBDEF char *nob_win32_error_message(DWORD err);

#endif // _WIN32

#endif // NOB_H_

#ifdef NOB_IMPLEMENTATION

// This is like nob_proc_wait() but waits asynchronously. Depending on the platform ms means different thing.
// On Windows it means timeout. On POSIX it means for how long to sleep after checking if the process exited,
// so to not peg the core too much. Since this API is kinda of weird, the function is private for now.
static int nob__proc_wait_async(Nob_Proc proc, int ms);

// Starts the process for the command. Its main purpose is to be the base for nob_cmd_run() and nob_cmd_run_opt().
static Nob_Proc nob__cmd_start_process(Nob_Cmd cmd, Nob_Fd *fdin, Nob_Fd *fdout, Nob_Fd *fderr);

// Any messages with the level below nob_minimal_log_level are going to be suppressed.
Nob_Log_Level nob_minimal_log_level = NOB_INFO;

NOBDEF void nob__cmd_append(Nob_Cmd *cmd, size_t n, const char **args)
{
    for (size_t i = 0; i < n; ++i) {
        nob_da_append(cmd, args[i]);
    }
}

#ifdef _WIN32

// Base on https://stackoverflow.com/a/75644008
// > .NET Core uses 4096 * sizeof(WCHAR) buffer on stack for FormatMessageW call. And...thats it.
// >
// > https://github.com/dotnet/runtime/blob/3b63eb1346f1ddbc921374a5108d025662fb5ffd/src/coreclr/utilcode/posterror.cpp#L264-L265
#ifndef NOB_WIN32_ERR_MSG_SIZE
#define NOB_WIN32_ERR_MSG_SIZE (4 * 1024)
#endif // NOB_WIN32_ERR_MSG_SIZE

NOBDEF char *nob_win32_error_message(DWORD err) {
    static char win32ErrMsg[NOB_WIN32_ERR_MSG_SIZE] = {0};
    DWORD errMsgSize = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, LANG_USER_DEFAULT, win32ErrMsg,
                                      NOB_WIN32_ERR_MSG_SIZE, NULL);

    if (errMsgSize == 0) {
        if (GetLastError() != ERROR_MR_MID_NOT_FOUND) {
            if (sprintf(win32ErrMsg, "Could not get error message for 0x%lX", err) > 0) {
                return (char *)&win32ErrMsg;
            } else {
                return NULL;
            }
        } else {
            if (sprintf(win32ErrMsg, "Invalid Windows Error code (0x%lX)", err) > 0) {
                return (char *)&win32ErrMsg;
            } else {
                return NULL;
            }
        }
    }

    while (errMsgSize > 1 && isspace(win32ErrMsg[errMsgSize - 1])) {
        win32ErrMsg[--errMsgSize] = '\0';
    }

    return win32ErrMsg;
}

#endif // _WIN32

// The implementation idea is stolen from https://github.com/zhiayang/nabs
NOBDEF void nob__go_rebuild_urself(int argc, char **argv, const char *source_path, ...)
{
    const char *binary_path = nob_shift(argv, argc);
#ifdef _WIN32
    // On Windows executables almost always invoked without extension, so
    // it's ./nob, not ./nob.exe. For renaming the extension is a must.
    if (!nob_sv_ends_with_cstr(nob_sv_from_cstr(binary_path), ".exe")) {
        binary_path = nob_temp_sprintf("%s.exe", binary_path);
    }
#endif

    Nob_File_Paths source_paths = {0};
    nob_da_append(&source_paths, source_path);
    va_list args;
    va_start(args, source_path);
    for (;;) {
        const char *path = va_arg(args, const char*);
        if (path == NULL) break;
        nob_da_append(&source_paths, path);
    }
    va_end(args);

    int rebuild_is_needed = nob_needs_rebuild(binary_path, source_paths.items, source_paths.count);
    if (rebuild_is_needed < 0) exit(1); // error
    if (!rebuild_is_needed) {           // no rebuild is needed
        NOB_FREE(source_paths.items);
        return;
    }

    Nob_Cmd cmd = {0};

    const char *old_binary_path = nob_temp_sprintf("%s.old", binary_path);

    if (!nob_rename(binary_path, old_binary_path)) exit(1);
    nob_cmd_append(&cmd, NOB_REBUILD_URSELF(binary_path, source_path));
    Nob_Cmd_Opt opt = {0};
    if (!nob_cmd_run_opt(&cmd, opt)) {
        nob_rename(old_binary_path, binary_path);
        exit(1);
    }
#ifdef NOB_EXPERIMENTAL_DELETE_OLD
    // TODO: this is an experimental behavior behind a compilation flag.
    // Once it is confirmed that it does not cause much problems on both POSIX and Windows
    // we may turn it on by default.
    nob_delete_file(old_binary_path);
#endif // NOB_EXPERIMENTAL_DELETE_OLD

    nob_cmd_append(&cmd, binary_path);
    nob_da_append_many(&cmd, argv, argc);
    if (!nob_cmd_run_opt(&cmd, opt)) exit(1);
    exit(0);
}

static size_t nob_temp_size = 0;
static char nob_temp[NOB_TEMP_CAPACITY] = {0};

NOBDEF bool nob_mkdir_if_not_exists(const char *path)
{
#ifdef _WIN32
    int result = _mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif
    if (result < 0) {
        if (errno == EEXIST) {
#ifndef NOB_NO_ECHO
            nob_log(NOB_INFO, "directory `%s` already exists", path);
#endif // NOB_NO_ECHO
            return true;
        }
        nob_log(NOB_ERROR, "could not create directory `%s`: %s", path, strerror(errno));
        return false;
    }

#ifndef NOB_NO_ECHO
    nob_log(NOB_INFO, "created directory `%s`", path);
#endif // NOB_NO_ECHO
    return true;
}

NOBDEF bool nob_copy_file(const char *src_path, const char *dst_path)
{
#ifndef NOB_NO_ECHO
    nob_log(NOB_INFO, "copying %s -> %s", src_path, dst_path);
#endif // NOB_NO_ECHO
#ifdef _WIN32
    if (!CopyFile(src_path, dst_path, FALSE)) {
        nob_log(NOB_ERROR, "Could not copy file: %s", nob_win32_error_message(GetLastError()));
        return false;
    }
    return true;
#else
    int src_fd = -1;
    int dst_fd = -1;
    size_t buf_size = 32*1024;
    char *buf = (char*)NOB_REALLOC(NULL, buf_size);
    NOB_ASSERT(buf != NULL && "Buy more RAM lol!!");
    bool result = true;

    src_fd = open(src_path, O_RDONLY);
    if (src_fd < 0) {
        nob_log(NOB_ERROR, "Could not open file %s: %s", src_path, strerror(errno));
        nob_return_defer(false);
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        nob_log(NOB_ERROR, "Could not get mode of file %s: %s", src_path, strerror(errno));
        nob_return_defer(false);
    }

    dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) {
        nob_log(NOB_ERROR, "Could not create file %s: %s", dst_path, strerror(errno));
        nob_return_defer(false);
    }

    for (;;) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) break;
        if (n < 0) {
            nob_log(NOB_ERROR, "Could not read from file %s: %s", src_path, strerror(errno));
            nob_return_defer(false);
        }
        char *buf2 = buf;
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) {
                nob_log(NOB_ERROR, "Could not write to file %s: %s", dst_path, strerror(errno));
                nob_return_defer(false);
            }
            n    -= m;
            buf2 += m;
        }
    }

defer:
    NOB_FREE(buf);
    close(src_fd);
    close(dst_fd);
    return result;
#endif
}

NOBDEF void nob_cmd_render(Nob_Cmd cmd, Nob_String_Builder *render)
{
    for (size_t i = 0; i < cmd.count; ++i) {
        const char *arg = cmd.items[i];
        if (arg == NULL) break;
        if (i > 0) nob_sb_append_cstr(render, " ");
        if (!strchr(arg, ' ')) {
            nob_sb_append_cstr(render, arg);
        } else {
            nob_da_append(render, '\'');
            nob_sb_append_cstr(render, arg);
            nob_da_append(render, '\'');
        }
    }
}

#ifdef _WIN32
// https://learn.microsoft.com/en-gb/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way
static void nob__win32_cmd_quote(Nob_Cmd cmd, Nob_String_Builder *quoted)
{
    for (size_t i = 0; i < cmd.count; ++i) {
        const char *arg = cmd.items[i];
        if (arg == NULL) break;
        size_t len = strlen(arg);
        if (i > 0) nob_da_append(quoted, ' ');
        if (len != 0 && NULL == strpbrk(arg, " \t\n\v\"")) {
            // no need to quote
            nob_da_append_many(quoted, arg, len);
        } else {
            // we need to escape:
            // 1. double quotes in the original arg
            // 2. consequent backslashes before a double quote
            size_t backslashes = 0;
            nob_da_append(quoted, '\"');
            for (size_t j = 0; j < len; ++j) {
                char x = arg[j];
                if (x == '\\') {
                    backslashes += 1;
                } else {
                    if (x == '\"') {
                        // escape backslashes (if any) and the double quote
                        for (size_t k = 0; k < 1+backslashes; ++k) {
                            nob_da_append(quoted, '\\');
                        }
                    }
                    backslashes = 0;
                }
                nob_da_append(quoted, x);
            }
            // escape backslashes (if any)
            for (size_t k = 0; k < backslashes; ++k) {
                nob_da_append(quoted, '\\');
            }
            nob_da_append(quoted, '\"');
        }
    }
}
#endif

NOBDEF int nob_nprocs(void)
{
#ifdef _WIN32
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    return siSysInfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

NOBDEF bool nob_cmd_run_opt(Nob_Cmd *cmd, Nob_Cmd_Opt opt)
{
    bool result = true;
    Nob_Fd fdin  = NOB_INVALID_FD;
    Nob_Fd fdout = NOB_INVALID_FD;
    Nob_Fd fderr = NOB_INVALID_FD;
    Nob_Fd *opt_fdin  = NULL;
    Nob_Fd *opt_fdout = NULL;
    Nob_Fd *opt_fderr = NULL;
    Nob_Proc proc = NOB_INVALID_PROC;

    size_t max_procs = opt.max_procs > 0 ? opt.max_procs : (size_t) nob_nprocs() + 1;

    if (opt.async && max_procs > 0) {
        while (opt.async->count >= max_procs) {
            for (size_t i = 0; i < opt.async->count; ++i) {
                int ret = nob__proc_wait_async(opt.async->items[i], 1);
                if (ret < 0) nob_return_defer(false);
                if (ret) {
                    nob_da_remove_unordered(opt.async, i);
                    break;
                }
            }
        }
    }

    if (opt.stdin_path) {
        fdin = nob_fd_open_for_read(opt.stdin_path);
        if (fdin == NOB_INVALID_FD) nob_return_defer(false);
        opt_fdin = &fdin;
    }
    if (opt.stdout_path) {
        fdout = nob_fd_open_for_write(opt.stdout_path);
        if (fdout == NOB_INVALID_FD) nob_return_defer(false);
        opt_fdout = &fdout;
    }
    if (opt.stderr_path) {
        fderr = nob_fd_open_for_write(opt.stderr_path);
        if (fderr == NOB_INVALID_FD) nob_return_defer(false);
        opt_fderr = &fderr;
    }
    proc = nob__cmd_start_process(*cmd, opt_fdin, opt_fdout, opt_fderr);

    if (opt.async) {
        if (proc == NOB_INVALID_PROC) nob_return_defer(false);
        nob_da_append(opt.async, proc);
    } else {
        if (!nob_proc_wait(proc)) nob_return_defer(false);
    }

defer:
    if (opt_fdin)  nob_fd_close(*opt_fdin);
    if (opt_fdout) nob_fd_close(*opt_fdout);
    if (opt_fderr) nob_fd_close(*opt_fderr);
    if (!opt.dont_reset) cmd->count = 0;
    return result;
}

NOBDEF bool nob_chain_begin_opt(Nob_Chain *chain, Nob_Chain_Begin_Opt opt)
{
    chain->cmd.count = 0;
    chain->err2out = false;
    chain->fdin = NOB_INVALID_FD;
    if (opt.stdin_path) {
        chain->fdin = nob_fd_open_for_read(opt.stdin_path);
        if (chain->fdin == NOB_INVALID_FD) return false;
    }
    return true;
}

NOBDEF bool nob_chain_cmd_opt(Nob_Chain *chain, Nob_Cmd *cmd, Nob_Chain_Cmd_Opt opt)
{
    bool result = true;
    Nob_Pipe pp = {0};
    struct {
        size_t count;
        Nob_Fd items[5]; // should be no more than 3, but we allocate 5 just in case
    } fds = {0};

    NOB_ASSERT(cmd->count > 0);

    if (chain->cmd.count != 0) { // not first cmd in the chain
        Nob_Fd *pfdin = NULL;
        if (chain->fdin != NOB_INVALID_FD) {
            nob_fa_append(&fds, chain->fdin);
            pfdin = &chain->fdin;
        }
        if (!nob_pipe_create(&pp)) nob_return_defer(false);
        nob_fa_append(&fds, pp.write);
        Nob_Fd *pfdout = &pp.write;
        Nob_Fd *pfderr = chain->err2out ? pfdout : NULL;

        Nob_Proc proc = nob__cmd_start_process(chain->cmd, pfdin, pfdout, pfderr);
        chain->cmd.count = 0;
        if (proc == NOB_INVALID_PROC) {
            nob_fa_append(&fds, pp.read);
            nob_return_defer(false);
        }
        chain->fdin = pp.read;
    }

    nob_da_append_many(&chain->cmd, cmd->items, cmd->count);
    chain->err2out = opt.err2out;

defer:
    for (size_t i = 0; i < fds.count; ++i) {
        nob_fd_close(fds.items[i]);
    }
    if (!opt.dont_reset) cmd->count = 0;
    return result;
}

static Nob_Fd nob__fd_stdout(void)
{
#ifdef _WIN32
    return GetStdHandle(STD_OUTPUT_HANDLE);
#else
    return STDOUT_FILENO;
#endif // _WIN32
}

NOBDEF bool nob_chain_end_opt(Nob_Chain *chain, Nob_Chain_End_Opt opt)
{
    bool result = true;

    Nob_Fd *pfdin = NULL;
    struct {
        size_t count;
        Nob_Fd items[5]; // should be no more than 3, but we allocate 5 just in case
    } fds = {0};

    if (chain->fdin != NOB_INVALID_FD) {
        nob_fa_append(&fds, chain->fdin);
        pfdin = &chain->fdin;
    }

    if (chain->cmd.count != 0) { // Non-empty chain case
        size_t max_procs = opt.max_procs > 0 ? opt.max_procs : (size_t) nob_nprocs() + 1;

        if (opt.async && max_procs > 0) {
            while (opt.async->count >= max_procs) {
                for (size_t i = 0; i < opt.async->count; ++i) {
                    int ret = nob__proc_wait_async(opt.async->items[i], 1);
                    if (ret < 0) nob_return_defer(false);
                    if (ret) {
                        nob_da_remove_unordered(opt.async, i);
                        break;
                    }
                }
            }
        }

        Nob_Fd fdout = nob__fd_stdout();
        if (opt.stdout_path) {
            fdout = nob_fd_open_for_write(opt.stdout_path);
            if (fdout == NOB_INVALID_FD) nob_return_defer(false);
            nob_fa_append(&fds, fdout);
        }

        Nob_Fd fderr = 0;
        Nob_Fd *pfderr = NULL;
        if (chain->err2out) pfderr = &fdout;
        if (opt.stderr_path) {
            if (pfderr == NULL) {
                fderr = nob_fd_open_for_write(opt.stderr_path);
                if (fderr == NOB_INVALID_FD) nob_return_defer(false);
                nob_fa_append(&fds, fderr);
                pfderr = &fderr;
            } else {
                // There was err2out set for the last command.
                // All the stderr will go to stdout.
                // So the stderr file is going to be empty.
                NOB_ASSERT(chain->err2out);
                if (!nob_write_entire_file(opt.stderr_path, NULL, 0)) nob_return_defer(false);
            }
        }

        Nob_Proc proc = nob__cmd_start_process(chain->cmd, pfdin, &fdout, pfderr);
        chain->cmd.count = 0;

        if (opt.async) {
            if (proc == NOB_INVALID_PROC) nob_return_defer(false);
            nob_da_append(opt.async, proc);
        } else {
            if (!nob_proc_wait(proc)) nob_return_defer(false);
        }
    }

defer:
    for (size_t i = 0; i < fds.count; ++i) {
        nob_fd_close(fds.items[i]);
    }
    return result;
}

// The maximum time span representable is 584 years.
NOBDEF uint64_t nob_nanos_since_unspecified_epoch(void)
{
#ifdef _WIN32
    LARGE_INTEGER Time;
    QueryPerformanceCounter(&Time);

    static LARGE_INTEGER Frequency = {0};
    if (Frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&Frequency);
    }

    uint64_t Secs  = Time.QuadPart / Frequency.QuadPart;
    uint64_t Nanos = Time.QuadPart % Frequency.QuadPart * NOB_NANOS_PER_SEC / Frequency.QuadPart;
    return NOB_NANOS_PER_SEC * Secs + Nanos;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return NOB_NANOS_PER_SEC * ts.tv_sec + ts.tv_nsec;
#endif // _WIN32
}

NOBDEF Nob_Proc nob_cmd_run_async_redirect(Nob_Cmd cmd, Nob_Cmd_Redirect redirect)
{
    return nob__cmd_start_process(cmd, redirect.fdin, redirect.fdout, redirect.fderr);
}

static Nob_Proc nob__cmd_start_process(Nob_Cmd cmd, Nob_Fd *fdin, Nob_Fd *fdout, Nob_Fd *fderr)
{
    if (cmd.count < 1) {
        nob_log(NOB_ERROR, "Could not run empty command");
        return NOB_INVALID_PROC;
    }

#ifndef NOB_NO_ECHO
    Nob_String_Builder sb = {0};
    nob_cmd_render(cmd, &sb);
    nob_sb_append_null(&sb);
    nob_log(NOB_INFO, "CMD: %s", sb.items);
    nob_sb_free(sb);
    memset(&sb, 0, sizeof(sb));
#endif // NOB_NO_ECHO

#ifdef _WIN32
    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError = fderr ? *fderr : GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = fdout ? *fdout : GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = fdin ? *fdin : GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    Nob_String_Builder quoted = {0};
    nob__win32_cmd_quote(cmd, &quoted);
    nob_sb_append_null(&quoted);
    BOOL bSuccess = CreateProcessA(NULL, quoted.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
    nob_sb_free(quoted);

    if (!bSuccess) {
        nob_log(NOB_ERROR, "Could not create child process for %s: %s", cmd.items[0], nob_win32_error_message(GetLastError()));
        return NOB_INVALID_PROC;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        nob_log(NOB_ERROR, "Could not fork child process: %s", strerror(errno));
        return NOB_INVALID_PROC;
    }

    if (cpid == 0) {
        if (fdin) {
            if (dup2(*fdin, STDIN_FILENO) < 0) {
                nob_log(NOB_ERROR, "Could not setup stdin for child process: %s", strerror(errno));
                exit(1);
            }
        }

        if (fdout) {
            if (dup2(*fdout, STDOUT_FILENO) < 0) {
                nob_log(NOB_ERROR, "Could not setup stdout for child process: %s", strerror(errno));
                exit(1);
            }
        }

        if (fderr) {
            if (dup2(*fderr, STDERR_FILENO) < 0) {
                nob_log(NOB_ERROR, "Could not setup stderr for child process: %s", strerror(errno));
                exit(1);
            }
        }

        // NOTE: This leaks a bit of memory in the child process.
        // But do we actually care? It's a one off leak anyway...
        Nob_Cmd cmd_null = {0};
        nob_da_append_many(&cmd_null, cmd.items, cmd.count);
        nob_cmd_append(&cmd_null, (const char*)NULL);

        if (execvp(cmd.items[0], (char * const*) cmd_null.items) < 0) {
            nob_log(NOB_ERROR, "Could not exec child process for %s: %s", cmd.items[0], strerror(errno));
            exit(1);
        }
        NOB_UNREACHABLE("nob_cmd_run_async_redirect");
    }

    return cpid;
#endif
}

NOBDEF Nob_Proc nob_cmd_run_async(Nob_Cmd cmd)
{
    return nob__cmd_start_process(cmd, NULL, NULL, NULL);
}

NOBDEF Nob_Proc nob_cmd_run_async_and_reset(Nob_Cmd *cmd)
{
    Nob_Proc proc = nob__cmd_start_process(*cmd, NULL, NULL, NULL);
    cmd->count = 0;
    return proc;
}

NOBDEF Nob_Proc nob_cmd_run_async_redirect_and_reset(Nob_Cmd *cmd, Nob_Cmd_Redirect redirect)
{
    Nob_Proc proc = nob__cmd_start_process(*cmd, redirect.fdin, redirect.fdout, redirect.fderr);
    cmd->count = 0;
    if (redirect.fdin) {
        nob_fd_close(*redirect.fdin);
        *redirect.fdin = NOB_INVALID_FD;
    }
    if (redirect.fdout) {
        nob_fd_close(*redirect.fdout);
        *redirect.fdout = NOB_INVALID_FD;
    }
    if (redirect.fderr) {
        nob_fd_close(*redirect.fderr);
        *redirect.fderr = NOB_INVALID_FD;
    }
    return proc;
}

NOBDEF Nob_Fd nob_fd_open_for_read(const char *path)
{
#ifndef _WIN32
    Nob_Fd result = open(path, O_RDONLY);
    if (result < 0) {
        nob_log(NOB_ERROR, "Could not open file %s: %s", path, strerror(errno));
        return NOB_INVALID_FD;
    }
    return result;
#else
    // https://docs.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    Nob_Fd result = CreateFile(
                    path,
                    GENERIC_READ,
                    0,
                    &saAttr,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_READONLY,
                    NULL);

    if (result == INVALID_HANDLE_VALUE) {
        nob_log(NOB_ERROR, "Could not open file %s: %s", path, nob_win32_error_message(GetLastError()));
        return NOB_INVALID_FD;
    }

    return result;
#endif // _WIN32
}

NOBDEF Nob_Fd nob_fd_open_for_write(const char *path)
{
#ifndef _WIN32
    Nob_Fd result = open(path,
                     O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (result < 0) {
        nob_log(NOB_ERROR, "could not open file %s: %s", path, strerror(errno));
        return NOB_INVALID_FD;
    }
    return result;
#else
    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    Nob_Fd result = CreateFile(
                    path,                            // name of the write
                    GENERIC_WRITE,                   // open for writing
                    0,                               // do not share
                    &saAttr,                         // default security
                    CREATE_ALWAYS,                   // create always
                    FILE_ATTRIBUTE_NORMAL,           // normal file
                    NULL                             // no attr. template
                );

    if (result == INVALID_HANDLE_VALUE) {
        nob_log(NOB_ERROR, "Could not open file %s: %s", path, nob_win32_error_message(GetLastError()));
        return NOB_INVALID_FD;
    }

    return result;
#endif // _WIN32
}

NOBDEF void nob_fd_close(Nob_Fd fd)
{
#ifdef _WIN32
    CloseHandle(fd);
#else
    close(fd);
#endif // _WIN32
}

NOBDEF bool nob_pipe_create(Nob_Pipe *pp)
{
#ifdef _WIN32
    // https://docs.microsoft.com/en-us/windows/win32/ProcThread/creating-a-child-process-with-redirected-input-and-output

    SECURITY_ATTRIBUTES saAttr = {0};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    if (!CreatePipe(&pp->read, &pp->write, &saAttr, 0)) {
        nob_log(NOB_ERROR, "Could not create pipe: %s", nob_win32_error_message(GetLastError()));
        return false;
    }

    return true;
#else
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        nob_log(NOB_ERROR, "Could not create pipe: %s\n", strerror(errno));
        return false;
    }

    pp->read  = pipefd[0];
    pp->write = pipefd[1];

    return true;
#endif // _WIN32
}

NOBDEF bool nob_procs_wait(Nob_Procs procs)
{
    bool success = true;
    for (size_t i = 0; i < procs.count; ++i) {
        success = nob_proc_wait(procs.items[i]) && success;
    }
    return success;
}

NOBDEF bool nob_procs_flush(Nob_Procs *procs)
{
    bool success = nob_procs_wait(*procs);
    procs->count = 0;
    return success;
}

NOBDEF bool nob_procs_wait_and_reset(Nob_Procs *procs)
{
    return nob_procs_flush(procs);
}

NOBDEF bool nob_proc_wait(Nob_Proc proc)
{
    if (proc == NOB_INVALID_PROC) return false;

#ifdef _WIN32
    DWORD result = WaitForSingleObject(
                       proc,    // HANDLE hHandle,
                       INFINITE // DWORD  dwMilliseconds
                   );

    if (result == WAIT_FAILED) {
        nob_log(NOB_ERROR, "could not wait on child process: %s", nob_win32_error_message(GetLastError()));
        return false;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        nob_log(NOB_ERROR, "could not get process exit code: %s", nob_win32_error_message(GetLastError()));
        return false;
    }

    if (exit_status != 0) {
        nob_log(NOB_ERROR, "command exited with exit code %lu", exit_status);
        return false;
    }

    CloseHandle(proc);

    return true;
#else
    for (;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) {
            nob_log(NOB_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
            return false;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                nob_log(NOB_ERROR, "command exited with exit code %d", exit_status);
                return false;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            nob_log(NOB_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
            return false;
        }
    }

    return true;
#endif
}

static int nob__proc_wait_async(Nob_Proc proc, int ms)
{
    if (proc == NOB_INVALID_PROC) return false;

#ifdef _WIN32
    DWORD result = WaitForSingleObject(
                       proc,    // HANDLE hHandle,
                       ms       // DWORD  dwMilliseconds
                   );

    if (result == WAIT_TIMEOUT) {
        return 0;
    }

    if (result == WAIT_FAILED) {
        nob_log(NOB_ERROR, "could not wait on child process: %s", nob_win32_error_message(GetLastError()));
        return -1;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        nob_log(NOB_ERROR, "could not get process exit code: %s", nob_win32_error_message(GetLastError()));
        return -1;
    }

    if (exit_status != 0) {
        nob_log(NOB_ERROR, "command exited with exit code %lu", exit_status);
        return -1;
    }

    CloseHandle(proc);

    return 1;
#else
    long ns = ms*1000*1000;
    struct timespec duration = {
        .tv_sec = ns/(1000*1000*1000),
        .tv_nsec = ns%(1000*1000*1000),
    };

    int wstatus = 0;
    pid_t pid = waitpid(proc, &wstatus, WNOHANG);
    if (pid < 0) {
        nob_log(NOB_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
        return -1;
    }

    if (pid == 0) {
        nanosleep(&duration, NULL);
        return 0;
    }

    if (WIFEXITED(wstatus)) {
        int exit_status = WEXITSTATUS(wstatus);
        if (exit_status != 0) {
            nob_log(NOB_ERROR, "command exited with exit code %d", exit_status);
            return -1;
        }

        return 1;
    }

    if (WIFSIGNALED(wstatus)) {
        nob_log(NOB_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
        return -1;
    }

    nanosleep(&duration, NULL);
    return 0;
#endif
}

NOBDEF bool nob_procs_append_with_flush(Nob_Procs *procs, Nob_Proc proc, size_t max_procs_count)
{
    nob_da_append(procs, proc);

    if (procs->count >= max_procs_count) {
        if (!nob_procs_flush(procs)) return false;
    }

    return true;
}

NOBDEF bool nob_cmd_run_sync_redirect(Nob_Cmd cmd, Nob_Cmd_Redirect redirect)
{
    Nob_Proc p = nob__cmd_start_process(cmd, redirect.fdin, redirect.fdout, redirect.fderr);
    return nob_proc_wait(p);
}

NOBDEF bool nob_cmd_run_sync(Nob_Cmd cmd)
{
    Nob_Proc p = nob__cmd_start_process(cmd, NULL, NULL, NULL);
    return nob_proc_wait(p);
}

NOBDEF bool nob_cmd_run_sync_and_reset(Nob_Cmd *cmd)
{
    Nob_Proc p = nob__cmd_start_process(*cmd, NULL, NULL, NULL);
    cmd->count = 0;
    return nob_proc_wait(p);
}

NOBDEF bool nob_cmd_run_sync_redirect_and_reset(Nob_Cmd *cmd, Nob_Cmd_Redirect redirect)
{
    Nob_Proc p = nob__cmd_start_process(*cmd, redirect.fdin, redirect.fdout, redirect.fderr);
    cmd->count = 0;
    if (redirect.fdin) {
        nob_fd_close(*redirect.fdin);
        *redirect.fdin = NOB_INVALID_FD;
    }
    if (redirect.fdout) {
        nob_fd_close(*redirect.fdout);
        *redirect.fdout = NOB_INVALID_FD;
    }
    if (redirect.fderr) {
        nob_fd_close(*redirect.fderr);
        *redirect.fderr = NOB_INVALID_FD;
    }
    return nob_proc_wait(p);
}

static Nob_Log_Handler *nob__log_handler = &nob_default_log_handler;

NOBDEF void nob_set_log_handler(Nob_Log_Handler *handler)
{
    nob__log_handler = handler;
}

NOBDEF Nob_Log_Handler *nob_get_log_handler(void)
{
    return nob__log_handler;
}

NOBDEF void nob_default_log_handler(Nob_Log_Level level, const char *fmt, va_list args)
{
    if (level < nob_minimal_log_level) return;

    switch (level) {
    case NOB_INFO:
        fprintf(stderr, "[INFO] ");
        break;
    case NOB_WARNING:
        fprintf(stderr, "[WARNING] ");
        break;
    case NOB_ERROR:
        fprintf(stderr, "[ERROR] ");
        break;
    case NOB_NO_LOGS: return;
    default:
        NOB_UNREACHABLE("Nob_Log_Level");
    }

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

NOBDEF void nob_null_log_handler(Nob_Log_Level level, const char *fmt, va_list args)
{
    NOB_UNUSED(level);
    NOB_UNUSED(fmt);
    NOB_UNUSED(args);
}

NOBDEF void nob_cancer_log_handler(Nob_Log_Level level, const char *fmt, va_list args)
{
    switch (level) {
    case NOB_INFO:
        fprintf(stderr, "ℹ️ \x1b[36m[INFO]\x1b[0m ");
        break;
    case NOB_WARNING:
        fprintf(stderr, "⚠️ \x1b[33m[WARNING]\x1b[0m ");
        break;
    case NOB_ERROR:
        fprintf(stderr, "🚨 \x1b[31m[ERROR]\x1b[0m ");
        break;
    case NOB_NO_LOGS: return;
    default:
        NOB_UNREACHABLE("Nob_Log_Level");
    }

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

NOBDEF void nob_log(Nob_Log_Level level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nob__log_handler(level, fmt, args);
    va_end(args);
}

NOBDEF bool nob_dir_entry_open(const char *dir_path, Nob_Dir_Entry *dir)
{
    memset(dir, 0, sizeof(*dir));
#ifdef _WIN32
    size_t temp_mark = nob_temp_save();
    char *buffer = nob_temp_sprintf("%s\\*", dir_path);
    dir->nob__private.win32_hFind = FindFirstFile(buffer, &dir->nob__private.win32_data);
    nob_temp_rewind(temp_mark);

    if (dir->nob__private.win32_hFind == INVALID_HANDLE_VALUE) {
        nob_log(NOB_ERROR, "Could not open directory %s: %s", dir_path, nob_win32_error_message(GetLastError()));
        dir->error = true;
        return false;
    }
#else
    dir->nob__private.posix_dir = opendir(dir_path);
    if (dir->nob__private.posix_dir == NULL) {
        nob_log(NOB_ERROR, "Could not open directory %s: %s", dir_path, strerror(errno));
        dir->error = true;
        return false;
    }
#endif // _WIN32
    return true;
}

NOBDEF bool nob_dir_entry_next(Nob_Dir_Entry *dir)
{
#ifdef _WIN32
    if (!dir->nob__private.win32_init) {
        dir->nob__private.win32_init = true;
        dir->name = dir->nob__private.win32_data.cFileName;
        return true;
    }

    if (!FindNextFile(dir->nob__private.win32_hFind, &dir->nob__private.win32_data)) {
        if (GetLastError() == ERROR_NO_MORE_FILES) return false;
        nob_log(NOB_ERROR, "Could not read next directory entry: %s", nob_win32_error_message(GetLastError()));
        dir->error = true;
        return false;
    }
    dir->name = dir->nob__private.win32_data.cFileName;
#else
    errno = 0;
    dir->nob__private.posix_ent = readdir(dir->nob__private.posix_dir);
    if (dir->nob__private.posix_ent == NULL) {
        if (errno == 0) return false;
        nob_log(NOB_ERROR, "Could not read next directory entry: %s", strerror(errno));
        dir->error = true;
        return false;
    }
    dir->name = dir->nob__private.posix_ent->d_name;
#endif // _WIN32
    return true;
}

NOBDEF void nob_dir_entry_close(Nob_Dir_Entry dir)
{
#ifdef _WIN32
    FindClose(dir.nob__private.win32_hFind);
#else
    if (dir.nob__private.posix_dir) closedir(dir.nob__private.posix_dir);
#endif
}

// On the moment of entering `nob__walk_dir_opt_impl()`, the `file_path` Nob_String_Builder is expected to be NULL-terminated.
// So you can freely pass `file_path->items` to functions that expect NULL-terminated file path.
// On existing `nob__walk_dir_opt_impl()` is expected to restore the original content of `file_path`
bool nob__walk_dir_opt_impl(Nob_String_Builder *file_path, Nob_Walk_Func func, size_t level, bool *stop, Nob_Walk_Dir_Opt opt)
{
    NOB_ASSERT(file_path->count > 0 && "file_path was probably not properly NULL-terminated");
    bool result = true;

    Nob_Dir_Entry dir = {0};
    size_t saved_file_path_count = file_path->count;
    Nob_Walk_Action action = NOB_WALK_CONT;

    Nob_File_Type file_type = nob_get_file_type(file_path->items);
    if (file_type < 0) nob_return_defer(false);

    // Pre-order walking
    if (!opt.post_order) {
        if (!func(NOB_CLIT(Nob_Walk_Entry) {
            .path = file_path->items,
            .type = file_type,
            .level = level,
            .data = opt.data,
            .action = &action,
        })) nob_return_defer(false);
        switch (action) {
        case NOB_WALK_CONT: break;
        case NOB_WALK_STOP: *stop = true; // fallthrough
        case NOB_WALK_SKIP: nob_return_defer(true);
        default: NOB_UNREACHABLE("Nob_Walk_Action");
        }
    }

    if (file_type == NOB_FILE_DIRECTORY) {
        if (!nob_dir_entry_open(file_path->items, &dir)) nob_return_defer(false);
        for (;;) {
            // Next entry
            if (!nob_dir_entry_next(&dir)) {
                if (!dir.error) break;
                nob_return_defer(false);
            }

            // Ignore . and ..
            if (strcmp(dir.name, ".")  == 0) continue;
            if (strcmp(dir.name, "..") == 0) continue;

            // Prepare the new file_path
            file_path->count = saved_file_path_count - 1;
#ifdef _WIN32
            nob_sb_appendf(file_path, "\\%s", dir.name);
#else
            nob_sb_appendf(file_path, "/%s", dir.name);
#endif // _WIN32
            nob_sb_append_null(file_path);

            // Recurse
            if (!nob__walk_dir_opt_impl(file_path, func, level+1, stop, opt)) nob_return_defer(false);
            if (*stop) nob_return_defer(true);
        }
        file_path->count = saved_file_path_count;
        nob_da_last(file_path) = '\0';
    }

    // Post-order walking
    if (opt.post_order) {
        if (!func(NOB_CLIT(Nob_Walk_Entry) {
            .path = file_path->items,
            .type = file_type,
            .level = level,
            .data = opt.data,
            .action = &action,
        })) nob_return_defer(false);
        switch (action) {
        case NOB_WALK_CONT: break;
        case NOB_WALK_STOP: *stop = true; // fallthrough
        case NOB_WALK_SKIP: nob_return_defer(true);
        default: NOB_UNREACHABLE("Nob_Walk_Action");
        }
    }

defer:
    // Always reset the file_path back to what it was
    file_path->count = saved_file_path_count;
    nob_da_last(file_path) = '\0';

    nob_dir_entry_close(dir);
    return result;
}

NOBDEF bool nob_walk_dir_opt(const char *root, Nob_Walk_Func func, Nob_Walk_Dir_Opt opt)
{
    Nob_String_Builder file_path = {0};

    nob_sb_appendf(&file_path, "%s", root);
    nob_sb_append_null(&file_path);

    bool stop = false;
    bool ok = nob__walk_dir_opt_impl(&file_path, func, 0, &stop, opt);
    free(file_path.items);
    return ok;
}

NOBDEF bool nob_read_entire_dir(const char *parent, Nob_File_Paths *children)
{
    if (strlen(parent) == 0) {
        nob_log(NOB_ERROR, "Cannot read empty path");
        return false;
    }
    bool result = true;
    Nob_Dir_Entry dir = {0};
    if (!nob_dir_entry_open(parent, &dir)) nob_return_defer(false);
    while (nob_dir_entry_next(&dir)) nob_da_append(children, nob_temp_strdup(dir.name));
    if (dir.error) nob_return_defer(false);
defer:
    nob_dir_entry_close(dir);
    return result;
}

NOBDEF bool nob_write_entire_file(const char *path, const void *data, size_t size)
{
    bool result = true;

    const char *buf = NULL;
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        nob_log(NOB_ERROR, "Could not open file %s for writing: %s\n", path, strerror(errno));
        nob_return_defer(false);
    }

    //           len
    //           v
    // aaaaaaaaaa
    //     ^
    //     data

    buf = (const char*)data;
    while (size > 0) {
        size_t n = fwrite(buf, 1, size, f);
        if (ferror(f)) {
            nob_log(NOB_ERROR, "Could not write into file %s: %s\n", path, strerror(errno));
            nob_return_defer(false);
        }
        size -= n;
        buf  += n;
    }

defer:
    if (f) fclose(f);
    return result;
}

NOBDEF Nob_File_Type nob_get_file_type(const char *path)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        nob_log(NOB_ERROR, "Could not get file attributes of %s: %s", path, nob_win32_error_message(GetLastError()));
        return (Nob_File_Type)-1;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY) return NOB_FILE_DIRECTORY;
    // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
    return NOB_FILE_REGULAR;
#else // _WIN32
    struct stat statbuf;
    if (lstat(path, &statbuf) < 0) {
        nob_log(NOB_ERROR, "Could not get stat of %s: %s", path, strerror(errno));
        return (Nob_File_Type)(-1);
    }

    if (S_ISREG(statbuf.st_mode)) return NOB_FILE_REGULAR;
    if (S_ISDIR(statbuf.st_mode)) return NOB_FILE_DIRECTORY;
    if (S_ISLNK(statbuf.st_mode)) return NOB_FILE_SYMLINK;
    return NOB_FILE_OTHER;
#endif // _WIN32
}

NOBDEF bool nob_delete_file(const char *path)
{
#ifndef NOB_NO_ECHO
    nob_log(NOB_INFO, "deleting %s", path);
#endif // NOB_NO_ECHO
#ifdef _WIN32
    Nob_File_Type type = nob_get_file_type(path);
    switch (type) {
    case NOB_FILE_DIRECTORY:
        if (!RemoveDirectoryA(path)) {
            nob_log(NOB_ERROR, "Could not delete directory %s: %s", path, nob_win32_error_message(GetLastError()));
            return false;
        }
        break;
    case NOB_FILE_REGULAR:
    case NOB_FILE_SYMLINK:
    case NOB_FILE_OTHER:
        if (!DeleteFileA(path)) {
            nob_log(NOB_ERROR, "Could not delete file %s: %s", path, nob_win32_error_message(GetLastError()));
            return false;
        }
        break;
    default: NOB_UNREACHABLE("Nob_File_Type");
    }
    return true;
#else
    if (remove(path) < 0) {
        nob_log(NOB_ERROR, "Could not delete file %s: %s", path, strerror(errno));
        return false;
    }
    return true;
#endif // _WIN32
}

NOBDEF bool nob_copy_directory_recursively(const char *src_path, const char *dst_path)
{
    bool result = true;
    Nob_File_Paths children = {0};
    Nob_String_Builder src_sb = {0};
    Nob_String_Builder dst_sb = {0};
    size_t temp_checkpoint = nob_temp_save();

    Nob_File_Type type = nob_get_file_type(src_path);
    if (type < 0) return false;

    switch (type) {
        case NOB_FILE_DIRECTORY: {
            if (!nob_mkdir_if_not_exists(dst_path)) nob_return_defer(false);
            if (!nob_read_entire_dir(src_path, &children)) nob_return_defer(false);

            for (size_t i = 0; i < children.count; ++i) {
                if (strcmp(children.items[i], ".") == 0) continue;
                if (strcmp(children.items[i], "..") == 0) continue;

                src_sb.count = 0;
                nob_sb_append_cstr(&src_sb, src_path);
                nob_sb_append_cstr(&src_sb, "/");
                nob_sb_append_cstr(&src_sb, children.items[i]);
                nob_sb_append_null(&src_sb);

                dst_sb.count = 0;
                nob_sb_append_cstr(&dst_sb, dst_path);
                nob_sb_append_cstr(&dst_sb, "/");
                nob_sb_append_cstr(&dst_sb, children.items[i]);
                nob_sb_append_null(&dst_sb);

                if (!nob_copy_directory_recursively(src_sb.items, dst_sb.items)) {
                    nob_return_defer(false);
                }
            }
        } break;

        case NOB_FILE_REGULAR: {
            if (!nob_copy_file(src_path, dst_path)) {
                nob_return_defer(false);
            }
        } break;

        case NOB_FILE_SYMLINK: {
            nob_log(NOB_WARNING, "TODO: Copying symlinks is not supported yet");
        } break;

        case NOB_FILE_OTHER: {
            nob_log(NOB_ERROR, "Unsupported type of file %s", src_path);
            nob_return_defer(false);
        } break;

        default: NOB_UNREACHABLE("nob_copy_directory_recursively");
    }

defer:
    nob_temp_rewind(temp_checkpoint);
    nob_da_free(src_sb);
    nob_da_free(dst_sb);
    nob_da_free(children);
    return result;
}

NOBDEF char *nob_temp_strdup(const char *cstr)
{
    size_t n = strlen(cstr);
    char *result = (char*)nob_temp_alloc(n + 1);
    NOB_ASSERT(result != NULL && "Increase NOB_TEMP_CAPACITY");
    memcpy(result, cstr, n);
    result[n] = '\0';
    return result;
}

NOBDEF char *nob_temp_strndup(const char *s, size_t n)
{
    char *r = (char*)nob_temp_alloc(n + 1);
    NOB_ASSERT(r != NULL && "Extend the size of the temporary allocator");
    memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

NOBDEF void *nob_temp_alloc(size_t requested_size)
{
    size_t word_size = sizeof(uintptr_t);
    size_t size = (requested_size + word_size - 1)/word_size*word_size;
    if (nob_temp_size + size > NOB_TEMP_CAPACITY) return NULL;
    void *result = &nob_temp[nob_temp_size];
    nob_temp_size += size;
    return result;
}

NOBDEF char *nob_temp_vsprintf(const char *format, va_list ap)
{
    va_list args;
    va_copy(args, ap);
    int n = vsnprintf(NULL, 0, format, args);
    va_end(args);

    NOB_ASSERT(n >= 0);
    char *result = (char*)nob_temp_alloc(n + 1);
    NOB_ASSERT(result != NULL && "Extend the size of the temporary allocator");
    // TODO: use proper arenas for the temporary allocator;
    va_copy(args, ap);
    vsnprintf(result, n + 1, format, args);
    va_end(args);

    return result;
}

NOBDEF char *nob_temp_sprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char *result = nob_temp_vsprintf(format, args);
    va_end(args);
    return result;
}

NOBDEF void nob_temp_reset(void)
{
    nob_temp_size = 0;
}

NOBDEF size_t nob_temp_save(void)
{
    return nob_temp_size;
}

NOBDEF void nob_temp_rewind(size_t checkpoint)
{
    nob_temp_size = checkpoint;
}

NOBDEF const char *nob_temp_sv_to_cstr(Nob_String_View sv)
{
    return nob_temp_strndup(sv.data, sv.count);
}

NOBDEF int nob_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count)
{
#ifdef _WIN32
    BOOL bSuccess;

    HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (output_path_fd == INVALID_HANDLE_VALUE) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (GetLastError() == ERROR_FILE_NOT_FOUND) return 1;
        nob_log(NOB_ERROR, "Could not open file %s: %s", output_path, nob_win32_error_message(GetLastError()));
        return -1;
    }
    FILETIME output_path_time;
    bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
    CloseHandle(output_path_fd);
    if (!bSuccess) {
        nob_log(NOB_ERROR, "Could not get time of %s: %s", output_path, nob_win32_error_message(GetLastError()));
        return -1;
    }

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        HANDLE input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (input_path_fd == INVALID_HANDLE_VALUE) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            nob_log(NOB_ERROR, "Could not open file %s: %s", input_path, nob_win32_error_message(GetLastError()));
            return -1;
        }
        FILETIME input_path_time;
        bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
        CloseHandle(input_path_fd);
        if (!bSuccess) {
            nob_log(NOB_ERROR, "Could not get time of %s: %s", input_path, nob_win32_error_message(GetLastError()));
            return -1;
        }

        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (CompareFileTime(&input_path_time, &output_path_time) == 1) return 1;
    }

    return 0;
#else
    struct stat statbuf = {0};

    if (stat(output_path, &statbuf) < 0) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (errno == ENOENT) return 1;
        nob_log(NOB_ERROR, "could not stat %s: %s", output_path, strerror(errno));
        return -1;
    }
    time_t output_path_time = statbuf.st_mtime;

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        if (stat(input_path, &statbuf) < 0) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            nob_log(NOB_ERROR, "could not stat %s: %s", input_path, strerror(errno));
            return -1;
        }
        time_t input_path_time = statbuf.st_mtime;
        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (input_path_time > output_path_time) return 1;
    }

    return 0;
#endif
}

NOBDEF int nob_needs_rebuild1(const char *output_path, const char *input_path)
{
    return nob_needs_rebuild(output_path, &input_path, 1);
}

NOBDEF const char *nob_path_name(const char *path)
{
#ifdef _WIN32
    const char *p1 = strrchr(path, '/');
    const char *p2 = strrchr(path, '\\');
    const char *p = (p1 > p2)? p1 : p2;  // NULL is ignored if the other search is successful
    return p ? p + 1 : path;
#else
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
#endif // _WIN32
}

NOBDEF bool nob_rename(const char *old_path, const char *new_path)
{
#ifndef NOB_NO_ECHO
    nob_log(NOB_INFO, "renaming %s -> %s", old_path, new_path);
#endif // NOB_NO_ECHO
#ifdef _WIN32
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
        nob_log(NOB_ERROR, "could not rename %s to %s: %s", old_path, new_path, nob_win32_error_message(GetLastError()));
        return false;
    }
#else
    if (rename(old_path, new_path) < 0) {
        nob_log(NOB_ERROR, "could not rename %s to %s: %s", old_path, new_path, strerror(errno));
        return false;
    }
#endif // _WIN32
    return true;
}

NOBDEF bool nob_read_entire_file(const char *path, Nob_String_Builder *sb)
{
    bool result = true;

    FILE *f = fopen(path, "rb");
    size_t new_count = 0;
    long long m = 0;
    if (f == NULL)                 nob_return_defer(false);
    if (fseek(f, 0, SEEK_END) < 0) nob_return_defer(false);
#ifndef _WIN32
    m = ftell(f);
#else
    m = _telli64(_fileno(f));
#endif
    if (m < 0)                     nob_return_defer(false);
    if (fseek(f, 0, SEEK_SET) < 0) nob_return_defer(false);

    new_count = sb->count + m;
    if (new_count > sb->capacity) {
        sb->items = NOB_DECLTYPE_CAST(sb->items)NOB_REALLOC(sb->items, new_count);
        NOB_ASSERT(sb->items != NULL && "Buy more RAM lool!!");
        sb->capacity = new_count;
    }

    fread(sb->items + sb->count, m, 1, f);
    if (ferror(f)) {
        // TODO: Afaik, ferror does not set errno. So the error reporting in defer is not correct in this case.
        nob_return_defer(false);
    }
    sb->count = new_count;

defer:
    if (!result) nob_log(NOB_ERROR, "Could not read file %s: %s", path, strerror(errno));
    if (f) fclose(f);
    return result;
}

NOBDEF int nob_sb_appendf(Nob_String_Builder *sb, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    // NOTE: the new_capacity needs to be +1 because of the null terminator.
    // However, further below we increase sb->count by n, not n + 1.
    // This is because we don't want the sb to include the null terminator. The user can always sb_append_null() if they want it
    nob_da_reserve(sb, sb->count + n + 1);
    char *dest = sb->items + sb->count;
    va_start(args, fmt);
    vsnprintf(dest, n+1, fmt, args);
    va_end(args);

    sb->count += n;

    return n;
}

NOBDEF void nob_sb_pad_align(Nob_String_Builder *sb, size_t size)
{
    size_t rem = sb->count%size;
    if (rem == 0) return;
    for (size_t i = 0; i < size - rem; ++i) {
        nob_da_append(sb, 0);
    }
}

NOBDEF Nob_String_View nob_sv_chop_while(Nob_String_View *sv, int (*p)(int x))
{
    size_t i = 0;
    while (i < sv->count && p(sv->data[i])) {
        i += 1;
    }

    Nob_String_View result = nob_sv_from_parts(sv->data, i);
    sv->count -= i;
    sv->data  += i;

    return result;
}

NOBDEF Nob_String_View nob_sv_chop_by_delim(Nob_String_View *sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    Nob_String_View result = nob_sv_from_parts(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

NOBDEF bool nob_sv_chop_prefix(Nob_String_View *sv, Nob_String_View prefix)
{
    if (nob_sv_starts_with(*sv, prefix)) {
        nob_sv_chop_left(sv, prefix.count);
        return true;
    }
    return false;
}

NOBDEF bool nob_sv_chop_suffix(Nob_String_View *sv, Nob_String_View suffix)
{
    if (nob_sv_ends_with(*sv, suffix)) {
        nob_sv_chop_right(sv, suffix.count);
        return true;
    }
    return false;
}

NOBDEF Nob_String_View nob_sv_chop_left(Nob_String_View *sv, size_t n)
{
    if (n > sv->count) {
        n = sv->count;
    }

    Nob_String_View result = nob_sv_from_parts(sv->data, n);

    sv->data  += n;
    sv->count -= n;

    return result;
}

NOBDEF Nob_String_View nob_sv_chop_right(Nob_String_View *sv, size_t n)
{
    if (n > sv->count) {
        n = sv->count;
    }

    Nob_String_View result = nob_sv_from_parts(sv->data + sv->count - n, n);

    sv->count -= n;

    return result;
}

NOBDEF Nob_String_View nob_sv_from_parts(const char *data, size_t count)
{
    Nob_String_View sv;
    sv.count = count;
    sv.data = data;
    return sv;
}

NOBDEF Nob_String_View nob_sv_trim_left(Nob_String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return nob_sv_from_parts(sv.data + i, sv.count - i);
}

NOBDEF Nob_String_View nob_sv_trim_right(Nob_String_View sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return nob_sv_from_parts(sv.data, sv.count - i);
}

NOBDEF Nob_String_View nob_sv_trim(Nob_String_View sv)
{
    return nob_sv_trim_right(nob_sv_trim_left(sv));
}

NOBDEF Nob_String_View nob_sv_from_cstr(const char *cstr)
{
    return nob_sv_from_parts(cstr, strlen(cstr));
}

NOBDEF bool nob_sv_eq(Nob_String_View a, Nob_String_View b)
{
    if (a.count != b.count) {
        return false;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

NOBDEF bool nob_sv_end_with(Nob_String_View sv, const char *cstr)
{
    return nob_sv_ends_with_cstr(sv, cstr);
}

NOBDEF bool nob_sv_ends_with_cstr(Nob_String_View sv, const char *cstr)
{
    return nob_sv_ends_with(sv, nob_sv_from_cstr(cstr));
}

NOBDEF bool nob_sv_ends_with(Nob_String_View sv, Nob_String_View suffix)
{
    if (sv.count >= suffix.count) {
        Nob_String_View sv_tail = {
            .count = suffix.count,
            .data = sv.data + sv.count - suffix.count,
        };
        return nob_sv_eq(sv_tail, suffix);
    }
    return false;
}

NOBDEF bool nob_sv_starts_with(Nob_String_View sv, Nob_String_View expected_prefix)
{
    if (expected_prefix.count <= sv.count) {
        Nob_String_View actual_prefix = nob_sv_from_parts(sv.data, expected_prefix.count);
        return nob_sv_eq(expected_prefix, actual_prefix);
    }

    return false;
}

// RETURNS:
//  0 - file does not exists
//  1 - file exists
NOBDEF int nob_file_exists(const char *file_path)
{
#if _WIN32
    return GetFileAttributesA(file_path) != INVALID_FILE_ATTRIBUTES;
#else
    return access(file_path, F_OK) == 0;
#endif
}

NOBDEF const char *nob_get_current_dir_temp(void)
{
#ifdef _WIN32
    DWORD nBufferLength = GetCurrentDirectory(0, NULL);
    if (nBufferLength == 0) {
        nob_log(NOB_ERROR, "could not get current directory: %s", nob_win32_error_message(GetLastError()));
        return NULL;
    }

    char *buffer = (char*) nob_temp_alloc(nBufferLength);
    if (GetCurrentDirectory(nBufferLength, buffer) == 0) {
        nob_log(NOB_ERROR, "could not get current directory: %s", nob_win32_error_message(GetLastError()));
        return NULL;
    }

    return buffer;
#else
    char *buffer = (char*) nob_temp_alloc(PATH_MAX);
    if (getcwd(buffer, PATH_MAX) == NULL) {
        nob_log(NOB_ERROR, "could not get current directory: %s", strerror(errno));
        return NULL;
    }

    return buffer;
#endif // _WIN32
}

NOBDEF bool nob_set_current_dir(const char *path)
{
#ifdef _WIN32
    if (!SetCurrentDirectory(path)) {
        nob_log(NOB_ERROR, "could not set current directory to %s: %s", path, nob_win32_error_message(GetLastError()));
        return false;
    }
    return true;
#else
    if (chdir(path) < 0) {
        nob_log(NOB_ERROR, "could not set current directory to %s: %s", path, strerror(errno));
        return false;
    }
    return true;
#endif // _WIN32
}

NOBDEF char *nob_temp_dir_name(const char *path)
{
#ifndef _WIN32
    // Stolen from the musl's implementation of dirname.
    // We are implementing our own one because libc vendors cannot agree on whether dirname(3)
    // modifies the path or not.
    if (!path || !*path) return nob_temp_strdup(".");
    size_t i = strlen(path) - 1;
    for (; path[i] == '/'; i--) if (!i) return nob_temp_strdup("/");
    for (; path[i] != '/'; i--) if (!i) return nob_temp_strdup(".");
    for (; path[i] == '/'; i--) if (!i) return nob_temp_strdup("/");
    return nob_temp_strndup(path, i + 1);
#else
    if (!path) path = ""; // Treating NULL as empty.
    char *drive = (char*) nob_temp_alloc(_MAX_DRIVE);
    char *dir   = (char*) nob_temp_alloc(_MAX_DIR);
    // https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/8e46eyt7(v=vs.100)
    errno_t ret = _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
    NOB_ASSERT(ret == 0);
    return nob_temp_sprintf("%s%s", drive, dir);
#endif // _WIN32
}

NOBDEF char *nob_temp_file_name(const char *path)
{
#ifndef _WIN32
    // Stolen from the musl's implementation of dirname.
    // We are implementing our own one because libc vendors cannot agree on whether basename(3)
    // modifies the path or not.
    if (!path || !*path) return nob_temp_strdup(".");
    char *s = nob_temp_strdup(path);
    size_t i = strlen(s)-1;
    for (; i&&s[i]=='/'; i--) s[i] = 0;
    for (; i&&s[i-1]!='/'; i--);
    return s+i;
#else
    if (!path) path = ""; // Treating NULL as empty.
    char *fname = (char*)nob_temp_alloc(_MAX_FNAME);
    char *ext   = (char*)nob_temp_alloc(_MAX_EXT);
    // https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/8e46eyt7(v=vs.100)
    errno_t ret = _splitpath_s(path, NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
    NOB_ASSERT(ret == 0);
    return nob_temp_sprintf("%s%s", fname, ext);
#endif // _WIN32
}

NOBDEF char *nob_temp_file_ext(const char *path)
{
#ifndef _WIN32
    return strrchr(nob_temp_file_name(path), '.');
#else
    if (!path) path = ""; // Treating NULL as empty.
    char *ext = (char*)nob_temp_alloc(_MAX_EXT);
    // https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/8e46eyt7(v=vs.100)
    errno_t ret = _splitpath_s(path, NULL, 0, NULL, 0, NULL, 0, ext, _MAX_EXT);
    NOB_ASSERT(ret == 0);
    return ext;
#endif // _WIN32
}

NOBDEF char *nob_temp_running_executable_path(void)
{
#if defined(__linux__)
    char buf[4096];
    int length = readlink("/proc/self/exe", buf, NOB_ARRAY_LEN(buf));
    if (length < 0) return nob_temp_strdup("");
    return nob_temp_strndup(buf, length);
#elif defined(_WIN32)
    char buf[MAX_PATH];
    int length = GetModuleFileNameA(NULL, buf, MAX_PATH);
    return nob_temp_strndup(buf, length);
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = NOB_ARRAY_LEN(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return nob_temp_strdup("");
    int length = strlen(buf);
    return nob_temp_strndup(buf, length);
#elif defined(__FreeBSD__)
    char buf[4096];
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t length = sizeof(buf);
    if (sysctl(mib, 4, buf, &length, NULL, 0) < 0) return nob_temp_strdup("");
    return nob_temp_strndup(buf, length);
#elif defined(__HAIKU__)
    int cookie = 0;
    image_info info;
    while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK)
        if (info.type == B_APP_IMAGE)
            break;
    return nob_temp_strndup(info.name, strlen(info.name));
#else
    fprintf(stderr, "%s:%d: TODO: nob_temp_running_executable_path is not implemented for this platform\n", __FILE__, __LINE__);
    return nob_temp_strdup("");
#endif
}

#endif // NOB_IMPLEMENTATION

#ifndef NOB_STRIP_PREFIX_GUARD_
#define NOB_STRIP_PREFIX_GUARD_
    // NOTE: The name stripping should be part of the header so it's not accidentally included
    // several times. At the same time, it should be at the end of the file so to not create any
    // potential conflicts in the NOB_IMPLEMENTATION. The header obviously cannot be at the end
    // of the file because NOB_IMPLEMENTATION needs the forward declarations from there. So the
    // solution is to split the header into two parts where the name stripping part is at the
    // end of the file after the NOB_IMPLEMENTATION.
    #ifndef NOB_UNSTRIP_PREFIX
        #define TODO NOB_TODO
        #define UNREACHABLE NOB_UNREACHABLE
        #define UNUSED NOB_UNUSED
        #define ARRAY_LEN NOB_ARRAY_LEN
        #define ARRAY_GET NOB_ARRAY_GET
        #define INFO NOB_INFO
        #define WARNING NOB_WARNING
        #define ERROR NOB_ERROR
        #define NO_LOGS NOB_NO_LOGS
        #define Log_Level Nob_Log_Level
        #define minimal_log_level nob_minimal_log_level
        #define log_handler nob_log_handler
        #define Log_Handler Nob_Log_Handler
        #define set_log_handler nob_set_log_handler
        #define get_log_handler nob_get_log_handler
        #define null_log_handler nob_null_log_handler
        #define default_log_handler nob_default_log_handler
        #define cancer_log_handler nob_cancer_log_handler
        // NOTE: Name log is already defined in math.h and historically always was the natural logarithmic function.
        // So there should be no reason to strip the `nob_` prefix in this specific case.
        // #define log nob_log
        #define shift nob_shift
        #define shift_args nob_shift_args
        #define GO_REBUILD_URSELF NOB_GO_REBUILD_URSELF
        #define GO_REBUILD_URSELF_PLUS NOB_GO_REBUILD_URSELF_PLUS
        #define File_Paths Nob_File_Paths
        #define FILE_REGULAR NOB_FILE_REGULAR
        #define FILE_DIRECTORY NOB_FILE_DIRECTORY
        #define FILE_SYMLINK NOB_FILE_SYMLINK
        #define FILE_OTHER NOB_FILE_OTHER
        #define File_Type Nob_File_Type
        #define mkdir_if_not_exists nob_mkdir_if_not_exists
        #define copy_file nob_copy_file
        #define copy_directory_recursively nob_copy_directory_recursively
        #define read_entire_dir nob_read_entire_dir
        #define WALK_CONT NOB_WALK_CONT
        #define WALK_SKIP NOB_WALK_SKIP
        #define WALK_STOP NOB_WALK_STOP
        #define Walk_Action Nob_Walk_Action
        #define Walk_Entry Nob_Walk_Entry
        #define Walk_Func Nob_Walk_Func
        #define Walk_Dir_Opt Nob_Walk_Dir_Opt
        #define walk_dir nob_walk_dir
        #define walk_dir_opt nob_walk_dir_opt
        #define write_entire_file nob_write_entire_file
        #define get_file_type nob_get_file_type
        #define delete_file nob_delete_file
        #define Dir_Entry Nob_Dir_Entry
        #define dir_entry_open nob_dir_entry_open
        #define dir_entry_next nob_dir_entry_next
        #define dir_entry_close nob_dir_entry_close
        #define return_defer nob_return_defer
        #define da_append nob_da_append
        #define da_free nob_da_free
        #define da_append_many nob_da_append_many
        #define da_resize nob_da_resize
        #define da_reserve nob_da_reserve
        #define da_last nob_da_last
        #define da_first nob_da_first
        #define da_pop nob_da_pop
        #define da_remove_unordered nob_da_remove_unordered
        #define da_foreach nob_da_foreach
        #define fa_append nob_fa_append
        #define swap nob_swap
        #define String_Builder Nob_String_Builder
        #define read_entire_file nob_read_entire_file
        #define sb_appendf nob_sb_appendf
        #define sb_append_buf nob_sb_append_buf
        #define sb_append_sv nob_sb_append_sv
        #define sb_append_cstr nob_sb_append_cstr
        #define sb_append_null nob_sb_append_null
        #define sb_append nob_sb_append
        #define sb_pad_align nob_sb_pad_align
        #define sb_free nob_sb_free
        #define Proc Nob_Proc
        #define INVALID_PROC NOB_INVALID_PROC
        #define Fd Nob_Fd
        #define Pipe Nob_Pipe
        #define pipe_create nob_pipe_create
        #define Chain Nob_Chain
        #define Chain_Begin_Opt Nob_Chain_Begin_Opt
        #define chain_begin nob_chain_begin
        #define chain_begin_opt nob_chain_begin_opt
        #define Chain_Cmd_Opt Nob_Chain_Cmd_Opt
        #define chain_cmd nob_chain_cmd
        #define chain_cmd_opt nob_chain_cmd_opt
        #define Chain_End_Opt Nob_Chain_End_Opt
        #define chain_end nob_chain_end
        #define chain_end_opt nob_chain_end_opt
        #define INVALID_FD NOB_INVALID_FD
        #define fd_open_for_read nob_fd_open_for_read
        #define fd_open_for_write nob_fd_open_for_write
        #define fd_close nob_fd_close
        #define Procs Nob_Procs
        #define proc_wait nob_proc_wait
        #define procs_wait nob_procs_wait
        #define procs_wait_and_reset nob_procs_wait_and_reset
        #define procs_append_with_flush nob_procs_append_with_flush
        #define procs_flush nob_procs_flush
        #define CLIT NOB_CLIT
        #define Cmd Nob_Cmd
        #define Cmd_Redirect Nob_Cmd_Redirect
        #define Cmd_Opt Nob_Cmd_Opt
        #define cmd_run_opt nob_cmd_run_opt
        #define cmd_run nob_cmd_run
        #define cmd_render nob_cmd_render
        #define cmd_append nob_cmd_append
        #define cmd_extend nob_cmd_extend
        #define cmd_free nob_cmd_free
        #define cmd_run_async nob_cmd_run_async
        #define cmd_run_async_and_reset nob_cmd_run_async_and_reset
        #define cmd_run_async_redirect nob_cmd_run_async_redirect
        #define cmd_run_async_redirect_and_reset nob_cmd_run_async_redirect_and_reset
        #define cmd_run_sync nob_cmd_run_sync
        #define cmd_run_sync_and_reset nob_cmd_run_sync_and_reset
        #define cmd_run_sync_redirect nob_cmd_run_sync_redirect
        #define cmd_run_sync_redirect_and_reset nob_cmd_run_sync_redirect_and_reset
        #define temp_strdup nob_temp_strdup
        #define temp_strndup nob_temp_strndup
        #define temp_alloc nob_temp_alloc
        #define temp_sprintf nob_temp_sprintf
        #define temp_vsprintf nob_temp_vsprintf
        #define temp_reset nob_temp_reset
        #define temp_save nob_temp_save
        #define temp_rewind nob_temp_rewind
        #define path_name nob_path_name
        // NOTE: rename(2) is widely known POSIX function. We never wanna collide with it.
        // #define rename nob_rename
        #define needs_rebuild nob_needs_rebuild
        #define needs_rebuild1 nob_needs_rebuild1
        #define file_exists nob_file_exists
        #define get_current_dir_temp nob_get_current_dir_temp
        #define set_current_dir nob_set_current_dir
        #define temp_dir_name nob_temp_dir_name
        #define temp_file_name nob_temp_file_name
        #define temp_file_ext nob_temp_file_ext
        #define temp_running_executable_path nob_temp_running_executable_path
        #define String_View Nob_String_View
        #define temp_sv_to_cstr nob_temp_sv_to_cstr
        #define sv_chop_by_delim nob_sv_chop_by_delim
        #define sv_chop_while nob_sv_chop_while
        #define sv_chop_prefix nob_sv_chop_prefix
        #define sv_chop_suffix nob_sv_chop_suffix
        #define sv_chop_left nob_sv_chop_left
        #define sv_chop_right nob_sv_chop_right
        #define sv_trim nob_sv_trim
        #define sv_trim_left nob_sv_trim_left
        #define sv_trim_right nob_sv_trim_right
        #define sv_eq nob_sv_eq
        #define sv_starts_with nob_sv_starts_with
        #define sv_end_with nob_sv_end_with
        #define sv_ends_with nob_sv_ends_with
        #define sv_ends_with_cstr nob_sv_ends_with_cstr
        #define sv_from_cstr nob_sv_from_cstr
        #define sv_from_parts nob_sv_from_parts
        #define sb_to_sv nob_sb_to_sv
        #define win32_error_message nob_win32_error_message
        #define nprocs nob_nprocs
        #define nanos_since_unspecified_epoch nob_nanos_since_unspecified_epoch
        #define NANOS_PER_SEC NOB_NANOS_PER_SEC
    #endif // NOB_STRIP_PREFIX
#endif // NOB_STRIP_PREFIX_GUARD_

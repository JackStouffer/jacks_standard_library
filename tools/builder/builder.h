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

#ifndef JSL_BUILDER_H_
    #define JSL_BUILDER_H_

    #ifdef _WIN32
        #ifndef _CRT_SECURE_NO_WARNINGS
            #define _CRT_SECURE_NO_WARNINGS (1)
        #endif // _CRT_SECURE_NO_WARNINGS
    #endif //  _WIN32

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
        #define WIN32_LEAN_AND_MEAN
        #define _WINUSER_
        #define _WINGDI_
        #define _IMM_
        #define _WINCON_
        #include <windows.h>
        #include <direct.h>
        #include <io.h>
        #include <shellapi.h>
    #else
        #ifdef __APPLE__
            #include <mach-o/dyld.h>
        #endif
        #ifdef __FreeBSD__
            #include <sys/sysctl.h>
        #endif

        #include <sys/types.h>
        #include <sys/wait.h>
        #include <sys/stat.h>
        #include <unistd.h>
        #include <fcntl.h>
        #include <dirent.h>
    #endif

    #ifdef __HAIKU__
        #include <image.h>
    #endif

    #ifdef _WIN32
        #define JSL_BUILDER_LINE_END "\r\n"
    #else
        #define JSL_BUILDER_LINE_END "\n"
    #endif

    #if defined(__GNUC__) || defined(__clang__)
    //   https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
    #ifdef __MINGW_PRINTF_FORMAT
    #define JSL_BUILDER_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__((format(__MINGW_PRINTF_FORMAT, STRING_INDEX, FIRST_TO_CHECK)))
    #else
    #define JSL_BUILDER_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK) __attribute__((format(printf, STRING_INDEX, FIRST_TO_CHECK)))
    #endif // __MINGW_PRINTF_FORMAT
    #else
    //   TODO: implement JSL_BUILDER_PRINTF_FORMAT for MSVC
    #define JSL_BUILDER_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
    #endif

    #define JSL_BUILDER_UNUSED(value) (void)(value)
    #define JSL_BUILDER_TODO(message)                                                  \
        do                                                                     \
        {                                                                      \
            fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); \
            abort();                                                           \
        } while (0)

    #define JSL_BUILDER_UNREACHABLE(message)                                                  \
        do                                                                            \
        {                                                                             \
            fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message); \
            abort();                                                                  \
        } while (0)

    #define JSL_BUILDER_ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
    #define JSL_BUILDER_ARRAY_GET(array, index) \
        (JSL_ASSERT((size_t)index < JSL_BUILDER_ARRAY_LEN(array)), array[(size_t)index])

    typedef enum
    {
        JSL_BUILDER_INFO,
        JSL_BUILDER_WARNING,
        JSL_BUILDER_ERROR,
        JSL_BUILDER_NO_LOGS,
    } JSL_BUILDER_Log_Level;

    // Any messages with the level below JSL_BUILDER_minimal_log_level are going to be suppressed by the JSL_BUILDER_default_log_handler.
    extern JSL_BUILDER_Log_Level JSL_BUILDER_minimal_log_level;

    typedef void(JSL_BUILDER_Log_Handler)(JSL_BUILDER_Log_Level level, const char *fmt, va_list args);
    JSL_BUILDER_DEPRECATED("Uncapitalized JSL_BUILDER_log_handler type is deprecated. Use JSL_BUILDER_Log_Handler instead. It's just when we were releasing the log handler feature we forgot that we had a convention that all the types must be capitalized like that. Sorry about it!")
    typedef JSL_BUILDER_Log_Handler JSL_BUILDER_log_handler;

    void JSL_BUILDER_set_log_handler(JSL_BUILDER_Log_Handler *handler);
    JSL_BUILDER_Log_Handler *JSL_BUILDER_get_log_handler(void);

    JSL_BUILDER_Log_Handler JSL_BUILDER_default_log_handler;
    JSL_BUILDER_Log_Handler JSL_BUILDER_cancer_log_handler;
    JSL_BUILDER_Log_Handler JSL_BUILDER_null_log_handler;

    void JSL_BUILDER_log(JSL_BUILDER_Log_Level level, const char *fmt, ...) JSL_BUILDER_PRINTF_FORMAT(2, 3);

    // It is an equivalent of shift command from bash (do `help shift` in bash). It basically
    // pops an element from the beginning of a sized array.
    #define JSL_BUILDER_shift(xs, xs_sz) (JSL_ASSERT((xs_sz) > 0), (xs_sz)--, *(xs)++)
    // NOTE: JSL_BUILDER_shift_args() is an alias for an old variant of JSL_BUILDER_shift that only worked with
    // the command line arguments passed to the main() function. JSL_BUILDER_shift() is more generic.
    // So JSL_BUILDER_shift_args() is semi-deprecated, but I don't see much reason to urgently
    // remove it. This alias does not hurt anybody.
    #define JSL_BUILDER_shift_args(argc, argv) JSL_BUILDER_shift(*argv, *argc)

    typedef struct
    {
        const char **items;
        size_t count;
        size_t capacity;
    } JSL_BUILDER_File_Paths;

    typedef enum
    {
        JSL_BUILDER_FILE_REGULAR = 0,
        JSL_BUILDER_FILE_DIRECTORY,
        JSL_BUILDER_FILE_SYMLINK,
        JSL_BUILDER_FILE_OTHER,
    } JSL_BUILDER_File_Type;

    bool JSL_BUILDER_mkdir_if_not_exists(const char *path);
    bool JSL_BUILDER_copy_file(const char *src_path, const char *dst_path);
    bool JSL_BUILDER_copy_directory_recursively(const char *src_path, const char *dst_path);
    bool JSL_BUILDER_read_entire_dir(const char *parent, JSL_BUILDER_File_Paths *children);
    bool JSL_BUILDER_write_entire_file(const char *path, const void *data, size_t size);
    JSL_BUILDER_File_Type JSL_BUILDER_get_file_type(const char *path);
    bool JSL_BUILDER_delete_file(const char *path);

    typedef enum
    {
        // If the current file is a directory go inside of it.
        JSL_BUILDER_WALK_CONT,
        // If the current file is a directory do not go inside of it.
        JSL_BUILDER_WALK_SKIP,
        // Stop the recursive traversal process entirely.
        JSL_BUILDER_WALK_STOP,
    } JSL_BUILDER_Walk_Action;

    typedef struct
    {
        // The path to the visited file. The lifetime of the path string is very short.
        // As soon as the execution exits the JSL_BUILDER_Walk_Func it's dead. Dup it somewhere
        // if you want to preserve it for longer periods of time.
        const char *path;
        // The type of the visited file.
        JSL_BUILDER_File_Type type;
        // How nested we currently are in the directory tree.
        size_t level;
        // User data supplied in JSL_BUILDER_Walk_Dir_Opt.data.
        void *data;
        // The action JSL_BUILDER_walk_dir_opt() must perform after the JSL_BUILDER_Walk_Func has returned.
        // Default is JSL_BUILDER_WALK_CONT.
        JSL_BUILDER_Walk_Action *action;
    } JSL_BUILDER_Walk_Entry;

    // A function that is called by JSL_BUILDER_walk_dir_opt() on each visited file.
    // JSL_BUILDER_Walk_Entry provides the details about the visited file and also
    // expects you to modify the `action` in case you want to alter the
    // usual behavior of the recursive walking algorithm.
    //
    // If the function returns `false`, an error is assumed which causes the entire
    // recursive walking process to exit and JSL_BUILDER_walk_dir_opt() return `false`.
    typedef bool (*JSL_BUILDER_Walk_Func)(JSL_BUILDER_Walk_Entry entry);

    typedef struct
    {
        // User data passed to JSL_BUILDER_Walk_Entry.data
        void *data;
        // Walk the directory in post-order visiting the leaf files first.
        bool post_order;
    } JSL_BUILDER_Walk_Dir_Opt;

    bool JSL_BUILDER_walk_dir_opt(const char *root, JSL_BUILDER_Walk_Func func, JSL_BUILDER_Walk_Dir_Opt);

    #define JSL_BUILDER_walk_dir(root, func, ...) JSL_BUILDER_walk_dir_opt((root), (func), JSL_BUILDER_CLIT(JSL_BUILDER_Walk_Dir_Opt){__VA_ARGS__})

    typedef struct
    {
        char *name;
        bool error;

        struct
        {
    #ifdef _WIN32
            WIN32_FIND_DATA win32_data;
            HANDLE win32_hFind;
            bool win32_init;
    #else
            DIR *posix_dir;
            struct dirent *posix_ent;
    #endif              // _WIN32
        } JSL_BUILDER__private; // TODO: we don't have solid conventions regarding private struct fields
    } JSL_BUILDER_Dir_Entry;

    // JSL_BUILDER_dir_entry_open() - open the directory entry for iteration.
    // RETURN:
    //   true  - Sucess.
    //   false - Error. I will be logged automatically with JSL_BUILDER_log().
    bool JSL_BUILDER_dir_entry_open(const char *dir_path, JSL_BUILDER_Dir_Entry *dir);
    // JSL_BUILDER_dir_entry_next() - acquire the next file in the directory.
    // RETURN:
    //   true - Successfully acquired the next file.
    //   false - Either failure or no more files to iterate. In case of failure dir->error is set to true.
    bool JSL_BUILDER_dir_entry_next(JSL_BUILDER_Dir_Entry *dir);
    void JSL_BUILDER_dir_entry_close(JSL_BUILDER_Dir_Entry dir);

    #define JSL_BUILDER_return_defer(value) \
        do                          \
        {                           \
            result = (value);       \
            goto defer;             \
        } while (0)

    // Initial capacity of a dynamic array
    #ifndef JSL_BUILDER_DA_INIT_CAP
    #define JSL_BUILDER_DA_INIT_CAP 256
    #endif

    #ifdef __cplusplus
    #define JSL_BUILDER_DECLTYPE_CAST(T) (decltype(T))
    #else
    #define JSL_BUILDER_DECLTYPE_CAST(T)
    #endif // __cplusplus

    #define JSL_BUILDER_da_reserve(da, expected_capacity)                                                                             \
        do                                                                                                                    \
        {                                                                                                                     \
            if ((expected_capacity) > (da)->capacity)                                                                         \
            {                                                                                                                 \
                if ((da)->capacity == 0)                                                                                      \
                {                                                                                                             \
                    (da)->capacity = JSL_BUILDER_DA_INIT_CAP;                                                                         \
                }                                                                                                             \
                while ((expected_capacity) > (da)->capacity)                                                                  \
                {                                                                                                             \
                    (da)->capacity *= 2;                                                                                      \
                }                                                                                                             \
                (da)->items = JSL_BUILDER_DECLTYPE_CAST((da)->items) JSL_BUILDER_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
                JSL_ASSERT((da)->items != NULL && "Buy more RAM lol");                                                        \
            }                                                                                                                 \
        } while (0)

    // Append an item to a dynamic array
    #define JSL_BUILDER_da_append(da, item)                \
        do                                         \
        {                                          \
            JSL_BUILDER_da_reserve((da), (da)->count + 1); \
            (da)->items[(da)->count++] = (item);   \
        } while (0)

    #define JSL_BUILDER_da_free(da) JSL_BUILDER_FREE((da).items)

    // Append several items to a dynamic array
    #define JSL_BUILDER_da_append_many(da, new_items, new_items_count)                                        \
        do                                                                                            \
        {                                                                                             \
            JSL_BUILDER_da_reserve((da), (da)->count + (new_items_count));                                    \
            memcpy((da)->items + (da)->count, (new_items), (new_items_count) * sizeof(*(da)->items)); \
            (da)->count += (new_items_count);                                                         \
        } while (0)

    #define JSL_BUILDER_da_resize(da, new_size)     \
        do                                  \
        {                                   \
            JSL_BUILDER_da_reserve((da), new_size); \
            (da)->count = (new_size);       \
        } while (0)

    #define JSL_BUILDER_da_pop(da) (da)->items[(JSL_ASSERT((da)->count > 0), --(da)->count)]
    #define JSL_BUILDER_da_first(da) (da)->items[(JSL_ASSERT((da)->count > 0), 0)]
    #define JSL_BUILDER_da_last(da) (da)->items[(JSL_ASSERT((da)->count > 0), (da)->count - 1)]
    #define JSL_BUILDER_da_remove_unordered(da, i)               \
        do                                               \
        {                                                \
            size_t j = (i);                              \
            JSL_ASSERT(j < (da)->count);                 \
            (da)->items[j] = (da)->items[--(da)->count]; \
        } while (0)

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
    // JSL_BUILDER_da_append(&xs, 69);
    // JSL_BUILDER_da_append(&xs, 420);
    // JSL_BUILDER_da_append(&xs, 1337);
    //
    // JSL_BUILDER_da_foreach(int, x, &xs) {
    //     // `x` here is a pointer to the current element. You can get its index by taking a difference
    //     // between `x` and the start of the array which is `x.items`.
    //     size_t index = x - xs.items;
    //     JSL_BUILDER_log(INFO, "%zu: %d", index, *x);
    // }
    // ```
    #define JSL_BUILDER_da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

    // The Fixed Array append. `items` fields must be a fixed size array. Its size determines the capacity.
    #define JSL_BUILDER_fa_append(fa, item)                            \
        (JSL_ASSERT((fa)->count < JSL_BUILDER_ARRAY_LEN((fa)->items)), \
        (fa)->items[(fa)->count++] = (item))

    typedef struct
    {
        char *items;
        size_t count;
        size_t capacity;
    } JSL_BUILDER_String_Builder;

    #define JSL_BUILDER_swap(T, a, b) \
        do                    \
        {                     \
            T t = a;          \
            a = b;            \
            b = t;            \
        } while (0)

    bool JSL_BUILDER_read_entire_file(const char *path, JSL_BUILDER_String_Builder *sb);
    int JSL_BUILDER_sb_appendf(JSL_BUILDER_String_Builder *sb, const char *fmt, ...) JSL_BUILDER_PRINTF_FORMAT(2, 3);
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
    void JSL_BUILDER_sb_pad_align(JSL_BUILDER_String_Builder *sb, size_t size);

    // Append a sized buffer to a string builder
    #define JSL_BUILDER_sb_append_buf(sb, buf, size) JSL_BUILDER_da_append_many(sb, buf, size)

    // Append a string view to a string builder
    #define JSL_BUILDER_sb_append_sv(sb, sv) JSL_BUILDER_sb_append_buf((sb), (sv).data, (sv).count)

    // Append a NULL-terminated string to a string builder
    #define JSL_BUILDER_sb_append_cstr(sb, cstr)  \
        do                                \
        {                                 \
            const char *s = (cstr);       \
            size_t n = strlen(s);         \
            JSL_BUILDER_da_append_many(sb, s, n); \
        } while (0)

    // Append a single NULL character at the end of a string builder. So then you can
    // use it a NULL-terminated C string
    #define JSL_BUILDER_sb_append_null(sb) JSL_BUILDER_da_append_many(sb, "", 1)

    #define JSL_BUILDER_sb_append JSL_BUILDER_da_append

    // Free the memory allocated by a string builder
    #define JSL_BUILDER_sb_free(sb) JSL_BUILDER_FREE((sb).items)

    // Process handle
    #ifdef _WIN32
    typedef HANDLE JSL_BUILDER_Proc;
    #define JSL_BUILDER_INVALID_PROC INVALID_HANDLE_VALUE
    typedef HANDLE JSL_BUILDER_Fd;
    #define JSL_BUILDER_INVALID_FD INVALID_HANDLE_VALUE
    #else
    typedef int JSL_BUILDER_Proc;
    #define JSL_BUILDER_INVALID_PROC (-1)
    typedef int JSL_BUILDER_Fd;
    #define JSL_BUILDER_INVALID_FD (-1)
    #endif // _WIN32

    JSL_BUILDER_Fd JSL_BUILDER_fd_open_for_read(const char *path);
    JSL_BUILDER_Fd JSL_BUILDER_fd_open_for_write(const char *path);
    void JSL_BUILDER_fd_close(JSL_BUILDER_Fd fd);

    typedef struct
    {
        JSL_BUILDER_Fd read;
        JSL_BUILDER_Fd write;
    } JSL_BUILDER_Pipe;

    bool JSL_BUILDER_pipe_create(JSL_BUILDER_Pipe *pp);

    typedef struct
    {
        JSL_BUILDER_Proc *items;
        size_t count;
        size_t capacity;
    } JSL_BUILDER_Procs;

    // Wait until the process has finished
    bool JSL_BUILDER_proc_wait(JSL_BUILDER_Proc proc);

    // Wait until all the processes have finished
    bool JSL_BUILDER_procs_wait(JSL_BUILDER_Procs procs);

    // Wait until all the processes have finished and empty the procs array.
    bool JSL_BUILDER_procs_flush(JSL_BUILDER_Procs *procs);

    // Alias to JSL_BUILDER_procs_flush
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_procs_flush(&procs)` instead.")
    bool JSL_BUILDER_procs_wait_and_reset(JSL_BUILDER_Procs *procs);

    // Append a new process to procs array and if procs.count reaches max_procs_count call JSL_BUILDER_procs_wait_and_reset() on it
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd, .async = &procs, .max_procs = <integer>)` instead")
    bool JSL_BUILDER_procs_append_with_flush(JSL_BUILDER_Procs *procs, JSL_BUILDER_Proc proc, size_t max_procs_count);

    // A command - the main workhorse of Nob. Nob is all about building commands and running them
    typedef struct
    {
        const char **items;
        size_t count;
        size_t capacity;
    } JSL_BUILDER_Cmd;

    // Options for JSL_BUILDER_cmd_run_opt() function.
    typedef struct
    {
        // Run the command asynchronously appending its JSL_BUILDER_Proc to the provided JSL_BUILDER_Procs array
        JSL_BUILDER_Procs *async;
        // Maximum processes allowed in the .async list. Zero implies JSL_BUILDER_nprocs().
        size_t max_procs;
        // Do not reset the command after execution.
        bool dont_reset;
        // Redirect stdin to file
        const char *stdin_path;
        // Redirect stdout to file
        const char *stdout_path;
        // Redirect stderr to file
        const char *stderr_path;
    } JSL_BUILDER_Cmd_Opt;

    // Run the command with options.
    bool JSL_BUILDER_cmd_run_opt(JSL_BUILDER_Cmd *cmd, JSL_BUILDER_Cmd_Opt opt);

    // Command Chains (in Shell Scripting they are know as Pipes)
    //
    // Usage:
    // ```c
    // JSL_BUILDER_Cmd cmd = {0};
    // JSL_BUILDER_Chain chain = {0};
    // if (!JSL_BUILDER_chain_begin(&chain)) return 1;
    // {
    //     JSL_BUILDER_cmd_append(&cmd, "echo", "Hello, World");
    //     if (!JSL_BUILDER_chain_cmd(&chain, &cmd)) return 1;
    //
    //     JSL_BUILDER_cmd_append(&cmd, "rev");
    //     if (!JSL_BUILDER_chain_cmd(&chain, &cmd)) return 1;
    //
    //     JSL_BUILDER_cmd_append(&cmd, "xxd");
    //     if (!JSL_BUILDER_chain_cmd(&chain, &cmd)) return 1;
    // }
    // if (!JSL_BUILDER_chain_end(&chain)) return 1;
    // ```
    //
    // The above is equivalent to a shell command:
    //
    // ```sh
    // echo "Hello, World" | rev | xxd
    // ```
    //
    // After JSL_BUILDER_chain_end() the JSL_BUILDER_Chain struct can be reused again.
    //
    // The fields of the JSL_BUILDER_Chain struct contain the intermediate state of the Command
    // Chain that is being built with the JSL_BUILDER_chain_cmd() calls and generally have no
    // particular use for the user.
    //
    // The only memory dynamically allocated within JSL_BUILDER_Chain belongs to the .cmd field.
    // So if you want to clean it all up you can just do free(chain.cmd.items).
    typedef struct
    {
        // The file descriptor of the output of the previous command. Will be used as the input for the next command.
        JSL_BUILDER_Fd fdin;
        // The command from the last JSL_BUILDER_chain_cmd() call.
        JSL_BUILDER_Cmd cmd;
        // The value of the optional .err2out parameter from the last JSL_BUILDER_chain_cmd() call.
        bool err2out;
    } JSL_BUILDER_Chain;

    typedef struct
    {
        const char *stdin_path;
    } JSL_BUILDER_Chain_Begin_Opt;
    #define JSL_BUILDER_chain_begin(chain, ...) JSL_BUILDER_chain_begin_opt((chain), JSL_BUILDER_CLIT(JSL_BUILDER_Chain_Begin_Opt){__VA_ARGS__})
    bool JSL_BUILDER_chain_begin_opt(JSL_BUILDER_Chain *chain, JSL_BUILDER_Chain_Begin_Opt opt);

    typedef struct
    {
        bool err2out;
        bool dont_reset;
    } JSL_BUILDER_Chain_Cmd_Opt;
    #define JSL_BUILDER_chain_cmd(chain, cmd, ...) JSL_BUILDER_chain_cmd_opt((chain), (cmd), JSL_BUILDER_CLIT(JSL_BUILDER_Chain_Cmd_Opt){__VA_ARGS__})
    bool JSL_BUILDER_chain_cmd_opt(JSL_BUILDER_Chain *chain, JSL_BUILDER_Cmd *cmd, JSL_BUILDER_Chain_Cmd_Opt opt);

    typedef struct
    {
        JSL_BUILDER_Procs *async;
        size_t max_procs;
        const char *stdout_path;
        const char *stderr_path;
    } JSL_BUILDER_Chain_End_Opt;
    #define JSL_BUILDER_chain_end(chain, ...) JSL_BUILDER_chain_end_opt((chain), JSL_BUILDER_CLIT(JSL_BUILDER_Chain_End_Opt){__VA_ARGS__})
    bool JSL_BUILDER_chain_end_opt(JSL_BUILDER_Chain *chain, JSL_BUILDER_Chain_End_Opt opt);

    // Get amount of processors on the machine.
    int JSL_BUILDER_nprocs(void);

    #define JSL_BUILDER_NANOS_PER_SEC (1000 * 1000 * 1000)

    // The maximum time span representable is 584 years.
    uint64_t JSL_BUILDER_nanos_since_unspecified_epoch(void);

    // Same as JSL_BUILDER_cmd_run_opt but using cool variadic macro to set the default options.
    // See https://x.com/vkrajacic/status/1749816169736073295 for more info on how to use such macros.
    #define JSL_BUILDER_cmd_run(cmd, ...) JSL_BUILDER_cmd_run_opt((cmd), JSL_BUILDER_CLIT(JSL_BUILDER_Cmd_Opt){__VA_ARGS__})

    // DEPRECATED:
    //
    // You were suppose to use this structure like this:
    //
    // ```c
    // JSL_BUILDER_Fd fdin = JSL_BUILDER_fd_open_for_read("input.txt");
    // if (fdin == JSL_BUILDER_INVALID_FD) fail();
    // JSL_BUILDER_Fd fdout = JSL_BUILDER_fd_open_for_write("output.txt");
    // if (fdout == JSL_BUILDER_INVALID_FD) fail();
    // JSL_BUILDER_Cmd cmd = {0};
    // JSL_BUILDER_cmd_append(&cmd, "cat");
    // if (!JSL_BUILDER_cmd_run_sync_redirect_and_reset(&cmd, (JSL_BUILDER_Cmd_Redirect) {
    //     .fdin = &fdin,
    //     .fdout = &fdout
    // })) fail();
    // ```
    //
    // But these days you should do:
    //
    // ```c
    // JSL_BUILDER_Cmd cmd = {0};
    // JSL_BUILDER_cmd_append(&cmd, "cat");
    // if (!JSL_BUILDER_cmd_run(&cmd, .stdin_path = "input.txt", .stdout_path = "output.txt")) fail();
    // ```
    typedef struct
    {
        JSL_BUILDER_Fd *fdin;
        JSL_BUILDER_Fd *fdout;
        JSL_BUILDER_Fd *fderr;
    } JSL_BUILDER_Cmd_Redirect;

    // Render a string representation of a command into a string builder. Keep in mind the the
    // string builder is not NULL-terminated by default. Use JSL_BUILDER_sb_append_null if you plan to
    // use it as a C string.
    void JSL_BUILDER_cmd_render(JSL_BUILDER_Cmd cmd, JSL_BUILDER_String_Builder *render);

    // Compound Literal
    #if defined(__cplusplus)
    #define JSL_BUILDER_CLIT(type) type
    #else
    #define JSL_BUILDER_CLIT(type) (type)
    #endif

    void JSL_BUILDER__cmd_append(JSL_BUILDER_Cmd *cmd, size_t n, const char **args);
    #if defined(__cplusplus)
    template <typename... Args>
    static inline void JSL_BUILDER__cpp_cmd_append_wrapper(JSL_BUILDER_Cmd *cmd, Args... strs)
    {
        const char *args[] = {strs...};
        JSL_BUILDER__cmd_append(cmd, sizeof(args) / sizeof(args[0]), args);
    }
    #define JSL_BUILDER_cmd_append(cmd, ...) JSL_BUILDER__cpp_cmd_append_wrapper(cmd, __VA_ARGS__)
    #else
    #define JSL_BUILDER_cmd_append(cmd, ...) \
        JSL_BUILDER__cmd_append(cmd, sizeof((const char *[]){__VA_ARGS__}) / sizeof(const char *), (const char *[]){__VA_ARGS__})
    #endif // __cplusplus

    // TODO: JSL_BUILDER_cmd_extend() evaluates other_cmd twice
    // It can be fixed by turning JSL_BUILDER_cmd_extend() call into a statement.
    // But that may break backward compatibility of the API.
    #define JSL_BUILDER_cmd_extend(cmd, other_cmd) \
        JSL_BUILDER_da_append_many(cmd, (other_cmd)->items, (other_cmd)->count)

    // Free all the memory allocated by command arguments
    #define JSL_BUILDER_cmd_free(cmd) JSL_BUILDER_FREE(cmd.items)

    // Run command asynchronously
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd, .async = &procs, .dont_reset = true)`.")
    JSL_BUILDER_Proc JSL_BUILDER_cmd_run_async(JSL_BUILDER_Cmd cmd);

    // JSL_BUILDER_cmd_run_async_and_reset() is just like JSL_BUILDER_cmd_run_async() except it also resets cmd.count to 0
    // so the JSL_BUILDER_Cmd instance can be seamlessly used several times in a row
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd, .async = &procs)` intead.")
    JSL_BUILDER_Proc JSL_BUILDER_cmd_run_async_and_reset(JSL_BUILDER_Cmd *cmd);

    // Run redirected command asynchronously
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd, "
                ".async = &procs, "
                ".stdin_path = \"path/to/stdin\", "
                ".stdout_path = \"path/to/stdout\", "
                ".stderr_path = \"path/to/stderr\", "
                ".dont_reset = true"
                ")` instead.")
    JSL_BUILDER_Proc JSL_BUILDER_cmd_run_async_redirect(JSL_BUILDER_Cmd cmd, JSL_BUILDER_Cmd_Redirect redirect);

    // Run redirected command asynchronously and set cmd.count to 0 and close all the opened files
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd, "
                ".async = &procs, "
                ".stdin_path = \"path/to/stdin\", "
                ".stdout_path = \"path/to/stdout\", "
                ".stderr_path = \"path/to/stderr\")` instead.")
    JSL_BUILDER_Proc JSL_BUILDER_cmd_run_async_redirect_and_reset(JSL_BUILDER_Cmd *cmd, JSL_BUILDER_Cmd_Redirect redirect);

    // Run command synchronously
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd, .dont_reset = true)` instead.")
    bool JSL_BUILDER_cmd_run_sync(JSL_BUILDER_Cmd cmd);

    // NOTE: JSL_BUILDER_cmd_run_sync_and_reset() is just like JSL_BUILDER_cmd_run_sync() except it also resets cmd.count to 0
    // so the JSL_BUILDER_Cmd instance can be seamlessly used several times in a row
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd)` instead.")
    bool JSL_BUILDER_cmd_run_sync_and_reset(JSL_BUILDER_Cmd *cmd);

    // Run redirected command synchronously
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd, "
                ".stdin_path  = \"path/to/stdin\", "
                ".stdout_path = \"path/to/stdout\", "
                ".stderr_path = \"path/to/stderr\", "
                ".dont_reset = true"
                ")` instead.")
    bool JSL_BUILDER_cmd_run_sync_redirect(JSL_BUILDER_Cmd cmd, JSL_BUILDER_Cmd_Redirect redirect);

    // Run redirected command synchronously and set cmd.count to 0 and close all the opened files
    JSL_BUILDER_DEPRECATED("Use `JSL_BUILDER_cmd_run(&cmd, "
                ".stdin_path = \"path/to/stdin\", "
                ".stdout_path = \"path/to/stdout\", "
                ".stderr_path = \"path/to/stderr\")` instead.")
    bool JSL_BUILDER_cmd_run_sync_redirect_and_reset(JSL_BUILDER_Cmd *cmd, JSL_BUILDER_Cmd_Redirect redirect);

    #ifndef JSL_BUILDER_TEMP_CAPACITY
    #define JSL_BUILDER_TEMP_CAPACITY (8 * 1024 * 1024)
    #endif // JSL_BUILDER_TEMP_CAPACITY
    char *JSL_BUILDER_temp_strdup(const char *cstr);
    char *JSL_BUILDER_temp_strndup(const char *cstr, size_t size);
    void *JSL_BUILDER_temp_alloc(size_t size);
    char *JSL_BUILDER_temp_sprintf(const char *format, ...) JSL_BUILDER_PRINTF_FORMAT(1, 2);
    char *JSL_BUILDER_temp_vsprintf(const char *format, va_list ap);
    // JSL_BUILDER_temp_reset() - Resets the entire temporary storage to 0.
    //
    // It is generally not recommended to call this function ever. What you usually want to do is let's say you have a loop,
    // that allocates some temporary objects and cleans them up at the end of each iteration. You should use
    // JSL_BUILDER_temp_save() and JSL_BUILDER_temp_rewind() to organize such loop like this:
    //
    // ```c
    // char *message = JSL_BUILDER_temp_sprintf("This message is still valid after the loop below");
    // while (!quit) {
    //     size_t mark = JSL_BUILDER_temp_save();
    //     JSL_BUILDER_temp_alloc(69);
    //     JSL_BUILDER_temp_alloc(420);
    //     JSL_BUILDER_temp_alloc(1337);
    //     JSL_BUILDER_temp_rewind(mark);
    // }
    // printf("%s\n", message);
    // ```
    //
    // That way all the temporary allocations created before the loop are still valid even after the loop.
    // Such save/rewind blocks define lifetime boundaries of the temporary objects which also could be nested.
    // This turns the temporary storage into kind of a second stack with a more manual management.
    void JSL_BUILDER_temp_reset(void);
    size_t JSL_BUILDER_temp_save(void);
    void JSL_BUILDER_temp_rewind(size_t checkpoint);

    // Given any path returns the last part of that path.
    // "/path/to/a/file.c" -> "file.c"; "/path/to/a/directory" -> "directory"
    const char *JSL_BUILDER_path_name(const char *path);
    bool JSL_BUILDER_rename(const char *old_path, const char *new_path);
    int JSL_BUILDER_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count);
    int JSL_BUILDER_needs_rebuild1(const char *output_path, const char *input_path);
    int JSL_BUILDER_file_exists(const char *file_path);
    const char *JSL_BUILDER_get_current_dir_temp(void);
    bool JSL_BUILDER_set_current_dir(const char *path);
    // Returns you the directory part of the path allocated on the temporary storage.
    char *JSL_BUILDER_temp_dir_name(const char *path);
    char *JSL_BUILDER_temp_file_name(const char *path);
    char *JSL_BUILDER_temp_file_ext(const char *path);
    char *JSL_BUILDER_temp_running_executable_path(void);

    // TODO: we should probably document somewhere all the compilers we support

    // The JSL_BUILDER_cc_* macros try to abstract away the specific compiler.
    // They are verify basic and not particularly flexible, but you can redefine them if you need to
    // or not use them at all and create your own abstraction on top of JSL_BUILDER_Cmd.

    #ifndef JSL_BUILDER_cc
    #if _WIN32
    #if defined(__GNUC__)
    #define JSL_BUILDER_cc(cmd) JSL_BUILDER_cmd_append(cmd, "cc")
    #elif defined(__clang__)
    #define JSL_BUILDER_cc(cmd) JSL_BUILDER_cmd_append(cmd, "clang")
    #elif defined(_MSC_VER)
    #define JSL_BUILDER_cc(cmd) JSL_BUILDER_cmd_append(cmd, "cl.exe")
    #elif defined(__TINYC__)
    #define JSL_BUILDER_cc(cmd) JSL_BUILDER_cmd_append(cmd, "tcc")
    #endif
    #else
    #define JSL_BUILDER_cc(cmd) JSL_BUILDER_cmd_append(cmd, "cc")
    #endif
    #endif // JSL_BUILDER_cc

    #ifndef JSL_BUILDER_cc_flags
    #if defined(_MSC_VER) && !defined(__clang__)
    #define JSL_BUILDER_cc_flags(cmd) JSL_BUILDER_cmd_append(cmd, "/W4", "/nologo", "/D_CRT_SECURE_NO_WARNINGS")
    #else
    #define JSL_BUILDER_cc_flags(cmd) JSL_BUILDER_cmd_append(cmd, "-Wall", "-Wextra")
    #endif
    #endif // JSL_BUILDER_cc_flags

    #ifndef JSL_BUILDER_cc_output
    #if defined(_MSC_VER) && !defined(__clang__)
    #define JSL_BUILDER_cc_output(cmd, output_path) JSL_BUILDER_cmd_append(cmd, JSL_BUILDER_temp_sprintf("/Fe:%s", (output_path)), JSL_BUILDER_temp_sprintf("/Fo:%s", (output_path)))
    #else
    #define JSL_BUILDER_cc_output(cmd, output_path) JSL_BUILDER_cmd_append(cmd, "-o", (output_path))
    #endif
    #endif // JSL_BUILDER_cc_output

    #ifndef JSL_BUILDER_cc_inputs
    #define JSL_BUILDER_cc_inputs(cmd, ...) JSL_BUILDER_cmd_append(cmd, __VA_ARGS__)
    #endif // JSL_BUILDER_cc_inputs

    // TODO: add MinGW support for Go Rebuild Urself™ Technology and all the JSL_BUILDER_cc_* macros above
    //   Musializer contributors came up with a pretty interesting idea of an optional prefix macro which could be useful for
    //   MinGW support:
    //   https://github.com/tsoding/musializer/blob/b7578cc76b9ecb573d239acc9ccf5a04d3aba2c9/src_build/JSL_BUILDER_win64_mingw.c#L3-L9
    // TODO: Maybe instead JSL_BUILDER_REBUILD_URSELF macro, the Go Rebuild Urself™ Technology should use the
    //   user defined JSL_BUILDER_cc_* macros instead?
    #ifndef JSL_BUILDER_REBUILD_URSELF
    #if defined(_WIN32)
    #if defined(__clang__)
    #if defined(__cplusplus)
    #define JSL_BUILDER_REBUILD_URSELF(binary_path, source_path) "clang", "-x", "c++", "-o", binary_path, source_path
    #else
    #define JSL_BUILDER_REBUILD_URSELF(binary_path, source_path) "clang", "-x", "c", "-o", binary_path, source_path
    #endif
    #elif defined(__GNUC__)
    #if defined(__cplusplus)
    #define JSL_BUILDER_REBUILD_URSELF(binary_path, source_path) "gcc", "-x", "c++", "-o", binary_path, source_path
    #else
    #define JSL_BUILDER_REBUILD_URSELF(binary_path, source_path) "gcc", "-x", "c", "-o", binary_path, source_path
    #endif
    #elif defined(_MSC_VER)
    #define JSL_BUILDER_REBUILD_URSELF(binary_path, source_path) "cl.exe", JSL_BUILDER_temp_sprintf("/Fe:%s", (binary_path)), source_path
    #elif defined(__TINYC__)
    #define JSL_BUILDER_REBUILD_URSELF(binary_path, source_path) "tcc", "-o", binary_path, source_path
    #endif
    #else
    #if defined(__cplusplus)
    #define JSL_BUILDER_REBUILD_URSELF(binary_path, source_path) "cc", "-x", "c++", "-o", binary_path, source_path
    #else
    #define JSL_BUILDER_REBUILD_URSELF(binary_path, source_path) "cc", "-x", "c", "-o", binary_path, source_path
    #endif
    #endif
    #endif

    // Go Rebuild Urself™ Technology
    //
    //   How to use it:
    //     int main(int argc, char** argv) {
    //         JSL_BUILDER_GO_REBUILD_URSELF(argc, argv);
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
    //   The rebuilding is done by using the JSL_BUILDER_REBUILD_URSELF macro which you can redefine
    //   if you need a special way of bootstraping your build system. (which I personally
    //   do not recommend since the whole idea of NoBuild is to keep the process of bootstrapping
    //   as simple as possible and doing all of the actual work inside of ./nob)
    //
    void JSL_BUILDER__go_rebuild_urself(int argc, char **argv, const char *source_path, ...);
    #define JSL_BUILDER_GO_REBUILD_URSELF(argc, argv) JSL_BUILDER__go_rebuild_urself(argc, argv, __FILE__, NULL)
    // Sometimes your nob.c includes additional files, so you want the Go Rebuild Urself™ Technology to check
    // if they also were modified and rebuild nob.c accordingly. For that we have JSL_BUILDER_GO_REBUILD_URSELF_PLUS():
    // ```c
    // #define JSL_BUILDER_IMPLEMENTATION
    // #include "nob.h"
    //
    // #include "foo.c"
    // #include "bar.c"
    //
    // int main(int argc, char **argv)
    // {
    //     JSL_BUILDER_GO_REBUILD_URSELF_PLUS(argc, argv, "foo.c", "bar.c");
    //     // ...
    //     return 0;
    // }
    #define JSL_BUILDER_GO_REBUILD_URSELF_PLUS(argc, argv, ...) JSL_BUILDER__go_rebuild_urself(argc, argv, __FILE__, __VA_ARGS__, NULL);

    typedef struct
    {
        size_t count;
        const char *data;
    } JSL_BUILDER_String_View;

    const char *JSL_BUILDER_temp_sv_to_cstr(JSL_BUILDER_String_View sv);

    JSL_BUILDER_String_View JSL_BUILDER_sv_chop_while(JSL_BUILDER_String_View *sv, int (*p)(int x));
    JSL_BUILDER_String_View JSL_BUILDER_sv_chop_by_delim(JSL_BUILDER_String_View *sv, char delim);
    JSL_BUILDER_String_View JSL_BUILDER_sv_chop_left(JSL_BUILDER_String_View *sv, size_t n);
    JSL_BUILDER_String_View JSL_BUILDER_sv_chop_right(JSL_BUILDER_String_View *sv, size_t n);
    // If `sv` starts with `prefix` chops off the prefix and returns true.
    // Otherwise, leaves `sv` unmodified and returns false.
    bool JSL_BUILDER_sv_chop_prefix(JSL_BUILDER_String_View *sv, JSL_BUILDER_String_View prefix);
    // If `sv` ends with `suffix` chops off the suffix and returns true.
    // Otherwise, leaves `sv` unmodified and returns false.
    bool JSL_BUILDER_sv_chop_suffix(JSL_BUILDER_String_View *sv, JSL_BUILDER_String_View suffix);
    JSL_BUILDER_String_View JSL_BUILDER_sv_trim(JSL_BUILDER_String_View sv);
    JSL_BUILDER_String_View JSL_BUILDER_sv_trim_left(JSL_BUILDER_String_View sv);
    JSL_BUILDER_String_View JSL_BUILDER_sv_trim_right(JSL_BUILDER_String_View sv);
    bool JSL_BUILDER_sv_eq(JSL_BUILDER_String_View a, JSL_BUILDER_String_View b);
    JSL_BUILDER_DEPRECATED("Use JSL_BUILDER_sv_ends_with_cstr(sv, suffix) instead. "
                "Pay attention to the `s` at the end of the `end`. "
                "The reason this function was deprecated is because "
                "of the typo in the name, of course, but also "
                "because the second argument was a NULL-terminated string "
                "while JSL_BUILDER_sv_starts_with() accepted JSL_BUILDER_String_View as the "
                "prefix which created an inconsistency in the API.")
    bool JSL_BUILDER_sv_end_with(JSL_BUILDER_String_View sv, const char *cstr);
    bool JSL_BUILDER_sv_ends_with_cstr(JSL_BUILDER_String_View sv, const char *cstr);
    bool JSL_BUILDER_sv_ends_with(JSL_BUILDER_String_View sv, JSL_BUILDER_String_View suffix);
    bool JSL_BUILDER_sv_starts_with(JSL_BUILDER_String_View sv, JSL_BUILDER_String_View prefix);
    JSL_BUILDER_String_View JSL_BUILDER_sv_from_cstr(const char *cstr);
    JSL_BUILDER_String_View JSL_BUILDER_sv_from_parts(const char *data, size_t count);
    // JSL_BUILDER_sb_to_sv() enables you to just view JSL_BUILDER_String_Builder as JSL_BUILDER_String_View
    #define JSL_BUILDER_sb_to_sv(sb) JSL_BUILDER_sv_from_parts((sb).items, (sb).count)

    // printf macros for String_View
    #ifndef SV_Fmt
    #define SV_Fmt "%.*s"
    #endif // SV_Fmt
    #ifndef SV_Arg
    #define SV_Arg(sv) (int)(sv).count, (sv).data
    #endif // SV_Arg
    // USAGE:
    //   String_View name = ...;
    //   printf("Name: "SV_Fmt"\n", SV_Arg(name));

    #ifdef _WIN32

    char *JSL_BUILDER_win32_error_message(DWORD err);

    #endif // _WIN32

#endif // JSL_BUILDER_H_

#ifdef JSL_BUILDER_IMPLEMENTATION

    // This is like JSL_BUILDER_proc_wait() but waits asynchronously. Depending on the platform ms means different thing.
    // On Windows it means timeout. On POSIX it means for how long to sleep after checking if the process exited,
    // so to not peg the core too much. Since this API is kinda of weird, the function is private for now.
    static int JSL_BUILDER__proc_wait_async(JSL_BUILDER_Proc proc, int ms);

    // Starts the process for the command. Its main purpose is to be the base for JSL_BUILDER_cmd_run() and JSL_BUILDER_cmd_run_opt().
    static JSL_BUILDER_Proc JSL_BUILDER__cmd_start_process(JSL_BUILDER_Cmd cmd, JSL_BUILDER_Fd *fdin, JSL_BUILDER_Fd *fdout, JSL_BUILDER_Fd *fderr);

    // Any messages with the level below JSL_BUILDER_minimal_log_level are going to be suppressed.
    JSL_BUILDER_Log_Level JSL_BUILDER_minimal_log_level = JSL_BUILDER_INFO;

    void JSL_BUILDER__cmd_append(JSL_BUILDER_Cmd *cmd, size_t n, const char **args)
    {
        for (size_t i = 0; i < n; ++i)
        {
            JSL_BUILDER_da_append(cmd, args[i]);
        }
    }

    #ifdef _WIN32

    // Base on https://stackoverflow.com/a/75644008
    // > .NET Core uses 4096 * sizeof(WCHAR) buffer on stack for FormatMessageW call. And...thats it.
    // >
    // > https://github.com/dotnet/runtime/blob/3b63eb1346f1ddbc921374a5108d025662fb5ffd/src/coreclr/utilcode/posterror.cpp#L264-L265
    #ifndef JSL_BUILDER_WIN32_ERR_MSG_SIZE
    #define JSL_BUILDER_WIN32_ERR_MSG_SIZE (4 * 1024)
    #endif // JSL_BUILDER_WIN32_ERR_MSG_SIZE

    char *JSL_BUILDER_win32_error_message(DWORD err)
    {
        static char win32ErrMsg[JSL_BUILDER_WIN32_ERR_MSG_SIZE] = {0};
        DWORD errMsgSize = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, LANG_USER_DEFAULT, win32ErrMsg,
                                        JSL_BUILDER_WIN32_ERR_MSG_SIZE, NULL);

        if (errMsgSize == 0)
        {
            if (GetLastError() != ERROR_MR_MID_NOT_FOUND)
            {
                if (sprintf(win32ErrMsg, "Could not get error message for 0x%lX", err) > 0)
                {
                    return (char *)&win32ErrMsg;
                }
                else
                {
                    return NULL;
                }
            }
            else
            {
                if (sprintf(win32ErrMsg, "Invalid Windows Error code (0x%lX)", err) > 0)
                {
                    return (char *)&win32ErrMsg;
                }
                else
                {
                    return NULL;
                }
            }
        }

        while (errMsgSize > 1 && isspace(win32ErrMsg[errMsgSize - 1]))
        {
            win32ErrMsg[--errMsgSize] = '\0';
        }

        return win32ErrMsg;
    }

    #endif // _WIN32

    // The implementation idea is stolen from https://github.com/zhiayang/nabs
    void JSL_BUILDER__go_rebuild_urself(int argc, char **argv, const char *source_path, ...)
    {
        const char *binary_path = JSL_BUILDER_shift(argv, argc);
    #ifdef _WIN32
        // On Windows executables almost always invoked without extension, so
        // it's ./nob, not ./nob.exe. For renaming the extension is a must.
        if (!JSL_BUILDER_sv_ends_with_cstr(JSL_BUILDER_sv_from_cstr(binary_path), ".exe"))
        {
            binary_path = JSL_BUILDER_temp_sprintf("%s.exe", binary_path);
        }
    #endif

        JSL_BUILDER_File_Paths source_paths = {0};
        JSL_BUILDER_da_append(&source_paths, source_path);
        va_list args;
        va_start(args, source_path);
        for (;;)
        {
            const char *path = va_arg(args, const char *);
            if (path == NULL)
                break;
            JSL_BUILDER_da_append(&source_paths, path);
        }
        va_end(args);

        int rebuild_is_needed = JSL_BUILDER_needs_rebuild(binary_path, source_paths.items, source_paths.count);
        if (rebuild_is_needed < 0)
            exit(1); // error
        if (!rebuild_is_needed)
        { // no rebuild is needed
            JSL_BUILDER_FREE(source_paths.items);
            return;
        }

        JSL_BUILDER_Cmd cmd = {0};

        const char *old_binary_path = JSL_BUILDER_temp_sprintf("%s.old", binary_path);

        if (!JSL_BUILDER_rename(binary_path, old_binary_path))
            exit(1);
        JSL_BUILDER_cmd_append(&cmd, JSL_BUILDER_REBUILD_URSELF(binary_path, source_path));
        JSL_BUILDER_Cmd_Opt opt = {0};
        if (!JSL_BUILDER_cmd_run_opt(&cmd, opt))
        {
            JSL_BUILDER_rename(old_binary_path, binary_path);
            exit(1);
        }
    #ifdef JSL_BUILDER_EXPERIMENTAL_DELETE_OLD
        // TODO: this is an experimental behavior behind a compilation flag.
        // Once it is confirmed that it does not cause much problems on both POSIX and Windows
        // we may turn it on by default.
        JSL_BUILDER_delete_file(old_binary_path);
    #endif // JSL_BUILDER_EXPERIMENTAL_DELETE_OLD

        JSL_BUILDER_cmd_append(&cmd, binary_path);
        JSL_BUILDER_da_append_many(&cmd, argv, argc);
        if (!JSL_BUILDER_cmd_run_opt(&cmd, opt))
            exit(1);
        exit(0);
    }

    static size_t JSL_BUILDER_temp_size = 0;
    static char JSL_BUILDER_temp[JSL_BUILDER_TEMP_CAPACITY] = {0};

    bool JSL_BUILDER_mkdir_if_not_exists(const char *path)
    {
    #ifdef _WIN32
        int result = _mkdir(path);
    #else
        int result = mkdir(path, 0755);
    #endif
        if (result < 0)
        {
            if (errno == EEXIST)
            {
    #ifndef JSL_BUILDER_NO_ECHO
                JSL_BUILDER_log(JSL_BUILDER_INFO, "directory `%s` already exists", path);
    #endif // JSL_BUILDER_NO_ECHO
                return true;
            }
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not create directory `%s`: %s", path, strerror(errno));
            return false;
        }

    #ifndef JSL_BUILDER_NO_ECHO
        JSL_BUILDER_log(JSL_BUILDER_INFO, "created directory `%s`", path);
    #endif // JSL_BUILDER_NO_ECHO
        return true;
    }

    bool JSL_BUILDER_copy_file(const char *src_path, const char *dst_path)
    {
    #ifndef JSL_BUILDER_NO_ECHO
        JSL_BUILDER_log(JSL_BUILDER_INFO, "copying %s -> %s", src_path, dst_path);
    #endif // JSL_BUILDER_NO_ECHO
    #ifdef _WIN32
        if (!CopyFile(src_path, dst_path, FALSE))
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not copy file: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            return false;
        }
        return true;
    #else
        int src_fd = -1;
        int dst_fd = -1;
        size_t buf_size = 32 * 1024;
        char *buf = (char *)JSL_BUILDER_REALLOC(NULL, buf_size);
        JSL_ASSERT(buf != NULL && "Buy more RAM lol!!");
        bool result = true;

        src_fd = open(src_path, O_RDONLY);
        if (src_fd < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s: %s", src_path, strerror(errno));
            JSL_BUILDER_return_defer(false);
        }

        struct stat src_stat;
        if (fstat(src_fd, &src_stat) < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not get mode of file %s: %s", src_path, strerror(errno));
            JSL_BUILDER_return_defer(false);
        }

        dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
        if (dst_fd < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not create file %s: %s", dst_path, strerror(errno));
            JSL_BUILDER_return_defer(false);
        }

        for (;;)
        {
            ssize_t n = read(src_fd, buf, buf_size);
            if (n == 0)
                break;
            if (n < 0)
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not read from file %s: %s", src_path, strerror(errno));
                JSL_BUILDER_return_defer(false);
            }
            char *buf2 = buf;
            while (n > 0)
            {
                ssize_t m = write(dst_fd, buf2, n);
                if (m < 0)
                {
                    JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not write to file %s: %s", dst_path, strerror(errno));
                    JSL_BUILDER_return_defer(false);
                }
                n -= m;
                buf2 += m;
            }
        }

    defer:
        JSL_BUILDER_FREE(buf);
        close(src_fd);
        close(dst_fd);
        return result;
    #endif
    }

    void JSL_BUILDER_cmd_render(JSL_BUILDER_Cmd cmd, JSL_BUILDER_String_Builder *render)
    {
        for (size_t i = 0; i < cmd.count; ++i)
        {
            const char *arg = cmd.items[i];
            if (arg == NULL)
                break;
            if (i > 0)
                JSL_BUILDER_sb_append_cstr(render, " ");
            if (!strchr(arg, ' '))
            {
                JSL_BUILDER_sb_append_cstr(render, arg);
            }
            else
            {
                JSL_BUILDER_da_append(render, '\'');
                JSL_BUILDER_sb_append_cstr(render, arg);
                JSL_BUILDER_da_append(render, '\'');
            }
        }
    }

    #ifdef _WIN32
    // https://learn.microsoft.com/en-gb/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way
    static void JSL_BUILDER__win32_cmd_quote(JSL_BUILDER_Cmd cmd, JSL_BUILDER_String_Builder *quoted)
    {
        for (size_t i = 0; i < cmd.count; ++i)
        {
            const char *arg = cmd.items[i];
            if (arg == NULL)
                break;
            size_t len = strlen(arg);
            if (i > 0)
                JSL_BUILDER_da_append(quoted, ' ');
            if (len != 0 && NULL == strpbrk(arg, " \t\n\v\""))
            {
                // no need to quote
                JSL_BUILDER_da_append_many(quoted, arg, len);
            }
            else
            {
                // we need to escape:
                // 1. double quotes in the original arg
                // 2. consequent backslashes before a double quote
                size_t backslashes = 0;
                JSL_BUILDER_da_append(quoted, '\"');
                for (size_t j = 0; j < len; ++j)
                {
                    char x = arg[j];
                    if (x == '\\')
                    {
                        backslashes += 1;
                    }
                    else
                    {
                        if (x == '\"')
                        {
                            // escape backslashes (if any) and the double quote
                            for (size_t k = 0; k < 1 + backslashes; ++k)
                            {
                                JSL_BUILDER_da_append(quoted, '\\');
                            }
                        }
                        backslashes = 0;
                    }
                    JSL_BUILDER_da_append(quoted, x);
                }
                // escape backslashes (if any)
                for (size_t k = 0; k < backslashes; ++k)
                {
                    JSL_BUILDER_da_append(quoted, '\\');
                }
                JSL_BUILDER_da_append(quoted, '\"');
            }
        }
    }
    #endif

    int JSL_BUILDER_nprocs(void)
    {
    #ifdef _WIN32
        SYSTEM_INFO siSysInfo;
        GetSystemInfo(&siSysInfo);
        return siSysInfo.dwNumberOfProcessors;
    #else
        return sysconf(_SC_NPROCESSORS_ONLN);
    #endif
    }

    bool JSL_BUILDER_cmd_run_opt(JSL_BUILDER_Cmd *cmd, JSL_BUILDER_Cmd_Opt opt)
    {
        bool result = true;
        JSL_BUILDER_Fd fdin = JSL_BUILDER_INVALID_FD;
        JSL_BUILDER_Fd fdout = JSL_BUILDER_INVALID_FD;
        JSL_BUILDER_Fd fderr = JSL_BUILDER_INVALID_FD;
        JSL_BUILDER_Fd *opt_fdin = NULL;
        JSL_BUILDER_Fd *opt_fdout = NULL;
        JSL_BUILDER_Fd *opt_fderr = NULL;
        JSL_BUILDER_Proc proc = JSL_BUILDER_INVALID_PROC;

        size_t max_procs = opt.max_procs > 0 ? opt.max_procs : (size_t)JSL_BUILDER_nprocs() + 1;

        if (opt.async && max_procs > 0)
        {
            while (opt.async->count >= max_procs)
            {
                for (size_t i = 0; i < opt.async->count; ++i)
                {
                    int ret = JSL_BUILDER__proc_wait_async(opt.async->items[i], 1);
                    if (ret < 0)
                        JSL_BUILDER_return_defer(false);
                    if (ret)
                    {
                        JSL_BUILDER_da_remove_unordered(opt.async, i);
                        break;
                    }
                }
            }
        }

        if (opt.stdin_path)
        {
            fdin = JSL_BUILDER_fd_open_for_read(opt.stdin_path);
            if (fdin == JSL_BUILDER_INVALID_FD)
                JSL_BUILDER_return_defer(false);
            opt_fdin = &fdin;
        }
        if (opt.stdout_path)
        {
            fdout = JSL_BUILDER_fd_open_for_write(opt.stdout_path);
            if (fdout == JSL_BUILDER_INVALID_FD)
                JSL_BUILDER_return_defer(false);
            opt_fdout = &fdout;
        }
        if (opt.stderr_path)
        {
            fderr = JSL_BUILDER_fd_open_for_write(opt.stderr_path);
            if (fderr == JSL_BUILDER_INVALID_FD)
                JSL_BUILDER_return_defer(false);
            opt_fderr = &fderr;
        }
        proc = JSL_BUILDER__cmd_start_process(*cmd, opt_fdin, opt_fdout, opt_fderr);

        if (opt.async)
        {
            if (proc == JSL_BUILDER_INVALID_PROC)
                JSL_BUILDER_return_defer(false);
            JSL_BUILDER_da_append(opt.async, proc);
        }
        else
        {
            if (!JSL_BUILDER_proc_wait(proc))
                JSL_BUILDER_return_defer(false);
        }

    defer:
        if (opt_fdin)
            JSL_BUILDER_fd_close(*opt_fdin);
        if (opt_fdout)
            JSL_BUILDER_fd_close(*opt_fdout);
        if (opt_fderr)
            JSL_BUILDER_fd_close(*opt_fderr);
        if (!opt.dont_reset)
            cmd->count = 0;
        return result;
    }

    bool JSL_BUILDER_chain_begin_opt(JSL_BUILDER_Chain *chain, JSL_BUILDER_Chain_Begin_Opt opt)
    {
        chain->cmd.count = 0;
        chain->err2out = false;
        chain->fdin = JSL_BUILDER_INVALID_FD;
        if (opt.stdin_path)
        {
            chain->fdin = JSL_BUILDER_fd_open_for_read(opt.stdin_path);
            if (chain->fdin == JSL_BUILDER_INVALID_FD)
                return false;
        }
        return true;
    }

    bool JSL_BUILDER_chain_cmd_opt(JSL_BUILDER_Chain *chain, JSL_BUILDER_Cmd *cmd, JSL_BUILDER_Chain_Cmd_Opt opt)
    {
        bool result = true;
        JSL_BUILDER_Pipe pp = {0};
        struct
        {
            size_t count;
            JSL_BUILDER_Fd items[5]; // should be no more than 3, but we allocate 5 just in case
        } fds = {0};

        JSL_ASSERT(cmd->count > 0);

        if (chain->cmd.count != 0)
        { // not first cmd in the chain
            JSL_BUILDER_Fd *pfdin = NULL;
            if (chain->fdin != JSL_BUILDER_INVALID_FD)
            {
                JSL_BUILDER_fa_append(&fds, chain->fdin);
                pfdin = &chain->fdin;
            }
            if (!JSL_BUILDER_pipe_create(&pp))
                JSL_BUILDER_return_defer(false);
            JSL_BUILDER_fa_append(&fds, pp.write);
            JSL_BUILDER_Fd *pfdout = &pp.write;
            JSL_BUILDER_Fd *pfderr = chain->err2out ? pfdout : NULL;

            JSL_BUILDER_Proc proc = JSL_BUILDER__cmd_start_process(chain->cmd, pfdin, pfdout, pfderr);
            chain->cmd.count = 0;
            if (proc == JSL_BUILDER_INVALID_PROC)
            {
                JSL_BUILDER_fa_append(&fds, pp.read);
                JSL_BUILDER_return_defer(false);
            }
            chain->fdin = pp.read;
        }

        JSL_BUILDER_da_append_many(&chain->cmd, cmd->items, cmd->count);
        chain->err2out = opt.err2out;

    defer:
        for (size_t i = 0; i < fds.count; ++i)
        {
            JSL_BUILDER_fd_close(fds.items[i]);
        }
        if (!opt.dont_reset)
            cmd->count = 0;
        return result;
    }

    static JSL_BUILDER_Fd JSL_BUILDER__fd_stdout(void)
    {
    #ifdef _WIN32
        return GetStdHandle(STD_OUTPUT_HANDLE);
    #else
        return STDOUT_FILENO;
    #endif // _WIN32
    }

    bool JSL_BUILDER_chain_end_opt(JSL_BUILDER_Chain *chain, JSL_BUILDER_Chain_End_Opt opt)
    {
        bool result = true;

        JSL_BUILDER_Fd *pfdin = NULL;
        struct
        {
            size_t count;
            JSL_BUILDER_Fd items[5]; // should be no more than 3, but we allocate 5 just in case
        } fds = {0};

        if (chain->fdin != JSL_BUILDER_INVALID_FD)
        {
            JSL_BUILDER_fa_append(&fds, chain->fdin);
            pfdin = &chain->fdin;
        }

        if (chain->cmd.count != 0)
        { // Non-empty chain case
            size_t max_procs = opt.max_procs > 0 ? opt.max_procs : (size_t)JSL_BUILDER_nprocs() + 1;

            if (opt.async && max_procs > 0)
            {
                while (opt.async->count >= max_procs)
                {
                    for (size_t i = 0; i < opt.async->count; ++i)
                    {
                        int ret = JSL_BUILDER__proc_wait_async(opt.async->items[i], 1);
                        if (ret < 0)
                            JSL_BUILDER_return_defer(false);
                        if (ret)
                        {
                            JSL_BUILDER_da_remove_unordered(opt.async, i);
                            break;
                        }
                    }
                }
            }

            JSL_BUILDER_Fd fdout = JSL_BUILDER__fd_stdout();
            if (opt.stdout_path)
            {
                fdout = JSL_BUILDER_fd_open_for_write(opt.stdout_path);
                if (fdout == JSL_BUILDER_INVALID_FD)
                    JSL_BUILDER_return_defer(false);
                JSL_BUILDER_fa_append(&fds, fdout);
            }

            JSL_BUILDER_Fd fderr = 0;
            JSL_BUILDER_Fd *pfderr = NULL;
            if (chain->err2out)
                pfderr = &fdout;
            if (opt.stderr_path)
            {
                if (pfderr == NULL)
                {
                    fderr = JSL_BUILDER_fd_open_for_write(opt.stderr_path);
                    if (fderr == JSL_BUILDER_INVALID_FD)
                        JSL_BUILDER_return_defer(false);
                    JSL_BUILDER_fa_append(&fds, fderr);
                    pfderr = &fderr;
                }
                else
                {
                    // There was err2out set for the last command.
                    // All the stderr will go to stdout.
                    // So the stderr file is going to be empty.
                    JSL_ASSERT(chain->err2out);
                    if (!JSL_BUILDER_write_entire_file(opt.stderr_path, NULL, 0))
                        JSL_BUILDER_return_defer(false);
                }
            }

            JSL_BUILDER_Proc proc = JSL_BUILDER__cmd_start_process(chain->cmd, pfdin, &fdout, pfderr);
            chain->cmd.count = 0;

            if (opt.async)
            {
                if (proc == JSL_BUILDER_INVALID_PROC)
                    JSL_BUILDER_return_defer(false);
                JSL_BUILDER_da_append(opt.async, proc);
            }
            else
            {
                if (!JSL_BUILDER_proc_wait(proc))
                    JSL_BUILDER_return_defer(false);
            }
        }

    defer:
        for (size_t i = 0; i < fds.count; ++i)
        {
            JSL_BUILDER_fd_close(fds.items[i]);
        }
        return result;
    }

    // The maximum time span representable is 584 years.
    uint64_t JSL_BUILDER_nanos_since_unspecified_epoch(void)
    {
    #ifdef _WIN32
        LARGE_INTEGER Time;
        QueryPerformanceCounter(&Time);

        static LARGE_INTEGER Frequency = {0};
        if (Frequency.QuadPart == 0)
        {
            QueryPerformanceFrequency(&Frequency);
        }

        uint64_t Secs = Time.QuadPart / Frequency.QuadPart;
        uint64_t Nanos = Time.QuadPart % Frequency.QuadPart * JSL_BUILDER_NANOS_PER_SEC / Frequency.QuadPart;
        return JSL_BUILDER_NANOS_PER_SEC * Secs + Nanos;
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        return JSL_BUILDER_NANOS_PER_SEC * ts.tv_sec + ts.tv_nsec;
    #endif // _WIN32
    }

    JSL_BUILDER_Proc JSL_BUILDER_cmd_run_async_redirect(JSL_BUILDER_Cmd cmd, JSL_BUILDER_Cmd_Redirect redirect)
    {
        return JSL_BUILDER__cmd_start_process(cmd, redirect.fdin, redirect.fdout, redirect.fderr);
    }

    static JSL_BUILDER_Proc JSL_BUILDER__cmd_start_process(JSL_BUILDER_Cmd cmd, JSL_BUILDER_Fd *fdin, JSL_BUILDER_Fd *fdout, JSL_BUILDER_Fd *fderr)
    {
        if (cmd.count < 1)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not run empty command");
            return JSL_BUILDER_INVALID_PROC;
        }

    #ifndef JSL_BUILDER_NO_ECHO
        JSL_BUILDER_String_Builder sb = {0};
        JSL_BUILDER_cmd_render(cmd, &sb);
        JSL_BUILDER_sb_append_null(&sb);
        JSL_BUILDER_log(JSL_BUILDER_INFO, "CMD: %s", sb.items);
        JSL_BUILDER_sb_free(sb);
        memset(&sb, 0, sizeof(sb));
    #endif // JSL_BUILDER_NO_ECHO

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

        JSL_BUILDER_String_Builder quoted = {0};
        JSL_BUILDER__win32_cmd_quote(cmd, &quoted);
        JSL_BUILDER_sb_append_null(&quoted);
        BOOL bSuccess = CreateProcessA(NULL, quoted.items, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo);
        JSL_BUILDER_sb_free(quoted);

        if (!bSuccess)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not create child process for %s: %s", cmd.items[0], JSL_BUILDER_win32_error_message(GetLastError()));
            return JSL_BUILDER_INVALID_PROC;
        }

        CloseHandle(piProcInfo.hThread);

        return piProcInfo.hProcess;
    #else
        pid_t cpid = fork();
        if (cpid < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not fork child process: %s", strerror(errno));
            return JSL_BUILDER_INVALID_PROC;
        }

        if (cpid == 0)
        {
            if (fdin)
            {
                if (dup2(*fdin, STDIN_FILENO) < 0)
                {
                    JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not setup stdin for child process: %s", strerror(errno));
                    exit(1);
                }
            }

            if (fdout)
            {
                if (dup2(*fdout, STDOUT_FILENO) < 0)
                {
                    JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not setup stdout for child process: %s", strerror(errno));
                    exit(1);
                }
            }

            if (fderr)
            {
                if (dup2(*fderr, STDERR_FILENO) < 0)
                {
                    JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not setup stderr for child process: %s", strerror(errno));
                    exit(1);
                }
            }

            // NOTE: This leaks a bit of memory in the child process.
            // But do we actually care? It's a one off leak anyway...
            JSL_BUILDER_Cmd cmd_null = {0};
            JSL_BUILDER_da_append_many(&cmd_null, cmd.items, cmd.count);
            JSL_BUILDER_cmd_append(&cmd_null, (const char *)NULL);

            if (execvp(cmd.items[0], (char *const *)cmd_null.items) < 0)
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not exec child process for %s: %s", cmd.items[0], strerror(errno));
                exit(1);
            }
            JSL_BUILDER_UNREACHABLE("JSL_BUILDER_cmd_run_async_redirect");
        }

        return cpid;
    #endif
    }

    JSL_BUILDER_Proc JSL_BUILDER_cmd_run_async(JSL_BUILDER_Cmd cmd)
    {
        return JSL_BUILDER__cmd_start_process(cmd, NULL, NULL, NULL);
    }

    JSL_BUILDER_Proc JSL_BUILDER_cmd_run_async_and_reset(JSL_BUILDER_Cmd *cmd)
    {
        JSL_BUILDER_Proc proc = JSL_BUILDER__cmd_start_process(*cmd, NULL, NULL, NULL);
        cmd->count = 0;
        return proc;
    }

    JSL_BUILDER_Proc JSL_BUILDER_cmd_run_async_redirect_and_reset(JSL_BUILDER_Cmd *cmd, JSL_BUILDER_Cmd_Redirect redirect)
    {
        JSL_BUILDER_Proc proc = JSL_BUILDER__cmd_start_process(*cmd, redirect.fdin, redirect.fdout, redirect.fderr);
        cmd->count = 0;
        if (redirect.fdin)
        {
            JSL_BUILDER_fd_close(*redirect.fdin);
            *redirect.fdin = JSL_BUILDER_INVALID_FD;
        }
        if (redirect.fdout)
        {
            JSL_BUILDER_fd_close(*redirect.fdout);
            *redirect.fdout = JSL_BUILDER_INVALID_FD;
        }
        if (redirect.fderr)
        {
            JSL_BUILDER_fd_close(*redirect.fderr);
            *redirect.fderr = JSL_BUILDER_INVALID_FD;
        }
        return proc;
    }

    JSL_BUILDER_Fd JSL_BUILDER_fd_open_for_read(const char *path)
    {
    #ifndef _WIN32
        JSL_BUILDER_Fd result = open(path, O_RDONLY);
        if (result < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s: %s", path, strerror(errno));
            return JSL_BUILDER_INVALID_FD;
        }
        return result;
    #else
        // https://docs.microsoft.com/en-us/windows/win32/fileio/opening-a-file-for-reading-or-writing
        SECURITY_ATTRIBUTES saAttr = {0};
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;

        JSL_BUILDER_Fd result = CreateFile(
            path,
            GENERIC_READ,
            0,
            &saAttr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_READONLY,
            NULL);

        if (result == INVALID_HANDLE_VALUE)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s: %s", path, JSL_BUILDER_win32_error_message(GetLastError()));
            return JSL_BUILDER_INVALID_FD;
        }

        return result;
    #endif // _WIN32
    }

    JSL_BUILDER_Fd JSL_BUILDER_fd_open_for_write(const char *path)
    {
    #ifndef _WIN32
        JSL_BUILDER_Fd result = open(path,
                            O_WRONLY | O_CREAT | O_TRUNC,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (result < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not open file %s: %s", path, strerror(errno));
            return JSL_BUILDER_INVALID_FD;
        }
        return result;
    #else
        SECURITY_ATTRIBUTES saAttr = {0};
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;

        JSL_BUILDER_Fd result = CreateFile(
            path,                  // name of the write
            GENERIC_WRITE,         // open for writing
            0,                     // do not share
            &saAttr,               // default security
            CREATE_ALWAYS,         // create always
            FILE_ATTRIBUTE_NORMAL, // normal file
            NULL                   // no attr. template
        );

        if (result == INVALID_HANDLE_VALUE)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s: %s", path, JSL_BUILDER_win32_error_message(GetLastError()));
            return JSL_BUILDER_INVALID_FD;
        }

        return result;
    #endif // _WIN32
    }

    void JSL_BUILDER_fd_close(JSL_BUILDER_Fd fd)
    {
    #ifdef _WIN32
        CloseHandle(fd);
    #else
        close(fd);
    #endif // _WIN32
    }

    bool JSL_BUILDER_pipe_create(JSL_BUILDER_Pipe *pp)
    {
    #ifdef _WIN32
        // https://docs.microsoft.com/en-us/windows/win32/ProcThread/creating-a-child-process-with-redirected-input-and-output

        SECURITY_ATTRIBUTES saAttr = {0};
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;

        if (!CreatePipe(&pp->read, &pp->write, &saAttr, 0))
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not create pipe: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            return false;
        }

        return true;
    #else
        int pipefd[2];
        if (pipe(pipefd) < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not create pipe: %s\n", strerror(errno));
            return false;
        }

        pp->read = pipefd[0];
        pp->write = pipefd[1];

        return true;
    #endif // _WIN32
    }

    bool JSL_BUILDER_procs_wait(JSL_BUILDER_Procs procs)
    {
        bool success = true;
        for (size_t i = 0; i < procs.count; ++i)
        {
            success = JSL_BUILDER_proc_wait(procs.items[i]) && success;
        }
        return success;
    }

    bool JSL_BUILDER_procs_flush(JSL_BUILDER_Procs *procs)
    {
        bool success = JSL_BUILDER_procs_wait(*procs);
        procs->count = 0;
        return success;
    }

    bool JSL_BUILDER_procs_wait_and_reset(JSL_BUILDER_Procs *procs)
    {
        return JSL_BUILDER_procs_flush(procs);
    }

    bool JSL_BUILDER_proc_wait(JSL_BUILDER_Proc proc)
    {
        if (proc == JSL_BUILDER_INVALID_PROC)
            return false;

    #ifdef _WIN32
        DWORD result = WaitForSingleObject(
            proc,    // HANDLE hHandle,
            INFINITE // DWORD  dwMilliseconds
        );

        if (result == WAIT_FAILED)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not wait on child process: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            return false;
        }

        DWORD exit_status;
        if (!GetExitCodeProcess(proc, &exit_status))
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not get process exit code: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            return false;
        }

        if (exit_status != 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "command exited with exit code %lu", exit_status);
            return false;
        }

        CloseHandle(proc);

        return true;
    #else
        for (;;)
        {
            int wstatus = 0;
            if (waitpid(proc, &wstatus, 0) < 0)
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
                return false;
            }

            if (WIFEXITED(wstatus))
            {
                int exit_status = WEXITSTATUS(wstatus);
                if (exit_status != 0)
                {
                    JSL_BUILDER_log(JSL_BUILDER_ERROR, "command exited with exit code %d", exit_status);
                    return false;
                }

                break;
            }

            if (WIFSIGNALED(wstatus))
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
                return false;
            }
        }

        return true;
    #endif
    }

    static int JSL_BUILDER__proc_wait_async(JSL_BUILDER_Proc proc, int ms)
    {
        if (proc == JSL_BUILDER_INVALID_PROC)
            return false;

    #ifdef _WIN32
        DWORD result = WaitForSingleObject(
            proc, // HANDLE hHandle,
            ms    // DWORD  dwMilliseconds
        );

        if (result == WAIT_TIMEOUT)
        {
            return 0;
        }

        if (result == WAIT_FAILED)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not wait on child process: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            return -1;
        }

        DWORD exit_status;
        if (!GetExitCodeProcess(proc, &exit_status))
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not get process exit code: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            return -1;
        }

        if (exit_status != 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "command exited with exit code %lu", exit_status);
            return -1;
        }

        CloseHandle(proc);

        return 1;
    #else
        long ns = ms * 1000 * 1000;
        struct timespec duration = {
            .tv_sec = ns / (1000 * 1000 * 1000),
            .tv_nsec = ns % (1000 * 1000 * 1000),
        };

        int wstatus = 0;
        pid_t pid = waitpid(proc, &wstatus, WNOHANG);
        if (pid < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
            return -1;
        }

        if (pid == 0)
        {
            nanosleep(&duration, NULL);
            return 0;
        }

        if (WIFEXITED(wstatus))
        {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0)
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "command exited with exit code %d", exit_status);
                return -1;
            }

            return 1;
        }

        if (WIFSIGNALED(wstatus))
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
            return -1;
        }

        nanosleep(&duration, NULL);
        return 0;
    #endif
    }

    bool JSL_BUILDER_procs_append_with_flush(JSL_BUILDER_Procs *procs, JSL_BUILDER_Proc proc, size_t max_procs_count)
    {
        JSL_BUILDER_da_append(procs, proc);

        if (procs->count >= max_procs_count)
        {
            if (!JSL_BUILDER_procs_flush(procs))
                return false;
        }

        return true;
    }

    bool JSL_BUILDER_cmd_run_sync_redirect(JSL_BUILDER_Cmd cmd, JSL_BUILDER_Cmd_Redirect redirect)
    {
        JSL_BUILDER_Proc p = JSL_BUILDER__cmd_start_process(cmd, redirect.fdin, redirect.fdout, redirect.fderr);
        return JSL_BUILDER_proc_wait(p);
    }

    bool JSL_BUILDER_cmd_run_sync(JSL_BUILDER_Cmd cmd)
    {
        JSL_BUILDER_Proc p = JSL_BUILDER__cmd_start_process(cmd, NULL, NULL, NULL);
        return JSL_BUILDER_proc_wait(p);
    }

    bool JSL_BUILDER_cmd_run_sync_and_reset(JSL_BUILDER_Cmd *cmd)
    {
        JSL_BUILDER_Proc p = JSL_BUILDER__cmd_start_process(*cmd, NULL, NULL, NULL);
        cmd->count = 0;
        return JSL_BUILDER_proc_wait(p);
    }

    bool JSL_BUILDER_cmd_run_sync_redirect_and_reset(JSL_BUILDER_Cmd *cmd, JSL_BUILDER_Cmd_Redirect redirect)
    {
        JSL_BUILDER_Proc p = JSL_BUILDER__cmd_start_process(*cmd, redirect.fdin, redirect.fdout, redirect.fderr);
        cmd->count = 0;
        if (redirect.fdin)
        {
            JSL_BUILDER_fd_close(*redirect.fdin);
            *redirect.fdin = JSL_BUILDER_INVALID_FD;
        }
        if (redirect.fdout)
        {
            JSL_BUILDER_fd_close(*redirect.fdout);
            *redirect.fdout = JSL_BUILDER_INVALID_FD;
        }
        if (redirect.fderr)
        {
            JSL_BUILDER_fd_close(*redirect.fderr);
            *redirect.fderr = JSL_BUILDER_INVALID_FD;
        }
        return JSL_BUILDER_proc_wait(p);
    }

    static JSL_BUILDER_Log_Handler *JSL_BUILDER__log_handler = &JSL_BUILDER_default_log_handler;

    void JSL_BUILDER_set_log_handler(JSL_BUILDER_Log_Handler *handler)
    {
        JSL_BUILDER__log_handler = handler;
    }

    JSL_BUILDER_Log_Handler *JSL_BUILDER_get_log_handler(void)
    {
        return JSL_BUILDER__log_handler;
    }

    void JSL_BUILDER_default_log_handler(JSL_BUILDER_Log_Level level, const char *fmt, va_list args)
    {
        if (level < JSL_BUILDER_minimal_log_level)
            return;

        switch (level)
        {
        case JSL_BUILDER_INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case JSL_BUILDER_WARNING:
            fprintf(stderr, "[WARNING] ");
            break;
        case JSL_BUILDER_ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
        case JSL_BUILDER_NO_LOGS:
            return;
        default:
            JSL_BUILDER_UNREACHABLE("JSL_BUILDER_Log_Level");
        }

        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
    }

    void JSL_BUILDER_null_log_handler(JSL_BUILDER_Log_Level level, const char *fmt, va_list args)
    {
        JSL_BUILDER_UNUSED(level);
        JSL_BUILDER_UNUSED(fmt);
        JSL_BUILDER_UNUSED(args);
    }

    void JSL_BUILDER_cancer_log_handler(JSL_BUILDER_Log_Level level, const char *fmt, va_list args)
    {
        switch (level)
        {
        case JSL_BUILDER_INFO:
            fprintf(stderr, "ℹ️ \x1b[36m[INFO]\x1b[0m ");
            break;
        case JSL_BUILDER_WARNING:
            fprintf(stderr, "⚠️ \x1b[33m[WARNING]\x1b[0m ");
            break;
        case JSL_BUILDER_ERROR:
            fprintf(stderr, "🚨 \x1b[31m[ERROR]\x1b[0m ");
            break;
        case JSL_BUILDER_NO_LOGS:
            return;
        default:
            JSL_BUILDER_UNREACHABLE("JSL_BUILDER_Log_Level");
        }

        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
    }

    void JSL_BUILDER_log(JSL_BUILDER_Log_Level level, const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        JSL_BUILDER__log_handler(level, fmt, args);
        va_end(args);
    }

    bool JSL_BUILDER_dir_entry_open(const char *dir_path, JSL_BUILDER_Dir_Entry *dir)
    {
        memset(dir, 0, sizeof(*dir));
    #ifdef _WIN32
        size_t temp_mark = JSL_BUILDER_temp_save();
        char *buffer = JSL_BUILDER_temp_sprintf("%s\\*", dir_path);
        dir->JSL_BUILDER__private.win32_hFind = FindFirstFile(buffer, &dir->JSL_BUILDER__private.win32_data);
        JSL_BUILDER_temp_rewind(temp_mark);

        if (dir->JSL_BUILDER__private.win32_hFind == INVALID_HANDLE_VALUE)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open directory %s: %s", dir_path, JSL_BUILDER_win32_error_message(GetLastError()));
            dir->error = true;
            return false;
        }
    #else
        dir->JSL_BUILDER__private.posix_dir = opendir(dir_path);
        if (dir->JSL_BUILDER__private.posix_dir == NULL)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open directory %s: %s", dir_path, strerror(errno));
            dir->error = true;
            return false;
        }
    #endif // _WIN32
        return true;
    }

    bool JSL_BUILDER_dir_entry_next(JSL_BUILDER_Dir_Entry *dir)
    {
    #ifdef _WIN32
        if (!dir->JSL_BUILDER__private.win32_init)
        {
            dir->JSL_BUILDER__private.win32_init = true;
            dir->name = dir->JSL_BUILDER__private.win32_data.cFileName;
            return true;
        }

        if (!FindNextFile(dir->JSL_BUILDER__private.win32_hFind, &dir->JSL_BUILDER__private.win32_data))
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
                return false;
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not read next directory entry: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            dir->error = true;
            return false;
        }
        dir->name = dir->JSL_BUILDER__private.win32_data.cFileName;
    #else
        errno = 0;
        dir->JSL_BUILDER__private.posix_ent = readdir(dir->JSL_BUILDER__private.posix_dir);
        if (dir->JSL_BUILDER__private.posix_ent == NULL)
        {
            if (errno == 0)
                return false;
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not read next directory entry: %s", strerror(errno));
            dir->error = true;
            return false;
        }
        dir->name = dir->JSL_BUILDER__private.posix_ent->d_name;
    #endif // _WIN32
        return true;
    }

    void JSL_BUILDER_dir_entry_close(JSL_BUILDER_Dir_Entry dir)
    {
    #ifdef _WIN32
        FindClose(dir.JSL_BUILDER__private.win32_hFind);
    #else
        if (dir.JSL_BUILDER__private.posix_dir)
            closedir(dir.JSL_BUILDER__private.posix_dir);
    #endif
    }

    // On the moment of entering `JSL_BUILDER__walk_dir_opt_impl()`, the `file_path` JSL_BUILDER_String_Builder is expected to be NULL-terminated.
    // So you can freely pass `file_path->items` to functions that expect NULL-terminated file path.
    // On existing `JSL_BUILDER__walk_dir_opt_impl()` is expected to restore the original content of `file_path`
    bool JSL_BUILDER__walk_dir_opt_impl(JSL_BUILDER_String_Builder *file_path, JSL_BUILDER_Walk_Func func, size_t level, bool *stop, JSL_BUILDER_Walk_Dir_Opt opt)
    {
        JSL_ASSERT(file_path->count > 0 && "file_path was probably not properly NULL-terminated");
        bool result = true;

        JSL_BUILDER_Dir_Entry dir = {0};
        size_t saved_file_path_count = file_path->count;
        JSL_BUILDER_Walk_Action action = JSL_BUILDER_WALK_CONT;

        JSL_BUILDER_File_Type file_type = JSL_BUILDER_get_file_type(file_path->items);
        if (file_type < 0)
            JSL_BUILDER_return_defer(false);

        // Pre-order walking
        if (!opt.post_order)
        {
            if (!func(JSL_BUILDER_CLIT(JSL_BUILDER_Walk_Entry){
                    .path = file_path->items,
                    .type = file_type,
                    .level = level,
                    .data = opt.data,
                    .action = &action,
                }))
                JSL_BUILDER_return_defer(false);
            switch (action)
            {
            case JSL_BUILDER_WALK_CONT:
                break;
            case JSL_BUILDER_WALK_STOP:
                *stop = true; // fallthrough
            case JSL_BUILDER_WALK_SKIP:
                JSL_BUILDER_return_defer(true);
            default:
                JSL_BUILDER_UNREACHABLE("JSL_BUILDER_Walk_Action");
            }
        }

        if (file_type == JSL_BUILDER_FILE_DIRECTORY)
        {
            if (!JSL_BUILDER_dir_entry_open(file_path->items, &dir))
                JSL_BUILDER_return_defer(false);
            for (;;)
            {
                // Next entry
                if (!JSL_BUILDER_dir_entry_next(&dir))
                {
                    if (!dir.error)
                        break;
                    JSL_BUILDER_return_defer(false);
                }

                // Ignore . and ..
                if (strcmp(dir.name, ".") == 0)
                    continue;
                if (strcmp(dir.name, "..") == 0)
                    continue;

                // Prepare the new file_path
                file_path->count = saved_file_path_count - 1;
    #ifdef _WIN32
                JSL_BUILDER_sb_appendf(file_path, "\\%s", dir.name);
    #else
                JSL_BUILDER_sb_appendf(file_path, "/%s", dir.name);
    #endif // _WIN32
                JSL_BUILDER_sb_append_null(file_path);

                // Recurse
                if (!JSL_BUILDER__walk_dir_opt_impl(file_path, func, level + 1, stop, opt))
                    JSL_BUILDER_return_defer(false);
                if (*stop)
                    JSL_BUILDER_return_defer(true);
            }
            file_path->count = saved_file_path_count;
            JSL_BUILDER_da_last(file_path) = '\0';
        }

        // Post-order walking
        if (opt.post_order)
        {
            if (!func(JSL_BUILDER_CLIT(JSL_BUILDER_Walk_Entry){
                    .path = file_path->items,
                    .type = file_type,
                    .level = level,
                    .data = opt.data,
                    .action = &action,
                }))
                JSL_BUILDER_return_defer(false);
            switch (action)
            {
            case JSL_BUILDER_WALK_CONT:
                break;
            case JSL_BUILDER_WALK_STOP:
                *stop = true; // fallthrough
            case JSL_BUILDER_WALK_SKIP:
                JSL_BUILDER_return_defer(true);
            default:
                JSL_BUILDER_UNREACHABLE("JSL_BUILDER_Walk_Action");
            }
        }

    defer:
        // Always reset the file_path back to what it was
        file_path->count = saved_file_path_count;
        JSL_BUILDER_da_last(file_path) = '\0';

        JSL_BUILDER_dir_entry_close(dir);
        return result;
    }

    bool JSL_BUILDER_walk_dir_opt(const char *root, JSL_BUILDER_Walk_Func func, JSL_BUILDER_Walk_Dir_Opt opt)
    {
        JSL_BUILDER_String_Builder file_path = {0};

        JSL_BUILDER_sb_appendf(&file_path, "%s", root);
        JSL_BUILDER_sb_append_null(&file_path);

        bool stop = false;
        bool ok = JSL_BUILDER__walk_dir_opt_impl(&file_path, func, 0, &stop, opt);
        free(file_path.items);
        return ok;
    }

    bool JSL_BUILDER_read_entire_dir(const char *parent, JSL_BUILDER_File_Paths *children)
    {
        if (strlen(parent) == 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Cannot read empty path");
            return false;
        }
        bool result = true;
        JSL_BUILDER_Dir_Entry dir = {0};
        if (!JSL_BUILDER_dir_entry_open(parent, &dir))
            JSL_BUILDER_return_defer(false);
        while (JSL_BUILDER_dir_entry_next(&dir))
            JSL_BUILDER_da_append(children, JSL_BUILDER_temp_strdup(dir.name));
        if (dir.error)
            JSL_BUILDER_return_defer(false);
    defer:
        JSL_BUILDER_dir_entry_close(dir);
        return result;
    }

    bool JSL_BUILDER_write_entire_file(const char *path, const void *data, size_t size)
    {
        bool result = true;

        const char *buf = NULL;
        FILE *f = fopen(path, "wb");
        if (f == NULL)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s for writing: %s\n", path, strerror(errno));
            JSL_BUILDER_return_defer(false);
        }

        //           len
        //           v
        // aaaaaaaaaa
        //     ^
        //     data

        buf = (const char *)data;
        while (size > 0)
        {
            size_t n = fwrite(buf, 1, size, f);
            if (ferror(f))
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not write into file %s: %s\n", path, strerror(errno));
                JSL_BUILDER_return_defer(false);
            }
            size -= n;
            buf += n;
        }

    defer:
        if (f)
            fclose(f);
        return result;
    }

    JSL_BUILDER_File_Type JSL_BUILDER_get_file_type(const char *path)
    {
    #ifdef _WIN32
        DWORD attr = GetFileAttributesA(path);
        if (attr == INVALID_FILE_ATTRIBUTES)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not get file attributes of %s: %s", path, JSL_BUILDER_win32_error_message(GetLastError()));
            return (JSL_BUILDER_File_Type)-1;
        }

        if (attr & FILE_ATTRIBUTE_DIRECTORY)
            return JSL_BUILDER_FILE_DIRECTORY;
        // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
        return JSL_BUILDER_FILE_REGULAR;
    #else  // _WIN32
        struct stat statbuf;
        if (lstat(path, &statbuf) < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not get stat of %s: %s", path, strerror(errno));
            return (JSL_BUILDER_File_Type)(-1);
        }

        if (S_ISREG(statbuf.st_mode))
            return JSL_BUILDER_FILE_REGULAR;
        if (S_ISDIR(statbuf.st_mode))
            return JSL_BUILDER_FILE_DIRECTORY;
        if (S_ISLNK(statbuf.st_mode))
            return JSL_BUILDER_FILE_SYMLINK;
        return JSL_BUILDER_FILE_OTHER;
    #endif // _WIN32
    }

    bool JSL_BUILDER_delete_file(const char *path)
    {
    #ifndef JSL_BUILDER_NO_ECHO
        JSL_BUILDER_log(JSL_BUILDER_INFO, "deleting %s", path);
    #endif // JSL_BUILDER_NO_ECHO
    #ifdef _WIN32
        JSL_BUILDER_File_Type type = JSL_BUILDER_get_file_type(path);
        switch (type)
        {
        case JSL_BUILDER_FILE_DIRECTORY:
            if (!RemoveDirectoryA(path))
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not delete directory %s: %s", path, JSL_BUILDER_win32_error_message(GetLastError()));
                return false;
            }
            break;
        case JSL_BUILDER_FILE_REGULAR:
        case JSL_BUILDER_FILE_SYMLINK:
        case JSL_BUILDER_FILE_OTHER:
            if (!DeleteFileA(path))
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not delete file %s: %s", path, JSL_BUILDER_win32_error_message(GetLastError()));
                return false;
            }
            break;
        default:
            JSL_BUILDER_UNREACHABLE("JSL_BUILDER_File_Type");
        }
        return true;
    #else
        if (remove(path) < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not delete file %s: %s", path, strerror(errno));
            return false;
        }
        return true;
    #endif // _WIN32
    }

    bool JSL_BUILDER_copy_directory_recursively(const char *src_path, const char *dst_path)
    {
        bool result = true;
        JSL_BUILDER_File_Paths children = {0};
        JSL_BUILDER_String_Builder src_sb = {0};
        JSL_BUILDER_String_Builder dst_sb = {0};
        size_t temp_checkpoint = JSL_BUILDER_temp_save();

        JSL_BUILDER_File_Type type = JSL_BUILDER_get_file_type(src_path);
        if (type < 0)
            return false;

        switch (type)
        {
        case JSL_BUILDER_FILE_DIRECTORY:
        {
            if (!JSL_BUILDER_mkdir_if_not_exists(dst_path))
                JSL_BUILDER_return_defer(false);
            if (!JSL_BUILDER_read_entire_dir(src_path, &children))
                JSL_BUILDER_return_defer(false);

            for (size_t i = 0; i < children.count; ++i)
            {
                if (strcmp(children.items[i], ".") == 0)
                    continue;
                if (strcmp(children.items[i], "..") == 0)
                    continue;

                src_sb.count = 0;
                JSL_BUILDER_sb_append_cstr(&src_sb, src_path);
                JSL_BUILDER_sb_append_cstr(&src_sb, "/");
                JSL_BUILDER_sb_append_cstr(&src_sb, children.items[i]);
                JSL_BUILDER_sb_append_null(&src_sb);

                dst_sb.count = 0;
                JSL_BUILDER_sb_append_cstr(&dst_sb, dst_path);
                JSL_BUILDER_sb_append_cstr(&dst_sb, "/");
                JSL_BUILDER_sb_append_cstr(&dst_sb, children.items[i]);
                JSL_BUILDER_sb_append_null(&dst_sb);

                if (!JSL_BUILDER_copy_directory_recursively(src_sb.items, dst_sb.items))
                {
                    JSL_BUILDER_return_defer(false);
                }
            }
        }
        break;

        case JSL_BUILDER_FILE_REGULAR:
        {
            if (!JSL_BUILDER_copy_file(src_path, dst_path))
            {
                JSL_BUILDER_return_defer(false);
            }
        }
        break;

        case JSL_BUILDER_FILE_SYMLINK:
        {
            JSL_BUILDER_log(JSL_BUILDER_WARNING, "TODO: Copying symlinks is not supported yet");
        }
        break;

        case JSL_BUILDER_FILE_OTHER:
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Unsupported type of file %s", src_path);
            JSL_BUILDER_return_defer(false);
        }
        break;

        default:
            JSL_BUILDER_UNREACHABLE("JSL_BUILDER_copy_directory_recursively");
        }

    defer:
        JSL_BUILDER_temp_rewind(temp_checkpoint);
        JSL_BUILDER_da_free(src_sb);
        JSL_BUILDER_da_free(dst_sb);
        JSL_BUILDER_da_free(children);
        return result;
    }

    char *JSL_BUILDER_temp_strdup(const char *cstr)
    {
        size_t n = strlen(cstr);
        char *result = (char *)JSL_BUILDER_temp_alloc(n + 1);
        JSL_ASSERT(result != NULL && "Increase JSL_BUILDER_TEMP_CAPACITY");
        memcpy(result, cstr, n);
        result[n] = '\0';
        return result;
    }

    char *JSL_BUILDER_temp_strndup(const char *s, size_t n)
    {
        char *r = (char *)JSL_BUILDER_temp_alloc(n + 1);
        JSL_ASSERT(r != NULL && "Extend the size of the temporary allocator");
        memcpy(r, s, n);
        r[n] = '\0';
        return r;
    }

    void *JSL_BUILDER_temp_alloc(size_t requested_size)
    {
        size_t word_size = sizeof(uintptr_t);
        size_t size = (requested_size + word_size - 1) / word_size * word_size;
        if (JSL_BUILDER_temp_size + size > JSL_BUILDER_TEMP_CAPACITY)
            return NULL;
        void *result = &JSL_BUILDER_temp[JSL_BUILDER_temp_size];
        JSL_BUILDER_temp_size += size;
        return result;
    }

    char *JSL_BUILDER_temp_vsprintf(const char *format, va_list ap)
    {
        va_list args;
        va_copy(args, ap);
        int n = vsnprintf(NULL, 0, format, args);
        va_end(args);

        JSL_ASSERT(n >= 0);
        char *result = (char *)JSL_BUILDER_temp_alloc(n + 1);
        JSL_ASSERT(result != NULL && "Extend the size of the temporary allocator");
        // TODO: use proper arenas for the temporary allocator;
        va_copy(args, ap);
        vsnprintf(result, n + 1, format, args);
        va_end(args);

        return result;
    }

    char *JSL_BUILDER_temp_sprintf(const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        char *result = JSL_BUILDER_temp_vsprintf(format, args);
        va_end(args);
        return result;
    }

    void JSL_BUILDER_temp_reset(void)
    {
        JSL_BUILDER_temp_size = 0;
    }

    size_t JSL_BUILDER_temp_save(void)
    {
        return JSL_BUILDER_temp_size;
    }

    void JSL_BUILDER_temp_rewind(size_t checkpoint)
    {
        JSL_BUILDER_temp_size = checkpoint;
    }

    const char *JSL_BUILDER_temp_sv_to_cstr(JSL_BUILDER_String_View sv)
    {
        return JSL_BUILDER_temp_strndup(sv.data, sv.count);
    }

    int JSL_BUILDER_needs_rebuild(const char *output_path, const char **input_paths, size_t input_paths_count)
    {
    #ifdef _WIN32
        BOOL bSuccess;

        HANDLE output_path_fd = CreateFile(output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (output_path_fd == INVALID_HANDLE_VALUE)
        {
            // NOTE: if output does not exist it 100% must be rebuilt
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                return 1;
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s: %s", output_path, JSL_BUILDER_win32_error_message(GetLastError()));
            return -1;
        }
        FILETIME output_path_time;
        bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
        CloseHandle(output_path_fd);
        if (!bSuccess)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not get time of %s: %s", output_path, JSL_BUILDER_win32_error_message(GetLastError()));
            return -1;
        }

        for (size_t i = 0; i < input_paths_count; ++i)
        {
            const char *input_path = input_paths[i];
            HANDLE input_path_fd = CreateFile(input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
            if (input_path_fd == INVALID_HANDLE_VALUE)
            {
                // NOTE: non-existing input is an error cause it is needed for building in the first place
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not open file %s: %s", input_path, JSL_BUILDER_win32_error_message(GetLastError()));
                return -1;
            }
            FILETIME input_path_time;
            bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
            CloseHandle(input_path_fd);
            if (!bSuccess)
            {
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not get time of %s: %s", input_path, JSL_BUILDER_win32_error_message(GetLastError()));
                return -1;
            }

            // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
            if (CompareFileTime(&input_path_time, &output_path_time) == 1)
                return 1;
        }

        return 0;
    #else
        struct stat statbuf = {0};

        if (stat(output_path, &statbuf) < 0)
        {
            // NOTE: if output does not exist it 100% must be rebuilt
            if (errno == ENOENT)
                return 1;
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not stat %s: %s", output_path, strerror(errno));
            return -1;
        }
        time_t output_path_time = statbuf.st_mtime;

        for (size_t i = 0; i < input_paths_count; ++i)
        {
            const char *input_path = input_paths[i];
            if (stat(input_path, &statbuf) < 0)
            {
                // NOTE: non-existing input is an error cause it is needed for building in the first place
                JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not stat %s: %s", input_path, strerror(errno));
                return -1;
            }
            time_t input_path_time = statbuf.st_mtime;
            // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
            if (input_path_time > output_path_time)
                return 1;
        }

        return 0;
    #endif
    }

    int JSL_BUILDER_needs_rebuild1(const char *output_path, const char *input_path)
    {
        return JSL_BUILDER_needs_rebuild(output_path, &input_path, 1);
    }

    const char *JSL_BUILDER_path_name(const char *path)
    {
    #ifdef _WIN32
        const char *p1 = strrchr(path, '/');
        const char *p2 = strrchr(path, '\\');
        const char *p = (p1 > p2) ? p1 : p2; // NULL is ignored if the other search is successful
        return p ? p + 1 : path;
    #else
        const char *p = strrchr(path, '/');
        return p ? p + 1 : path;
    #endif // _WIN32
    }

    bool JSL_BUILDER_rename(const char *old_path, const char *new_path)
    {
    #ifndef JSL_BUILDER_NO_ECHO
        JSL_BUILDER_log(JSL_BUILDER_INFO, "renaming %s -> %s", old_path, new_path);
    #endif // JSL_BUILDER_NO_ECHO
    #ifdef _WIN32
        if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING))
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not rename %s to %s: %s", old_path, new_path, JSL_BUILDER_win32_error_message(GetLastError()));
            return false;
        }
    #else
        if (rename(old_path, new_path) < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not rename %s to %s: %s", old_path, new_path, strerror(errno));
            return false;
        }
    #endif // _WIN32
        return true;
    }

    bool JSL_BUILDER_read_entire_file(const char *path, JSL_BUILDER_String_Builder *sb)
    {
        bool result = true;

        FILE *f = fopen(path, "rb");
        size_t new_count = 0;
        long long m = 0;
        if (f == NULL)
            JSL_BUILDER_return_defer(false);
        if (fseek(f, 0, SEEK_END) < 0)
            JSL_BUILDER_return_defer(false);
    #ifndef _WIN32
        m = ftell(f);
    #else
        m = _telli64(_fileno(f));
    #endif
        if (m < 0)
            JSL_BUILDER_return_defer(false);
        if (fseek(f, 0, SEEK_SET) < 0)
            JSL_BUILDER_return_defer(false);

        new_count = sb->count + m;
        if (new_count > sb->capacity)
        {
            sb->items = JSL_BUILDER_DECLTYPE_CAST(sb->items) JSL_BUILDER_REALLOC(sb->items, new_count);
            JSL_ASSERT(sb->items != NULL && "Buy more RAM lool!!");
            sb->capacity = new_count;
        }

        fread(sb->items + sb->count, m, 1, f);
        if (ferror(f))
        {
            // TODO: Afaik, ferror does not set errno. So the error reporting in defer is not correct in this case.
            JSL_BUILDER_return_defer(false);
        }
        sb->count = new_count;

    defer:
        if (!result)
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "Could not read file %s: %s", path, strerror(errno));
        if (f)
            fclose(f);
        return result;
    }

    int JSL_BUILDER_sb_appendf(JSL_BUILDER_String_Builder *sb, const char *fmt, ...)
    {
        va_list args;

        va_start(args, fmt);
        int n = vsnprintf(NULL, 0, fmt, args);
        va_end(args);

        // NOTE: the new_capacity needs to be +1 because of the null terminator.
        // However, further below we increase sb->count by n, not n + 1.
        // This is because we don't want the sb to include the null terminator. The user can always sb_append_null() if they want it
        JSL_BUILDER_da_reserve(sb, sb->count + n + 1);
        char *dest = sb->items + sb->count;
        va_start(args, fmt);
        vsnprintf(dest, n + 1, fmt, args);
        va_end(args);

        sb->count += n;

        return n;
    }

    void JSL_BUILDER_sb_pad_align(JSL_BUILDER_String_Builder *sb, size_t size)
    {
        size_t rem = sb->count % size;
        if (rem == 0)
            return;
        for (size_t i = 0; i < size - rem; ++i)
        {
            JSL_BUILDER_da_append(sb, 0);
        }
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_chop_while(JSL_BUILDER_String_View *sv, int (*p)(int x))
    {
        size_t i = 0;
        while (i < sv->count && p(sv->data[i]))
        {
            i += 1;
        }

        JSL_BUILDER_String_View result = JSL_BUILDER_sv_from_parts(sv->data, i);
        sv->count -= i;
        sv->data += i;

        return result;
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_chop_by_delim(JSL_BUILDER_String_View *sv, char delim)
    {
        size_t i = 0;
        while (i < sv->count && sv->data[i] != delim)
        {
            i += 1;
        }

        JSL_BUILDER_String_View result = JSL_BUILDER_sv_from_parts(sv->data, i);

        if (i < sv->count)
        {
            sv->count -= i + 1;
            sv->data += i + 1;
        }
        else
        {
            sv->count -= i;
            sv->data += i;
        }

        return result;
    }

    bool JSL_BUILDER_sv_chop_prefix(JSL_BUILDER_String_View *sv, JSL_BUILDER_String_View prefix)
    {
        if (JSL_BUILDER_sv_starts_with(*sv, prefix))
        {
            JSL_BUILDER_sv_chop_left(sv, prefix.count);
            return true;
        }
        return false;
    }

    bool JSL_BUILDER_sv_chop_suffix(JSL_BUILDER_String_View *sv, JSL_BUILDER_String_View suffix)
    {
        if (JSL_BUILDER_sv_ends_with(*sv, suffix))
        {
            JSL_BUILDER_sv_chop_right(sv, suffix.count);
            return true;
        }
        return false;
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_chop_left(JSL_BUILDER_String_View *sv, size_t n)
    {
        if (n > sv->count)
        {
            n = sv->count;
        }

        JSL_BUILDER_String_View result = JSL_BUILDER_sv_from_parts(sv->data, n);

        sv->data += n;
        sv->count -= n;

        return result;
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_chop_right(JSL_BUILDER_String_View *sv, size_t n)
    {
        if (n > sv->count)
        {
            n = sv->count;
        }

        JSL_BUILDER_String_View result = JSL_BUILDER_sv_from_parts(sv->data + sv->count - n, n);

        sv->count -= n;

        return result;
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_from_parts(const char *data, size_t count)
    {
        JSL_BUILDER_String_View sv;
        sv.count = count;
        sv.data = data;
        return sv;
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_trim_left(JSL_BUILDER_String_View sv)
    {
        size_t i = 0;
        while (i < sv.count && isspace(sv.data[i]))
        {
            i += 1;
        }

        return JSL_BUILDER_sv_from_parts(sv.data + i, sv.count - i);
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_trim_right(JSL_BUILDER_String_View sv)
    {
        size_t i = 0;
        while (i < sv.count && isspace(sv.data[sv.count - 1 - i]))
        {
            i += 1;
        }

        return JSL_BUILDER_sv_from_parts(sv.data, sv.count - i);
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_trim(JSL_BUILDER_String_View sv)
    {
        return JSL_BUILDER_sv_trim_right(JSL_BUILDER_sv_trim_left(sv));
    }

    JSL_BUILDER_String_View JSL_BUILDER_sv_from_cstr(const char *cstr)
    {
        return JSL_BUILDER_sv_from_parts(cstr, strlen(cstr));
    }

    bool JSL_BUILDER_sv_eq(JSL_BUILDER_String_View a, JSL_BUILDER_String_View b)
    {
        if (a.count != b.count)
        {
            return false;
        }
        else
        {
            return memcmp(a.data, b.data, a.count) == 0;
        }
    }

    bool JSL_BUILDER_sv_end_with(JSL_BUILDER_String_View sv, const char *cstr)
    {
        return JSL_BUILDER_sv_ends_with_cstr(sv, cstr);
    }

    bool JSL_BUILDER_sv_ends_with_cstr(JSL_BUILDER_String_View sv, const char *cstr)
    {
        return JSL_BUILDER_sv_ends_with(sv, JSL_BUILDER_sv_from_cstr(cstr));
    }

    bool JSL_BUILDER_sv_ends_with(JSL_BUILDER_String_View sv, JSL_BUILDER_String_View suffix)
    {
        if (sv.count >= suffix.count)
        {
            JSL_BUILDER_String_View sv_tail = {
                .count = suffix.count,
                .data = sv.data + sv.count - suffix.count,
            };
            return JSL_BUILDER_sv_eq(sv_tail, suffix);
        }
        return false;
    }

    bool JSL_BUILDER_sv_starts_with(JSL_BUILDER_String_View sv, JSL_BUILDER_String_View expected_prefix)
    {
        if (expected_prefix.count <= sv.count)
        {
            JSL_BUILDER_String_View actual_prefix = JSL_BUILDER_sv_from_parts(sv.data, expected_prefix.count);
            return JSL_BUILDER_sv_eq(expected_prefix, actual_prefix);
        }

        return false;
    }

    // RETURNS:
    //  0 - file does not exists
    //  1 - file exists
    int JSL_BUILDER_file_exists(const char *file_path)
    {
    #if _WIN32
        return GetFileAttributesA(file_path) != INVALID_FILE_ATTRIBUTES;
    #else
        return access(file_path, F_OK) == 0;
    #endif
    }

    const char *JSL_BUILDER_get_current_dir_temp(void)
    {
    #ifdef _WIN32
        DWORD nBufferLength = GetCurrentDirectory(0, NULL);
        if (nBufferLength == 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not get current directory: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            return NULL;
        }

        char *buffer = (char *)JSL_BUILDER_temp_alloc(nBufferLength);
        if (GetCurrentDirectory(nBufferLength, buffer) == 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not get current directory: %s", JSL_BUILDER_win32_error_message(GetLastError()));
            return NULL;
        }

        return buffer;
    #else
        char *buffer = (char *)JSL_BUILDER_temp_alloc(PATH_MAX);
        if (getcwd(buffer, PATH_MAX) == NULL)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not get current directory: %s", strerror(errno));
            return NULL;
        }

        return buffer;
    #endif // _WIN32
    }

    bool JSL_BUILDER_set_current_dir(const char *path)
    {
    #ifdef _WIN32
        if (!SetCurrentDirectory(path))
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not set current directory to %s: %s", path, JSL_BUILDER_win32_error_message(GetLastError()));
            return false;
        }
        return true;
    #else
        if (chdir(path) < 0)
        {
            JSL_BUILDER_log(JSL_BUILDER_ERROR, "could not set current directory to %s: %s", path, strerror(errno));
            return false;
        }
        return true;
    #endif // _WIN32
    }

    char *JSL_BUILDER_temp_dir_name(const char *path)
    {
    #ifndef _WIN32
        // Stolen from the musl's implementation of dirname.
        // We are implementing our own one because libc vendors cannot agree on whether dirname(3)
        // modifies the path or not.
        if (!path || !*path)
            return JSL_BUILDER_temp_strdup(".");
        size_t i = strlen(path) - 1;
        for (; path[i] == '/'; i--)
            if (!i)
                return JSL_BUILDER_temp_strdup("/");
        for (; path[i] != '/'; i--)
            if (!i)
                return JSL_BUILDER_temp_strdup(".");
        for (; path[i] == '/'; i--)
            if (!i)
                return JSL_BUILDER_temp_strdup("/");
        return JSL_BUILDER_temp_strndup(path, i + 1);
    #else
        if (!path)
            path = ""; // Treating NULL as empty.
        char *drive = (char *)JSL_BUILDER_temp_alloc(_MAX_DRIVE);
        char *dir = (char *)JSL_BUILDER_temp_alloc(_MAX_DIR);
        // https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/8e46eyt7(v=vs.100)
        errno_t ret = _splitpath_s(path, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
        JSL_ASSERT(ret == 0);
        return JSL_BUILDER_temp_sprintf("%s%s", drive, dir);
    #endif // _WIN32
    }

    char *JSL_BUILDER_temp_file_name(const char *path)
    {
    #ifndef _WIN32
        // Stolen from the musl's implementation of dirname.
        // We are implementing our own one because libc vendors cannot agree on whether basename(3)
        // modifies the path or not.
        if (!path || !*path)
            return JSL_BUILDER_temp_strdup(".");
        char *s = JSL_BUILDER_temp_strdup(path);
        size_t i = strlen(s) - 1;
        for (; i && s[i] == '/'; i--)
            s[i] = 0;
        for (; i && s[i - 1] != '/'; i--)
            ;
        return s + i;
    #else
        if (!path)
            path = ""; // Treating NULL as empty.
        char *fname = (char *)JSL_BUILDER_temp_alloc(_MAX_FNAME);
        char *ext = (char *)JSL_BUILDER_temp_alloc(_MAX_EXT);
        // https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/8e46eyt7(v=vs.100)
        errno_t ret = _splitpath_s(path, NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
        JSL_ASSERT(ret == 0);
        return JSL_BUILDER_temp_sprintf("%s%s", fname, ext);
    #endif // _WIN32
    }

    char *JSL_BUILDER_temp_file_ext(const char *path)
    {
    #ifndef _WIN32
        return strrchr(JSL_BUILDER_temp_file_name(path), '.');
    #else
        if (!path)
            path = ""; // Treating NULL as empty.
        char *ext = (char *)JSL_BUILDER_temp_alloc(_MAX_EXT);
        // https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2010/8e46eyt7(v=vs.100)
        errno_t ret = _splitpath_s(path, NULL, 0, NULL, 0, NULL, 0, ext, _MAX_EXT);
        JSL_ASSERT(ret == 0);
        return ext;
    #endif // _WIN32
    }

    char *JSL_BUILDER_temp_running_executable_path(void)
    {
    #if defined(__linux__)
        char buf[4096];
        int length = readlink("/proc/self/exe", buf, JSL_BUILDER_ARRAY_LEN(buf));
        if (length < 0)
            return JSL_BUILDER_temp_strdup("");
        return JSL_BUILDER_temp_strndup(buf, length);
    #elif defined(_WIN32)
        char buf[MAX_PATH];
        int length = GetModuleFileNameA(NULL, buf, MAX_PATH);
        return JSL_BUILDER_temp_strndup(buf, length);
    #elif defined(__APPLE__)
        char buf[4096];
        uint32_t size = JSL_BUILDER_ARRAY_LEN(buf);
        if (_NSGetExecutablePath(buf, &size) != 0)
            return JSL_BUILDER_temp_strdup("");
        int length = strlen(buf);
        return JSL_BUILDER_temp_strndup(buf, length);
    #elif defined(__FreeBSD__)
        char buf[4096];
        int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
        size_t length = sizeof(buf);
        if (sysctl(mib, 4, buf, &length, NULL, 0) < 0)
            return JSL_BUILDER_temp_strdup("");
        return JSL_BUILDER_temp_strndup(buf, length);
    #elif defined(__HAIKU__)
        int cookie = 0;
        image_info info;
        while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK)
            if (info.type == B_APP_IMAGE)
                break;
        return JSL_BUILDER_temp_strndup(info.name, strlen(info.name));
    #else
        fprintf(stderr, "%s:%d: TODO: JSL_BUILDER_temp_running_executable_path is not implemented for this platform\n", __FILE__, __LINE__);
        return JSL_BUILDER_temp_strdup("");
    #endif
    }

#endif // JSL_BUILDER_IMPLEMENTATION

#ifndef JSL_BUILDER_STRIP_PREFIX_GUARD_
#define JSL_BUILDER_STRIP_PREFIX_GUARD_
// NOTE: The name stripping should be part of the header so it's not accidentally included
// several times. At the same time, it should be at the end of the file so to not create any
// potential conflicts in the JSL_BUILDER_IMPLEMENTATION. The header obviously cannot be at the end
// of the file because JSL_BUILDER_IMPLEMENTATION needs the forward declarations from there. So the
// solution is to split the header into two parts where the name stripping part is at the
// end of the file after the JSL_BUILDER_IMPLEMENTATION.
#ifndef JSL_BUILDER_UNSTRIP_PREFIX
#define TODO JSL_BUILDER_TODO
#define UNREACHABLE JSL_BUILDER_UNREACHABLE
#define UNUSED JSL_BUILDER_UNUSED
#define ARRAY_LEN JSL_BUILDER_ARRAY_LEN
#define ARRAY_GET JSL_BUILDER_ARRAY_GET
#define INFO JSL_BUILDER_INFO
#define WARNING JSL_BUILDER_WARNING
#define ERROR JSL_BUILDER_ERROR
#define NO_LOGS JSL_BUILDER_NO_LOGS
#define Log_Level JSL_BUILDER_Log_Level
#define minimal_log_level JSL_BUILDER_minimal_log_level
#define log_handler JSL_BUILDER_log_handler
#define Log_Handler JSL_BUILDER_Log_Handler
#define set_log_handler JSL_BUILDER_set_log_handler
#define get_log_handler JSL_BUILDER_get_log_handler
#define null_log_handler JSL_BUILDER_null_log_handler
#define default_log_handler JSL_BUILDER_default_log_handler
#define cancer_log_handler JSL_BUILDER_cancer_log_handler
// NOTE: Name log is already defined in math.h and historically always was the natural logarithmic function.
// So there should be no reason to strip the `JSL_BUILDER_` prefix in this specific case.
// #define log JSL_BUILDER_log
#define shift JSL_BUILDER_shift
#define shift_args JSL_BUILDER_shift_args
#define GO_REBUILD_URSELF JSL_BUILDER_GO_REBUILD_URSELF
#define GO_REBUILD_URSELF_PLUS JSL_BUILDER_GO_REBUILD_URSELF_PLUS
#define File_Paths JSL_BUILDER_File_Paths
#define FILE_REGULAR JSL_BUILDER_FILE_REGULAR
#define FILE_DIRECTORY JSL_BUILDER_FILE_DIRECTORY
#define FILE_SYMLINK JSL_BUILDER_FILE_SYMLINK
#define FILE_OTHER JSL_BUILDER_FILE_OTHER
#define File_Type JSL_BUILDER_File_Type
#define mkdir_if_not_exists JSL_BUILDER_mkdir_if_not_exists
#define copy_file JSL_BUILDER_copy_file
#define copy_directory_recursively JSL_BUILDER_copy_directory_recursively
#define read_entire_dir JSL_BUILDER_read_entire_dir
#define WALK_CONT JSL_BUILDER_WALK_CONT
#define WALK_SKIP JSL_BUILDER_WALK_SKIP
#define WALK_STOP JSL_BUILDER_WALK_STOP
#define Walk_Action JSL_BUILDER_Walk_Action
#define Walk_Entry JSL_BUILDER_Walk_Entry
#define Walk_Func JSL_BUILDER_Walk_Func
#define Walk_Dir_Opt JSL_BUILDER_Walk_Dir_Opt
#define walk_dir JSL_BUILDER_walk_dir
#define walk_dir_opt JSL_BUILDER_walk_dir_opt
#define write_entire_file JSL_BUILDER_write_entire_file
#define get_file_type JSL_BUILDER_get_file_type
#define delete_file JSL_BUILDER_delete_file
#define Dir_Entry JSL_BUILDER_Dir_Entry
#define dir_entry_open JSL_BUILDER_dir_entry_open
#define dir_entry_next JSL_BUILDER_dir_entry_next
#define dir_entry_close JSL_BUILDER_dir_entry_close
#define return_defer JSL_BUILDER_return_defer
#define da_append JSL_BUILDER_da_append
#define da_free JSL_BUILDER_da_free
#define da_append_many JSL_BUILDER_da_append_many
#define da_resize JSL_BUILDER_da_resize
#define da_reserve JSL_BUILDER_da_reserve
#define da_last JSL_BUILDER_da_last
#define da_first JSL_BUILDER_da_first
#define da_pop JSL_BUILDER_da_pop
#define da_remove_unordered JSL_BUILDER_da_remove_unordered
#define da_foreach JSL_BUILDER_da_foreach
#define fa_append JSL_BUILDER_fa_append
#define swap JSL_BUILDER_swap
#define String_Builder JSL_BUILDER_String_Builder
#define read_entire_file JSL_BUILDER_read_entire_file
#define sb_appendf JSL_BUILDER_sb_appendf
#define sb_append_buf JSL_BUILDER_sb_append_buf
#define sb_append_sv JSL_BUILDER_sb_append_sv
#define sb_append_cstr JSL_BUILDER_sb_append_cstr
#define sb_append_null JSL_BUILDER_sb_append_null
#define sb_append JSL_BUILDER_sb_append
#define sb_pad_align JSL_BUILDER_sb_pad_align
#define sb_free JSL_BUILDER_sb_free
#define Proc JSL_BUILDER_Proc
#define INVALID_PROC JSL_BUILDER_INVALID_PROC
#define Fd JSL_BUILDER_Fd
#define Pipe JSL_BUILDER_Pipe
#define pipe_create JSL_BUILDER_pipe_create
#define Chain JSL_BUILDER_Chain
#define Chain_Begin_Opt JSL_BUILDER_Chain_Begin_Opt
#define chain_begin JSL_BUILDER_chain_begin
#define chain_begin_opt JSL_BUILDER_chain_begin_opt
#define Chain_Cmd_Opt JSL_BUILDER_Chain_Cmd_Opt
#define chain_cmd JSL_BUILDER_chain_cmd
#define chain_cmd_opt JSL_BUILDER_chain_cmd_opt
#define Chain_End_Opt JSL_BUILDER_Chain_End_Opt
#define chain_end JSL_BUILDER_chain_end
#define chain_end_opt JSL_BUILDER_chain_end_opt
#define INVALID_FD JSL_BUILDER_INVALID_FD
#define fd_open_for_read JSL_BUILDER_fd_open_for_read
#define fd_open_for_write JSL_BUILDER_fd_open_for_write
#define fd_close JSL_BUILDER_fd_close
#define Procs JSL_BUILDER_Procs
#define proc_wait JSL_BUILDER_proc_wait
#define procs_wait JSL_BUILDER_procs_wait
#define procs_wait_and_reset JSL_BUILDER_procs_wait_and_reset
#define procs_append_with_flush JSL_BUILDER_procs_append_with_flush
#define procs_flush JSL_BUILDER_procs_flush
#define CLIT JSL_BUILDER_CLIT
#define Cmd JSL_BUILDER_Cmd
#define Cmd_Redirect JSL_BUILDER_Cmd_Redirect
#define Cmd_Opt JSL_BUILDER_Cmd_Opt
#define cmd_run_opt JSL_BUILDER_cmd_run_opt
#define cmd_run JSL_BUILDER_cmd_run
#define cmd_render JSL_BUILDER_cmd_render
#define cmd_append JSL_BUILDER_cmd_append
#define cmd_extend JSL_BUILDER_cmd_extend
#define cmd_free JSL_BUILDER_cmd_free
#define cmd_run_async JSL_BUILDER_cmd_run_async
#define cmd_run_async_and_reset JSL_BUILDER_cmd_run_async_and_reset
#define cmd_run_async_redirect JSL_BUILDER_cmd_run_async_redirect
#define cmd_run_async_redirect_and_reset JSL_BUILDER_cmd_run_async_redirect_and_reset
#define cmd_run_sync JSL_BUILDER_cmd_run_sync
#define cmd_run_sync_and_reset JSL_BUILDER_cmd_run_sync_and_reset
#define cmd_run_sync_redirect JSL_BUILDER_cmd_run_sync_redirect
#define cmd_run_sync_redirect_and_reset JSL_BUILDER_cmd_run_sync_redirect_and_reset
#define temp_strdup JSL_BUILDER_temp_strdup
#define temp_strndup JSL_BUILDER_temp_strndup
#define temp_alloc JSL_BUILDER_temp_alloc
#define temp_sprintf JSL_BUILDER_temp_sprintf
#define temp_vsprintf JSL_BUILDER_temp_vsprintf
#define temp_reset JSL_BUILDER_temp_reset
#define temp_save JSL_BUILDER_temp_save
#define temp_rewind JSL_BUILDER_temp_rewind
#define path_name JSL_BUILDER_path_name
// NOTE: rename(2) is widely known POSIX function. We never wanna collide with it.
// #define rename JSL_BUILDER_rename
#define needs_rebuild JSL_BUILDER_needs_rebuild
#define needs_rebuild1 JSL_BUILDER_needs_rebuild1
#define file_exists JSL_BUILDER_file_exists
#define get_current_dir_temp JSL_BUILDER_get_current_dir_temp
#define set_current_dir JSL_BUILDER_set_current_dir
#define temp_dir_name JSL_BUILDER_temp_dir_name
#define temp_file_name JSL_BUILDER_temp_file_name
#define temp_file_ext JSL_BUILDER_temp_file_ext
#define temp_running_executable_path JSL_BUILDER_temp_running_executable_path
#define String_View JSL_BUILDER_String_View
#define temp_sv_to_cstr JSL_BUILDER_temp_sv_to_cstr
#define sv_chop_by_delim JSL_BUILDER_sv_chop_by_delim
#define sv_chop_while JSL_BUILDER_sv_chop_while
#define sv_chop_prefix JSL_BUILDER_sv_chop_prefix
#define sv_chop_suffix JSL_BUILDER_sv_chop_suffix
#define sv_chop_left JSL_BUILDER_sv_chop_left
#define sv_chop_right JSL_BUILDER_sv_chop_right
#define sv_trim JSL_BUILDER_sv_trim
#define sv_trim_left JSL_BUILDER_sv_trim_left
#define sv_trim_right JSL_BUILDER_sv_trim_right
#define sv_eq JSL_BUILDER_sv_eq
#define sv_starts_with JSL_BUILDER_sv_starts_with
#define sv_end_with JSL_BUILDER_sv_end_with
#define sv_ends_with JSL_BUILDER_sv_ends_with
#define sv_ends_with_cstr JSL_BUILDER_sv_ends_with_cstr
#define sv_from_cstr JSL_BUILDER_sv_from_cstr
#define sv_from_parts JSL_BUILDER_sv_from_parts
#define sb_to_sv JSL_BUILDER_sb_to_sv
#define win32_error_message JSL_BUILDER_win32_error_message
#define nprocs JSL_BUILDER_nprocs
#define nanos_since_unspecified_epoch JSL_BUILDER_nanos_since_unspecified_epoch
#define NANOS_PER_SEC JSL_BUILDER_NANOS_PER_SEC
#endif // JSL_BUILDER_STRIP_PREFIX
#endif // JSL_BUILDER_STRIP_PREFIX_GUARD_

/**
 * Helper program spawned by tests/test_subprocess.c. Implements a small set
 * of sub-commands so the subprocess test suite can exercise exit codes,
 * stdout/stderr handling, stdin piping, environment variables, and working
 * directories without depending on platform-specific utilities like
 * /bin/echo or cmd.exe.
 *
 * Usage:
 *   helper exit N            -> exit with code N
 *   helper echo ARGS...      -> write args space-separated + '\n' to stdout
 *   helper env VAR           -> write getenv(VAR) (or "<null>") to stdout
 *   helper cat               -> copy stdin to stdout until EOF
 *   helper stderr OUT ERR    -> write OUT to stdout, ERR to stderr, exit 0
 *   helper cwd               -> write current working directory to stdout
 *   helper sleep MS          -> sleep MS milliseconds then exit 0
 *   helper spew N            -> write "x" to stdout N times then exit 0
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
    #include <direct.h>
    #include <io.h>
    #include <fcntl.h>
    #include <windows.h>
    #define HELPER_GETCWD _getcwd
    static void helper_sleep_ms(int ms) { Sleep((DWORD) ms); }
#else
    #include <unistd.h>
    #include <time.h>
    #define HELPER_GETCWD getcwd
    static void helper_sleep_ms(int ms)
    {
        struct timespec ts;
        ts.tv_sec  = ms / 1000;
        ts.tv_nsec = ((long) ms % 1000L) * 1000000L;
        nanosleep(&ts, NULL);
    }
#endif

int main(int argc, char** argv)
{
    if (argc < 2)
        return 2;

    #if defined(_WIN32)
        // Use binary mode on Windows so the cat command does not translate
        // \n to \r\n on stdout and does not strip \r on stdin.
        _setmode(_fileno(stdin),  _O_BINARY);
        _setmode(_fileno(stdout), _O_BINARY);
        _setmode(_fileno(stderr), _O_BINARY);
    #endif

    const char* command = argv[1];
    int ret = 0;

    if (strcmp(command, "exit") == 0)
    {
        if (argc >= 3)
            ret = atoi(argv[2]);
        else
            ret = 0;
    }
    else if (strcmp(command, "echo") == 0)
    {
        for (int i = 2; i < argc; ++i)
        {
            if (i > 2)
                fputc(' ', stdout);
            fputs(argv[i], stdout);
        }
        fputc('\n', stdout);
    }
    else if (strcmp(command, "env") == 0)
    {
        if (argc < 3)
            return 2;

        const char* value = getenv(argv[2]);
        if (value == NULL)
            fputs("<null>", stdout);
        else
            fputs(value, stdout);
    }
    else if (strcmp(command, "cat") == 0)
    {
        unsigned char buffer[1024];
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), stdin)) > 0)
        {
            size_t written = fwrite(buffer, 1, n, stdout);
            if (written != n)
                return 3;
        }
    }
    else if (strcmp(command, "stderr") == 0)
    {
        const char* out_msg = (argc >= 3) ? argv[2] : "";
        const char* err_msg = (argc >= 4) ? argv[3] : "";
        fputs(out_msg, stdout);
        fputs(err_msg, stderr);
    }
    else if (strcmp(command, "cwd") == 0)
    {
        char buffer[4096];
        if (HELPER_GETCWD(buffer, sizeof(buffer)) == NULL)
            return 4;
        fputs(buffer, stdout);
    }
    else if (strcmp(command, "sleep") == 0)
    {
        int ms = (argc >= 3) ? atoi(argv[2]) : 0;
        if (ms < 0) ms = 0;
        helper_sleep_ms(ms);
    }
    else if (strcmp(command, "spew") == 0)
    {
        int n = (argc >= 3) ? atoi(argv[2]) : 0;
        if (n < 0) n = 0;
        for (int i = 0; i < n; ++i)
            fputc('x', stdout);
    }
    else
    {
        return 2;
    }

    fflush(stdout);
    fflush(stderr);
    return ret;
}

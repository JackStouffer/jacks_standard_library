#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define JSL_IMPLEMENTATION
#define JSL_INCLUDE_FILE_UTILS
#include "../src/jacks_standard_library.h"

#define NOB_IMPLEMENTATION
#include "nob.h"


int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    #if defined(_WIN32)
        Nob_Cmd cmd = {0};
        nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-o", "main", "main.c");
        if (!nob_cmd_run(&cmd)) return 1;
        return 0;
    #elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    
    #else
        #error "Unrecognized platform. Only windows and POSIX platforms are supported."
    #endif
}

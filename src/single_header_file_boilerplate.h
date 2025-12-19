/**
 * # My Lib
 * 
 * Simple boilerplate for a single header lib that you can copy/paste
 * to get started.
 * 
 * ## License
 *
 * YOUR LICENSE HERE 
 */


#ifndef MY_LIB_H_INCLUDED
    #define MY_LIB_H_INCLUDED

    // type definition or macro only headers 
    #include <stdint.h>
    #include <stddef.h>
    #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #include <stdbool.h>
    #endif

    /* Versioning to catch mismatches across deps */
    #ifndef MY_LIB_VERSION
        #define MY_LIB_VERSION 0x010200  /* 1.2.0 */
    #else
        #if MY_LIB_VERSION != 0x010200
            #error "my_lib.h version mismatch across includes"
        #endif
    #endif

    #ifndef MY_LIB_DEF
        #define MY_LIB_DEF
    #endif

    #ifdef __cplusplus
    extern "C" {
    #endif


    MY_LIB_DEF int32_t my_func(void* b);

    
    #ifdef __cplusplus
    }
    #endif
    
#endif /* MY_LIB_H_INCLUDED */

#ifdef MY_LIB_IMPLEMENTATION

    MY_LIB_DEF int my_func(void* b)
    {
        return 1;
    }

#endif /* MY_LIB_IMPLEMENTATION */

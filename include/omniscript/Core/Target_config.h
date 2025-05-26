#pragma once

#if !defined(TARGET_32BIT) && !defined(TARGET_64BIT)
    #if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
        #define TARGET_64BIT 1
    #elif defined(__i386__) || defined(_M_IX86) || defined(__arm__)
        #define TARGET_32BIT 1
    #else
        #error "Unknown architecture: define TARGET_32BIT or TARGET_64BIT manually"
    #endif
#endif

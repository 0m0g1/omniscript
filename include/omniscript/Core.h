// core.h
#ifndef Omniscript_Core_H
#define Omniscript_Core_H

#include <omniscript/omniscript_pch.h>

#if defined(__AVX512F__)
    #include <immintrin.h>
    #define SIMD_OPTIMIZATION_LEVEL 512
#elif defined(__AVX2__)
    #include <immintrin.h>
    #define SIMD_OPTIMIZATION_LEVEL 256
#elif defined(__AVX__)
    #include <immintrin.h>
    #define SIMD_OPTIMIZATION_LEVEL 128
#else
    #define SIMD_OPTIMIZATION_LEVEL 64
#endif

#ifdef DEBUG
    #define DEBUG_LOG(msg) console.debug(msg)
#else
    #define DEBUG_LOG(msg) // Nothing in release mode
#endif

namespace Omniscript {
    // Global boolean flag to track GLFW initialization state
    extern bool glfwInitialized;

    // Function to check if GLFW is initialized
    bool isGlfwInitialized();

    // Current position
    struct filePosition {
        int col = 0; 
        int line = 0; 
        std::string fileName;
        std::string filePath;
    };

    extern filePosition pos;
    
    inline void setPosition(int line, int column, const std::string& path) {
        pos.line = line;
        pos.col = column;
        pos.filePath = path;
    }

    inline void setPosition(filePosition position) {
        pos.line = position.line;
        pos.col = position.col;
        pos.fileName = position.fileName;
        pos.filePath = position.filePath;
    }
    
    inline filePosition getPosition() {
        return pos;
    }

    extern bool allThreadsDone;
} // namespace Omniscript

#endif // CORE_H

// core.h
#ifndef Omniscript_Core_H
#define Omniscript_Core_H

#include <omniscript/omniscript_pch.h>

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
    };

    extern filePosition pos;
    inline void setPosition(int line, int column, const std::string& file) {
        pos.line = line;
        pos.col = column;
        pos.fileName = file;
    }
    inline void setPosition(filePosition position) {
        pos.line = position.line;
        pos.col = position.col;
        pos.fileName = position.fileName;
    }
    inline filePosition getPosition() {
        return pos;
    }
    extern bool allThreadsDone;
} // namespace Omniscript

#endif // CORE_H

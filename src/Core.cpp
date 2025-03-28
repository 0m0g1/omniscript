// core.cpp
#include <omniscript/Core.h>

namespace Omniscript {

    // Define the global boolean variable
    bool glfwInitialized = false;

    // Getter function for glfwInitialized
    bool isGlfwInitialized() {
        return glfwInitialized;
    }

    filePosition pos;
    bool allThreadsDone = true;
} // namespace Omniscript

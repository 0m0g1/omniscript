#pragma once
#include <string>

namespace Omniscript {

// Cross-platform library paths
struct LibraryPaths {
    std::string cLibrary;
    std::string windowsDynamic;    // .dll
    std::string windowsStatic;     // .lib/.a
    std::string linuxShared;       // .so
    std::string linuxStatic;       // .a
    std::string macosShared;       // .dylib
    std::string macosStatic;       // .a
    std::string genericDynamic;    // fallback dynamic
    std::string genericStatic;     // fallback static
};

}
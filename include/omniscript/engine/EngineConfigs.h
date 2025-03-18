#pragma once
#include <omniscript/omniscript_pch.h>

struct Config {
    bool debugMode = false;
    bool executeStatements = false; // JIT execution flag
    bool useCompiler = false;        // AOT compilation flag
    std::string filePath;
};

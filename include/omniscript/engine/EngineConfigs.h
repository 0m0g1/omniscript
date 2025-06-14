#pragma once
#include <omniscript/omniscript_pch.h>

enum class CompileMode {
    None,
    JIT,         // Execute with JIT backend
    AOT,         // Emit executable or object using AOT backend
    DryCompile   // AOT backend but skip linking or execution
};

struct Config {
    // File and target
    std::string filePath;
    std::string outputPath = "a.out"; // Default output file;
    std::string entry;               // Function to call when starting

    // Compilation mode
    CompileMode mode = CompileMode::JIT;

    // Debug and logging
    bool debugMode = false;
    bool logFinalCode = false;
    bool logAsm = false;
    bool showMetadata = false;

    // Optimization
    int optimizationLevel = 2;

    bool keepIntermediateFiles = false;
};

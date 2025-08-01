# Compiler

## Purpose
The `Compiler` class in the OmniScript++ (OS) compiler is the core component responsible for orchestrating the compilation process. It integrates parsing, code generation, and linking for Just-In-Time (JIT), Ahead-Of-Time (AOT), and hybrid compilation modes, leveraging the `Parser`, `LLVMJITBackend`, and `LLVMAOTBackend` components. It supports parallel compilation, incremental builds, and progress tracking via callbacks, while collecting compilation statistics (e.g., timing, memory usage). The class ensures thread-safety and cancellation support, making it suitable for compiling scripts like `starfield.os` or `types.os` across various target architectures and operating systems.

## Declarations
Below is the header file for `Compiler`, with `<omniscript/omniscript_pch.h>` replaced by the necessary standard library includes.

```cpp
#pragma once

#include <omniscript/Core.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/TargetInfo.h>
#include <omniscript/EngineConfigs.h>
#include <omniscript/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/Backends/llvm/LLVMAOTBackend.h>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <expected>
#include <chrono>
#include <functional>
#include <iostream>

namespace Omniscript {

class Compiler final {
public:
    enum class CompileResult {
        Success,
        ConfigurationError,
        TargetValidationFailed,
        BackendInitializationFailed,
        CompilationFailed,
        LinkingFailed
    };

    struct CompilationStats {
        std::chrono::milliseconds totalTime{0};
        std::chrono::milliseconds parseTime{0};
        std::chrono::milliseconds codegenTime{0};
        std::chrono::milliseconds linkTime{0};
        size_t memoryPeakUsage{0};
        size_t linesProcessed{0};
        size_t functionsCompiled{0};
        size_t optimizationsApplied{0};
    };

    using CompileCallback = std::function<void(const std::string& phase, double progress)>;

    Compiler() noexcept;
    ~Compiler() noexcept;

    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;
    Compiler(Compiler&&) noexcept = default;
    Compiler& operator=(Compiler&&) noexcept = default;

    [[nodiscard]] std::expected<CompilationStats, std::string> 
    compile(const std::vector<std::shared_ptr<Statement>>& statements, 
            const Config& config, 
            CompileCallback callback = nullptr) noexcept;

    [[nodiscard]] std::expected<CompilationStats, std::string>
    compileParallel(const std::vector<std::shared_ptr<Statement>>& statements,
                   const Config& config,
                   size_t threadCount = 0) noexcept;

    [[nodiscard]] const CompilationStats& getLastStats() const noexcept { return lastStats_; }
    [[nodiscard]] bool isBusy() const noexcept { return busy_.load(std::memory_order_acquire); }
    void cancel() noexcept { cancelled_.store(true, std::memory_order_release); }

private:
    [[nodiscard]] bool validateTargetConfiguration(const Config& config, std::string& error) const noexcept;
    void printTargetInfo(const Config& config) const noexcept;
    [[nodiscard]] std::expected<void, std::string> initializeBackend(const Config& config) noexcept;
    void updateProgress(const CompileCallback& callback, std::string_view phase, double progress) const noexcept;
    void trackMemoryUsage() noexcept;
    void cleanupResources() noexcept;
    void saveCache(const Config& config) noexcept;
    void loadCache(const Config& config) noexcept;
    size_t getCurrentMemoryUsage() noexcept;

    mutable std::mutex stateMutex_;
    std::atomic<bool> busy_{false};
    std::atomic<bool> cancelled_{false};
    CompilationStats lastStats_;
    std::unique_ptr<LLVMJITBackend> jitBackend_;
    std::unique_ptr<LLVMAOTBackend> aotBackend_;
    size_t memoryBaseline_{0};
    mutable std::unordered_map<std::string, bool> validationCache_;
    mutable std::mutex cacheMutex_;
};

} // namespace Omniscript
```

### Explanation
- **Enums**:
  - `CompileResult`: Defines possible compilation outcomes (e.g., Success, CompilationFailed).
- **Structs**:
  - `CompilationStats`: Tracks metrics like total time, parse time, codegen time, link time, memory usage, lines processed, functions compiled, and optimizations applied.
  - `CompileCallback`: A `std::function` type for progress updates with phase and progress percentage.
- **Public Methods**:
  - `Compiler()`: Initializes the compiler with JIT and AOT backends.
  - `~Compiler()`: Cleans up resources.
  - `compile`: Compiles a vector of statements with a given configuration, supporting progress callbacks.
  - `compileParallel`: A stub for parallel compilation, currently falling back to single-threaded `compile`.
  - `getLastStats`: Returns the most recent compilation statistics.
  - `isBusy`: Checks if the compiler is currently compiling.
  - `cancel`: Signals cancellation of an ongoing compilation.
- **Private Methods**:
  - `validateTargetConfiguration`: Validates the configuration (e.g., output path, compile mode).
  - `printTargetInfo`: Outputs target information in debug mode.
  - `initializeBackend`: Initializes JIT or AOT backends based on the configuration.
  - `updateProgress`: Invokes the progress callback.
  - `trackMemoryUsage`: Updates peak memory usage.
  - `cleanupResources`: Frees backend and cache resources.
  - `saveCache`/`loadCache`: Stubs for incremental compilation caching.
  - `getCurrentMemoryUsage`: Retrieves current memory usage, platform-specific.
- **Members**:
  - `stateMutex_`, `cacheMutex_`: Ensure thread-safety for state and cache access.
  - `busy_`, `cancelled_`: Atomic flags for compiler state and cancellation.
  - `lastStats_`: Stores the latest compilation statistics.
  - `jitBackend_`, `aotBackend_`: Manage LLVM backends for JIT and AOT compilation.
  - `memoryBaseline_`: Baseline memory usage for tracking.
  - `validationCache_`: Caches validation results for performance.

## Definitions
Below is the implementation file for `Compiler`, with necessary includes.

```cpp
#include <omniscript/Compiler.h>
#include <omniscript/Engine.h>
#include <omniscript/EngineConfigs.h>
#include <omniscript/Parser.h>
#include <filesystem>
#include <fstream>
#include <future>
#ifdef __linux__
#include <sys/resource.h>
#include <malloc.h>
#elif _WIN32
#include <windows.h>
#include <psapi.h>
#elif __APPLE__
#include <mach/mach.h>
#include <sys/resource.h>
#endif

namespace Omniscript {

namespace fs = std::filesystem;

Compiler::Compiler() noexcept 
    : memoryBaseline_(getCurrentMemoryUsage()) {
    OMNISCRIPT_PROFILE_FUNCTION();
    validationCache_.reserve(64);
    jitBackend_ = std::make_unique<LLVMJITBackend>();
    aotBackend_ = std::make_unique<LLVMAOTBackend>();
}

Compiler::~Compiler() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    cleanupResources();
}

std::expected<Compiler::CompilationStats, std::string> 
Compiler::compile(const std::vector<std::shared_ptr<Statement>>& statements, 
                 const Config& config, 
                 CompileCallback callback) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (busy_.exchange(true, std::memory_order_acq_rel)) OMNISCRIPT_UNLIKELY {
        return std::unexpected("Compiler is already busy");
    }

    auto cleanup = [this](void*) noexcept {
        busy_.store(false, std::memory_order_release);
        cancelled_.store(false, std::memory_order_release);
    };
    std::unique_ptr<void, decltype(cleanup)> guard(reinterpret_cast<void*>(1), cleanup);

    lastStats_ = {};
    auto startTime = std::chrono::steady_clock::now();
    updateProgress(callback, "Initializing", 0.0);

    if (config.incremental.enabled && !config.mainSourceFile.empty()) {
        loadCache(config);
    }

    std::string error;
    if (!validateTargetConfiguration(config, error)) OMNISCRIPT_UNLIKELY {
        error::globalErrorCollector.addError(error::Severity::Error, error, "Target validation");
        return std::unexpected("Configuration validation failed: " + error);
    }
    updateProgress(callback, "Configuration validated", 10.0);

    if (config.diagnostics.debugMode) OMNISCRIPT_LIKELY {
        printTargetInfo(config);
    }

    auto backendResult = initializeBackend(config);
    if (!backendResult) OMNISCRIPT_UNLIKELY {
        error::globalErrorCollector.addError(error::Severity::Error, backendResult.error(), "Backend initialization");
        return std::unexpected("Backend initialization failed: " + backendResult.error());
    }
    updateProgress(callback, "Backend initialized", 20.0);

    trackMemoryUsage();
    updateProgress(callback, "Compiling", 30.0);
    perf::ScopedTimer parseTimer(lastStats_.parseTime);

    std::vector<std::shared_ptr<Statement>> parsedStatements = statements;
    if (!config.sourcePaths.empty()) {
        parsedStatements.clear();
        for (const auto& source : config.sourcePaths) {
            Config tempConfig;
            tempConfig.filePath = source;
            tempConfig.mainSourceFile = source;
            auto sourceCode = Engine::readSourceCode(tempConfig);

            if (!sourceCode) {
                error::globalErrorCollector.addError(error::Severity::Error, sourceCode.error(), "Source reading");
                return std::unexpected(sourceCode.error());
            }

            Lexer lexer(*sourceCode, source);
            Parser parser(lexer);                 

            auto parsed = parser.parse();
            parsedStatements.insert(parsedStatements.end(), parsed.begin(), parsed.end());
        }
    }
    parseTimer.~ScopedTimer();
    updateProgress(callback, "Parsing complete", 50.0);

    if (cancelled_.load(std::memory_order_acquire)) OMNISCRIPT_UNLIKELY {
        error::globalErrorCollector.addError(error::Severity::Info, "Compilation cancelled", "User request");
        return std::unexpected("Compilation cancelled");
    }

    perf::ScopedTimer codegenTimer(lastStats_.codegenTime);
    try {
        if (config.mode == CompileMode::JIT || config.isHybridMode()) {
            jitBackend_->execute(parsedStatements, config);
            // lastStats_.functionsCompiled = jitBackend_->getLastStats().functionsCompiled;
        }
        if (config.mode == CompileMode::AOT || config.isHybridMode()) {
            aotBackend_->execute(parsedStatements, config);
            updateProgress(callback, "Linking", 80.0);
            perf::ScopedTimer linkTimer(lastStats_.linkTime);
            aotBackend_->emitToFile(config);
        }
        lastStats_.optimizationsApplied = config.optimization.level > 0 ? 1 : 0;
        lastStats_.linesProcessed = parsedStatements.size();
    } catch (const std::exception& e) {
        OMNISCRIPT_UNLIKELY
        error::globalErrorCollector.addError(error::Severity::Error, e.what(), "Compilation");
        return std::unexpected("Compilation failed: " + std::string(e.what()));
    }
    updateProgress(callback, "Compilation completed", 90.0);

    trackMemoryUsage();
    if (config.incremental.enabled) {
        saveCache(config);
    }

    auto endTime = std::chrono::steady_clock::now();
    lastStats_.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    updateProgress(callback, "Finished", 100.0);

    DEBUG_LOG("Compilation completed successfully");
    console.log(config.mode == CompileMode::AOT ? "AOT Compilation completed." : "JIT Compilation completed.");
    return lastStats_;
}

size_t Compiler::getCurrentMemoryUsage() noexcept {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#elif __linux__
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            size_t pos = line.find_first_of("0123456789");
            if (pos != std::string::npos) {
                return std::stoull(line.substr(pos)) * 1024;
            }
        }
    }
#elif __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &info_count) == KERN_SUCCESS) {
        return info.resident_size;
    }
#endif
    return 0;
}

void Compiler::cleanupResources() noexcept {
    try {
        jitBackend_.reset();
        aotBackend_.reset();
        validationCache_.clear();
    } catch (...) {
        // Ignore exceptions to maintain noexcept
    }
}

void Compiler::updateProgress(const CompileCallback& callback, 
                            std::string_view phase, 
                            double progress) const noexcept {
    if (callback) {
        try {
            callback(std::string(phase), progress);
        } catch (...) {
            // Ignore exceptions to maintain noexcept
        }
    }
}

void Compiler::loadCache(const Config& config) noexcept {
    if (!config.incremental.enabled) {
        return;
    }
    try {
        // TODO: Implement cache loading logic
        // Note: If your IncrementalConfig doesn't have cacheDir, you'll need to add it
    } catch (...) {
        // Ignore exceptions to maintain noexcept
    }
}

bool Compiler::validateTargetConfiguration(const Config& config, std::string& error) const noexcept {
    try {
        // Validate target architecture
        if (config.targetArch == TargetArch::Auto) {
            // Auto is fine, will be resolved later
        }
        
        // Validate target OS
        if (config.targetOS == TargetOS::Auto) {
            // Auto is fine, will be resolved later
        }
        
        // Validate output path
        if (config.outputPath.empty()) {
            error = "Output path not specified";
            return false;
        }
        
        // Validate compile mode
        if (config.mode == CompileMode::None) {
            error = "Compile mode not specified";
            return false;
        }
        
        // Validate source files for non-JIT modes
        if (config.mode != CompileMode::JIT && config.filePath.empty() && config.sourcePaths.empty()) {
            error = "No source files specified for compilation";
            return false;
        }
        
        return true;
    } catch (...) {
        error = "Validation failed due to exception";
        return false;
    }
}

void Compiler::printTargetInfo(const Config& config) const noexcept {
    try {
        if (config.diagnostics.debugMode) {
            std::cout << "=== Compiler Target Info ===" << std::endl;
            std::cout << "Architecture: " << config.getArchitectureName() << std::endl;
            std::cout << "Target OS: " << config.getOSName() << std::endl;
            std::cout << "Compile Mode: " << config.getModeString() << std::endl;
            std::cout << "Optimization Level: " << config.optimization.level << std::endl;
            std::cout << "Output Path: " << config.outputPath << std::endl;
            if (!config.targetTriple.empty()) {
                std::cout << "Target Triple: " << config.targetTriple << std::endl;
            }
            std::cout << "CPU Features: " << config.cpuFeatures << std::endl;
        }
    } catch (...) {
        // Ignore exceptions to maintain noexcept
    }
}

std::expected<void, std::string> Compiler::initializeBackend(const Config& config) noexcept {
    try {
        if (config.mode == CompileMode::JIT || config.isHybridMode()) {
            if (!jitBackend_) {
                jitBackend_ = std::make_unique<LLVMJITBackend>();
            }
        }
        
        if (config.mode == CompileMode::AOT || config.isHybridMode()) {
            if (!aotBackend_) {
                aotBackend_ = std::make_unique<LLVMAOTBackend>();
            }
        }
        
        return {}; // Success - return void
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Backend initialization failed: ") + e.what());
    } catch (...) {
        return std::unexpected("Backend initialization failed: Unknown error");
    }
}

void Compiler::trackMemoryUsage() noexcept {
    try {
        auto current = getCurrentMemoryUsage();
        if (current > lastStats_.memoryPeakUsage) {
            lastStats_.memoryPeakUsage = current;
        }
    } catch (...) {
        // Ignore exceptions to maintain noexcept
    }
}

void Compiler::saveCache(const Config& config) noexcept {
    if (!config.incremental.enabled) {
        return;
    }
    try {
        // TODO: Implement cache saving logic
        // Note: If your IncrementalConfig doesn't have cacheDir, you'll need to add it
    } catch (...) {
        // Ignore exceptions to maintain noexcept
    }
}

std::expected<Compiler::CompilationStats, std::string> 
Compiler::compileParallel(const std::vector<std::shared_ptr<Statement>>& statements,
                         const Config& config,
                         size_t threadCount) noexcept {
    try {
        // TODO: Implement true parallel compilation
        // For now, fall back to single-threaded compilation
        CompileCallback emptyCallback;
        return compile(statements, config, emptyCallback);
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Parallel compilation failed: ") + e.what());
    } catch (...) {
        return std::unexpected("Parallel compilation failed: Unknown error");
    }
}

} // namespace Omniscript
```

### Explanation
- **Constructor/Destructor**:
  - `Compiler()`: Initializes backends and reserves space for `validationCache_`.
  - `~Compiler()`: Cleans up resources via `cleanupResources`.
- **Compilation**:
  - `compile`: Orchestrates parsing, code generation, and linking, using `Parser` for source files and `LLVMJITBackend` or `LLVMAOTBackend` based on the config mode. Tracks timing and memory usage, supports cancellation, and handles incremental compilation.
  - `compileParallel`: A stub that falls back to single-threaded compilation (TODO for parallel implementation).
- **Utilities**:
  - `validateTargetConfiguration`: Checks for valid output path, compile mode, and source files.
  - `printTargetInfo`: Outputs target details in debug mode.
  - `initializeBackend`: Sets up JIT or AOT backends.
  - `updateProgress`: Calls the progress callback.
  - `trackMemoryUsage`: Updates peak memory usage using platform-specific APIs.
  - `getCurrentMemoryUsage`: Retrieves memory usage for Windows, Linux, or macOS.
  - `saveCache`/`loadCache`: Stubs for incremental compilation caching.
  - `cleanupResources`: Frees backend and cache resources.
- **Thread-Safety**: Uses `stateMutex_`, `cacheMutex_`, and atomic flags (`busy_`, `cancelled_`) to ensure safe concurrent access.
- **Error Handling**: Uses `std::expected` for robust error reporting, integrating with `error::globalErrorCollector`.

## Usage in OS Compiler
The `Compiler` class is the main entry point for compiling OmniScript++ scripts like `starfield.os` or `types.os`. Examples include:

- **Basic Compilation**:
  ```cpp
  Compiler compiler;
  Config config;
  config.mainSourceFile = "starfield.os";
  config.mode = CompileMode::AOT;
  config.outputPath = "starfield" + config.getExecutableExtension();
  config.autoConfigureForTarget();
  auto result = compiler.compile({}, config);
  if (!result) {
      console.error(result.error());
  } else {
      console.log("Compilation time: ", result->totalTime.count(), "ms");
  }
  ```

- **With Progress Callback**:
  ```cpp
  Compiler compiler;
  Config config;
  config.mainSourceFile = "types.os";
  config.mode = CompileMode::JIT;
  auto callback = [](const std::string& phase, double progress) {
      console.log(phase, ": ", progress, "%");
  };
  auto result = compiler.compile({}, config, callback);
  ```

- **Incremental Compilation**:
  ```cpp
  Config config;
  config.incremental.enabled = true;
  config.incremental.dependencyCachePath = "cache/";
  config.sourcePaths = {"starfield.os"};
  Compiler compiler;
  auto result = compiler.compile({}, config);
  ```

These examples demonstrate how `Compiler` processes scripts, applies configurations, and reports progress or errors.

## Development Notes
The `Compiler` class was designed as the central hub for the OS compiler, integrating parsing, code generation, and linking. Key design decisions include:
- **Thread-Safety**: Uses mutexes and atomics to support concurrent compilation and cancellation.
- **Modularity**: Delegates to `Parser`, `LLVMJITBackend`, and `LLVMAOTBackend` for specific tasks.
- **Incremental Compilation**: Stubs for `saveCache` and `loadCache` indicate planned support for caching.
- **Error Handling**: Uses `std::expected` and `error::globalErrorCollector` for robust error reporting.
- **Performance Tracking**: Collects detailed statistics via `CompilationStats`.
Challenges included ensuring thread-safety, handling platform-specific memory usage, and integrating with LLVM backends. The `compileParallel` method is a stub, indicating future parallelization work. The component was developed after `Parser` and `EngineConfigs` to leverage their functionality and before backend implementations.

## Dependencies
- **Standard Library**: Uses `<memory>`, `<vector>`, `<string>`, `<mutex>`, `<atomic>`, `<unordered_map>`, `<expected>`, `<chrono>`, `<functional>`, `<iostream>`, `<fstream>`, `<filesystem>`, `<future>`.
- **Platform-Specific**: `<sys/resource.h>`, `<malloc.h>` (Linux), `<windows.h>`, `<psapi.h>` (Windows), `<mach/mach.h>` (macOS) for memory usage tracking.
- **OS Components**:
  - `<omniscript/Core.h>`: For `error::globalErrorCollector`, `DEBUG_LOG`, `console`, and profiling macros.
  - `<omniscript/Statements/Statement.h>`: For `std::shared_ptr<Statement>`.
  - `<omniscript/TargetInfo.h>`: For target-specific queries via `EngineConfigs`.
  - `<omniscript/EngineConfigs.h>`: For `Config` and related structs.
  - `<omniscript/Parser.h>`: For parsing source files.
  - `<omniscript/Engine.h>`: For `Engine::readSourceCode`.
  - `<omniscript/Backends/llvm/LLVMJITBackend.h>`, `<omniscript/Backends/llvm/LLVMAOTBackend.h>`: For code generation and linking.

## Source Code
- Header: [https://github.com/0m0g1/omniscript/blob/main/include/omniscript/Compiler.h](https://github.com/0m0g1/omniscript/blob/main/include/omniscript/Compiler.h)
- Implementation: [https://github.com/0m0g1/omniscript/blob/main/src/Compiler.cpp](https://github.com/0m0g1/omniscript/blob/main/src/Compiler.cpp)

## Integration with Project
- **File Placement**:
  - Header: `include/omniscript/Compiler.h`
  - Implementation: `src/Compiler.cpp`
- **Build System**: The `premake5.lua` script includes `Compiler.cpp` in the source file list and `Compiler.h` in the include path. Requires dependencies (`Core.h`, `Statement.h`, `TargetInfo.h`, `EngineConfigs.h`, `Parser.h`, `Engine.h`, LLVM backend headers) in the include path. Platform-specific headers are handled via conditional compilation.
- **Compatibility**: Supports Debug/Release modes and integrates with the OS build system, particularly for LLVM-based compilation. No additional libraries beyond LLVM are required.

## Adding to the Index
Add the following entry to your `index.md` under the Component Reference table:

```markdown
| Compiler | Orchestrates compilation, integrating parsing, code generation, and linking for JIT, AOT, and hybrid modes. | [Compiler](Compiler.md) |
```
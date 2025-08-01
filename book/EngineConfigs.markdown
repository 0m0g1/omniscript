# EngineConfigs

## Purpose
The `EngineConfigs` component in the OmniScript++ (OS) compiler provides a comprehensive configuration system for controlling compilation modes, optimization settings, runtime behavior, and target-specific options. It defines enums and structs for managing Just-In-Time (JIT), Ahead-Of-Time (AOT), and hybrid compilation, along with settings for garbage collection, security, profiling, and diagnostics. This component enables the compiler to adapt to diverse use cases, such as generating executables, libraries, or WebAssembly, while supporting cross-compilation and performance tuning. It integrates with `TargetInfo` to ensure compatibility with target architectures and operating systems.

## Declarations
Below is the header file for `EngineConfigs`, with `<omniscript/omniscript_pch.h>` replaced by the necessary standard library includes.

```cpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <filesystem>
#include <regex>
#include <fstream>
#include <cstdio>
#include <omniscript/TargetInfo.h>
#include <nlohmann/json.hpp>

namespace Omniscript {

enum class CompileMode {
    None,
    JIT,
    AOT,
    DryCompile,
    Hybrid
};

enum class OutputFormat {
    Executable,
    StaticLib,
    SharedLib,
    ObjectFile,
    Assembly,
    LLVM_IR,
    Bitcode,
    MachineCode,
    Relocatable,
    Archive,
    ModuleFile,
    TextualIR,
    BinaryIR,
    PrecompiledHeader,
    WebAssembly,
    PTX,
    SPIR_V,
    DebugInfo,
    SymbolTable
};

enum class JITEngine {
    LLVM_ORC,
    LLVM_MCJIT,
    Custom
};

enum class GCStrategy {
    None,
    RefCounting,
    MarkSweep,
    Generational,
    Incremental
};

enum class SafetyLevel {
    Unsafe,
    Minimal,
    Standard,
    Paranoid
};

enum class ErrorRecoveryMode {
    StopOnFirst,
    Continue,
    Recover
};

enum class ProfilerType {
    None,
    GProf,
    Perf,
    VTune,
    Custom
};

struct ProfileGuidedOptimization {
    bool enabled = false;
    std::string profileDataPath;
    std::string profileOutputPath;
    bool instrumentForProfiling = false;
    int hotThreshold = 1000;
};

struct LinkTimeOptimization {
    bool enabled = false;
    bool thinLTO = true;
    int parallelJobs = 0;
    bool wholeProgram = false;
};

struct JITConfig {
    JITEngine engine = JITEngine::LLVM_ORC;
    bool lazyCompilation = true;
    bool enableSpeculation = false;
    size_t codeGenerationThreads = 1;
    size_t compilationThreshold = 10;
    bool enableTieredCompilation = false;
    size_t maxCodeCacheSize = 256 * 1024 * 1024;
    bool enableInlining = true;
    bool enableDebugging = false;
};

struct HybridConfig {
    size_t hotCodeThreshold = 1000;
    std::string aotOutputPath;
    bool enableDynamicFallback = true;
    OutputFormat aotFormat = OutputFormat::Executable;
};

struct AOTConfig {
    OutputFormat outputFormat = OutputFormat::Executable;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    std::vector<std::string> frameworkPaths;
    std::vector<std::string> frameworks;
    std::string linkerScript;
    std::vector<std::string> linkerFlags;
    std::string linkerPath;
    bool staticLinking = false;
    bool stripSymbols = false;
    bool generateDebugInfo = true;
    std::string debugInfoFormat = "dwarf";
    LinkTimeOptimization lto;

    void populateDefaultPaths(TargetArch arch, TargetOS os);
};

struct SecurityConfig {
    bool enableStackProtection = true;
    bool enableControlFlowIntegrity = false;
    bool enableAddressSanitizer = false;
    bool enableMemorySanitizer = false;
    bool enableThreadSanitizer = false;
    bool enableUndefinedBehaviorSanitizer = false;
    bool enablePositionIndependentCode = true;
    bool enableDataExecutionPrevention = true;
    bool enableAddressSpaceLayoutRandomization = true;

    void configureForTarget(TargetOS os);
};

struct RuntimeConfig {
    GCStrategy gcStrategy = GCStrategy::RefCounting;
    size_t heapSize = 64 * 1024 * 1024;
    size_t stackSize = 8 * 1024 * 1024;
    SafetyLevel safetyLevel = SafetyLevel::Unsafe;
    bool enableParallelGC = true;
    int gcThreads = 0;
    bool enableConcurrentGC = false;

    void adjustForArchitecture(TargetArch arch);
};

struct OptimizationConfig {
    int level = 3;
    bool enableVectorization = true;
    bool enableLoopUnrolling = true;
    bool enableFunctionInlining = true;
    bool enableTailCallOptimization = true;
    bool enableDeadCodeElimination = true;
    bool enableConstantFolding = true;
    bool enableCommonSubexpressionElimination = true;
    bool enableLoopInvariantCodeMotion = true;
    bool fastMath = false;
    ProfileGuidedOptimization pgo;
    std::unordered_map<std::string, std::string> customPasses;
    std::vector<std::string> supportedPasses;

    void configureForTarget(TargetArch arch);
};

struct DiagnosticsConfig {
    bool debugMode = false;
    bool verbose = false;
    bool logFinalCode = false;
    bool logAsm = false;
    bool showMetadata = false;
    bool logOptimizationRemarks = false;
    bool logTimings = false;
    bool generateReports = false;
    std::string reportOutputPath;
    bool enableProfiling = false;
    bool measureMemoryUsage = false;
    int warningLevel = 2;
    bool warningsAsErrors = false;
    std::vector<std::string> suppressedWarnings;
};

struct ModuleConfig {
    bool enableModules = false;
    std::vector<std::string> modulePaths;
    std::string moduleCachePath;
    bool exportModules = false;
};

struct ProfilerConfig {
    ProfilerType type = ProfilerType::None;
    std::string outputPath;
    bool enableSampling = false;
    int samplingFrequency = 1000;
};

struct ErrorHandlingConfig {
    ErrorRecoveryMode mode = ErrorRecoveryMode::Continue;
    size_t maxErrorCount = 100;
    bool logToFile = false;
    std::string errorLogPath;
};

struct IncrementalConfig {
    bool enabled = false;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> fileTimestamps;
    std::string dependencyCachePath;
};

struct Config {
    std::string filePath;
    std::string mainSourceFile;
    std::string outputPath = "a.out";
    std::string entry;
    CompileMode mode = CompileMode::JIT;
    std::vector<std::pair<TargetArch, TargetOS>> multiTargets;
    std::string targetTriple;
    std::string cpuFeatures = "native";
    std::string toolchainPath;
    std::vector<std::string> includePaths;
    std::vector<std::string> sourcePaths;
    std::vector<std::string> importPaths;
    std::vector<std::string> precompiledHeaders;
    std::unordered_map<std::string, std::string> defines;
    std::vector<std::string> undefines;
    bool enablePreprocessorOutput = false;
    JITConfig jit;
    AOTConfig aot;
    HybridConfig hybrid;
    ModuleConfig modules;
    IncrementalConfig incremental;
    OptimizationConfig optimization;
    RuntimeConfig runtime;
    SecurityConfig security;
    ProfilerConfig profiler;
    ErrorHandlingConfig errorHandling;
    DiagnosticsConfig diagnostics;
    bool keepIntermediateFiles = false;
    std::string tempDirectory;
    int parallelJobs = 0;
    bool enableCaching = true;
    std::string cacheDirectory;
    std::string languageStandard = "latest";
    bool enableExperimentalFeatures = false;
    std::vector<std::string> enabledFeatures;
    std::vector<std::string> disabledFeatures;
    struct PluginConfig {
        std::string name;
        std::string path;
        std::string version;
        std::unordered_map<std::string, std::string> options;
        bool (*callback)(const Config&, void*) = nullptr;
    };
    std::vector<PluginConfig> plugins;
    std::unordered_map<std::string, std::string> environmentVariables;
    std::string workingDirectory;
    size_t maxCompilationTime = 0;
    size_t maxMemoryUsage = 0;
    TargetArch targetArch = TargetArch::Auto;
    TargetOS targetOS = TargetOS::Auto;

    bool isJITMode() const;
    bool isAOTMode() const;
    bool isHybridMode() const;
    bool isDryRun() const;
    bool isCrossCompilation() const;
    std::string getArchitectureName(TargetArch arch = TargetArch::Auto) const;
    std::string getOSName(TargetOS os = TargetOS::Auto) const;
    bool isArchitecture64Bit(TargetArch arch = TargetArch::Auto) const;
    int getPointerSize(TargetArch arch = TargetArch::Auto) const;
    std::string getExecutableExtension(TargetOS os = TargetOS::Auto) const;
    std::string getSharedLibExtension(TargetOS os = TargetOS::Auto) const;
    std::string getStaticLibExtension(TargetOS os = TargetOS::Auto) const;
    std::string getObjectFileExtension(TargetOS os = TargetOS::Auto) const;
    bool isUnixLikeOS(TargetOS os = TargetOS::Auto) const;
    std::vector<std::string> getAvailableFeatures(TargetArch arch = TargetArch::Auto) const;
    bool supportsFeature(const std::string& feature, TargetArch arch = TargetArch::Auto) const;
    std::string getDefaultCPU(TargetArch arch = TargetArch::Auto) const;
    std::string getEffectiveTargetTriple(TargetArch arch = TargetArch::Auto, TargetOS os = TargetOS::Auto) const;
    std::string getTargetSummary(TargetArch arch = TargetArch::Auto, TargetOS os = TargetOS::Auto) const;
    void autoConfigureForTarget();
    bool validate(std::string& errorMessage) const;
    bool loadFromFile(const std::string& configPath);
    bool saveToFile(const std::string& configPath) const;
    void mergeWith(const Config& other);
    void printSummary() const;
    TargetArch resolveTargetArch() const;
    TargetOS resolveTargetOS() const;
    std::string getModeString() const;

private:
    static bool isValidVersion(const std::string& version);
    static std::string profilerTypeToString(ProfilerType type);
};

} // namespace Omniscript
```

### Explanation
- **Enums**:
  - `CompileMode`: Defines compilation modes (JIT, AOT, Hybrid, DryCompile, None).
  - `OutputFormat`: Specifies output types (e.g., Executable, LLVM_IR, WebAssembly).
  - `JITEngine`: Lists JIT engines (LLVM_ORC, LLVM_MCJIT, Custom).
  - `GCStrategy`: Defines garbage collection strategies (e.g., RefCounting, MarkSweep).
  - `SafetyLevel`: Sets safety levels (Unsafe, Minimal, Standard, Paranoid).
  - `ErrorRecoveryMode`: Controls error handling (StopOnFirst, Continue, Recover).
  - `ProfilerType`: Specifies profiling tools (None, GProf, Perf, VTune, Custom).
- **Structs**:
  - `ProfileGuidedOptimization`: Configures profile-guided optimization (PGO) settings.
  - `LinkTimeOptimization`: Manages link-time optimization (LTO) options.
  - `JITConfig`: Configures JIT compilation parameters (e.g., engine, lazy compilation).
  - `HybridConfig`: Sets hybrid compilation options (e.g., hot code threshold).
  - `AOTConfig`: Defines AOT compilation settings (e.g., output format, linker options).
  - `SecurityConfig`: Enables security features (e.g., stack protection, ASLR).
  - `RuntimeConfig`: Configures runtime settings (e.g., heap size, GC strategy).
  - `OptimizationConfig`: Manages optimization passes and levels.
  - `DiagnosticsConfig`: Controls diagnostic output (e.g., logging, profiling).
  - `ModuleConfig`: Handles module-based compilation.
  - `ProfilerConfig`: Configures profiling settings.
  - `ErrorHandlingConfig`: Manages error handling behavior.
  - `IncrementalConfig`: Supports incremental compilation with file timestamps.
  - `Config`: Aggregates all configurations, including paths, targets, and plugins.
- **Methods**:
  - `Config` methods query target info (e.g., `getArchitectureName`, `isCrossCompilation`) via `TargetInfo`.
  - `autoConfigureForTarget`: Sets default paths and configurations based on target.
  - `validate`: Checks configuration validity (e.g., triple, feature support).
  - `loadFromFile`/`saveToFile`: Serialize/deserialize config to/from JSON.
  - `mergeWith`: Combines configurations with precedence rules.
  - `printSummary`: Outputs a configuration overview.

## Definitions
Below is the implementation file for `EngineConfigs`, with necessary includes.

```cpp
#include <omniscript/EngineConfigs.h>
#include <fstream>

namespace Omniscript {

void AOTConfig::populateDefaultPaths(TargetArch arch, TargetOS os) {
    if (libraryPaths.empty()) {
        libraryPaths = TargetInfo::getDefaultLibraryPaths(os, arch);
    }
    auto sysLibs = TargetInfo::getRequiredSystemLibraries(os);
    for (const auto& lib : sysLibs) {
        if (std::find(libraries.begin(), libraries.end(), lib) == libraries.end()) {
            libraries.push_back(lib);
        }
    }
}

void SecurityConfig::configureForTarget(TargetOS os) {
    auto osInfo = TargetInfo::getOSInfo(os);
    enablePositionIndependentCode = osInfo.supportsPIC;
    if (os == TargetOS::WebAssembly) {
        enableStackProtection = false;
        enableAddressSpaceLayoutRandomization = false;
        enableDataExecutionPrevention = false;
    }
}

void RuntimeConfig::adjustForArchitecture(TargetArch arch) {
    auto archInfo = TargetInfo::getArchitectureInfo(arch);
    if (!archInfo.is64Bit) {
        heapSize = std::min(heapSize, size_t(512 * 1024 * 1024));
        stackSize = std::min(stackSize, size_t(4 * 1024 * 1024));
    }
    if (arch == TargetArch::WASM32 || arch == TargetArch::WASM64) {
        enableParallelGC = false;
        enableConcurrentGC = false;
        gcThreads = 1;
    }
}

void OptimizationConfig::configureForTarget(TargetArch arch) {
    auto features = TargetInfo::getAvailableFeatures(arch);
    if (std::find(features.begin(), features.end(), "sse") != features.end() ||
        std::find(features.begin(), features.end(), "neon") != features.end() ||
        std::find(features.begin(), features.end(), "simd128") != features.end()) {
        enableVectorization = true;
    }
    if (arch == TargetArch::ARM32 || arch == TargetArch::WASM32) {
        level = std::min(level, 2);
    }
    supportedPasses = {"dce", "inlining", "vectorize", "loop-unroll", "const-fold", "cse", "licm"};
}

bool Config::isJITMode() const { return mode == CompileMode::JIT || mode == CompileMode::Hybrid; }
bool Config::isAOTMode() const { return mode == CompileMode::AOT || mode == CompileMode::DryCompile; }
bool Config::isHybridMode() const { return mode == CompileMode::Hybrid; }
bool Config::isDryRun() const { return mode == CompileMode::DryCompile; }

bool Config::isCrossCompilation() const {
    if (multiTargets.empty()) {
        return TargetInfo::isCrossCompilation(resolveTargetArch(), resolveTargetOS());
    }
    return std::any_of(multiTargets.begin(), multiTargets.end(), [](const auto& pair) {
        return TargetInfo::isCrossCompilation(pair.first, pair.second);
    });
}

std::string Config::getArchitectureName(TargetArch arch) const {
    return TargetInfo::getArchitectureName(arch == TargetArch::Auto ? resolveTargetArch() : arch);
}

std::string Config::getOSName(TargetOS os) const {
    return TargetInfo::getOSName(os == TargetOS::Auto ? resolveTargetOS() : os);
}

bool Config::isArchitecture64Bit(TargetArch arch) const {
    return TargetInfo::isArchitecture64Bit(arch == TargetArch::Auto ? resolveTargetArch() : arch);
}

int Config::getPointerSize(TargetArch arch) const {
    return TargetInfo::getPointerSize(arch == TargetArch::Auto ? resolveTargetArch() : arch);
}

std::string Config::getExecutableExtension(TargetOS os) const {
    return TargetInfo::getExecutableExtension(os == TargetOS::Auto ? resolveTargetOS() : os);
}

std::string Config::getSharedLibExtension(TargetOS os) const {
    return TargetInfo::getSharedLibExtension(os == TargetOS::Auto ? resolveTargetOS() : os);
}

std::string Config::getStaticLibExtension(TargetOS os) const {
    return TargetInfo::getStaticLibExtension(os == TargetOS::Auto ? resolveTargetOS() : os);
}

std::string Config::getObjectFileExtension(TargetOS os) const {
    return TargetInfo::getObjectFileExtension(os == TargetOS::Auto ? resolveTargetOS() : os);
}

bool Config::isUnixLikeOS(TargetOS os) const {
    return TargetInfo::isUnixLikeOS(os == TargetOS::Auto ? resolveTargetOS() : os);
}

std::vector<std::string> Config::getAvailableFeatures(TargetArch arch) const {
    return TargetInfo::getAvailableFeatures(arch == TargetArch::Auto ? resolveTargetArch() : arch);
}

bool Config::supportsFeature(const std::string& feature, TargetArch arch) const {
    return TargetInfo::supportsFeature(arch == TargetArch::Auto ? resolveTargetArch() : arch, feature);
}

std::string Config::getDefaultCPU(TargetArch arch) const {
    return TargetInfo::getDefaultCPUForArch(arch == TargetArch::Auto ? resolveTargetArch() : arch);
}

std::string Config::getEffectiveTargetTriple(TargetArch arch, TargetOS os) const {
    if (!targetTriple.empty()) {
        return targetTriple;
    }
    return TargetInfo::generateTriple(
        arch == TargetArch::Auto ? resolveTargetArch() : arch,
        os == TargetOS::Auto ? resolveTargetOS() : os);
}

std::string Config::getTargetSummary(TargetArch arch, TargetOS os) const {
    return TargetInfo::getTargetSummary(
        arch == TargetArch::Auto ? resolveTargetArch() : arch,
        os == TargetOS::Auto ? resolveTargetOS() : os);
}

void Config::autoConfigureForTarget() {
    auto arch = resolveTargetArch();
    auto os = resolveTargetOS();
    aot.populateDefaultPaths(arch, os);
    security.configureForTarget(os);
    runtime.adjustForArchitecture(arch);
    optimization.configureForTarget(arch);
    if (outputPath == "a.out") {
        outputPath = "a" + getExecutableExtension();
    }
    if (includePaths.empty()) {
        includePaths = TargetInfo::getDefaultIncludePaths(os, arch);
    }
    if (cpuFeatures == "native" && isCrossCompilation()) {
        cpuFeatures = getDefaultCPU();
    }
}

bool Config::validate(std::string& errorMessage) const {
    auto arch = resolveTargetArch();
    auto os = resolveTargetOS();

    if (!targetTriple.empty() && !TargetInfo::isValidTriple(targetTriple)) {
        errorMessage = "Invalid target triple: " + targetTriple;
        return false;
    }
    if (isCrossCompilation() && isJITMode()) {
        errorMessage = "JIT mode is not supported for cross-compilation";
        return false;
    }
    if (isHybridMode() && isCrossCompilation()) {
        if (!TargetInfo::isArchitectureCompatible(TargetInfo::detectHostArchitecture(), arch)) {
            errorMessage = "Hybrid mode requires compatible target architecture";
            return false;
        }
    }
    auto availableFeatures = getAvailableFeatures();
    for (const auto& feature : enabledFeatures) {
        if (!supportsFeature(feature)) {
            errorMessage = "Feature '" + feature + "' is not supported on " + getArchitectureName();
            return false;
        }
    }
    if (aot.outputFormat == OutputFormat::SharedLib && aot.staticLinking) {
        errorMessage = "Cannot create shared library with static linking enabled";
        return false;
    }
    if (!toolchainPath.empty() && !std::filesystem::exists(toolchainPath)) {
        errorMessage = "Toolchain path does not exist: " + toolchainPath;
        return false;
    }
    if (incremental.enabled && incremental.dependencyCachePath.empty()) {
        errorMessage = "Incremental compilation requires a dependency cache path";
        return false;
    }
    for (const auto& pass : optimization.customPasses) {
        if (std::find(optimization.supportedPasses.begin(), optimization.supportedPasses.end(), pass.first) == optimization.supportedPasses.end()) {
            errorMessage = "Unsupported optimization pass: " + pass.first;
            return false;
        }
    }
    for (const auto& plugin : plugins) {
        if (!plugin.version.empty() && !isValidVersion(plugin.version)) {
            errorMessage = "Invalid plugin version for " + plugin.name + ": " + plugin.version;
            return false;
        }
    }
    return true;
}

bool Config::loadFromFile(const std::string& configPath) {
    try {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            return false;
        }
        nlohmann::json json;
        file >> json;

        filePath = json.value("filePath", "");
        mainSourceFile = json.value("mainSourceFile", "");
        outputPath = json.value("outputPath", "a.out");
        entry = json.value("entry", "");
        mode = static_cast<CompileMode>(json.value("mode", static_cast<int>(CompileMode::JIT)));
        targetTriple = json.value("targetTriple", "");
        cpuFeatures = json.value("cpuFeatures", "native");
        toolchainPath = json.value("toolchainPath", "");
        includePaths = json.value("includePaths", std::vector<std::string>{});
        sourcePaths = json.value("sourcePaths", std::vector<std::string>{});
        importPaths = json.value("importPaths", std::vector<std::string>{});
        precompiledHeaders = json.value("precompiledHeaders", std::vector<std::string>{});
        defines = json.value("defines", std::unordered_map<std::string, std::string>{});
        undefines = json.value("undefines", std::vector<std::string>{});
        enablePreprocessorOutput = json.value("enablePreprocessorOutput", false);
        keepIntermediateFiles = json.value("keepIntermediateFiles", false);
        tempDirectory = json.value("tempDirectory", "");
        parallelJobs = json.value("parallelJobs", 0);
        enableCaching = json.value("enableCaching", true);
        cacheDirectory = json.value("cacheDirectory", "");
        languageStandard = json.value("languageStandard", "latest");
        enableExperimentalFeatures = json.value("enableExperimentalFeatures", false);
        enabledFeatures = json.value("enabledFeatures", std::vector<std::string>{});
        disabledFeatures = json.value("disabledFeatures", std::vector<std::string>{});
        environmentVariables = json.value("environmentVariables", std::unordered_map<std::string, std::string>{});
        workingDirectory = json.value("workingDirectory", "");
        maxCompilationTime = json.value("maxCompilationTime", 0);
        maxMemoryUsage = json.value("maxMemoryUsage", 0);

        if (json.contains("jit")) {
            jit.engine = static_cast<JITEngine>(json["jit"].value("engine", static_cast<int>(JITEngine::LLVM_ORC)));
            jit.lazyCompilation = json["jit"].value("lazyCompilation", true);
            jit.enableSpeculation = json["jit"].value("enableSpeculation", false);
            jit.codeGenerationThreads = json["jit"].value("codeGenerationThreads", 1);
            jit.compilationThreshold = json["jit"].value("compilationThreshold", 10);
            jit.enableTieredCompilation = json["jit"].value("enableTieredCompilation", false);
            jit.maxCodeCacheSize = json["jit"].value("maxCodeCacheSize", 256 * 1024 * 1024);
            jit.enableInlining = json["jit"].value("enableInlining", true);
            jit.enableDebugging = json["jit"].value("enableDebugging", false);
        }
        // TODO: Load other nested configs (aot, hybrid, modules, etc.) similarly
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool Config::saveToFile(const std::string& configPath) const {
    try {
        nlohmann::json json;
        json["filePath"] = filePath;
        json["mainSourceFile"] = mainSourceFile;
        json["outputPath"] = outputPath;
        json["entry"] = entry;
        json["mode"] = static_cast<int>(mode);
        json["targetTriple"] = targetTriple;
        json["cpuFeatures"] = cpuFeatures;
        json["toolchainPath"] = toolchainPath;
        json["includePaths"] = includePaths;
        json["sourcePaths"] = sourcePaths;
        json["importPaths"] = importPaths;
        json["precompiledHeaders"] = precompiledHeaders;
        json["defines"] = defines;
        json["undefines"] = undefines;
        json["enablePreprocessorOutput"] = enablePreprocessorOutput;
        json["keepIntermediateFiles"] = keepIntermediateFiles;
        json["tempDirectory"] = tempDirectory;
        json["parallelJobs"] = parallelJobs;
        json["enableCaching"] = enableCaching;
        json["cacheDirectory"] = cacheDirectory;
        json["languageStandard"] = languageStandard;
        json["enableExperimentalFeatures"] = enableExperimentalFeatures;
        json["enabledFeatures"] = enabledFeatures;
        json["disabledFeatures"] = disabledFeatures;
        json["environmentVariables"] = environmentVariables;
        json["workingDirectory"] = workingDirectory;
        json["maxCompilationTime"] = maxCompilationTime;
        json["maxMemoryUsage"] = maxMemoryUsage;

        json["jit"]["engine"] = static_cast<int>(jit.engine);
        json["jit"]["lazyCompilation"] = jit.lazyCompilation;
        json["jit"]["enableSpeculation"] = jit.enableSpeculation;
        json["jit"]["codeGenerationThreads"] = jit.codeGenerationThreads;
        json["jit"]["compilationThreshold"] = jit.compilationThreshold;
        json["jit"]["enableTieredCompilation"] = jit.enableTieredCompilation;
        json["jit"]["maxCodeCacheSize"] = jit.maxCodeCacheSize;
        json["jit"]["enableInlining"] = jit.enableInlining;
        json["jit"]["enableDebugging"] = jit.enableDebugging;
        // TODO: Serialize other nested configs (aot, hybrid, modules, etc.) similarly

        std::ofstream file(configPath);
        if (!file.is_open()) {
            return false;
        }
        file << json.dump(4);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

void Config::mergeWith(const Config& other) {
    if (!other.filePath.empty()) filePath = other.filePath;
    if (!other.mainSourceFile.empty()) mainSourceFile = other.mainSourceFile;
    if (!other.outputPath.empty()) outputPath = other.outputPath;
    if (!other.entry.empty()) entry = other.entry;
    if (other.mode != CompileMode::None) mode = other.mode;
    if (!other.targetTriple.empty()) targetTriple = other.targetTriple;
    if (!other.cpuFeatures.empty()) cpuFeatures = other.cpuFeatures;
    if (!other.toolchainPath.empty()) toolchainPath = other.toolchainPath;
    if (!other.includePaths.empty()) includePaths = other.includePaths;
    if (!other.sourcePaths.empty()) sourcePaths = other.sourcePaths;
    if (!other.importPaths.empty()) importPaths = other.importPaths;
    if (!other.precompiledHeaders.empty()) precompiledHeaders = other.precompiledHeaders;
    defines.insert(other.defines.begin(), other.defines.end());
    undefines.insert(undefines.end(), other.undefines.begin(), other.undefines.end());
    if (other.enablePreprocessorOutput) enablePreprocessorOutput = true;
    if (other.keepIntermediateFiles) keepIntermediateFiles = true;
    if (!other.tempDirectory.empty()) tempDirectory = other.tempDirectory;
    if (other.parallelJobs != 0) parallelJobs = other.parallelJobs;
    if (!other.enableCaching) enableCaching = false;
    if (!other.cacheDirectory.empty()) cacheDirectory = other.cacheDirectory;
    if (!other.languageStandard.empty()) languageStandard = other.languageStandard;
    if (other.enableExperimentalFeatures) enableExperimentalFeatures = true;
    enabledFeatures.insert(enabledFeatures.end(), other.enabledFeatures.begin(), other.enabledFeatures.end());
    disabledFeatures.insert(disabledFeatures.end(), other.disabledFeatures.begin(), other.disabledFeatures.end());
    environmentVariables.insert(other.environmentVariables.begin(), other.environmentVariables.end());
    if (!other.workingDirectory.empty()) workingDirectory = other.workingDirectory;
    if (other.maxCompilationTime != 0) maxCompilationTime = other.maxCompilationTime;
    if (other.maxMemoryUsage != 0) maxMemoryUsage = other.maxMemoryUsage;
    // TODO: Merge nested configs (jit, aot, hybrid, etc.) similarly
}

void Config::printSummary() const {
    printf("Configuration Summary:\n");
    printf("  Mode: %s\n", getModeString().c_str());
    if (multiTargets.empty()) {
        printf("  Target: %s\n", getTargetSummary().c_str());
    } else {
        printf("  Targets:\n");
        for (const auto& [arch, os] : multiTargets) {
            printf("    - %s\n", getTargetSummary(arch, os).c_str());
        }
    }
    printf("  Cross-compilation: %s\n", isCrossCompilation() ? "Yes" : "No");
    printf("  Main Source: %s\n", mainSourceFile.empty() ? filePath.c_str() : mainSourceFile.c_str());
    printf("  Output: %s\n", outputPath.c_str());
    if (isCrossCompilation()) {
        printf("  Host: %s\n", TargetInfo::detectHostTriple().c_str());
    }
    if (!toolchainPath.empty()) {
        printf("  Toolchain: %s\n", toolchainPath.c_str());
    }
    if (incremental.enabled) {
        printf("  Incremental Compilation: Enabled (Cache: %s)\n", incremental.dependencyCachePath.c_str());
    }
    if (modules.enableModules) {
        printf("  Modules: Enabled\n");
    }
    if (profiler.type != ProfilerType::None) {
        printf("  Profiler: %s\n", profilerTypeToString(profiler.type).c_str());
    }
}

TargetArch Config::resolveTargetArch() const {
    if (multiTargets.empty()) {
        return targetArch == TargetArch::Auto ? TargetInfo::detectHostArchitecture() : targetArch;
    }
    return multiTargets[0].first;
}

TargetOS Config::resolveTargetOS() const {
    if (multiTargets.empty()) {
        return targetOS == TargetOS::Auto ? TargetInfo::detectHostOS() : targetOS;
    }
    return multiTargets[0].second;
}

std::string Config::getModeString() const {
    switch (mode) {
        case CompileMode::JIT: return "JIT";
        case CompileMode::AOT: return "AOT";
        case CompileMode::DryCompile: return "Dry Compile";
        case CompileMode::Hybrid: return "Hybrid";
        default: return "None";
    }
}

bool Config::isValidVersion(const std::string& version) {
    std::regex versionRegex(R"(\d+\.\d+\.\d+)");
    return std::regex_match(version, versionRegex);
}

std::string Config::profilerTypeToString(ProfilerType type) {
    switch (type) {
        case ProfilerType::None: return "None";
        case ProfilerType::GProf: return "GProf";
        case ProfilerType::Perf: return "Perf";
        case ProfilerType::VTune: return "VTune";
        case ProfilerType::Custom: return "Custom";
        default: return "Unknown";
    }
}

} // namespace Omniscript
```

### Explanation
- **AOTConfig::populateDefaultPaths**: Sets default library paths and system libraries using `TargetInfo`.
- **SecurityConfig::configureForTarget**: Adjusts security settings based on OS, disabling certain features for WebAssembly.
- **RuntimeConfig::adjustForArchitecture**: Limits heap/stack sizes for 32-bit architectures and disables parallel GC for WebAssembly.
- **OptimizationConfig::configureForTarget**: Enables vectorization for architectures with SIMD support and limits optimization level for certain architectures.
- **Config Methods**:
  - Mode checks (`isJITMode`, `isAOTMode`, etc.) determine the compilation mode.
  - Target queries (`getArchitectureName`, `getPointerSize`, etc.) delegate to `TargetInfo` with resolved targets.
  - `autoConfigureForTarget`: Configures paths, security, runtime, and optimization settings based on target.
  - `validate`: Checks for invalid configurations (e.g., JIT with cross-compilation, invalid triples).
  - `loadFromFile`/`saveToFile`: Handle JSON serialization/deserialization, with partial implementation for nested configs.
  - `mergeWith`: Combines configurations, prioritizing non-empty/non-default values.
  - `printSummary`: Outputs a concise configuration overview.
  - `resolveTargetArch`/`resolveTargetOS`: Resolve target architecture/OS, preferring `multiTargets` or falling back to `targetArch`/`targetOS`.
  - `isValidVersion`: Validates plugin version strings using a regex.
  - `profilerTypeToString`: Converts `ProfilerType` to string for display.

## Usage in OS Compiler
The `EngineConfigs` component is used to configure the compiler’s behavior for scripts like `starfield.os` or `types.os`. Examples include:

- **Loading Configuration**:
  ```cpp
  Config config;
  if (!config.loadFromFile("config.json")) {
      console.error("Failed to load config");
      return 1;
  }
  config.autoConfigureForTarget();
  ```

- **AOT Compilation**:
  ```cpp
  if (config.isAOTMode()) {
      llvm::Triple triple(config.getEffectiveTargetTriple());
      auto target = llvm::TargetRegistry::lookupTarget(triple.str(), error);
      config.aot.populateDefaultPaths(config.resolveTargetArch(), config.resolveTargetOS());
  }
  ```

- **Error Handling**:
  ```cpp
  std::string error;
  if (!config.validate(error)) {
      error::globalErrorCollector.addError(error::Severity::Error, error);
      return 1;
  }
  ```

- **Profiling**:
  ```cpp
  if (config.diagnostics.enableProfiling) {
      OMNISCRIPT_PROFILE_FUNCTION();
      compile_program();
  }
  ```

These configurations ensure the compiler adapts to the target platform and user requirements, such as generating WebAssembly for `starfield.os` or optimizing `types.os` for performance.

## Development Notes
The `EngineConfigs` component was developed to centralize compiler configuration, supporting flexibility for JIT, AOT, and hybrid modes. Key design decisions include:
- **JSON Serialization**: Using `nlohmann::json` for configuration persistence, though nested config loading/saving is incomplete (noted with TODOs).
- **Target Integration**: Delegates to `TargetInfo` for platform-specific settings, ensuring consistency.
- **Extensibility**: Supports plugins and custom optimization passes for future extensions.
- **Validation**: Robust checks in `validate` prevent invalid configurations (e.g., JIT with cross-compilation).
Challenges included balancing flexibility with complexity, particularly for nested structs. The JSON-based configuration simplifies command-line and file-based input but requires careful error handling. The component was developed after `TargetInfo` to leverage its utilities and before the compiler frontend to provide configuration for parsing and code generation.

## Dependencies
- **Standard Library**: Uses `<string>`, `<vector>`, `<unordered_map>`, `<chrono>`, `<optional>`, `<filesystem>`, `<regex>`, `<fstream>`, `<cstdio>` for data structures and file operations.
- **External Library**: `<nlohmann/json.hpp>` for JSON serialization/deserialization.
- **OS Components**: `<omniscript/TargetInfo.h>` for target-specific queries and path configuration. Indirectly interacts with `Core` (e.g., `error::globalErrorCollector` for validation errors) and `Console` for logging.
- **Note**: `<optional>` is included but unused; it may be intended for future extensions.

## Source Code
- Header: [https://github.com/0m0g1/omniscript/blob/main/include/omniscript/EngineConfigs.h](https://github.com/0m0g1/omniscript/blob/main/include/omniscript/EngineConfigs.h)
- Implementation: [https://github.com/0m0g1/omniscript/blob/main/src/EngineConfigs.cpp](https://github.com/0m0g1/omniscript/blob/main/src/EngineConfigs.cpp)

## Integration with Project
- **File Placement**:
  - Header: `include/omniscript/EngineConfigs.h`
  - Implementation: `src/EngineConfigs.cpp`
- **Build System**: The `premake5.lua` script includes `EngineConfigs.cpp` in the source file list and `EngineConfigs.h` in the include path. Requires `TargetInfo.h` and `nlohmann/json.hpp` in the include path. The build system must link against the JSON library if it’s not header-only.
- **Compatibility**: Supports Debug/Release modes and integrates with the OS build system, particularly for LLVM-based compilation. No platform-specific dependencies beyond `TargetInfo`.

## Adding to the Index
Add the following entry to your `index.md` under the Component Reference table:

```markdown
| EngineConfigs | Configures compilation modes, optimizations, runtime, and target-specific settings. | [EngineConfigs](EngineConfigs.md) |
```
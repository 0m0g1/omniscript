#pragma once
#include <omniscript/omniscript_pch.h>
#include <omniscript/Target_config.h>
#include <nlohmann/json.hpp> // For JSON serialization
#include <filesystem>
#include <chrono>
#include <optional>

namespace Omniscript {

enum class CompileMode {
    None,
    JIT,         // Execute with JIT backend
    AOT,         // Emit executable or object using AOT backend
    DryCompile,  // AOT backend but skip linking or execution
    Hybrid       // JIT with AOT fallback for hot code
};

enum class OutputFormat {
    Executable,      // Final executable
    StaticLib,       // Static library (.a/.lib)
    SharedLib,       // Shared library (.so/.dll/.dylib)
    ObjectFile,      // Object file (.o/.obj)
    Assembly,        // Assembly source (.s/.asm)
    LLVM_IR,         // LLVM intermediate representation (.ll)
    Bitcode,         // LLVM bitcode (.bc)
    MachineCode,     // Raw machine code (binary)
    Relocatable,     // Relocatable object
    Archive,         // Archive file (alias for StaticLib)
    ModuleFile,      // LLVM module file
    TextualIR,       // Human-readable LLVM IR
    BinaryIR,        // Binary LLVM IR (alias for Bitcode)
    PrecompiledHeader, // Precompiled header (.pch/.gch)
    WebAssembly,     // WebAssembly module (.wasm)
    PTX,             // NVIDIA PTX assembly (for CUDA)
    SPIR_V,          // SPIR-V for OpenCL/Vulkan
    DebugInfo,       // Debug information file (.dSYM/.pdb)
    SymbolTable      // Symbol table file
};

enum class JITEngine {
    LLVM_ORC,    // LLVM ORC JIT (recommended)
    LLVM_MCJIT,  // Legacy LLVM MCJIT
    Custom       // Custom JIT implementation
};

enum class GCStrategy {
    None,        // No garbage collection
    RefCounting, // Reference counting
    MarkSweep,   // Mark and sweep
    Generational,// Generational GC
    Incremental  // Incremental GC
};

enum class SafetyLevel {
    Unsafe,      // No safety checks (maximum performance)
    Minimal,     // Basic null/bounds checks
    Standard,    // Standard safety checks
    Paranoid     // Extensive runtime validation
};

enum class ErrorRecoveryMode {
    StopOnFirst,    // Stop compilation on first error
    Continue,       // Continue compilation, report all errors
    Recover         // Attempt recovery and continue
};

enum class ProfilerType {
    None,           // No profiling
    GProf,          // GNU profiler
    Perf,           // Linux perf
    VTune,          // Intel VTune
    Custom          // Custom profiler
};

struct ProfileGuidedOptimization {
    bool enabled = false;
    std::string profileDataPath;
    std::string profileOutputPath;
    bool instrumentForProfiling = false;
    int hotThreshold = 1000;  // Call count threshold for hot functions
};

struct LinkTimeOptimization {
    bool enabled = false;
    bool thinLTO = true;      // Use ThinLTO instead of full LTO
    int parallelJobs = 0;     // 0 = auto-detect
    bool wholeProgram = false;
};

struct JITConfig {
    JITEngine engine = JITEngine::LLVM_ORC;
    bool lazyCompilation = true;
    bool enableSpeculation = false;
    size_t codeGenerationThreads = 1;
    size_t compilationThreshold = 10;  // Execute count before JIT compilation
    bool enableTieredCompilation = false;
    size_t maxCodeCacheSize = 256 * 1024 * 1024; // 256MB default
    bool enableInlining = true;
    bool enableDebugging = false;
};

struct HybridConfig {
    size_t hotCodeThreshold = 1000; // Execution count for AOT fallback
    std::string aotOutputPath;      // Path for AOT-compiled hot code
    bool enableDynamicFallback = true; // Dynamically switch to AOT
    OutputFormat aotFormat = OutputFormat::Executable;
};

struct AOTConfig {
    OutputFormat outputFormat = OutputFormat::Executable;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    std::vector<std::string> frameworkPaths; // macOS/iOS
    std::vector<std::string> frameworks;     // macOS/iOS
    std::string linkerScript;
    std::vector<std::string> linkerFlags;
    std::string linkerPath; // Custom linker (e.g., lld, gold)
    bool staticLinking = false;
    bool stripSymbols = false;
    bool generateDebugInfo = true;
    std::string debugInfoFormat = "dwarf"; // dwarf, codeview, etc.
    LinkTimeOptimization lto;

    void populateDefaultPaths(TargetArch arch, TargetOS os) {
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

    void configureForTarget(TargetOS os) {
        auto osInfo = TargetInfo::getOSInfo(os);
        enablePositionIndependentCode = osInfo.supportsPIC;
        if (os == TargetOS::WebAssembly) {
            enableStackProtection = false;
            enableAddressSpaceLayoutRandomization = false;
            enableDataExecutionPrevention = false;
        }
    }
};

struct RuntimeConfig {
    GCStrategy gcStrategy = GCStrategy::RefCounting;
    size_t heapSize = 64 * 1024 * 1024; // 64MB default
    size_t stackSize = 8 * 1024 * 1024;  // 8MB default
    SafetyLevel safetyLevel = SafetyLevel::Unsafe;
    bool enableParallelGC = true;
    int gcThreads = 0; // 0 = auto
    bool enableConcurrentGC = false;

    void adjustForArchitecture(TargetArch arch) {
        auto archInfo = TargetInfo::getArchitectureInfo(arch);
        if (!archInfo.is64Bit) {
            heapSize = std::min(heapSize, size_t(512 * 1024 * 1024)); // Cap at 512MB for 32-bit
            stackSize = std::min(stackSize, size_t(4 * 1024 * 1024));  // Cap at 4MB for 32-bit
        }
        if (arch == TargetArch::WASM32 || arch == TargetArch::WASM64) {
            enableParallelGC = false;
            enableConcurrentGC = false;
            gcThreads = 1;
        }
    }
};

struct OptimizationConfig {
    int level = 3; // 0=none, 1=basic, 2=aggressive, 3=maximum
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
    std::vector<std::string> supportedPasses; // List of valid optimization passes

    void configureForTarget(TargetArch arch) {
        auto features = TargetInfo::getAvailableFeatures(arch);
        if (std::find(features.begin(), features.end(), "sse") != features.end() ||
            std::find(features.begin(), features.end(), "neon") != features.end() ||
            std::find(features.begin(), features.end(), "simd128") != features.end()) {
            enableVectorization = true;
        }
        if (arch == TargetArch::ARM32 || arch == TargetArch::WASM32) {
            level = std::min(level, 2);
        }
        // Populate supported passes
        supportedPasses = {"dce", "inlining", "vectorize", "loop-unroll", "const-fold", "cse", "licm"};
    }
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
    int warningLevel = 2; // 0=none, 1=basic, 2=standard, 3=pedantic
    bool warningsAsErrors = false;
    std::vector<std::string> suppressedWarnings;
};

struct ModuleConfig {
    bool enableModules = false;
    std::vector<std::string> modulePaths;
    std::string moduleCachePath;
    bool exportModules = false; // Export as BMI (Binary Module Interface)
};

struct ProfilerConfig {
    ProfilerType type = ProfilerType::None;
    std::string outputPath;
    bool enableSampling = false;
    int samplingFrequency = 1000; // Samples per second
};

struct ErrorHandlingConfig {
    ErrorRecoveryMode mode = ErrorRecoveryMode::Continue;
    size_t maxErrorCount = 100; // Max errors before stopping
    bool logToFile = false;
    std::string errorLogPath;
};

struct IncrementalConfig {
    bool enabled = false;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> fileTimestamps;
    std::string dependencyCachePath;
};

struct Config {
    // === Core Configuration ===
    std::string filePath;
    std::string mainSourceFile; // Primary source file
    std::string outputPath = "a.out";
    std::string entry;
    CompileMode mode = CompileMode::JIT;

    // === Target Configuration ===
    std::vector<std::pair<TargetArch, TargetOS>> multiTargets; // Support multiple targets
    std::string targetTriple;
    std::string cpuFeatures = "native";
    std::string toolchainPath; // Custom toolchain (e.g., clang, lld)

    // === Source and Include Paths ===
    std::vector<std::string> includePaths;
    std::vector<std::string> sourcePaths;
    std::vector<std::string> importPaths;
    std::vector<std::string> precompiledHeaders;

    // === Preprocessor ===
    std::unordered_map<std::string, std::string> defines;
    std::vector<std::string> undefines;
    bool enablePreprocessorOutput = false;

    // === Mode-Specific Configuration ===
    JITConfig jit;
    AOTConfig aot;
    HybridConfig hybrid;
    ModuleConfig modules;
    IncrementalConfig incremental;

    // === Optimization and Performance ===
    OptimizationConfig optimization;
    RuntimeConfig runtime;
    SecurityConfig security;
    ProfilerConfig profiler;
    ErrorHandlingConfig errorHandling;

    // === Diagnostics and Debugging ===
    DiagnosticsConfig diagnostics;

    // === Build Configuration ===
    bool keepIntermediateFiles = false;
    std::string tempDirectory;
    int parallelJobs = 0;
    bool enableCaching = true;
    std::string cacheDirectory;

    // === Language Features ===
    std::string languageStandard = "latest";
    bool enableExperimentalFeatures = false;
    std::vector<std::string> enabledFeatures;
    std::vector<std::string> disabledFeatures;

    // === Plugin and Extension System ===
    struct PluginConfig {
        std::string name;
        std::string path;
        std::string version;
        std::unordered_map<std::string, std::string> options;
        bool (*callback)(const Config&, void*) = nullptr; // Plugin callback
    };
    std::vector<PluginConfig> plugins;

    // === Environment ===
    std::unordered_map<std::string, std::string> environmentVariables;
    std::string workingDirectory;

    // === Validation and Constraints ===
    size_t maxCompilationTime = 0; // Seconds, 0 = unlimited
    size_t maxMemoryUsage = 0; // Bytes, 0 = unlimited

    // === Utility Methods ===
    bool isJITMode() const { return mode == CompileMode::JIT || mode == CompileMode::Hybrid; }
    bool isAOTMode() const { return mode == CompileMode::AOT || mode == CompileMode::DryCompile; }
    bool isHybridMode() const { return mode == CompileMode::Hybrid; }
    bool isDryRun() const { return mode == CompileMode::DryCompile; }
    bool isCrossCompilation() const {
        if (multiTargets.empty()) {
            return TargetInfo::isCrossCompilation(resolveTargetArch(), resolveTargetOS());
        }
        return std::any_of(multiTargets.begin(), multiTargets.end(), [](const auto& pair) {
            return TargetInfo::isCrossCompilation(pair.first, pair.second);
        });
    }

    std::string getArchitectureName(TargetArch arch = TargetArch::Auto) const {
        return TargetInfo::getArchitectureName(arch == TargetArch::Auto ? resolveTargetArch() : arch);
    }

    std::string getOSName(TargetOS os = TargetOS::Auto) const {
        return TargetInfo::getOSName(os == TargetOS::Auto ? resolveTargetOS() : os);
    }

    bool isArchitecture64Bit(TargetArch arch = TargetArch::Auto) const {
        return TargetInfo::isArchitecture64Bit(arch == TargetArch::Auto ? resolveTargetArch() : arch);
    }

    int getPointerSize(TargetArch arch = TargetArch::Auto) const {
        return TargetInfo::getPointerSize(arch == TargetArch::Auto ? resolveTargetArch() : arch);
    }

    std::string getExecutableExtension(TargetOS os = TargetOS::Auto) const {
        return TargetInfo::getExecutableExtension(os == TargetOS::Auto ? resolveTargetOS() : os);
    }

    std::string getSharedLibExtension(TargetOS os = TargetOS::Auto) const {
        return TargetInfo::getSharedLibExtension(os == TargetOS::Auto ? resolveTargetOS() : os);
    }

    std::string getStaticLibExtension(TargetOS os = TargetOS::Auto) const {
        return TargetInfo::getStaticLibExtension(os == TargetOS::Auto ? resolveTargetOS() : os);
    }

    std::string getObjectFileExtension(TargetOS os = TargetOS::Auto) const {
        return TargetInfo::getObjectFileExtension(os == TargetOS::Auto ? resolveTargetOS() : os);
    }

    bool isUnixLikeOS(TargetOS os = TargetOS::Auto) const {
        return TargetInfo::isUnixLikeOS(os == TargetOS::Auto ? resolveTargetOS() : os);
    }

    std::vector<std::string> getAvailableFeatures(TargetArch arch = TargetArch::Auto) const {
        return TargetInfo::getAvailableFeatures(arch == TargetArch::Auto ? resolveTargetArch() : arch);
    }

    bool supportsFeature(const std::string& feature, TargetArch arch = TargetArch::Auto) const {
        return TargetInfo::supportsFeature(arch == TargetArch::Auto ? resolveTargetArch() : arch, feature);
    }

    std::string getDefaultCPU(TargetArch arch = TargetArch::Auto) const {
        return TargetInfo::getDefaultCPUForArch(arch == TargetArch::Auto ? resolveTargetArch() : arch);
    }

    std::string getEffectiveTargetTriple(TargetArch arch = TargetArch::Auto, TargetOS os = TargetOS::Auto) const {
        if (!targetTriple.empty()) {
            return targetTriple;
        }
        return TargetInfo::generateTriple(
            arch == TargetArch::Auto ? resolveTargetArch() : arch,
            os == TargetOS::Auto ? resolveTargetOS() : os);
    }

    std::string getTargetSummary(TargetArch arch = TargetArch::Auto, TargetOS os = TargetOS::Auto) const {
        return TargetInfo::getTargetSummary(
            arch == TargetArch::Auto ? resolveTargetArch() : arch,
            os == TargetOS::Auto ? resolveTargetOS() : os);
    }

    void autoConfigureForTarget() {
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

    bool validate(std::string& errorMessage) const {
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

    bool loadFromFile(const std::string& configPath) {
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

            // Load nested configs
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
            // Similarly load aot, hybrid, modules, incremental, optimization, runtime, security, profiler, errorHandling, diagnostics, plugins
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }

    bool saveToFile(const std::string& configPath) const {
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
            // Similarly serialize aot, hybrid, modules, incremental, optimization, runtime, security, profiler, errorHandling, diagnostics, plugins

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

    void mergeWith(const Config& other) {
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
        // Merge nested configs similarly
    }

    void printSummary() const {
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

    TargetArch resolveTargetArch() const {
        if (multiTargets.empty()) {
            return targetArch == TargetArch::Auto ? TargetInfo::detectHostArchitecture() : targetArch;
        }
        return multiTargets[0].first; // Use first target as default
    }

    TargetOS resolveTargetOS() const {
        if (multiTargets.empty()) {
            return targetOS == TargetOS::Auto ? TargetInfo::detectHostOS() : targetOS;
        }
        return multiTargets[0].second;
    }

    std::string getModeString() const {
        switch (mode) {
            case CompileMode::JIT: return "JIT";
            case CompileMode::AOT: return "AOT";
            case CompileMode::DryCompile: return "Dry Compile";
            case CompileMode::Hybrid: return "Hybrid";
            default: return "None";
        }
    }

private:
    TargetArch targetArch = TargetArch::Auto; // Legacy field for single-target
    TargetOS targetOS = TargetOS::Auto;       // Legacy field for single-target

    static bool isValidVersion(const std::string& version) {
        // Basic semantic versioning check (e.g., "1.2.3")
        std::regex versionRegex(R"(\d+\.\d+\.\d+)");
        return std::regex_match(version, versionRegex);
    }

    static std::string profilerTypeToString(ProfilerType type) {
        switch (type) {
            case ProfilerType::None: return "None";
            case ProfilerType::GProf: return "GProf";
            case ProfilerType::Perf: return "Perf";
            case ProfilerType::VTune: return "VTune";
            case ProfilerType::Custom: return "Custom";
            default: return "Unknown";
        }
    }
};

} // namespace Omniscript
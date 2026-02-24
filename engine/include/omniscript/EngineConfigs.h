#pragma once
#include <omniscript/TargetConfig.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <optional>
#include <vector>
#include <string>
#include <unordered_map>
#include <regex>
#include <vector>
#include <string>
#include <unordered_map>

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
    static std::string profilerTypeToString(ProfilerType type);
    static bool isValidVersion(const std::string& version);
};

} // namespace Omniscript
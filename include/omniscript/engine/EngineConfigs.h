#pragma once
#include <omniscript/omniscript_pch.h>
#include <omniscript/Core/Target_config.h>

enum class CompileMode {
    None,
    JIT,         // Execute with JIT backend
    AOT,         // Emit executable or object using AOT backend
    DryCompile,  // AOT backend but skip linking or execution
    Hybrid       // JIT with AOT fallback for hot code
};

enum class OutputFormat {
    Executable,  // Final executable
    StaticLib,   // Static library (.a/.lib)
    SharedLib,   // Shared library (.so/.dll/.dylib)
    ObjectFile,  // Object file (.o/.obj)
    Assembly,    // Assembly source
    LLVM_IR,     // LLVM intermediate representation
    Bitcode      // LLVM bitcode
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

struct AOTConfig {
    OutputFormat outputFormat = OutputFormat::Executable;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    std::vector<std::string> frameworkPaths; // macOS/iOS
    std::vector<std::string> frameworks;     // macOS/iOS
    std::string linkerScript;
    std::vector<std::string> linkerFlags;
    bool staticLinking = false;
    bool stripSymbols = false;
    bool generateDebugInfo = true;
    std::string debugInfoFormat = "dwarf"; // dwarf, codeview, etc.
    LinkTimeOptimization lto;
    
    // Auto-populate paths based on target
    void populateDefaultPaths(TargetArch arch, TargetOS os) {
        if (libraryPaths.empty()) {
            libraryPaths = TargetInfo::getDefaultLibraryPaths(os, arch);
        }
        
        // Add required system libraries
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
    
    // Auto-configure based on target OS
    void configureForTarget(TargetOS os) {
        auto osInfo = TargetInfo::getOSInfo(os);
        enablePositionIndependentCode = osInfo.supportsPIC;
        
        // Disable some features for embedded/WASM targets
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
    SafetyLevel safetyLevel = SafetyLevel::Standard;
    bool enableParallelGC = true;
    int gcThreads = 0; // 0 = auto
    bool enableConcurrentGC = false;
    
    // Adjust runtime parameters based on architecture
    void adjustForArchitecture(TargetArch arch) {
        auto archInfo = TargetInfo::getArchitectureInfo(arch);
        
        // Adjust heap/stack sizes for 32-bit architectures
        if (!archInfo.is64Bit) {
            heapSize = std::min(heapSize, size_t(512 * 1024 * 1024)); // Cap at 512MB for 32-bit
            stackSize = std::min(stackSize, size_t(4 * 1024 * 1024));  // Cap at 4MB for 32-bit
        }
        
        // Disable parallel GC for single-threaded targets
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
    bool fastMath = false; // Allow mathematically unsafe optimizations
    ProfileGuidedOptimization pgo;
    std::unordered_map<std::string, std::string> customPasses;
    
    // Configure optimizations based on target architecture features
    void configureForTarget(TargetArch arch) {
        auto features = TargetInfo::getAvailableFeatures(arch);
        
        // Enable vectorization if SIMD features are available
        if (std::find(features.begin(), features.end(), "sse") != features.end() ||
            std::find(features.begin(), features.end(), "neon") != features.end() ||
            std::find(features.begin(), features.end(), "simd128") != features.end()) {
            enableVectorization = true;
        }
        
        // Adjust optimization level for embedded targets
        if (arch == TargetArch::ARM32 || arch == TargetArch::WASM32) {
            level = std::min(level, 2); // Cap optimization for smaller targets
        }
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

struct Config {
    // === Core Configuration ===
    std::string filePath;
    std::string outputPath = "a.out";
    std::string entry = "main"; // Function to call when starting
    CompileMode mode = CompileMode::JIT;
    
    // === Target Configuration (using TargetInfo enums) ===
    TargetArch targetArch = TargetArch::Auto;
    TargetOS targetOS = TargetOS::Auto;
    std::string targetTriple; // Override for custom targets
    std::string cpuFeatures = "native"; // CPU-specific features
    
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
    
    // === Optimization and Performance ===
    OptimizationConfig optimization;
    RuntimeConfig runtime;
    SecurityConfig security;
    
    // === Diagnostics and Debugging ===
    DiagnosticsConfig diagnostics;
    
    // === Build Configuration ===
    bool keepIntermediateFiles = false;
    std::string tempDirectory;
    int parallelJobs = 0; // 0 = auto-detect
    bool enableCaching = true;
    std::string cacheDirectory;
    
    // === Language Features ===
    std::string languageStandard = "latest";
    bool enableExperimentalFeatures = false;
    std::vector<std::string> enabledFeatures;
    std::vector<std::string> disabledFeatures;
    
    // === Plugin and Extension System ===
    std::vector<std::string> plugins;
    std::vector<std::string> pluginPaths;
    std::unordered_map<std::string, std::string> pluginOptions;
    
    // === Environment ===
    std::unordered_map<std::string, std::string> environmentVariables;
    std::string workingDirectory;
    
    // === Validation and Constraints ===
    size_t maxCompilationTime = 0; // 0 = unlimited (seconds)
    size_t maxMemoryUsage = 0; // 0 = unlimited (bytes)
    
    // === Utility Methods ===
    bool isJITMode() const { return mode == CompileMode::JIT || mode == CompileMode::Hybrid; }
    bool isAOTMode() const { return mode == CompileMode::AOT || mode == CompileMode::DryCompile; }
    bool isHybridMode() const { return mode == CompileMode::Hybrid; }
    bool isDryRun() const { return mode == CompileMode::DryCompile; }
    bool isCrossCompilation() const { return TargetInfo::isCrossCompilation(targetArch, targetOS); }
    
    // === Target Information Methods (using TargetInfo) ===
    std::string getArchitectureName() const {
        return TargetInfo::getArchitectureName(resolveTargetArch());
    }
    
    std::string getOSName() const {
        return TargetInfo::getOSName(resolveTargetOS());
    }
    
    bool isArchitecture64Bit() const {
        return TargetInfo::isArchitecture64Bit(resolveTargetArch());
    }
    
    int getPointerSize() const {
        return TargetInfo::getPointerSize(resolveTargetArch());
    }
    
    std::string getExecutableExtension() const {
        return TargetInfo::getExecutableExtension(resolveTargetOS());
    }
    
    std::string getSharedLibExtension() const {
        return TargetInfo::getSharedLibExtension(resolveTargetOS());
    }
    
    std::string getStaticLibExtension() const {
        return TargetInfo::getStaticLibExtension(resolveTargetOS());
    }
    
    std::string getObjectFileExtension() const {
        return TargetInfo::getObjectFileExtension(resolveTargetOS());
    }
    
    bool isUnixLikeOS() const {
        return TargetInfo::isUnixLikeOS(resolveTargetOS());
    }
    
    std::vector<std::string> getAvailableFeatures() const {
        return TargetInfo::getAvailableFeatures(resolveTargetArch());
    }
    
    bool supportsFeature(const std::string& feature) const {
        return TargetInfo::supportsFeature(resolveTargetArch(), feature);
    }
    
    std::string getDefaultCPU() const {
        return TargetInfo::getDefaultCPUForArch(resolveTargetArch());
    }
    
    // Get effective target triple (using TargetInfo)
    std::string getEffectiveTargetTriple() const {
        if (!targetTriple.empty()) {
            return targetTriple;
        }
        return TargetInfo::generateTriple(resolveTargetArch(), resolveTargetOS());
    }
    
    // Get target summary
    std::string getTargetSummary() const {
        return TargetInfo::getTargetSummary(resolveTargetArch(), resolveTargetOS());
    }
    
    // Auto-configure all subsystems based on target
    void autoConfigureForTarget() {
        auto arch = resolveTargetArch();
        auto os = resolveTargetOS();
        
        // Configure subsystems
        aot.populateDefaultPaths(arch, os);
        security.configureForTarget(os);
        runtime.adjustForArchitecture(arch);
        optimization.configureForTarget(arch);
        
        // Set default output path with appropriate extension
        if (outputPath == "a.out") {
            outputPath = "a" + getExecutableExtension();
        }
        
        // Auto-populate include paths if empty
        if (includePaths.empty()) {
            includePaths = TargetInfo::getDefaultIncludePaths(os, arch);
        }
        
        // Set CPU features if still default
        if (cpuFeatures == "native" && isCrossCompilation()) {
            cpuFeatures = getDefaultCPU();
        }
    }
    
    // Validate configuration consistency
    bool validate(std::string& errorMessage) const {
        auto arch = resolveTargetArch();
        auto os = resolveTargetOS();
        
        // Check if target triple is valid if provided
        if (!targetTriple.empty() && !TargetInfo::isValidTriple(targetTriple)) {
            errorMessage = "Invalid target triple: " + targetTriple;
            return false;
        }
        
        // Validate cross-compilation setup
        if (isCrossCompilation() && isJITMode()) {
            errorMessage = "JIT mode is not supported for cross-compilation";
            return false;
        }
        
        // Check architecture compatibility for hybrid mode
        if (isHybridMode() && isCrossCompilation()) {
            if (!TargetInfo::isArchitectureCompatible(TargetInfo::detectHostArchitecture(), arch)) {
                errorMessage = "Hybrid mode requires compatible target architecture";
                return false;
            }
        }
        
        // Validate feature requests
        auto availableFeatures = getAvailableFeatures();
        for (const auto& feature : enabledFeatures) {
            if (!supportsFeature(feature)) {
                errorMessage = "Feature '" + feature + "' is not supported on " + getArchitectureName();
                return false;
            }
        }
        
        // Check output format compatibility
        if (aot.outputFormat == OutputFormat::SharedLib && aot.staticLinking) {
            errorMessage = "Cannot create shared library with static linking enabled";
            return false;
        }
        
        return true;
    }
    
    // Load/save configuration from/to file
    bool loadFromFile(const std::string& configPath);
    bool saveToFile(const std::string& configPath) const;
    
    // Merge with another configuration (useful for inheritance)
    void mergeWith(const Config& other);
    
    // Print configuration summary
    void printSummary() const {
        printf("Configuration Summary:\n");
        printf("  Mode: %s\n", getModeString().c_str());
        printf("  Target: %s\n", getTargetSummary().c_str());
        printf("  Cross-compilation: %s\n", isCrossCompilation() ? "Yes" : "No");
        printf("  Output: %s\n", outputPath.c_str());
        if (isCrossCompilation()) {
            printf("  Host: %s\n", TargetInfo::detectHostTriple().c_str());
        }
    }

    // Resolve Auto targets to actual values
    TargetArch resolveTargetArch() const {
        return (targetArch == TargetArch::Auto) ? TargetInfo::detectHostArchitecture() : targetArch;
    }
    
    TargetOS resolveTargetOS() const {
        return (targetOS == TargetOS::Auto) ? TargetInfo::detectHostOS() : targetOS;
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
    
};
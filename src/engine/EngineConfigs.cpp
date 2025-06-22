#include <omniscript/engine/EngineConfigs.h>

namespace fs = std::filesystem;

// Helper function to trim whitespace
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

// Helper function to split string
static std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        token = trim(token);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// Helper function to parse key-value pairs
static std::pair<std::string, std::string> parseKeyValue(const std::string& line) {
    size_t pos = line.find('=');
    if (pos == std::string::npos) {
        return {trim(line), ""};
    }
    return {trim(line.substr(0, pos)), trim(line.substr(pos + 1))};
}

// Helper function to parse boolean
static bool parseBool(const std::string& value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
}

bool Config::loadFromFile(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string currentSection;
    
    while (std::getline(file, line)) {
        line = trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Handle sections
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
        auto [key, value] = parseKeyValue(line);
        if (key.empty()) continue;
        
        try {
            // Core configuration
            if (currentSection.empty() || currentSection == "core") {
                if (key == "filePath") filePath = value;
                else if (key == "outputPath") outputPath = value;
                else if (key == "entry") entry = value;
                else if (key == "mode") {
                    if (value == "JIT") mode = CompileMode::JIT;
                    else if (value == "AOT") mode = CompileMode::AOT;
                    else if (value == "DryCompile") mode = CompileMode::DryCompile;
                    else if (value == "Hybrid") mode = CompileMode::Hybrid;
                }
                else if (key == "targetArch") {
                    if (value == "X86_64") targetArch = TargetArch::X86_64;
                    else if (value == "ARM64") targetArch = TargetArch::ARM64;
                    else if (value == "X86_32") targetArch = TargetArch::X86_32;
                    else if (value == "ARM32") targetArch = TargetArch::ARM32;
                    else if (value == "RISCV64") targetArch = TargetArch::RISCV64;
                    else if (value == "WASM32") targetArch = TargetArch::WASM32;
                    else if (value == "WASM64") targetArch = TargetArch::WASM64;
                    else if (value == "Auto") targetArch = TargetArch::Auto;
                }
                else if (key == "targetOS") {
                    if (value == "Linux") targetOS = TargetOS::Linux;
                    else if (value == "Windows") targetOS = TargetOS::Windows;
                    else if (value == "MacOS") targetOS = TargetOS::MacOS;
                    else if (value == "FreeBSD") targetOS = TargetOS::FreeBSD;
                    else if (value == "Android") targetOS = TargetOS::Android;
                    else if (value == "iOS") targetOS = TargetOS::iOS;
                    else if (value == "WebAssembly") targetOS = TargetOS::WebAssembly;
                    else if (value == "Auto") targetOS = TargetOS::Auto;
                }
                else if (key == "targetTriple") targetTriple = value;
                else if (key == "cpuFeatures") cpuFeatures = value;
                else if (key == "languageStandard") languageStandard = value;
                else if (key == "enableExperimentalFeatures") enableExperimentalFeatures = parseBool(value);
                else if (key == "keepIntermediateFiles") keepIntermediateFiles = parseBool(value);
                else if (key == "tempDirectory") tempDirectory = value;
                else if (key == "parallelJobs") parallelJobs = std::stoi(value);
                else if (key == "enableCaching") enableCaching = parseBool(value);
                else if (key == "cacheDirectory") cacheDirectory = value;
                else if (key == "workingDirectory") workingDirectory = value;
                else if (key == "maxCompilationTime") maxCompilationTime = std::stoull(value);
                else if (key == "maxMemoryUsage") maxMemoryUsage = std::stoull(value);
            }
            
            // JIT configuration
            else if (currentSection == "jit") {
                if (key == "engine") {
                    if (value == "LLVM_ORC") jit.engine = JITEngine::LLVM_ORC;
                    else if (value == "LLVM_MCJIT") jit.engine = JITEngine::LLVM_MCJIT;
                    else if (value == "Custom") jit.engine = JITEngine::Custom;
                }
                else if (key == "lazyCompilation") jit.lazyCompilation = parseBool(value);
                else if (key == "enableSpeculation") jit.enableSpeculation = parseBool(value);
                else if (key == "codeGenerationThreads") jit.codeGenerationThreads = std::stoul(value);
                else if (key == "compilationThreshold") jit.compilationThreshold = std::stoul(value);
                else if (key == "enableTieredCompilation") jit.enableTieredCompilation = parseBool(value);
                else if (key == "maxCodeCacheSize") jit.maxCodeCacheSize = std::stoul(value);
                else if (key == "enableInlining") jit.enableInlining = parseBool(value);
                else if (key == "enableDebugging") jit.enableDebugging = parseBool(value);
            }
            
            // AOT configuration
            else if (currentSection == "aot") {
                if (key == "outputFormat") {
                    if (value == "Executable") aot.outputFormat = OutputFormat::Executable;
                    else if (value == "StaticLib") aot.outputFormat = OutputFormat::StaticLib;
                    else if (value == "SharedLib") aot.outputFormat = OutputFormat::SharedLib;
                    else if (value == "ObjectFile") aot.outputFormat = OutputFormat::ObjectFile;
                    else if (value == "Assembly") aot.outputFormat = OutputFormat::Assembly;
                    else if (value == "LLVM_IR") aot.outputFormat = OutputFormat::LLVM_IR;
                    else if (value == "Bitcode") aot.outputFormat = OutputFormat::Bitcode;
                }
                else if (key == "libraryPaths") aot.libraryPaths = split(value, ';');
                else if (key == "libraries") aot.libraries = split(value, ';');
                else if (key == "frameworkPaths") aot.frameworkPaths = split(value, ';');
                else if (key == "frameworks") aot.frameworks = split(value, ';');
                else if (key == "linkerScript") aot.linkerScript = value;
                else if (key == "linkerFlags") aot.linkerFlags = split(value, ';');
                else if (key == "staticLinking") aot.staticLinking = parseBool(value);
                else if (key == "stripSymbols") aot.stripSymbols = parseBool(value);
                else if (key == "generateDebugInfo") aot.generateDebugInfo = parseBool(value);
                else if (key == "debugInfoFormat") aot.debugInfoFormat = value;
            }
            
            // Optimization configuration
            else if (currentSection == "optimization") {
                if (key == "level") optimization.level = std::stoi(value);
                else if (key == "enableVectorization") optimization.enableVectorization = parseBool(value);
                else if (key == "enableLoopUnrolling") optimization.enableLoopUnrolling = parseBool(value);
                else if (key == "enableFunctionInlining") optimization.enableFunctionInlining = parseBool(value);
                else if (key == "enableTailCallOptimization") optimization.enableTailCallOptimization = parseBool(value);
                else if (key == "enableDeadCodeElimination") optimization.enableDeadCodeElimination = parseBool(value);
                else if (key == "enableConstantFolding") optimization.enableConstantFolding = parseBool(value);
                else if (key == "enableCommonSubexpressionElimination") optimization.enableCommonSubexpressionElimination = parseBool(value);
                else if (key == "enableLoopInvariantCodeMotion") optimization.enableLoopInvariantCodeMotion = parseBool(value);
                else if (key == "fastMath") optimization.fastMath = parseBool(value);
            }
            
            // Runtime configuration
            else if (currentSection == "runtime") {
                if (key == "gcStrategy") {
                    if (value == "None") runtime.gcStrategy = GCStrategy::None;
                    else if (value == "RefCounting") runtime.gcStrategy = GCStrategy::RefCounting;
                    else if (value == "MarkSweep") runtime.gcStrategy = GCStrategy::MarkSweep;
                    else if (value == "Generational") runtime.gcStrategy = GCStrategy::Generational;
                    else if (value == "Incremental") runtime.gcStrategy = GCStrategy::Incremental;
                }
                else if (key == "heapSize") runtime.heapSize = std::stoul(value);
                else if (key == "stackSize") runtime.stackSize = std::stoul(value);
                else if (key == "safetyLevel") {
                    if (value == "Unsafe") runtime.safetyLevel = SafetyLevel::Unsafe;
                    else if (value == "Minimal") runtime.safetyLevel = SafetyLevel::Minimal;
                    else if (value == "Standard") runtime.safetyLevel = SafetyLevel::Standard;
                    else if (value == "Paranoid") runtime.safetyLevel = SafetyLevel::Paranoid;
                }
                else if (key == "enableParallelGC") runtime.enableParallelGC = parseBool(value);
                else if (key == "gcThreads") runtime.gcThreads = std::stoi(value);
                else if (key == "enableConcurrentGC") runtime.enableConcurrentGC = parseBool(value);
            }
            
            // Security configuration
            else if (currentSection == "security") {
                if (key == "enableStackProtection") security.enableStackProtection = parseBool(value);
                else if (key == "enableControlFlowIntegrity") security.enableControlFlowIntegrity = parseBool(value);
                else if (key == "enableAddressSanitizer") security.enableAddressSanitizer = parseBool(value);
                else if (key == "enableMemorySanitizer") security.enableMemorySanitizer = parseBool(value);
                else if (key == "enableThreadSanitizer") security.enableThreadSanitizer = parseBool(value);
                else if (key == "enableUndefinedBehaviorSanitizer") security.enableUndefinedBehaviorSanitizer = parseBool(value);
                else if (key == "enablePositionIndependentCode") security.enablePositionIndependentCode = parseBool(value);
                else if (key == "enableDataExecutionPrevention") security.enableDataExecutionPrevention = parseBool(value);
                else if (key == "enableAddressSpaceLayoutRandomization") security.enableAddressSpaceLayoutRandomization = parseBool(value);
            }
            
            // Diagnostics configuration
            else if (currentSection == "diagnostics") {
                if (key == "debugMode") diagnostics.debugMode = parseBool(value);
                else if (key == "verbose") diagnostics.verbose = parseBool(value);
                else if (key == "logFinalCode") diagnostics.logFinalCode = parseBool(value);
                else if (key == "logAsm") diagnostics.logAsm = parseBool(value);
                else if (key == "showMetadata") diagnostics.showMetadata = parseBool(value);
                else if (key == "logOptimizationRemarks") diagnostics.logOptimizationRemarks = parseBool(value);
                else if (key == "logTimings") diagnostics.logTimings = parseBool(value);
                else if (key == "generateReports") diagnostics.generateReports = parseBool(value);
                else if (key == "reportOutputPath") diagnostics.reportOutputPath = value;
                else if (key == "enableProfiling") diagnostics.enableProfiling = parseBool(value);
                else if (key == "measureMemoryUsage") diagnostics.measureMemoryUsage = parseBool(value);
                else if (key == "warningLevel") diagnostics.warningLevel = std::stoi(value);
                else if (key == "warningsAsErrors") diagnostics.warningsAsErrors = parseBool(value);
                else if (key == "suppressedWarnings") diagnostics.suppressedWarnings = split(value, ';');
            }
            
            // Include paths
            else if (currentSection == "includes") {
                if (key == "paths") includePaths = split(value, ';');
            }
            
            // Source paths
            else if (currentSection == "sources") {
                if (key == "paths") sourcePaths = split(value, ';');
            }
            
            // Import paths
            else if (currentSection == "imports") {
                if (key == "paths") importPaths = split(value, ';');
            }
            
            // Preprocessor defines
            else if (currentSection == "defines") {
                defines[key] = value;
            }
            
            // Environment variables
            else if (currentSection == "environment") {
                environmentVariables[key] = value;
            }
            
            // Plugin options
            else if (currentSection == "plugins") {
                if (key == "paths") pluginPaths = split(value, ';');
                else if (key == "load") plugins = split(value, ';');
                else pluginOptions[key] = value;
            }
        }
        catch (const std::exception& e) {
            // Skip invalid values
            continue;
        }
    }
    
    return true;
}

bool Config::saveToFile(const std::string& configPath) const {
    std::ofstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# OmniScript Compiler Configuration\n\n";
    
    // Core configuration
    file << "[core]\n";
    file << "filePath=" << filePath << "\n";
    file << "outputPath=" << outputPath << "\n";
    file << "entry=" << entry << "\n";
    
    file << "mode=";
    switch (mode) {
        case CompileMode::JIT: file << "JIT"; break;
        case CompileMode::AOT: file << "AOT"; break;
        case CompileMode::DryCompile: file << "DryCompile"; break;
        case CompileMode::Hybrid: file << "Hybrid"; break;
        default: file << "None"; break;
    }
    file << "\n";
    
    file << "targetArch=";
    switch (targetArch) {
        case TargetArch::X86_64: file << "X86_64"; break;
        case TargetArch::ARM64: file << "ARM64"; break;
        case TargetArch::X86_32: file << "X86_32"; break;
        case TargetArch::ARM32: file << "ARM32"; break;
        case TargetArch::RISCV64: file << "RISCV64"; break;
        case TargetArch::WASM32: file << "WASM32"; break;
        case TargetArch::WASM64: file << "WASM64"; break;
        default: file << "Auto"; break;
    }
    file << "\n";
    
    file << "targetOS=";
    switch (targetOS) {
        case TargetOS::Linux: file << "Linux"; break;
        case TargetOS::Windows: file << "Windows"; break;
        case TargetOS::MacOS: file << "MacOS"; break;
        case TargetOS::FreeBSD: file << "FreeBSD"; break;
        case TargetOS::Android: file << "Android"; break;
        case TargetOS::iOS: file << "iOS"; break;
        case TargetOS::WebAssembly: file << "WebAssembly"; break;
        default: file << "Auto"; break;
    }
    file << "\n";
    
    if (!targetTriple.empty()) file << "targetTriple=" << targetTriple << "\n";
    file << "cpuFeatures=" << cpuFeatures << "\n";
    file << "languageStandard=" << languageStandard << "\n";
    file << "enableExperimentalFeatures=" << (enableExperimentalFeatures ? "true" : "false") << "\n";
    file << "keepIntermediateFiles=" << (keepIntermediateFiles ? "true" : "false") << "\n";
    if (!tempDirectory.empty()) file << "tempDirectory=" << tempDirectory << "\n";
    file << "parallelJobs=" << parallelJobs << "\n";
    file << "enableCaching=" << (enableCaching ? "true" : "false") << "\n";
    if (!cacheDirectory.empty()) file << "cacheDirectory=" << cacheDirectory << "\n";
    if (!workingDirectory.empty()) file << "workingDirectory=" << workingDirectory << "\n";
    if (maxCompilationTime > 0) file << "maxCompilationTime=" << maxCompilationTime << "\n";
    if (maxMemoryUsage > 0) file << "maxMemoryUsage=" << maxMemoryUsage << "\n";
    
    // JIT configuration
    file << "\n[jit]\n";
    file << "engine=";
    switch (jit.engine) {
        case JITEngine::LLVM_ORC: file << "LLVM_ORC"; break;
        case JITEngine::LLVM_MCJIT: file << "LLVM_MCJIT"; break;
        case JITEngine::Custom: file << "Custom"; break;
    }
    file << "\n";
    file << "lazyCompilation=" << (jit.lazyCompilation ? "true" : "false") << "\n";
    file << "enableSpeculation=" << (jit.enableSpeculation ? "true" : "false") << "\n";
    file << "codeGenerationThreads=" << jit.codeGenerationThreads << "\n";
    file << "compilationThreshold=" << jit.compilationThreshold << "\n";
    file << "enableTieredCompilation=" << (jit.enableTieredCompilation ? "true" : "false") << "\n";
    file << "maxCodeCacheSize=" << jit.maxCodeCacheSize << "\n";
    file << "enableInlining=" << (jit.enableInlining ? "true" : "false") << "\n";
    file << "enableDebugging=" << (jit.enableDebugging ? "true" : "false") << "\n";
    
    // AOT configuration
    file << "\n[aot]\n";
    file << "outputFormat=";
    switch (aot.outputFormat) {
        case OutputFormat::Executable: file << "Executable"; break;
        case OutputFormat::StaticLib: file << "StaticLib"; break;
        case OutputFormat::SharedLib: file << "SharedLib"; break;
        case OutputFormat::ObjectFile: file << "ObjectFile"; break;
        case OutputFormat::Assembly: file << "Assembly"; break;
        case OutputFormat::LLVM_IR: file << "LLVM_IR"; break;
        case OutputFormat::Bitcode: file << "Bitcode"; break;
    }
    file << "\n";
    
    if (!aot.libraryPaths.empty()) {
        file << "libraryPaths=";
        for (size_t i = 0; i < aot.libraryPaths.size(); ++i) {
            if (i > 0) file << ";";
            file << aot.libraryPaths[i];
        }
        file << "\n";
    }
    
    if (!aot.libraries.empty()) {
        file << "libraries=";
        for (size_t i = 0; i < aot.libraries.size(); ++i) {
            if (i > 0) file << ";";
            file << aot.libraries[i];
        }
        file << "\n";
    }
    
    file << "staticLinking=" << (aot.staticLinking ? "true" : "false") << "\n";
    file << "stripSymbols=" << (aot.stripSymbols ? "true" : "false") << "\n";
    file << "generateDebugInfo=" << (aot.generateDebugInfo ? "true" : "false") << "\n";
    file << "debugInfoFormat=" << aot.debugInfoFormat << "\n";
    
    // Optimization configuration
    file << "\n[optimization]\n";
    file << "level=" << optimization.level << "\n";
    file << "enableVectorization=" << (optimization.enableVectorization ? "true" : "false") << "\n";
    file << "enableLoopUnrolling=" << (optimization.enableLoopUnrolling ? "true" : "false") << "\n";
    file << "enableFunctionInlining=" << (optimization.enableFunctionInlining ? "true" : "false") << "\n";
    file << "enableTailCallOptimization=" << (optimization.enableTailCallOptimization ? "true" : "false") << "\n";
    file << "enableDeadCodeElimination=" << (optimization.enableDeadCodeElimination ? "true" : "false") << "\n";
    file << "enableConstantFolding=" << (optimization.enableConstantFolding ? "true" : "false") << "\n";
    file << "enableCommonSubexpressionElimination=" << (optimization.enableCommonSubexpressionElimination ? "true" : "false") << "\n";
    file << "enableLoopInvariantCodeMotion=" << (optimization.enableLoopInvariantCodeMotion ? "true" : "false") << "\n";
    file << "fastMath=" << (optimization.fastMath ? "true" : "false") << "\n";
    
    // Runtime configuration
    file << "\n[runtime]\n";
    file << "gcStrategy=";
    switch (runtime.gcStrategy) {
        case GCStrategy::None: file << "None"; break;
        case GCStrategy::RefCounting: file << "RefCounting"; break;
        case GCStrategy::MarkSweep: file << "MarkSweep"; break;
        case GCStrategy::Generational: file << "Generational"; break;
        case GCStrategy::Incremental: file << "Incremental"; break;
    }
    file << "\n";
    file << "heapSize=" << runtime.heapSize << "\n";
    file << "stackSize=" << runtime.stackSize << "\n";
    file << "safetyLevel=";
    switch (runtime.safetyLevel) {
        case SafetyLevel::Unsafe: file << "Unsafe"; break;
        case SafetyLevel::Minimal: file << "Minimal"; break;
        case SafetyLevel::Standard: file << "Standard"; break;
        case SafetyLevel::Paranoid: file << "Paranoid"; break;
    }
    file << "\n";
    file << "enableParallelGC=" << (runtime.enableParallelGC ? "true" : "false") << "\n";
    file << "gcThreads=" << runtime.gcThreads << "\n";
    file << "enableConcurrentGC=" << (runtime.enableConcurrentGC ? "true" : "false") << "\n";
    
    // Security configuration
    file << "\n[security]\n";
    file << "enableStackProtection=" << (security.enableStackProtection ? "true" : "false") << "\n";
    file << "enableControlFlowIntegrity=" << (security.enableControlFlowIntegrity ? "true" : "false") << "\n";
    file << "enableAddressSanitizer=" << (security.enableAddressSanitizer ? "true" : "false") << "\n";
    file << "enableMemorySanitizer=" << (security.enableMemorySanitizer ? "true" : "false") << "\n";
    file << "enableThreadSanitizer=" << (security.enableThreadSanitizer ? "true" : "false") << "\n";
    file << "enableUndefinedBehaviorSanitizer=" << (security.enableUndefinedBehaviorSanitizer ? "true" : "false") << "\n";
    file << "enablePositionIndependentCode=" << (security.enablePositionIndependentCode ? "true" : "false") << "\n";
    file << "enableDataExecutionPrevention=" << (security.enableDataExecutionPrevention ? "true" : "false") << "\n";
    file << "enableAddressSpaceLayoutRandomization=" << (security.enableAddressSpaceLayoutRandomization ? "true" : "false") << "\n";
    
    // Diagnostics configuration
    file << "\n[diagnostics]\n";
    file << "debugMode=" << (diagnostics.debugMode ? "true" : "false") << "\n";
    file << "verbose=" << (diagnostics.verbose ? "true" : "false") << "\n";
    file << "logFinalCode=" << (diagnostics.logFinalCode ? "true" : "false") << "\n";
    file << "logAsm=" << (diagnostics.logAsm ? "true" : "false") << "\n";
    file << "showMetadata=" << (diagnostics.showMetadata ? "true" : "false") << "\n";
    file << "logOptimizationRemarks=" << (diagnostics.logOptimizationRemarks ? "true" : "false") << "\n";
    file << "logTimings=" << (diagnostics.logTimings ? "true" : "false") << "\n";
    file << "generateReports=" << (diagnostics.generateReports ? "true" : "false") << "\n";
    if (!diagnostics.reportOutputPath.empty()) file << "reportOutputPath=" << diagnostics.reportOutputPath << "\n";
    file << "enableProfiling=" << (diagnostics.enableProfiling ? "true" : "false") << "\n";
    file << "measureMemoryUsage=" << (diagnostics.measureMemoryUsage ? "true" : "false") << "\n";
    file << "warningLevel=" << diagnostics.warningLevel << "\n";
    file << "warningsAsErrors=" << (diagnostics.warningsAsErrors ? "true" : "false") << "\n";
    
    if (!diagnostics.suppressedWarnings.empty()) {
        file << "suppressedWarnings=";
        for (size_t i = 0; i < diagnostics.suppressedWarnings.size(); ++i) {
            if (i > 0) file << ";";
            file << diagnostics.suppressedWarnings[i];
        }
        file << "\n";
    }
    
    // Include paths
    if (!includePaths.empty()) {
        file << "\n[includes]\n";
        file << "paths=";
        for (size_t i = 0; i < includePaths.size(); ++i) {
            if (i > 0) file << ";";
            file << includePaths[i];
        }
        file << "\n";
    }
    
    // Source paths
    if (!sourcePaths.empty()) {
        file << "\n[sources]\n";
        file << "paths=";
        for (size_t i = 0; i < sourcePaths.size(); ++i) {
            if (i > 0) file << ";";
            file << sourcePaths[i];
        }
        file << "\n";
    }
    
    // Import paths
    if (!importPaths.empty()) {
        file << "\n[imports]\n";
        file << "paths=";
        for (size_t i = 0; i < importPaths.size(); ++i) {
            if (i > 0) file << ";";
            file << importPaths[i];
        }
        file << "\n";
    }
    
    // Preprocessor defines
    if (!defines.empty()) {
        file << "\n[defines]\n";
        for (const auto& [key, value] : defines) {
            file << key << "=" << value << "\n";
        }
    }
    
    // Environment variables
    if (!environmentVariables.empty()) {
        file << "\n[environment]\n";
        for (const auto& [key, value] : environmentVariables) {
            file << key << "=" << value << "\n";
        }
    }
    
    // Plugin configuration
    if (!plugins.empty() || !pluginPaths.empty() || !pluginOptions.empty()) {
        file << "\n[plugins]\n";
        
        if (!pluginPaths.empty()) {
            file << "paths=";
            for (size_t i = 0; i < pluginPaths.size(); ++i) {
                if (i > 0) file << ";";
                file << pluginPaths[i];
            }
            file << "\n";
        }
        
        if (!plugins.empty()) {
            file << "load=";
            for (size_t i = 0; i < plugins.size(); ++i) {
                if (i > 0) file << ";";
                file << plugins[i];
            }
            file << "\n";
        }
        
        for (const auto& [key, value] : pluginOptions) {
            file << key << "=" << value << "\n";
        }
    }
    
    return true;
}

void Config::mergeWith(const Config& other) {
    // Merge core configuration (don't override if already set)
    if (filePath.empty() && !other.filePath.empty()) filePath = other.filePath;
    if (outputPath == "a.out" && other.outputPath != "a.out") outputPath = other.outputPath;
    if (entry == "main" && other.entry != "main") entry = other.entry;
    if (mode == CompileMode::JIT && other.mode != CompileMode::JIT) mode = other.mode;
    if (targetArch == TargetArch::Auto && other.targetArch != TargetArch::Auto) targetArch = other.targetArch;
    if (targetOS == TargetOS::Auto && other.targetOS != TargetOS::Auto) targetOS = other.targetOS;
    if (targetTriple.empty() && !other.targetTriple.empty()) targetTriple = other.targetTriple;
    if (cpuFeatures == "native" && other.cpuFeatures != "native") cpuFeatures = other.cpuFeatures;
    
    // Merge paths (append unique entries)
    for (const auto& path : other.includePaths) {
        if (std::find(includePaths.begin(), includePaths.end(), path) == includePaths.end()) {
            includePaths.push_back(path);
        }
    }
    
    for (const auto& path : other.sourcePaths) {
        if (std::find(sourcePaths.begin(), sourcePaths.end(), path) == sourcePaths.end()) {
            sourcePaths.push_back(path);
        }
    }
    
    for (const auto& path : other.importPaths) {
        if (std::find(importPaths.begin(), importPaths.end(), path) == importPaths.end()) {
            importPaths.push_back(path);
        }
    }
    
    for (const auto& path : other.pluginPaths) {
        if (std::find(pluginPaths.begin(), pluginPaths.end(), path) == pluginPaths.end()) {
            pluginPaths.push_back(path);
        }
    }
    
    for (const auto& plugin : other.plugins) {
        if (std::find(plugins.begin(), plugins.end(), plugin) == plugins.end()) {
            plugins.push_back(plugin);
        }
    }
    
    // Merge defines (other takes precedence)
    for (const auto& [key, value] : other.defines) {
        defines[key] = value;
    }
    
    // Merge environment variables (other takes precedence)
    for (const auto& [key, value] : other.environmentVariables) {
        environmentVariables[key] = value;
    }
    
    // Merge plugin options (other takes precedence)
    for (const auto& [key, value] : other.pluginOptions) {
        pluginOptions[key] = value;
    }
    
    // For AOT libraries, append unique entries
    for (const auto& lib : other.aot.libraries) {
        if (std::find(aot.libraries.begin(), aot.libraries.end(), lib) == aot.libraries.end()) {
            aot.libraries.push_back(lib);
        }
    }
    
    for (const auto& path : other.aot.libraryPaths) {
        if (std::find(aot.libraryPaths.begin(), aot.libraryPaths.end(), path) == aot.libraryPaths.end()) {
            aot.libraryPaths.push_back(path);
        }
    }
    
    // For flags, append all from other
    aot.linkerFlags.insert(aot.linkerFlags.end(), other.aot.linkerFlags.begin(), other.aot.linkerFlags.end());
    
    // Override scalar values if they differ from defaults
    if (other.optimization.level != 3) optimization.level = other.optimization.level;
    if (other.runtime.heapSize != 64 * 1024 * 1024) runtime.heapSize = other.runtime.heapSize;
    if (other.runtime.stackSize != 8 * 1024 * 1024) runtime.stackSize = other.runtime.stackSize;
    if (other.diagnostics.warningLevel != 2) diagnostics.warningLevel = other.diagnostics.warningLevel;
    
    // Merge boolean flags (true in either config means true in result)
    diagnostics.debugMode = diagnostics.debugMode || other.diagnostics.debugMode;
    diagnostics.verbose = diagnostics.verbose || other.diagnostics.verbose;
    diagnostics.logFinalCode = diagnostics.logFinalCode || other.diagnostics.logFinalCode;
    diagnostics.logAsm = diagnostics.logAsm || other.diagnostics.logAsm;
    diagnostics.showMetadata = diagnostics.showMetadata || other.diagnostics.showMetadata;
    diagnostics.enableProfiling = diagnostics.enableProfiling || other.diagnostics.enableProfiling;
    
    keepIntermediateFiles = keepIntermediateFiles || other.keepIntermediateFiles;
    enableCaching = enableCaching || other.enableCaching;
    enableExperimentalFeatures = enableExperimentalFeatures || other.enableExperimentalFeatures;
}

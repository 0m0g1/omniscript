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
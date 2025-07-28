#include <omniscript/main.h>
#include <omniscript/Engine.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Omniscript {

namespace fs = std::filesystem;

thread_local Engine::ExecutionStats Engine::currentStats_;

const std::unordered_map<std::string_view, std::function<Engine::ParseResult(Config&, std::span<char*>&)>> 
Engine::ArgumentParser::argHandlers = {
    {"--main-source", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.mainSourceFile = args[0];
        config.sourcePaths.push_back(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--source", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.sourcePaths.push_back(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--enable-modules", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.modules.enableModules = true;
        config.modules.modulePaths.push_back(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--incremental", [](Config& config, std::span<char*>&) {
        config.incremental.enabled = true;
        return ParseResult::Success;
    }},
    {"--cache-dir", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.enableCaching = true;
        config.cacheDirectory = args[0];
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--parallel-jobs", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        try {
            config.parallelJobs = std::stoi(args[0]);
            args = args.subspan(1);
            return ParseResult::Success;
        } catch (...) {
            return ParseResult::InvalidValue;
        }
    }},
    {"--profiler", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.profiler.type = ProfilerType::Perf; // Simplified
        if (!args.subspan(1).empty() && args[1][0] != '-') {
            config.profiler.outputPath = args[1];
            args = args.subspan(2);
        } else {
            args = args.subspan(1);
        }
        config.diagnostics.enableProfiling = true;
        return ParseResult::Success;
    }},
    {"--error-log", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.errorHandling.logToFile = true;
        config.errorHandling.errorLogPath = args[0];
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--max-errors", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        try {
            config.errorHandling.maxErrorCount = std::stoi(args[0]);
            args = args.subspan(1);
            return ParseResult::Success;
        } catch (...) {
            return ParseResult::InvalidValue;
        }
    }},
    {"--debug", handleDebugArgs},
    {"-d", handleDebugArgs},
    {"--target-arch", handleTargetArgs},
    {"--target-os", handleTargetArgs},
    {"--target-triple", handleTargetArgs},
    {"--optimization-level", handleOptimizationArgs},
    {"-O", handleOptimizationArgs},
    {"--gc", handleRuntimeArgs},
    {"--safety", handleSecurityArgs},
    {"--heap-size", handleRuntimeArgs},
    {"--stack-size", handleRuntimeArgs},
    {"--enable-stack-protection", handleSecurityArgs},
    {"--enable-cfi", handleSecurityArgs},
    {"--enable-asan", handleSecurityArgs},
    {"--enable-msan", handleSecurityArgs},
    {"--enable-tsan", handleSecurityArgs},
    {"--enable-ubsan", handleSecurityArgs},
    {"--enable-pic", handleSecurityArgs},
    {"--enable-lto", handleOptimizationArgs},
    {"--enable-pgo", handleOptimizationArgs},
    {"--enable-vectorization", handleOptimizationArgs},
    {"--enable-inlining", handleOptimizationArgs},
    {"--enable-tail-calls", handleOptimizationArgs},
    {"--fast-math", handleOptimizationArgs},
    {"--emit-staticlib", handleLinkingArgs},
    {"--emit-sharedlib", handleLinkingArgs},
    {"--emit-assembly", handleLinkingArgs},
    {"--emit-ir", handleLinkingArgs},
    {"--emit-object", handleLinkingArgs},
    {"--emit-bitcode", handleLinkingArgs},
    {"--emit-wasm", handleLinkingArgs}
};

const std::unordered_map<std::string_view, TargetArch> Engine::ArgumentParser::archMap{
    {"x86_64", TargetArch::X86_64},
    {"arm64", TargetArch::ARM64},
    {"aarch64", TargetArch::ARM64},
    {"x86_32", TargetArch::X86_32},
    {"i386", TargetArch::X86_32},
    {"arm32", TargetArch::ARM32},
    {"arm", TargetArch::ARM32},
    {"riscv64", TargetArch::RISCV64},
    {"wasm32", TargetArch::WASM32},
    {"wasm64", TargetArch::WASM64},
    {"auto", TargetArch::Auto}
};

const std::unordered_map<std::string_view, TargetOS> Engine::ArgumentParser::osMap{
    {"linux", TargetOS::Linux},
    {"windows", TargetOS::Windows},
    {"win32", TargetOS::Windows},
    {"macos", TargetOS::MacOS},
    {"darwin", TargetOS::MacOS},
    {"freebsd", TargetOS::FreeBSD},
    {"android", TargetOS::Android},
    {"ios", TargetOS::iOS},
    {"wasm", TargetOS::WebAssembly},
    {"webassembly", TargetOS::WebAssembly},
    {"auto", TargetOS::Auto}
};

const std::unordered_map<std::string_view, GCStrategy> Engine::ArgumentParser::gcMap{
    {"none", GCStrategy::None},
    {"refcounting", GCStrategy::RefCounting},
    {"marksweep", GCStrategy::MarkSweep},
    {"generational", GCStrategy::Generational},
    {"incremental", GCStrategy::Incremental}
};

const std::unordered_map<std::string_view, SafetyLevel> Engine::ArgumentParser::safetyMap{
    {"unsafe", SafetyLevel::Unsafe},
    {"minimal", SafetyLevel::Minimal},
    {"standard", SafetyLevel::Standard},
    {"paranoid", SafetyLevel::Paranoid}
};

constexpr uint64_t Engine::ArgumentParser::hash(std::string_view str) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : str) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::expected<Config, std::string> 
Engine::parseArguments(int argc, char* argv[]) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (argc > MAX_ARGS) OMNISCRIPT_UNLIKELY {
        return std::unexpected("Too many arguments (limit: " + std::to_string(MAX_ARGS) + ")");
    }

    Config config;
    bool fileSpecified = false;
    config.targetArch = TargetArch::Auto;
    config.targetOS = TargetOS::Auto;
    perf::ScopedTimer parseTimer(currentStats_.parseTime);

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        auto handlerIt = ArgumentParser::argHandlers.find(arg);
        if (handlerIt != ArgumentParser::argHandlers.end()) {
            std::span<char*> args(argv + i + 1, argc - i - 1);
            auto result = handlerIt->second(config, args);
            if (result != ParseResult::Success) {
                currentStats_.result = result;
                return std::unexpected(formatError(result, arg));
            }
            i += args.size() - (argc - i - 1);
        } else if (arg == "--verbose") {
            config.diagnostics.verbose = true;
        } else if (arg == "--execute") {
            config.mode = CompileMode::JIT;
        } else if (arg == "--make") {
            config.mode = CompileMode::AOT;
        } else if (arg == "--dry") {
            config.mode = CompileMode::DryCompile;
        } else if (arg == "--version") {
            printVersion();
            std::exit(0);
        } else if (arg == "--help") {
            printUsage();
            std::exit(0);
        } else if (arg == "--list-targets") {
            listTargets();
            std::exit(0);
        } else if (arg == "--show-host-info") {
            // Assuming TargetInfo::printHostInfo exists
            std::exit(0);
        } else if (arg == "--entry") {
            if (i + 1 >= argc) return std::unexpected("Missing function name after '--entry'");
            config.entry = argv[++i];
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 >= argc) return std::unexpected("Missing path after '--output'");
            config.outputPath = argv[++i];
        } else if (arg == "--keep-obj") {
            config.keepIntermediateFiles = true;
        } else if (arg == "--log-asm") {
            config.diagnostics.logAsm = true;
        } else if (arg == "--log-final-code") {
            config.diagnostics.logFinalCode = true;
        } else if (arg == "--show-metadata") {
            config.diagnostics.showMetadata = true;
        } else if (arg == "--enable-parallel-gc") {
            config.runtime.enableParallelGC = true;
        } else if (arg == "--enable-concurrent-gc") {
            config.runtime.enableConcurrentGC = true;
        } else if (!arg.starts_with("-")) {
            if (!fileSpecified) OMNISCRIPT_LIKELY {
                config.filePath = std::string(arg);
                config.sourcePaths.push_back(std::string(arg));
                fileSpecified = true;
            } else {
                config.sourcePaths.push_back(std::string(arg));
                console.warn("Multiple file paths provided. All will be processed.");
            }
        } else {
            return std::unexpected("Unknown argument: " + std::string(arg));
        }
    }

    if (!fileSpecified && config.sourcePaths.empty()) OMNISCRIPT_UNLIKELY {
        return std::unexpected("File path is required. Use '-' to read from stdin.");
    }

    if (config.mode == CompileMode::None) {
        config.mode = CompileMode::JIT;
    }

    if (config.mainSourceFile.empty() && !config.sourcePaths.empty()) {
        config.mainSourceFile = config.sourcePaths[0];
    }

    resolveTargetConfiguration(config);
    if (config.outputPath.empty() && config.isAOTMode()) {
        setDefaultOutputPath(config);
    }
    config.autoConfigureForTarget();
    std::string validationError;
    if (!config.validate(validationError)) OMNISCRIPT_UNLIKELY {
        return std::unexpected("Configuration validation failed: " + validationError);
    }
    printConfigDebugInfo(config);
    trackResourceUsage(currentStats_);
    return config;
}

std::expected<Engine::ExecutionStats, std::string>
Engine::run(const Config& config) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    currentStats_ = {};
    perf::ScopedTimer totalTimer(currentStats_.totalTime);

    std::vector<std::shared_ptr<Statement>> statements;
    perf::ScopedTimer parseTimer(currentStats_.parseTime);
    for (const auto& source : config.sourcePaths.empty() ? std::vector<std::string>{config.filePath} : config.sourcePaths) {
        auto sourceResult = readSourceCode({.filePath = source, .mainSourceFile = source});
        if (!sourceResult) OMNISCRIPT_UNLIKELY {
            currentStats_.result = ParseResult::FileAccessError;
            error::globalErrorCollector.addError(error::Severity::Error, sourceResult.error(), "Source reading");
            return std::unexpected("Failed to read source: " + sourceResult.error());
        }
        Lexer lexer(*sourceResult, source);
        Parser parser(lexer);
        parser.setDebugMode(config.diagnostics.debugMode);
        auto parsed = parser.parse();
        statements.insert(statements.end(), parsed.begin(), parsed.end());
        // for (const auto& err : parser.getErrors()) {
        //     error::globalErrorCollector.addError(error::Severity::Error, err.message, err.context);
        // }
    }
    parseTimer.~ScopedTimer();

    if (config.diagnostics.verbose) OMNISCRIPT_UNLIKELY {
        // parser.printParseTree();
        // parser.printTokenStream();
    }
    if (config.diagnostics.debugMode) OMNISCRIPT_LIKELY {
        printConfigDebugInfo(config);
    }

    perf::MemoryTracker memTracker;
    perf::ScopedTimer compileTimer(currentStats_.compileTime);
    if (config.isAOTMode() || config.isHybridMode()) {
        Compiler compiler;
        auto compileResult = config.parallelJobs > 1 ? 
            compiler.compileParallel(statements, config, config.parallelJobs) :
            compiler.compile(statements, config, 
                [](const std::string& phase, double progress) {
                    if (console.isDebugging()) {
                        DEBUG_LOG("Compilation progress: " + phase + " (" + 
                                std::to_string(static_cast<int>(progress)) + "%)");
                    }
                });
        if (!compileResult) OMNISCRIPT_UNLIKELY {
            currentStats_.result = ParseResult::ConfigurationError;
            error::globalErrorCollector.addError(error::Severity::Error, compileResult.error(), "Compilation");
            return std::unexpected("Compilation failed: " + compileResult.error());
        }
        currentStats_.compileTime = compileResult->totalTime;
        if (config.mode == CompileMode::AOT) {
            console.log("Compilation done. Output emitted to: " + config.outputPath);
        } else if (config.isDryRun()) {
            console.log("Dry compilation complete. Output written to: " + config.outputPath);
        }
    } else if (config.isJITMode()) {
        if (config.diagnostics.debugMode) {
            DEBUG_LOG("JIT execution using target: " + config.getTargetSummary());
        }
        auto backend = std::make_shared<LLVMJITBackend>();
        backend->initialize();
        auto jitStart = std::chrono::steady_clock::now();
        backend->execute(statements, config);
        auto jitEnd = std::chrono::steady_clock::now();
        currentStats_.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(jitEnd - jitStart);
    }

    currentStats_.memoryUsage = memTracker.getPeakUsage();
    currentStats_.result = ParseResult::Success;
    return currentStats_;
}

std::expected<size_t, std::string> Engine::parseSizeString(std::string_view sizeStr) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (sizeStr.empty()) {
        OMNISCRIPT_UNLIKELY
        return std::unexpected("Empty size string");
    }
    if (sizeStr.back() >= '0' && sizeStr.back() <= '9') {
        try {
            return std::stoull(std::string(sizeStr));
        } catch (...)  {
            OMNISCRIPT_UNLIKELY
            return std::unexpected("Invalid numeric value");
        }
    }
    auto suffixPos = sizeStr.find_first_not_of("0123456789.");
    if (suffixPos == 0) {
        OMNISCRIPT_UNLIKELY
        return std::unexpected("Invalid size string: no numeric part");
    }
    double value;
    try {
        value = std::stod(std::string(sizeStr.substr(0, suffixPos)));
    } catch (...) {
        OMNISCRIPT_UNLIKELY 
        return std::unexpected("Invalid numeric value");
    }
    if (value < 0) {
        OMNISCRIPT_UNLIKELY 
        return std::unexpected("Size cannot be negative");
    }
    size_t multiplier = 1;
    if (suffixPos != std::string_view::npos) {
        auto suffix = sizeStr.substr(suffixPos);
        std::string suffixUpper;
        suffixUpper.reserve(suffix.size());
        std::transform(suffix.begin(), suffix.end(), std::back_inserter(suffixUpper),
                      [](char c) { return std::toupper(c); });
        if (suffixUpper == "B" || suffixUpper.empty()) {
            multiplier = 1;
        } else if (suffixUpper == "KB" || suffixUpper == "K") {
            multiplier = 1024ULL;
        } else if (suffixUpper == "MB" || suffixUpper == "M") {
            multiplier = 1024ULL * 1024ULL;
        } else if (suffixUpper == "GB" || suffixUpper == "G") {
            multiplier = 1024ULL * 1024ULL * 1024ULL;
        } else if (suffixUpper == "TB" || suffixUpper == "T") {
            multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
        } else {
            return std::unexpected("Unknown size suffix: " + std::string(suffix));
        }
    }
    double result = value * multiplier;
    if (result > static_cast<double>(SIZE_MAX)) {
        OMNISCRIPT_UNLIKELY
        return std::unexpected("Size value too large");
    }
    return static_cast<size_t>(result);
}

TargetArch Engine::parseTargetArch(std::string_view arch) noexcept {
    auto it = ArgumentParser::archMap.find(arch);
    if (it != ArgumentParser::archMap.end()) {
        OMNISCRIPT_LIKELY
        return it->second;
    }
    console.warn("Unknown architecture '" + std::string(arch) + "', using auto-detection");
    return TargetArch::Auto;
}

TargetOS Engine::parseTargetOS(std::string_view os) noexcept {
    auto it = ArgumentParser::osMap.find(os);
    if (it != ArgumentParser::osMap.end()) {
        OMNISCRIPT_LIKELY
        return it->second;
    }
    console.warn("Unknown operating system '" + std::string(os) + "', using auto-detection");
    return TargetOS::Auto;
}

std::pair<TargetArch, TargetOS> Engine::parseTargetTriple(std::string_view triple) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    auto parsed = TargetInfo::TargetTriple::parse(std::string(triple));
    TargetArch arch = TargetArch::Auto;
    auto archIt = ArgumentParser::archMap.find(parsed.arch);
    if (archIt != ArgumentParser::archMap.end()) {
        arch = archIt->second;
    }
    TargetOS os = TargetOS::Auto;
    auto osIt = ArgumentParser::osMap.find(parsed.os);
    if (osIt != ArgumentParser::osMap.end()) {
        os = osIt->second;
    }
    return {arch, os};
}

GCStrategy Engine::parseGCStrategy(std::string_view gc) noexcept {
    auto it = ArgumentParser::gcMap.find(gc);
    if (it != ArgumentParser::gcMap.end()) OMNISCRIPT_LIKELY {
        return it->second;
    }
    return GCStrategy::RefCounting;
}

SafetyLevel Engine::parseSafetyLevel(std::string_view safety) noexcept {
    auto it = ArgumentParser::safetyMap.find(safety);
    if (it != ArgumentParser::safetyMap.end()) {
        OMNISCRIPT_LIKELY
        return it->second;
    }
    return SafetyLevel::Standard;
}

std::expected<std::string, std::string> Engine::readSourceCode(const Config& config) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    try {
        auto filePath = config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile;
        if (filePath == "-") {
            std::ostringstream buffer;
            buffer << std::cin.rdbuf();
            return buffer.str();
        } else {
            fs::path path(filePath);
            if (!fs::exists(path)) {
                OMNISCRIPT_UNLIKELY
                return std::unexpected("File not found: " + filePath);
            }
            auto fileSize = fs::file_size(path);
            if (fileSize == 0) {
                OMNISCRIPT_UNLIKELY 
                return std::string{};
            }
            if (fileSize < 1024 * 1024) {
                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    OMNISCRIPT_UNLIKELY 
                    return std::unexpected("Cannot open file: " + filePath);
                }
                std::string content;
                content.reserve(fileSize);
                file.seekg(0, std::ios::end);
                content.resize(file.tellg());
                file.seekg(0, std::ios::beg);
                file.read(content.data(), content.size());
                return content;
            } else {
                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    OMNISCRIPT_UNLIKELY 
                    return std::unexpected("Cannot open file: " + filePath);
                }
                std::ostringstream buffer;
                buffer << file.rdbuf();
                return buffer.str();
            }
        }
    } catch (const std::exception& e) {
        OMNISCRIPT_UNLIKELY
        return std::unexpected("I/O error: " + std::string(e.what()));
    }
}

void Engine::printUsage() noexcept {
    console.log("Usage: omniscript [options] <file>");
    console.log("Options:");
    console.log("  --main-source <file>     Specify main source file");
    console.log("  --source <file>          Add additional source file");
    console.log("  --enable-modules <path>  Enable module support");
    console.log("  --incremental            Enable incremental compilation");
    console.log("  --cache-dir <dir>        Set cache directory");
    console.log("  --parallel-jobs <n>      Set number of parallel jobs");
    console.log("  --profiler <type>        Enable profiling (gprof, perf)");
    console.log("  --error-log <file>       Log errors to file");
    console.log("  --max-errors <n>         Set maximum error count");
    console.log("  --debug, -d              Enable debug mode");
    console.log("  --dry                    Compile to object file only");
    console.log("  --entry <function>       Set entry function");
    console.log("  --emit-staticlib         Emit a static library");
    console.log("  --emit-sharedlib         Emit a shared library");
    console.log("  --emit-assembly          Emit assembly source");
    console.log("  --emit-ir                Emit LLVM IR");
    console.log("  --execute                Execute using JIT");
    console.log("  --keep-obj               Keep intermediate object files");
    console.log("  --log-asm                Log generated assembly code");
    console.log("  --log-final-code         Log final IR code");
    console.log("  --make                   Compile AOT");
    console.log("  --output, -o <file>      Set output file path");
    console.log("  --optimization-level, -O <n> Optimization level: 0-3");
    console.log("  --target-arch <arch>     Target architecture: x86_64, arm64, etc.");
    console.log("  --target-os <os>         Target OS: linux, windows, macos, etc.");
    console.log("  --target-triple <triple> Set target triple");
    console.log("  --list-targets           List available targets");
    console.log("  --show-host-info         Show host system info");
    console.log("  --gc <strategy>          GC strategy: none, refcounting, etc.");
    console.log("  --safety <level>         Safety level: unsafe, minimal, standard, paranoid");
    console.log("  --enable-lto             Enable Link Time Optimization");
    console.log("  --enable-pgo             Enable Profile Guided Optimization");
    console.log("  --version                Display version");
    console.log("  --help                   Display this help");
    console.log("  --verbose                Show parse tree and token stream");
    console.log("  --show-metadata          Show metadata");
    console.log("Security Options:");
    console.log("  --enable-stack-protection   Enable stack protection");
    console.log("  --enable-cfi                Enable Control Flow Integrity");
    console.log("  --enable-asan               Enable AddressSanitizer");
    console.log("  --enable-msan               Enable MemorySanitizer");
    console.log("  --enable-tsan               Enable ThreadSanitizer");
    console.log("  --enable-ubsan              Enable UndefinedBehaviorSanitizer");
    console.log("  --enable-pic                Enable Position Independent Code");
    console.log("Optimization Options:");
    console.log("  --enable-vectorization      Enable vectorization");
    console.log("  --enable-inlining           Enable function inlining");
    console.log("  --enable-tail-calls         Enable tail call optimization");
    console.log("  --fast-math                 Enable fast math");
    console.log("Runtime Configuration:");
    console.log("  --heap-size <size>       Set heap size (e.g., 64MB)");
    console.log("  --stack-size <size>      Set stack size (e.g., 8MB)");
    console.log("  --enable-parallel-gc     Enable parallel GC");
    console.log("  --enable-concurrent-gc   Enable concurrent GC");
}

void Engine::listTargets() noexcept {
    console.log("Available Target Architectures:");
    console.log("  x86_64  - 64-bit x86 (Intel/AMD)");
    console.log("  arm64   - 64-bit ARM (Apple Silicon, etc.)");
    console.log("  x86_32  - 32-bit x86");
    console.log("  arm32   - 32-bit ARM");
    console.log("  riscv64 - 64-bit RISC-V");
    console.log("  wasm32  - 32-bit WebAssembly");
    console.log("  wasm64  - 64-bit WebAssembly");
    console.log("  auto    - Auto-detect (default)");
    console.log("");
    console.log("Available Target Operating Systems:");
    console.log("  linux   - Linux");
    console.log("  windows - Windows");
    console.log("  macos   - macOS");
    console.log("  freebsd - FreeBSD");
    console.log("  android - Android");
    console.log("  ios     - iOS");
    console.log("  wasm    - WebAssembly");
    console.log("  auto    - Auto-detect (default)");
}

void Engine::printVersion() noexcept {
    console.log(std::string("Omniscript version ") + std::string(VERSION));
}

void Engine::resolveTargetConfiguration(Config& config) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (config.targetArch == TargetArch::Auto) {
        config.targetArch = TargetInfo::detectHostArchitecture();
        DEBUG_LOG("Auto-detected target architecture: " + TargetInfo::getArchitectureName(config.targetArch));
    }
    if (config.targetOS == TargetOS::Auto) {
        config.targetOS = TargetInfo::detectHostOS();
        DEBUG_LOG("Auto-detected target OS: " + TargetInfo::getOSName(config.targetOS));
    }
}

void Engine::setDefaultOutputPath(Config& config) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (config.outputPath.empty() || config.outputPath == "a.out") {
        fs::path inputPath(config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile);
        std::string baseName = inputPath.stem().string();
        std::string extension;
        switch (config.aot.outputFormat) {
            case OutputFormat::Executable:
                extension = config.getExecutableExtension();
                break;
            case OutputFormat::SharedLib:
                extension = config.getSharedLibExtension();
                break;
            case OutputFormat::StaticLib:
                extension = config.getStaticLibExtension();
                break;
            case OutputFormat::Assembly:
                extension = ".s";
                break;
            case OutputFormat::LLVM_IR:
                extension = ".ll";
                break;
            case OutputFormat::ObjectFile:
                extension = config.getObjectFileExtension();
                break;
            case OutputFormat::Bitcode:
                extension = ".bc";
                break;
            case OutputFormat::WebAssembly:
                extension = ".wasm";
                break;
        }
        config.outputPath = baseName + extension;
        DEBUG_LOG("Default output path set to: " + config.outputPath);
    }
}

void Engine::printConfigDebugInfo(const Config& config) noexcept {
    DEBUG_LOG("=== Configuration Debug Info ===");
    DEBUG_LOG("File: " + config.filePath);
    DEBUG_LOG("Mode: " + std::to_string(static_cast<int>(config.mode)));
    DEBUG_LOG("Entry: " + config.entry);
    DEBUG_LOG("Output Path: " + config.outputPath);
    DEBUG_LOG("Optimization Level: " + std::to_string(config.optimization.level));
    DEBUG_LOG("Debug Mode: " + std::string(config.diagnostics.debugMode ? "true" : "false"));
    DEBUG_LOG("Target Summary: " + config.getTargetSummary());
    DEBUG_LOG("Cross Compilation: " + std::string(config.isCrossCompilation() ? "yes" : "no"));
}

size_t Engine::getCurrentMemoryUsage() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
#ifdef __linux__
    struct mallinfo mi = mallinfo();
    return mi.uordblks;
#elif _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#elif __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, 
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        return info.resident_size;
    }
#endif
    return 0;
}

void Engine::trackResourceUsage(ExecutionStats& stats) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    stats.memoryUsage = getCurrentMemoryUsage();
}

std::string Engine::formatError(ParseResult result, std::string_view context) noexcept {
    switch (result) {
        case ParseResult::Success: return "No error";
        case ParseResult::InvalidArgument: return "Invalid argument: " + std::string(context);
        case ParseResult::MissingValue: return "Missing value for argument: " + std::string(context);
        case ParseResult::InvalidValue: return "Invalid value for argument: " + std::string(context);
        case ParseResult::ConfigurationError: return "Configuration error: " + std::string(context);
        case ParseResult::FileAccessError: return "File access error: " + std::string(context);
        case ParseResult::ResourceLimitExceeded: return "Resource limit exceeded: " + std::string(context);
        default: return "Unknown error: " + std::string(context);
    }
}

void Engine::handleCriticalError(std::string_view error) noexcept {
    console.error(std::string(error));
    std::exit(1);
}

Engine::ParseResult Engine::handleDebugArgs(Config& config, std::span<char*>& args) noexcept {
    config.diagnostics.debugMode = true;
    console.enableDebug();
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleTargetArgs(Config& config, std::span<char*>& args) noexcept {
    if (args.empty()) return ParseResult::MissingValue;
    if (args[0] == "--target-arch") {
        config.targetArch = parseTargetArch(args[0]);
        args = args.subspan(1);
    } else if (args[0] == "--target-os") {
        config.targetOS = parseTargetOS(args[0]);
        args = args.subspan(1);
    } else if (args[0] == "--target-triple") {
        auto [arch, os] = parseTargetTriple(args[0]);
        config.targetArch = arch;
        config.targetOS = os;
        args = args.subspan(1);
    }
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleOptimizationArgs(Config& config, std::span<char*>& args) noexcept {
    if (args[0] == "--optimization-level" || args[0] == "-O") {
        if (args.size() < 2) return ParseResult::MissingValue;
        try {
            config.optimization.level = std::stoi(args[1]);
            args = args.subspan(2);
        } catch (...) {
            return ParseResult::InvalidValue;
        }
    } else if (args[0] == "--enable-lto") {
        config.aot.lto.enabled = true;
    } else if (args[0] == "--enable-pgo") {
        config.optimization.pgo.enabled = true;
    } else if (args[0] == "--enable-vectorization") {
        config.optimization.enableVectorization = true;
    } else if (args[0] == "--enable-inlining") {
        config.optimization.enableFunctionInlining = true;
    } else if (args[0] == "--enable-tail-calls") {
        config.optimization.enableTailCallOptimization = true;
    } else if (args[0] == "--fast-math") {
        config.optimization.fastMath = true;
    }
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleRuntimeArgs(Config& config, std::span<char*>& args) noexcept {
    if (args.empty()) return ParseResult::MissingValue;
    if (args[0] == "--gc") {
        config.runtime.gcStrategy = parseGCStrategy(args[0]);
        args = args.subspan(1);
    } else if (args[0] == "--heap-size") {
        auto sizeResult = parseSizeString(args[0]);
        if (!sizeResult) return ParseResult::InvalidValue;
        config.runtime.heapSize = *sizeResult;
        args = args.subspan(1);
    } else if (args[0] == "--stack-size") {
        auto sizeResult = parseSizeString(args[0]);
        if (!sizeResult) return ParseResult::InvalidValue;
        config.runtime.stackSize = *sizeResult;
        args = args.subspan(1);
    }
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleSecurityArgs(Config& config, std::span<char*>& args) noexcept {
    if (args[0] == "--safety") {
        if (args.size() < 2) return ParseResult::MissingValue;
        config.runtime.safetyLevel = parseSafetyLevel(args[1]);
        args = args.subspan(2);
    } else if (args[0] == "--enable-stack-protection") {
        config.security.enableStackProtection = true;
    } else if (args[0] == "--enable-cfi") {
        config.security.enableControlFlowIntegrity = true;
    } else if (args[0] == "--enable-asan") {
        config.security.enableAddressSanitizer = true;
    } else if (args[0] == "--enable-msan") {
        config.security.enableMemorySanitizer = true;
    } else if (args[0] == "--enable-tsan") {
        config.security.enableThreadSanitizer = true;
    } else if (args[0] == "--enable-ubsan") {
        config.security.enableUndefinedBehaviorSanitizer = true;
    } else if (args[0] == "--enable-pic") {
        config.security.enablePositionIndependentCode = true;
    }
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleLinkingArgs(Config& config, std::span<char*>& args) noexcept {
    config.mode = CompileMode::AOT;
    if (args[0] == "--emit-staticlib") {
        config.aot.outputFormat = OutputFormat::StaticLib;
    } else if (args[0] == "--emit-sharedlib") {
        config.aot.outputFormat = OutputFormat::SharedLib;
    } else if (args[0] == "--emit-assembly") {
        config.aot.outputFormat = OutputFormat::Assembly;
    } else if (args[0] == "--emit-ir") {
        config.aot.outputFormat = OutputFormat::LLVM_IR;
    } else if (args[0] == "--emit-object") {
        config.aot.outputFormat = OutputFormat::ObjectFile;
    } else if (args[0] == "--emit-bitcode") {
        config.aot.outputFormat = OutputFormat::Bitcode;
    } else if (args[0] == "--emit-wasm") {
        config.aot.outputFormat = OutputFormat::WebAssembly;
    }
    return ParseResult::Success;
}

} // namespace Omniscript
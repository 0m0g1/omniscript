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

// Fixed method implementations with correct signatures

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
        // For now, this is a stub implementation
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
        // Fixed: Use the correct member name from CompilationStats
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

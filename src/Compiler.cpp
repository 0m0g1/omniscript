#include <omniscript/main.h>
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
            Omniscript::Config tempConfig;
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

} // namespace Omniscript

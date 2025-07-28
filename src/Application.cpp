#include <omniscript/Application.h>
#include <omniscript/Main.h> 
#include <filesystem>
#include <fstream>
#include <iostream>
#include <csignal>
#ifdef __linux__
#include <sys/resource.h>
#elif _WIN32
#include <windows.h>
#endif

namespace Omniscript {

namespace fs = std::filesystem;

std::atomic<bool> Application::shutdownRequested_{false};

Application::Application(int argc, char* argv[]) noexcept 
    : startTime_(std::chrono::steady_clock::now()) {
    OMNISCRIPT_PROFILE_FUNCTION();
    initializeGlobalState();
    config_ = Engine::parseArguments(argc, argv);
}

Application::~Application() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    cleanupGlobalState();
}

int Application::run() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (!config_) OMNISCRIPT_UNLIKELY {
        console.error("Configuration error: " + config_.error());
        error::globalErrorCollector.addError(error::Severity::Fatal, config_.error(), "Configuration");
        logErrors(*config_);
        return 1;
    }

    auto result = Engine::run(*config_);
    if (!result) OMNISCRIPT_UNLIKELY {
        console.error("Execution failed: " + result.error());
        error::globalErrorCollector.addError(error::Severity::Error, result.error(), "Execution");
        logErrors(*config_);
        return 1;
    }

    if (config_->diagnostics.debugMode) {
        const auto& stats = *result;
        DEBUG_LOG("=== Performance Statistics ===");
        DEBUG_LOG("Total Time: " + std::to_string(stats.totalTime.count()) + "ms");
        DEBUG_LOG("Parse Time: " + std::to_string(stats.parseTime.count()) + "ms");
        DEBUG_LOG("Compile Time: " + std::to_string(stats.compileTime.count()) + "ms");
        DEBUG_LOG("Memory Usage: " + std::to_string(stats.memoryUsage / 1024) + "KB");
        DEBUG_LOG("Result: " + formatError(stats.result, ""));
    }

    displayProfilerResults(*config_);
    logErrors(*config_);

    if (error::globalErrorCollector.hasFatalErrors()) OMNISCRIPT_UNLIKELY {
        console.error("Fatal errors encountered during execution:");
        for (const auto& err : error::globalErrorCollector.getErrors()) {
            if (err.severity == error::Severity::Fatal) {
                console.error(err.message + " (Context: " + err.context + ")");
            }
        }
        return 1;
    }
    return 0;
}

void Application::initializeGlobalState() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    error::globalErrorCollector.clear();
    detail::globalInterner.clear();
}

void Application::cleanupGlobalState() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    console.clear();
    detail::globalInterner.clear();
    error::globalErrorCollector.clear();
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
#ifdef __linux__
    std::signal(SIGUSR1, SIG_DFL);
    std::signal(SIGUSR2, SIG_DFL);
#endif
}

void Application::signalHandler(int signal) {
    console.warn("Received signal " + std::to_string(signal));
    shutdownRequested_.store(true, std::memory_order_release);
    error::globalErrorCollector.addError(error::Severity::Info, "Shutdown requested", "Signal " + std::to_string(signal));
}

void Application::setupSignalHandlers() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef __linux__
    std::signal(SIGUSR1, signalHandler);
    std::signal(SIGUSR2, signalHandler);
#endif
}

void Application::setupMemoryLimits() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
#ifdef __linux__
    struct rlimit mem_limit;
    if (getrlimit(RLIMIT_AS, &mem_limit) == 0) {
        mem_limit.rlim_cur = 8ULL * 1024 * 1024 * 1024;
        mem_limit.rlim_max = mem_limit.rlim_cur;
        setrlimit(RLIMIT_AS, &mem_limit);
    }
    struct rlimit stack_limit;
    if (getrlimit(RLIMIT_STACK, &stack_limit) == 0) {
        stack_limit.rlim_cur = 16 * 1024 * 1024;
        stack_limit.rlim_max = stack_limit.rlim_cur;
        setrlimit(RLIMIT_STACK, &stack_limit);
    }
#elif _WIN32
    SIZE_T min_size = 0, max_size = 8ULL * 1024 * 1024 * 1024;
    SetProcessWorkingSetSize(GetCurrentProcess(), min_size, max_size);
#endif
}

void Application::displayProfilerResults(const Config& config) noexcept {
    if (!config.diagnostics.enableProfiling || config.profiler.outputPath.empty()) return;
    if (config.profiler.type == ProfilerType::Perf && fs::exists(config.profiler.outputPath)) {
        std::string cmd = "perf report -i " + config.profiler.outputPath;
        std::system(cmd.c_str());
    } else if (config.profiler.type == ProfilerType::GProf && fs::exists("gmon.out")) {
        std::string cmd = "gprof -b " + (config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile) + " gmon.out";
        std::system(cmd.c_str());
    }
}

void Application::logErrors(const Config& config) noexcept {
    if (config.errorHandling.logToFile && !config.errorHandling.errorLogPath.empty()) {
        std::ofstream logFile(config.errorHandling.errorLogPath, std::ios::app);
        if (logFile.is_open()) {
            for (const auto& err : error::globalErrorCollector.getErrors()) {
                logFile << "[" << err.timestamp << "] " << (int)err.severity << ": " << err.message << " (" << err.context << ")\n";
            }
            logFile.close();
        }
        if (config.diagnostics.verbose) {
            std::cout << "Error Log (" << config.errorHandling.errorLogPath << "):\n";
            std::ifstream logFileRead(config.errorHandling.errorLogPath);
            if (logFileRead.is_open()) {
                std::cout << logFileRead.rdbuf() << std::endl;
                logFileRead.close();
            }
        }
    }
}

} // namespace Omniscript
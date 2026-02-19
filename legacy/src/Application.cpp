#include <omniscript/Application.h>
#include <omniscript/Core.h>
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
    DEBUG_PHASE("Application Initialization");
    initializeGlobalState();
    config_ = Engine::parseArguments(argc, argv);
    DEBUG_PHASE_END();
}

Application::~Application() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    DEBUG_PHASE("Application Cleanup");
    cleanupGlobalState();
    DEBUG_PHASE_END();
}

int Application::run() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    DEBUG_PHASE("Application Run");
    
    if (!config_) {
        OMNISCRIPT_UNLIKELY
        // Use proper console error reporting instead of console.error
        REPORT_ERROR(Console::ErrorType::PARSE_ERROR, "Configuration parsing failed: " + config_.error());
        error::globalErrorCollector.addError(error::Severity::Fatal, config_.error(), "Configuration");
        logErrors(*config_);
        DEBUG_PHASE_END();
        return 1;
    }

    console.info("Starting Omniscript engine execution...");
    DEBUG_NOTE("Configuration loaded successfully");

    auto result = Engine::run(*config_);
    if (!result) {
        OMNISCRIPT_UNLIKELY
        // Use proper console error reporting
        REPORT_ERROR(Console::ErrorType::RUNTIME_ERROR, "Engine execution failed: " + result.error());
        error::globalErrorCollector.addError(error::Severity::Error, result.error(), "Execution");
        logErrors(*config_);
        DEBUG_PHASE_END();
        return 1;
    }

    console.info("\nEngine execution completed successfully");

    if (config_->diagnostics.debugMode) {
        const auto& stats = *result;
        console.info("=== Performance Statistics ===");
        console.info("Total Time: " + std::to_string(stats.totalTime.count()) + "ms");
        console.info("Parse Time: " + std::to_string(stats.parseTime.count()) + "ms");
        console.info("Compile Time: " + std::to_string(stats.compileTime.count()) + "ms");
        console.info("Memory Usage: " + std::to_string(stats.memoryUsage / 1024) + "KB");
        // Display result if it has a meaningful string representation
        // console.info("Result: " + stats.result.toString()); // Uncomment if result has toString method
    }

    displayProfilerResults(*config_);
    logErrors(*config_);

    if (error::globalErrorCollector.hasFatalErrors()) {
        OMNISCRIPT_UNLIKELY
        console.fatal("Fatal errors encountered during execution:");
        for (const auto& err : error::globalErrorCollector.getErrors()) {
            if (err.severity == error::Severity::Fatal) {
                console.error("  " + err.message + " (Context: " + err.context + ")");
            }
        }
        DEBUG_PHASE_END();
        return 1;
    }
    
    DEBUG_PHASE_END();
    console.info("Application completed successfully");
    return 0;
}

void Application::initializeGlobalState() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    DEBUG_LOG("Initializing global state");
    
    error::globalErrorCollector.clear();
    detail::globalInterner.clear();
    
    // Setup signal handlers early in initialization
    setupSignalHandlers();
    setupMemoryLimits();
    
    DEBUG_LOG("Global state initialization complete");
}

void Application::cleanupGlobalState() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    DEBUG_LOG("Cleaning up global state");
    
    // Reset signal handlers first
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
#ifdef __linux__
    std::signal(SIGUSR1, SIG_DFL);
    std::signal(SIGUSR2, SIG_DFL);
#endif

    // Clear global state
    detail::globalInterner.clear();
    error::globalErrorCollector.clear();
    
    // Report final statistics if debug mode is enabled
    if (console.isDebugging()) {
        console.info("Final error count: " + std::to_string(console.getErrorCount()));
        console.info("Final warning count: " + std::to_string(console.getWarningCount()));
    }
    
    DEBUG_LOG("Global state cleanup complete");
}

void Application::signalHandler(int signal) {
    // Use proper console reporting for signal handling
    console.warn("Received signal " + std::to_string(signal) + " - initiating graceful shutdown");
    shutdownRequested_.store(true, std::memory_order_release);
    error::globalErrorCollector.addError(error::Severity::Info, "Shutdown requested", "Signal " + std::to_string(signal));
    
    // Provide helpful information based on signal type
    switch (signal) {
        case SIGINT:
            console.info("Interrupt signal received (Ctrl+C)");
            break;
        case SIGTERM:
            console.info("Termination signal received");
            break;
#ifdef __linux__
        case SIGUSR1:
            console.info("User-defined signal 1 received");
            break;
        case SIGUSR2:
            console.info("User-defined signal 2 received");
            break;
#endif
        default:
            console.warn("Unknown signal received: " + std::to_string(signal));
            break;
    }
}

void Application::setupSignalHandlers() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    DEBUG_LOG("Setting up signal handlers");
    
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef __linux__
    std::signal(SIGUSR1, signalHandler);
    std::signal(SIGUSR2, signalHandler);
#endif
    
    DEBUG_LOG("Signal handlers configured successfully");
}

void Application::setupMemoryLimits() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    DEBUG_LOG("Setting up memory limits");

#ifdef __linux__
    struct rlimit mem_limit;
    if (getrlimit(RLIMIT_AS, &mem_limit) == 0) {
        mem_limit.rlim_cur = 8ULL * 1024 * 1024 * 1024; // 8GB
        mem_limit.rlim_max = mem_limit.rlim_cur;
        if (setrlimit(RLIMIT_AS, &mem_limit) == 0) {
            DEBUG_LOG("Virtual memory limit set to 8GB");
        } else {
            REPORT_WARNING_F("Failed to set virtual memory limit: %s", strerror(errno));
        }
    } else {
        REPORT_WARNING_F("Failed to get current memory limits: %s", strerror(errno));
    }

    struct rlimit stack_limit;
    if (getrlimit(RLIMIT_STACK, &stack_limit) == 0) {
        stack_limit.rlim_cur = 16 * 1024 * 1024; // 16MB
        stack_limit.rlim_max = stack_limit.rlim_cur;
        if (setrlimit(RLIMIT_STACK, &stack_limit) == 0) {
            DEBUG_LOG("Stack size limit set to 16MB");
        } else {
            REPORT_WARNING_F("Failed to set stack size limit: %s", strerror(errno));
        }
    } else {
        REPORT_WARNING_F("Failed to get current stack limits: %s", strerror(errno));
    }
#elif _WIN32
    SIZE_T min_size = 1 * 1024 * 1024; // 1MB
    SIZE_T max_size = 4ULL * 1024 * 1024 * 1024; // 4GB (reduced for testing)
    if (SetProcessWorkingSetSize(GetCurrentProcess(), min_size, max_size)) {
        DEBUG_LOG("Working set size configured for Windows");
    } else {
        DWORD error = GetLastError();
        char errorMsg[256] = "Unknown error";
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        errorMsg, sizeof(errorMsg), NULL);
        REPORT_WARNING_F("Failed to set working set size on Windows: %s (Error code: %lu)", errorMsg, error);
    }
#endif

    DEBUG_LOG("Memory limits configuration complete");
}

void Application::displayProfilerResults(const Config& config) noexcept {
    if (!config.diagnostics.enableProfiling || config.profiler.outputPath.empty()) {
        DEBUG_LOG("Profiling disabled or no output path specified");
        return;
    }
    
    console.info("Displaying profiler results...");
    
    if (config.profiler.type == ProfilerType::Perf && fs::exists(config.profiler.outputPath)) {
        console.info("Running perf report for profile data: " + config.profiler.outputPath);
        std::string cmd = "perf report -i " + config.profiler.outputPath;
        int result = std::system(cmd.c_str());
        if (result != 0) {
            REPORT_WARNING("Perf report execution failed with code: " + std::to_string(result));
        }
    } else if (config.profiler.type == ProfilerType::GProf && fs::exists("gmon.out")) {
        console.info("Running gprof analysis...");
        std::string executable = config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile;
        std::string cmd = "gprof -b " + executable + " gmon.out";
        int result = std::system(cmd.c_str());
        if (result != 0) {
            REPORT_WARNING("GProf analysis failed with code: " + std::to_string(result));
        }
    } else {
        REPORT_WARNING("Profiler output file not found or unsupported profiler type");
    }
}

void Application::logErrors(const Config& config) noexcept {
    const auto& errors = error::globalErrorCollector.getErrors();
    
    if (errors.empty()) {
        DEBUG_LOG("No errors to log");
        return;
    }
    
    DEBUG_LOG("Logging " + std::to_string(errors.size()) + " errors");
    
    if (config.errorHandling.logToFile && !config.errorHandling.errorLogPath.empty()) {
        std::ofstream logFile(config.errorHandling.errorLogPath, std::ios::app);
        if (logFile.is_open()) {
            console.info("Writing errors to log file: " + config.errorHandling.errorLogPath);
            
            for (const auto& err : errors) {
                logFile << "[" << err.timestamp << "] " 
                       << static_cast<int>(err.severity) << ": " 
                       << err.message << " (" << err.context << ")\n";
            }
            logFile.close();
            
            console.info("Successfully wrote " + std::to_string(errors.size()) + " errors to log file");
        } else {
            IO_ERROR("Failed to open error log file: " + config.errorHandling.errorLogPath);
        }
        
        // Display log contents if verbose mode is enabled
        if (config.diagnostics.verbose) {
            console.info("Error Log Contents (" + config.errorHandling.errorLogPath + "):");
            console.info(std::string(50, '-'));
            
            std::ifstream logFileRead(config.errorHandling.errorLogPath);
            if (logFileRead.is_open()) {
                std::string line;
                while (std::getline(logFileRead, line)) {
                    console.log("  " + line);
                }
                logFileRead.close();
                console.info(std::string(50, '-'));
            } else {
                IO_ERROR("Failed to read error log file for display");
            }
        }
    } else {
        DEBUG_LOG("Error logging to file is disabled");
        
        // If not logging to file, at least display error summary
        if (config.diagnostics.verbose) {
            console.warn("Error Summary:");
            for (const auto& err : errors) {
                std::string severityStr;
                switch (err.severity) {
                    case error::Severity::Info: severityStr = "INFO"; break;
                    case error::Severity::Warning: severityStr = "WARN"; break;
                    case error::Severity::Error: severityStr = "ERROR"; break;
                    case error::Severity::Fatal: severityStr = "FATAL"; break;
                    default: severityStr = "UNKNOWN"; break;
                }
                console.log("  [" + severityStr + "] " + err.message + " (" + err.context + ")");
            }
        }
    }
}

} // namespace Omniscript
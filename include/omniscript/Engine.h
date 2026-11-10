#pragma once

#include <omniscript/Core.h>
#include <omniscript/Target_config.h>
#include <omniscript/EngineConfigs.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Compiler.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <span>
#include <expected>

namespace Omniscript {

class Engine final {
public:
    static constexpr std::string_view VERSION = "1.0.0";
    static constexpr size_t MAX_ARGS = 1024;
    static constexpr size_t DEFAULT_THREAD_COUNT = 0;

    enum class ParseResult {
        Success,
        InvalidArgument,
        MissingValue,
        InvalidValue,
        ConfigurationError,
        FileAccessError,
        ResourceLimitExceeded
    };

    struct ExecutionStats {
        std::chrono::milliseconds totalTime{0};
        std::chrono::milliseconds parseTime{0};
        std::chrono::milliseconds compileTime{0};
        size_t memoryUsage{0};
        ParseResult result{ParseResult::Success};
    };

    struct ArgumentParser {
        static const std::unordered_map<std::string_view, std::function<ParseResult(Config&, std::span<char*>&)>> argHandlers;
        static const std::unordered_map<std::string_view, TargetArch> archMap;
        static const std::unordered_map<std::string_view, TargetOS> osMap;
        static const std::unordered_map<std::string_view, GCStrategy> gcMap;
        static const std::unordered_map<std::string_view, SafetyLevel> safetyMap;
        static constexpr uint64_t hash(std::string_view str) noexcept;
    };

    [[nodiscard]] static std::expected<Config, std::string> 
    parseArguments(int argc, char* argv[]) noexcept;
    
    [[nodiscard]] static std::expected<ExecutionStats, std::string>
    run(const Config& config) noexcept;

    [[nodiscard]] static std::expected<size_t, std::string>
    parseSizeString(std::string_view sizeStr) noexcept;
    
    [[nodiscard]] static TargetArch parseTargetArch(std::string_view arch) noexcept;
    [[nodiscard]] static TargetOS parseTargetOS(std::string_view os) noexcept;
    [[nodiscard]] static std::pair<TargetArch, TargetOS> parseTargetTriple(std::string_view triple) noexcept;
    [[nodiscard]] static GCStrategy parseGCStrategy(std::string_view gc) noexcept;
    [[nodiscard]] static SafetyLevel parseSafetyLevel(std::string_view safety) noexcept;

    [[nodiscard]] static std::expected<std::string, std::string>
    readSourceCode(const Config& config) noexcept;
    
    static void printUsage() noexcept;
    static void listTargets() noexcept;
    static void printVersion() noexcept;

private:
    [[nodiscard]] static ParseResult handleDebugArgs(Config& config, std::span<char*>& args) noexcept;
    [[nodiscard]] static ParseResult handleTargetArgs(Config& config, std::span<char*>& args) noexcept;
    [[nodiscard]] static ParseResult handleOptimizationArgs(Config& config, std::span<char*>& args) noexcept;
    [[nodiscard]] static ParseResult handleRuntimeArgs(Config& config, std::span<char*>& args) noexcept;
    [[nodiscard]] static ParseResult handleSecurityArgs(Config& config, std::span<char*>& args) noexcept;
    [[nodiscard]] static ParseResult handleLinkingArgs(Config& config, std::span<char*>& args) noexcept;
    
    static void resolveTargetConfiguration(Config& config) noexcept;
    static void setDefaultOutputPath(Config& config) noexcept;
    static void printConfigDebugInfo(const Config& config) noexcept;
    
    [[nodiscard]] static size_t getCurrentMemoryUsage() noexcept;
    static void trackResourceUsage(ExecutionStats& stats) noexcept;
    
    [[nodiscard]] static std::string formatError(ParseResult result, std::string_view context) noexcept;
    static void handleCriticalError(std::string_view error) noexcept;
    
    thread_local static ExecutionStats currentStats_;
};

} // namespace Omniscript
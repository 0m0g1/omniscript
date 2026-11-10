#pragma once

#include <omniscript/Core.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Target_config.h>
#include <omniscript/EngineConfigs.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Backends/llvm/LLVMJITBackend.h>
#include <omniscript/Backends/llvm/LLVMAOTBackend.h>
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <expected>

namespace Omniscript {

class Compiler final {
public:
    enum class CompileResult {
        Success,
        ConfigurationError,
        TargetValidationFailed,
        BackendInitializationFailed,
        CompilationFailed,
        LinkingFailed
    };

    struct CompilationStats {
        std::chrono::milliseconds totalTime{0};
        std::chrono::milliseconds parseTime{0};
        std::chrono::milliseconds codegenTime{0};
        std::chrono::milliseconds linkTime{0};
        size_t memoryPeakUsage{0};
        size_t linesProcessed{0};
        size_t functionsCompiled{0};
        size_t optimizationsApplied{0};
    };

    using CompileCallback = std::function<void(const std::string& phase, double progress)>;

    Compiler() noexcept;
    ~Compiler() noexcept;

    Compiler(const Compiler&) = delete;
    Compiler& operator=(const Compiler&) = delete;
    Compiler(Compiler&&) noexcept = default;
    Compiler& operator=(Compiler&&) noexcept = default;

    [[nodiscard]] std::expected<CompilationStats, std::string> 
    compile(const std::vector<std::shared_ptr<Statement>>& statements, 
            const Config& config, 
            CompileCallback callback = nullptr) noexcept;

    [[nodiscard]] std::expected<CompilationStats, std::string>
    compileParallel(const std::vector<std::shared_ptr<Statement>>& statements,
                   const Config& config,
                   size_t threadCount = 0) noexcept;

    [[nodiscard]] const CompilationStats& getLastStats() const noexcept { return lastStats_; }
    [[nodiscard]] bool isBusy() const noexcept { return busy_.load(std::memory_order_acquire); }
    void cancel() noexcept { cancelled_.store(true, std::memory_order_release); }

private:
    [[nodiscard]] bool validateTargetConfiguration(const Config& config, std::string& error) const noexcept;
    void printTargetInfo(const Config& config) const noexcept;
    [[nodiscard]] std::expected<void, std::string> initializeBackend(const Config& config) noexcept;
    void updateProgress(const CompileCallback& callback, std::string_view phase, double progress) const noexcept;
    void trackMemoryUsage() noexcept;
    void cleanupResources() noexcept;
    void saveCache(const Config& config) noexcept;
    void loadCache(const Config& config) noexcept;
    size_t getCurrentMemoryUsage() noexcept;

    mutable std::mutex stateMutex_;
    std::atomic<bool> busy_{false};
    std::atomic<bool> cancelled_{false};
    CompilationStats lastStats_;
    std::unique_ptr<LLVMJITBackend> jitBackend_;
    std::unique_ptr<LLVMAOTBackend> aotBackend_;
    size_t memoryBaseline_{0};
    mutable std::unordered_map<std::string, bool> validationCache_;
    mutable std::mutex cacheMutex_;
};

} // namespace Omniscript
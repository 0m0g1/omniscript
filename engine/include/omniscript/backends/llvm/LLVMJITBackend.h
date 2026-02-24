#pragma once

#include <omniscript/backends/JITBackend.h>
#include <omniscript/backends/llvm/IRGenerator.h>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace Omniscript {

class LLVMJITBackend : public JITBackend {
public:
    LLVMJITBackend();
    ~LLVMJITBackend() override = default;

    void initialize(const Config& config) override;
    void execute(const Program& program, const Config& config) override;

    void cleanup();

    // JIT-specific helpers (optional; keep parity with legacy API)
    bool isCompiled(const std::string& functionName) const;
    void invalidateFunction(const std::string& functionName);

    // Cache controls
    void clearCodeCache();
    size_t getCodeCacheSize() const;
    void setCodeCacheLimit(size_t limit);

private:
    std::unique_ptr<llvm::orc::LLJIT> m_jit;
    std::shared_ptr<IRGenerator> m_irGen;

    std::chrono::system_clock::time_point m_compilationStartTime;
    size_t m_codeCacheSize = 0;
    size_t m_peakMemoryUsage = 0;
    bool m_profilerInitialized = false;

    std::map<std::string, std::filesystem::file_time_type> m_fileCache;
    mutable std::mutex m_cacheMutex;

private:
    // Setup methods (mirrors your legacy structure)
    void setupJITEngine(const Config& config);
    void setupExternalResolvers(const Config& config);
    void configureJITOptions(const Config& config);

    llvm::Expected<llvm::orc::JITTargetMachineBuilder>
    createTargetMachineBuilder(const Config& config);

    std::unique_ptr<llvm::orc::DefinitionGenerator>
    createHostProcessResolver(const Config& config);

    void registerRuntimeSymbols(const Config& config);

    void updateFileCache(const Config& config);
    bool isFileUpToDate(const std::string& filePath);

    void initializeProfiler(const Config& config);
    void finalizeProfiler(const Config& config);

    void executePluginCallbacks(const Config& config, const std::string& stage);
    void trackMemoryUsage(const Config& config);
    bool checkTimeLimit(const Config& config);
    void logError(const Config& config, const std::string& message);

    // Execution helpers
    llvm::Expected<llvm::orc::ExecutorAddr> lookupFunction(const std::string& name);
    void executeFunction(const std::string& functionName, const Config& config);

    bool hasMainFunction() const;
};

} // namespace Omniscript
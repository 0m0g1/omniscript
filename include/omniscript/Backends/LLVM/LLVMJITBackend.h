#pragma once
#include <omniscript/omniscript_pch.h>
#include <omniscript/Target_config.h>
#include <omniscript/Backends/JITBackend.h>
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DarwinLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/AndroidLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/SmartPlatformLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>
#include <omniscript/Backends/llvm/IRGenerator.h>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <filesystem>
#include <mutex>

namespace Omniscript {
class LLVMJITBackend : public JITBackend {
private:
    std::unique_ptr<llvm::orc::LLJIT> jit;
    std::shared_ptr<llvm::orc::SymbolStringPool> symbolStringPool;
    std::shared_ptr<IRGenerator> irGen;
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope;
    std::chrono::system_clock::time_point compilationStartTime;
    size_t codeCacheSize = 0;
    size_t peakMemoryUsage = 0;
    bool profilerInitialized = false;
    std::map<std::string, std::filesystem::file_time_type> fileCache;
    std::mutex cacheMutex;

    // Configuration-based setup methods
    void setupJITEngine(const Config& config);
    void setupExternalResolvers(const Config& config);
    void configureJITOptions(const Config& config);
    llvm::Expected<llvm::orc::JITTargetMachineBuilder> createTargetMachineBuilder(const Config& config);
    std::unique_ptr<llvm::orc::DefinitionGenerator> createHostProcessResolver(const Config& config);
    void registerRuntimeSymbols(const Config& config);
    void updateFileCache(const Config& config);
    bool isFileUpToDate(const std::string& filePath);
    void initializeProfiler(const Config& config);
    void finalizeProfiler(const Config& config);
    void executePluginCallbacks(const Config& config, const std::string& stage);
    void trackMemoryUsage(const Config& config);
    bool checkTimeLimit(const Config& config);
    void logError(const Config& config, const std::string& message);
    void emitAOTOutput(std::unique_ptr<llvm::Module> module, const std::string& outputPath, const Config& config);
    size_t estimateFunctionSize(const std::string& functionName);

    // Execution helpers
    llvm::Expected<llvm::orc::ExecutorAddr> lookupFunction(const std::string& name);
    void executeFunction(const std::string& functionName, const Config& config);
    bool hasMainFunction();

public:
    LLVMJITBackend();
    ~LLVMJITBackend() override = default;

    void initialize() override;
    void execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) override;
    void cleanup();

    // JIT-specific methods
    bool isCompiled(const std::string& functionName) const;
    void compileFunction(const std::string& functionName);
    void invalidateFunction(const std::string& functionName);

    // Memory and cache management
    void clearCodeCache();
    size_t getCodeCacheSize() const;
    void setCodeCacheLimit(size_t limit);
};

} // namespace Omniscript
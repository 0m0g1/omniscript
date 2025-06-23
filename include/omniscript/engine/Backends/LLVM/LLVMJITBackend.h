#pragma once
#include <omniscript/omniscript_pch.h>
#include <omniscript/Core/Target_config.h>
#include <omniscript/engine/Backends/JITBackend.h>
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/DarwinLLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/AndroidLLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/SmartPlatformLLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>

class LLVMJITBackend : public JITBackend {
private:
    std::unique_ptr<llvm::orc::LLJIT> jit;
    std::shared_ptr<llvm::orc::SymbolStringPool> symbolStringPool;
    std::shared_ptr<IRGenerator> irGen;
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope;
    
    // Configuration-based setup methods
    void setupJITEngine(const Config& config);
    void setupTargetMachine(const Config& config);
    void setupExternalResolvers(const Config& config);
    void configureJITOptions(const Config& config);
    void setupMemoryManager(const Config& config);
    void setupCompilationLayers(const Config& config);
    
    // JIT-specific helper methods
    llvm::Expected<llvm::orc::JITTargetMachineBuilder> createTargetMachineBuilder(const Config& config);
    std::unique_ptr<llvm::orc::DefinitionGenerator> createHostProcessResolver(const Config& config);
    void registerRuntimeSymbols(const Config& config);
    
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
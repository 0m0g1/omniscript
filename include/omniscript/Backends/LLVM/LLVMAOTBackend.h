#pragma once

#include <omniscript/Backends/AOTBackend.h>
#include <omniscript/Backends/llvm/IRGenerator.h>
#include <omniscript/omniscript_pch.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <chrono>
#include <mutex>
#include <unordered_map>

namespace Omniscript {

class LLVMAOTBackend : public AOTBackend {
public:

    LLVMAOTBackend();
    void execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) override;
    void initialize() override;
    void emitToFile(const Config& config) override;
        
private:
    
    std::shared_ptr<IRGenerator> irGen;
    std::vector<std::pair<std::string, std::shared_ptr<llvm::TargetMachine>>> targetMachines; // Multi-target support
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope;
    std::string outputPath;
    LinkDependencies linkerDependencies;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> fileCache; // Incremental compilation
    std::mutex cacheMutex; // Thread-safe cache access
    std::chrono::system_clock::time_point compilationStartTime; // Time tracking
    size_t peakMemoryUsage = 0; // Memory tracking
    bool profilerInitialized = false; // Profiler state

    // Emission methods
    void emitObjectFile(const std::string& objFile, const std::shared_ptr<llvm::TargetMachine>& tm);
    void emitAssemblyFile(const std::string& asmFile, const std::shared_ptr<llvm::TargetMachine>& tm);
    void emitLLVMIR(const std::string& irFile);
    void emitBitcode(const std::string& bcFile);
    void createStaticLibrary(const std::string& objFile, const std::string& libFile);
    void createSharedLibrary(const std::string& objFile, const std::string& libFile, const Config& config);
    void emitMachineCode(const std::string& mcFile, const std::shared_ptr<llvm::TargetMachine>& tm);
    void emitModuleFile(const std::string& modFile);
    void emitPrecompiledHeader(const std::string& pchFile);
    void emitWebAssembly(const std::string& wasmFile);
    void emitPTX(const std::string& ptxFile);
    void emitSPIRV(const std::string& spirvFile);
    void emitDebugInfo(const std::string& debugFile);
    void emitSymbolTable(const std::string& symFile);

    // Linking
    void linkExecutable(const std::string& objFile, const std::string& exeFile, const Config& config);
    std::vector<std::string> buildLinkerArgs(
        const std::string& exeFile,
        const std::string& objFile,
        const std::vector<std::string>& additionalLibs,
        const std::vector<std::string>& defaultLibs,
        const Config& config
    );
    std::vector<std::string> buildMSVCLinkerArgs(
        const std::string& exeFile,
        const std::string& objFile,
        const std::vector<std::string>& additionalLibs,
        const std::vector<std::string>& defaultLibs,
        const Config& config
    );
    bool isLinkerAvailable(const std::string& linker, const Config& config);

    // Target machine configuration
    void setupTargetMachine(const Config& config);
    llvm::TargetOptions buildTargetOptions(const Config& config);
    std::optional<llvm::Reloc::Model> getRelocationModel(const Config& config);
    std::optional<llvm::CodeModel::Model> getCodeModel(const Config& config);
    llvm::CodeGenOptLevel mapOptimizationLevel(int level);
    void configureTargetMachineSettings(const Config& config, std::shared_ptr<llvm::TargetMachine> tm);

    // External resolvers
    void setupExternalResolvers(const Config& config);

    // Utility methods
    std::filesystem::path getTemporaryPath(const Config& config, const std::string& extension);
    void updateFileCache(const Config& config); // Incremental compilation
    bool isFileUpToDate(const std::string& filePath); // Incremental compilation
    void initializeProfiler(const Config& config); // Profiler setup
    void finalizeProfiler(const Config& config); // Profiler cleanup
    void executePluginCallbacks(const Config& config, const std::string& stage); // Plugin integration
    void trackMemoryUsage(const Config& config); // Memory constraint enforcement
    bool checkTimeLimit(const Config& config); // Time constraint enforcement
    void logError(const Config& config, const std::string& message); // Error handling
};

} // namespace Omniscript
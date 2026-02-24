#pragma once

#include <omniscript/backends/AOTBackend.h>
#include <omniscript/backends/llvm/IRGenerator.h>

#include <llvm/Target/TargetMachine.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Omniscript {

class LLVMAOTBackend : public AOTBackend {
public:
    LLVMAOTBackend();
    ~LLVMAOTBackend() override = default;

    void initialize(const Config& config) override;
    void execute(const Program& program, const Config& config) override;
    void emitToFile(const Config& config) override;

private:
    std::shared_ptr<IRGenerator> m_irGen;

    std::vector<std::pair<std::string, std::shared_ptr<llvm::TargetMachine>>> m_targetMachines;

    std::string m_outputPath;
    LinkDependencies m_linkerDependencies;

    std::unordered_map<std::string, std::chrono::system_clock::time_point> m_fileCache;
    std::mutex m_cacheMutex;

    std::chrono::system_clock::time_point m_compilationStartTime;
    size_t m_peakMemoryUsage = 0;
    bool m_profilerInitialized = false;

private:
    void setupTargetMachine(const Config& config);
    llvm::TargetOptions buildTargetOptions(const Config& config);
    std::optional<llvm::Reloc::Model> getRelocationModel(const Config& config);
    std::optional<llvm::CodeModel::Model> getCodeModel(const Config& config);
    llvm::CodeGenOptLevel mapOptimizationLevel(int level);
    void configureTargetMachineSettings(const Config& config, std::shared_ptr<llvm::TargetMachine> tm);

    void setupExternalResolvers(const Config& config);

    // Emission
    void emitObjectFile(const std::string& objFile, const std::shared_ptr<llvm::TargetMachine>& tm);
    void emitAssemblyFile(const std::string& asmFile, const std::shared_ptr<llvm::TargetMachine>& tm);
    void emitLLVMIR(const std::string& irFile);
    void emitBitcode(const std::string& bcFile);
    void linkExecutable(const std::string& objFile, const std::string& exeFile, const Config& config);

    std::filesystem::path getTemporaryPath(const Config& config, const std::string& extension);

    void updateFileCache(const Config& config);
    bool isFileUpToDate(const std::string& filePath);
    bool areAllFilesUpToDate(const Config& config);

    void initializeProfiler(const Config& config);
    void finalizeProfiler(const Config& config);

    void executePluginCallbacks(const Config& config, const std::string& stage);
    void trackMemoryUsage(const Config& config);
    bool checkTimeLimit(const Config& config);
    void logError(const Config& config, const std::string& message);
};

} // namespace Omniscript
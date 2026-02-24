#include <omniscript/backends/llvm/LLVMAOTBackend.h>

#include <llvm/TargetParser/Host.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>

#include <filesystem>
#include <cstdlib>
#include <sstream>

namespace fs = std::filesystem;

namespace Omniscript {

LLVMAOTBackend::LLVMAOTBackend() = default;

void LLVMAOTBackend::initialize(const Config& config) {
    (void)config;
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
}

void LLVMAOTBackend::execute(const Program& program, const Config& config) {
    m_compilationStartTime = std::chrono::system_clock::now();
    initializeProfiler(config);
    executePluginCallbacks(config, "pre-execute");

    std::string validationError;
    if (!config.validate(validationError)) {
        logError(config, "Configuration validation failed: " + validationError);
        throw std::runtime_error("Configuration validation failed: " + validationError);
    }

    if (config.incremental.enabled && areAllFilesUpToDate(config) && fs::exists(config.outputPath)) {
        executePluginCallbacks(config, "post-execute");
        finalizeProfiler(config);
        return;
    }

    // IR generator (AST-native)
    m_irGen = std::make_shared<IRGenerator>(config);
    m_irGen->initialize();
    setupExternalResolvers(config);

    // Generate IR from Program
    m_irGen->codegenProgram(program);
    m_irGen->finalize();

    // Keep link deps for executable link step
    m_linkerDependencies = m_irGen->getLinkDependencies();

    // Targets
    setupTargetMachine(config);

    // Emit
    emitToFile(config);

    updateFileCache(config);
    executePluginCallbacks(config, "post-execute");
    finalizeProfiler(config);
}

void LLVMAOTBackend::emitToFile(const Config& config) {
    fs::path out(config.outputPath);
    if (out.has_parent_path()) fs::create_directories(out.parent_path());

    for (const auto& [triple, tm] : m_targetMachines) {
        std::string suffix = (m_targetMachines.size() > 1) ? ("_" + triple) : "";
        fs::path targetOut = out;
        targetOut = out.parent_path() / (out.stem().string() + suffix + out.extension().string());

        switch (config.aot.outputFormat) {
            case OutputFormat::LLVM_IR:
            case OutputFormat::TextualIR:
                emitLLVMIR(targetOut.string());
                break;

            case OutputFormat::Bitcode:
            case OutputFormat::BinaryIR:
                emitBitcode(targetOut.string());
                break;

            case OutputFormat::Assembly:
                emitAssemblyFile(targetOut.string(), tm);
                break;

            case OutputFormat::ObjectFile:
            case OutputFormat::Relocatable:
                emitObjectFile(targetOut.string(), tm);
                break;

            case OutputFormat::Executable: {
                fs::path obj = getTemporaryPath(config, suffix + ".o");
                emitObjectFile(obj.string(), tm);
                linkExecutable(obj.string(), targetOut.string(), config);
                if (!config.keepIntermediateFiles) fs::remove(obj);
                break;
            }

            default:
                logError(config, "Unsupported output format in minimal AOT backend");
                throw std::runtime_error("Unsupported output format");
        }
    }
}

void LLVMAOTBackend::setupTargetMachine(const Config& config) {
    m_targetMachines.clear();

    auto targets = config.multiTargets.empty()
        ? std::vector<std::pair<TargetArch, TargetOS>>{{config.resolveTargetArch(), config.resolveTargetOS()}}
        : config.multiTargets;

    for (const auto& [arch, os] : targets) {
        const std::string tripleStr = config.getEffectiveTargetTriple(arch, os);

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(tripleStr, error);
        if (!target) {
            logError(config, "Target lookup failed for '" + tripleStr + "': " + error);
            throw std::runtime_error("Target lookup failed: " + error);
        }

        const std::string cpu = (config.cpuFeatures == "native" && !config.isCrossCompilation())
            ? llvm::sys::getHostCPUName().str()
            : config.getDefaultCPU(arch);

        // NOTE: feature string can come from your TargetInfo later; keep empty for now
        const std::string features = "";

        llvm::TargetOptions opt = buildTargetOptions(config);
        auto relocModel = getRelocationModel(config);
        auto codeModel  = getCodeModel(config);
        llvm::CodeGenOptLevel optLevel = mapOptimizationLevel(config.optimization.level);

        auto tm = std::shared_ptr<llvm::TargetMachine>(
            target->createTargetMachine(tripleStr, cpu, features, opt, relocModel, codeModel, optLevel)
        );
        if (!tm) {
            logError(config, "Failed to create target machine for: " + tripleStr);
            throw std::runtime_error("Failed to create target machine");
        }

        configureTargetMachineSettings(config, tm);
        m_targetMachines.emplace_back(tripleStr, tm);
    }
}

llvm::TargetOptions LLVMAOTBackend::buildTargetOptions(const Config& config) {
    llvm::TargetOptions opt;
    opt.UnsafeFPMath = config.optimization.fastMath;
    opt.NoInfsFPMath = config.optimization.fastMath;
    opt.NoNaNsFPMath = config.optimization.fastMath;
    opt.NoSignedZerosFPMath = config.optimization.fastMath;
    opt.GuaranteedTailCallOpt = config.optimization.enableTailCallOptimization;
    opt.ThreadModel = (config.runtime.enableParallelGC || config.runtime.gcThreads > 1)
        ? llvm::ThreadModel::POSIX
        : llvm::ThreadModel::Single;
    return opt;
}

std::optional<llvm::Reloc::Model> LLVMAOTBackend::getRelocationModel(const Config& config) {
    if (config.security.enablePositionIndependentCode || config.aot.outputFormat == OutputFormat::SharedLib)
        return llvm::Reloc::PIC_;
    if (config.aot.staticLinking)
        return llvm::Reloc::Static;
    return std::nullopt;
}

std::optional<llvm::CodeModel::Model> LLVMAOTBackend::getCodeModel(const Config& config) {
    auto arch = config.resolveTargetArch();
    if (arch == TargetArch::ARM32 || arch == TargetArch::WASM32) return llvm::CodeModel::Small;
    if (arch == TargetArch::X86_64 || arch == TargetArch::ARM64) return llvm::CodeModel::Large;
    return std::nullopt;
}

llvm::CodeGenOptLevel LLVMAOTBackend::mapOptimizationLevel(int level) {
    switch (level) {
        case 0: return llvm::CodeGenOptLevel::None;
        case 1: return llvm::CodeGenOptLevel::Less;
        case 2: return llvm::CodeGenOptLevel::Default;
        case 3: return llvm::CodeGenOptLevel::Aggressive;
        default: return llvm::CodeGenOptLevel::Default;
    }
}

void LLVMAOTBackend::configureTargetMachineSettings(const Config&, std::shared_ptr<llvm::TargetMachine>) {
    // keep as hooks; most config is baked into TargetOptions/Module flags
}

void LLVMAOTBackend::setupExternalResolvers(const Config& config) {
    (void)config;
    // If your new IRGenerator supports resolvers / link deps, wire them here.
    // Example:
    // m_irGen->addExternalResolver("C", std::make_unique<CStdLibResolver>());
}

void LLVMAOTBackend::emitLLVMIR(const std::string& irFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(irFile, ec, llvm::sys::fs::OF_Text);
    if (ec) throw std::runtime_error("Failed to open IR output: " + ec.message());

    llvm::Module* M = m_irGen->getModule();
    M->print(dest, nullptr);
    dest.flush();
}

void LLVMAOTBackend::emitBitcode(const std::string& bcFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(bcFile, ec, llvm::sys::fs::OF_None);
    if (ec) throw std::runtime_error("Failed to open bitcode output: " + ec.message());

    llvm::Module* M = m_irGen->getModule();
    llvm::WriteBitcodeToFile(*M, dest);
    dest.flush();
}

void LLVMAOTBackend::emitObjectFile(const std::string& objFile, const std::shared_ptr<llvm::TargetMachine>& tm) {
    llvm::Module* M = m_irGen->getModule();
    M->setTargetTriple(tm->getTargetTriple().str());
    M->setDataLayout(tm->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream dest(objFile, ec, llvm::sys::fs::OF_None);
    if (ec) throw std::runtime_error("Failed to open object output: " + ec.message());

    llvm::legacy::PassManager pm;
    if (tm->addPassesToEmitFile(pm, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        throw std::runtime_error("TargetMachine can't emit object file");
    }
    pm.run(*M);
    dest.flush();
}

void LLVMAOTBackend::emitAssemblyFile(const std::string& asmFile, const std::shared_ptr<llvm::TargetMachine>& tm) {
    llvm::Module* M = m_irGen->getModule();
    M->setTargetTriple(tm->getTargetTriple().str());
    M->setDataLayout(tm->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream dest(asmFile, ec, llvm::sys::fs::OF_None);
    if (ec) throw std::runtime_error("Failed to open asm output: " + ec.message());

    llvm::legacy::PassManager pm;
    if (tm->addPassesToEmitFile(pm, dest, nullptr, llvm::CodeGenFileType::AssemblyFile)) {
        throw std::runtime_error("TargetMachine can't emit assembly file");
    }
    pm.run(*M);
    dest.flush();
}

void LLVMAOTBackend::linkExecutable(const std::string& objFile, const std::string& exeFile, const Config& config) {
    // Minimal: call clang++/g++ with obj and libs.
    // Later you can port your full legacy linker strategy.
    std::vector<std::string> cmd;

#ifdef _WIN32
    cmd = {"clang++", objFile, "-o", exeFile};
#else
    cmd = {"clang++", objFile, "-o", exeFile};
#endif

    // add -L / -l from config
    for (const auto& p : config.aot.libraryPaths) cmd.push_back("-L" + p);
    for (const auto& l : config.aot.libraries) cmd.push_back("-l" + l);

    // add linker deps discovered by resolvers
    for (const auto& f : m_linkerDependencies.getLinkerFlags()) cmd.push_back(f);

    // join command
    std::ostringstream oss;
    for (auto& s : cmd) {
        oss << "\"" << s << "\" ";
    }

    int rc = std::system(oss.str().c_str());
    if (rc != 0) {
        throw std::runtime_error("Linking failed with exit code: " + std::to_string(rc));
    }

#ifndef _WIN32
    fs::permissions(exeFile,
        fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
        fs::perm_options::add);
#endif
}

std::filesystem::path LLVMAOTBackend::getTemporaryPath(const Config& config, const std::string& extension) {
    fs::path base = !config.tempDirectory.empty()
        ? fs::path(config.tempDirectory)
        : fs::path(config.outputPath).parent_path();

    if (base.empty()) base = ".";
    return base / (fs::path(config.outputPath).stem().string() + extension);
}

// --- cache / profiler / hooks (minimal stubs) ---

void LLVMAOTBackend::updateFileCache(const Config& config) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto main = config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile;
    if (!main.empty() && fs::exists(main)) {
        m_fileCache[main] = std::chrono::system_clock::now();
    }
    for (auto& p : config.sourcePaths) {
        if (fs::exists(p)) m_fileCache[p] = std::chrono::system_clock::now();
    }
}

bool LLVMAOTBackend::isFileUpToDate(const std::string&) { return false; }
bool LLVMAOTBackend::areAllFilesUpToDate(const Config&) { return false; }

void LLVMAOTBackend::initializeProfiler(const Config&) {}
void LLVMAOTBackend::finalizeProfiler(const Config&) {}
void LLVMAOTBackend::executePluginCallbacks(const Config&, const std::string&) {}
void LLVMAOTBackend::trackMemoryUsage(const Config&) {}
bool LLVMAOTBackend::checkTimeLimit(const Config&) { return true; }

void LLVMAOTBackend::logError(const Config& config, const std::string& message) {
    (void)config;
    // console.error(message);
}

} // namespace Omniscript
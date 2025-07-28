#include <omniscript/Backends/llvm/LLVMAOTBackend.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Program.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetRegistry.h>
#include <thread>
#include <chrono>
#include <fstream>

namespace Omniscript {

LLVMAOTBackend::LLVMAOTBackend() {
    scope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>();
}

void LLVMAOTBackend::initialize() {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    DEBUG_LOG("LLVMAOTBackend initialized LLVM target registry");
}

void LLVMAOTBackend::setupTargetMachine(const Config& config) {
    targetMachines.clear();
    auto targets = config.multiTargets.empty() ? 
        std::vector<std::pair<TargetArch, TargetOS>>{{config.resolveTargetArch(), config.resolveTargetOS()}} : 
        config.multiTargets;

    for (const auto& [arch, os] : targets) {
        std::string targetTriple = config.getEffectiveTargetTriple(arch, os);
        std::string cpu = irGen->resolveCPUName((config.cpuFeatures == "native" && !config.isCrossCompilation()) ? 
            "native" : config.getDefaultCPU(arch));
        std::string features = irGen->buildFeatureString(targetTriple);

        DEBUG_LOG("Target triple: " + targetTriple);
        DEBUG_LOG("CPU: " + cpu);
        DEBUG_LOG("Features: " + features);

        std::string error;
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
        if (!target) {
            logError(config, "Target lookup failed for '" + targetTriple + "': " + error);
            throw std::runtime_error("Target lookup failed: " + error);
        }

        llvm::TargetOptions opt = buildTargetOptions(config);
        auto relocModel = getRelocationModel(config);
        auto codeModel = getCodeModel(config);
        llvm::CodeGenOptLevel optLevel = mapOptimizationLevel(config.optimization.level);

        auto tm = std::shared_ptr<llvm::TargetMachine>(
            target->createTargetMachine(
                targetTriple,
                cpu,
                features,
                opt,
                relocModel,
                codeModel,
                optLevel
            )
        );

        if (!tm) {
            logError(config, "Failed to create target machine for: " + targetTriple);
            throw std::runtime_error("Failed to create target machine");
        }

        configureTargetMachineSettings(config, tm);
        targetMachines.emplace_back(targetTriple, tm);
    }
}

llvm::TargetOptions LLVMAOTBackend::buildTargetOptions(const Config& config) {
    llvm::TargetOptions opt;
    opt.EnableFastISel = config.optimization.level < 2;
    opt.EnableGlobalISel = config.optimization.level >= 2;
    opt.UnsafeFPMath = config.optimization.fastMath;
    opt.NoInfsFPMath = config.optimization.fastMath;
    opt.NoNaNsFPMath = config.optimization.fastMath;
    opt.NoSignedZerosFPMath = config.optimization.fastMath;
    opt.GuaranteedTailCallOpt = config.optimization.enableTailCallOptimization;
    opt.ThreadModel = (config.runtime.enableParallelGC || config.runtime.gcThreads > 1) ? 
        llvm::ThreadModel::POSIX : llvm::ThreadModel::Single;

    if (config.security.enableStackProtection) {
        opt.StackAlignmentOverride = 16;
    }
    if (config.security.enableControlFlowIntegrity) {
        opt.EnableMachineOutliner = true;
    }
    if (config.security.enableAddressSanitizer) {
        opt.Sanitizers.emplace_back(llvm::SanitizerKind::Address);
    }
    if (config.security.enableMemorySanitizer) {
        opt.Sanitizers.emplace_back(llvm::SanitizerKind::Memory);
    }
    if (config.security.enableThreadSanitizer) {
        opt.Sanitizers.emplace_back(llvm::SanitizerKind::Thread);
    }
    if (config.security.enableUndefinedBehaviorSanitizer) {
        opt.Sanitizers.emplace_back(llvm::SanitizerKind::Undefined);
    }

    return opt;
}

std::optional<llvm::Reloc::Model> LLVMAOTBackend::getRelocationModel(const Config& config) {
    if (config.security.enablePositionIndependentCode || config.aot.outputFormat == OutputFormat::SharedLib) {
        return llvm::Reloc::PIC_;
    } else if (config.aot.staticLinking) {
        return llvm::Reloc::Static;
    }
    return std::nullopt;
}

std::optional<llvm::CodeModel::Model> LLVMAOTBackend::getCodeModel(const Config& config) {
    auto arch = config.resolveTargetArch();
    if (arch == TargetArch::ARM32 || arch == TargetArch::WASM32) {
        return llvm::CodeModel::Small;
    } else if (arch == TargetArch::X86_64 || arch == TargetArch::ARM64) {
        return llvm::CodeModel::Large;
    }
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

void LLVMAOTBackend::configureTargetMachineSettings(const Config& config, std::shared_ptr<llvm::TargetMachine> tm) {
    if (config.diagnostics.debugMode || config.aot.generateDebugInfo) {
        tm->setDebugInfoKind(llvm::DebugInfoKind::FullDebugInfo);
        if (config.aot.debugInfoFormat == "dwarf") {
            tm->setDwarfFormat(llvm::DwarfFormat::DWARF32);
        } else if (config.aot.debugInfoFormat == "codeview") {
            tm->setDwarfFormat(llvm::DwarfFormat::WinCodeView);
        }
    }
    if (config.diagnostics.logTimings) {
        tm->setCollectStats(true);
    }
}

void LLVMAOTBackend::setupExternalResolvers(const Config& config) {
    for (const auto& lib : config.aot.libraries) {
        irGen->registerExternalLibrary(lib, config.aot.libraryPaths);
    }
    for (const auto& framework : config.aot.frameworks) {
        irGen->registerFramework(framework, config.aot.frameworkPaths);
    }
    if (!config.environmentVariables.empty()) {
        for (const auto& [key, value] : config.environmentVariables) {
            irGen->setEnvironmentVariable(key, value);
        }
    }
}

bool LLVMAOTBackend::isLinkerAvailable(const std::string& linker, const Config& config) {
    std::string cmd = 
#ifdef _WIN32
        "where " + linker + " >nul 2>&1";
#else
        "which " + linker + " >/dev/null 2>&1";
#endif
    if (!config.toolchainPath.empty()) {
        cmd = config.toolchainPath + "/" + linker + cmd.substr(linker.length());
    }
    return std::system(cmd.c_str()) == 0;
}

void LLVMAOTBackend::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) {
    compilationStartTime = std::chrono::system_clock::now();
    initializeProfiler(config);
    executePluginCallbacks(config, "pre-execute");

    std::string validationError;
    if (!config.validate(validationError)) {
        logError(config, "Configuration validation failed: " + validationError);
        throw std::runtime_error("Configuration validation failed: " + validationError);
    }

    if (config.diagnostics.verbose) {
        config.printSummary();
    }

    if (config.incremental.enabled && isFileUpToDate(config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile)) {
        DEBUG_LOG("Skipping compilation: Source file up to date");
        executePluginCallbacks(config, "post-execute");
        finalizeProfiler(config);
        return;
    }

    scope->setName(config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile);
    irGen = std::make_shared<IRGenerator>(config);
    setupTargetMachine(config);
    setupExternalResolvers(config);

    if (config.modules.enableModules) {
        for (const auto& modPath : config.modules.modulePaths) {
            irGen->loadModule(modPath, config.modules.moduleCachePath);
        }
    }

    size_t errorCount = 0;
    for (const auto& statement : statements) {
        if (!checkTimeLimit(config) || peakMemoryUsage > config.maxMemoryUsage) {
            logError(config, "Compilation aborted: Time or memory limit exceeded");
            throw std::runtime_error("Compilation limits exceeded");
        }

        DEBUG_LOG("Evaluating " + statement->toString());
        Omniscript::setPosition(statement->getStartPosition());
        auto expr = statement->express(scope);
        if (!expr) {
            if (config.errorHandling.mode == ErrorRecoveryMode::StopOnFirst) {
                logError(config, "Expression evaluation failed for statement: " + statement->toString());
                throw std::runtime_error("Expression evaluation failed");
            }
            if (++errorCount >= config.errorHandling.maxErrorCount) {
                logError(config, "Maximum error count reached");
                throw std::runtime_error("Too many errors");
            }
            continue;
        }

        DEBUG_LOG("Generating LLVM IR for: " + expr->toString());
        irGen->codegen(expr, scope);
        trackMemoryUsage(config);
    }

    irGen->compileAllFunctionBodies(scope);
    irGen->finalizeGlobalInitializers();
    irGen->finalize();

    if (config.entry.empty()) {
        irGen->addMainFunction();
    } else {
        DEBUG_LOG("Using custom entry point: " + config.entry);
    }

    if (config.diagnostics.logFinalCode) {
        console.log("========= Unoptimized LLVM IR =========");
        irGen->printIR();
    }

    irGen->printErrors();

    if (config.optimization.level > 0) {
        for (const auto& [pass, options] : config.optimization.customPasses) {
            irGen->addCustomPass(pass, options);
        }
        irGen->optimizeModule(config.optimization.level);
    }

    if (config.diagnostics.logFinalCode && config.optimization.level > 0) {
        console.log("========= Optimized LLVM IR =========");
        irGen->printIR();
    }

    if (config.diagnostics.logAsm) {
        irGen->printAssembly(irGen->getModule().get());
    }

    linkerDependencies = irGen->getLinkDependencies();
    emitToFile(config);
    updateFileCache(config);
    executePluginCallbacks(config, "post-execute");
    finalizeProfiler(config);
}

void LLVMAOTBackend::emitToFile(const Config& config) {
    fs::path outputPath(config.outputPath);
    if (outputPath.has_parent_path()) {
        fs::create_directories(outputPath.parent_path());
    }

    for (const auto& [triple, tm] : targetMachines) {
        std::string suffix = targetMachines.size() > 1 ? "_" + triple : "";
        std::string targetOutput = outputPath.stem().string() + suffix + outputPath.extension().string();

        switch (config.aot.outputFormat) {
            case OutputFormat::Executable: {
                fs::path objPath = getTemporaryPath(config, ".o" + suffix);
                emitObjectFile(objPath.string(), tm);
                linkExecutable(objPath.string(), targetOutput, config);
                if (!config.keepIntermediateFiles) {
                    fs::remove(objPath);
                }
                break;
            }
            case OutputFormat::ObjectFile:
            case OutputFormat::Relocatable: {
                emitObjectFile(targetOutput, tm);
                break;
            }
            case OutputFormat::Assembly: {
                emitAssemblyFile(targetOutput, tm);
                break;
            }
            case OutputFormat::LLVM_IR:
            case OutputFormat::TextualIR: {
                emitLLVMIR(targetOutput);
                break;
            }
            case OutputFormat::Bitcode:
            case OutputFormat::BinaryIR: {
                emitBitcode(targetOutput);
                break;
            }
            case OutputFormat::StaticLib:
            case OutputFormat::Archive: {
                fs::path objPath = getTemporaryPath(config, ".o" + suffix);
                emitObjectFile(objPath.string(), tm);
                createStaticLibrary(objPath.string(), targetOutput);
                if (!config.keepIntermediateFiles) {
                    fs::remove(objPath);
                }
                break;
            }
            case OutputFormat::SharedLib: {
                fs::path objPath = getTemporaryPath(config, ".o" + suffix);
                emitObjectFile(objPath.string(), tm);
                createSharedLibrary(objPath.string(), targetOutput, config);
                if (!config.keepIntermediateFiles) {
                    fs::remove(objPath);
                }
                break;
            }
            case OutputFormat::MachineCode: {
                emitMachineCode(targetOutput, tm);
                break;
            }
            case OutputFormat::ModuleFile: {
                emitModuleFile(targetOutput);
                break;
            }
            case OutputFormat::PrecompiledHeader: {
                emitPrecompiledHeader(targetOutput);
                break;
            }
            case OutputFormat::WebAssembly: {
                emitWebAssembly(targetOutput);
                break;
            }
            case OutputFormat::PTX: {
                emitPTX(targetOutput);
                break;
            }
            case OutputFormat::SPIR_V: {
                emitSPIRV(targetOutput);
                break;
            }
            case OutputFormat::DebugInfo: {
                emitDebugInfo(targetOutput);
                break;
            }
            case OutputFormat::SymbolTable: {
                emitSymbolTable(targetOutput);
                break;
            }
            default:
                logError(config, "Unsupported output format");
                throw std::runtime_error("Unsupported output format");
        }
    }

    if (config.isHybridMode() && config.hybrid.enableDynamicFallback) {
        fs::path hybridOutput = config.hybrid.aotOutputPath.empty() ? 
            outputPath.stem().string() + "_hybrid.o" : config.hybrid.aotOutputPath;
        emitObjectFile(hybridOutput.string(), targetMachines[0].second);
        DEBUG_LOG("Hybrid AOT output emitted to: " + hybridOutput.string());
    }
}

void LLVMAOTBackend::emitObjectFile(const std::string& objFile, const std::shared_ptr<llvm::TargetMachine>& tm) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(objFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open output file: " + ec.message());
    }

    auto module = irGen->getModule();
    module->setTargetTriple(tm->getTargetTriple().str());
    module->setDataLayout(tm->createDataLayout());

    llvm::legacy::PassManager pass;
    if (tm->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
        throw std::runtime_error("TargetMachine can't emit an object file");
    }

    pass.run(*module);
    dest.flush();
    DEBUG_LOG("Object file emitted to: " + objFile);
}

void LLVMAOTBackend::emitAssemblyFile(const std::string& asmFile, const std::shared_ptr<llvm::TargetMachine>& tm) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(asmFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open output file: " + ec.message());
    }

    auto module = irGen->getModule();
    module->setTargetTriple(tm->getTargetTriple().str());
    module->setDataLayout(tm->createDataLayout());

    llvm::legacy::PassManager pass;
    if (tm->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_AssemblyFile)) {
        throw std::runtime_error("TargetMachine can't emit an assembly file");
    }

    pass.run(*module);
    dest.flush();
    DEBUG_LOG("Assembly file emitted to: " + asmFile);
}

void LLVMAOTBackend::emitLLVMIR(const std::string& irFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(irFile, ec, llvm::sys::fs::OF_Text);
    if (ec) {
        throw std::runtime_error("Failed to open IR output file: " + ec.message());
    }

    auto module = irGen->getModule();
    module->print(dest, nullptr);
    dest.flush();
    DEBUG_LOG("LLVM IR emitted to: " + irFile);
}

void LLVMAOTBackend::emitBitcode(const std::string& bcFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(bcFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open bitcode output file: " + ec.message());
    }

    auto module = irGen->getModule();
    llvm::WriteBitcodeToFile(*module, dest);
    dest.flush();
    DEBUG_LOG("LLVM bitcode emitted to: " + bcFile);
}

void LLVMAOTBackend::createStaticLibrary(const std::string& objFile, const std::string& libFile) {
    std::string cmd = 
#ifdef _WIN32
        "lib /OUT:" + libFile + " " + objFile;
#else
        "ar rcs " + libFile + " " + objFile;
#endif
    DEBUG_LOG("Creating static library with command: " + cmd);
    if (std::system(cmd.c_str()) != 0) {
        throw std::runtime_error("Failed to create static library: " + libFile);
    }
    DEBUG_LOG("Static library created: " + libFile);
}

void LLVMAOTBackend::createSharedLibrary(const std::string& objFile, const std::string& libFile, const Config& config) {
    std::vector<std::string> args = {"-shared", "-o", libFile, objFile};
    for (const auto& path : config.aot.libraryPaths) {
        args.push_back("-L" + path);
    }
    for (const auto& lib : config.aot.libraries) {
        args.push_back("-l" + lib);
    }
    for (const auto& flag : config.aot.linkerFlags) {
        args.push_back(flag);
    }

    std::vector<std::string> linkers = config.aot.linkerPath.empty() ? 
        std::vector<std::string>{"clang++", "g++", "ld"} : 
        std::vector<std::string>{config.aot.linkerPath};

    bool success = false;
    for (const auto& linker : linkers) {
        if (isLinkerAvailable(linker, config)) {
            std::string cmd = linker;
            for (const auto& arg : args) {
                cmd += " " + arg;
            }
            DEBUG_LOG("Creating shared library with command: " + cmd);
            if (std::system(cmd.c_str()) == 0) {
                success = true;
                break;
            }
        }
    }

    if (!success) {
        throw std::runtime_error("Failed to create shared library: " + libFile);
    }
    DEBUG_LOG("Shared library created: " + libFile);
}

void LLVMAOTBackend::emitMachineCode(const std::string& mcFile, const std::shared_ptr<llvm::TargetMachine>& tm) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(mcFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open machine code output file: " + ec.message());
    }

    auto module = irGen->getModule();
    module->setTargetTriple(tm->getTargetTriple().str());
    module->setDataLayout(tm->createDataLayout());

    llvm::legacy::PassManager pass;
    if (tm->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
        throw std::runtime_error("TargetMachine can't emit machine code");
    }

    pass.run(*module);
    dest.flush();
    DEBUG_LOG("Machine code emitted to: " + mcFile);
}

void LLVMAOTBackend::emitModuleFile(const std::string& modFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(modFile, ec, llvm::sys::fs::OF_Text);
    if (ec) {
        throw std::runtime_error("Failed to open module output file: " + ec.message());
    }

    auto module = irGen->getModule();
    dest << "; Module: " << module->getName() << "\n";
    dest << "; Target: " << module->getTargetTriple() << "\n";
    dest << "; Data Layout: " << module->getDataLayoutStr() << "\n\n";
    module->print(dest, nullptr);
    dest.flush();
    DEBUG_LOG("Module file emitted to: " + modFile);
}

void LLVMAOTBackend::emitPrecompiledHeader(const std::string& pchFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(pchFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open precompiled header output file: " + ec.message());
    }

    auto module = irGen->getModule();
    llvm::WriteBitcodeToFile(*module, dest);
    dest.flush();
    DEBUG_LOG("Precompiled header emitted to: " + pchFile);
}

void LLVMAOTBackend::emitWebAssembly(const std::string& wasmFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(wasmFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open WebAssembly output file: " + ec.message());
    }

    auto module = irGen->getModule();
    std::string wasmTriple = "wasm32-unknown-unknown";
    module->setTargetTriple(wasmTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(wasmTriple, error);
    if (!target) {
        throw std::runtime_error("WebAssembly target not available: " + error);
    }

    auto wasmTM = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(wasmTriple, "", "", {}, {})
    );
    if (!wasmTM) {
        throw std::runtime_error("Failed to create WebAssembly target machine");
    }

    module->setDataLayout(wasmTM->createDataLayout());

    llvm::legacy::PassManager pass;
    if (wasmTM->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
        throw std::runtime_error("TargetMachine can't emit WebAssembly file");
    }

    pass.run(*module);
    dest.flush();
    DEBUG_LOG("WebAssembly file emitted to: " + wasmFile);
}

void LLVMAOTBackend::emitPTX(const std::string& ptxFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(ptxFile, ec, llvm::sys::fs::OF_Text);
    if (ec) {
        throw std::runtime_error("Failed to open PTX output file: " + ec.message());
    }

    auto module = irGen->getModule();
    std::string ptxTriple = "nvptx64-nvidia-cuda";
    module->setTargetTriple(ptxTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(ptxTriple, error);
    if (!target) {
        throw std::runtime_error("NVPTX target not available: " + error);
    }

    auto ptxTM = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(ptxTriple, "", "", {}, {})
    );
    if (!ptxTM) {
        throw std::runtime_error("Failed to create NVPTX target machine");
    }

    module->setDataLayout(ptxTM->createDataLayout());

    llvm::legacy::PassManager pass;
    if (ptxTM->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_AssemblyFile)) {
        throw std::runtime_error("TargetMachine can't emit PTX file");
    }

    pass.run(*module);
    dest.flush();
    DEBUG_LOG("PTX file emitted to: " + ptxFile);
}

void LLVMAOTBackend::emitSPIRV(const std::string& spirvFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(spirvFile, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open SPIR-V output file: " + ec.message());
    }

    auto module = irGen->getModule();
    std::string spirvTriple = "spir64-unknown-unknown";
    module->setTargetTriple(spirvTriple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(spirvTriple, error);
    if (!target) {
        throw std::runtime_error("SPIR target not available: " + error);
    }

    auto spirvTM = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(spirvTriple, "", "", {}, {})
    );
    if (!spirvTM) {
        throw std::runtime_error("Failed to create SPIR target machine");
    }

    module->setDataLayout(spirvTM->createDataLayout());

    llvm::legacy::PassManager pass;
    if (spirvTM->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
        throw std::runtime_error("TargetMachine can't emit SPIR-V file");
    }

    pass.run(*module);
    dest.flush();
    DEBUG_LOG("SPIR-V file emitted to: " + spirvFile);
}

void LLVMAOTBackend::emitDebugInfo(const std::string& debugFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(debugFile, ec, llvm::sys::fs::OF_Text);
    if (ec) {
        throw std::runtime_error("Failed to open debug info output file: " + ec.message());
    }

    auto module = irGen->getModule();
    dest << "; Debug Information for Module: " << module->getName() << "\n\n";
    llvm::NamedMDNode* compileUnits = module->getNamedMetadata("llvm.dbg.cu");
    if (compileUnits) {
        dest << "; Compile Units:\n";
        for (unsigned i = 0; i < compileUnits->getNumOperands(); ++i) {
            auto* cu = compileUnits->getOperand(i);
            if (cu) {
                cu->print(dest);
                dest << "\n";
            }
        }
    }
    for (auto& namedMD : module->named_metadata()) {
        if (namedMD.getName().starts_with("llvm.dbg")) {
            dest << "; " << namedMD.getName() << ":\n";
            namedMD.print(dest);
            dest << "\n";
        }
    }
    dest.flush();
    DEBUG_LOG("Debug info emitted to: " + debugFile);
}

void LLVMAOTBackend::emitSymbolTable(const std::string& symFile) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(symFile, ec, llvm::sys::fs::OF_Text);
    if (ec) {
        throw std::runtime_error("Failed to open symbol table output file: " + ec.message());
    }

    auto module = irGen->getModule();
    dest << "# Symbol Table for Module: " << module->getName() << "\n\n";
    dest << "## Global Variables:\n";
    for (auto& global : module->globals()) {
        dest << "GLOBAL: " << global.getName();
        if (global.hasInitializer()) {
            dest << " (initialized)";
        }
        dest << " - Type: ";
        global.getType()->print(dest);
        dest << "\n";
    }
    dest << "\n## Functions:\n";
    for (auto& func : module->functions()) {
        dest << "FUNCTION: " << func.getName();
        if (func.isDeclaration()) {
            dest << " (declaration)";
        }
        dest << " - Type: ";
        func.getType()->print(dest);
        dest << "\n";
        if (!func.arg_empty()) {
            dest << "  Arguments:\n";
            for (auto& arg : func.args()) {
                dest << "    " << arg.getName() << " - Type: ";
                arg.getType()->print(dest);
                dest << "\n";
            }
        }
    }
    dest << "\n## Aliases:\n";
    for (auto& alias : module->aliases()) {
        dest << "ALIAS: " << alias.getName() << " -> ";
        if (auto* aliasee = alias.getAliasee()) {
            dest << aliasee->getName();
        }
        dest << "\n";
    }
    dest.flush();
    DEBUG_LOG("Symbol table emitted to: " + symFile);
}

void LLVMAOTBackend::linkExecutable(const std::string& objFile, const std::string& exeFile, const Config& config) {
    std::vector<std::string> additionalLibs = linkerDependencies.getLinkerFlags();
    std::vector<std::string> defaultLibs = 
#ifdef _WIN32
        {"user32.lib", "gdi32.lib", "shell32.lib", "kernel32.lib", "ntdll.lib"};
#else
        {"-lm", "-ldl", "-lpthread"};
#endif

    std::vector<std::pair<std::string, std::vector<std::string>>> linkerOptions = {
#ifdef _WIN32
        {"clang++", buildLinkerArgs(exeFile, objFile, additionalLibs, defaultLibs, config)},
        {"g++", buildLinkerArgs(exeFile, objFile, additionalLibs, defaultLibs, config)},
        {"link", buildMSVCLinkerArgs(exeFile, objFile, additionalLibs, defaultLibs, config)}
#else
        {"clang++", buildLinkerArgs(exeFile, objFile, additionalLibs, defaultLibs, config)},
        {"g++", buildLinkerArgs(exeFile, objFile, additionalLibs, defaultLibs, config)}
#endif
    };

    if (!config.aot.linkerPath.empty()) {
        linkerOptions.insert(linkerOptions.begin(), 
            {config.aot.linkerPath, buildLinkerArgs(exeFile, objFile, additionalLibs, defaultLibs, config)});
    }

    bool linked = false;
    std::vector<std::string> cmds, errors, triedLinkers;

    for (const auto& [linker, args] : linkerOptions) {
        if (!isLinkerAvailable(linker, config)) continue;

        std::string cmd = linker;
        for (const auto& arg : args) {
            cmd += " " + arg;
        }
        DEBUG_LOG("Trying linker: " + cmd);
        cmds.push_back(cmd);
        triedLinkers.push_back(linker);

        int result = std::system(cmd.c_str());
        if (result == 0) {
            linked = true;
            break;
        } else {
            std::ostringstream errorStream;
            errorStream << "Linker `" << linker << "` failed with exit code " << result;
            errors.push_back(errorStream.str());
            DEBUG_LOG(errorStream.str());
        }
    }

    if (!linked) {
        std::ostringstream report;
        report << "Linking failed.\nAvailable linkers: ";
        for (const auto& linker : triedLinkers) {
            report << linker << " ";
        }
        report << "\n\n";
        for (size_t i = 0; i < triedLinkers.size(); ++i) {
            report << "Linker: " << triedLinkers[i] << "\n"
                   << "Command: " << cmds[i] << "\n"
                   << "Error: " << errors[i] << "\n\n";
        }
        logError(config, report.str());
        throw std::runtime_error("Linking failed");
    }

#ifndef _WIN32
    fs::permissions(exeFile, 
        fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
        fs::perm_options::add);
#endif
    DEBUG_LOG("Executable created: " + exeFile);
}

std::vector<std::string> LLVMAOTBackend::buildLinkerArgs(
    const std::string& exeFile, 
    const std::string& objFile,
    const std::vector<std::string>& additionalLibs,
    const std::vector<std::string>& defaultLibs,
    const Config& config) {
    std::vector<std::string> args = {"-o", exeFile, objFile};
    for (const auto& lib : additionalLibs) {
        args.push_back(lib);
    }
    for (const auto& lib : defaultLibs) {
        args.push_back(lib);
    }
    for (const auto& path : config.aot.libraryPaths) {
        args.push_back("-L" + path);
    }
    for (const auto& flag : config.aot.linkerFlags) {
        args.push_back(flag);
    }
    return args;
}

std::vector<std::string> LLVMAOTBackend::buildMSVCLinkerArgs(
    const std::string& exeFile,
    const std::string& objFile,
    const std::vector<std::string>& additionalLibs,
    const std::vector<std::string>& defaultLibs,
    const Config& config) {
    std::vector<std::string> args = {"/OUT:" + exeFile, objFile};
    for (const auto& lib : additionalLibs) {
        args.push_back(lib.starts_with("-l") ? lib.substr(2) + ".lib" : lib);
    }
    for (const auto& lib : defaultLibs) {
        args.push_back(lib);
    }
    for (const auto& path : config.aot.libraryPaths) {
        args.push_back("/LIBPATH:" + path);
    }
    return args;
}

fs::path LLVMAOTBackend::getTemporaryPath(const Config& config, const std::string& extension) {
    fs::path basePath = !config.tempDirectory.empty() ? 
        fs::path(config.tempDirectory) : 
        fs::path(config.outputPath).parent_path();
    return basePath / (fs::path(config.outputPath).stem().string() + extension);
}

void LLVMAOTBackend::updateFileCache(const Config& config) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto sourceFile = config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile;
    if (!sourceFile.empty()) {
        try {
            fileCache[sourceFile] = fs::last_write_time(sourceFile);
        } catch (const fs::filesystem_error& e) {
            DEBUG_LOG("Failed to update file cache: " + std::string(e.what()));
        }
    }
    for (const auto& path : config.sourcePaths) {
        try {
            fileCache[path] = fs::last_write_time(path);
        } catch (const fs::filesystem_error& e) {
            DEBUG_LOG("Failed to update file cache for " + path + ": " + std::string(e.what()));
        }
    }
}

bool LLVMAOTBackend::isFileUpToDate(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = fileCache.find(filePath);
    if (it == fileCache.end()) {
        return false;
    }
    try {
        return fs::last_write_time(filePath) <= it->second;
    } catch (const fs::filesystem_error& e) {
        DEBUG_LOG("Failed to check file timestamp: " + std::string(e.what()));
        return false;
    }
}

void LLVMAOTBackend::initializeProfiler(const Config& config) {
    if (config.profiler.type == ProfilerType::None || profilerInitialized) {
        return;
    }
    switch (config.profiler.type) {
        case ProfilerType::GProf:
            // Initialize gprof (e.g., set GMON_OUT_PREFIX)
            setenv("GMON_OUT_PREFIX", config.profiler.outputPath.c_str(), 1);
            break;
        case ProfilerType::Perf:
            // Launch perf in background if sampling enabled
            if (config.profiler.enableSampling) {
                std::string cmd = "perf record -F " + std::to_string(config.profiler.samplingFrequency) + 
                                  " -o " + config.profiler.outputPath + " &";
                std::system(cmd.c_str());
            }
            break;
        case ProfilerType::VTune:
        case ProfilerType::Custom:
            // Placeholder for custom profiler initialization
            break;
        default:
            break;
    }
    profilerInitialized = true;
    DEBUG_LOG("Profiler initialized: " + config.profilerTypeToString(config.profiler.type));
}

void LLVMAOTBackend::finalizeProfiler(const Config& config) {
    if (config.profiler.type == ProfilerType::None || !profilerInitialized) {
        return;
    }
    switch (config.profiler.type) {
        case ProfilerType::Perf:
            // Stop perf recording
            std::system("pkill -SIGINT perf");
            break;
        default:
            break;
    }
    profilerInitialized = false;
    DEBUG_LOG("Profiler finalized");
}

void LLVMAOTBackend::executePluginCallbacks(const Config& config, const std::string& stage) {
    for (const auto& plugin : config.plugins) {
        if (plugin.callback) {
            try {
                plugin.callback(config, const_cast<void*>(static_cast<const void*>(&stage)));
            } catch (const std::exception& e) {
                logError(config, "Plugin " + plugin.name + " failed at stage " + stage + ": " + e.what());
            }
        }
    }
}

void LLVMAOTBackend::trackMemoryUsage(const Config& config) {
    size_t currentUsage = 0;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS memInfo;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memInfo, sizeof(memInfo))) {
        currentUsage = memInfo.PeakWorkingSetSize;
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        currentUsage = usage.ru_maxrss * 1024; // Convert KB to bytes
    }
#endif
    peakMemoryUsage = std::max(peakMemoryUsage, currentUsage);
    if (config.diagnostics.measureMemoryUsage) {
        DEBUG_LOG("Current memory usage: " + std::to_string(currentUsage / 1024 / 1024) + " MB");
    }
}

bool LLVMAOTBackend::checkTimeLimit(const Config& config) {
    if (config.maxCompilationTime == 0) {
        return true;
    }
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - compilationStartTime).count();
    return elapsed < static_cast<long>(config.maxCompilationTime);
}

void LLVMAOTBackend::logError(const Config& config, const std::string& message) {
    console.error(message);
    if (config.errorHandling.logToFile && !config.errorHandling.errorLogPath.empty()) {
        std::ofstream logFile(config.errorHandling.errorLogPath, std::ios::app);
        if (logFile.is_open()) {
            logFile << "[" << std::time(nullptr) << "] ERROR: " << message << "\n";
            logFile.close();
        }
    }
}

} // namespace Omniscript
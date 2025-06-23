#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Backends/llvm/LLVMAOTBackend.h>

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

namespace fs = std::filesystem;

LLVMAOTBackend::LLVMAOTBackend() {    
    scope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>();
}

void LLVMAOTBackend::initialize() {
    
}

void LLVMAOTBackend::setupTargetMachine(const Config& config) {
    
    std::string targetTriple = config.getEffectiveTargetTriple();
    std::string cpu = irGen->resolveCPUName((config.cpuFeatures == "native" && !config.isCrossCompilation()) ? 
                      "native" : config.getDefaultCPU());
    
    
    std::string features = irGen->buildFeatureString(targetTriple);
    
    DEBUG_LOG("Target triple: " + targetTriple);
    DEBUG_LOG("CPU: " + cpu);
    DEBUG_LOG("Features: " + features);
    
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        throw std::runtime_error("Target lookup failed for '" + targetTriple + "': " + error);
    }

    llvm::TargetOptions opt;// = buildTargetOptions(config);
    
    // std::optional<llvm::Reloc::Model> relocModel = getRelocationModel(config);
    auto relocModel = std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    
    std::optional<llvm::CodeModel::Model> codeModel = getCodeModel(config);
    
    llvm::CodeGenOptLevel optLevel = mapOptimizationLevel(config.optimization.level);
    
    targetMachine = std::shared_ptr<llvm::TargetMachine>(
        target->createTargetMachine(
            targetTriple,
            cpu,
            features,
            opt,
            relocModel,
            // std::nullopt,
            // codeModel,
            std::nullopt,
            optLevel
        )
    );
    
    if (!targetMachine) {
        throw std::runtime_error("Failed to create target machine for: " + targetTriple);
    }
    
    configureTargetMachineSettings(config);
}

llvm::TargetOptions LLVMAOTBackend::buildTargetOptions(const Config& config) {
    llvm::TargetOptions opt;
    
    opt.EnableFastISel = config.optimization.level < 2;
    opt.EnableGlobalISel = config.optimization.level >= 2;
    
    if (config.security.enableStackProtection) {
        // opt.StackProtectorGuard = llvm::StackProtectorGuards::TLS;
    }
    
    if (config.security.enableControlFlowIntegrity) {
        // opt.CFIntegrity = true;
    }
    
    opt.UnsafeFPMath = config.optimization.fastMath;
    opt.NoInfsFPMath = config.optimization.fastMath;
    opt.NoNaNsFPMath = config.optimization.fastMath;
    opt.NoSignedZerosFPMath = config.optimization.fastMath;
    
    opt.GuaranteedTailCallOpt = config.optimization.enableTailCallOptimization;
       
    if (config.runtime.enableParallelGC || config.runtime.gcThreads > 1) {
        opt.ThreadModel = llvm::ThreadModel::POSIX;
    } else {
        opt.ThreadModel = llvm::ThreadModel::Single;
    }
    
    return opt;
}

std::optional<llvm::Reloc::Model> LLVMAOTBackend::getRelocationModel(const Config& config) {
    if (config.security.enablePositionIndependentCode ||
        config.aot.outputFormat == OutputFormat::SharedLib) {
        return llvm::Reloc::PIC_;
    } else if (config.aot.staticLinking) {
        return llvm::Reloc::Static;
    }
    
    return std::nullopt; 
}

std::optional<llvm::CodeModel::Model> LLVMAOTBackend::getCodeModel(const Config& config) { 
    if (config.resolveTargetArch() == TargetArch::ARM32 || 
        config.resolveTargetArch() == TargetArch::WASM32) {
        return llvm::CodeModel::Small;
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

void LLVMAOTBackend::configureTargetMachineSettings(const Config& config) {   
    if (config.diagnostics.debugMode || config.aot.generateDebugInfo) {
             
    }
    
    
    if (config.diagnostics.debugMode) {
        // creates an error unable to translate function call always
        // targetMachine->setO0WantsFastISel(true);
    }
}

void LLVMAOTBackend::setupExternalResolvers(const Config& config) {
    auto targetOS = config.resolveTargetOS();
    auto targetArch = config.resolveTargetArch();
    
    DEBUG_LOG("Setting up external resolver for target: " + 
              config.getTargetSummary());
    
    
    irGen->addExternalResolver("C", std::make_unique<CStdLibResolver>());
    
    
    switch (targetOS) {
        case TargetOS::Windows:
            irGen->addExternalResolver("kernel32", std::make_unique<WindowsAPIResolver>("kernel32"));
            irGen->addExternalResolver("user32",   std::make_unique<WindowsAPIResolver>("user32"));
            irGen->addExternalResolver("gdi32",    std::make_unique<WindowsAPIResolver>("gdi32"));
            irGen->addExternalResolver("shell32",  std::make_unique<WindowsAPIResolver>("shell32"));
            irGen->addExternalResolver("ntdll",    std::make_unique<WindowsAPIResolver>("ntdll"));
            irGen->addExternalResolver("msvcrt",   std::make_unique<WindowsAPIResolver>("msvcrt"));
            break;
            
        case TargetOS::Linux:
        case TargetOS::FreeBSD:
            irGen->addExternalResolver("libc", std::make_unique<PosixResolver>());
            irGen->addExternalResolver("libm", std::make_unique<PosixResolver>());
            irGen->addExternalResolver("libdl", std::make_unique<PosixResolver>());
            irGen->addExternalResolver("libpthread", std::make_unique<PosixResolver>());
            break;
            
        case TargetOS::MacOS:
        case TargetOS::iOS:
            irGen->addExternalResolver("libSystem", std::make_unique<DarwinResolver>());
            irGen->addExternalResolver("Foundation", std::make_unique<DarwinResolver>());
            break;
            
        case TargetOS::Android:
            irGen->addExternalResolver("libc", std::make_unique<AndroidResolver>());
            irGen->addExternalResolver("libm", std::make_unique<AndroidResolver>());
            break;
            
        case TargetOS::WebAssembly:
            irGen->addExternalResolver("wasm", std::make_unique<WebAssemblyResolver>());
            break;
            
        default:
            DEBUG_LOG("Warning: Unknown target OS, using generic resolvers only");
            break;
    }
    
    
    for (const auto& libPath : config.aot.libraryPaths) {
        irGen->addExternalResolver("static_lib", 
            std::make_unique<StaticLibraryResolver>(libPath));
    }
    
    
    for (const auto& lib : config.aot.libraries) {
        irGen->addExternalResolver("dynamic_lib", 
            std::make_unique<DynamicLibraryResolver>(lib));
    }
    
    DEBUG_LOG("External function resolver setup completed");
}

bool LLVMAOTBackend::isLinkerAvailable(const std::string& linker) {
    std::string cmd = 
    #ifdef _WIN32
        "where " + linker + " >nul 2>&1";
    #else
        "which " + linker + " >/dev/null 2>&1";
    #endif
    
    return std::system(cmd.c_str()) == 0;
}

void LLVMAOTBackend::execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) {
    DEBUG_LOG();
    DEBUG_LOG("Executing with LLVM AOT Backend");
    DEBUG_LOG("==============================");
    
    
    std::string validationError;
    if (!config.validate(validationError)) {
        throw std::runtime_error("Configuration validation failed: " + validationError);
    }
    
    if (config.diagnostics.verbose) {
        config.printSummary();
    }

    scope->setName(config.filePath);
    irGen = std::make_shared<IRGenerator>(config);
    
    setupTargetMachine(config);
    
    setupExternalResolvers(config);

    DEBUG_LOG("Evaluating statements");
    DEBUG_LOG("====================");

    for (const auto& statement : statements) {
        DEBUG_LOG();
        DEBUG_LOG("Evaluating " + statement->toString());
        Omniscript::setPosition(statement->getPosition());
        auto expr = statement->express(scope);
        if (!expr) continue;

        DEBUG_LOG("Generating LLVM IR for: " + expr->toString());
        irGen->codegen(expr, scope);
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

    
    irGen->optimizeModule(config.optimization.level);

    if (config.diagnostics.logFinalCode && config.optimization.level > -1) {
        console.log();
        console.log("========= Optimized LLVM IR =========");
        irGen->printIR();
    }

    if (config.diagnostics.logAsm) {
        DEBUG_LOG();
        irGen->printAssembly(irGen->getModule().get());
    }

    linkerDependencies = irGen->getLinkDependencies();

    
    handleOutput(config);
}

void LLVMAOTBackend::handleOutput(const Config& config) {
    fs::path outputPath(config.outputPath);
    
    switch (config.aot.outputFormat) {
        case OutputFormat::Executable: {
            fs::path objPath = getTemporaryPath(config, ".o");
            emitObjectFile(objPath.string());
            linkExecutable(objPath.string(), config.outputPath);
            
            if (!config.keepIntermediateFiles) {
                fs::remove(objPath);
            }
            break;
        }
        
        case OutputFormat::ObjectFile: {
            emitObjectFile(config.outputPath);
            break;
        }
        
        case OutputFormat::Assembly: {
            emitAssemblyFile(config.outputPath);
            break;
        }
        
        case OutputFormat::LLVM_IR: {
            emitLLVMIR(config.outputPath);
            break;
        }
        
        case OutputFormat::Bitcode: {
            emitBitcode(config.outputPath);
            break;
        }
        
        case OutputFormat::StaticLib: {
            fs::path objPath = getTemporaryPath(config, ".o");
            emitObjectFile(objPath.string());
            createStaticLibrary(objPath.string(), config.outputPath);
            
            if (!config.keepIntermediateFiles) {
                fs::remove(objPath);
            }
            break;
        }
        
        case OutputFormat::SharedLib: {
            fs::path objPath = getTemporaryPath(config, ".o");
            emitObjectFile(objPath.string());
            createSharedLibrary(objPath.string(), config.outputPath, config);
            
            if (!config.keepIntermediateFiles) {
                fs::remove(objPath);
            }
            break;
        }
    }
}

fs::path LLVMAOTBackend::getTemporaryPath(const Config& config, const std::string& extension) {
    fs::path basePath;
    
    if (!config.tempDirectory.empty()) {
        basePath = fs::path(config.tempDirectory);
    } else {
        basePath = fs::path(config.outputPath).parent_path();
    }
    
    return basePath / (fs::path(config.outputPath).stem().string() + extension);
}

void LLVMAOTBackend::emitToFile(const std::string& filename) {
    fs::path path(filename);
    std::string ext = path.extension().string();
    
    if (ext == ".o" || ext == ".obj") {
        emitObjectFile(filename);
    } 
    else if (ext == ".s" || ext == ".asm") {
        emitAssemblyFile(filename);
    }
    else if (ext == ".ll") {
        emitLLVMIR(filename);
    }
    else if (ext == ".bc") {
        emitBitcode(filename);
    }
    else {
        fs::path objPath = path.parent_path() / (path.stem().string() + ".o");
        emitObjectFile(objPath.string());
    }
}

void LLVMAOTBackend::emitObjectFile(const std::string& objFilename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(objFilename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open output file: " + ec.message());
    }

    auto module = irGen->getModule();
    module->setTargetTriple(targetMachine->getTargetTriple().str());
    module->setDataLayout(targetMachine->createDataLayout());

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        throw std::runtime_error("TargetMachine can't emit an object file.");
    }

    pass.run(*module);
    dest.flush();

    DEBUG_LOG("Object file emitted to: " + objFilename);
}

void LLVMAOTBackend::emitAssemblyFile(const std::string& asmFilename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(asmFilename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open output file: " + ec.message());
    }

    auto module = irGen->getModule();
    module->setTargetTriple(targetMachine->getTargetTriple().str());
    module->setDataLayout(targetMachine->createDataLayout());

    llvm::legacy::PassManager pass;
    auto fileType = llvm::CodeGenFileType::AssemblyFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        throw std::runtime_error("TargetMachine can't emit an assembly file.");
    }

    pass.run(*module);
    dest.flush();

    DEBUG_LOG("Assembly file emitted to: " + asmFilename);
}

void LLVMAOTBackend::emitLLVMIR(const std::string& irFilename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(irFilename, ec, llvm::sys::fs::OF_Text);
    if (ec) {
        throw std::runtime_error("Failed to open IR output file: " + ec.message());
    }

    auto module = irGen->getModule();
    module->print(dest, nullptr);
    dest.flush();

    DEBUG_LOG("LLVM IR emitted to: " + irFilename);
}

void LLVMAOTBackend::emitBitcode(const std::string& bcFilename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(bcFilename, ec, llvm::sys::fs::OF_None);
    if (ec) {
        throw std::runtime_error("Failed to open bitcode output file: " + ec.message());
    }

    auto module = irGen->getModule();
    llvm::WriteBitcodeToFile(*module, dest);
    dest.flush();

    DEBUG_LOG("LLVM bitcode emitted to: " + bcFilename);
}

void LLVMAOTBackend::createStaticLibrary(const std::string& objFile, const std::string& libFile) {
    std::string cmd;
    
#ifdef _WIN32
    cmd = "lib /OUT:" + libFile + " " + objFile;
#else
    cmd = "ar rcs " + libFile + " " + objFile;
#endif
    
    DEBUG_LOG("Creating static library with command: " + cmd);
    
    int result = std::system(cmd.c_str());
    if (result != 0) {
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
    
    
    std::vector<std::string> linkers = {"clang++", "g++", "ld"};
    
    bool success = false;
    for (const auto& linker : linkers) {
        if (isLinkerAvailable(linker)) {
            std::string cmd = linker;
            for (const auto& arg : args) {
                cmd += " " + arg;
            }
            
            DEBUG_LOG("Creating shared library with command: " + cmd);
            
            int result = std::system(cmd.c_str());
            if (result == 0) {
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

void LLVMAOTBackend::linkExecutable(const std::string& objFile, const std::string& exeFile) {
    std::vector<std::string> additionalLibs = linkerDependencies.getLinkerFlags();
    
    std::vector<std::pair<std::string, std::vector<std::string>>> linkerOptions = {
#ifdef _WIN32
        {"clang++", buildLinkerArgs(exeFile, objFile, additionalLibs, {
            "-luser32", "-lgdi32", "-lshell32", "-lkernel32", "-lntdll"
        })},
        {"g++", buildLinkerArgs(exeFile, objFile, additionalLibs, {
            "-luser32", "-lgdi32", "-lshell32", "-lkernel32", "-lntdll"
        })},
        {"link", buildMSVCLinkerArgs(exeFile, objFile, additionalLibs, {
            "user32.lib", "gdi32.lib", "shell32.lib", "kernel32.lib", "ntdll.lib"
        })}
#else
        {"clang++", buildLinkerArgs(exeFile, objFile, additionalLibs, {
            "-lm", "-ldl", "-lpthread"
        })},
        {"g++", buildLinkerArgs(exeFile, objFile, additionalLibs, {
            "-lm", "-ldl", "-lpthread"
        })}
#endif
    };

    bool linked = false;
    std::string cmd;
    std::string lastError;

    for (const auto& [linker, args] : linkerOptions) {
        if (isLinkerAvailable(linker)) {
            cmd = linker;
            for (const auto& arg : args) {
                cmd += " " + arg;
            }
            DEBUG_LOG("Linking with command: " + cmd);
            
            int result = std::system(cmd.c_str());
            if (result == 0) {
                linked = true;
                break;
            } else {
                lastError = "Linker " + linker + " failed with exit code: " + std::to_string(result);
                DEBUG_LOG(lastError);
            }
        }
    }

    if (!linked) {
        std::string availableLinkers;
        for (const auto& [linker, _] : linkerOptions) {
            if (isLinkerAvailable(linker)) {
                availableLinkers += linker + " ";
            }
        }
        
        throw std::runtime_error(
            "Linking failed. Available linkers: " + availableLinkers + 
            ". Last error: " + lastError
        );
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
    const std::vector<std::string>& defaultLibs) {
    
    std::vector<std::string> args = {"-o", exeFile, objFile};
    
    for (const auto& lib : additionalLibs) {
        args.push_back(lib);
    }
    
    for (const auto& lib : defaultLibs) {
        args.push_back(lib);
    }
    
    return args;
}

std::vector<std::string> LLVMAOTBackend::buildMSVCLinkerArgs(
    const std::string& exeFile,
    const std::string& objFile,
    const std::vector<std::string>& additionalLibs,
    const std::vector<std::string>& defaultLibs) {
    
    std::vector<std::string> args = {"/OUT:" + exeFile, objFile};
    
    for (const auto& lib : additionalLibs) {
        if (lib.starts_with("-l")) {
            args.push_back(lib.substr(2) + ".lib");
        } else {
            args.push_back(lib);
        }
    }
    
    for (const auto& lib : defaultLibs) {
        args.push_back(lib);
    }
    
    return args;
}
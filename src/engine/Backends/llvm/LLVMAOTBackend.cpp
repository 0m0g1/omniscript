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
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();

    std::string triple = llvm::sys::getDefaultTargetTriple();
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);
    if (!target) {
        throw std::runtime_error("Target lookup failed: " + error);
    }

    llvm::TargetOptions opt;
    auto relocModel = std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    targetMachine = std::shared_ptr<llvm::TargetMachine>(
        target->createTargetMachine(
            triple, 
            "generic", 
            "", 
            opt, 
            relocModel,
            std::nullopt,
            llvm::CodeGenOptLevel::Aggressive
        )
    );

    // Add these target machine configurations
    targetMachine->setO0WantsFastISel(true);  // Better debug at O0
    targetMachine->setGlobalISel(true);       // Enable global instruction selector
    
    scope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>();
}

void LLVMAOTBackend::initialize() {
    // Initialize platform-specific external function resolver
    
}

void LLVMAOTBackend::setupExternalResolvers() {
    // Create a smart platform resolver that automatically detects the current platform
    // and sets up appropriate resolvers for C standard library, system APIs, etc.
    
    auto platform = PlatformInfo::getCurrentPlatform();
    auto arch = PlatformInfo::getCurrentArchitecture();
    
    DEBUG_LOG("Setting up external resolver for platform: " + 
              PlatformInfo::getPlatformString() + " (" + PlatformInfo::getArchString() + ")");
    
    // Set up C standard library resolver (universal)
    irGen->addExternalResolver("C", std::make_unique<CStdLibResolver>());
    
    // Set up platform-specific system API resolvers
    switch (platform) {
#ifdef _WIN32
        case PlatformInfo::Platform::Windows:
            irGen->addExternalResolver("msvcrt", std::make_unique<WindowsAPIResolver>());
            irGen->addExternalResolver("kernel32", std::make_unique<WindowsAPIResolver>());
            irGen->addExternalResolver("user32", std::make_unique<WindowsAPIResolver>());
            irGen->addExternalResolver("gdi32", std::make_unique<WindowsAPIResolver>());
            irGen->addExternalResolver("shell32", std::make_unique<WindowsAPIResolver>());
            irGen->addExternalResolver("ntdll", std::make_unique<WindowsAPIResolver>());
            break;
#else
        case PlatformInfo::Platform::Linux:
        case PlatformInfo::Platform::FreeBSD:
            irGen->addExternalResolver("libc", std::make_unique<POSIXResolver>());
            irGen->addExternalResolver("libc", std::make_unique<POSIXResolver>());
            irGen->addExternalResolver("libm", std::make_unique<POSIXResolver>());
            irGen->addExternalResolver("libdl", std::make_unique<POSIXResolver>());
            irGen->addExternalResolver("libpthread", std::make_unique<POSIXResolver>());
            break;
        case PlatformInfo::Platform::MacOS:
            irGen->addExternalResolver("libSystem", std::make_unique<DarwinResolver>());
            irGen->addExternalResolver("Foundation", std::make_unique<DarwinResolver>());
            break;
#endif
        default:
            DEBUG_LOG("Warning: Unknown platform, using generic resolvers only");
            break;
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

    scope->setName(config.filePath);
    irGen = std::make_shared<IRGenerator>(config);
    
    setupExternalResolvers();

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
    }
    if (config.logFinalCode) {
        console.log("========= Unoptimized LLVM IR =========");
        irGen->printIR();
    }

    irGen->printErrors();

    irGen->optimizeModule(config.optimizationLevel);

    if (config.logFinalCode && config.optimizationLevel > -1) {
        console.log();
        console.log("========= Optimized LLVM IR =========");
        irGen->printIR();
    }

    if (config.logAsm) {
        DEBUG_LOG();
        irGen->printAssembly(irGen->getModule().get());
    }

    linkerDependencies = irGen->getLinkDependencies();

    fs::path outputPath(config.outputPath);
    fs::path objPath = outputPath.parent_path() / (outputPath.stem().string() + ".o");
    
    emitObjectFile(objPath.string());
    linkExecutable(objPath.string(), config.outputPath);
    
    // Clean up temporary object file if requested
    if (!config.keepIntermediateFiles) {
        std::error_code ec;
        if (!fs::remove(objPath, ec)) {
            DEBUG_LOG("Warning: Could not remove temporary object file: " + ec.message());
        }
    }
}

void LLVMAOTBackend::emitToFile(const std::string& filename) {
    // Decide what to emit based on file extension
    fs::path path(filename);
    std::string ext = path.extension().string();
    
    if (ext == ".o" || ext == ".obj") {
        emitObjectFile(filename);
    } 
    else if (ext == ".s" || ext == ".asm") {
        emitAssemblyFile(filename);
    }
    else {
        // Default to object file but with different name
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

void LLVMAOTBackend::linkExecutable(const std::string& objFile, const std::string& exeFile) {
    // Get the required libraries from the IR generator's link dependencies
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
    
    // Add additional libraries from dependency tracking
    for (const auto& lib : additionalLibs) {
        args.push_back(lib);
    }
    
    // Add default system libraries
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
    
    // Add additional libraries from dependency tracking
    for (const auto& lib : additionalLibs) {
        // Convert Unix-style flags to MSVC style if needed
        if (lib.starts_with("-l")) {
            args.push_back(lib.substr(2) + ".lib");
        } else {
            args.push_back(lib);
        }
    }
    
    // Add default system libraries
    for (const auto& lib : defaultLibs) {
        args.push_back(lib);
    }
    
    return args;
}
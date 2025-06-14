#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Backends/llvm/LLVMAOTBackend.h>
#include <cstdlib>
#include <filesystem>

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
    // Optional target machine specific initialization
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
    irGen = std::make_shared<IRGenerator>(config.filePath);

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
    irGen->optimizeModule(config.optimizationLevel);

    if (config.logFinalCode) {
        DEBUG_LOG();
        irGen->printIR();
        irGen->printErrors();
    }

    if (config.logAsm) {
        DEBUG_LOG();
        irGen->printAssembly(irGen->getModule().get());
    }

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
        emitAssemblyFile(filename);  // You might want to implement this
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
    std::vector<std::pair<std::string, std::vector<std::string>>> linkerOptions = {
#ifdef _WIN32
        {"clang++", {
            "-o", exeFile, 
            objFile,
            "-luser32",
            "-lgdi32",
            "-lshell32"
        }},
        {"g++", {
            "-o", exeFile,
            objFile,
            "-luser32",
            "-lgdi32",
            "-lshell32"
        }}
#else
        {"clang++", {
            "-o", exeFile,
            objFile,
            "-lm",
            "-ldl",
            "-lpthread"
        }},
        {"g++", {
            "-o", exeFile,
            objFile,
            "-lm", 
            "-ldl",
            "-lpthread"
        }}
#endif
    };

    bool linked = false;
    std::string cmd;

    for (const auto& [linker, args] : linkerOptions) {
        if (isLinkerAvailable(linker)) {
            cmd = linker;
            for (const auto& arg : args) {
                cmd += " " + arg;
            }
            DEBUG_LOG("Linking with command: " + cmd);
            linked = true;
            break;
        }
    }

    if (!linked) {
        throw std::runtime_error("No suitable linker found");
    }

    int result = std::system(cmd.c_str());
    if (result != 0) {
        throw std::runtime_error("Linking failed with exit code: " + std::to_string(result));
    }

#ifndef _WIN32
    fs::permissions(exeFile, 
        fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
        fs::perm_options::add);
#endif

    DEBUG_LOG("Executable created: " + exeFile);
}


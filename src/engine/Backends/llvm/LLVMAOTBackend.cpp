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
    auto relocModel = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    targetMachine = std::shared_ptr<llvm::TargetMachine>(
        target->createTargetMachine(
            triple, 
            "generic", 
            "", 
            opt, 
            relocModel,
            llvm::None,
            llvm::CodeGenOpt::Aggressive
        )
    );

    scope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>();
}

void LLVMAOTBackend::initialize() {
    // Optional target machine specific initialization
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
        fs::remove(objPath);
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
    auto fileType = llvm::CGFT_ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        throw std::runtime_error("TargetMachine can't emit an object file.");
    }

    pass.run(*module);
    dest.flush();

    DEBUG_LOG("Object file emitted to: " + objFilename);
}

void LLVMAOTBackend::linkExecutable(const std::string& objFile, const std::string& exeFile) {
    std::string linker;
    std::vector<std::string> linkerArgs;
    
    // Detect system linker
    #ifdef _WIN32
        linker = "clang";
        linkerArgs = {objFile, "-o", exeFile};
    #else
        linker = "clang++";
        linkerArgs = {objFile, "-o", exeFile, "-lm", "-ldl", "-lpthread"};
    #endif

        // Build command line
        std::string cmd = linker;
        for (const auto& arg : linkerArgs) {
            cmd += " " + arg;
        }

        DEBUG_LOG("Linking with command: " + cmd);
        
        // Execute linking command
        int result = std::system(cmd.c_str());
        if (result != 0) {
            throw std::runtime_error("Linking failed with exit code: " + std::to_string(result));
        }
        
        // Make executable on Unix-like systems
    #ifndef _WIN32
        fs::permissions(exeFile, 
            fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
            fs::perm_options::add);
    #endif

    DEBUG_LOG("Executable created: " + exeFile);
}
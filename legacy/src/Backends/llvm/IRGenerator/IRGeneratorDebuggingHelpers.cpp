#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <llvm/IR/Verifier.h>

namespace Omniscript {
void IRGenerator::printAssembly(llvm::Module* module) {
    // Initialize targets
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        llvm::errs() << "Error: " << error << "\n";
        return;
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();
    std::unique_ptr<llvm::TargetMachine> targetMachine(
        target->createTargetMachine(targetTriple, cpu, features, opt, RM));

    module->setDataLayout(targetMachine->createDataLayout());
    module->setTargetTriple(targetTriple);

    // Verify module before emission
    if (llvm::verifyModule(*module, &llvm::errs())) {
        llvm::errs() << "LLVM module verification failed!\n";
        module->print(llvm::errs(), nullptr);
        return;
    }

    // Create the pass manager
    llvm::legacy::PassManager pass;
    llvm::SmallVector<char, 0> asmOutput;
    llvm::raw_svector_ostream outStream(asmOutput);

    // Configure for assembly output
    if (targetMachine->addPassesToEmitFile(pass, outStream, nullptr, 
                                          llvm::CodeGenFileType::AssemblyFile)) {
        llvm::errs() << "TargetMachine can't emit a file of this type\n";
        return;
    }

    // Run the passes
    pass.run(*module);

    // Print the assembly
    llvm::outs().write(asmOutput.data(), asmOutput.size());
    llvm::outs().flush();
}

std::string IRGenerator::debugType(llvm::Type* type) {
    std::string str;
    llvm::raw_string_ostream rso(str);
    type->print(rso);
    return rso.str();
}

void IRGenerator::printIR() {
    Module->print(llvm::outs(), nullptr);
}

void IRGenerator::printErrors() {
    std::string errorStr;
    llvm::raw_string_ostream errorStream(errorStr);

    if (llvm::verifyModule(*Module, &errorStream)) {
        errorStream.flush();
        llvm::errs() << "Module verification for '" << Module->getModuleIdentifier() << "' failed!\n";
        llvm::errs() << errorStr << "\n";
    }
}

void IRGenerator::printErrors(llvm::Module& module) {
    if (llvm::verifyModule(module, &llvm::errs())) {
        llvm::errs() << "Module verification for '" << module.getModuleIdentifier() << " failed! '\n";
        module.print(llvm::outs(), nullptr);
    } 
    // else {
    //     llvm::errs() << "No errors found in: '" << module.getModuleIdentifier() << "'.\n";
    // }
}

void IRGenerator::setupDebugInfo() {
    DEBUG_LOG("Setting up debugging info, (does nothing for now)");
    // if (!configs.aot.generateDebugInfo && !configs.diagnostics.debugMode) {
    //     return;
    // }
    
    // // Create debug info builder
    // auto debugBuilder = std::make_unique<llvm::DIBuilder>(*Module);
    
    // // Create compile unit
    // std::string filename = configs.filePath.empty() ? "unknown" : configs.filePath;
    // size_t lastSlash = filename.find_last_of("/\\");
    // std::string directory = (lastSlash != std::string::npos) ? filename.substr(0, lastSlash) : ".";
    // std::string file = (lastSlash != std::string::npos) ? filename.substr(lastSlash + 1) : filename;
    
    // auto debugCompileUnit = debugBuilder->createCompileUnit(
    //     llvm::dwarf::DW_LANG_C,  // You might want to define your own language constant
    //     debugBuilder->createFile(file, directory),
    //     "OmniScript Compiler",
    //     configs.optimization.level > 0,  // isOptimized
    //     "",  // flags
    //     0    // runtime version
    // );
}

}

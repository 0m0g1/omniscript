#pragma once

#include <omniscript/engine/Backends/AOTBackend.h>
#include <omniscript/engine/Backends/llvm/IRGenerator.h>
#include <omniscript/omniscript_pch.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>

class LLVMAOTBackend : public AOTBackend {
private:
    std::shared_ptr<IRGenerator> irGen;
    std::shared_ptr<llvm::TargetMachine> targetMachine;
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope;
    std::string outputPath;

    void emitObjectFile(const std::string& objFile);
    void linkExecutable(const std::string& objFile, const std::string& exeFile);

public:
    LLVMAOTBackend();

    void initialize() override;

    void execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) override;

    void emitToFile(const std::string& filename) override;
    void emitAssemblyFile(const std::string& asmFilename);
    bool isLinkerAvailable(const std::string& linker);
};

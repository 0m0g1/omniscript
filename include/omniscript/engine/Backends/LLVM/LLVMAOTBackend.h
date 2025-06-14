#pragma once

#include <omniscript/engine/Backends/AOTBackend.h>
#include <omniscript/engine/Backends/llvm/IRGenerator.h>
#include <omniscript/omniscript_pch.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Host.h>

class LLVMAOTBackend : public AOTBackend {
private:
    std::shared_ptr<IRGenerator> irGen;
    std::shared_ptr<llvm::TargetMachine> targetMachine;
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope;
    std::string outputPath;

public:
    LLVMAOTBackend();

    void initialize() override;

    void execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) override;

    void emitToFile(const std::string& filename) override;
};

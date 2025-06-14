#pragma once

#include <omniscript/engine/Backends/llvm/IRGenerator.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/EngineConfigs.h>
#include <omniscript/engine/Backends/JITBackend.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetSelect.h>

class LLVMJITBackend : public JITBackend {
private:
    std::shared_ptr<IRGenerator> irGen;
    std::unique_ptr<llvm::orc::LLJIT> jit;
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope;

public:
    LLVMJITBackend() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        
        auto jitOrError = llvm::orc::LLJITBuilder().create();
        if (!jitOrError) {
            throw std::runtime_error("Failed to create LLJIT");
        }

        jit = std::move(*jitOrError);
        scope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>();
    }

    void initialize() override; 
    void execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config& config) override;
};
    
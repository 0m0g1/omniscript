#pragma once

#include <omniscript/utils.h>
#include <omniscript/runtime/Statement.h>
#include <omniscript/engine/IRGenerator.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/EngineConfigs.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/TargetSelect.h>

class JITCompiler {
private:
    IRGenerator& irGen;
    std::unique_ptr<llvm::orc::LLJIT> jit;

public:
    JITCompiler(IRGenerator& generator) : irGen(generator) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    auto jitOrError = llvm::orc::LLJITBuilder().create();
    if (!jitOrError) {
        throw std::runtime_error("Failed to create LLJIT");
    }
        jit = std::move(*jitOrError);
    }

    void execute(const std::vector<std::shared_ptr<Statement>>& statements, const Config &config);
};

// engine/src/backends/llvm/IRGenerator/Codegen.cpp
#include <omniscript/backends/llvm/IRGenerator.h>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include <stdexcept>

namespace Omniscript {

llvm::Function* IRGenerator::ensureMainWrapper() {
    if (!m_module || !m_builder) {
        throw std::runtime_error("IRGenerator not initialized (call initialize() first)");
    }

    auto& C = getContext();

    // __top_level__ : void()
    llvm::Function* top = m_module->getFunction("__top_level__");
    if (!top) {
        auto* topTy = llvm::FunctionType::get(llvm::Type::getVoidTy(C), false);
        top = llvm::Function::Create(topTy, llvm::Function::ExternalLinkage,
                                     "__top_level__", m_module.get());
        llvm::BasicBlock::Create(C, "entry", top);
    }

    // main : i32()
    llvm::Function* mainFn = m_module->getFunction("main");
    if (!mainFn) {
        auto* i32 = llvm::Type::getInt32Ty(C);
        auto* mainTy = llvm::FunctionType::get(i32, false);
        mainFn = llvm::Function::Create(mainTy, llvm::Function::ExternalLinkage,
                                        "main", m_module.get());

        auto* entry = llvm::BasicBlock::Create(C, "entry", mainFn);
        llvm::IRBuilder<> b(entry);
        b.CreateCall(top);
        b.CreateRet(llvm::ConstantInt::get(i32, 0));
    }

    return top;
}

llvm::Function* IRGenerator::declareExternFunction(const FunctionDeclStmt& fnDecl) {
    if (!m_module) {
        throw std::runtime_error("IRGenerator not initialized (call initialize() first)");
    }

    const std::string name = fnDecl.name.value();
    if (auto* existing = m_module->getFunction(name)) return existing;

    // Temporary prototype until you map types: void(...)
    auto& C = getContext();
    auto* fty = llvm::FunctionType::get(llvm::Type::getVoidTy(C), /*isVarArg=*/true);
    return llvm::Function::Create(fty, llvm::Function::ExternalLinkage, name, m_module.get());
}

void IRGenerator::codegenTopLevelStmt(const Stmt&) {
    // TODO: implement
}

llvm::Value* IRGenerator::codegenExpr(const Expr&) {
    // TODO: implement
    return nullptr;
}

llvm::Value* IRGenerator::codegenCStringLiteral(const Token&) { return nullptr; }
llvm::Value* IRGenerator::codegenIntLiteral(const Token&) { return nullptr; }
llvm::Value* IRGenerator::codegenFloatLiteral(const Token&) { return nullptr; }
llvm::Value* IRGenerator::codegenBoolLiteral(const Token&) { return nullptr; }

// THIS is the missing symbol your linker needs:
void IRGenerator::codegenProgram(const Program& program) {
    (void)program;

    llvm::Function* top = ensureMainWrapper();
    m_builder->SetInsertPoint(&top->getEntryBlock());

    if (!m_builder->GetInsertBlock()->getTerminator()) {
        m_builder->CreateRetVoid();
    }
}

} // namespace Omniscript
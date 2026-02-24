// engine/include/omniscript/backends/llvm/IRGenerator.h
#pragma once

#include <omniscript/EngineConfigs.h>
#include <omniscript/ast/Ast.h>
#include <omniscript/semantics/Type.h>

#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h> // legacy resolver interface

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h> // provides ThreadSafeModule (+ often ThreadSafeContext)
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Omniscript {

class IRGenerator {
public:
    explicit IRGenerator(const Config& config);
    ~IRGenerator() = default;

    void initialize();
    void finalize();

    void codegenProgram(const Program& program);

    llvm::orc::ThreadSafeModule takeThreadSafeModule();

    std::unique_ptr<llvm::Module> takeModule();
    llvm::Module* getModule() const { return m_module.get(); }

    llvm::LLVMContext& getContext();
    llvm::IRBuilder<>& getBuilder();

    void setTargetMachine(std::shared_ptr<llvm::TargetMachine> tm);
    std::shared_ptr<llvm::TargetMachine> getTargetMachine() const { return m_targetMachine; }

    void addExternalResolver(const std::string& language,
                             std::unique_ptr<ExternalFunctionResolver> resolver);

    LinkDependencies& getLinkDependencies() { return m_linkDeps; }
    const LinkDependencies& getLinkDependencies() const { return m_linkDeps; }

    llvm::Type* toLLVMType(const Type& t);

    llvm::Function* declareExternFunction(const FunctionDeclStmt& fnDecl);

private:
    const Config& m_config;

    // ORC-friendly context ownership:
    // NOTE: ThreadSafeContext is declared by LLVM ORC headers; we avoid including
    // ThreadSafeContext.h because some LLVM 20 installs don’t ship it.
    llvm::orc::ThreadSafeContext m_tsc;

    std::unique_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;

    std::shared_ptr<llvm::TargetMachine> m_targetMachine;

    std::unordered_map<std::string, std::unique_ptr<ExternalFunctionResolver>> m_resolvers;
    LinkDependencies m_linkDeps;

    // Minimal local env for bootstrap IRGen: identifier -> alloca/value (current function)
    std::unordered_map<std::string, llvm::Value*> m_locals;

    llvm::Function* ensureMainWrapper();
    void codegenTopLevelStmt(const Stmt& st);
    llvm::Value* codegenExpr(const Expr& ex);

    llvm::Value* codegenCStringLiteral(const Token& tok);
    llvm::Value* codegenIntLiteral(const Token& tok);
    llvm::Value* codegenFloatLiteral(const Token& tok);
    llvm::Value* codegenBoolLiteral(const Token& tok);
};

} // namespace Omniscript
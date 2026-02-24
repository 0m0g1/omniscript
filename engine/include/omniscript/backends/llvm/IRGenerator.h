#pragma once

#include <omniscript/EngineConfigs.h>
#include <omniscript/ast/Ast.h>
#include <omniscript/semantics/Type.h>

#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h> // your legacy resolver interface
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Omniscript {

/// AST-native LLVM IR generator.
/// Owns a ThreadSafeContext and produces a ThreadSafeModule for ORC.
/// Also supports "AOT-style" emission by exposing the generated Module.
class IRGenerator {
public:
    explicit IRGenerator(const Config& config);
    ~IRGenerator() = default;

    // Lifecycle
    void initialize();
    void finalize();

    // Generate IR from the new AST Program.
    // Convention: top-level statements get wrapped in an auto-generated main()
    // unless you later enforce explicit entry points.
    void codegenProgram(const Program& program);

    // ORC consumption (JIT)
    llvm::orc::ThreadSafeModule takeThreadSafeModule();

    // AOT consumption (emit to file)
    // NOTE: for AOT you often want the raw module; this gives you ownership.
    std::unique_ptr<llvm::Module> takeModule();
    llvm::Module* getModule() const { return m_module.get(); }

    llvm::LLVMContext& getContext();
    llvm::IRBuilder<>& getBuilder();

    // Target machine support (AOT, or JIT configuration)
    void setTargetMachine(std::shared_ptr<llvm::TargetMachine> tm);
    std::shared_ptr<llvm::TargetMachine> getTargetMachine() const { return m_targetMachine; }

    // External resolver system (reuses your legacy resolver interface and LinkDependencies)
    void addExternalResolver(const std::string& language,
                             std::unique_ptr<ExternalFunctionResolver> resolver);
    LinkDependencies& getLinkDependencies() { return m_linkDeps; }
    const LinkDependencies& getLinkDependencies() const { return m_linkDeps; }

    // Minimal type mapping from your semantic Type to LLVM type.
    llvm::Type* toLLVMType(const Type& t);

    // Extern decl helpers (useful for ExternStmt / FFI expansion)
    llvm::Function* declareExternFunction(const FunctionDeclStmt& fnDecl);

private:
    const Config& m_config;

    // ORC-friendly context ownership
    llvm::orc::ThreadSafeContext m_tsc;

    // Module + builder owned here; module is created within m_tsc context.
    std::unique_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;

    std::shared_ptr<llvm::TargetMachine> m_targetMachine;

    // External resolution
    std::unordered_map<std::string, std::unique_ptr<ExternalFunctionResolver>> m_resolvers;
    LinkDependencies m_linkDeps;

    // --- Codegen helpers (AST walking) ---
    llvm::Function* ensureMainWrapper();
    void codegenTopLevelStmt(const Stmt& st);
    llvm::Value* codegenExpr(const Expr& ex);

    // literal helpers
    llvm::Value* codegenCStringLiteral(const Token& tok); // string literal => i8*
    llvm::Value* codegenIntLiteral(const Token& tok);     // integer literal => i64 (or config-driven)
    llvm::Value* codegenFloatLiteral(const Token& tok);   // float literal => double
    llvm::Value* codegenBoolLiteral(const Token& tok);    // bool literal => i1
};

} // namespace Omniscript
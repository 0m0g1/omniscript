#pragma once

#include <utility>
#include <vector>

#include <omniscript/ast/Ast.h>

namespace Omniscript {

// ----------------- Statements -----------------
// ----------------- Extern support -----------------
//
// Extern inputs can mix libraries + headers:
//   extern "mylib.h", "mylib.dll" { ... }
//   extern "mylib.h", "mylib.dll";                // auto-import (statements empty)
//   extern "mylib.h", "mylib.dll" fn ...          // single decl form (statements size=1)
//
// Inside extern blocks you can write:
//   fn / let / const  (manual typed declarations)
//   import fn <name> [as <alias>]   (auto signature from header(s))
//   import *;                        (optional: import everything from header(s))
//
// The header paths are inferred from the extern inputs by extension (.h/.hpp/...).

enum class ExternArtifactKind : std::uint8_t { Library, Header, Unknown };

struct ExternArtifact {
    Token pathTok;                // string literal token
    ExternArtifactKind kind;
};

// import kinds
// ImportStmt in Statement.h
enum class ImportKind : uint8_t { Fn, Var, Const, All };

struct ImportStmt final : Stmt {
    ImportKind kind;
    Token name;   // empty for All
    Token alias;  // optional

    ImportStmt(ImportKind k, Token n = {}, Token a = {}, FileSpan s = {})
      : Stmt(NodeKind::ImportStmt, std::move(s)), kind(k), name(std::move(n)), alias(std::move(a)) {}

    void accept(AstVisitor& v) override;
};

struct ExternStmt final : Stmt {
    // Raw extern inputs as authored in source.
    // Example: ["mylib.h", "mylib.dll", "libmylib.a"]
    std::vector<Token> inputTokens;

    // Split views (filled by parser for convenience).
    // Headers: *.h, *.hpp, *.hh, *.hxx, *.inl
    // Libraries: *.dll, *.so, *.dylib, *.a, *.lib, *.wa
    std::vector<std::string> headerPaths;
    std::vector<std::string> libraryPaths;

    // Decls inside extern block (or single-decl extern form).
    // Empty means auto-import mode (e.g. extern "mylib.h", "mylib.dll";).
    std::vector<StmtPtr> statements;

    explicit ExternStmt(std::vector<Token> inputs = {},
                        std::vector<std::string> headers = {},
                        std::vector<std::string> libs = {},
                        std::vector<StmtPtr> stmts = {},
                        FileSpan s = {})
        : Stmt(NodeKind::ExternStmt, std::move(s)),
          inputTokens(std::move(inputs)),
          headerPaths(std::move(headers)),
          libraryPaths(std::move(libs)),
          statements(std::move(stmts)) {}

    void accept(AstVisitor& v) override;
};

// AstStatement.h (add this)
struct ParamDecl {
    Token name;                 // identifier token
    std::vector<Token> type;    // minimal "type tokens" (e.g. ["int"], or ["MyType","*"])
    std::vector<Token> typeToks;
    FileSpan span{};
    bool isVarArg = false;
};

struct FunctionDeclStmt final : Stmt {
    Token name;                      // identifier
    std::vector<ParamDecl> params;
    std::vector<Token> returnType;   // empty => infer void
    StmtPtr body;                    // BlockStmt or null for prototypes
    bool isPrototype = false;        // true if ends with ';' (or newline) without body

    FunctionDeclStmt(Token n,
                     std::vector<ParamDecl> ps,
                     std::vector<Token> ret,
                     StmtPtr b,
                     bool proto,
                     FileSpan s = {})
        : Stmt(NodeKind::FunctionDeclStmt, std::move(s)),
          name(std::move(n)),
          params(std::move(ps)),
          returnType(std::move(ret)),
          body(std::move(b)),
          isPrototype(proto) {
        if (span.start.filePath.empty()) span = this->name.span();
    }

    void accept(AstVisitor& v) override;
};

struct BlockStmt final : Stmt {
    std::vector<StmtPtr> statements;

    explicit BlockStmt(std::vector<StmtPtr> stmts = {}, FileSpan s = {})
        : Stmt(NodeKind::BlockStmt, std::move(s)), statements(std::move(stmts)) {}

    void accept(AstVisitor& v) override;
};

struct ExprStmt final : Stmt {
    ExprPtr expr;

    explicit ExprStmt(ExprPtr e, FileSpan s = {})
        : Stmt(NodeKind::ExprStmt, std::move(s)), expr(std::move(e)) {}

    void accept(AstVisitor& v) override;
};

struct VarDeclStmt final : Stmt {
    VarFlavor flavor;
    Token name;          // identifier token
    ExprPtr initializer; // may be null

    VarDeclStmt(VarFlavor f, Token n, ExprPtr init, FileSpan s = {})
        : Stmt(NodeKind::VarDeclStmt, std::move(s)),
          flavor(f),
          name(std::move(n)),
          initializer(std::move(init)) {
        if (span.start.filePath.empty()) span = name.span();
    }

    void accept(AstVisitor& v) override;
};

struct IfStmt final : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch; // may be null

    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e = nullptr, FileSpan s = {})
        : Stmt(NodeKind::IfStmt, std::move(s)),
          condition(std::move(c)),
          thenBranch(std::move(t)),
          elseBranch(std::move(e)) {}

    void accept(AstVisitor& v) override;
};

struct WhileStmt final : Stmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr c, StmtPtr b, FileSpan s = {})
        : Stmt(NodeKind::WhileStmt, std::move(s)),
          condition(std::move(c)),
          body(std::move(b)) {}

    void accept(AstVisitor& v) override;
};

struct ReturnStmt final : Stmt {
    ExprPtr value; // may be null

    explicit ReturnStmt(ExprPtr v = nullptr, FileSpan s = {})
        : Stmt(NodeKind::ReturnStmt, std::move(s)), value(std::move(v)) {}

    void accept(AstVisitor& v) override;
};

} // namespace Omniscript
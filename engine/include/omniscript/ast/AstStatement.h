#pragma once

#include <utility>
#include <vector>

#include <omniscript/ast/Ast.h>

namespace Omniscript {

// ----------------- Extern / FFI support -----------------

enum class ExternArtifactKind : std::uint8_t { Library, Header, Unknown };

struct ExternArtifact {
    Token pathTok;
    ExternArtifactKind kind;
};

enum class ImportKind : std::uint8_t { Fn, Var, Const, All };

struct ImportStmt final : Stmt {
    ImportKind kind;
    Token name;
    Token alias;

    ImportStmt(ImportKind k, Token n = {}, Token a = {}, FileSpan s = {})
      : Stmt(NodeKind::ImportStmt, std::move(s)), kind(k), name(std::move(n)), alias(std::move(a)) {}

    void accept(AstVisitor& v) override;
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

enum class ExternLang : std::uint8_t { Auto, C, Cpp };

struct ExternStmt final : Stmt {
    std::vector<Token>       inputTokens;
    ExternLang               lang         = ExternLang::Auto;
    bool                     allowMangled = false;
    std::vector<std::string> headerPaths;
    std::vector<std::string> libraryPaths;
    std::vector<StmtPtr>     statements;

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
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

// ----------------- Declarations -----------------

struct ParamDecl {
    Token              name;
    std::vector<Token> type;
    std::vector<Token> typeToks;
    FileSpan           span{};
    bool               isVarArg = false;
};

struct FunctionDeclStmt final : Stmt {
    Token                  name;
    std::vector<ParamDecl> params;
    std::vector<Token>     returnType;
    StmtPtr                body;
    bool                   isPrototype = false;

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
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct BlockStmt final : Stmt {
    std::vector<StmtPtr> statements;

    explicit BlockStmt(std::vector<StmtPtr> stmts = {}, FileSpan s = {})
        : Stmt(NodeKind::BlockStmt, std::move(s)), statements(std::move(stmts)) {}

    void accept(AstVisitor& v) override;
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct ExprStmt final : Stmt {
    ExprPtr expr;

    explicit ExprStmt(ExprPtr e, FileSpan s = {})
        : Stmt(NodeKind::ExprStmt, std::move(s)), expr(std::move(e)) {}

    void accept(AstVisitor& v) override;
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct VarDeclStmt final : Stmt {
    VarFlavor flavor;
    Token name;
    std::vector<Token> declaredType;   // <--- ADD THIS (empty = inferred)
    ExprPtr initializer;

    VarDeclStmt(VarFlavor f, Token n, std::vector<Token> ty, ExprPtr init, FileSpan s = {})
        : Stmt(NodeKind::VarDeclStmt, std::move(s)),
          flavor(f),
          name(std::move(n)),
          declaredType(std::move(ty)),
          initializer(std::move(init)) {
        if (span.start.filePath.empty()) span = name.span();
    }

    void accept(AstVisitor& v) override;
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct IfStmt final : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;

    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e = nullptr, FileSpan s = {})
        : Stmt(NodeKind::IfStmt, std::move(s)),
          condition(std::move(c)),
          thenBranch(std::move(t)),
          elseBranch(std::move(e)) {}

    void accept(AstVisitor& v) override;
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct WhileStmt final : Stmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr c, StmtPtr b, FileSpan s = {})
        : Stmt(NodeKind::WhileStmt, std::move(s)),
          condition(std::move(c)),
          body(std::move(b)) {}

    void accept(AstVisitor& v) override;
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct ReturnStmt final : Stmt {
    ExprPtr value;

    explicit ReturnStmt(ExprPtr v = nullptr, FileSpan s = {})
        : Stmt(NodeKind::ReturnStmt, std::move(s)), value(std::move(v)) {}

    void accept(AstVisitor& v) override;
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

// -----------------------------------------------------------------------
// Struct declarations
//
//   struct Vec2 {
//       x: f32;
//       y: f32 = 0.0;
//   }
//
// typeToks stores the raw type tokens so the type-checker can resolve them
// later using the same logic as FunctionDeclStmt.returnType.
// -----------------------------------------------------------------------

struct StructField {
    Token              name;         // field identifier
    std::vector<Token> typeToks;     // raw type tokens, e.g. ["f32"] or ["char","*"]
    ExprPtr            defaultValue; // optional initializer expression (may be null)
    FileSpan           span{};
};

struct StructDeclStmt final : Stmt {
    Token                    name;
    std::vector<StructField> fields;

    StructDeclStmt(Token n,
                   std::vector<StructField> f,
                   FileSpan s = {})
        : Stmt(NodeKind::StructDeclStmt, std::move(s))
        , name(std::move(n))
        , fields(std::move(f))
    {
        if (span.start.filePath.empty()) span = this->name.span();
    }

    void accept(AstVisitor& v) override;
    bool evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

} // namespace Omniscript
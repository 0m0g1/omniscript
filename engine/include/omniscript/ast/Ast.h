// Ast.h
#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include <omniscript/FileSpan.h>
#include <omniscript/tokens/Tokens.h>

namespace Omniscript {

enum class NodeKind : std::uint16_t {
    // statements
    Program,
    BlockStmt,
    ExprStmt,
    VarDeclStmt,
    IfStmt,
    WhileStmt,
    ReturnStmt,

    // expressions
    IdentifierExpr,
    LiteralExpr,
    UnaryExpr,
    BinaryExpr,
    GroupExpr,
};

struct Node {
    NodeKind kind;
    FileSpan span;

    explicit Node(NodeKind k, FileSpan s = {}) : kind(k), span(std::move(s)) {}
    virtual ~Node() = default;

    // Core: accept a visitor. Printers (recursive or not) implement the visitor.
    virtual void accept(struct AstVisitor& v) = 0;
};

struct Stmt : Node { using Node::Node; };
struct Expr : Node { using Node::Node; };

template <typename T>
using Ptr = std::unique_ptr<T>;
using StmtPtr = Ptr<Stmt>;
using ExprPtr = Ptr<Expr>;

inline FileSpan mergeSpans(const FileSpan& a, const FileSpan& b) {
    FileSpan s = a;
    s.end = b.end;
    return s;
}

enum class VarFlavor : std::uint8_t { Let, Var, Const };

struct Program final : Node {
    std::vector<StmtPtr> statements;

    explicit Program(std::vector<StmtPtr> stmts = {}, FileSpan s = {})
        : Node(NodeKind::Program, std::move(s)), statements(std::move(stmts)) {}

    void accept(AstVisitor& v) override;
};

// ----------------- Statements -----------------

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

// ----------------- Expressions -----------------

struct IdentifierExpr final : Expr {
    Token name;

    explicit IdentifierExpr(Token n)
        : Expr(NodeKind::IdentifierExpr, n.span()), name(std::move(n)) {}

    void accept(AstVisitor& v) override;
};

struct LiteralExpr final : Expr {
    Token literal; // integer/float/string/true/false/null...

    explicit LiteralExpr(Token t)
        : Expr(NodeKind::LiteralExpr, t.span()), literal(std::move(t)) {}

    void accept(AstVisitor& v) override;
};

struct GroupExpr final : Expr {
    ExprPtr inner;

    explicit GroupExpr(ExprPtr e, FileSpan s = {})
        : Expr(NodeKind::GroupExpr, std::move(s)), inner(std::move(e)) {}

    void accept(AstVisitor& v) override;
};

struct UnaryExpr final : Expr {
    Token op;
    ExprPtr right;

    UnaryExpr(Token o, ExprPtr r, FileSpan s = {})
        : Expr(NodeKind::UnaryExpr, std::move(s)), op(std::move(o)), right(std::move(r)) {}

    void accept(AstVisitor& v) override;
};

struct BinaryExpr final : Expr {
    ExprPtr left;
    Token op;
    ExprPtr right;

    BinaryExpr(ExprPtr l, Token o, ExprPtr r, FileSpan s = {})
        : Expr(NodeKind::BinaryExpr, std::move(s)),
          left(std::move(l)),
          op(std::move(o)),
          right(std::move(r)) {}

    void accept(AstVisitor& v) override;
};

} // namespace Omniscript

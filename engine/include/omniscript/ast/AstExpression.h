#pragma once

#include <utility>

#include <omniscript/ast/Ast.h>

namespace Omniscript {
struct IdentifierPath {
    // Identifiers in order: ["std", "io", "File"]
    std::vector<Token> parts;

    // Separators between parts: "::" or "."
    // size == parts.size()-1
    std::vector<TokenType> seps;

    FileSpan span{};
};

// ----------------- Expressions -----------------

struct IdentifierExpr final : Expr {
    IdentifierPath name;

    explicit IdentifierExpr(IdentifierPath p)
        : Expr(NodeKind::IdentifierExpr, p.span), name(std::move(p)) {}

    void accept(AstVisitor& v) override;
    Type evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct CallExpr final : Expr {
    // simplest: callee is a path like AudioAccess.getAvailableSoundDevices
    IdentifierPath callee;
    std::vector<ExprPtr> args;

    CallExpr(IdentifierPath c, std::vector<ExprPtr> a, FileSpan s = {})
        : Expr(NodeKind::CallExpr, std::move(s)),
          callee(std::move(c)),
          args(std::move(a)) {}

    void accept(AstVisitor& v) override;
    Type evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct LiteralExpr final : Expr {
    Token literal; // integer/float/string/true/false/null...

    explicit LiteralExpr(Token t)
        : Expr(NodeKind::LiteralExpr, t.span()), literal(std::move(t)) {}

    void accept(AstVisitor& v) override;
    Type evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct GroupExpr final : Expr {
    ExprPtr inner;

    explicit GroupExpr(ExprPtr e, FileSpan s = {})
        : Expr(NodeKind::GroupExpr, std::move(s)), inner(std::move(e)) {}

    void accept(AstVisitor& v) override;
    Type evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

struct UnaryExpr final : Expr {
    Token op;
    ExprPtr right;

    UnaryExpr(Token o, ExprPtr r, FileSpan s = {})
        : Expr(NodeKind::UnaryExpr, std::move(s)), op(std::move(o)), right(std::move(r)) {}

    void accept(AstVisitor& v) override;
    Type evaluate(SymbolTable& scope, EvalContext& ctx) override;
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
    Type evaluate(SymbolTable& scope, EvalContext& ctx) override;
};

} // namespace Omniscript
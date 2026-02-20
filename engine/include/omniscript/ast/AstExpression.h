#pragma once

#include <utility>

#include <omniscript/ast/Ast.h>

namespace Omniscript {

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
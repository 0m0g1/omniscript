// Visitor.h
#pragma once
#include <omniscript/ast/Ast.h>

namespace Omniscript {

// Forward decls
struct Program;

struct BlockStmt;
struct ExprStmt;
struct VarDeclStmt;
struct IfStmt;
struct WhileStmt;
struct ReturnStmt;

struct IdentifierExpr;
struct LiteralExpr;
struct GroupExpr;
struct UnaryExpr;
struct BinaryExpr;

struct AstVisitor {
    virtual ~AstVisitor() = default;

    // Statements / Program
    virtual void visit(Program&) = 0;
    virtual void visit(BlockStmt&) = 0;
    virtual void visit(ExprStmt&) = 0;
    virtual void visit(VarDeclStmt&) = 0;
    virtual void visit(IfStmt&) = 0;
    virtual void visit(WhileStmt&) = 0;
    virtual void visit(ReturnStmt&) = 0;

    // Expressions
    virtual void visit(IdentifierExpr&) = 0;
    virtual void visit(LiteralExpr&) = 0;
    virtual void visit(GroupExpr&) = 0;
    virtual void visit(UnaryExpr&) = 0;
    virtual void visit(BinaryExpr&) = 0;
};

} // namespace Omniscript

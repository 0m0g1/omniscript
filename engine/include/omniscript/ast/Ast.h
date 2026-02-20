#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <omniscript/FileSpan.h>
#include <omniscript/tokens/Tokens.h>

namespace Omniscript {

// ----------------- Kinds -----------------

enum class NodeKind : std::uint16_t {
    // statements
    Program,
    ExternStmt,
    ImportStmt,
    FunctionDeclStmt,
    StructDeclStmt,
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
    CallExpr,
};

enum class VarFlavor : std::uint8_t { Let, Var, Const };

// ----------------- Forward decls -----------------

struct AstVisitor;

struct Node;
struct Stmt;
struct Expr;

template <typename T>
using Ptr = std::unique_ptr<T>;
using StmtPtr = Ptr<Stmt>;
using ExprPtr = Ptr<Expr>;

// ----------------- Base nodes -----------------

struct Node {
    NodeKind kind;
    FileSpan span;

    explicit Node(NodeKind k, FileSpan s = {}) : kind(k), span(std::move(s)) {}
    virtual ~Node() = default;

    virtual void accept(AstVisitor& v) = 0;
};

struct Stmt : Node { using Node::Node; };
struct Expr : Node { using Node::Node; };

// ----------------- Helpers -----------------

inline FileSpan mergeSpans(const FileSpan& a, const FileSpan& b) {
    FileSpan s = a;
    s.end = b.end;
    return s;
}

// ----------------- Program -----------------

struct Program final : Node {
    std::vector<StmtPtr> statements;

    explicit Program(std::vector<StmtPtr> stmts = {}, FileSpan s = {})
        : Node(NodeKind::Program, std::move(s)), statements(std::move(stmts)) {}

    void accept(AstVisitor& v) override;
};

} // namespace Omniscript

// Pull in concrete node definitions.
#include <omniscript/ast/AstStatement.h>
#include <omniscript/ast/AstExpression.h>
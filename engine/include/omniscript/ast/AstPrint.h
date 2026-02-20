// AstPrint.h
#pragma once
#include <ostream>
#include <string>
#include <vector>

#include <omniscript/ast/Ast.h>
#include <omniscript/ast/AstVisitor.h>

namespace Omniscript {

// Toggle: recursive vs non-recursive printing
enum class PrintMode : std::uint8_t { Recursive, NonRecursive };

struct AstPrinter final : AstVisitor {
    explicit AstPrinter(std::ostream& out, PrintMode mode = PrintMode::Recursive)
        : os(out), mode(mode) {}

    void print(Node& root) { root.accept(*this); }

    // Visitor overrides
    void visit(Program&) override;
    void visit(ExternStmt&) override;
    void visit(ImportStmt&) override;
    void visit(FunctionDeclStmt&) override;
    void visit(BlockStmt&) override;
    void visit(ExprStmt&) override;
    void visit(VarDeclStmt&) override;
    void visit(IfStmt&) override;
    void visit(WhileStmt&) override;
    void visit(ReturnStmt&) override;

    void visit(IdentifierExpr&) override;
    void visit(CallExpr& n) override;
    void visit(LiteralExpr&) override;
    void visit(GroupExpr&) override;
    void visit(UnaryExpr&) override;
    void visit(BinaryExpr&) override;

private:
    std::ostream& os;
    PrintMode mode;
    int indent = 0;

    // helpers
    void line(const std::string& s);
    void pushIndent() { indent += 2; }
    void popIndent() { indent -= 2; if (indent < 0) indent = 0; }

    void printChild(Node* n, const char* label);

    // non-recursive traversal support
    struct Frame {
        Node* node;
        const char* label;
        int depth;
    };
    void printNonRecursive(Node& root);

    static const char* flavorToString(VarFlavor f);
};

} // namespace Omniscript

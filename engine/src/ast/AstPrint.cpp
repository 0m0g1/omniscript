// AstPrint.cpp
#include <omniscript/ast/AstPrint.h>
#include <sstream>

namespace Omniscript {

static std::string tokText(const Token& t) {
    // Adjust depending on your Token API (lexeme/text/value)
    // Fallback: if you don't have text, remove this from output.
    return t.toString();
}

const char* AstPrinter::flavorToString(VarFlavor f) {
    switch (f) {
        case VarFlavor::Let:   return "let";
        case VarFlavor::Var:   return "var";
        case VarFlavor::Const: return "const";
    }
    return "?";
}

void AstPrinter::line(const std::string& s) {
    for (int i = 0; i < indent; ++i) os << ' ';
    os << s << "\n";
}

void AstPrinter::printChild(Node* n, const char* label) {
    if (!n) {
        std::ostringstream ss;
        ss << label << ": <null>";
        line(ss.str());
        return;
    }

    if (mode == PrintMode::Recursive) {
        std::ostringstream ss;
        ss << label << ":";
        line(ss.str());
        pushIndent();
        n->accept(*this);
        popIndent();
    } else {
        // In non-recursive mode, just show one line summary
        std::ostringstream ss;
        ss << label << ": <" << static_cast<int>(n->kind) << ">";
        line(ss.str());
    }
}

void AstPrinter::printNonRecursive(Node& root) {
    // Simple stack-based preorder traversal with indentation depth tracking.
    // Prints each node once; children discovered based on node type.
    std::vector<Frame> stack;
    stack.push_back(Frame{&root, "root", 0});

    auto emitNodeHeader = [&](Node& n, const char* label, int depth) {
        indent = depth * 2;
        std::ostringstream ss;
        ss << label << ": ";
        switch (n.kind) {
            case NodeKind::Program:         ss << "Program"; break;
            case NodeKind::BlockStmt:       ss << "BlockStmt"; break;
            case NodeKind::ExprStmt:        ss << "ExprStmt"; break;
            case NodeKind::VarDeclStmt:     ss << "VarDeclStmt"; break;
            case NodeKind::IfStmt:          ss << "IfStmt"; break;
            case NodeKind::WhileStmt:       ss << "WhileStmt"; break;
            case NodeKind::ReturnStmt:      ss << "ReturnStmt"; break;
            case NodeKind::IdentifierExpr:  ss << "IdentifierExpr"; break;
            case NodeKind::LiteralExpr:     ss << "LiteralExpr"; break;
            case NodeKind::UnaryExpr:       ss << "UnaryExpr"; break;
            case NodeKind::BinaryExpr:      ss << "BinaryExpr"; break;
            case NodeKind::GroupExpr:       ss << "GroupExpr"; break;
        }
        line(ss.str());
    };

    while (!stack.empty()) {
        Frame f = stack.back();
        stack.pop_back();

        emitNodeHeader(*f.node, f.label, f.depth);

        // Push children in reverse order so they print in natural order.
        switch (f.node->kind) {
            case NodeKind::Program: {
                auto& n = static_cast<Program&>(*f.node);
                for (int i = (int)n.statements.size() - 1; i >= 0; --i)
                    stack.push_back(Frame{n.statements[i].get(), "stmt", f.depth + 1});
            } break;

            case NodeKind::BlockStmt: {
                auto& n = static_cast<BlockStmt&>(*f.node);
                for (int i = (int)n.statements.size() - 1; i >= 0; --i)
                    stack.push_back(Frame{n.statements[i].get(), "stmt", f.depth + 1});
            } break;

            case NodeKind::ExprStmt: {
                auto& n = static_cast<ExprStmt&>(*f.node);
                if (n.expr) stack.push_back(Frame{n.expr.get(), "expr", f.depth + 1});
            } break;

            case NodeKind::VarDeclStmt: {
                auto& n = static_cast<VarDeclStmt&>(*f.node);
                if (n.initializer) stack.push_back(Frame{n.initializer.get(), "init", f.depth + 1});
            } break;

            case NodeKind::IfStmt: {
                auto& n = static_cast<IfStmt&>(*f.node);
                if (n.elseBranch)  stack.push_back(Frame{n.elseBranch.get(), "else", f.depth + 1});
                if (n.thenBranch)  stack.push_back(Frame{n.thenBranch.get(), "then", f.depth + 1});
                if (n.condition)   stack.push_back(Frame{n.condition.get(), "cond", f.depth + 1});
            } break;

            case NodeKind::WhileStmt: {
                auto& n = static_cast<WhileStmt&>(*f.node);
                if (n.body)      stack.push_back(Frame{n.body.get(), "body", f.depth + 1});
                if (n.condition) stack.push_back(Frame{n.condition.get(), "cond", f.depth + 1});
            } break;

            case NodeKind::ReturnStmt: {
                auto& n = static_cast<ReturnStmt&>(*f.node);
                if (n.value) stack.push_back(Frame{n.value.get(), "value", f.depth + 1});
            } break;

            case NodeKind::GroupExpr: {
                auto& n = static_cast<GroupExpr&>(*f.node);
                if (n.inner) stack.push_back(Frame{n.inner.get(), "inner", f.depth + 1});
            } break;

            case NodeKind::UnaryExpr: {
                auto& n = static_cast<UnaryExpr&>(*f.node);
                if (n.right) stack.push_back(Frame{n.right.get(), "right", f.depth + 1});
            } break;

            case NodeKind::BinaryExpr: {
                auto& n = static_cast<BinaryExpr&>(*f.node);
                if (n.right) stack.push_back(Frame{n.right.get(), "right", f.depth + 1});
                if (n.left)  stack.push_back(Frame{n.left.get(), "left", f.depth + 1});
            } break;

            case NodeKind::IdentifierExpr:
            case NodeKind::LiteralExpr:
                // leaf nodes
                break;
        }
    }

    indent = 0;
}

// -------- Recursive visitor printing --------

void AstPrinter::visit(Program& n) {
    if (mode == PrintMode::NonRecursive) return printNonRecursive(n);

    line("Program");
    pushIndent();
    for (auto& s : n.statements) printChild(s.get(), "stmt");
    popIndent();
}

void AstPrinter::visit(BlockStmt& n) {
    line("BlockStmt");
    pushIndent();
    for (auto& s : n.statements) printChild(s.get(), "stmt");
    popIndent();
}

void AstPrinter::visit(ExprStmt& n) {
    line("ExprStmt");
    pushIndent();
    printChild(n.expr.get(), "expr");
    popIndent();
}

void AstPrinter::visit(VarDeclStmt& n) {
    std::ostringstream ss;
    ss << "VarDeclStmt " << flavorToString(n.flavor) << " name=" << tokText(n.name);
    line(ss.str());

    pushIndent();
    printChild(n.initializer.get(), "initializer");
    popIndent();
}

void AstPrinter::visit(IfStmt& n) {
    line("IfStmt");
    pushIndent();
    printChild(n.condition.get(), "condition");
    printChild(n.thenBranch.get(), "then");
    printChild(n.elseBranch.get(), "else");
    popIndent();
}

void AstPrinter::visit(WhileStmt& n) {
    line("WhileStmt");
    pushIndent();
    printChild(n.condition.get(), "condition");
    printChild(n.body.get(), "body");
    popIndent();
}

void AstPrinter::visit(ReturnStmt& n) {
    line("ReturnStmt");
    pushIndent();
    printChild(n.value.get(), "value");
    popIndent();
}

void AstPrinter::visit(IdentifierExpr& n) {
    std::ostringstream ss;
    ss << "IdentifierExpr name=" << tokText(n.name);
    line(ss.str());
}

void AstPrinter::visit(LiteralExpr& n) {
    std::ostringstream ss;
    ss << "LiteralExpr value=" << tokText(n.literal);
    line(ss.str());
}

void AstPrinter::visit(GroupExpr& n) {
    line("GroupExpr");
    pushIndent();
    printChild(n.inner.get(), "inner");
    popIndent();
}

void AstPrinter::visit(UnaryExpr& n) {
    std::ostringstream ss;
    ss << "UnaryExpr op=" << tokText(n.op);
    line(ss.str());

    pushIndent();
    printChild(n.right.get(), "right");
    popIndent();
}

void AstPrinter::visit(BinaryExpr& n) {
    std::ostringstream ss;
    ss << "BinaryExpr op=" << tokText(n.op);
    line(ss.str());

    pushIndent();
    printChild(n.left.get(), "left");
    printChild(n.right.get(), "right");
    popIndent();
}

} // namespace Omni

// Ast.cpp (or AstAccept.cpp) - define accept() methods
#include <omniscript/ast/Ast.h>
#include <omniscript/ast/AstVisitor.h>

namespace Omniscript {

void Program::accept(AstVisitor& v) { v.visit(*this); }

void ExternStmt::accept(AstVisitor& v) { v.visit(*this); }
void ImportStmt::accept(AstVisitor& v) { v.visit(*this); }
void FunctionDeclStmt::accept(AstVisitor& v) { v.visit(*this); }
void BlockStmt::accept(AstVisitor& v) { v.visit(*this); }
void ExprStmt::accept(AstVisitor& v) { v.visit(*this); }
void VarDeclStmt::accept(AstVisitor& v) { v.visit(*this); }
void IfStmt::accept(AstVisitor& v) { v.visit(*this); }
void WhileStmt::accept(AstVisitor& v) { v.visit(*this); }
void ReturnStmt::accept(AstVisitor& v) { v.visit(*this); }

void IdentifierExpr::accept(AstVisitor& v) { v.visit(*this); }
void LiteralExpr::accept(AstVisitor& v) { v.visit(*this); }
void CallExpr::accept(AstVisitor& v) { v.visit(*this); }
void GroupExpr::accept(AstVisitor& v) { v.visit(*this); }
void UnaryExpr::accept(AstVisitor& v) { v.visit(*this); }
void BinaryExpr::accept(AstVisitor& v) { v.visit(*this); }

} // namespace Omniscript

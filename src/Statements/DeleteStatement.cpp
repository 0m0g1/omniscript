// DeleteStatement.cpp
#include <omniscript/Statements/DeleteStatement.h>
#include <omniscript/Expressions/DeleteExpression.h>
#include <omniscript/Core.h>
#include <omniscript/utils.h>

namespace Omniscript {

std::shared_ptr<Expression> DeleteStatement::express(SymbolTableType scope) {
    if (!operand) {
        console.reportError(Console::SYNTAX_ERROR, "delete needs an operand", getSpan());
        return nullptr;
    }

    DEBUG_LOG("Creating a delete expression");
    auto operandExpr = operand->express(scope);
    if (!operandExpr) {
        console.reportError(Console::RUNTIME_ERROR,
                            "Failed to evaluate the operand of delete", operand->getSpan());
        return nullptr;
    }

    auto deleteExpr = std::make_shared<DeleteExpression>(operandExpr, isArray);
    deleteExpr->setSpan(this->getSpan());
    return deleteExpr;
}

} // namespace Omniscript
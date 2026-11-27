// omniscript/Statements/DeferStatement.h
#pragma once
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Expressions/DeferExpression.h>
#include <omniscript/Core.h>

namespace Omniscript {

// defer <statement>;   or   defer { ... }
class DeferStatement : public ControlFlowStatement {
public:
    std::shared_ptr<Statement> action;

    explicit DeferStatement(std::shared_ptr<Statement> action) : action(std::move(action)) {}

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }

    std::shared_ptr<Expression> express(SymbolTableType scope) override {
        if (!action) {
            console.reportError(Console::SYNTAX_ERROR, "defer needs a statement", getSpan());
            return nullptr;
        }
        auto actionExpr = action->express(scope);
        if (!actionExpr) {
            console.reportError(Console::RUNTIME_ERROR,
                                "Failed to evaluate the statement of defer", action->getSpan());
            return nullptr;
        }
        auto deferExpr = std::make_shared<DeferExpression>(actionExpr);
        deferExpr->setSpan(this->getSpan());
        return deferExpr;
    }

    std::string toString() const override {
        return "Defer: " + (action ? action->toString() : std::string("null"));
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in defer statement.\n" + msg;
    }
    std::shared_ptr<Statement> clone() const override {
        auto copy = std::make_shared<DeferStatement>(action ? action->clone() : nullptr);
        copy->setSpan(getSpan());
        return copy;
    }
};

} // namespace Omniscript

// omniscript/Expressions/DeferExpression.h
#pragma once
#include <omniscript/Expressions/Expression.h>

namespace Omniscript {

// `defer <statement>` / `defer { ... }`: does not run where it is written. The IR generator records it
// in the current destructor scope (see IRGenerator::registerDefer) and emits `action` again at every
// exit of that scope (fall-through, return, break, continue), in reverse order with destructors.
struct DeferExpression : public Expression {
    std::shared_ptr<Expression> action;

    explicit DeferExpression(std::shared_ptr<Expression> action) : action(std::move(action)) {
        type = Type::createInvalid();   // statement-like, produces no value (same as Delete/Break)
    }

    std::string toString() const override {
        return "Defer(" + (action ? action->toString() : std::string("null")) + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        auto copy = std::make_shared<DeferExpression>(action ? action->clone() : nullptr);
        copy->setSpan(getSpan());
        return copy;
    }
};

} // namespace Omniscript

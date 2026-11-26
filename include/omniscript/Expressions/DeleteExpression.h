#pragma once
#include <omniscript/Expressions/Expression.h>

namespace Omniscript {

// `delete <operand>`: the operand evaluates to a pointer; the IR generator null-checks it,
// calls `<Struct>.destructor` when the pointee type has one, then frees the memory.
struct DeleteExpression : public Expression {
    std::shared_ptr<Expression> operand;

    explicit DeleteExpression(std::shared_ptr<Expression> operand) : operand(std::move(operand)) {
        type = Type::createInvalid();   // statement-like, produces no value (same as Break/Continue)
    }

    std::string toString() const override {
        return "Delete(" + (operand ? operand->toString() : std::string("null")) + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        auto copy = std::make_shared<DeleteExpression>(operand ? operand->clone() : nullptr);
        copy->setSpan(getSpan());
        return copy;
    }
};

} // namespace Omniscript

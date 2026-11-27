#pragma once

#include <omniscript/Statements/Statement.h>

namespace Omniscript {

// `delete <expr>;`
//
// Lowered by the IR generator (see DeleteExpression):
//   1. evaluate <expr> to a pointer value
//   2. if the pointer is null, do nothing (same as C++ `delete nullptr`)
//   3. if the pointee is a named struct with a `<Struct>.destructor`, call it
//   4. free the allocation
class DeleteStatement : public TypedStatement, public ASTExpression {
    friend class GenericInstantiator;   // generic body substitution must rewrite the operand
    std::shared_ptr<Statement> operand;
    bool isArray = false;   // `delete[] p;`
public:
    explicit DeleteStatement(std::shared_ptr<Statement> operand, bool isArray = false)
        : operand(std::move(operand)), isArray(isArray) {}

    const std::shared_ptr<Statement>& getOperand() const { return operand; }

    std::shared_ptr<Expression> express(SymbolTableType scope) override;

    bool getIsArray() const { return isArray; }

    std::string toString() const override {
        return std::string(isArray ? "delete[] " : "delete ") + (operand ? operand->toString() : std::string("<null>"));
    }

    std::string formatError(const std::string& msg) const override {
        return "Error in delete statement.\n" + msg;
    }

    std::shared_ptr<Statement> clone() const override {
        auto copy = std::make_shared<DeleteStatement>(operand ? operand->clone() : nullptr, isArray);
        copy->setSpan(getSpan());
        return copy;
    }
};

} // namespace Omniscript
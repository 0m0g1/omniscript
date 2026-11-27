#pragma once

#include <omniscript/Statements/Statement.h>

namespace Omniscript {

// `new T(args);` / `new T{ field: v }`  ->  NewExpression (type `T*`).
// Reuses ObjectConstructorStatement for default members, field initialisers and constructor
// resolution, so both stay in sync with `T{...}`. The IR generator then places the object on the heap.
class NewStatement : public TypedStatement, public ASTExpression {
    friend class GenericInstantiator;
    std::string typeName;
    std::vector<std::shared_ptr<Statement>> args;
    std::shared_ptr<Statement> count;   // `new T[count](args)`; nullptr for a single object
    public:
    NewStatement(std::string typeName, std::vector<std::shared_ptr<Statement>> args,
        std::shared_ptr<Statement> count = nullptr)
        : typeName(std::move(typeName)), args(std::move(args)), count(std::move(count)) {}
        
    std::shared_ptr<Statement> allocator;   // `new(a) T`: an Arena*; nullptr = heap
    std::shared_ptr<Expression> express(SymbolTableType scope) override;

    std::string toString() const override { return "new " + typeName; }

    std::string formatError(const std::string& msg) const override {
        return "Error in new expression.\n" + msg;
    }

    std::shared_ptr<Statement> clone() const override {
        std::vector<std::shared_ptr<Statement>> copied;
        for (const auto& a : args) copied.push_back(a ? a->clone() : nullptr);
        auto copy = std::make_shared<NewStatement>(typeName, copied, count ? count->clone() : nullptr);
        copy->setSpan(getSpan());
        return copy;
    }
};

} // namespace Omniscript
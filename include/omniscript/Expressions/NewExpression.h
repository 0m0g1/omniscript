#pragma once
#include <omniscript/Expressions/Expression.h>
#include <omniscript/Expressions/StructExpression.h>     // MemberExpression users
#include <omniscript/Expressions/CallableExpression.h>   // MemberExpression

namespace Omniscript {

// `new T(args)` / `new T{...}`: heap-allocates a T, initialises its fields, runs `T.constructor`
// (with `this` = the heap pointer) and yields a `T*`.
struct NewExpression : public Expression {
    std::string allocTypeName;                           // name of T as written
    std::string hiddenName;                              // scope name the ctor's `this` resolves through
    std::vector<std::shared_ptr<MemberExpression>> members;   // one initial value per field
    std::shared_ptr<Expression> ctorCall;                // nullptr when T has no constructor
    std::shared_ptr<Expression> count;                   // `new T[count]`: element count; nullptr for one object
    std::shared_ptr<Expression> allocator;               // `new(a) T`: an Arena*; nullptr = heap (malloc)

    NewExpression(std::string typeName, std::string hidden,
                  std::vector<std::shared_ptr<MemberExpression>> members,
                  std::shared_ptr<Expression> ctorCall,
                  std::shared_ptr<Type> pointerType,
                  std::shared_ptr<Expression> countExpr = nullptr)
        : allocTypeName(std::move(typeName)), hiddenName(std::move(hidden)),
          members(std::move(members)), ctorCall(std::move(ctorCall)), count(std::move(countExpr)) {
        type = pointerType;
        rootType = pointerType;
    }

    std::string toString() const override { return "New(" + allocTypeName + ")"; }

    std::shared_ptr<Expression> clone() const override {
        auto copy = std::make_shared<NewExpression>(allocTypeName, hiddenName, members, ctorCall, type,
                                                    count ? count->clone() : nullptr);
        copy->allocator = allocator ? allocator->clone() : nullptr;
        copy->setSpan(getSpan());
        return copy;
    }
};

} // namespace Omniscript
#pragma once
// Place at include/omniscript/Expressions/NullableExpressions.h
// Expression nodes for the `T?` intrinsics:  opt.value   and   opt.valueOr(d)
// Lowering (IRGenerator::createNullableValue / createNullableValueOr) lives in IRGeneratorOptionalHelpers.cpp.
#include <omniscript/Expressions/Expression.h>   // adjust to wherever `Expression` is declared (same header NullableExpression uses)

namespace Omniscript {

// `opt.value`: payload of a nullable, no check (C-style; the caller tests `!= null` first).
struct NullableValueExpression : public Expression {
    std::shared_ptr<Expression> optional;

    NullableValueExpression(std::shared_ptr<Expression> opt, std::shared_ptr<Type> innerType)
        : optional(std::move(opt)) {
        type = innerType;
        rootType = innerType;
    }

    std::string toString() const override {
        return "NullableValue(" + (optional ? optional->toString() : std::string("null")) + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullableValueExpression>(optional ? optional->clone() : nullptr, type);
    }
};

// `opt.valueOr(d)`: the payload when present, else `fallback`.
struct NullableValueOrExpression : public Expression {
    std::shared_ptr<Expression> optional;
    std::shared_ptr<Expression> fallback;

    NullableValueOrExpression(std::shared_ptr<Expression> opt, std::shared_ptr<Expression> fb,
                              std::shared_ptr<Type> innerType)
        : optional(std::move(opt)), fallback(std::move(fb)) {
        type = innerType;
        rootType = innerType;
    }

    std::string toString() const override {
        return "NullableValueOr(" + (optional ? optional->toString() : std::string("null")) + ", " +
               (fallback ? fallback->toString() : std::string("null")) + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullableValueOrExpression>(
            optional ? optional->clone() : nullptr, fallback ? fallback->clone() : nullptr, type);
    }
};

} // namespace Omniscript

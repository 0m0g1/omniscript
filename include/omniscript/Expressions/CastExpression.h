#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
struct CastExpression : public Expression {
    std::shared_ptr<Expression> targetExpr;
    std::shared_ptr<Type> castTargetType;

    CastExpression(std::shared_ptr<Expression> expr, std::shared_ptr<Type> targetType)
        : targetExpr(expr), castTargetType(targetType) {
        type = castTargetType;
    }

    std::string toString() const override {
        return "Cast<" + (castTargetType ? castTargetType->toString() : "unknown") +
               ">(" + (targetExpr ? targetExpr->toString() : "null") + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<CastExpression>(
            targetExpr ? targetExpr->clone() : nullptr,
            castTargetType ? castTargetType->clone() : nullptr
        );
    }
};
}
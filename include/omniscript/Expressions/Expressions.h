#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
struct BinaryExpression : public Expression {
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    Token op;

    BinaryExpression(std::shared_ptr<Expression> lhs, Token op, std::shared_ptr<Expression> rhs, std::shared_ptr<Type> resultType)
        : left(lhs), right(rhs), op(op) {
        this->type = resultType;
    }

    std::string toString() const override {
        return "(" + left->toString() + " " + op.getValue() + " " + right->toString() + ")";
        
    }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<BinaryExpression>(
            left ? left->clone() : nullptr,
            op,
            right ? right->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};

struct TernaryExpression : public Expression {
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> truthy;
    std::shared_ptr<Expression> falsey;

    TernaryExpression(std::shared_ptr<Expression> cond,
                            std::shared_ptr<Expression> ifTrue,
                            std::shared_ptr<Expression> ifFalse,
                            std::shared_ptr<Type> resultType)
        : condition(cond), truthy(ifTrue), falsey(ifFalse) {
        this->type = resultType;
    }

    std::string toString() const override {
        return "(" + condition->toString() + " ? " + truthy->toString() + " : " + falsey->toString() + ")";
    }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<TernaryExpression>(
            condition ? condition->clone() : nullptr,
            truthy ? truthy->clone() : nullptr,
            falsey ? falsey->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};

struct UnaryExpression : public Expression {
    Token op;
    std::shared_ptr<Expression> operand;
    bool isPrefix;

    UnaryExpression(
                    Token op,
                    std::shared_ptr<Expression> operand,
                    std::shared_ptr<Type> resultType,
                    bool isPrefix
                )
        : op(op), operand(operand), isPrefix(isPrefix) {
        this->type = resultType;
    }

    std::string toString() const override {
        Token opStr = op;
        if (isPrefix) {
            return  "(" + op.getValue() + (operand ? operand->toString() : "nulloperand") + ")";
        }
        return  "(" + (operand ? operand->toString() : "nulloperand") + op.getValue() + ")";
    }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<UnaryExpression>(
            op,
            operand ? operand->clone() : nullptr,
            type ? type->clone() : nullptr,
            isPrefix
        );
    }
};
}
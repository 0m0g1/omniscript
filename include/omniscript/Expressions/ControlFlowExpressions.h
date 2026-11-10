#pragma once
#include <omniscript/Expressions/Expression.h>

namespace Omniscript {
struct ReturnExpression : public Expression {
    std::shared_ptr<Expression> value;
    ReturnExpression(std::shared_ptr<Expression> value, std::shared_ptr<Type> returnType) : value(value) {
        type = returnType;
    }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ReturnExpression>(
            value ? value->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};

struct IfExpression : public Expression {
    std::vector<std::shared_ptr<Expression>> conditions;  
    std::vector<std::shared_ptr<Expression>> bodies;      
    std::shared_ptr<Expression> elseBody;                 

    IfExpression(
        std::vector<std::shared_ptr<Expression>> conditions, 
        std::vector<std::shared_ptr<Expression>> bodies,
        std::shared_ptr<Expression> elseBody = nullptr
    ) : conditions(conditions),
        bodies(bodies),
        elseBody(elseBody) {

        this->type = Type::createInvalid(); 
    }

    std::string toString() const override {
        std::string result = "IfExpression with " + std::to_string(conditions.size()) + " branches";

        for (size_t i = 0; i < conditions.size(); ++i) {
            result += "\n  if (" + conditions[i]->toString() + ") " + bodies[i]->toString();
        }

        if (elseBody) {
            result += "\n  else " + elseBody->toString();
        }

        return result;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedConditions;
        for (const auto& cond : conditions) {
            clonedConditions.push_back(cond->clone());
        }

        std::vector<std::shared_ptr<Expression>> clonedBodies;
        for (const auto& body : bodies) {
            clonedBodies.push_back(body->clone());
        }

        std::shared_ptr<Expression> clonedElseBody = nullptr;
        if (elseBody) {
            clonedElseBody = elseBody->clone();
        }

        return std::make_shared<IfExpression>(clonedConditions, clonedBodies, clonedElseBody);
    }
};

struct ForLoopExpression : public Expression {
    std::shared_ptr<Expression> initializer;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> increment;
    std::shared_ptr<Expression> body;

    ForLoopExpression(
        std::shared_ptr<Expression> initializer,
        std::shared_ptr<Expression> condition,
        std::shared_ptr<Expression> increment,
        std::shared_ptr<Expression> body
    ) : initializer(initializer),
        condition(condition),
        increment(increment),
        body(body) {}

    std::string toString() const override {
        return "ForLoop(init: " + (initializer ? initializer->toString() : "null") +
               ", cond: " + (condition ? condition->toString() : "null") +
               ", inc: " + (increment ? increment->toString() : "null") +
               ", body: " + (body ? body->toString() : "null") + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ForLoopExpression>(
            initializer ? initializer->clone() : nullptr,
            condition ? condition->clone() : nullptr,
            increment ? increment->clone() : nullptr,
            body ? body->clone() : nullptr
        );
    }
};

struct WhileLoopExpression : public Expression {
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> body;

    WhileLoopExpression(
        std::shared_ptr<Expression> condition,
        std::shared_ptr<Expression> body
    ) : condition(condition), body(body) {}

    std::string toString() const override {
        return "WhileLoop(cond: " + (condition ? condition->toString() : "null") +
               ", body: " + (body ? body->toString() : "null") + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<WhileLoopExpression>(
            condition ? condition->clone() : nullptr,
            body ? body->clone() : nullptr
        );
    }
};
}
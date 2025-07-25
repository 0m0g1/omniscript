#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
struct BlockExpression : public Expression {
    bool isGlobal = true;
    std::vector<std::shared_ptr<Expression>> values;  

    BlockExpression(std::vector<std::shared_ptr<Expression>> values)
        : values(values) {
        type = Type::createInvalid();
    }

    std::string toString() const override {
        std::string result = "Block: [ ";
        for (const auto& val : values) {
            result += val ? val->toString() : "null";
            result += " ";
        }
        result += "]";
        return result;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedValues;
        for (const auto& val : values) {
            clonedValues.push_back(val ? val->clone() : nullptr);
        }
        return std::make_shared<BlockExpression>(clonedValues);
    }
};
}
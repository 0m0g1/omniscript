#pragma once
#include <omniscript/Expressions/Expression.h>

namespace Omniscript {
struct TypeDeclarationExpression : public Expression {
    std::string originalTypeName;
    bool isAliasingOtherType = false;
    std::string typeName;

    TypeDeclarationExpression(std::string name, std::shared_ptr<Type> value) {
        this->typeName = name;
        this->type = value;
        this->rootType = value;
    }

    void setIsAliasing(const std::string& originalTypeName) {
        this->originalTypeName = originalTypeName;
        isAliasingOtherType = true;
    }

    bool isAliasing() const {
        return isAliasingOtherType;
    }

    std::string getOriginalTypeName() const {
        return originalTypeName;
    }

    std::string toString() const override {
        if (isAliasingOtherType) {
            return "Declare '" + typeName + "' Alias of type '" + originalTypeName + "' = '" + (type? type->toString() : "null") + "'"; 
        }
        return "Declare type '" + typeName + "' = '" + (type? type->toString() : "null") + "'";
    }
    
    std::shared_ptr<Expression> clone() const override {
        auto clone = std::make_shared<TypeDeclarationExpression>(typeName, type);
        return clone;
    }
};
}
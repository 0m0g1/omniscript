#pragma once
#include <omniscript/Core/Expression.h>

namespace Omniscript {
struct VariableAssignment : public Expression {
    bool isStatic = false;
    bool isConstant = false;
    bool isGlobal = true;
    bool isReassignment = false;
    bool isVolatile = false;
    std::string variableName;
    std::shared_ptr<Expression> assignedValue;

    VariableAssignment(std::string name, std::shared_ptr<Expression> value, bool isGlobal = false, bool isReassignment = false)
        : variableName(std::move(name)), assignedValue(std::move(value)), isGlobal(isGlobal), isReassignment(isReassignment) {
        type = assignedValue->type;  
    }

    std::shared_ptr<Expression> getValue() const { return assignedValue; }
    std::string toString() const override {
        return "Assign: " + variableName + " = " + assignedValue->toString();
    }
    
    std::shared_ptr<Expression> clone() const override {
        auto clone = std::make_shared<VariableAssignment>(
            variableName,
            assignedValue ? assignedValue->clone() : nullptr
        );
        clone->isStatic = isStatic;
        clone->isGlobal = isGlobal;
        clone->isConstant = isConstant;
        clone->isReassignment = isReassignment;
        clone->isVolatile = isVolatile;
        return clone;
    }
};
}
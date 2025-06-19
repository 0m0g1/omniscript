#pragma once
#include <omniscript/Core/Expression.h>

namespace Omniscript {
struct VariableAssignment : public Expression {
    bool isStatic;
    bool isConstant;
    bool isGlobal;
    bool isReassignment;
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
        return std::make_shared<VariableAssignment>(
            variableName,
            assignedValue ? assignedValue->clone() : nullptr
        );
    }
};
}
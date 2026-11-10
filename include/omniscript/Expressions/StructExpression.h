#pragma once
#include <omniscript/Types/DerivedTypes.h>
#include <omniscript/Expressions/Expression.h>
#include <omniscript/Expressions/CallableExpression.h>
#include <omniscript/Expressions/AggregateExpressions.h>

namespace Omniscript {
struct StructExpression : 
public Callable,
public AggregateExpression {
    std::string structName;
    std::vector<std::string> elementNames;

    StructExpression(
        const std::string& structName,
        const std::string& mangledName,
        const std::vector<std::shared_ptr<Expression>>& fields = {},
        const std::vector<std::string>& fieldNames = {},
        bool isVarArg = false
    )
        : Callable(structName, mangledName, fields, isVarArg),
          structName(structName),
          elementNames(fieldNames)
    {
        
        std::vector<std::shared_ptr<Type>> fieldTypes;
        for (auto& f : parameters)
            fieldTypes.push_back(f->type);

        type = std::make_shared<UserDefinedType>(name);
    }

    std::string toString() const override {
        return "Struct : " + structName;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedFields;
        for (const auto& field : parameters)
            clonedFields.push_back(field ? field->clone() : nullptr);

        return std::make_shared<StructExpression>(
            structName,
            mangledName,
            clonedFields,
            elementNames,
            isVarArg
        );
    }
};
}
#pragma once
#include <omniscript/Expression.h>
#include <omniscript/Expressions/FunctionInputExpression.h>

namespace Omniscript {
struct Callable : public virtual Expression {
    std::string mangledName;
    std::vector<std::shared_ptr<Expression>> parameters;
    bool isVarArg;

    Callable(
            const std::string& name,
            const std::string& mangledName,
            std::vector<std::shared_ptr<Expression>> params = {},
            bool isVarArg = false
        )
        : parameters(params), 
            isVarArg(isVarArg) {
        this->name = name;
        this->mangledName = mangledName;
    }

    
    std::vector<std::shared_ptr<FunctionInputExpression>> cloneParameters() const {
        std::vector<std::shared_ptr<FunctionInputExpression>> clonedParams;
        for (const auto& parameter : parameters) {
            clonedParams.push_back(std::dynamic_pointer_cast<FunctionInputExpression>(parameter->clone()));
        }
        return clonedParams;
    }

    std::vector<std::shared_ptr<Expression>> getParameters() const {
        return parameters;
    }

    std::string toString() const override {
        std::string paramsStr;
        for (const auto& param : parameters) {
            if (!paramsStr.empty()) paramsStr += ", ";
            paramsStr += param->toString();
        }
        return "Callable: " + name + "(" + paramsStr + ")";
    }

    
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedParams;
        for (const auto& param : parameters) {
            clonedParams.push_back(param ? param->clone() : nullptr);
        }
        return std::make_shared<Callable>(name, mangledName, clonedParams, isVarArg);
    }
};

struct CallExpression : public Expression {
    std::string calleeName;
    std::string functionTypeName;
    std::string instanceName;
    std::vector<std::shared_ptr<Expression>> args;
    std::vector<std::shared_ptr<MemberExpression>> members;

    bool isGlobal;

    CallExpression(const std::string& calleeName, const std::vector<std::shared_ptr<Expression>>& args = {}, std::shared_ptr<Type> returnType = nullptr)
    : calleeName(calleeName), args(args) {
        type = returnType;
    }

    CallExpression(const std::string& objectName,
        const std::string& instanceName,
        const std::vector<std::shared_ptr<Expression>>& args = {},
        std::shared_ptr<Type> returnType = nullptr,
        bool isGlobal = true
    )
    : calleeName(objectName), instanceName(instanceName), args(args), isGlobal(isGlobal) {
        type = returnType;
    }

    
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedArgs;
        for (const auto& arg : args) {
            clonedArgs.push_back(arg ? arg->clone() : nullptr);
        }
        return std::make_shared<CallExpression>(
            calleeName,
            instanceName,
            clonedArgs,
            type ? type->clone() : nullptr
        );
    }
    std::string toString() const override {
        if (instanceName.empty()) {
            return "Call: " + calleeName;
        }
        return "Call create instance '" + instanceName + "' of object '" + calleeName + "'.";
    }
};
}
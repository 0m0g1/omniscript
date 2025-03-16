#pragma once
#include <omniscript/runtime/Statement.h>
#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>

class Function : public Object {
    public:
        Function(
            // const std::string &name,
            // std::vector<std::string> paramNames,
            // std::vector<std::shared_ptr<Statement>> parameters,
            // std::vector<std::shared_ptr<Statement>> body,
            // std::optional<SymbolTable::ValueType> returnType = std::nullopt,
            // std::vector<std::pair<std::string, std::string>> typeParameters = {} // 🆕 Generic type params
        );
        // : name(name), paramNames(std::move(paramNames)), parameters(std::move(parameters)), 
        //   body(std::move(body)), returnType(returnType), typeParameters(std::move(typeParameters)) {}
    
        // // 🆕 Method to retrieve generic type parameters
        // const std::vector<std::pair<std::string, std::string>>& getTypeParameters() const {
        //     return typeParameters;
        // }
    
        // // 🆕 Check type constraints before function execution
        // void checkTypeConstraints(const std::vector<SymbolTable::ValueType>& args) const;
    
        // // Get the return value of the function
        // std::optional<SymbolTable::ValueType> getReturnValue() const {
        //     return returnValue;
        // }
    
        // // Clone the function
        // // std::shared_ptr<Function> clone() const {
        // //     return std::make_shared<Function>(*this);
        // // }
    
        // // Evaluate function execution
        // SymbolTable::ValueType evaluate(SymbolTable &scope, std::vector<SymbolTable::ValueType> args);
    
        // // Add a parameter dynamically
        // inline void addParameter(const std::string& parameterName, std::shared_ptr<Statement> newArg) {
        //     paramNames.push_back(parameterName);

        //     if (newArg != nullptr) {
        //         parameters.push_back(newArg);
        //     }
        // }

        //     std::string name;
        // private:
        //     std::vector<std::string> paramNames;
        //     std::vector<std::shared_ptr<Statement>> parameters;
        //     std::vector<std::shared_ptr<Statement>> body;
        
        //     std::optional<SymbolTable::ValueType> returnType;
        //     std::optional<SymbolTable::ValueType> returnValue; // Store the return value
        
        //     std::vector<std::pair<std::string, std::string>> typeParameters; // 🆕 Generic type parameters
    };
    
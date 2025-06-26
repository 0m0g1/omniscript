#pragma once
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Statements/CallableStatement.h>

class FunctionDeclaration : 
public Callable, 
public TypedStatement, 
public GenericHolder,
public ContextAwareStatement {
public:
    std::string mangledName;

    bool isRegistered = false;
    bool bodyCompiled = false;
    bool isIntrinsic = false;
    bool isExtern = false;
    bool isStatic = false;
    
    std::string externName;
    std::string intrinsicName;
    std::string staticLibPath;
    std::string dynamicLibPath;
    
    std::shared_ptr<Omniscript::Type> returnType;
    std::vector<std::pair<std::string, std::string>> typeParams; // Generic types
    std::vector<std::shared_ptr<Statement>> parameters;
    std::shared_ptr<BlockStatement> body;
    SymbolTableType localScope;

    FunctionDeclaration(
        const std::string& functionName,
        const std::vector<std::shared_ptr<Statement>>& parameters,
        std::shared_ptr<BlockStatement> body,
        std::shared_ptr<Omniscript::Type> returnType_ = nullptr // Default to nullptr if return type is unknown
    ) : parameters(parameters), body(body), Callable(parameters) {
        setType(std::move(returnType_)); // Store the return type using `TypedStatement`
        returnType = getType();
        setName(functionName);
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { 
        return "Decleration Function: " + name + " returns " + ( returnType? returnType->toString() : "void"); 
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in function '" + name + "'.\n" + msg;
    };
    std::string generateMangledName() const;

    void setReturnTypes();
    void setReturnTypesInStatement(
        const std::shared_ptr<Statement>& stmt, 
        std::shared_ptr<Omniscript::Type> returnType
    );

    void registerInScope(SymbolTableType scope);
    void compileBody(SymbolTableType scope);

    std::shared_ptr<Statement> clone() const override {
        std::vector<std::shared_ptr<Statement>> clonedParameters;
        for (const auto& param : parameters) {
            clonedParameters.push_back(param->clone());
        }

        std::shared_ptr<BlockStatement> clonedBody = body ? std::dynamic_pointer_cast<BlockStatement>(body->clone()) : nullptr;

        return std::make_shared<FunctionDeclaration>(name, clonedParameters, clonedBody, returnType);
    }
};
    
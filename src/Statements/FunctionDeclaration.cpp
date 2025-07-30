#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/ModuleAndImportStatements.h>
#include <omniscript/Statements/ClassConstructorStatement.h>
#include <omniscript/Statements/StructConstructorStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Types/Types.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Expressions/ClassExpression.h>
#include <omniscript/Expressions/StructExpression.h>
#include <omniscript/Expressions/CallableExpression.h>
#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Expressions/FunctionInputExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>

namespace Omniscript {

void FunctionDeclaration::registerInScope(SymbolTableType scope) {
    if (isRegistered) {
        return;
    }
    DEBUG_LOG();
    DEBUG_LOG("[Function] Constructing a function '" + name + "' prototype in '" + scope->getName() + "' the return Type is '" + type->toString() + "'.");

    DEBUG_LOG("[Function] Creating a local scope for the function");
    localScope = scope->createChildScope(name);

    if (name == "main") {
        name = "main";
    }

    std::vector<std::shared_ptr<TypeExpression>> genericTypes = createTypeExpressionListFromBoundGenerics();
    for (const auto& genericType : genericTypes) {
        localScope->setConstant(genericType->name, genericType);
    }

    DEBUG_LOG("[Function] Setting the function's return type");
    if (!type) {
        std::vector<std::string> retType = {"void"};
        type = resolveType(retType);
        returnType = type;
    } else if (type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            returnType = type;
            rootType = type;
            if (!type) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify type '%s' is defined in scope '%s'\n"
                    "2. Check for correct namespace imports\n"
                    "3. Ensure type is declared before use",
                    unresolved->joinedTypeString.c_str(), scope->getName().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Return type '%s' does not exist in scope '%s'",
                                     unresolved->joinedTypeString.c_str(), scope->getName().c_str()),
                    suggestion,
                    getSpan()
                );
                return;
            }
        }
    } else if (type->isGeneric()) {
        type = resolveGeneric(type->getName());
        returnType = type;
    } else {
        returnType = type;
    }
    
    DEBUG_LOG("[Function] Setting the function's body's return type to " + type->toString());
    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(body)) {
        typed->setType(returnType);
    }
    
    setReturnTypes();
    
    DEBUG_LOG("[Function] Extracting argument values for function type construction");
    std::vector<std::shared_ptr<Expression>> argValues;
    bool isVarArg = false;
    
    int paramIndex = 0;
    for (const auto& param : parameters) {
        DEBUG_LOG("[Function] The parameter is '" + param->toString() + "'.");
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(param)) {
            auto paramType = typed->getType();
            DEBUG_LOG("[Function] Parameter has type '" + paramType->toString() + "'.");
            
            if (paramType->isGeneric()) {
                typed->setType(resolveGeneric(paramType->getName()));
            }
        }
        auto result = param->express(localScope);
        if (!result) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify parameter '%s' is correctly defined\n"
                "2. Check parameter type and initialization\n"
                "3. Add debug output for parameter expression",
                param->toString().c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to evaluate parameter '%s'",
                                 param->toString().c_str()),
                suggestion,
                param->getSpan()
            );
            return;
        }
        if (auto paramStmt = std::dynamic_pointer_cast<ParameterStatement>(param)) {
            if (paramStmt->isVariadic) {
                isVarArg = true;
                DEBUG_LOG("The function is variadic");
            }
            if (paramIndex == 0 && paramStmt->getName() == "this") {
                std::dynamic_pointer_cast<FunctionInputExpression>(result)->isConstant = true;
            } else {
                std::dynamic_pointer_cast<FunctionInputExpression>(result)->isConstant = false;
            }
        }
        argValues.push_back(result);
        DEBUG_LOG("[Function] Parameter '" + result->name + "' has type " + result->getType()->toString());
        paramIndex++;
    }
    
    DEBUG_LOG("[Function] Passing generic type bindings from function to body block");
    if (auto holder = std::dynamic_pointer_cast<GenericHolder>(body)) {
        holder->inheritGenericsFrom(*this);
    }
    
    extendContextOf(body);
    
    std::vector<std::shared_ptr<Expression>> functionBody = {};

    std::string mangledName = (name == "main" ? "main" : generateMangledName());

    // Build the FunctionType object from param types and return type
    DEBUG_LOG("[Function] Building the function type");
    std::vector<std::shared_ptr<Type>> paramTypes;
    for (const auto& arg : argValues) {
        paramTypes.push_back(arg->getType());
    }
    
    DEBUG_LOG("[Function] Creating FunctionValue");
    auto functionVal = std::make_shared<FunctionExpression>(name, mangledName, returnType, functionBody, argValues, paramTypes, isVarArg);
    functionVal->mangledName = mangledName;
    functionVal->windowsDynamic = libraryPaths.windowsDynamic;
    functionVal->windowsStatic = libraryPaths.windowsStatic;
    functionVal->linuxShared = libraryPaths.linuxShared;
    functionVal->linuxStatic = libraryPaths.linuxStatic;
    functionVal->macosShared = libraryPaths.macosShared;
    functionVal->macosStatic = libraryPaths.macosStatic;
    functionVal->genericDynamic = libraryPaths.genericDynamic;
    functionVal->genericStatic = libraryPaths.genericStatic;
    functionVal->isExtern = isExtern;
    functionVal->externName = externName;
    functionVal->isIntrinsic = isIntrinsic;
    functionVal->intrinsicName = intrinsicName;
    functionVal->isVarArg = isVarArg;

    DEBUG_LOG("[Function] Storing overloaded function in scope '" + scope->getName() + "' under base name: " + name + " (mangled as: " + mangledName + ")");
    
    scope->addOverloadable(name, functionVal);
    this->mangledName = mangledName;
    isRegistered = true;
}

void FunctionDeclaration::compileBody(SymbolTableType scope) {
    if (bodyCompiled) {
        return;
    }

    auto overloads = scope->getOverloads(name);
    if (!overloads.empty()) {
        for (const auto& overload : overloads) {
            if (auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(overload)) {
                // if is external function don't add an args count parameter and change the variadic to an array
                if (!isExtern && !isIntrinsic) {
                    for (const auto& parameter : funcExpr->parameters) {
                        auto param = std::dynamic_pointer_cast<FunctionInputExpression>(parameter);
                        if (param->isVariadic) {
                            // override the variadic's name with a static array
                            auto argsCount = std::make_shared<Integer<int>>(0);
                            auto argsArray = std::make_shared<ArrayExpression>(param->getType());
                            DEBUG_LOG("The variadic parameter's name is '" + param->getName() + "' of type '" + param->getType()->toString() + "'.");
                            localScope->set(param->getName() + "_count", argsCount);
                            localScope->set(param->getName(), argsArray);
                            break;
                        }
                    }
                }
                if (mangledName == funcExpr->mangledName) {
                    body->isGlobal = false;
                    std::vector<std::shared_ptr<Expression>> functionBody = body->expressAsVector(localScope);
                    if (functionBody.empty()) {
                        std::string suggestion = Console::formatString(
                            "To resolve this:\n"
                            "1. Verify function body for '%s' contains valid statements\n"
                            "2. Check for correct block syntax\n"
                            "3. Add debug output to trace body evaluation",
                            name.c_str()
                        );
                        console.reportError(
                            Console::RUNTIME_ERROR,
                            Console::formatString("Failed to evaluate function body for '%s'",
                                             name.c_str()),
                            suggestion,
                            body->getSpan()
                        );
                        return;
                    }
                    funcExpr->body = functionBody;
                }
            }
        }
    } else {
        body->isGlobal = false;
        auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(scope->get(name));
        if (!funcExpr) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Ensure function '%s' is registered in scope '%s'\n"
                "2. Check for correct function declaration\n"
                "3. Verify scope hierarchy",
                name.c_str(), scope->getName().c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Function '%s' not found in scope '%s'",
                                 name.c_str(), scope->getName().c_str()),
                suggestion,
                getSpan()
            );
            return;
        }
        std::vector<std::shared_ptr<Expression>> functionBody = body->expressAsVector(localScope);
        if (functionBody.empty()) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify function body for '%s' contains valid statements\n"
                "2. Check for correct block syntax\n"
                "3. Add debug output to trace body evaluation",
                name.c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to evaluate function body for '%s'",
                                 name.c_str()),
                suggestion,
                body->getSpan()
            );
            return;
        }
        funcExpr->body = functionBody;
    }

    bodyCompiled = true;
}

std::shared_ptr<Expression> FunctionDeclaration::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    registerInScope(scope);
    
    if (!isExtern && !isIntrinsic) {
        compileBody(scope);
    }
    
    auto overloads = scope->getOverloads(name);
    for (const auto& overload : overloads) {
        if (auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(overload)) {
            if (mangledName == funcExpr->mangledName) {
                funcExpr->setSpan(this->getSpan());
                return funcExpr;
            }
        }
    }
    
    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Verify function '%s' is correctly defined\n"
        "2. Check for matching mangled name '%s'\n"
        "3. Ensure function is registered in scope '%s'",
        name.c_str(), mangledName.c_str(), scope->getName().c_str()
    );
    console.reportError(
        Console::RUNTIME_ERROR,
        Console::formatString("Failed compiling an overload '%s' for function / method '%s'",
                         mangledName.c_str(), name.c_str()),
        suggestion,
        getSpan()
    );
    return nullptr;
}

std::string FunctionDeclaration::generateMangledName() const {
    std::string mangled = name + "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(parameters[i])) {
            auto paramType = typed->getType();
            if (paramType) {
                if (paramType->isPointer()) {
                    auto baseType = paramType->getPointeeType();
                    mangled += baseType ? (baseType->toString() + "*") : "void*";
                } else {
                    mangled += paramType->toString();
                }
            } else {
                mangled += "unknown";
            }
        } else {
            mangled += "any";
        }
        if (i < parameters.size() - 1) mangled += ",";
    }
    mangled += ")";
    return mangled;
}

void FunctionDeclaration::setReturnTypes() {
    DEBUG_LOG("[Function] Setting the function's return type to " + returnType->toString());
    std::shared_ptr<Type> funcReturnType = getType();

    for (const auto& stmt : body->statements) {
        setReturnTypesInStatement(stmt, funcReturnType);
    }
}

void FunctionDeclaration::setReturnTypesInStatement(
    const std::shared_ptr<Statement>& stmt, 
    std::shared_ptr<Type> returnType
) {
    if (auto retStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        retStmt->setType(returnType);
        return;
    }

    if (auto block = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        for (const auto& subStmt : block->statements) {
            setReturnTypesInStatement(subStmt, returnType);
        }
    }
}

} // namespace Omniscript

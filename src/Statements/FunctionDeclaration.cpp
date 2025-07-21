#include <omniscript/Statement.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/ModuleAndImportStatements.h>
#include <omniscript/Statements/ClassConstructorStatement.h>
#include <omniscript/Statements/StructConstructorStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Expressions/ClassExpression.h>
#include <omniscript/Expressions/StructExpression.h>
#include <omniscript/Expressions/CallableExpression.h>
#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/FunctionInputExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>


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

    std::vector<std::shared_ptr<Omniscript::TypeExpression>> genericTypes = createTypeExpressionListFromBoundGenerics();
    for (const auto& genericType : genericTypes) {
        localScope->setConstant(genericType->name, genericType);
    }

    DEBUG_LOG("[Function] Setting the function's return type");
    if (!type) {
        std::vector<std::string> retType = {"void"};
        type = Omniscript::resolveType(retType);
        returnType = type;
    } else if (type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<Omniscript::UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            returnType = type;
            rootType = type;
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
    std::vector<std::shared_ptr<Omniscript::Expression>> argValues;
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
        if (auto paramStmt = std::dynamic_pointer_cast<ParameterStatement>(param)) {
            if (paramStmt->isVariadic) {
                isVarArg = true;
                DEBUG_LOG("The function is variadic");
            }
            if (paramIndex == 0 && paramStmt->getName() == "this") {
                std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(result)->isConstant = true;
            } else {
                std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(result)->isConstant = false;
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
    
    // auto bod = body->resolveExpressions(localScope);
    extendContextOf(body);
    
    std::vector<std::shared_ptr<Omniscript::Expression>> functionBody = {};
    // for (auto& stmt : body->statements) {
    //     functionBody.push_back(stmt->express(localScope));
    // }

    std::string mangledName = (name == "main" ? "main" : generateMangledName());

    // Build the FunctionType object from param types and return type
    DEBUG_LOG("[Function] Building the function type");
    std::vector<std::shared_ptr<Omniscript::Type>> paramTypes;
    for (const auto& arg : argValues) {
        paramTypes.push_back(arg->getType());
    }
    
    DEBUG_LOG("[Function] Creating FunctionValue");
    auto functionVal = std::make_shared<Omniscript::FunctionExpression>(name, mangledName, returnType, functionBody, argValues, paramTypes, isVarArg);
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
            if (auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overload)) {
                // if is external function don't add an args count parameter and change the variadic to an array
                if (!isExtern && !isIntrinsic) {
                    for (const auto& parameter : funcExpr->parameters) {
                        auto param = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(parameter);
                        if (param->isVariadic) {
                            // overide the variadics name with a static array
                            auto argsCount = std::make_shared<Omniscript::Integer<int>>(0);
                            auto argsArray = std::make_shared<Omniscript::ArrayExpression>(param->getType());
                            DEBUG_LOG("The variadic paramter's name is '" + param->getName() + "' of type '" + param->getType()->toString() + "'.");
                            localScope->set(param->getName() + "_count", argsCount);
                            localScope->set(param->getName(), argsArray);
                            break;
                        }
                    }
                }
                if (mangledName == funcExpr->mangledName) {
                    body->isGlobal = false;
                    std::vector<std::shared_ptr<Omniscript::Expression>> functionBody = body->expressAsVector(localScope);
                    funcExpr->body = functionBody;
                }
            }
        }
    } else {
        body->isGlobal = false;
        auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(scope->get(name));
        std::vector<std::shared_ptr<Omniscript::Expression>> functionBody = body->expressAsVector(localScope);
        funcExpr->body = functionBody;
    }

    bodyCompiled = true;
}

std::shared_ptr<Omniscript::Expression> FunctionDeclaration::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    registerInScope(scope);
    
    if (!isExtern && !isIntrinsic) {
        compileBody(scope);
    }
    
    auto overloads = scope->getOverloads(name);
    for (const auto& overload : overloads) {
        if (auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overload)) {
            if (mangledName == funcExpr->mangledName) {
                funcExpr->setPosition(getPosition());
                return funcExpr;
            }
        }
    }
    
    console.error("Failed compiling an overload '" + mangledName + "' for function / method '" + name + "'.");
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
    std::shared_ptr<Omniscript::Type> funcReturnType = getType();

    for (const auto& stmt : body->statements) {
        setReturnTypesInStatement(stmt, funcReturnType);
    }
}

void FunctionDeclaration::setReturnTypesInStatement(
    const std::shared_ptr<Statement>& stmt, 
    std::shared_ptr<Omniscript::Type> returnType
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
    // else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
    //     if (ifStmt->thenBranch)
    //         setReturnTypesInStatement(ifStmt->thenBranch, returnType);
    //     if (ifStmt->elseBranch)
    //         setReturnTypesInStatement(ifStmt->elseBranch, returnType);
    // }
    // else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
    //     if (whileStmt->body)
    //         setReturnTypesInStatement(whileStmt->body, returnType);
    // }
    // else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
    //     if (forStmt->body)
    //         setReturnTypesInStatement(forStmt->body, returnType);
    // }
}
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

// ============================== Prototypes  ============================== //
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
                typed->setType(std::move(resolveGeneric(paramType->getName())));
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

std::shared_ptr<Omniscript::Expression> ParameterStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("[Parameter] Creating parameter '" + name + "' of kind " + (type ? type->toString() : "undefined"));
    
    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<Omniscript::UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            rootType = type;
        }
    }

    std::shared_ptr<Omniscript::Expression> result;
    bool isValidDefaultValue = true;

    if (defaultValue) {
        auto typed = std::dynamic_pointer_cast<TypedStatement>(defaultValue);
        if (typed) {
            if (typed->getRootType()) {
                if (typed->getRootType()->isInvalid()) {
                    auto resultType = typed->clone()->express(scope)->getType();
                    isValidDefaultValue = !resultType->isInvalid();
                } else {
                    isValidDefaultValue = true;
                }
            } else if (typed->getType()) {
                if (typed->getType()->isInvalid()) {
                    auto resultType = typed->clone()->express(scope)->getType();
                    isValidDefaultValue = !resultType->isInvalid();
                } else {
                    isValidDefaultValue = true;
                }
            } else {
                auto clone = typed->clone();
                auto result = clone->express(scope);
                auto resultType = result->getType();
                isValidDefaultValue = true;
                // console.error("The the default value " + defaultValue->toString() + " of parameter '" + name + "' has no type.");
            }
        } else {
            isValidDefaultValue = false;
        }
    } else {
        isValidDefaultValue = false;
    }

    if (isValidDefaultValue) {
        DEBUG_LOG("The default value is " + defaultValue->toString());
        extendContextOf(defaultValue);
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(defaultValue)) {
            if (!type) {
                if (!typed->getType()) {
                    result = defaultValue->express(scope);
                    type = result->getType();
                } else {
                    type = typed->getType();
                    result = defaultValue->express(scope);
                }
                DEBUG_LOG("The inferred type is " + type->toString());
            } else {
                typed->setType(type);
                result = defaultValue->express(scope);
            }
        } else {
            // Not a TypedStatement — just evaluate it
            result = defaultValue->express(scope);
        }
    } else {
        // No default value — use null expression based on type
        if (type->isPointer()) {
            result = std::make_shared<Omniscript::NullPointerExpression>(type);
        } else {
            result = std::make_shared<Omniscript::NullExpression>(type);
        }
    }

    if (result->getType()) {
        DEBUG_LOG("[Parameter] Created value for parameter '" + name + "' of kind '" + result->getType()->toString() + "'.");
    } else {
        DEBUG_LOG("[Parameter] Created value for parameter '" + name + "' which is '" + result->toString() + "'.");
    }
    
    if (isConstant) {
        scope->setConstant(name, result);
    } else {
        scope->set(name, result);
    }
    
    DEBUG_LOG("[Parameter] Stored parameter '" + name + "' in scope '" + scope->getName() + "'.");
    
    auto param = std::make_shared<Omniscript::FunctionInputExpression>(name, type, result, isConstant);
    param->isVariadic = isVariadic;
    param->setPosition(getPosition());
    return param;
}

std::shared_ptr<Omniscript::Expression> ArgumentStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<Omniscript::UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            rootType = type;
            if (!type) {
                console.error("Type '" + unresolved->joinedTypeString + "' does not exist in scope '" + scope->getName() + "'.");
            }
        }
    }
    DEBUG_LOG("[Argument] Creating argument " + name);
    extendContextOf(value);
    std::shared_ptr<Omniscript::Expression> result;
    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
        if (type) {
            typed->setType(type);
            result = value->express(scope);
        } else {
            result = value->express(scope);
            setType(typed->getType());
        }
    }
    DEBUG_LOG("[Argument] The value for argument '" + name + "' is " + result->toString());
    auto arg = std::make_shared<Omniscript::FunctionInputExpression>(name, type, result);
    arg->setPosition(getPosition());
    return arg;
}

std::shared_ptr<Statement> ParameterStatement::getDefaultValue() {
    // if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(defaultValue)) {
    //     stmt->setType(type);
    // }
    return defaultValue;
    // return nullptr;
}

std::shared_ptr<Omniscript::Expression> ClassMember::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ConstructStructPrototype::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("[ConstructStructPrototype] Constructing a struct expression");

    std::vector<std::shared_ptr<Omniscript::Expression>> fields;
    std::vector<std::shared_ptr<Omniscript::Expression>> methods;
    std::vector<std::shared_ptr<FunctionDeclaration>> methodDeclr;
    std::vector<std::shared_ptr<Omniscript::Type>> fieldTypes;
    std::vector<std::string> fieldNames;

    SymbolTableType localScope = scope->createChildScope(name);

    for (const auto& field : body) {
        if (auto paramDecl = std::dynamic_pointer_cast<ParameterStatement>(field)) {
            std::string fieldName = paramDecl->getName();
            fieldNames.push_back(fieldName);

            if (paramDecl->getDefaultValue()) {
                
                std::shared_ptr<Omniscript::Expression> fieldExpr = paramDecl->express(localScope);
    
                fields.push_back(fieldExpr);
                fieldExpr->getType()->parameterName = fieldName;
                fieldTypes.push_back(fieldExpr->getType());
                DEBUG_LOG("Parameter '" + fieldName + "' has type " + fieldExpr->getType()->toString());
            }

        } else {
            if (auto method = std::dynamic_pointer_cast<FunctionDeclaration>(field)) {
                    methodDeclr.push_back(method);
            } else {
                console.warn("Skipping non-method and non-field declaration in struct body");
            }
        }
    }

    auto structType = Omniscript::Type::createUserDefinedType(name, Omniscript::Kind::Struct, fieldTypes);
    scope->addType(name, structType);
    
    setType(structType);
    setRootType(structType);

    // Phase 1: Register all methods (e.g., for mutual recursion or early references)
    for (const auto& field : methodDeclr) {
        auto thisParam = std::make_shared<ParameterStatement>("this");
        thisParam->setType(Omniscript::Type::createPointerType(getType()));
        field->parameters.insert(field->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));
        field->registerInScope(scope);
    }

    // 🧱 Construct the StructExpression as a Callable
    auto structExpr = std::make_shared<Omniscript::StructExpression>(
        getName(),
        getName(),
        fields,
        fieldNames,
        /* isVarArg */ false
    );

    scope->set(getName(), structExpr);
    structExpr->setPosition(getPosition());

    // Phase 2: Compile methods and build method expressions
    for (const auto& field : methodDeclr) {
        auto thisParam = std::make_shared<ParameterStatement>("this");
        thisParam->setType(Omniscript::Type::createPointerType(scope->getType(name)));

        // Insert 'this' as the first parameter
        field->parameters.insert(field->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));

        // Compile the method to an expression (LLVM function pointer, etc.)
        std::shared_ptr<Omniscript::Expression> method = field->express(scope);
        methods.push_back(method);
    }
    
    std::vector<std::shared_ptr<Omniscript::Expression>> stmts = {structExpr};

    for (const auto& method : methods) {
        stmts.push_back(method);
    }
    
    auto block = std::make_shared<Omniscript::BlockExpression>(stmts);

    return block;
}

std::shared_ptr<Omniscript::Expression> ConstructClassPrototype::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG();
    DEBUG_LOG("[ConstructClassPrototype] Constructing a class '" + getName() + "'.");

    std::vector<std::shared_ptr<Omniscript::Expression>> fields;

    std::vector<std::shared_ptr<Omniscript::FunctionExpression>> constructors;
    std::shared_ptr<Omniscript::FunctionExpression> destructor = nullptr;

    // Step 2: Create class type
    auto classType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(Omniscript::Type::createUserDefinedType(name, Omniscript::Kind::Class));
    scope->addType(name, classType);
    DEBUG_LOG("Added class type '" + name + "' to the scope");

    auto structExpr = std::make_shared<Omniscript::StructExpression>(
        name,
        name
    );

    auto classExpr = std::make_shared<Omniscript::ClassExpression>(name, structExpr);
    scope->set(name, classExpr);

    classExpr->type = classType;
    structExpr->type = classType;

    SymbolTableType localScope = scope->createChildScope(name);

    // Step 1: Process parameters (fields)
    for (const auto& member : body) {
        if (auto method = std::dynamic_pointer_cast<FunctionDeclaration>(member->getDefaultValue())) {
            continue;
        }
        auto param = std::make_shared<ParameterStatement>(
            member->getName(),
            member->getDefaultValue(),
            false
        );
        if (member->getType()) {
            param->setType(member->getType());
        }
        std::string fieldName = param->getName();
        structExpr->elementNames.push_back(fieldName);

        std::shared_ptr<Omniscript::Expression> fieldExpr = param->express(localScope);
        fields.push_back(fieldExpr);
        fieldExpr->getType()->parameterName = fieldName;
        classType->paramTypes.push_back(fieldExpr->getType());
        structExpr->parameters.push_back(fieldExpr);

        auto classMemberExpr = std::make_shared<Omniscript::ClassMemberExpression>(
            member->getName(),
            fieldExpr,
            member->getModifiers()
        );

        classExpr->members.push_back(classMemberExpr);
    }

    // Step 3: Process methods (functions, constructor, destructor)
    // Step 3.1: Register all methods (including constructor and destructor) in the current scope
    for (const auto& member : body) {
        if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(member->getDefaultValue())) {
            // Add implicit 'this' parameter before registration
            auto thisParam = std::make_shared<ParameterStatement>("this");
            thisParam->setType(Omniscript::Type::createPointerType(classType));
            func->parameters.insert(func->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));

            func->registerInScope(scope); // Register prototype in the current scope
        }
    }

    // Step 3.2: Compile method bodies and build expressions
    for (const auto& member : body) {
        if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(member->getDefaultValue())) {
            auto funcName = func->getName();

            func->compileBody(scope); // Compile function body into expressions

            auto methodExpr = func->express(scope); // Retrieve compiled function expression
            if (!methodExpr) {
                console.error("Failed to generate function expression for: " + funcName);
                continue;
            }

            DEBUG_LOG("Is " + funcName + " = " + name + ".constructor?");
            if (funcName == name + ".constructor") {
                auto ctorExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(methodExpr);
                classExpr->constructors.push_back(ctorExpr);
            } else if (funcName == name + ".destructor") {
                auto dtorExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(methodExpr);
                classExpr->destructor = dtorExpr;
            }

            // Wrap method expression into a class member
            auto classMemberExpr = std::make_shared<Omniscript::ClassMemberExpression>(
                member->getName(),
                methodExpr,
                member->getModifiers()
            );

            structExpr->parameters.push_back(methodExpr);       // Add to internal structure
            classExpr->members.push_back(classMemberExpr);      // Add to class definition
        }
    }

    classExpr->parameters = structExpr->parameters;
    classExpr->setPosition(getPosition());
    return classExpr;
}
 
std::shared_ptr<Omniscript::Expression> ObjectConstructorStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("Constructing '" + instanceName + "' of type '" + objectType + "'.");

    // std::vector<std::shared_ptr<Omniscript::Expression>> argValues;
    // for (const auto& arg : constructorArgs) {
    //     argValues.push_back(arg->express(scope));
    // }

    if (scope->getType(objectType)) {
        type = std::make_shared<Omniscript::UserDefinedType>(objectType);
        auto constructorCall = std::make_shared<Call>(objectType, "", constructorArgs);
        auto call = std::dynamic_pointer_cast<Omniscript::CallExpression>(constructorCall->express(scope));

        auto instance = std::make_shared<Omniscript::InstanceExpression>(
            objectType,
            instanceName,
            call->members
        );

        instance->instanceType = scope->getType(objectType);
        instance->type = scope->getType(objectType);
        setType(instance->type);
        setRootType(type);
        scope->set(instanceName, instance);
        call->setPosition(getPosition());
        return call;
    } else {
        console.error("Object type was not found in the scope");
    }

    // // 5. Return the allocated instance
    // return std::make_shared<Omniscript::CallExpression>(callee, finalArgs, type);
    return nullptr;
    // return std::make_shared<Omniscript::CallExpression>(objectType, instanceName, argValues); //, type);
}

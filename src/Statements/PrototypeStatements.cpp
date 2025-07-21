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

    if (!scope->getType(objectType)) {
        console.error("Object type was not found in the scope");
    }

    type = std::make_shared<Omniscript::UserDefinedType>(objectType);
    auto constructorCall = std::make_shared<Call>(objectType, instanceName, constructorArgs);
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
    instance->setPosition(getPosition());

    if (!scope->getOverloads(objectType + ".constructor").empty()) {
        auto overloads = scope->getOverloads(objectType + ".constructor");
        std::vector<std::shared_ptr<Omniscript::Expression>> ctorExpressions;
        ctorExpressions.push_back(instance);
        auto thisArg = std::make_shared<ReferenceTo>(instanceName);
        auto thisArgType = scope->getType(objectType);
        thisArg->setType(Omniscript::Type::createPointerType(thisArgType));
        thisArg->setRootType(thisArg->getType());
        constructorArgs.insert(constructorArgs.begin(), thisArg);
        DEBUG_LOG("The 'this' arg is of instance '" + instanceName + "' and of type '" + thisArg->getType()->toString() + "'.");
        auto ctorCall = std::make_shared<Call>(objectType + ".constructor", instanceName, constructorArgs)->express(scope);
        ctorExpressions.push_back(ctorCall);
        auto constructionBlock = std::make_shared<Omniscript::BlockExpression>(ctorExpressions);
        constructionBlock->setPosition(getPosition());
        return constructionBlock;
    }
    return instance;
}

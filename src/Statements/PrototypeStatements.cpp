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
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Expressions/ClassExpression.h>
#include <omniscript/Expressions/BlockExpression.h>
#include <omniscript/Expressions/StructExpression.h>
#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Expressions/CallableExpression.h>
#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/FunctionInputExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>

namespace Omniscript {

std::shared_ptr<Expression> ParameterStatement::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG("[Parameter] Creating parameter '" + name + "' of kind " + (type ? type->toString() : "undefined"));
    
    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
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
                    Console::formatString("Type '%s' does not exist in scope '%s'",
                                     unresolved->joinedTypeString.c_str(), scope->getName().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        }
    }

    std::shared_ptr<Expression> result;
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
                // setType(resultType);
                isValidDefaultValue = true;
                // Previously: console.error("The the default value " + defaultValue->toString() + " of parameter '" + name + "' has no type.");
                if (!resultType) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Ensure default value '%s' has a defined type\n"
                        "2. Check for proper type annotations\n"
                        "3. Verify expression initialization",
                        defaultValue->toString().c_str()
                    );
                    console.reportError(
                        Console::TYPE_ERROR,
                        Console::formatString("The default value '%s' of parameter '%s' has no type",
                                         defaultValue->toString().c_str(), name.c_str()),
                        suggestion,
                        defaultValue->getSpan()
                    );
                }
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
            result = std::make_shared<NullPointerExpression>(type);
        } else {
            result = std::make_shared<NullExpression>(type);
        }
    }

    if (result->getType()) {
        DEBUG_LOG("[Parameter] Created value for parameter '" + name + "' which is of type '" + result->getType()->toString() + "'.");
        if (type->isInvalid()) {
            type = result->getType();
        }
    } else {
        DEBUG_LOG("[Parameter] Created value for parameter '" + name + "' which is '" + result->toString() + "'.");
    }
    
    if (isConstant) {
        scope->setConstant(name, result);
    } else {
        scope->set(name, result);
    }
    
    DEBUG_LOG("[Parameter] Stored parameter '" + name + "' in scope '" + scope->getName() + "'.");
    
    auto param = std::make_shared<FunctionInputExpression>(name, type, result, isConstant);
    type = param->type;
    param->isVariadic = isVariadic;
    param->setSpan(this->getSpan());
    return param;
}

std::shared_ptr<Expression> ArgumentStatement::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
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
                    Console::formatString("Type '%s' does not exist in scope '%s'",
                                     unresolved->joinedTypeString.c_str(), scope->getName().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        }
    }
    DEBUG_LOG("[Argument] Creating argument " + name);
    extendContextOf(value);
    std::shared_ptr<Expression> result;
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
    auto arg = std::make_shared<FunctionInputExpression>(name, type, result);
    arg->setSpan(this->getSpan());
    return arg;
}

std::shared_ptr<Statement> ParameterStatement::getDefaultValue() {
    return defaultValue;
}

std::shared_ptr<Expression> ClassMember::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    return nullptr;
}

std::shared_ptr<Expression> ConstructStructPrototype::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG("[ConstructStructPrototype] Constructing a struct " + name + " expression");

    std::vector<std::shared_ptr<Expression>> fields;
    std::vector<std::shared_ptr<Expression>> methods;
    std::vector<std::shared_ptr<FunctionDeclaration>> methodDeclr;
    std::vector<std::shared_ptr<Type>> fieldTypes;
    std::vector<std::string> fieldNames;

    SymbolTableType localScope = scope->createChildScope(name);

    for (const auto& field : body) {
        if (auto paramDecl = std::dynamic_pointer_cast<ParameterStatement>(field)) {
            std::string fieldName = paramDecl->getName();
            fieldNames.push_back(fieldName);

            if (paramDecl->getDefaultValue()) {
                
                if (paramDecl->getType()) {
                    DEBUG_LOG("Parameter '" + fieldName + "' has type " + paramDecl->getType()->toString());
                }

                std::shared_ptr<Expression> fieldExpr = paramDecl->express(localScope);
                if (!fieldExpr) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Verify field '%s' is correctly defined\n"
                        "2. Check field type and default value\n"
                        "3. Add debug output for field expression",
                        fieldName.c_str()
                    );
                    console.reportError(
                        Console::RUNTIME_ERROR,
                        Console::formatString("Failed to evaluate field '%s' in struct '%s'",
                                         fieldName.c_str(), name.c_str()),
                        suggestion,
                        paramDecl->getSpan()
                    );
                    return nullptr;
                }
    
                fields.push_back(fieldExpr);
                fieldExpr->getType()->parameterName = fieldName;
                fieldTypes.push_back(fieldExpr->getType());
                DEBUG_LOG("Parameter '" + fieldName + "' has type " + fieldExpr->getType()->toString());
            }

        } else {
            if (auto method = std::dynamic_pointer_cast<FunctionDeclaration>(field)) {
                methodDeclr.push_back(method);
            } else {
                std::string suggestion = "To resolve this:\n"
                                       "1. Ensure struct body contains only fields or methods\n"
                                       "2. Check for valid struct member declarations\n"
                                       "3. Verify syntax of struct body";
                console.reportError(
                    Console::SEMANTIC_ERROR,
                    "Skipping non-method and non-field declaration in struct body",
                    suggestion,
                    field->getSpan()
                );
            }
        }
    }

    auto structType = Type::createUserDefinedType(name, Kind::Struct, fieldTypes);
    scope->addType(name, structType);
    
    setType(structType);
    setRootType(structType);

    // Phase 1: Register all methods (e.g., for mutual recursion or early references)
    for (const auto& field : methodDeclr) {
        auto thisParam = std::make_shared<ParameterStatement>("this");
        thisParam->setType(Type::createPointerType(getType()));
        field->parameters.insert(field->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));
        field->registerInScope(scope);
    }

    // 🧱 Construct the StructExpression as a Callable
    auto structExpr = std::make_shared<StructExpression>(
        getName(),
        getName(),
        fields,
        fieldNames,
        /* isVarArg */ false
    );

    scope->set(getName(), structExpr);
    structExpr->setSpan(this->getSpan());

    // Phase 2: Compile methods and build method expressions
    for (const auto& field : methodDeclr) {
        auto thisParam = std::make_shared<ParameterStatement>("this");
        thisParam->setType(Type::createPointerType(scope->getType(name)));

        // Insert 'this' as the first parameter
        field->parameters.insert(field->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));

        // Compile the method to an expression (LLVM function pointer, etc.)
        std::shared_ptr<Expression> method = field->express(scope);
        if (!method) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify method in struct '%s' is correctly defined\n"
                "2. Check method body and parameters\n"
                "3. Add debug output for method compilation",
                name.c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to compile method in struct '%s'",
                                 name.c_str()),
                suggestion,
                field->getSpan()
            );
            return nullptr;
        }
        methods.push_back(method);
    }
    
    std::vector<std::shared_ptr<Expression>> stmts = {structExpr};

    for (const auto& method : methods) {
        stmts.push_back(method);
    }
    
    auto block = std::make_shared<BlockExpression>(stmts);
    block->setSpan(this->getSpan());

    return block;
}

std::shared_ptr<Expression> ConstructClassPrototype::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG();
    DEBUG_LOG("[ConstructClassPrototype] Constructing a class '" + getName() + "'.");

    std::vector<std::shared_ptr<Expression>> fields;

    std::vector<std::shared_ptr<FunctionExpression>> constructors;
    std::shared_ptr<FunctionExpression> destructor = nullptr;

    // Step 2: Create class type
    auto classType = std::dynamic_pointer_cast<UserDefinedType>(Type::createUserDefinedType(name, Kind::Class));
    scope->addType(name, classType);
    DEBUG_LOG("Added class type '" + name + "' to the scope");

    auto structExpr = std::make_shared<StructExpression>(
        name,
        name
    );

    auto classExpr = std::make_shared<ClassExpression>(name, structExpr);
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

        std::shared_ptr<Expression> fieldExpr = param->express(localScope);
        if (!fieldExpr) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify field '%s' in class '%s' is correctly defined\n"
                "2. Check field type and default value\n"
                "3. Add debug output for field expression",
                fieldName.c_str(), name.c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to evaluate field '%s' in class '%s'",
                                 fieldName.c_str(), name.c_str()),
                suggestion,
                param->getSpan()
            );
            return nullptr;
        }
        fields.push_back(fieldExpr);
        fieldExpr->getType()->parameterName = fieldName;
        classType->paramTypes.push_back(fieldExpr->getType());
        structExpr->parameters.push_back(fieldExpr);

        auto classMemberExpr = std::make_shared<ClassMemberExpression>(
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
            thisParam->setType(Type::createPointerType(classType));
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
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify method '%s' in class '%s' is correctly defined\n"
                    "2. Check method body and parameters\n"
                    "3. Add debug output for method compilation",
                    funcName.c_str(), name.c_str()
                );
                console.reportError(
                    Console::RUNTIME_ERROR,
                    Console::formatString("Failed to generate function expression for: '%s'",
                                     funcName.c_str()),
                    suggestion,
                    func->getSpan()
                );
                continue;
            }

            DEBUG_LOG("Is " + funcName + " = " + name + ".constructor?");
            if (funcName == name + ".constructor") {
                auto ctorExpr = std::dynamic_pointer_cast<FunctionExpression>(methodExpr);
                classExpr->constructors.push_back(ctorExpr);
            } else if (funcName == name + ".destructor") {
                auto dtorExpr = std::dynamic_pointer_cast<FunctionExpression>(methodExpr);
                classExpr->destructor = dtorExpr;
            }

            // Wrap method expression into a class member
            auto classMemberExpr = std::make_shared<ClassMemberExpression>(
                member->getName(),
                methodExpr,
                member->getModifiers()
            );

            structExpr->parameters.push_back(methodExpr);       // Add to internal structure
            classExpr->members.push_back(classMemberExpr);      // Add to class definition
        }
    }

    classExpr->parameters = structExpr->parameters;
    classExpr->setSpan(this->getSpan());
    return classExpr;
}
 
std::shared_ptr<Expression> ObjectConstructorStatement::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG("Constructing '" + instanceName + "' of type '" + objectType + "'.");
    
    if (!scope->getType(objectType)) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify type '%s' is defined in scope '%s'\n"
            "2. Check for correct namespace imports\n"
            "3. Ensure type is declared before use",
            objectType.c_str(), scope->getName().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Object type '%s' was not found in the scope",
                             objectType.c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    
    type = scope->getType(objectType);
    
    // Always create instance with default values first (empty args)
    std::vector<std::shared_ptr<Statement>> emptyArgs = {};
    auto constructorCall = std::make_shared<Call>(objectType, emptyArgs);
    auto call = std::dynamic_pointer_cast<CallExpression>(constructorCall->express(scope));
    
    if (!call) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify type '%s' has a valid default constructor\n"
            "2. Check constructor accessibility\n"
            "3. Add debug output for constructor call",
            objectType.c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Failed to create instance of '%s'",
                             objectType.c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    
    // In ObjectConstructorStatement::express()
    std::string actualInstanceName = instanceName;
    if (actualInstanceName.empty()) {
        // Generate a unique name for anonymous instances
        static int anonymousCounter = 0;
        actualInstanceName = "__anonymous_" + objectType + "_" + std::to_string(anonymousCounter++);
    }

    auto instance = std::make_shared<InstanceExpression>(
        objectType,
        actualInstanceName,  // Always has a name now
        call->members
    );

    // Store in scope with the actual name
    scope->set(actualInstanceName, instance);
    
    instance->instanceType = scope->getType(objectType);
    instance->type = scope->getType(objectType);
    setType(instance->type);
    setRootType(type);
    scope->set(instanceName, instance);
    call->setSpan(this->getSpan());
    instance->setSpan(this->getSpan());
    
    // Check if constructor exists and we have arguments to pass to it
    if (!scope->getOverloads(objectType + ".constructor").empty()) {
        DEBUG_LOG("Found constructor for '" + objectType + "', calling with provided arguments");
        
        std::vector<std::shared_ptr<Expression>> ctorExpressions;
        ctorExpressions.push_back(instance);
        
        // Create 'this' argument for constructor
        auto thisArg = std::make_shared<ReferenceTo>(actualInstanceName);
        auto thisArgType = scope->getType(objectType);
        thisArg->setType(Type::createPointerType(thisArgType));
        thisArg->setRootType(thisArg->getType());
        
        // Create copy of constructor args and insert 'this' at the beginning
        std::vector<std::shared_ptr<Statement>> ctorArgs = constructorArgs;
        ctorArgs.insert(ctorArgs.begin(), thisArg);
        
        DEBUG_LOG("The 'this' arg is of instance '" + instanceName + "' and of type '" + thisArg->getType()->toString() + "'.");
        
        // Call constructor with the provided arguments
        auto ctorCall = std::make_shared<Call>(objectType + ".constructor", instanceName, ctorArgs)->express(scope);
        if (!ctorCall) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify constructor for '%s' accepts provided arguments\n"
                "2. Check argument types and count\n"
                "3. Add debug output for constructor arguments",
                objectType.c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to call constructor for '%s'",
                                 objectType.c_str()),
                suggestion,
                getSpan()
            );
            return nullptr;
        }
        
        ctorExpressions.push_back(ctorCall);
        auto constructionBlock = std::make_shared<BlockExpression>(ctorExpressions);
        constructionBlock->setSpan(this->getSpan());
        return constructionBlock;
    }
    
    // No constructor found
    if (!constructorArgs.empty()) {
        DEBUG_LOG("No constructor found but arguments provided - this might be field initialization");
        // For now, just return the instance and let the system handle field initialization
        // The arguments should be used for direct field assignment if possible
    }
    
    return instance;
}

} // namespace Omniscript

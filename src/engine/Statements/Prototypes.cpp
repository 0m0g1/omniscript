#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>


// ============================== Prototypes  ============================== //


std::shared_ptr<Omniscript::Expression> Call::express(SymbolTableType scope) {
    DEBUG_LOG();
    auto named = std::dynamic_pointer_cast<NamedStatement>(expr);
    std::string targetName = named ? named->getName() : instanceName;
    DEBUG_LOG("The target name is: " + targetName);
    if (!targetName.empty()) {
        if (auto obj = scope->get(targetName)) {
            std::string typeName = obj->getType()->getName();
            if (typeName == callee) {
                std::vector<std::shared_ptr<Omniscript::Expression>> ctorExpressions;
                auto realExpr = std::make_shared<ObjectConstructorStatement>(nullptr, typeName, instanceName, args);
                auto objConstructor = realExpr->express(scope);
                ctorExpressions.push_back(objConstructor);
                auto ctorCall = std::make_shared<Call>("constructor", instanceName, args)->express(scope);
                auto callExpr = std::dynamic_pointer_cast<Omniscript::CallExpression>(ctorCall);
                callExpr->instanceName = "";
                ctorExpressions.push_back(ctorCall);
                auto constructionBlock = std::make_shared<Omniscript::BlockExpression>(ctorExpressions);
                return constructionBlock;
            }
            callee = typeName + "." + callee;
            auto thisArg = std::make_shared<AddressOf>(instanceName);
            thisArg->setType(Omniscript::Type::createPointerType(obj->getType()));
            thisArg->setRootType(thisArg->getType());
            args.insert(args.begin(), thisArg);
            DEBUG_LOG("The 'this' arg is " + thisArg->getType()->pointerDescription());
        }
    }
    
    DEBUG_LOG("[Call] Evaluating call to '" + callee + "'");
    if (!instanceName.empty()) {
        DEBUG_LOG("For " + instanceName);
    }
    
    std::string originalCallee = callee;
    DEBUG_LOG("[Call] Looking up callee '" + originalCallee + "' in scope");
    std::shared_ptr<Omniscript::Expression> called;

    // Attempt overload resolution
    DEBUG_LOG("[Call] Attempting overload resolution for '" + originalCallee + "'");
    auto overloads = scope->getOverloads(originalCallee);
    if (!overloads.empty()) {
        DEBUG_LOG("[Call] Found " + std::to_string(overloads.size()) + " overload candidates");

        for (auto& overload : overloads) {
            auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overload);
            if (!funcExpr) {
                DEBUG_LOG("[Call] Skipping non-function overload.");
                continue;
            }
        
            DEBUG_LOG("[Call] Checking if the overload '" + funcExpr->mangledName + "' (" + funcExpr->name + ") is the required overload.");
        
            auto paramList = funcExpr->getParameters();
            DEBUG_LOG("[Call] Function '" + funcExpr->mangledName + "' expects " + std::to_string(paramList.size()) + " parameters.");
        
            // First, collect parameter information
            std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> inputParams;
            for (const auto& param : paramList) {
                auto casted = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(param);
                if (!casted) {
                    console.error(formatError("Failed to cast parameter to FunctionInputExpression."));
                    continue;
                }
                inputParams.push_back(casted);
                DEBUG_LOG("[Call] Parameter '" + casted->name + "' of type '" + casted->getType()->kindName() + "'");
            }

            // Then, set argument types based on parameters before evaluation
            std::unordered_set<std::string> providedParams;
            size_t positionalArgIndex = 0;
            bool typeMismatch = false;

            for (const auto& param : inputParams) {
                const std::string& paramName = param->name;
                std::shared_ptr<Statement> matchingArg;

                // Find matching argument
                for (const auto& arg : args) {
                    if (auto namedArg = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                        if (namedArg->getName() == paramName) {
                            matchingArg = namedArg->value;
                            providedParams.insert(paramName);
                            break;
                        }
                    }
                }

                if (!matchingArg && positionalArgIndex < args.size()) {
                    matchingArg = args[positionalArgIndex++];
                }

                if (matchingArg) {
                    if (auto typedArg = std::dynamic_pointer_cast<TypedStatement>(matchingArg)) {
                        if (!typedArg->getType() || 
                            (!Omniscript::isSameOrCastableTo(typedArg->getRootType(), param->getType()) &&
                             !Omniscript::isSameOrCastableTo(typedArg->getType(), param->getType()))) {
                            // Set the expected type before evaluation
                            typedArg->setType(param->getType());
                            typedArg->setRootType(param->getType());
                            DEBUG_LOG("[Call] Set type for argument to match parameter '" + paramName + 
                                     "': " + param->getType()->kindName());
                        }
                    }
                }
            }

            // Now evaluate arguments with proper types set
            std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> evaluatedArgs;
            for (const auto& arg : args) {
                std::shared_ptr<Omniscript::Expression> result;
                if (auto argStatement = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                    result = argStatement->value->express(scope);
                    DEBUG_LOG("[Call] Evaluated a named argument '" + argStatement->getName() + "'.");
                    auto inputExpr = std::make_shared<Omniscript::FunctionInputExpression>(argStatement->getName(), result->getType(), result);
                    evaluatedArgs.push_back(inputExpr);
                } else {
                    result = arg->express(scope);
                    DEBUG_LOG("[Call] Evaluated an unamed argument");
                    auto inputExpr = std::make_shared<Omniscript::FunctionInputExpression>("", result->getType(), result);
                    evaluatedArgs.push_back(inputExpr);
                }
                if (auto typed = std::dynamic_pointer_cast<TypedStatement>(arg)) {
                    DEBUG_LOG("[Call] Evaluated argument has type: " + typed->getType()->kindName() + "'.");
                }
            }
        
            if (matchArgumentsToParameters(evaluatedArgs, inputParams, scope)) {
                DEBUG_LOG("[Call] ✅ Matched overload: using mangled name '" + funcExpr->mangledName + "'");
                callee = funcExpr->mangledName;
                called = funcExpr;
                DEBUG_LOG("[Call] Called is now " + called->toString());
                break;
            } else {
                DEBUG_LOG("[Call] ❌ Overload '" + funcExpr->mangledName + "' did not match.");
            }
        }
    } else {
        called = scope->get(originalCallee);
    }

    if (!called) {
        DEBUG_LOG("[Call] ERROR: Callee '" + originalCallee + "' not found in scope");
        console.error(formatError("Callable '" + originalCallee + "' not found in scope " + scope->getName()));
        return nullptr;
    }

    DEBUG_LOG("[Call] Found callee '" + callee + "' of type '" + 
              (called->getType() ? called->getType()->kindName() : "null") + "'");

    // Extract parameter list and return type
    std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> parameters;
    if (auto callable = std::dynamic_pointer_cast<Omniscript::Callable>(called)) {
        DEBUG_LOG("[Call] Callee is callable, cloning parameters");
        parameters = callable->cloneParameters();
        type = callable->getType();
        if (type->isFunction()) {
            type = type->getReturnType();
        }
        DEBUG_LOG("[Call] Cloned " + std::to_string(parameters.size()) + " parameters");
    } else {
        DEBUG_LOG("[Call] ERROR: Callee is not callable");
        console.error(formatError("'" + callee + "' is not callable; it is of kind '" + 
                      (called->getType() ? called->getType()->kindName() : "null") + "'."));
        return nullptr;
    }

    // Create a local scope
    DEBUG_LOG("[Call] Creating local scope for call to '" + callee + "'");
    auto localScope = scope->createChildScope("call_" + callee);
    DEBUG_LOG("[Call] Created local scope with " + std::to_string(parameters.size()) + " parameters");

    std::unordered_set<std::string> providedParams;
    size_t positionalArgIndex = 0;
    size_t namedArgsCount = 0;

    DEBUG_LOG("[Call] Processing " + std::to_string(args.size()) + " arguments");

    // First pass: named arguments
    for (const auto& arg : args) {
        if (auto namedArg = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
            namedArgsCount++;
            const std::string& paramName = namedArg->getName();
            DEBUG_LOG("[Call] Processing named argument '" + paramName + "'");

            bool found = false;
            for (const auto& param : parameters) {
                if (param->name == paramName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                DEBUG_LOG("[Call] ERROR: Unknown parameter '" + paramName + "'");
                console.error(formatError("Unknown parameter '" + paramName + "' for callable '" + callee + "'"));
                continue;
            }

            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(namedArg->value)) {
                typed->setType(type);
                DEBUG_LOG("[Call] Set type for named argument '" + paramName + "'");
            }

            auto evaluated = namedArg->value ? namedArg->value->express(scope) : nullptr;
            localScope->set(paramName, evaluated);
            providedParams.insert(paramName);
            DEBUG_LOG("[Call] Set named parameter '" + paramName + "' in local scope");
        }
    }

    // Second pass: positional arguments and defaults
    for (const auto& param : parameters) {
        const std::string& paramName = param->name;

        if (providedParams.count(paramName)) {
            continue;
        }

        if (positionalArgIndex < args.size()) {
            auto arg = args[positionalArgIndex++];

            if (std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                DEBUG_LOG("[Call] ERROR: Positional argument after named argument");
                console.error(formatError("Positional argument after named argument is not allowed."));
                continue;
            }

            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(arg)) {
                if (Omniscript::isSameOrCastableTo(typed->getRootType(), param->getType()) || Omniscript::isSameOrCastableTo(typed->getType(), param->getType())) {
                    if (!typed->getType()) {
                        typed->setType(param->getType());
                    }
                } else {
                    console.error(formatError("Cannot bind argument of type '" + typed->getRootType()->kindName() +
                                  "' to parameter '" + paramName + "'; expected '" + param->getType()->kindName() + "'"));
                }
            }

            auto value = arg->express(scope);
            if (!value || value->getType()->isInvalid()) {
                console.error(formatError("Invalid argument for parameter '" + paramName + "'"));
            }

            localScope->set(paramName, value);
            DEBUG_LOG("[Call] Set positional argument for '" + paramName + "'");

        } else if (param->value) {
            DEBUG_LOG("[Call] Using default value for parameter '" + paramName + "'");
            localScope->set(paramName, param->value);
        } else {
            DEBUG_LOG("[Call] ERROR: Missing required parameter '" + paramName + "'");
            console.error(formatError("Missing required argument for parameter '" + paramName + "'"));
        }
    }

    // Check for extra args (varargs or error)
    if (positionalArgIndex + namedArgsCount < args.size()) {
        if (auto func = std::dynamic_pointer_cast<Omniscript::Callable>(called)) {
            if (!func->isVarArg) {
                DEBUG_LOG("[Call] ERROR: Too many arguments provided");
                console.error(formatError("Too many arguments provided to '" + callee + "'"));
                return nullptr;
            }
        }
    }

    DEBUG_LOG("[Call] Preparing arguments for CallExpression");
    
    std::vector<std::shared_ptr<Omniscript::Expression>> finalArgs;
    int paramIndex = 0;
    for (const auto& param : parameters) {
        auto val = localScope->get(param->name);
        val->name = param->name;
        
        DEBUG_LOG("Parameter '" + std::to_string(paramIndex) + "' is '" + val->name + "'.");
        
        paramIndex++;
        finalArgs.push_back(val);
    }

    if (instanceName.empty()) {
        DEBUG_LOG("[Call] Returning CallExpression for '" + callee + "' with " + std::to_string(finalArgs.size()) + " args");
        return std::make_shared<Omniscript::CallExpression>(callee, finalArgs, type);
    }

    return std::make_shared<Omniscript::CallExpression>(callee, instanceName, finalArgs);
}

bool Call::matchArgumentsToParameters(
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& args,
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& params,
    SymbolTableType scope
) {
    DEBUG_LOG("[Call] Starting argument-to-parameter matching");

    std::unordered_set<std::string> matchedNames;
    std::unordered_map<std::string, std::shared_ptr<Omniscript::FunctionInputExpression>> namedArgs;
    std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> positionalArgs;

    // Separate named and positional args
    for (const auto& arg : args) {
        if (!arg->name.empty()) {
            DEBUG_LOG("[Call] Found named arg: " + arg->name);
            namedArgs[arg->name] = arg;
        } else {
            DEBUG_LOG("[Call] Found positional arg at index: " + std::to_string(positionalArgs.size()) + " of kind '" + arg->value->getRootType()->kindName() + "'.");
            positionalArgs.push_back(arg);
        }
    }

    size_t positionalIndex = 0;

    for (const auto& param : params) {
        const std::string& paramName = param->name;
        std::shared_ptr<Omniscript::FunctionInputExpression> matchingArg;

        DEBUG_LOG("[Call] Matching parameter: " + paramName);

        if (namedArgs.count(paramName)) {
            matchingArg = namedArgs[paramName];
            matchedNames.insert(paramName);
            DEBUG_LOG("[Call] Matched named argument: " + paramName);
        } else if (positionalIndex < positionalArgs.size()) {
            matchingArg = positionalArgs[positionalIndex++];
            DEBUG_LOG("[Call] Matched positional argument to parameter '" + paramName + "'");
        } else if (param->value) {
            DEBUG_LOG("[Call] No argument provided for '" + paramName + "', using default value");
            continue;
        } else {
            DEBUG_LOG("[Call] Missing required argument for parameter: " + paramName);
            return false;
        }

        if (!Omniscript::isSameOrCastableTo(matchingArg->value->getRootType(), param->getType()) &&
            !Omniscript::isSameOrCastableTo(matchingArg->value->getType(), param->getType())) {
            DEBUG_LOG("[Call] Type mismatch for parameter: " + paramName);
            DEBUG_LOG("[Call] Expected type: " + param->getType()->kindName());
            DEBUG_LOG("[Call] Provided argument type: " + matchingArg->value->getRootType()->kindName());
            return false;
        }
    }

    for (const auto& [name, _] : namedArgs) {
        if (matchedNames.count(name) == 0) {
            DEBUG_LOG("[Call] Unused named argument: " + name);
            return false;
        }
    }

    if (positionalIndex < positionalArgs.size()) {
        DEBUG_LOG("[Call] Too many positional arguments: expected " + std::to_string(positionalIndex) +
                  ", but got " + std::to_string(positionalArgs.size()));
        return false;
    }

    DEBUG_LOG("[Call] All arguments matched successfully");
    return true;
}

std::shared_ptr<Omniscript::Expression> FunctionDeclaration::express(SymbolTableType scope) {
    DEBUG_LOG();
    DEBUG_LOG("[Function] Constructing a function " + name + " prototype the return Type is '" + type->kindName() + "'.");

    DEBUG_LOG("[Function] Creating a local scope for the function");
    localScope = scope->createChildScope(name);

    if (name == "main") {
        name = "__main";
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
    } else if (type->isGeneric()) {
        type = resolveGeneric(type->getName());
        returnType = type;
    }

    DEBUG_LOG("[Function] Setting the function's body's return type to " + type->kindName());
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
            DEBUG_LOG("[Function] Parameter has type '" + paramType->kindName() + "'.");

            if (paramType->isGeneric()) {
                typed->setType(std::move(resolveGeneric(paramType->getName())));
            }
        }
        auto result = param->express(localScope);
        if (auto paramStmt = std::dynamic_pointer_cast<ParameterStatement>(param)) {
            if (paramIndex == 0 && paramStmt->getName() == "this") {
                std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(result)->isConstant = true;
            } else {
                std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(result)->isConstant = false;
            }
        }
        argValues.push_back(result);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
        DEBUG_LOG("[Function] Parameter '" + result->name + "' has type " + result->getType()->kindName());
        paramIndex++;
    }

    DEBUG_LOG("[Function] Passing generic type bindings from function to body block");
    if (auto holder = std::dynamic_pointer_cast<GenericHolder>(body)) {
        holder->inheritGenericsFrom(*this);
    }

    // auto bod = body->resolveExpressions(localScope);

    std::vector<std::shared_ptr<Omniscript::Expression>> functionBody = body->expressAsVector(localScope);
    // for (auto& stmt : body->statements) {
    //     functionBody.push_back(stmt->express(localScope));
    // }

    std::string mangledName = (name == "__main" ? "__main" : generateMangledName());

    DEBUG_LOG("[Function] Creating FunctionValue");
    auto functionVal = std::make_shared<Omniscript::FunctionExpression>(name, mangledName, returnType, functionBody, argValues, isVarArg);

    DEBUG_LOG("[Function] Storing overloaded function in scope '" + scope->getName() + "' under base name: " + name + " (mangled as: " + mangledName + ")");
    scope->addOverloadable(name, functionVal);

    return functionVal;
}

std::string FunctionDeclaration::generateMangledName() const {
    std::string mangled = name + "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(parameters[i])) {
            auto paramType = typed->getType();
            mangled += paramType ? (paramType->isPointer() ? paramType->getBasePointeeType()->kindName() + "*" : paramType->kindName()) : "unknown";
            // mangled += paramType ? paramType->kindName() : "unknown";
        } else {
            mangled += "any";
        }
        if (i < parameters.size() - 1) mangled += ",";
    }
    mangled += ")";
    return mangled;
}

void FunctionDeclaration::setReturnTypes() {
    DEBUG_LOG("[Function] Setting the function's return type to " + returnType->kindName());
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
    DEBUG_LOG("[Parameter] Creating parameter " + name + " of kind " + (type ? type->kindName() : "undefined"));

    std::shared_ptr<Omniscript::Expression> result;

    if (defaultValue) {
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(defaultValue)) {
            if (!type) {
                if (!typed->getType()) {
                    result = defaultValue->express(scope);
                    type = result->getType();
                } else {
                    type = typed->getType();
                    result = defaultValue->express(scope);
                }
                DEBUG_LOG("The inferred type is " + type->kindName());
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

    DEBUG_LOG("[Parameter] Created value for parameter " + name + " of kind " + result->getType()->kindName());

    if (isConstant) {
        scope->setConstant(name, result);
    } else {
        scope->set(name, result);
    }

    return std::make_shared<Omniscript::FunctionInputExpression>(name, type, result, isConstant);
}

std::shared_ptr<Omniscript::Expression> ArgumentStatement::express(SymbolTableType scope) {
    DEBUG_LOG("[Argument] Creating argument " + name);
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
    return std::make_shared<Omniscript::FunctionInputExpression>(name, type, result);
}

std::shared_ptr<Statement> ParameterStatement::getDefaultValue() {
    // if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(defaultValue)) {
    //     stmt->setType(type);
    // }
    return defaultValue;
    // return nullptr;
}

std::shared_ptr<Omniscript::Expression> ClassMember::express(SymbolTableType scope) {
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ConstructClassPrototype::express(SymbolTableType scope) {
    DEBUG_LOG("[ConstructClassPrototype] Constructing a class as a struct with methods");

    std::vector<std::shared_ptr<Omniscript::Expression>> fields;
    std::vector<std::shared_ptr<Omniscript::Type>> fieldTypes;
    std::vector<std::string> fieldNames;

    std::vector<std::shared_ptr<Omniscript::FunctionExpression>> constructors;
    std::shared_ptr<Omniscript::FunctionExpression> destructor = nullptr;

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
        fieldNames.push_back(fieldName);

        std::shared_ptr<Omniscript::Expression> fieldExpr = param->express(scope);
        fields.push_back(fieldExpr);
        fieldExpr->getType()->parameterName = fieldName;
        fieldTypes.push_back(fieldExpr->getType());

        // if (auto param = std::dynamic_pointer_cast<ParameterStatement>(member)) {
        //     std::string fieldName = param->getName();
        //     fieldNames.push_back(fieldName);

        //     std::shared_ptr<Omniscript::Expression> fieldExpr = param->express(scope);
        //     fields.push_back(fieldExpr);
        //     fieldExpr->getType()->parameterName = fieldName;
        //     fieldTypes.push_back(fieldExpr->getType());
        // }
    }

    // Step 2: Create class type
    auto classType = Omniscript::Type::createUserDefinedType(name, Omniscript::Kind::Class, fieldTypes);
    scope->addType(name, classType);

    // Step 3: Process methods (functions, constructor, destructor)
    for (const auto& member : body) {
        if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(member->getDefaultValue())) {
            auto funcName = func->getName();

            // add `this` parameter
            auto thisParam = std::make_shared<ParameterStatement>("this");
            thisParam->setType(Omniscript::Type::createPointerType(classType));
            func->parameters.insert(func->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));

            auto methodExpr = func->express(scope);
            fields.push_back(methodExpr);

            console.info(funcName + " " + name + ".constructor");
            if (funcName == name + ".constructor") {

                auto ctorExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(methodExpr);
                constructors.push_back(ctorExpr);

            } else if (funcName == name + ".destructor") {
                // Destructor
                auto dtorExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(methodExpr);
                destructor = dtorExpr;
            } 
        }
    }

    // Step 4: Create the StructExpression for the class body
    auto structExpr = std::make_shared<Omniscript::StructExpression>(
        name,
        name, // mangledName (can change if needed)
        fields,
        fieldNames,
        false
    );

    // Step 5: Construct ClassExpression
    auto classExpr = std::make_shared<Omniscript::ClassExpression>(
        name,
        structExpr,
        constructors,
        destructor
    );

    classExpr->parameters = structExpr->parameters;

    scope->set(name, classExpr);
    return classExpr;
}

std::shared_ptr<Omniscript::Expression> ConstructStructPrototype::express(SymbolTableType scope) {
    DEBUG_LOG("[ConstructStructPrototype] Constructing a struct expression");

    std::vector<std::shared_ptr<Omniscript::Expression>> fields;
    std::vector<std::shared_ptr<Omniscript::Type>> fieldTypes;
    std::vector<std::string> fieldNames;

    for (const auto& field : body) {
        if (auto paramDecl = std::dynamic_pointer_cast<ParameterStatement>(field)) {
            std::string fieldName = paramDecl->getName();
            fieldNames.push_back(fieldName);

            std::shared_ptr<Omniscript::Expression> fieldExpr = paramDecl->express(scope);

            fields.push_back(fieldExpr);
            fieldExpr->getType()->parameterName = fieldName;
            fieldTypes.push_back(fieldExpr->getType());
            DEBUG_LOG("Parameter '" + fieldName + "' has type " + fieldExpr->getType()->kindName());
        } else {
            DEBUG_LOG("Skipping non-variable declaration in struct body");
        }
    }

    auto structType = Omniscript::Type::createUserDefinedType(name, Omniscript::Kind::Struct, fieldTypes);
    scope->addType(name, structType);
    
    setType(structType);

    for (const auto& field : body) {
        if (auto methodStmt = std::dynamic_pointer_cast<FunctionDeclaration>(field)) {
            auto thisParam = std::make_shared<ParameterStatement>("this");
            thisParam->setType(Omniscript::Type::createPointerType(scope->getType(name)));
            methodStmt->parameters.insert(methodStmt->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));
            std::shared_ptr<Omniscript::Expression> method = methodStmt->express(scope);
            fields.push_back(method);
        } else {
            if (!std::dynamic_pointer_cast<ParameterStatement>(field)) {
                DEBUG_LOG("Skipping non-method declaration in struct body");
            }
        }
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

    return structExpr;
}
 
std::shared_ptr<Omniscript::Expression> ObjectConstructorStatement::express(SymbolTableType scope) {
    DEBUG_LOG("Constructing " + objectType + " " + instanceName);

    // std::vector<std::shared_ptr<Omniscript::Expression>> argValues;
    // for (const auto& arg : constructorArgs) {
    //     argValues.push_back(arg->express(scope));
    // }

    if (scope->getType(objectType)) {
        type = std::make_shared<Omniscript::UserDefinedType>(objectType);
        auto constructorCall = std::make_shared<Call>(objectType, instanceName, constructorArgs);
        auto call = std::dynamic_pointer_cast<Omniscript::CallExpression>(constructorCall->express(scope));

        auto instance = std::make_shared<Omniscript::InstanceExpression>(
            objectType,
            instanceName,
            call->args
        );

        instance->instanceType = scope->getType(objectType);
        instance->type = scope->getType(objectType);
        scope->set(instanceName, instance);
        return call;
    } else {
        console.error("Object type was not found in the scope");
    }

    // // 5. Return the allocated instance
    // return std::make_shared<Omniscript::CallExpression>(callee, finalArgs, type);
    return nullptr;
    // return std::make_shared<Omniscript::CallExpression>(objectType, instanceName, argValues); //, type);
}

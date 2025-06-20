#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/Core/Expressions/ClassExpression.h>
#include <omniscript/Core/Expressions/StructExpression.h>
#include <omniscript/Core/Expressions/CallableExpression.h>
#include <omniscript/Core/Expressions/FunctionExpression.h>
#include <omniscript/Core/Expressions/FunctionInputExpression.h>
#include <omniscript/Core/Expressions/VariableAccessExpression.h>

// ============================== Prototypes  ============================== //
std::shared_ptr<Omniscript::Expression> Call::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG();
    auto named = std::dynamic_pointer_cast<NamedStatement>(expr);
    std::string targetName = named ? named->getName() : instanceName;
    DEBUG_LOG("The callee is '" + callee + "' target name is '" + targetName + "' and instance name is '" + instanceName + "'.");
   
    DEBUG_LOG("The args are ");
    DEBUG_LOG("===============");

    for (const auto& arg : args) {
        DEBUG_LOG("Arg: " + arg->toString());

        auto typed = std::dynamic_pointer_cast<TypedStatement>(arg);
        if (typed) {
            auto rootType = typed->getRootType();
            auto fallbackType = typed->getType();

            if (rootType) {
                DEBUG_LOG("Type: " + rootType->toString());
            } else if (fallbackType) {
                DEBUG_LOG("Type: " + fallbackType->toString());
            } else {
                DEBUG_LOG("Type: undefined");
            }
        } else {
            DEBUG_LOG("Not a TypedStatement");
        }
    }
    DEBUG_LOG("===============");
    
    std::string impliedTargetName;
    if (targetName == "this" && instanceName.empty()) {
        if (!accessContext.empty()) {
            impliedTargetName = accessContext.back();
        } else {
            // Optional: handle error or set default
            console.error("Accessing 'this' outside of any valid context");
            // instanceName = "<invalid-this>";
        }
    }
    
    // If there is no target we are just calling a function
    // If there is a target we are calling a method
    if (!targetName.empty()) {
        auto obj = scope->get(impliedTargetName.empty() ? targetName : impliedTargetName);

        std::string contextualName;
        for (size_t i = 0; i < accessContext.size(); ++i) {
            if (!contextualName.empty()) contextualName += ".";
            contextualName += accessContext[i];

            std::string fullName = contextualName + "." + callee;
            DEBUG_LOG("[MemberAccess] Trying contextual name: " + fullName);
        }

        contextualName += "." + callee;

        auto contextualObj = scope->get(contextualName);
        if (!contextualObj) {
            auto contextualArray = scope->getOverloads(contextualName);
            if (!contextualArray.empty()) {
                contextualObj = contextualArray[0];
            }
        }

        if (contextualObj) {
            callee = contextualName;
            obj = contextualObj;
        }

        if (!std::dynamic_pointer_cast<Omniscript::FunctionExpression>(obj) && obj) {
            std::string typeName = obj->getType()->getName();
            DEBUG_LOG("Type name is '" + typeName + "' callee is '" + callee + "'.");

            validateAccessiblity(typeName, callee, scope);

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
            } else {
                const std::string moduleTypeSuffix = "_module_type";

                std::string baseTypeName = typeName;
                bool isModuleType = false;

                // Strip '_module_type' suffix if present
                if (baseTypeName.size() > moduleTypeSuffix.size() &&
                    baseTypeName.compare(baseTypeName.size() - moduleTypeSuffix.size(), moduleTypeSuffix.size(), moduleTypeSuffix) == 0) {
                    isModuleType = true;
                    baseTypeName = baseTypeName.substr(0, baseTypeName.size() - moduleTypeSuffix.size());
                }

                callee = baseTypeName + "." + callee;

                if (!isModuleType) {
                    auto thisArg = std::make_shared<AddressOf>((instanceName.empty() ? targetName : instanceName));
                    if (auto thisArgType = scope->getType(typeName)) {
                        thisArg->setType(Omniscript::Type::createPointerType(thisArgType));
                        thisArg->setRootType(thisArg->getType());
                        args.insert(args.begin(), thisArg);
                        DEBUG_LOG("The 'this' arg is of instance '" + (instanceName.empty() ? targetName : instanceName) + "' and of type '" + thisArg->getType()->toString() + "'.");
                    } else {
                        console.error("The type '" + typeName + "' does not exist in the scope.");
                    }
                }
            }
        } else if (isFromAssignment) {
            std::shared_ptr<Statement> assignmentExpr = std::make_shared<GetVariable>(targetName);
            auto methodCall = std::make_shared<Call>(assignmentExpr, callee, args);
            auto stmt = std::make_shared<AssignVariable>(instanceName, type, methodCall);
            if (isFromConstantAssignment) {
                stmt->markAsConstant();
            }
            return stmt->express(scope);
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

    if (overloads.empty()) {
        auto& contextList = getAccessContext();
        std::string qualifiedName;
    
        for (size_t i = 0; i < contextList.size(); ++i) {
            if (!qualifiedName.empty()) {
                qualifiedName += ".";
            }
            qualifiedName += contextList[i];
    
            auto fullCalleeName = qualifiedName + "." + originalCallee;
            overloads = scope->getOverloads(fullCalleeName);
             DEBUG_LOG("[Call] Attempting overload resolution for '" + fullCalleeName + "'");
            if (!overloads.empty()) {
                DEBUG_LOG("Found overloads for: " + fullCalleeName + "\n");
                break;
            }
        }
    }

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
                DEBUG_LOG("[Call] Parameter '" + casted->name + "' of type '" + casted->getType()->toString() + "'");
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
                        DEBUG_LOG("Coercing matching arg's type");
                        std::shared_ptr<Omniscript::Type> matchingArgType = typedArg->getRootType() ? typedArg->getRootType() : typedArg->getType();
                        typedArg->setType(param->getType());
                        typedArg->setRootType(param->getType());
                        DEBUG_LOG("[Call] Coerced argument to match parameter '" + paramName + 
                                    "': " + param->getType()->toString());
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
                    DEBUG_LOG("[Call] Evaluated an unamed argument " + result->toString());
                    auto inputExpr = std::make_shared<Omniscript::FunctionInputExpression>("", result->getType(), result);
                    evaluatedArgs.push_back(inputExpr);
                }
                if (auto typed = std::dynamic_pointer_cast<TypedStatement>(arg)) {
                    DEBUG_LOG("[Call] Evaluated argument has type: " + typed->getType()->toString() + "'.");
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
              (called->getType() ? called->getType()->toString() : "null") + "'");
    
    auto calledFunc = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(called);

    // Extract parameter list and return type
    std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> parameters;
    if (auto callable = std::dynamic_pointer_cast<Omniscript::Callable>(called)) {
        DEBUG_LOG("[Call] Callee is callable, cloning parameters");
        parameters = callable->cloneParameters();
        type = callable->getType();
        if (type->isFunction()) {
            DEBUG_LOG("The function's return type is '" + type->getReturnType()->toString() + "'.");
            type = type->getReturnType();
        }
        DEBUG_LOG("[Call] Cloned " + std::to_string(parameters.size()) + " parameters");
    } else {
        DEBUG_LOG("[Call] ERROR: Callee is not callable");
        console.error(formatError("'" + callee + "' is not callable; it is of kind '" + 
                      (called->getType() ? called->getType()->toString() : "null") + "'."));
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
                    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(namedArg->value)) {
                        typed->setType(param->getType());
                        DEBUG_LOG("[Call] Set type for named argument '" + paramName + "' to '" + type->toString() + "'.");
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                DEBUG_LOG("[Call] ERROR: Unknown parameter '" + paramName + "'");
                console.error(formatError("Unknown parameter '" + paramName + "' for callable '" + callee + "'"));
                continue;
            }


            auto evaluated = namedArg->value ? namedArg->value->express(scope) : nullptr;
            localScope->set(paramName, evaluated);
            providedParams.insert(paramName);
            DEBUG_LOG("[Call] Set named parameter '" + paramName + "' in local scope with value '" + evaluated->toString() + "' type '" + evaluated->getType()->toString() + "'.");
        }
    }

    // Second pass: positional arguments and defaults
    for (int i = 0; i < parameters.size(); i++) {
        auto& param = parameters[i];
        std::string paramName = param->name;
        
        if (providedParams.count(paramName)) {
            continue;
        }
        
        if (calledFunc) {
            int variadicIndex = i;
            // if the function is extern don't add an implied count
            // The variadic parameter should be the current parameter not the parameter after the countd 
            if (!calledFunc->isExtern && !calledFunc->isIntrinsic) {
                variadicIndex++;
            }
            if (variadicIndex < parameters.size()) {   
                // Check if the next parameter is variadic and automatically insert the args count    
                if (parameters[variadicIndex]->isVariadic) {
                    paramName = parameters[variadicIndex]->name;
                    int varArgsCountIndex = i;
        
                    DEBUG_LOG("[Call] Handling variadic parameter '" + paramName + "'");
                    
                    std::vector<std::shared_ptr<Omniscript::Expression>> collectedArgs;
                    
                    int varArgsCount = 0;
                    while (positionalArgIndex < args.size()) {
                        auto arg = args[positionalArgIndex++];
        
                        if (std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                            DEBUG_LOG("[Call] ERROR: Positional argument after named argument in variadic");
                            console.error(formatError("Positional argument after named argument is not allowed."));
                            continue;
                        }
        
                        auto typed = std::dynamic_pointer_cast<TypedStatement>(arg);
                        if (!typed) {
                            console.error(formatError("Expected typed argument for variadic param '" + paramName + "'"));
                            continue;
                        }
        
                        auto argType = typed->getRootType() ? typed->getRootType() : typed->getType();
        
                        if (!argType) {
                            auto tempScope = localScope->createChildScope("temp");
                            argType = (typed->clone()->express(tempScope))->getType();
                            if (!argType) {
                                console.error(formatError("The variadic argument '" + arg->toString() + "' has no type"));
                            }
                        }
        
                        // if (!Omniscript::isSameOrCastableTo(argType, param->getType())) {
                        //     console.error(formatError("Type mismatch in variadic arg for parameter '" + paramName + "'"));
                        //     continue;
                        // }
        
                        auto value = arg->express(scope);
                        if (!value || value->getType()->isInvalid()) {
                            console.error(formatError("Invalid value in variadic argument for '" + paramName + "'"));
                            continue;
                        }
        
                        varArgsCount++;
                        collectedArgs.push_back(value);
                    }
                    
                    // skip the args count
                    positionalArgIndex++;
                    i++;
                    
                    // external functions don't have an implied count, only an explicit count
                    if (!calledFunc->isExtern) {
                        auto argsCountExpr = std::make_shared<Omniscript::Integer<int>>(varArgsCount);
                        localScope->set(paramName + "_count", argsCountExpr);
                    }
                    
                    // Wrap the collected values into an array-like container
                    auto arrayValue = std::make_shared<Omniscript::ArrayExpression>(param->getType(), collectedArgs, /* isVariadic */ true);
        
                    localScope->set(paramName, arrayValue);
                    DEBUG_LOG("[Call] Bound variadic parameter '" + paramName + "' with " + std::to_string(collectedArgs.size()) + " arguments");
                    continue; // Do not try to assign anything else to this param
                }
            }
        }

        if (positionalArgIndex < args.size()) {
            auto arg = args[positionalArgIndex++];

            if (std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                DEBUG_LOG("[Call] ERROR: Positional argument after named argument");
                console.error(formatError("Positional argument after named argument is not allowed."));
                continue;
            }

            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(arg)) {
                auto argType = typed->getRootType() ? typed->getRootType() : typed->getType();
                
                if (!argType) {
                    auto tempScope = localScope->createChildScope("temp");
                    argType = (typed->clone()->express(tempScope))->getType();
                    if (!argType) {
                        console.error(formatError("The argument '" + arg->toString() + "' has no type"));
                    }
                }

                if (Omniscript::isSameOrCastableTo(argType, param->getType())) {
                    if (!typed->getType()) {
                        typed->setType(param->getType());
                    }
                } else {
                    console.error(formatError("Cannot bind argument of type '" + argType->toString() +
                                  "' to parameter '" + paramName + "'; expected '" + param->getType()->toString() + "'"));
                }
            }

            auto value = arg->express(scope);
            if (!value || value->getType()->isInvalid()) {
                console.error(formatError("Invalid argument for parameter '" + paramName + "'"));
            }

            localScope->set(paramName, value);
            DEBUG_LOG("[Call] Set positional argument for '" + paramName + "' with value '" + value->toString() + "' and type '" + value->getType()->toString() + "'.");

        } else if (param->defaultValue) {
            DEBUG_LOG("[Call] Using default value for parameter '" + paramName + "'");
            localScope->set(paramName, param->defaultValue);
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

    if (std::dynamic_pointer_cast<Omniscript::FunctionExpression>(called)) {
        DEBUG_LOG("[Call] Returning CallExpression for '" + callee + "' with " + std::to_string(finalArgs.size()) + " args");
        return std::make_shared<Omniscript::CallExpression>(callee, finalArgs, type);
    }

    std::vector<std::shared_ptr<Omniscript::MemberExpression>> instanceMembers;
    auto instanceConstructor = std::make_shared<Omniscript::CallExpression>(callee, instanceName, finalArgs);
    for (const auto& param : parameters) {
        auto instanceMember = std::make_shared<Omniscript::MemberExpression>(
            param->getName(),
            param->getType(),
            param->defaultValue
        );
        instanceConstructor->members.push_back(instanceMember);
    }

    return instanceConstructor;
}

std::string Call::resolveFunctionOverload(
    const std::string& calleeName,
    const std::vector<std::shared_ptr<Statement>>& args,
    const SymbolTableType& scope)
{
    DEBUG_LOG("[OverloadResolver] Resolving overload for '" + calleeName + "'");
    auto overloads = scope->getOverloads(calleeName);

    if (overloads.empty()) {
        DEBUG_LOG("[OverloadResolver] No overloads found for '" + calleeName + "'");
        return "";
    }

    for (const auto& overload : overloads) {
        auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overload);
        if (!funcExpr) {
            continue;
        }

        auto paramList = funcExpr->getParameters();
        std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> inputParams;
        for (const auto& param : paramList) {
            auto casted = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(param);
            if (!casted) continue;
            inputParams.push_back(casted);
        }

        std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> evaluatedArgs;
        size_t positionalArgIndex = 0;

        for (const auto& arg : args) {
            std::shared_ptr<Omniscript::Expression> result;
            if (auto namedArg = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                result = namedArg->value->express(scope);
                evaluatedArgs.push_back(std::make_shared<Omniscript::FunctionInputExpression>(
                    namedArg->getName(), result->getType(), result));
            } else {
                result = arg->express(scope);
                evaluatedArgs.push_back(std::make_shared<Omniscript::FunctionInputExpression>(
                    "", result->getType(), result));
            }
        }

        if (matchArgumentsToParameters(evaluatedArgs, inputParams, scope)) {
            DEBUG_LOG("[OverloadResolver] ✅ Match found: " + funcExpr->mangledName);
            return funcExpr->mangledName;
        } else {
            DEBUG_LOG("[OverloadResolver] ❌ Mismatch with: " + funcExpr->mangledName);
        }
    }

    return "";
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
            if (arg->defaultValue->getRootType()->isInvalid()) {
                auto tempScope = scope->createChildScope("temp");
                arg->defaultValue->rootType = arg->defaultValue->getType();
            }
            DEBUG_LOG("[Call] Found positional arg at index: " + std::to_string(positionalArgs.size()) + " of kind '" + arg->defaultValue->getRootType()->toString() + "'.");
            positionalArgs.push_back(arg);
        }
    }

    size_t positionalIndex = 0;

    for (const auto& param : params) {
        const std::string& paramName = param->name;
        std::shared_ptr<Omniscript::FunctionInputExpression> matchingArg;

        DEBUG_LOG("[Call] Matching parameter: " + paramName);

        if (param->isVariadic) {
            DEBUG_LOG("[Call] Parameter is variadic: " + paramName);

            while (positionalIndex < positionalArgs.size()) {
                auto arg = positionalArgs[positionalIndex++];
                auto argType = (arg->defaultValue->getRootType() ? arg->defaultValue->getRootType() : arg->defaultValue->getType());

                // if (!Omniscript::isSameOrCastableTo(argType, param->getType())) {
                //     DEBUG_LOG("[Call] Type mismatch in variadic arguments for parameter: " + paramName);
                //     return false;
                // }
            }

            // After this variadic param, we ignore any further param definitions
            break;
        }

        if (namedArgs.count(paramName)) {
            matchingArg = namedArgs[paramName];
            matchedNames.insert(paramName);
            DEBUG_LOG("[Call] Matched named argument: " + paramName);
        } else if (positionalIndex < positionalArgs.size()) {
            matchingArg = positionalArgs[positionalIndex++];
            DEBUG_LOG("[Call] Matched positional argument to parameter '" + paramName + "'");
        } else if (param->defaultValue) {
            DEBUG_LOG("[Call] No argument provided for '" + paramName + "', using default value");
            continue;
        } else {
            DEBUG_LOG("[Call] Missing required argument for parameter: " + paramName);
            return false;
        }

        auto matchingArgType = (matchingArg->defaultValue->getRootType()->isInvalid() ? matchingArg->defaultValue->getType() : matchingArg->defaultValue->getRootType());
        if (!Omniscript::isSameOrCastableTo(matchingArgType, param->getType())) {
            DEBUG_LOG("[Call] Type mismatch for parameter: " + paramName);
            DEBUG_LOG("[Call] Expected type: '" + param->getType()->toString() + "' type got '" );
            DEBUG_LOG("[Call] Provided argument type: " + matchingArgType->toString());
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
        DEBUG_LOG("[Call] Too many positional arguments left unmatched (and no variadic parameter found)");
        return false;
    }

    DEBUG_LOG("[Call] All arguments matched successfully");
    return true;
}

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
    functionVal->staticLibPath = staticLibPath;
    functionVal->dynamicLibPath = dynamicLibPath;
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
            mangled += paramType ? (paramType->isPointer() ? paramType->getBasePointeeType()->toString() + "*" : paramType->toString()) : "unknown";
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
    DEBUG_LOG("[Parameter] Creating parameter " + name + " of kind " + (type ? type->toString() : "undefined"));
    
    std::shared_ptr<Omniscript::Expression> result;

    bool isValidDefaultValue = true;

    if (defaultValue) {
        auto typed = std::dynamic_pointer_cast<TypedStatement>(defaultValue);
        if (typed) {
            if (typed->getRootType()->isInvalid()) {
                auto resultType = typed->clone()->express(scope)->getType();
                isValidDefaultValue = !resultType->isInvalid();
            } else {
                isValidDefaultValue = true;
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

    DEBUG_LOG("[Parameter] Created value for parameter " + name + " of kind " + result->getType()->toString());

    if (isConstant) {
        scope->setConstant(name, result);
    } else {
        scope->set(name, result);
    }

    auto param = std::make_shared<Omniscript::FunctionInputExpression>(name, type, result, isConstant);
    param->isVariadic = isVariadic;

    return param;
}

std::shared_ptr<Omniscript::Expression> ArgumentStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
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
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ConstructStructPrototype::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("[ConstructStructPrototype] Constructing a struct expression");

    std::vector<std::shared_ptr<Omniscript::Expression>> fields;
    std::vector<std::shared_ptr<Omniscript::Type>> fieldTypes;
    std::vector<std::string> fieldNames;

    SymbolTableType localScope = scope->createChildScope(name);

    for (const auto& field : body) {
        if (auto paramDecl = std::dynamic_pointer_cast<ParameterStatement>(field)) {
            std::string fieldName = paramDecl->getName();
            fieldNames.push_back(fieldName);

            std::shared_ptr<Omniscript::Expression> fieldExpr = paramDecl->express(localScope);

            fields.push_back(fieldExpr);
            fieldExpr->getType()->parameterName = fieldName;
            fieldTypes.push_back(fieldExpr->getType());
            DEBUG_LOG("Parameter '" + fieldName + "' has type " + fieldExpr->getType()->toString());
        } else {
            DEBUG_LOG("Skipping non-variable declaration in struct body");
        }
    }

    auto structType = Omniscript::Type::createUserDefinedType(name, Omniscript::Kind::Struct, fieldTypes);
    scope->addType(name, structType);
    
    setType(structType);

    // Phase 1: Register all methods (e.g., for mutual recursion or early references)
    for (const auto& field : body) {
        if (auto methodStmt = std::dynamic_pointer_cast<FunctionDeclaration>(field)) {
            auto thisParam = std::make_shared<ParameterStatement>("this");
            thisParam->setType(Omniscript::Type::createPointerType(localScope->getType(name)));
            methodStmt->parameters.insert(methodStmt->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));
            methodStmt->registerInScope(scope);
        } else {
            if (!std::dynamic_pointer_cast<ParameterStatement>(field)) {
                DEBUG_LOG("Skipping non-method declaration in struct body");
            }
        }
    }

    // Phase 2: Compile methods and build method expressions
    for (const auto& field : body) {
        if (auto methodStmt = std::dynamic_pointer_cast<FunctionDeclaration>(field)) {
            auto thisParam = std::make_shared<ParameterStatement>("this");
            thisParam->setType(Omniscript::Type::createPointerType(localScope->getType(name)));

            // Insert 'this' as the first parameter
            methodStmt->parameters.insert(methodStmt->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));

            // Compile the method to an expression (LLVM function pointer, etc.)
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

    return classExpr;
}
 
std::shared_ptr<Omniscript::Expression> ObjectConstructorStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
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
            call->members
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

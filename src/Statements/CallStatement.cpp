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


std::shared_ptr<Omniscript::Expression> Call::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG();
    auto named = std::dynamic_pointer_cast<NamedStatement>(expr);
    std::string evaluatedCallee = callee;
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
    std::string contextualName;
    {
        int index = 0;
        for (size_t i = 0; i < accessContext.size(); ++i) {
            if (!contextualName.empty()) contextualName += ".";
            if (i == 0) {
                auto obj = scope->get(impliedTargetName.empty() ? targetName : impliedTargetName);
                if (auto udt = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(obj->getType())) {
                    contextualName += udt->name;
                    auto thisArg = std::make_shared<ReferenceTo>((instanceName.empty() ? targetName : instanceName));
                    thisArg->setType(Omniscript::Type::createPointerType(udt));
                    thisArg->setRootType(thisArg->getType());
                    args.insert(args.begin(), thisArg);
                    DEBUG_LOG("The 'this' arg is of instance '" + (instanceName.empty() ? targetName : instanceName) + "' and of type '" + thisArg->getType()->toString() + "'.");
                } else {
                    contextualName += accessContext[i];
                }
            } else {
                contextualName += accessContext[i];
            }

            std::string fullName = contextualName + "." + callee;
            DEBUG_LOG("[MemberAccess] Trying contextual name: " + fullName);
            index++;
        }

        if (index == 0) {
            contextualName += callee;
        } else {
            contextualName += "." + callee;
        }
    }

    if (!targetName.empty()) {
        auto obj = scope->get(impliedTargetName.empty() ? targetName : impliedTargetName);

        auto contextualObj = scope->get(contextualName);
        if (!contextualObj) {
            auto contextualArray = scope->getOverloads(contextualName);
            if (!contextualArray.empty()) {
                contextualObj = contextualArray[0];
            }
        }

        if (contextualObj) {
            evaluatedCallee = contextualName;
            obj = contextualObj;
        }
    }
    
    
    DEBUG_LOG("[Call] Evaluating call to '" + evaluatedCallee + "'");
    if (!instanceName.empty()) {
        DEBUG_LOG("For " + instanceName);
    }
    
    std::string originalCallee = callee;
    DEBUG_LOG("[Call] Looking up callee '" + originalCallee + "' in scope");
    std::shared_ptr<Omniscript::Expression> called;

    // Attempt overload resolution
    DEBUG_LOG("[Call] Attempting overload resolution for '" + originalCallee + "'");
    auto overloads = scope->getOverloads(contextualName);

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
                evaluatedCallee = funcExpr->mangledName;
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

    DEBUG_LOG("[Call] Found callee '" + evaluatedCallee + "' of type '" + 
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
        console.error(formatError("'" + evaluatedCallee + "' is not callable; it is of kind '" + 
                      (called->getType() ? called->getType()->toString() : "null") + "'."));
        return nullptr;
    }

    // Create a local scope
    DEBUG_LOG("[Call] Creating local scope for call to '" + evaluatedCallee + "'");
    auto localScope = scope->createChildScope("call_" + evaluatedCallee);
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
                console.error(formatError("Unknown parameter '" + paramName + "' for callable '" + evaluatedCallee + "'"));
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
        
                        // if (!Omniscript::Type::isSameOrCastableTo(argType, param->getType())) {
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

                if (Omniscript::Type::isSameOrCastableTo(argType, param->getType())) {
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
        DEBUG_LOG("[Call] Returning CallExpression for '" + evaluatedCallee + "' with " + std::to_string(finalArgs.size()) + " args");
        auto callExpr = std::make_shared<Omniscript::CallExpression>(evaluatedCallee, finalArgs, type);
        callExpr->setPosition(getPosition());
        return callExpr;
    }

    std::vector<std::shared_ptr<Omniscript::MemberExpression>> instanceMembers;
    auto instanceConstructor = std::make_shared<Omniscript::CallExpression>(evaluatedCallee, instanceName, finalArgs);
    int index = 0;
    for (const auto& param : parameters) {
        auto instanceMember = std::make_shared<Omniscript::MemberExpression>(
            param->getName(),
            param->getType(),
            finalArgs[index]
        );
        instanceConstructor->members.push_back(instanceMember);
        index++;
    }

    instanceConstructor->setPosition(getPosition());
    
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

                // if (!Omniscript::Type::isSameOrCastableTo(argType, param->getType())) {
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
        if (!Omniscript::Type::isSameOrCastableTo(matchingArgType, param->getType())) {
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

#include <omniscript/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
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
#include <omniscript/Expressions/AccessExpressions.h>
#include <omniscript/Expressions/CallableExpression.h>
#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/AggregateExpressions.h>
#include <omniscript/Expressions/FunctionInputExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>



std::shared_ptr<Omniscript::Expression> Call::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG();
    
    // Check if expr is a MemberAccess that should be handled as a method call
    if (auto memberAccess = std::dynamic_pointer_cast<MemberAccess>(expr)) {
        return handleMemberAccessCall(memberAccess, scope);
    }
    
    // Resolve target name and context
    auto named = std::dynamic_pointer_cast<NamedStatement>(expr);
    std::string targetName = named ? named->getName() : instanceName;
    std::string impliedTargetName = resolveImpliedTargetName(targetName);
    std::string contextualName = buildContextualName(targetName, impliedTargetName, scope);
    
    DEBUG_LOG("The callee is '" + callee + "' target name is '" + targetName + "' and instance name is '" + instanceName + "'.");
    logArgumentDetails();
    
    // Find the callable
    std::shared_ptr<Omniscript::Expression> called = findCallable(contextualName, scope);
    if (!called) {
        console.error(formatError("Callable '" + callee + "' not found in scope " + scope->getName()));
        return nullptr;
    }
    
    std::string evaluatedCallee = getEvaluatedCalleeName(called, contextualName);
    DEBUG_LOG("[Call] Found callee '" + evaluatedCallee + "' of type '" + 
              (called->getType() ? called->getType()->toString() : "null") + "'");
    
    // Verify callable and extract parameters
    auto callable = std::dynamic_pointer_cast<Omniscript::Callable>(called);
    if (!callable) {
        console.error(formatError("'" + evaluatedCallee + "' is not callable; it is of kind '" + 
                      (called->getType() ? called->getType()->toString() : "null") + "'."));
        return nullptr;
    }
    
    auto parameters = callable->cloneParameters();
    type = callable->getType();
    if (type->isFunction()) {
        DEBUG_LOG("The function's return type is '" + type->getReturnType()->toString() + "'.");
        type = type->getReturnType();
    }
    
    // Process arguments and create call
    auto localScope = scope->createChildScope("call_" + evaluatedCallee);
    if (!processArguments(parameters, localScope, scope)) {
        return nullptr;
    }
    
    return createCallExpression(evaluatedCallee, parameters, localScope, called);
}

std::shared_ptr<Omniscript::Expression> Call::handleMemberAccessCall(
    std::shared_ptr<MemberAccess> memberAccess, SymbolTableType scope) {
    
    DEBUG_LOG("[Call] Handling member access call");
    
    // First, evaluate the member access to get its expression and type
    auto memberExpr = memberAccess->express(scope);
    if (!memberExpr) {
        console.error(formatError("Failed to evaluate member access expression"));
        return nullptr;
    }
    
    // Extract the type name from the member access type
    auto memberType = memberExpr->getType();
    std::string baseTypeName;
    
    if (auto udt = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(memberType)) {
        baseTypeName = udt->name;
    } else if (auto pointeeType = memberType->getBasePointeeType()) {
        if (auto pointeeUdt = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(pointeeType)) {
            baseTypeName = pointeeUdt->name;
        }
    }
    
    if (baseTypeName.empty()) {
        console.error(formatError("Could not determine base type for member access"));
        return nullptr;
    }
    
    // Build method name: TypeName.methodName
    std::string methodName = baseTypeName + "." + callee;
    DEBUG_LOG("[Call] Looking for method: " + methodName);
    
    // Find the method
    std::shared_ptr<Omniscript::Expression> method = scope->get(methodName);
    if (!method) {
        // Try overload resolution
        auto overloads = scope->getOverloads(methodName);
        if (!overloads.empty()) {
            method = resolveMethodOverload(overloads, memberExpr, scope);
        }
    }
    
    if (!method) {
        console.error(formatError("Method '" + methodName + "' not found"));
        return nullptr;
    }
    
    // Verify it's callable
    auto callable = std::dynamic_pointer_cast<Omniscript::Callable>(method);
    if (!callable) {
        console.error(formatError("'" + methodName + "' is not callable"));
        return nullptr;
    }
    
    // Prepare arguments with 'this' as first argument
    std::vector<std::shared_ptr<Statement>> methodArgs;
    auto thisArg = std::make_shared<ReferenceTo>(memberAccess->toString());
    thisArg->setType(memberType);
    thisArg->setRootType(memberType);
    methodArgs.push_back(thisArg);
    
    // Add the original arguments
    methodArgs.insert(methodArgs.end(), args.begin(), args.end());
    
    // Replace args with method args for processing
    auto originalArgs = std::move(args);
    args = std::move(methodArgs);
    
    // Get parameters and process
    auto parameters = callable->cloneParameters();
    type = callable->getType();
    if (type->isFunction()) {
        type = type->getReturnType();
    }
    
    auto localScope = scope->createChildScope("call_" + methodName);
    if (!processArguments(parameters, localScope, scope)) {
        return nullptr;
    }
    
    return createCallExpression(methodName, parameters, localScope, method);
}

std::shared_ptr<Omniscript::Expression> Call::resolveMethodOverload(
    const std::vector<std::shared_ptr<Omniscript::Expression>>& overloads,
    std::shared_ptr<Omniscript::Expression> baseExpr,
    SymbolTableType scope) {
    
    DEBUG_LOG("[Call] Resolving method overload from " + std::to_string(overloads.size()) + " candidates");
    
    for (auto& overload : overloads) {
        auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overload);
        if (!funcExpr) continue;
        
        auto paramList = funcExpr->getParameters();
        if (paramList.empty()) continue;
        
        // Check if first parameter matches the base expression type (for 'this')
        auto firstParam = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(paramList[0]);
        if (!firstParam) continue;
        
        if (Omniscript::Type::isSameOrCastableTo(baseExpr->getType(), firstParam->getType())) {
            // This overload is compatible with the base type
            DEBUG_LOG("[Call] Found compatible method overload: " + funcExpr->mangledName);
            return funcExpr;
        }
    }
    
    return nullptr;
}

std::string Call::resolveImpliedTargetName(const std::string& targetName) {
    if (targetName == "this" && instanceName.empty()) {
        if (!accessContext.empty()) {
            return accessContext.back();
        } else {
            console.error("Accessing 'this' outside of any valid context");
            return "";
        }
    }
    return "";
}

std::string Call::buildContextualName(const std::string& targetName, const std::string& impliedTargetName, SymbolTableType scope) {
    std::string contextualName;
    std::string actualTargetName = impliedTargetName.empty() ? targetName : impliedTargetName;
    
    // Handle member access expressions (like this.position.log)
    if (auto memberAccess = std::dynamic_pointer_cast<MemberAccess>(expr)) {
        // Get the type of the member access expression
        auto memberExpr = memberAccess->express(scope);
        if (memberExpr && memberExpr->getType()) {
            auto memberType = memberExpr->getType();
            std::string typeName;
            
            // Extract the type name from the member's type
            if (auto udt = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(memberType)) {
                typeName = udt->name;
            } else if (auto pointeeType = memberType->getBasePointeeType()) {
                if (auto pointeeUdt = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(pointeeType)) {
                    typeName = pointeeUdt->name;
                }
            }
            
            if (!typeName.empty()) {
                contextualName = typeName + "." + callee;
                DEBUG_LOG("[Call] Built contextual name from member access type: " + contextualName);
                
                // Add 'this' argument for the member access target
                auto thisArg = std::make_shared<ReferenceTo>(memberAccess->toString());
                thisArg->setType(memberType);
                thisArg->setRootType(memberType);
                args.insert(args.begin(), thisArg);
                DEBUG_LOG("Added 'this' arg for member access: " + memberAccess->toString() + 
                          " of type: " + memberType->toString());
                
                return contextualName;
            }
        }
    }
    
    // Original logic for regular access contexts
    for (size_t i = 0; i < accessContext.size(); ++i) {
        if (!contextualName.empty()) contextualName += ".";
        
        if (i == 0 && !targetName.empty()) {
            auto obj = scope->get(actualTargetName);
            if (obj) {
                if (auto udt = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(obj->getType())) {
                    contextualName += udt->name;
                    addThisArgument(actualTargetName, udt);
                } else {
                    contextualName += accessContext[i];
                }
            } else {
                contextualName += accessContext[i];
            }
        } else {
            contextualName += accessContext[i];
        }
        
        DEBUG_LOG("[MemberAccess] Trying contextual name: " + contextualName + "." + callee);
    }
    
    if (accessContext.empty()) {
        contextualName = callee;
    } else {
        contextualName += "." + callee;
    }
    
    return contextualName;
}

void Call::addThisArgument(const std::string& targetName, std::shared_ptr<Omniscript::UserDefinedType> udt) {
    auto thisArg = std::make_shared<ReferenceTo>(instanceName.empty() ? targetName : instanceName);
    thisArg->setType(Omniscript::Type::createPointerType(udt));
    thisArg->setRootType(thisArg->getType());
    args.insert(args.begin(), thisArg);
    DEBUG_LOG("The 'this' arg is of instance '" + (instanceName.empty() ? targetName : instanceName) + 
              "' and of type '" + thisArg->getType()->toString() + "'.");
}

void Call::logArgumentDetails() {
    DEBUG_LOG("The args are ");
    DEBUG_LOG("===============");
    for (const auto& arg : args) {
        DEBUG_LOG("Arg: " + arg->toString());
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(arg)) {
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
}

std::shared_ptr<Omniscript::Expression> Call::findCallable(const std::string& contextualName, SymbolTableType scope) {
    DEBUG_LOG("[Call] Evaluating call to '" + contextualName + "'");
    
    // Try overload resolution first
    auto overloads = scope->getOverloads(contextualName);
    if (overloads.empty()) {
        overloads = findOverloadsInContext(scope);
    }
    
    if (!overloads.empty()) {
        return resolveOverload(overloads, scope);
    }
    
    // Fall back to direct lookup
    return scope->get(callee);
}

std::vector<std::shared_ptr<Omniscript::Expression>> Call::findOverloadsInContext(SymbolTableType scope) {
    auto& contextList = getAccessContext();
    std::string qualifiedName;
    
    for (size_t i = 0; i < contextList.size(); ++i) {
        if (!qualifiedName.empty()) {
            qualifiedName += ".";
        }
        qualifiedName += contextList[i];
        
        auto fullCalleeName = qualifiedName + "." + callee;
        auto overloads = scope->getOverloads(fullCalleeName);
        DEBUG_LOG("[Call] Attempting overload resolution for '" + fullCalleeName + "'");
        if (!overloads.empty()) {
            DEBUG_LOG("Found overloads for: " + fullCalleeName + "\n");
            return overloads;
        }
    }
    
    return {};
}

std::shared_ptr<Omniscript::Expression> Call::resolveOverload(
    const std::vector<std::shared_ptr<Omniscript::Expression>>& overloads, 
    SymbolTableType scope) {
    
    DEBUG_LOG("[Call] Found " + std::to_string(overloads.size()) + " overload candidates");
    
    for (auto& overload : overloads) {
        auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overload);
        if (!funcExpr) {
            DEBUG_LOG("[Call] Skipping non-function overload.");
            continue;
        }
        
        DEBUG_LOG("[Call] Checking if the overload '" + funcExpr->mangledName + "' (" + funcExpr->name + ") is the required overload.");
        
        auto paramList = funcExpr->getParameters();
        std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> inputParams;
        for (const auto& param : paramList) {
            auto casted = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(param);
            if (!casted) {
                console.error(formatError("Failed to cast parameter to FunctionInputExpression."));
                continue;
            }
            inputParams.push_back(casted);
        }
        
        // Coerce argument types and evaluate
        coerceArgumentTypes(inputParams);
        auto evaluatedArgs = evaluateArguments(scope);
        
        if (matchArgumentsToParameters(evaluatedArgs, inputParams, scope)) {
            DEBUG_LOG("[Call] ✅ Matched overload: using mangled name '" + funcExpr->mangledName + "'");
            return funcExpr;
        } else {
            DEBUG_LOG("[Call] ❌ Overload '" + funcExpr->mangledName + "' did not match.");
        }
    }
    
    return nullptr;
}

void Call::coerceArgumentTypes(const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& inputParams) {
    std::unordered_set<std::string> providedParams;
    size_t positionalArgIndex = 0;
    
    for (const auto& param : inputParams) {
        const std::string& paramName = param->name;
        std::shared_ptr<Statement> matchingArg;
        
        // Find matching argument (named first, then positional)
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
                typedArg->setType(param->getType());
                typedArg->setRootType(param->getType());
                DEBUG_LOG("[Call] Coerced argument to match parameter '" + paramName + 
                          "': " + param->getType()->toString());
            }
        }
    }
}

std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> Call::evaluateArguments(SymbolTableType scope) {
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
    
    return evaluatedArgs;
}

std::string Call::getEvaluatedCalleeName(std::shared_ptr<Omniscript::Expression> called, const std::string& contextualName) {
    if (auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(called)) {
        return funcExpr->mangledName;
    }
    return contextualName;
}

bool Call::processArguments(
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    SymbolTableType scope) {
    
    std::unordered_set<std::string> providedParams;
    size_t positionalArgIndex = 0;
    size_t namedArgsCount = 0;
    
    DEBUG_LOG("[Call] Processing " + std::to_string(args.size()) + " arguments");
    
    // First pass: named arguments
    if (!processNamedArguments(parameters, localScope, scope, providedParams, namedArgsCount)) {
        return false;
    }
    
    // Second pass: positional arguments and defaults
    if (!processPositionalArguments(parameters, localScope, scope, providedParams, positionalArgIndex, namedArgsCount)) {
        return false;
    }
    
    return true;
}

bool Call::processNamedArguments(
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    SymbolTableType scope,
    std::unordered_set<std::string>& providedParams,
    size_t& namedArgsCount) {
    
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
                        DEBUG_LOG("[Call] Set type for named argument '" + paramName + "' to '" + param->getType()->toString() + "'.");
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
            DEBUG_LOG("[Call] Set named parameter '" + paramName + "' in local scope with value '" + 
                      evaluated->toString() + "' type '" + evaluated->getType()->toString() + "'.");
        }
    }
    
    return true;
}

bool Call::processPositionalArguments(
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    SymbolTableType scope,
    const std::unordered_set<std::string>& providedParams,
    size_t& positionalArgIndex,
    size_t namedArgsCount) {
    
    auto calledFunc = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(
        localScope->getParent()->get(callee));
    
    for (int i = 0; i < parameters.size(); i++) {
        auto& param = parameters[i];
        std::string paramName = param->name;
        
        if (providedParams.count(paramName)) {
            continue;
        }
        
        // Handle variadic parameters
        if (calledFunc && handleVariadicParameter(parameters, localScope, scope, i, positionalArgIndex, calledFunc)) {
            continue;
        }
        
        // Handle regular positional arguments
        if (positionalArgIndex < args.size()) {
            if (!processRegularPositionalArgument(args[positionalArgIndex], param, localScope, scope, positionalArgIndex)) {
                return false;
            }
        } else if (param->defaultValue) {
            DEBUG_LOG("[Call] Using default value for parameter '" + paramName + "'");
            localScope->set(paramName, param->defaultValue);
        } else {
            DEBUG_LOG("[Call] ERROR: Missing required parameter '" + paramName + "'");
            console.error(formatError("Missing required argument for parameter '" + paramName + "'"));
            return false;
        }
    }
    
    // Check for extra arguments
    if (positionalArgIndex + namedArgsCount > args.size()) {
        auto func = std::dynamic_pointer_cast<Omniscript::Callable>(localScope->getParent()->get(callee));
        if (!func || !func->isVarArg) {
            DEBUG_LOG("[Call] ERROR: Too many arguments provided");
            console.error(formatError("Too many arguments provided to '" + callee + "'"));
            return false;
        }
    }
    
    return true;
}

bool Call::handleVariadicParameter(
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    SymbolTableType scope,
    int& i,
    size_t& positionalArgIndex,
    std::shared_ptr<Omniscript::FunctionExpression> calledFunc) {
    
    int variadicIndex = i;
    if (!calledFunc->isExtern && !calledFunc->isIntrinsic) {
        variadicIndex++;
    }
    
    if (variadicIndex >= parameters.size() || !parameters[variadicIndex]->isVariadic) {
        return false;
    }
    
    auto& variadicParam = parameters[variadicIndex];
    std::string paramName = variadicParam->name;
    
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
        
        auto value = arg->express(scope);
        if (!value || value->getType()->isInvalid()) {
            console.error(formatError("Invalid value in variadic argument for '" + paramName + "'"));
            continue;
        }
        
        varArgsCount++;
        collectedArgs.push_back(value);
    }
    
    positionalArgIndex++;
    i++;
    
    // External functions don't have an implied count, only an explicit count
    if (!calledFunc->isExtern) {
        auto argsCountExpr = std::make_shared<Omniscript::Integer<int>>(varArgsCount);
        localScope->set(paramName + "_count", argsCountExpr);
    }
    
    auto arrayValue = std::make_shared<Omniscript::ArrayExpression>(
        variadicParam->getType(), collectedArgs, /* isVariadic */ true);
    localScope->set(paramName, arrayValue);
    
    DEBUG_LOG("[Call] Bound variadic parameter '" + paramName + "' with " + 
              std::to_string(collectedArgs.size()) + " arguments");
    
    return true;
}

bool Call::processRegularPositionalArgument(
    std::shared_ptr<Statement> arg,
    std::shared_ptr<Omniscript::FunctionInputExpression> param,
    SymbolTableType localScope,
    SymbolTableType scope,
    size_t& positionalArgIndex) {
    
    if (std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
        DEBUG_LOG("[Call] ERROR: Positional argument after named argument");
        console.error(formatError("Positional argument after named argument is not allowed."));
        return false;
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
                          "' to parameter '" + param->name + "'; expected '" + param->getType()->toString() + "'"));
        }
    }
    
    auto value = arg->express(scope);
    if (!value || value->getType()->isInvalid()) {
        console.error(formatError("Invalid argument for parameter '" + param->name + "'"));
        return false;
    }
    
    localScope->set(param->name, value);
    positionalArgIndex++;
    
    DEBUG_LOG("[Call] Set positional argument for '" + param->name + "' with value '" + 
              value->toString() + "' and type '" + value->getType()->toString() + "'.");
    
    return true;
}

std::shared_ptr<Omniscript::Expression> Call::createCallExpression(
    const std::string& evaluatedCallee,
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    std::shared_ptr<Omniscript::Expression> called) {
    
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
        DEBUG_LOG("[Call] Returning CallExpression for '" + evaluatedCallee + "' with " + 
                  std::to_string(finalArgs.size()) + " args");
        auto callExpr = std::make_shared<Omniscript::CallExpression>(evaluatedCallee, finalArgs, type);
        callExpr->setPosition(getPosition());
        return callExpr;
    }
    
    // Handle constructor calls
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

// Keep existing methods unchanged
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
                arg->defaultValue->rootType = arg->defaultValue->getType();
            }
            DEBUG_LOG("[Call] Found positional arg at index: " + std::to_string(positionalArgs.size()) + 
                      " of kind '" + arg->defaultValue->getRootType()->toString() + "'.");
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
                positionalIndex++;
            }
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

        auto matchingArgType = (matchingArg->defaultValue->getRootType()->isInvalid() ? 
                               matchingArg->defaultValue->getType() : 
                               matchingArg->defaultValue->getRootType());
        
        if (!Omniscript::Type::isSameOrCastableTo(matchingArgType, param->getType())) {
            DEBUG_LOG("[Call] Type mismatch for parameter: " + paramName);
            DEBUG_LOG("[Call] Expected type: '" + param->getType()->toString() + "' got '" + 
                      matchingArgType->toString() + "'");
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
// CallStatement.cpp
#include <cstdio>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/ModuleAndImportStatements.h>
#include <omniscript/Statements/ClassConstructorStatement.h>
#include <omniscript/Statements/StructConstructorStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Types/DerivedTypes.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
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

namespace Omniscript {

namespace {
// When two types print identically but are not compatible, the plain message is useless.
// Add kind / pointer-ness / pointee / object identity so the cause is visible.
std::string describeTypeForDiag(const std::shared_ptr<Type>& t) {
    if (!t) return "<null>";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%p", static_cast<const void*>(t.get()));
    std::string out = t->toString() + " [kind=" + std::string(t->kindName()) +
                      ", pointer=" + (t->isPointer() ? "yes" : "no");
    if (auto pointee = t->getBasePointeeType()) {
        out += ", pointee=" + pointee->toString() + "/" + std::string(pointee->kindName());
    }
    out += ", id=" + std::string(buf) + "]";
    return out;
}

std::string bindMismatchMessage(const std::string& argTypeStr, const std::string& paramName,
                                const std::shared_ptr<Type>& got, const std::shared_ptr<Type>& want) {
    std::string msg = "Cannot bind argument of type '" + argTypeStr + "' to parameter '" + paramName +
                      "'; expected '" + (want ? want->toString() : std::string("<null>")) + "'";
    if (want && argTypeStr == want->toString()) {
        msg += "\n  Both types print identically but are not compatible:"
               "\n    argument:  " + describeTypeForDiag(got) +
               "\n    parameter: " + describeTypeForDiag(want);
    }
    return msg;
}
} // namespace

namespace {
// Resolves the type of a variable holding a function pointer to its FunctionType.
// `type X = fn(...) => T;` registers "X" (pointer) and "*X" (the function type) in the scope;
// `let f: X;` leaves the variable typed as the user-defined name "X".
struct FnPtrInfo {
    std::shared_ptr<FunctionType> fn;
    std::string typeName; // alias name, empty if the variable is typed `fn(...)` directly
};

FnPtrInfo resolveFunctionPointerType(std::shared_ptr<Type> t, SymbolTableType scope) {
    FnPtrInfo info;
    for (int depth = 0; t && depth < 4; ++depth) {
        if (auto fn = std::dynamic_pointer_cast<FunctionType>(t)) {
            info.fn = fn;
            return info;
        }
        if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(t)) {
            if (auto target = scope->getType("*" + udt->name)) {
                if (auto fn = std::dynamic_pointer_cast<FunctionType>(target)) {
                    info.fn = fn;
                    info.typeName = udt->name;
                    return info;
                }
            }
            auto aliased = scope->getType(udt->name);
            if (!aliased || aliased == t) return info;
            t = aliased;
            continue;
        }
        if (t->isPointer()) {
            t = t->getPointeeType();
            continue;
        }
        break;
    }
    return info;
}
} // namespace

std::shared_ptr<Expression> Call::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
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
    // A struct/class name used as a callee is a constructor call, so an inherited access
    // context must not turn its name into 'Context.Type'.
    auto plainCallee = scope->get(callee);
    const bool calleeIsType = plainCallee &&
        (std::dynamic_pointer_cast<StructExpression>(plainCallee) ||
         std::dynamic_pointer_cast<ClassExpression>(plainCallee));
    
    DEBUG_LOG("The callee is '" + callee + "' target name is '" + targetName + "' and instance name is '" + instanceName + "'.");
    logArgumentDetails();
    
    // Find the callable
    // Bare `f(...)` with a stale/irrelevant inherited access context (e.g. `g = f(...)`):
    // if nothing matches the contextual name but a plain `f` exists, use `f` directly.
    // Probed silently because findCallable() reports an error on failure.
    if (contextualName != callee && instanceName.empty() &&
        !std::dynamic_pointer_cast<MemberAccess>(expr) &&
        scope->getOverloads(contextualName).empty() &&
        findOverloadsInContext(scope).empty() &&
        (!scope->getOverloads(callee).empty() || calleeIsType)) {
        DEBUG_LOG("[Call] Ignoring inherited access context for bare call to '" + callee + "'.");
        contextualName = callee;
    }
    std::shared_ptr<Expression> called = findCallable(contextualName, scope);
    if (!called && contextualName != callee && instanceName.empty() &&
        !std::dynamic_pointer_cast<MemberAccess>(expr)) {
        // Bare `f(...)` call carrying an inherited access context: try plain `f`.
        called = findCallable(callee, scope);
    }
    if (!called) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify function '%s' is defined\n"
            "2. Check for correct scope or namespace\n"
            "3. Ensure proper import or module inclusion",
            callee.c_str()
        );
        console.reportError(
            Console::SEMANTIC_ERROR,
            Console::formatString("Callable '%s' not found in scope '%s'", 
                callee.c_str(), scope->getName().c_str()),
            suggestion
        );
        return nullptr;
    }
    
    std::string evaluatedCallee = getEvaluatedCalleeName(called, contextualName);
    DEBUG_LOG("[Call] Found callee '" + evaluatedCallee + "' of type '" + 
              (called->getType() ? called->getType()->toString() : "null") + "'");
    
    // Verify callable and extract parameters
    auto callable = std::dynamic_pointer_cast<Callable>(called);
    if (!callable) {
        // Indirect call through a variable holding a function pointer, e.g. `glClear(mask)`
        // where `let glClear: GLCLEAR;`.
        auto fp = resolveFunctionPointerType(called->getType(), scope);
        if (fp.fn) {
            // `let x = fp(...)`: declare `x` from the call's value (same as ordinary functions).
            if (isFromAssignment && !instanceName.empty()) {
                auto valueCall = std::make_shared<Call>(*this);
                valueCall->isFromAssignment = false;
                valueCall->instanceName.clear();

                auto declaration = std::make_shared<AssignVariable>(instanceName, nullptr, valueCall);
                declaration->setSpan(this->getSpan());
                return declaration->express(scope);
            }

            std::vector<std::shared_ptr<Expression>> argExprs;
            argExprs.reserve(args.size());
            for (const auto& arg : args) {
                auto argExpr = arg ? arg->express(scope) : nullptr;
                if (!argExpr) {
                    return nullptr; // the argument already reported its own error
                }
                argExprs.push_back(argExpr);
            }

            const size_t arity = fp.fn->getArity();
            if (argExprs.size() < arity || (!fp.fn->isVarArg && argExprs.size() != arity)) {
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Function pointer '%s' expects %zu argument(s) but %zu were provided",
                        callee.c_str(), arity, argExprs.size()),
                    "Check the argument list against the function pointer's type",
                    this->getSpan()
                );
                return nullptr;
            }

            type = fp.fn->getReturnType();
            auto callExpr = std::make_shared<CallExpression>(callee, argExprs, type);
            // Non-empty functionTypeName marks this as an indirect call in the IR generator.
            callExpr->functionTypeName = fp.typeName.empty() ? callee : fp.typeName;
            callExpr->setSpan(this->getSpan());
            return callExpr;
        }

        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure '%s' is a function or callable object\n"
            "2. Check for naming conflicts\n"
            "3. Verify type definitions",
            evaluatedCallee.c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("'%s' is not callable; it is of kind '%s'", 
                evaluatedCallee.c_str(),
                (called->getType() ? called->getType()->toString().c_str() : "null")),
            suggestion,
            this->getSpan()
        );
        return nullptr;
    }

    // `let x = f(...)` where f is an ordinary function (not a constructor):
    // the call only produces a value, so declare `x` in the current scope and
    // bind the call's result to it. Constructors keep the instance-name path below.
    if (isFromAssignment && !instanceName.empty() &&
        std::dynamic_pointer_cast<FunctionExpression>(called)) {
        auto valueCall = std::make_shared<Call>(*this);
        valueCall->isFromAssignment = false;
        valueCall->instanceName.clear();

        auto declaration = std::make_shared<AssignVariable>(instanceName, nullptr, valueCall);
        declaration->setSpan(this->getSpan());
        return declaration->express(scope);
    }
    
    auto parameters = callable->cloneParameters();
    type = callable->getType();
    if (type->isFunction()) {
        DEBUG_LOG("The function's return type is '" + type->getReturnType()->toString() + "'.");
        type = type->getReturnType();
    }
    
    // Process arguments and create call
    auto localScope = scope->createChildScope("call_" + evaluatedCallee);
    std::vector<std::shared_ptr<Expression>> collectedArgs;
    
    if (!processArguments(parameters, localScope, scope, collectedArgs)) {
        return nullptr;
    }
    
    return createCallExpression(evaluatedCallee, parameters, localScope, called, collectedArgs);
}

std::shared_ptr<Expression> Call::handleMemberAccessCall(
    std::shared_ptr<MemberAccess> memberAccess, SymbolTableType scope) {
    
    DEBUG_LOG("[Call] Handling member access call");
    
    // First, evaluate the member access to get its expression and type
    auto memberExpr = memberAccess->express(scope);
    if (!memberExpr) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify the base object exists\n"
                               "2. Check member accessibility\n"
                               "3. Ensure proper initialization";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to evaluate member access expression",
            suggestion,
            memberAccess->getSpan()
        );
        return nullptr;
    }
    
    // Extract the type name from the member access type
    auto memberType = memberExpr->getType();
    std::string baseTypeName;
    
    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(memberType)) {
        baseTypeName = udt->name;
    } else if (auto pointeeType = memberType->getBasePointeeType()) {
        if (auto pointeeUdt = std::dynamic_pointer_cast<UserDefinedType>(pointeeType)) {
            baseTypeName = pointeeUdt->name;
        }
    }
    
    if (baseTypeName.empty()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Check type definition is complete\n"
                               "2. Verify proper type imports\n"
                               "3. Ensure correct type hierarchy";
        console.reportError(
            Console::TYPE_ERROR,
            "Could not determine base type for member access",
            suggestion,
            memberAccess->getSpan()
        );
        return nullptr;
    }
    
    // Build method name: TypeName.methodName
    std::string methodName = baseTypeName + "." + callee;
    DEBUG_LOG("[Call] Looking for method: " + methodName);
    
    // Find the method
    std::shared_ptr<Expression> method = scope->get(methodName);
    if (!method) {
        // Try overload resolution
        auto overloads = scope->getOverloads(methodName);
        if (!overloads.empty()) {
            method = resolveMethodOverload(overloads, memberExpr, scope);
        }
    }
    
    if (!method) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check method '%s' exists in type '%s'\n"
            "2. Verify method visibility\n"
            "3. Check for correct namespace or imports",
            callee.c_str(), baseTypeName.c_str()
        );
        console.reportError(
            Console::SEMANTIC_ERROR,
            Console::formatString("Method '%s' not found", methodName.c_str()),
            suggestion,
            this->getSpan()
        );
        return nullptr;
    }
    
    // Verify it's callable
    auto callable = std::dynamic_pointer_cast<Callable>(method);
    if (!callable) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure '%s' is a method or callable\n"
            "2. Check for naming conflicts\n"
            "3. Verify type definitions",
            methodName.c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("'%s' is not callable", methodName.c_str()),
            suggestion,
            this->getSpan()
        );
        return nullptr;
    }
    
    // Get parameters first: the method signature owns the canonical 'this' pointer type
    auto parameters = callable->cloneParameters();
    type = callable->getType();
    if (type->isFunction()) {
        type = type->getReturnType();
    }

    // Prepare arguments with 'this' as first argument (same scheme as MemberAccess::express)
    std::shared_ptr<Type> thisPtrType;
    if (!parameters.empty() && parameters[0]->getType() && parameters[0]->getType()->isPointer()) {
        thisPtrType = parameters[0]->getType();
    } else if (memberType->isPointer()) {
        thisPtrType = memberType;
    } else {
        thisPtrType = Type::createPointerType(memberType);
    }

    std::shared_ptr<Statement> thisArg;
    if (memberType->isPointer()) {
        // Already an address: pass the member access itself
        thisArg = memberAccess;
    } else {
        // Reference the member access *statement* (not its debug string, which is not a scope key)
        auto ref = std::make_shared<ReferenceTo>(std::static_pointer_cast<Statement>(memberAccess));
        ref->setType(thisPtrType);
        ref->setRootType(thisPtrType);
        ref->setSpan(this->getSpan());
        thisArg = ref;
    }

    std::vector<std::shared_ptr<Statement>> methodArgs;
    methodArgs.push_back(thisArg);
    
    // Add the original arguments
    methodArgs.insert(methodArgs.end(), args.begin(), args.end());
    
    // Replace args with method args for processing (restored afterwards)
    auto originalArgs = std::move(args);
    args = std::move(methodArgs);
    
    auto localScope = scope->createChildScope("call_" + methodName);
    std::vector<std::shared_ptr<Expression>> collectedArgs;
    
    if (!processArguments(parameters, localScope, scope, collectedArgs)) {
        args = std::move(originalArgs);
        return nullptr;
    }
    
    auto callResult = createCallExpression(methodName, parameters, localScope, method, collectedArgs);
    args = std::move(originalArgs);
    return callResult;
}

std::shared_ptr<Expression> Call::resolveMethodOverload(
    const std::vector<std::shared_ptr<Expression>>& overloads,
    std::shared_ptr<Expression> baseExpr,
    SymbolTableType scope) {
    
    DEBUG_LOG("[Call] Resolving method overload from " + std::to_string(overloads.size()) + " candidates");
    
    for (auto& overload : overloads) {
        auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(overload);
        if (!funcExpr) continue;
        
        auto paramList = funcExpr->getParameters();
        if (paramList.empty()) continue;
        
        // Check if first parameter matches the base expression type (for 'this')
        auto firstParam = std::dynamic_pointer_cast<FunctionInputExpression>(paramList[0]);
        if (!firstParam) continue;
        
        if (Type::isSameOrCastableTo(baseExpr->getType(), firstParam->getType())) {
            // This overload is compatible with the base type
            DEBUG_LOG("[Call] Found compatible method overload: " + funcExpr->mangledName);
            return funcExpr;
        }
    }
    
    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Check method signature compatibility\n"
        "2. Verify base type '%s' has the method\n"
        "3. Ensure correct parameter types",
        baseExpr->getType()->toString().c_str()
    );
    console.reportError(
        Console::TYPE_ERROR,
        Console::formatString("No compatible method overload found for base type '%s'", 
            baseExpr->getType()->toString().c_str()),
        suggestion,
        this->getSpan()
    );
    return nullptr;
}

std::string Call::resolveImpliedTargetName(const std::string& targetName) {
    if (targetName == "this" && instanceName.empty()) {
        if (!accessContext.empty()) {
            return accessContext.back();
        } else {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure 'this' is used within a valid class/struct context\n"
                                   "2. Check for proper instance scope\n"
                                   "3. Verify method is called within a member function";
            console.reportError(
                Console::SEMANTIC_ERROR,
                "Accessing 'this' outside of any valid context",
                suggestion,
                this->getSpan()
            );
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
            if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(memberType)) {
                typeName = udt->name;
            } else if (auto pointeeType = memberType->getBasePointeeType()) {
                if (auto pointeeUdt = std::dynamic_pointer_cast<UserDefinedType>(pointeeType)) {
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
                // A struct/class name is a type, not a receiver: a bare constructor call such as
                // `PerformanceConfig(120, 60)` must not get an implicit 'this' argument.
                const bool isTypeName = std::dynamic_pointer_cast<StructExpression>(obj) != nullptr ||
                                        std::dynamic_pointer_cast<ClassExpression>(obj) != nullptr;
                if (isTypeName) {
                    contextualName += accessContext[i];
                } else if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(obj->getType())) {
                    contextualName += udt->name;
                    addThisArgument(actualTargetName, udt);
                } else {
                    contextualName += accessContext[i];
                }
            } else {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify object '%s' is defined\n"
                    "2. Check scope accessibility\n"
                    "3. Ensure proper initialization",
                    actualTargetName.c_str()
                );
                // Not an object: the inherited access context may simply be stale
                // (e.g. `g = f(...)`). Don't report here; Call::express falls back
                // to a plain lookup and reports "Callable not found" if that fails too.
                (void)suggestion;
                DEBUG_LOG("[Call] '" + actualTargetName + "' is not an object in scope; "
                          "will fall back to a plain lookup if the contextual name fails.");
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

void Call::addThisArgument(const std::string& targetName, std::shared_ptr<UserDefinedType> udt) {
    auto thisArg = std::make_shared<ReferenceTo>(instanceName.empty() ? targetName : instanceName);
    thisArg->setType(Type::createPointerType(udt));
    thisArg->setRootType(thisArg->getType());
    thisArg->setSpan(this->getSpan());
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

std::shared_ptr<Expression> Call::findCallable(const std::string& contextualName, SymbolTableType scope) {
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
    auto result = scope->get(callee);
    if (!result) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify function '%s' is defined\n"
            "2. Check for correct scope or namespace\n"
            "3. Ensure proper import or module inclusion",
            callee.c_str()
        );
        console.reportError(
            Console::SEMANTIC_ERROR,
            Console::formatString("Callable '%s' not found", callee.c_str()),
            suggestion,
            this->getSpan()
        );
    }
    return result;
}

std::vector<std::shared_ptr<Expression>> Call::findOverloadsInContext(SymbolTableType scope) {
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

std::shared_ptr<Expression> Call::resolveOverload(
    const std::vector<std::shared_ptr<Expression>>& overloads, 
    SymbolTableType scope) {
    
    DEBUG_LOG("[Call] Found " + std::to_string(overloads.size()) + " overload candidates");
    
    for (auto& overload : overloads) {
        auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(overload);
        if (!funcExpr) {
            DEBUG_LOG("[Call] Skipping non-function overload.");
            continue;
        }
        
        DEBUG_LOG("[Call] Checking if the overload '" + funcExpr->mangledName + "' (" + funcExpr->name + ") is the required overload.");
        
        auto paramList = funcExpr->getParameters();
        std::vector<std::shared_ptr<FunctionInputExpression>> inputParams;
        for (const auto& param : paramList) {
            auto casted = std::dynamic_pointer_cast<FunctionInputExpression>(param);
            if (!casted) {
                std::string suggestion = "To resolve this:\n"
                                       "1. Check parameter definitions\n"
                                       "2. Verify function signature\n"
                                       "3. Ensure correct type casting";
                console.reportError(
                    Console::INTERNAL_ERROR,
                    "Failed to cast parameter to FunctionInputExpression",
                    suggestion,
                    this->getSpan()
                );
                continue;
            }
            inputParams.push_back(casted);
        }
        
        // With several overloads, skip the ones whose shape cannot accept this call (unknown named
        // argument, too many/few positional arguments). Without this, matchArgumentsToParameters
        // reports a hard error for every non-matching candidate even when a later overload matches.
        if (overloads.size() > 1) {
            std::unordered_set<std::string> paramNames, namedGiven;
            bool hasVariadic = false;
            for (const auto& ip : inputParams) {
                paramNames.insert(ip->name);
                if (ip->isVariadic) hasVariadic = true;
            }
            size_t positionalGiven = 0;
            bool shapeOk = true;
            for (const auto& a : args) {
                if (auto na = std::dynamic_pointer_cast<ArgumentStatement>(a)) {
                    if (!paramNames.count(na->getName())) { shapeOk = false; break; }
                    namedGiven.insert(na->getName());
                } else {
                    positionalGiven++;
                }
            }
            if (shapeOk) {
                size_t freeParams = 0, requiredFree = 0;
                for (const auto& ip : inputParams) {
                    if (ip->isVariadic || namedGiven.count(ip->name)) continue;
                    freeParams++;
                    if (!ip->defaultValue) requiredFree++;
                }
                if ((!hasVariadic && positionalGiven > freeParams) || positionalGiven < requiredFree) {
                    shapeOk = false;
                }
            }
            if (!shapeOk) {
                DEBUG_LOG("[Call] Overload '" + funcExpr->mangledName + "' skipped: argument shape does not fit.");
                continue;
            }
        }

        // Coerce argument types and evaluate
        coerceArgumentTypes(inputParams);
        auto evaluatedArgs = evaluateArguments(scope);
        if (evaluatedArgs.size() != args.size()) {
            // An argument failed to evaluate (already reported). No overload can match.
            DEBUG_LOG("[Call] Argument evaluation failed while resolving '" + callee + "'.");
            return nullptr;
        }
        
        if (matchArgumentsToParameters(evaluatedArgs, inputParams, scope)) {
            DEBUG_LOG("[Call] ✅ Matched overload: using mangled name '" + funcExpr->mangledName + "'");
            return funcExpr;
        } else {
            DEBUG_LOG("[Call] ❌ Overload '" + funcExpr->mangledName + "' did not match.");
        }
    }
    
    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Check function signature for '%s'\n"
        "2. Verify parameter types match\n"
        "3. Ensure correct argument count",
        callee.c_str()
    );
    console.reportError(
        Console::TYPE_ERROR,
        Console::formatString("No matching overload found for '%s'", callee.c_str()),
        suggestion,
        this->getSpan()
    );
    return nullptr;
}

void Call::coerceArgumentTypes(const std::vector<std::shared_ptr<FunctionInputExpression>>& inputParams) {
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

std::vector<std::shared_ptr<FunctionInputExpression>> Call::evaluateArguments(SymbolTableType scope) {
    std::vector<std::shared_ptr<FunctionInputExpression>> evaluatedArgs;
    
    for (const auto& arg : args) {
        std::shared_ptr<Expression> result;
        if (auto argStatement = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
            result = argStatement->value ? argStatement->value->express(scope) : nullptr;
            if (!result || !result->getType()) {
                // The argument already reported its own error. An empty result tells the caller
                // (size != args.size()) that evaluation failed, instead of dereferencing null.
                return {};
            }
            DEBUG_LOG("[Call] Evaluated a named argument '" + argStatement->getName() + "'.");
            auto inputExpr = std::make_shared<FunctionInputExpression>(argStatement->getName(), result->getType(), result);
            evaluatedArgs.push_back(inputExpr);
        } else {
            result = arg ? arg->express(scope) : nullptr;
            if (!result || !result->getType()) {
                return {};
            }
            DEBUG_LOG("[Call] Evaluated an unamed argument " + result->toString());
            auto inputExpr = std::make_shared<FunctionInputExpression>("", result->getType(), result);
            evaluatedArgs.push_back(inputExpr);
        }
        
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(arg)) {
            DEBUG_LOG("[Call] Evaluated argument has type: " + (typed->getType() ? typed->getType()->toString() : std::string("<null>")) + "'.");
        }
    }
    
    return evaluatedArgs;
}

std::string Call::getEvaluatedCalleeName(std::shared_ptr<Expression> called, const std::string& contextualName) {
    if (auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(called)) {
        return funcExpr->mangledName;
    }
    return contextualName;
}

bool Call::processArguments(
    const std::vector<std::shared_ptr<FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    SymbolTableType scope,
    std::vector<std::shared_ptr<Expression>>& collectedArgs) {
    
    std::unordered_set<std::string> providedParams;
    size_t positionalArgIndex = 0;
    size_t namedArgsCount = 0;
    
    DEBUG_LOG("[Call] Processing " + std::to_string(args.size()) + " arguments");
    
    // First pass: named arguments
    if (!processNamedArguments(parameters, localScope, scope, providedParams, namedArgsCount, collectedArgs)) {
        return false;
    }
    
    // Second pass: positional arguments and defaults
    if (!processPositionalArguments(parameters, localScope, scope, providedParams, positionalArgIndex, namedArgsCount, collectedArgs)) {
        return false;
    }
    
    return true;
}

bool Call::processNamedArguments(
    const std::vector<std::shared_ptr<FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    SymbolTableType scope,
    std::unordered_set<std::string>& providedParams,
    size_t& namedArgsCount,
    std::vector<std::shared_ptr<Expression>>& collectedArgs) {
    
    // Initialize collectedArgs with nullptrs for all parameters
    size_t totalArgsSize;
    if (args.size() > parameters.size()) {
        totalArgsSize = args.size();
    } else {
        totalArgsSize = parameters.size();
    }

    collectedArgs.resize(totalArgsSize, nullptr);
    
    for (const auto& arg : args) {
        if (auto namedArg = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
            namedArgsCount++;
            const std::string& paramName = namedArg->getName();
            DEBUG_LOG("[Call] Processing named argument '" + paramName + "'");
            
            int paramIndex = -1;
            for (int i = 0; i < parameters.size(); i++) {
                if (parameters[i]->name == paramName) {
                    paramIndex = i;
                    break;
                }
            }
            
            if (paramIndex == -1) {
                std::string availableParams;
                for (const auto& param : parameters) {
                    availableParams += " - " + param->name + "\n";
                }
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Check parameter name spelling\n"
                    "2. Available parameters:\n%s"
                    "3. Verify function signature",
                    availableParams.c_str()
                );
                console.reportError(
                    Console::SEMANTIC_ERROR,
                    Console::formatString("Unknown parameter '%s' for callable '%s'", 
                        paramName.c_str(), callee.c_str()),
                    suggestion,
                    namedArg->getSpan()
                );
                return false;
            }
            
            auto param = parameters[paramIndex];
            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(namedArg->value)) {
                typed->setType(param->getType());
                DEBUG_LOG("[Call] Set type for named argument '" + paramName + "' to '" + param->getType()->toString() + "'.");
            }
            
            auto evaluated = namedArg->value ? namedArg->value->express(scope) : nullptr;
            if (!evaluated) {
                std::string suggestion = "To resolve this:\n"
                                       "1. Verify argument expression is valid\n"
                                       "2. Check for proper initialization\n"
                                       "3. Add debug output for argument value";
                console.reportError(
                    Console::RUNTIME_ERROR,
                    Console::formatString("Failed to evaluate named argument '%s'", 
                        paramName.c_str()),
                    suggestion,
                    namedArg->getSpan()
                );
                return false;
            }
            localScope->set(paramName, evaluated);
            collectedArgs[paramIndex] = evaluated;
            providedParams.insert(paramName);
            DEBUG_LOG("[Call] Set named parameter '" + paramName + "' in local scope with value '" + 
                      evaluated->toString() + "' type '" + evaluated->getType()->toString() + "'.");
        }
    }
    
    return true;
}

bool Call::processPositionalArguments(
    const std::vector<std::shared_ptr<FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    SymbolTableType scope,
    const std::unordered_set<std::string>& providedParams,
    size_t& positionalArgIndex,
    size_t namedArgsCount,
    std::vector<std::shared_ptr<Expression>>& collectedArgs) 
{
    for (int i = 0; i < parameters.size(); i++) {
        auto& param = parameters[i];
        std::string paramName = param->name;

        // Skip parameters that were provided by name
        if (providedParams.count(paramName)) {
            continue;
        }

        // Handle variadic parameters
        if (param->isVariadic) {
            // Only the variadic path needs the concrete function; resolve it lazily here
            // instead of re-running overload resolution (and re-evaluating every argument)
            // for every call.
            auto called = findCallable(callee, scope);
            if (!called) {
                called = scope->get(callee);
            }
            if (!called) {
                console.reportError(
                    Console::SEMANTIC_ERROR,
                    Console::formatString("Function or type '%s' not found", callee.c_str()),
                    "To resolve this:\n"
                    "1. Verify function is defined\n"
                    "2. Check for correct scope or namespace\n"
                    "3. Ensure proper import or module inclusion",
                    this->getSpan()
                );
                return false;
            }
            auto calledFunc = std::dynamic_pointer_cast<FunctionExpression>(called);
            if (!handleVariadicParameter(parameters, localScope, scope, i, positionalArgIndex, calledFunc, collectedArgs)) {
                return false;
            }
            // After handling variadic, we're done with positional args
            break;
        }

        // Handle regular positional arguments
        if (positionalArgIndex < args.size()) {
            if (!processRegularPositionalArgument(args[positionalArgIndex], param, 
                                               localScope, scope, positionalArgIndex, i, collectedArgs)) {
                return false;
            }
            positionalArgIndex++;
        } 
        else if (param->defaultValue) {
            DEBUG_LOG("[Call] Using default value for parameter '" + paramName + "'");
            localScope->set(paramName, param->defaultValue);
            collectedArgs[i] = param->defaultValue;
        } 
        else {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Provide a value for parameter '%s'\n"
                "2. Check function signature\n"
                "3. Consider using a default value",
                paramName.c_str()
            );
            console.reportError(
                Console::SEMANTIC_ERROR,
                Console::formatString("Missing required argument for parameter '%s'", 
                    paramName.c_str()),
                suggestion,
                this->getSpan()
            );
            return false;
        }
    }

    // Check for extra arguments (only if no variadic parameter consumed them)
    if (positionalArgIndex < args.size() && 
        !(parameters.size() > 0 && parameters.back()->isVariadic)) {
        std::string expectedParams;
        for (const auto& param : parameters) {
            expectedParams += " - " + param->name + "\n";
        }
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check number of arguments required\n"
            "2. Expected parameters:\n%s"
            "3. Verify function signature",
            expectedParams.c_str()
        );
        console.reportError(
            Console::SEMANTIC_ERROR,
            Console::formatString("Too many arguments provided to '%s'", callee.c_str()),
            suggestion,
            this->getSpan()
        );
        return false;
    }

    return true;
}

bool Call::handleVariadicParameter(
    const std::vector<std::shared_ptr<FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    SymbolTableType scope,
    int& i,
    size_t& positionalArgIndex,
    std::shared_ptr<FunctionExpression> calledFunc,
    std::vector<std::shared_ptr<Expression>>& collectedArgs) 
{
    if (!calledFunc) {
        console.reportError(
            Console::INTERNAL_ERROR,
            Console::formatString("Variadic call to '%s' did not resolve to a function", callee.c_str()),
            "Check that the callee is a declared function",
            this->getSpan()
        );
        return false;
    }

    // Verify this is actually a variadic parameter
    if (i >= parameters.size() || !parameters[i]->isVariadic) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify variadic parameter declaration\n"
                               "2. Check function signature\n"
                               "3. Ensure correct parameter index";
        console.reportError(
            Console::INTERNAL_ERROR,
            "Invalid variadic parameter access",
            suggestion,
            this->getSpan()
        );
        return false;
    }

    auto& variadicParam = parameters[i];
    std::string paramName = variadicParam->name;
    DEBUG_LOG("[Call] Handling variadic parameter '" + paramName + "'");

    // Get the expected element type (for arrays, get the element type)
    auto expectedType = variadicParam->getType();
    if (expectedType->isArray()) {
        expectedType = expectedType->getBasePointeeType();
    }

    std::vector<std::shared_ptr<Expression>> variadicArgs;
    int varArgsCount = 0;

    // Collect all remaining arguments
    while (positionalArgIndex < args.size()) {
        auto arg = args[positionalArgIndex];
        
        // Error if named arguments appear in variadic section
        if (auto namedArg = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Place named arguments before variadic arguments\n"
                "2. Check argument order\n"
                "3. Verify function signature for '%s'",
                callee.c_str()
            );
            console.reportError(
                Console::SEMANTIC_ERROR,
                Console::formatString("Named argument '%s' cannot appear after variadic arguments", 
                    namedArg->getName().c_str()),
                suggestion,
                namedArg->getSpan()
            );
            positionalArgIndex++;
            continue;
        }

        // Evaluate the argument
        auto value = arg->express(scope);
        if (!value || value->getType()->isInvalid()) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify argument expression is valid\n"
                                   "2. Check for proper initialization\n"
                                   "3. Add debug output for argument value";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Invalid value in variadic argument",
                suggestion,
                arg->getSpan()
            );
            positionalArgIndex++;
            continue;
        }
        
        // Check type compatibility
        // if (!Type::isSameOrCastableTo(value->getType(), expectedType)) {
        //     std::string suggestion = Console::formatString(
        //         "To resolve this:\n"
        //         "1. Check type requirements\n"
        //         "2. Available conversions:\n"
        //         " - Explicit cast: `(%s)value`\n"
        //         " - Conversion method: `value.to_%s()`\n"
        //         "3. Verify source type implements required traits",
        //         expectedType->toString().c_str(),
        //         expectedType->toString().c_str()
        //     );
        //     console.reportError(
        //         Console::TYPE_ERROR,
        //         Console::formatString("Variadic argument type '%s' does not match expected type '%s'", 
        //             value->getType()->toString().c_str(), expectedType->toString().c_str()),
        //         suggestion,
        //         arg->getSpan()
        //     );
        //     positionalArgIndex++;
        //     continue;
        // }
        
        variadicArgs.push_back(value);
        varArgsCount++;
        positionalArgIndex++;
    }

    // Add all variadic arguments to collected args
    for (auto& varArg : variadicArgs) {
        collectedArgs[i] = varArg;
        i++;
    }

    // For external functions, we just pass the arguments directly
    if (calledFunc->isExtern) {
        // Store the variadic arguments directly in the scope
        for (size_t j = 0; j < variadicArgs.size(); j++) {
            localScope->set(paramName + "_" + std::to_string(j), variadicArgs[j]);
        }
    } 
    else {
        // For non-extern functions, create an array value
        // auto arrayType = Type::createArrayType(expectedType);
        // auto arrayValue = std::make_shared<ArrayExpression>(
        //     arrayType, variadicArgs, /* isVariadic */ true);
        // localScope->set(paramName, arrayValue);

        // // Store count if needed
        // if (!calledFunc->isIntrinsic) {
        //     auto argsCountExpr = std::make_shared<Integer<int>>(varArgsCount);
        //     localScope->set(paramName + "_count", argsCountExpr);
        // }
    }

    DEBUG_LOG("[Call] Bound variadic parameter '" + paramName + "' with " + 
              std::to_string(variadicArgs.size()) + " arguments");
    
    return true;
}

bool Call::processRegularPositionalArgument(
    std::shared_ptr<Statement> arg,
    std::shared_ptr<FunctionInputExpression> param,
    SymbolTableType localScope,
    SymbolTableType scope,
    size_t& positionalArgIndex,
    int paramIndex,
    std::vector<std::shared_ptr<Expression>>& collectedArgs)
{
    // The argument may be a synthesized node (e.g. the implicit 'this') with no span
    auto reportSpan = arg->getSpan();
    if (reportSpan.start.line == static_cast<decltype(reportSpan.start.line)>(-1)) {
        reportSpan = this->getSpan();
    }

    // Error if named arguments appear in positional section
    if (std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
        std::string suggestion = "To resolve this:\n"
                               "1. Place named arguments before positional arguments\n"
                               "2. Check argument order\n"
                               "3. Verify function signature";
        console.reportError(
            Console::SEMANTIC_ERROR,
            "Positional argument after named argument is not allowed",
            suggestion,
            reportSpan
        );
        return false;
    }

    // Evaluate the argument
    auto value = arg->express(scope);
    if (!value || value->getType()->isInvalid()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify argument expression is valid\n"
                               "2. Check for proper initialization\n"
                               "3. Add debug output for argument value";
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Invalid argument for parameter '%s'", 
                param->name.c_str()),
            suggestion,
            reportSpan
        );
        return false;
    }

    // Check type compatibility
    if (!Type::isSameOrCastableTo(value->getType(), param->getType())) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check type requirements\n"
            "2. Available conversions:\n"
            " - Explicit cast: `(%s)value`\n"
            " - Conversion method: `value.to_%s()`\n"
            "3. Verify source type implements required traits",
            param->getType()->toString().c_str(),
            param->getType()->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            bindMismatchMessage(value->getType()->toString(), param->name, value->getType(), param->getType()),
            suggestion,
            reportSpan
        );
        return false;
    }

    // Store in scope and collected args
    localScope->set(param->name, value);
    collectedArgs[paramIndex] = value;
    DEBUG_LOG("[Call] Set positional argument for '" + param->name + "' with value '" + 
              value->toString() + "' and type '" + value->getType()->toString() + "'.");

    return true;
}

std::shared_ptr<Expression> Call::createCallExpression(
    const std::string& evaluatedCallee,
    const std::vector<std::shared_ptr<FunctionInputExpression>>& parameters,
    SymbolTableType localScope,
    std::shared_ptr<Expression> called,
    const std::vector<std::shared_ptr<Expression>>& collectedArgs) {
    
    DEBUG_LOG("[Call] Creating CallExpression with " + std::to_string(collectedArgs.size()) + " collected arguments");
    
    // Use collected args directly instead of processing parameters
    std::vector<std::shared_ptr<Expression>> finalArgs = collectedArgs;
    
    if (std::dynamic_pointer_cast<FunctionExpression>(called)) {
        DEBUG_LOG("[Call] Returning CallExpression for '" + evaluatedCallee + "' with " + 
                  std::to_string(finalArgs.size()) + " args");
        auto callExpr = std::make_shared<CallExpression>(evaluatedCallee, finalArgs, type);
        callExpr->setSpan(this->getSpan());
        return callExpr;
    }
    
    // Handle constructor calls
    // An unnamed constructor call (e.g. `this.field = Type(1, 2)`) still needs somewhere to
    // construct the object. With an empty instance name the IR generator treats it as a plain
    // function call and fails with "Callable 'Type' not found", so give it a temporary name.
    static int anonymousConstructorCounter = 0;
    const std::string ctorInstanceName = instanceName.empty()
        ? "__ctor_tmp_" + evaluatedCallee + "_" + std::to_string(anonymousConstructorCounter++)
        : instanceName;
    auto instanceConstructor = std::make_shared<CallExpression>(evaluatedCallee, ctorInstanceName, finalArgs);
    // Without a type, the constructed value can't be used as a field default or an assigned value
    instanceConstructor->type = type;
    int index = 0;
    for (const auto& param : parameters) {
        if (index < finalArgs.size()) {
            auto instanceMember = std::make_shared<MemberExpression>(
                param->getName(),
                param->getType(),
                finalArgs[index]
            );
            instanceConstructor->members.push_back(instanceMember);
        }
        index++;
    }
    
    instanceConstructor->setSpan(this->getSpan());
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
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify function '%s' is defined\n"
            "2. Check for correct scope or namespace\n"
            "3. Ensure proper import or module inclusion",
            calleeName.c_str()
        );
        console.reportError(
            Console::SEMANTIC_ERROR,
            Console::formatString("No overloads found for '%s'", calleeName.c_str()),
            suggestion
        );
        return "";
    }

    for (const auto& overload : overloads) {
        auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(overload);
        if (!funcExpr) {
            DEBUG_LOG("[OverloadResolver] Skipping non-function overload for '" + calleeName + "'");
            continue;
        }

        auto paramList = funcExpr->getParameters();
        std::vector<std::shared_ptr<FunctionInputExpression>> inputParams;
        for (const auto& param : paramList) {
            auto casted = std::dynamic_pointer_cast<FunctionInputExpression>(param);
            if (!casted) {
                std::string suggestion = "To resolve this:\n"
                                       "1. Check parameter definitions\n"
                                       "2. Verify function signature\n"
                                       "3. Ensure correct type casting";
                console.reportError(
                    Console::INTERNAL_ERROR,
                    Console::formatString("Failed to cast parameter to FunctionInputExpression for '%s'", 
                        funcExpr->mangledName.c_str()),
                    suggestion
                );
                continue;
            }
            inputParams.push_back(casted);
        }

        std::vector<std::shared_ptr<FunctionInputExpression>> evaluatedArgs;
        for (const auto& arg : args) {
            std::shared_ptr<Expression> result;
            if (auto namedArg = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                result = namedArg->value->express(scope);
                if (!result) {
                    std::string suggestion = "To resolve this:\n"
                                           "1. Verify named argument '%s' is valid\n"
                                           "2. Check for proper initialization\n"
                                           "3. Add debug output for argument value";
                    console.reportError(
                        Console::RUNTIME_ERROR,
                        Console::formatString("Failed to evaluate named argument '%s' for '%s'", 
                            namedArg->getName().c_str(), calleeName.c_str()),
                        suggestion,
                        namedArg->getSpan()
                    );
                    continue;
                }
                evaluatedArgs.push_back(std::make_shared<FunctionInputExpression>(
                    namedArg->getName(), result->getType(), result));
            } else {
                result = arg->express(scope);
                if (!result) {
                    std::string suggestion = "To resolve this:\n"
                                           "1. Verify argument expression is valid\n"
                                           "2. Check for proper initialization\n"
                                           "3. Add debug output for argument value";
                    console.reportError(
                        Console::RUNTIME_ERROR,
                        Console::formatString("Failed to evaluate argument for '%s'", 
                            calleeName.c_str()),
                        suggestion,
                        arg->getSpan()
                    );
                    continue;
                }
                evaluatedArgs.push_back(std::make_shared<FunctionInputExpression>(
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

    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Check function signature for '%s'\n"
        "2. Verify argument types and count\n"
        "3. Ensure correct parameter order",
        calleeName.c_str()
    );
    console.reportError(
        Console::TYPE_ERROR,
        Console::formatString("No matching overload found for '%s'", calleeName.c_str()),
        suggestion
    );
    return "";
}

bool Call::matchArgumentsToParameters(
    const std::vector<std::shared_ptr<FunctionInputExpression>>& args,
    const std::vector<std::shared_ptr<FunctionInputExpression>>& params,
    SymbolTableType scope
) {
    DEBUG_LOG("[Call] Starting argument-to-parameter matching");

    std::unordered_set<std::string> matchedNames;
    std::unordered_map<std::string, std::shared_ptr<FunctionInputExpression>> namedArgs;
    std::vector<std::shared_ptr<FunctionInputExpression>> positionalArgs;

    // Separate named and positional args
    for (const auto& arg : args) {
        if (!arg->name.empty()) {
            DEBUG_LOG("[Call] Found named arg: " + arg->name);
            namedArgs[arg->name] = arg;
        } else {
            if (!arg->defaultValue->getRootType() || arg->defaultValue->getRootType()->isInvalid()) {
                arg->defaultValue->rootType = arg->defaultValue->getType();
            }
            DEBUG_LOG("[Call] Found positional arg at index: " + std::to_string(positionalArgs.size()) + 
                      " of kind '" + (arg->defaultValue->getRootType() ? arg->defaultValue->getRootType()->toString() : std::string("<null>")) + "'.");
            positionalArgs.push_back(arg);
        }
    }

    size_t positionalIndex = 0;

    for (const auto& param : params) {
        const std::string& paramName = param->name;
        std::shared_ptr<FunctionInputExpression> matchingArg;

        DEBUG_LOG("[Call] Matching parameter: " + paramName);

        if (param->isVariadic) {
            DEBUG_LOG("[Call] Parameter is variadic: " + paramName);
            while (positionalIndex < positionalArgs.size()) {
                auto arg = positionalArgs[positionalIndex];
                auto argType = ((!arg->defaultValue->getRootType() || arg->defaultValue->getRootType()->isInvalid()) ? 
                              arg->defaultValue->getType() : 
                              arg->defaultValue->getRootType());
                auto expectedType = param->getType()->isArray() ? param->getType()->getBasePointeeType() : param->getType();
                
                // if (!Type::isSameOrCastableTo(argType, expectedType)) {
                //     std::string suggestion = Console::formatString(
                //         "To resolve this:\n"
                //         "1. Check type requirements for variadic parameter\n"
                //         "2. Available conversions:\n"
                //         " - Explicit cast: `(%s)value`\n"
                //         " - Conversion method: `value.to_%s()`\n"
                //         "3. Verify source type implements required traits",
                //         expectedType->toString().c_str(),
                //         expectedType->toString().c_str()
                //     );
                //     console.reportError(
                //         Console::TYPE_ERROR,
                //         Console::formatString("Variadic argument type '%s' does not match expected type '%s' for parameter '%s'", 
                //             argType->toString().c_str(), expectedType->toString().c_str(), paramName.c_str()),
                //         suggestion
                //     );
                //     return false;
                // }
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
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Provide a value for parameter '%s'\n"
                "2. Check function signature\n"
                "3. Consider using a default value",
                paramName.c_str()
            );
            console.reportError(
                Console::SEMANTIC_ERROR,
                Console::formatString("Missing required argument for parameter '%s'", 
                    paramName.c_str()),
                suggestion
            );
            return false;
        }

        auto matchingArgType = ((!matchingArg->defaultValue->getRootType() || matchingArg->defaultValue->getRootType()->isInvalid()) ? 
                               matchingArg->defaultValue->getType() : 
                               matchingArg->defaultValue->getRootType());
        
        if (!Type::isSameOrCastableTo(matchingArgType, param->getType())) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Check type requirements\n"
                "2. Available conversions:\n"
                " - Explicit cast: `(%s)value`\n"
                " - Conversion method: `value.to_%s()`\n"
                "3. Verify source type implements required traits",
                param->getType()->toString().c_str(),
                param->getType()->toString().c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                bindMismatchMessage(matchingArgType->toString(), paramName, matchingArgType, param->getType()),
                suggestion
            );
            return false;
        }
    }

    for (const auto& [name, _] : namedArgs) {
        if (matchedNames.count(name) == 0) {
            std::string availableParams;
            for (const auto& param : params) {
                availableParams += " - " + param->name + "\n";
            }
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Check parameter name spelling\n"
                "2. Available parameters:\n%s"
                "3. Verify function signature",
                availableParams.c_str()
            );
            console.reportError(
                Console::SEMANTIC_ERROR,
                Console::formatString("Unused named argument '%s'", name.c_str()),
                suggestion
            );
            return false;
        }
    }

    if (positionalIndex < positionalArgs.size()) {
        std::string expectedParams;
        for (const auto& param : params) {
            expectedParams += " - " + param->name + "\n";
        }
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check number of arguments required\n"
            "2. Expected parameters:\n%s"
            "3. Verify function signature",
            expectedParams.c_str()
        );
        console.reportError(
            Console::SEMANTIC_ERROR,
            "Too many positional arguments provided",
            suggestion
        );
        return false;
    }

    DEBUG_LOG("[Call] All arguments matched successfully");
    return true;
}

} // namespace Omniscript
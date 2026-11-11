// MemberAccessStatement.cpp
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Expressions/ClassExpression.h>
#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/FunctionInputExpression.h>
#include <omniscript/Expressions/StructExpression.h>
#include <omniscript/Expressions/AccessExpressions.h>
#include <omniscript/Expressions/AggregateExpressions.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Symboltable.h>

namespace Omniscript {

namespace {
// `Stats?` has no members of its own: member lookups (fields and methods) go through the wrapped
// type, otherwise the base type name comes out as "<null>" and `r.report()` is looked up as
// "<null>.report".
std::shared_ptr<Type> unwrapNullableType(std::shared_ptr<Type> t) {
    while (auto nullable = std::dynamic_pointer_cast<NullableType>(t)) {
        if (!nullable->innerType) break;
        t = nullable->innerType;
    }
    return t;
}
} // namespace

MemberAccess::MemberAccess(const std::string& obj, const std::string& member, std::shared_ptr<Statement> assignVal) {
    this->objectName = obj;
    this->memberName = member;
    this->name = obj;
    setAssignmentValueTo(assignVal);
}

MemberAccess::MemberAccess(std::shared_ptr<Statement> obj, const std::string& member, std::shared_ptr<Statement> assignVal) {
    this->expr = obj;
    this->object = obj;
    this->memberName = member;
    auto named = std::dynamic_pointer_cast<NamedStatement>(obj);
    if (!named) {
        console.reportError(Console::SEMANTIC_ERROR, "The object having members should be named", FileSpan{});
    } else {
        this->name = named->getName();
    }
    setAssignmentValueTo(assignVal);
}

// Public methods
const std::shared_ptr<Statement>& MemberAccess::getObject() const {
    return object;
}

std::shared_ptr<Statement> MemberAccess::clone() const {
    auto cloned = object
        ? std::make_shared<MemberAccess>(expr->clone(), memberName, assignmentValue ? assignmentValue->clone() : nullptr)
        : std::make_shared<MemberAccess>(objectName, memberName, assignmentValue ? assignmentValue->clone() : nullptr);

    cloned->arguments.reserve(arguments.size());
    for (const auto& arg : arguments) {
        cloned->arguments.push_back(arg->clone());
    }
    cloned->isCall = isCall;
    cloned->setSpan(getSpan());
    return cloned;
}

std::shared_ptr<Statement> MemberAccess::evaluate(SymbolTableType scope) {
    return nullptr; // Evaluation logic to be filled
}

std::shared_ptr<Expression> MemberAccess::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    
    // Validate access context chain
    validateAccessChain(scope);
    
    // Get base expression and type info using public methods
    std::shared_ptr<Expression> baseExpr;
    std::string baseTypeName;
    std::string resolvedObjectName;
    
    // Resolve base expression manually since resolveBaseExpression might be private
    DEBUG_LOG("The object is " + (object ? object->toString() : "null"));
    
    if (!object) {
        // Handle string-based object name
        auto expr = scope->get(objectName);
        if (!expr) {
            console.reportError(Console::SEMANTIC_ERROR, "Variable '" + objectName + "' not found in scope", FileSpan{});
            return nullptr;
        }
        baseExpr = std::make_shared<VariableAccessExpression>(objectName, expr->getType());
        baseTypeName = extractTypeName(unwrapNullableType(expr->getType()));
        resolvedObjectName = objectName;
    } else {
        // Handle expression-based object
        baseExpr = object->express(scope);
        if (!baseExpr) {
            console.reportError(Console::SEMANTIC_ERROR, "Failed to evaluate base expression for member access", FileSpan{});
            return nullptr;
        }
        baseTypeName = extractTypeName(unwrapNullableType(baseExpr->getType()));
        if (auto named = std::dynamic_pointer_cast<NamedStatement>(object)) {
            resolvedObjectName = named->getName();
        }
    }
    
    if (!baseExpr) {
        return nullptr;
    }
    
    DEBUG_LOG("Base type name: " + baseTypeName + " (from '" +
              (baseExpr->getType() ? baseExpr->getType()->toString() : std::string("<no type>")) + "')");

    if (isMethodCall() || !arguments.empty()) {
        DEBUG_LOG("[MemberAccess] Detected method call: " + baseTypeName + "." + memberName);
        
        // Look for method with mangled name: TypeName.methodName
        std::string methodName = baseTypeName + "." + memberName;
        auto method = scope->get(methodName);
        
        if (!method) {
            // Check for overloaded methods
            auto overloads = scope->getOverloads(methodName);
            if (!overloads.empty()) {
                DEBUG_LOG("[MemberAccess] Found overloaded methods for: " + methodName);
                method = overloads[0]; // Use first overload for now
            }
        }

        // Module alias call, e.g. `CMath.fabs(x)` with `module CMath = import "...";`.
        // The alias is not a value, so it has no type to mangle the method name from
        // (that is where "<null>.fabs" came from) and there is no 'this' argument.
        // Only alias-qualified names are tried: falling back to the bare name could
        // resolve to a wrapper of the same name (Math.os abs -> abs) and recurse forever.
        std::string moduleCallName;
        if (!method) {
            std::string aliasName = resolvedObjectName.empty() ? objectName : resolvedObjectName;
            std::vector<std::string> candidates;
            if (!aliasName.empty()) {
                candidates.push_back(aliasName + "." + memberName);
                std::string qualifiedName;
                for (const auto& context : accessContext) {
                    if (!qualifiedName.empty()) qualifiedName += ".";
                    qualifiedName += context;
                    candidates.push_back(qualifiedName + "." + aliasName + "." + memberName);
                }
            }
            for (const auto& candidate : candidates) {
                DEBUG_LOG("[MemberAccess] Trying module-qualified name: " + candidate);
                auto found = scope->get(candidate);
                if (!found) {
                    auto moduleOverloads = scope->getOverloads(candidate);
                    if (!moduleOverloads.empty()) found = moduleOverloads[0];
                }
                if (found) {
                    method = found;
                    moduleCallName = candidate;
                    break;
                }
            }
        }
        
        if (method) {
            DEBUG_LOG("[MemberAccess] Found method: " + methodName);

            if (!moduleCallName.empty()) {
                auto moduleCall = std::make_shared<Call>(moduleCallName, arguments);
                moduleCall->setSpan(this->getSpan());
                return moduleCall->express(scope);
            }
            
            // Create the 'this' argument for the method call
            std::shared_ptr<Statement> thisArgument;
            
            if (object) {
                // For nested access like this.position.log(), use the object directly
                // Don't create a MemberAccess with the method name - methods aren't data members!
                thisArgument = object;
            } else {
                // Simple case: just use the object name
                auto getVar = std::make_shared<GetVariable>(objectName);
                getVar->setSpan(this->getSpan());
                thisArgument = getVar;
            }
            
            // Prefer the 'this' pointer type declared by the resolved method's own signature,
            // so the argument and parameter share the same type object (no fresh
            // createPointerType per evaluation).
            std::shared_ptr<Type> thisPtrType;
            if (auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(method)) {
                auto sigParams = funcExpr->getParameters();
                if (!sigParams.empty()) {
                    if (auto first = std::dynamic_pointer_cast<FunctionInputExpression>(sigParams[0])) {
                        auto sigType = first->getType();
                        if (sigType && sigType->isPointer() && extractTypeName(sigType) == baseTypeName) {
                            thisPtrType = sigType;
                        }
                    }
                }
            }

            std::shared_ptr<Statement> thisRef;
            auto baseType = unwrapNullableType(baseExpr->getType());
            if (baseType && baseType->isPointer()) {
                // The base is already a pointer (e.g. 'this' inside a method): pass it as-is
                // instead of taking the address of a pointer (which gave '* * T').
                thisRef = thisArgument;
            } else {
                auto ref = std::make_shared<ReferenceTo>(thisArgument);
                ref->setType(thisPtrType ? thisPtrType : Type::createPointerType(baseType));
                ref->setRootType(ref->getType());
                thisRef = ref;
            }
            // Without a span, downstream diagnostics print u64(-1):u64(-1)
            thisRef->setSpan(this->getSpan());
            
            // Insert the object reference as the first argument
            std::vector<std::shared_ptr<Statement>> methodArgs;
            methodArgs.push_back(thisRef);
            
            // Add the original arguments
            methodArgs.insert(methodArgs.end(), arguments.begin(), arguments.end());
            
            DEBUG_LOG("[MemberAccess] Creating method call with " + std::to_string(methodArgs.size()) + 
                    " arguments, first arg (this): " + thisRef->toString() + 
                    " of type: " + (thisPtrType ? thisPtrType->toString() : baseType->toString()));
            
            // Create a Call statement to handle the method call properly
            // IMPORTANT: Don't set access context on the method call to avoid contextual name building
            auto methodCall = std::make_shared<Call>(methodName, methodArgs);
            // methodCall->setAccessContext(accessContext); // Remove this line!
            methodCall->setSpan(this->getSpan());
            
            // Express the call to get the final CallExpression
            return methodCall->express(scope);
        } else {
            console.reportError(Console::SEMANTIC_ERROR, "Method '" + methodName + "' not found in scope", FileSpan{});
            return nullptr;
        }
    }

    // Original member access logic for data members
    // Get user-defined type
    auto userType = getUserDefinedType(scope, baseTypeName);
    if (!userType) {
        return nullptr;
    }
    
    // Find member and set type
    int memberIndex = findMemberIndex(userType);
    if (memberIndex == -1) {
        return nullptr;
    }
    
    // Handle assignment if present
    auto assignmentExpr = processAssignment(scope);
    if (assignmentValue && !assignmentExpr) {
        return nullptr;
    }
    
    // Create final expression
    auto result = std::make_shared<MemberAccessExpression>(
        baseExpr, baseTypeName, resolvedObjectName, memberName, 
        memberIndex, type, assignmentExpr
    );
    result->type = type;
    result->setSpan(this->getSpan());
    return result;
}

std::string MemberAccess::toString() const {
    std::string base = object ? "ObjectMember(" + object->toString() + ")" : objectName;
    base += "." + memberName;

    if (isCall) {
        std::string argsStr = "(";
        for (size_t i = 0; i < arguments.size(); ++i) {
            argsStr += arguments[i] ? arguments[i]->toString() : "null";
            if (i + 1 < arguments.size()) argsStr += ", ";
        }
        argsStr += ")";
        return "Call: " + base + argsStr;
    } else if (isSetter()) {
        return "Set: " + base + " = " + (assignmentValue ? assignmentValue->toString() : "null");
    } else {
        return "Get: " + base;
    }
}

std::string MemberAccess::formatError(const std::string& msg) const {
    return "Error in '" + toString() + "'.\n" + msg;
}

// Private helper method implementations
void MemberAccess::validateAccessChain(SymbolTableType scope) {
    if (accessContext.empty()) return;
    
    std::string prefix = accessContext[0];
    for (size_t i = 0; i + 1 < accessContext.size(); ++i) {
        const std::string& baseName = accessContext[i];
        const std::string& memberName = accessContext[i + 1];

        DEBUG_LOG("Validating access for '" + memberName + "' in '" + baseName + "'.");
        validateAccessiblity(baseName, memberName, scope);
    }
}

std::tuple<std::shared_ptr<Expression>, std::string, std::string> 
MemberAccess::resolveBaseExpression(SymbolTableType scope) {
    DEBUG_LOG("The object is " + (object ? object->toString() : "null"));
    
    if (!object) {
        return resolveVariableBase(scope, objectName);
    }
    
    if (auto getter = std::dynamic_pointer_cast<GetVariable>(object)) {
        return resolveVariableBase(scope, getter->getName());
    }
    
    if (auto memberAcc = std::dynamic_pointer_cast<MemberAccess>(object)) {
        return resolveChainedAccess(scope, memberAcc);
    }
    
    // General expression case
    auto baseExpr = object->express(scope);
    if (!baseExpr) {
        console.reportError(Console::SEMANTIC_ERROR, "Failed to evaluate base expression for member access", FileSpan{});
        return {nullptr, "", ""};
    }
    
    auto baseTypeName = extractTypeName(baseExpr->getType());
    return {baseExpr, baseTypeName, ""};
}

std::tuple<std::shared_ptr<Expression>, std::string, std::string>
MemberAccess::resolveVariableBase(SymbolTableType scope, const std::string& varName) {
    auto [expr, resolvedName] = findVariableInScope(scope, varName);
    if (!expr) {
        console.reportError(Console::SEMANTIC_ERROR, "Variable '" + varName + "' not found in current or contextual scope", FileSpan{});
        return {nullptr, "", ""};
    }
    
    auto baseTypeName = extractTypeName(expr->getType());
    auto baseExpr = std::make_shared<VariableAccessExpression>(resolvedName, expr->getType());
    
    DEBUG_LOG("Resolved variable '" + varName + "' as type '" + baseTypeName + "'");
    return {baseExpr, baseTypeName, resolvedName};
}

std::tuple<std::shared_ptr<Expression>, std::string, std::string>
MemberAccess::resolveChainedAccess(SymbolTableType scope, std::shared_ptr<MemberAccess> memberAcc) {
    auto baseExpr = memberAcc->express(scope);
    if (!baseExpr) {
        console.reportError(Console::SEMANTIC_ERROR, "Failed to evaluate chained member access base expression", FileSpan{});
        return {nullptr, "", ""};
    }
    
    auto baseTypeName = extractTypeName(baseExpr->getType());
    auto resolvedName = memberAcc->getName();
    
    DEBUG_LOG("Chained access resolved to type '" + baseTypeName + "'");
    return {baseExpr, baseTypeName, resolvedName};
}

std::pair<std::shared_ptr<Expression>, std::string>
MemberAccess::findVariableInScope(SymbolTableType scope, const std::string& varName) {
    // Try direct lookup first
    if (auto expr = scope->get(varName)) {
        return {expr, varName};
    }
    
    // Try contextual lookups
    std::string qualifiedName;
    for (const auto& context : accessContext) {
        if (!qualifiedName.empty()) qualifiedName += ".";
        qualifiedName += context;
        
        std::string fullName = qualifiedName + "." + varName;
        if (auto expr = scope->get(fullName)) {
            DEBUG_LOG("[MemberAccess] Found contextual name: " + fullName);
            return {expr, fullName};
        }
    }
    
    return {nullptr, ""};
}

std::string MemberAccess::extractTypeName(std::shared_ptr<Type> type) {
    auto pointee = type->getBasePointeeType();
    
    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        DEBUG_LOG("Direct type: " + udt->toString());
        return udt->name;
    }
    
    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(pointee)) {
        DEBUG_LOG("Pointee type: " + udt->toString());
        return udt->name;
    }
    
    if (pointee) {
        DEBUG_LOG("Fallback pointee: " + pointee->kindName());
        return pointee->getName();
    }
    
    return "<null>";
}

std::shared_ptr<UserDefinedType> 
MemberAccess::getUserDefinedType(SymbolTableType scope, const std::string& typeName) {
    auto userType = std::dynamic_pointer_cast<UserDefinedType>(scope->getType(typeName));
    if (!userType) {
        if (auto type = scope->getType(typeName)) {
            DEBUG_LOG("Found type but not UDT: " + type->toString());
        } else {
            DEBUG_LOG("No type definition found for: " + typeName);
        }
        console.reportError(Console::TYPE_ERROR, "Type '" + typeName + "' is not a user-defined type.", FileSpan{});
    }
    return userType;
}

int MemberAccess::findMemberIndex(std::shared_ptr<UserDefinedType> userType) {
    for (int i = 0; i < userType->paramTypes.size(); ++i) {
        if (userType->paramTypes[i]->getParameterName() == memberName) {
            setType(userType->paramTypes[i]);
            return i;
        }
    }
    
    std::string available;
    for (size_t i = 0; i < userType->paramTypes.size(); ++i) {
        available += "[" + std::to_string(i) + "] '" +
                     userType->paramTypes[i]->getParameterName() + "' ";
    }
    console.reportError(Console::TYPE_ERROR, "Type '" + userType->name + "' has no member '" + memberName +
                  "'. Members: " + available, FileSpan{});
    return -1;
}

std::shared_ptr<Expression> 
MemberAccess::processAssignment(SymbolTableType scope) {
    if (!assignmentValue) {
        return nullptr;
    }
    
    extendContextOf(assignmentValue);
    
    if (auto constructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(assignmentValue)) {
        constructor->setInstanceName(getName());
    }
    
    auto assignmentExpr = assignmentValue->express(scope);
    if (!assignmentExpr) {
        console.reportError(Console::SEMANTIC_ERROR, "Failed to evaluate assignment expression", FileSpan{});
    }
    
    return assignmentExpr;
}

} // namespace Omniscript
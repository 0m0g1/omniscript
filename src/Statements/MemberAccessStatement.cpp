#include <omniscript/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Expressions/ClassExpression.h>
#include <omniscript/Expressions/StructExpression.h>
#include <omniscript/Expressions/AccessExpressions.h>
#include <omniscript/Expressions/AggregateExpressions.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Symboltable.h>

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
        console.error("The object having members should be named");
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
    return cloned;
}

std::shared_ptr<Statement> MemberAccess::evaluate(SymbolTableType scope) {
    return nullptr; // Evaluation logic to be filled
}

std::shared_ptr<Omniscript::Expression> MemberAccess::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    
    // Validate access context chain
    validateAccessChain(scope);
    
    // Get base expression and type info
    auto [baseExpr, baseTypeName, resolvedObjectName] = resolveBaseExpression(scope);
    if (!baseExpr) {
        return nullptr;
    }
    
    // Check if this is a method call (has arguments or isCall is true)
    if (isCall || !arguments.empty()) {
        DEBUG_LOG("[MemberAccess] Detected method call: " + baseTypeName + "." + memberName);
        
        // Look for method with mangled name: TypeName.methodName
        std::string methodName = baseTypeName + "." + memberName;
        auto method = scope->get(methodName);
        
        if (method) {
            DEBUG_LOG("[MemberAccess] Found method: " + methodName);
            // Return a special expression that indicates this should be handled as a method call
            // We'll create a CallExpression with the base expression as the 'this' argument
            
            // Create arguments list with 'this' as first argument
            std::vector<std::shared_ptr<Omniscript::Expression>> callArgs;
            callArgs.push_back(baseExpr);
            
            // Add other arguments if any
            for (const auto& arg : arguments) {
                if (auto argExpr = arg->express(scope)) {
                    callArgs.push_back(argExpr);
                }
            }
            
            // Get method type for return type
            auto methodType = method->getType();
            auto returnType = methodType->isFunction() ? methodType->getReturnType() : methodType;
            
            auto callExpr = std::make_shared<Omniscript::CallExpression>(methodName, callArgs, returnType);
            callExpr->setPosition(getPosition());
            return callExpr;
        } else {
            // Check for overloaded methods
            auto overloads = scope->getOverloads(methodName);
            if (!overloads.empty()) {
                DEBUG_LOG("[MemberAccess] Found overloaded methods for: " + methodName);
                // For now, return the first overload - proper overload resolution should happen in Call
                auto firstOverload = overloads[0];
                
                std::vector<std::shared_ptr<Omniscript::Expression>> callArgs;
                callArgs.push_back(baseExpr);
                
                for (const auto& arg : arguments) {
                    if (auto argExpr = arg->express(scope)) {
                        callArgs.push_back(argExpr);
                    }
                }
                
                auto methodType = firstOverload->getType();
                auto returnType = methodType->isFunction() ? methodType->getReturnType() : methodType;
                
                auto callExpr = std::make_shared<Omniscript::CallExpression>(methodName, callArgs, returnType);
                callExpr->setPosition(getPosition());
                return callExpr;
            }
            
            console.error("Method '" + methodName + "' not found in scope");
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
    auto result = std::make_shared<Omniscript::MemberAccessExpression>(
        baseExpr, baseTypeName, resolvedObjectName, memberName, 
        memberIndex, type, assignmentExpr
    );
    result->type = type;
    result->setPosition(getPosition());
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
    for (size_t i = 0; i < accessContext.size() - 1; ++i) {
        const std::string& baseName = accessContext[i];
        const std::string& memberName = accessContext[i + 1];
        
        DEBUG_LOG("Validating access for '" + memberName + "' in '" + baseName + "'.");
        validateAccessiblity(baseName, memberName, scope);
        prefix += "." + memberName;
    }
}

std::tuple<std::shared_ptr<Omniscript::Expression>, std::string, std::string> 
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
        console.error("Failed to evaluate base expression for member access");
        return {nullptr, "", ""};
    }
    
    auto baseTypeName = extractTypeName(baseExpr->getType());
    return {baseExpr, baseTypeName, ""};
}

std::tuple<std::shared_ptr<Omniscript::Expression>, std::string, std::string>
MemberAccess::resolveVariableBase(SymbolTableType scope, const std::string& varName) {
    auto [expr, resolvedName] = findVariableInScope(scope, varName);
    if (!expr) {
        console.error("Variable '" + varName + "' not found in current or contextual scope");
        return {nullptr, "", ""};
    }
    
    auto baseTypeName = extractTypeName(expr->getType());
    auto baseExpr = std::make_shared<Omniscript::VariableAccessExpression>(resolvedName, expr->getType());
    
    DEBUG_LOG("Resolved variable '" + varName + "' as type '" + baseTypeName + "'");
    return {baseExpr, baseTypeName, resolvedName};
}

std::tuple<std::shared_ptr<Omniscript::Expression>, std::string, std::string>
MemberAccess::resolveChainedAccess(SymbolTableType scope, std::shared_ptr<MemberAccess> memberAcc) {
    auto baseExpr = memberAcc->express(scope);
    if (!baseExpr) {
        console.error("Failed to evaluate chained member access base expression");
        return {nullptr, "", ""};
    }
    
    auto baseTypeName = extractTypeName(baseExpr->getType());
    auto resolvedName = memberAcc->getName();
    
    DEBUG_LOG("Chained access resolved to type '" + baseTypeName + "'");
    return {baseExpr, baseTypeName, resolvedName};
}

std::pair<std::shared_ptr<Omniscript::Expression>, std::string>
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

std::string MemberAccess::extractTypeName(std::shared_ptr<Omniscript::Type> type) {
    auto pointee = type->getBasePointeeType();
    
    if (auto udt = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(type)) {
        DEBUG_LOG("Direct type: " + udt->toString());
        return udt->name;
    }
    
    if (auto udt = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(pointee)) {
        DEBUG_LOG("Pointee type: " + udt->toString());
        return udt->name;
    }
    
    if (pointee) {
        DEBUG_LOG("Fallback pointee: " + pointee->kindName());
        return pointee->getName();
    }
    
    return "<null>";
}

std::shared_ptr<Omniscript::UserDefinedType> 
MemberAccess::getUserDefinedType(SymbolTableType scope, const std::string& typeName) {
    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(scope->getType(typeName));
    if (!userType) {
        if (auto type = scope->getType(typeName)) {
            DEBUG_LOG("Found type but not UDT: " + type->toString());
        } else {
            DEBUG_LOG("No type definition found for: " + typeName);
        }
        console.error("Type '" + typeName + "' is not a user-defined type.");
    }
    return userType;
}

int MemberAccess::findMemberIndex(std::shared_ptr<Omniscript::UserDefinedType> userType) {
    for (int i = 0; i < userType->paramTypes.size(); ++i) {
        if (userType->paramTypes[i]->getParameterName() == memberName) {
            setType(userType->paramTypes[i]);
            return i;
        }
    }
    
    console.error("Member '" + memberName + "' not found in user-defined type parameters");
    return -1;
}

std::shared_ptr<Omniscript::Expression> 
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
        console.error("Failed to evaluate assignment expression");
    }
    
    return assignmentExpr;
}
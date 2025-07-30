#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Expressions/ClassExpression.h>
#include <omniscript/Expressions/StructExpression.h>
#include <omniscript/Expressions/AccessExpressions.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Symboltable.h>

namespace Omniscript {

ArrowAccess::ArrowAccess(std::shared_ptr<Statement> ptr, const std::string& member) 
    : pointer(ptr) {
    memberName = member;
    expr = ptr;
}

std::shared_ptr<Expression> ArrowAccess::resolvePointerExpression(SymbolTableType scope) {
    if (auto arrowAcc = std::dynamic_pointer_cast<ArrowAccess>(pointer)) {
        DEBUG_LOG("Resolving chained arrow access: " + arrowAcc->toString());
        auto baseExpr = arrowAcc->express(scope);
        if (!baseExpr) {
            std::string suggestion = "To resolve chained access:\n"
                                   "1. Check each link in the chain is valid\n"
                                   "2. Verify intermediate pointers are initialized\n"
                                   "3. Add debug output for each access step";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate chained arrow access expression",
                suggestion,
                arrowAcc->getSpan()
            );
            return nullptr;
        }
        return baseExpr;
    }
    
    if (auto memberAcc = std::dynamic_pointer_cast<MemberAccess>(pointer)) {
        DEBUG_LOG("Resolving member access as pointer base: " + memberAcc->toString());
        auto baseExpr = memberAcc->express(scope);
        if (!baseExpr) {
            std::string suggestion = "To resolve member access:\n"
                                   "1. Verify the base object exists\n"
                                   "2. Check member accessibility\n"
                                   "3. Ensure proper initialization";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate member access base expression",
                suggestion,
                memberAcc->getSpan()
            );
            return nullptr;
        }
        return baseExpr;
    }
    
    auto pointerExpr = pointer->express(scope);
    if (!pointerExpr) {
        std::string suggestion = "To resolve pointer expression:\n"
                               "1. Check variable is declared\n"
                               "2. Verify proper initialization\n"
                               "3. Add debug output before access";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to evaluate pointer expression for arrow access",
            suggestion,
            pointer->getSpan()
        );
        return nullptr;
    }
    
    DEBUG_LOG("Resolved pointer expression: " + pointerExpr->getType()->toString());
    return pointerExpr;
}

std::shared_ptr<UserDefinedType> ArrowAccess::validateAndGetPointeeType(
    std::shared_ptr<Type> pointerType) {
    
    if (!pointerType->isPointer()) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Declare as pointer: `%s* var`\n"
            "2. Use address-of operator: `&variable`\n"
            "3. Check variable declaration",
            pointerType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Arrow access requires pointer type (got '%s')", 
                               pointerType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    
    auto baseType = pointerType->getBasePointeeType();
    if (!baseType) {
        console.reportError(
            Console::INTERNAL_ERROR,
            "Failed to determine pointee type",
            "This is likely a compiler bug - please report it",
            getSpan()
        );
        return nullptr;
    }
    
    auto userType = std::dynamic_pointer_cast<UserDefinedType>(baseType);
    if (!userType) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check type definition is complete\n"
            "2. For built-in types, use dot notation\n"
            "3. Verify proper type imports",
            baseType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Arrow access requires user-defined type (got '%s')", 
                               baseType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    
    DEBUG_LOG("Validated pointee type: " + userType->toString());
    return userType;
}

int ArrowAccess::findMemberInType(std::shared_ptr<UserDefinedType> userType) {
    for (int i = 0; i < userType->paramTypes.size(); ++i) {
        if (userType->paramTypes[i]->getParameterName() == memberName) {
            auto memberType = userType->paramTypes[i];
            
            if (type && !Type::isSameOrCastableTo(memberType, type)) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Check type requirements\n"
                    "2. Available conversions:\n"
                    " - Explicit cast: `(%s)value`\n"
                    " - Conversion method: `value.to_%s()`\n"
                    "3. Verify source type implements required traits",
                    memberType->toString().c_str(),
                    memberType->toString().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Type mismatch: cannot use '%s' as '%s'", 
                                       memberType->toString().c_str(),
                                       type->toString().c_str()),
                    suggestion,
                    getSpan()
                );
                return -1;
            }
            
            setType(memberType);
            DEBUG_LOG("Found member '" + memberName + "' at index " + std::to_string(i));
            return i;
        }
    }
    
    std::string availableMembers;
    for (const auto& param : userType->paramTypes) {
        availableMembers += " - " + param->getParameterName() + "\n";
    }
    
    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Check member name spelling\n"
        "2. Available members:\n%s\n"
        "3. Verify inheritance chain",
        availableMembers.c_str()
    );
    
    console.reportError(
        Console::TYPE_ERROR,
        Console::formatString("Member '%s' not found in '%s'", 
                           memberName.c_str(),
                           userType->toString().c_str()),
        suggestion,
        getSpan()
    );
    return -1;
}

std::shared_ptr<Expression> ArrowAccess::processAssignment(SymbolTableType scope) {
    if (!assignmentValue) {
        return nullptr;
    }
    
    extendContextOf(assignmentValue);
    
    if (auto constructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(assignmentValue)) {
        constructor->setInstanceName(getName());
    }
    
    auto assignmentExpr = assignmentValue->express(scope);
    if (!assignmentExpr) {
        std::string suggestion = "To resolve assignment:\n"
                               "1. Check right-hand expression validity\n"
                               "2. Verify type compatibility\n"
                               "3. Add debug output for the value";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Invalid assignment value in arrow access",
            suggestion,
            assignmentValue->getSpan()
        );
    }
    
    if (assignmentExpr && type && !Type::isSameOrCastableTo(type, assignmentExpr->getType())) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check type requirements\n"
            "2. Available conversions:\n"
            " - Explicit cast: `(%s)value`\n"
            " - Conversion method: `value.to_%s()`\n"
            "3. Verify source type implements required traits",
            type->toString().c_str(),
            type->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Cannot assign '%s' to '%s'", 
                               assignmentExpr->getType()->toString().c_str(),
                               type->toString().c_str()),
            suggestion,
            assignmentValue->getSpan()
        );
        return nullptr;
    }
    
    DEBUG_LOG("Processed assignment: " + (assignmentExpr ? assignmentExpr->toString() : "null"));
    return assignmentExpr;
}

std::shared_ptr<Statement> ArrowAccess::clone() const {
    auto cloned = std::make_shared<ArrowAccess>(pointer->clone(), memberName);
    cloned->assignmentValue = assignmentValue ? assignmentValue->clone() : nullptr;
    
    cloned->arguments.reserve(arguments.size());
    for (const auto& arg : arguments) {
        cloned->arguments.push_back(arg->clone());
    }
    cloned->isCall = isCall;
    return cloned;
}

std::shared_ptr<Expression> ArrowAccess::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    
    // Resolve the pointer expression (handles chaining)
    auto pointerExpr = resolvePointerExpression(scope);
    if (!pointerExpr) {
        return nullptr;
    }
    
    // Validate pointer type and get pointee
    auto userType = validateAndGetPointeeType(pointerExpr->getType());
    if (!userType) {
        return nullptr;
    }
    
    // Find the member in the type
    int memberIndex = findMemberInType(userType);
    if (memberIndex == -1) {
        return nullptr;
    }
    
    // Handle assignment if present
    auto assignmentExpr = processAssignment(scope);
    if (assignmentValue && !assignmentExpr) {
        return nullptr;
    }
    
    // Create the arrow access expression
    auto result = std::make_shared<ArrowAccessExpression>(
        pointerExpr, memberName, memberIndex, type, assignmentExpr
    );
    
    result->type = type;
    result->setSpan(this->getSpan());
    return result;
}

std::string ArrowAccess::toString() const {
    std::string base = "(" + (pointer ? pointer->toString() : "null") + "->" + memberName + ")";
    
    if (isSetter()) {
        return base + " = " + (assignmentValue ? assignmentValue->toString() : "null");
    } else if (isCall) {
        std::string argsStr = "(";
        for (size_t i = 0; i < arguments.size(); ++i) {
            argsStr += arguments[i] ? arguments[i]->toString() : "null";
            if (i + 1 < arguments.size()) argsStr += ", ";
        }
        argsStr += ")";
        return "Call: " + base + argsStr;
    } else {
        return "Get: " + base;
    }
}

std::string ArrowAccess::formatError(const std::string& msg) const {
    return "Error in '" + toString() + "'.\n" + msg;
}

} // namespace Omniscript

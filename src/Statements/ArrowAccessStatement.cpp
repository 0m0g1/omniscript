#include <omniscript/Statement.h>
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

ArrowAccess::ArrowAccess(std::shared_ptr<Statement> ptr, const std::string& member) 
    : pointer(ptr) {
    memberName = member;
    expr = ptr;
}

// Public methods
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

std::shared_ptr<Omniscript::Expression> ArrowAccess::express(SymbolTableType scope) {
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    
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
    auto result = std::make_shared<Omniscript::ArrowAccessExpression>(
        pointerExpr, memberName, memberIndex, type, assignmentExpr
    );
    
    result->type = type;
    result->setSpan(getSpan());
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

// Private helper method implementations
std::shared_ptr<Omniscript::Expression> ArrowAccess::resolvePointerExpression(SymbolTableType scope) {
    // Handle chained arrow access (e.g., this->position->x)
    if (auto arrowAcc = std::dynamic_pointer_cast<ArrowAccess>(pointer)) {
        DEBUG_LOG("Resolving chained arrow access: " + arrowAcc->toString());
        auto baseExpr = arrowAcc->express(scope);
        if (!baseExpr) {
            console.error("Failed to evaluate chained arrow access base expression");
            return nullptr;
        }
        return baseExpr;
    }
    
    // Handle member access as pointer base (e.g., obj.ptr->member)
    if (auto memberAcc = std::dynamic_pointer_cast<MemberAccess>(pointer)) {
        DEBUG_LOG("Resolving member access as pointer base: " + memberAcc->toString());
        auto baseExpr = memberAcc->express(scope);
        if (!baseExpr) {
            console.error("Failed to evaluate member access base expression");
            return nullptr;
        }
        return baseExpr;
    }
    
    // General case - evaluate the pointer expression
    auto pointerExpr = pointer->express(scope);
    if (!pointerExpr) {
        console.error("Failed to evaluate pointer expression for arrow access");
        return nullptr;
    }
    
    DEBUG_LOG("Resolved pointer expression: " + pointerExpr->getType()->toString());
    return pointerExpr;
}

std::shared_ptr<Omniscript::UserDefinedType> ArrowAccess::validateAndGetPointeeType(
    std::shared_ptr<Omniscript::Type> pointerType) {
    
    if (!pointerType->isPointer()) {
        console.error("Arrow access requires a pointer type, got: " + pointerType->toString());
        return nullptr;
    }
    
    auto baseType = pointerType->getBasePointeeType();
    if (!baseType) {
        console.error("Failed to get pointee type from pointer");
        return nullptr;
    }
    
    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(baseType);
    if (!userType) {
        console.error("Arrow access requires a pointer to user-defined type, got pointer to: " + 
                     baseType->toString());
        return nullptr;
    }
    
    DEBUG_LOG("Validated pointee type: " + userType->toString());
    return userType;
}

int ArrowAccess::findMemberInType(std::shared_ptr<Omniscript::UserDefinedType> userType) {
    for (int i = 0; i < userType->paramTypes.size(); ++i) {
        if (userType->paramTypes[i]->getParameterName() == memberName) {
            auto memberType = userType->paramTypes[i];
            
            // Handle type compatibility checking
            if (type) {
                if (Omniscript::Type::isSameOrCastableTo(memberType, type)) {
                    setType(type);
                } else {
                    console.error("Cannot cast " + memberType->toString() + 
                                " to " + type->toString());
                    return -1;
                }
            } else {
                setType(memberType);
            }
            
            DEBUG_LOG("Found member '" + memberName + "' at index " + std::to_string(i) + 
                     " with type: " + memberType->toString());
            return i;
        }
    }
    
    console.error("Member '" + memberName + "' not found in type '" + userType->toString() + "'");
    return -1;
}

std::shared_ptr<Omniscript::Expression> ArrowAccess::processAssignment(SymbolTableType scope) {
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
    
    DEBUG_LOG("Processed assignment: " + (assignmentExpr ? assignmentExpr->toString() : "null"));
    return assignmentExpr;
}
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

IndexAccess::IndexAccess(std::shared_ptr<Statement> expr, std::shared_ptr<Statement> index) 
    : index(index) {
    this->expr = expr;
}

// Public methods
std::shared_ptr<Statement> IndexAccess::clone() const {
    auto cloned = std::make_shared<IndexAccess>(expr->clone(), index->clone());
    cloned->assignmentValue = assignmentValue ? assignmentValue->clone() : nullptr;
    
    cloned->arguments.reserve(arguments.size());
    for (const auto& arg : arguments) {
        cloned->arguments.push_back(arg->clone());
    }
    cloned->isCall = isCall;
    return cloned;
}

std::shared_ptr<Omniscript::Expression> IndexAccess::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    
    // Resolve container expression (handles chaining)
    auto containerExpr = resolveContainerExpression(scope);
    if (!containerExpr) {
        return nullptr;
    }
    
    // Resolve index expression
    auto indexExprValue = resolveIndexExpression(scope);
    if (!indexExprValue) {
        return nullptr;
    }
    
    // Validate container type and get element type
    auto elementType = validateAndGetElementType(containerExpr->getType());
    if (!elementType) {
        return nullptr;
    }
    
    setType(elementType);
    
    // Handle assignment if present
    auto assignmentExpr = processAssignment(scope);
    if (assignmentValue && !assignmentExpr) {
        return nullptr;
    }
    
    // Create the index access expression
    auto result = std::make_shared<Omniscript::IndexAccessExpression>(
        containerExpr, indexExprValue, type, assignmentExpr
    );
    
    result->type = type;
    result->setPosition(getPosition());
    return result;
}

std::string IndexAccess::toString() const {
    std::string base = "(" + (expr ? expr->toString() : "null") + 
                      "[" + (index ? index->toString() : "null") + "])";
    
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

std::string IndexAccess::formatError(const std::string& msg) const {
    return "Error in '" + toString() + "'.\n" + msg;
}

// Private helper method implementations
std::shared_ptr<Omniscript::Expression> IndexAccess::resolveContainerExpression(SymbolTableType scope) {
    // Handle chained index access (e.g., arr[0][1][2])
    if (auto indexAcc = std::dynamic_pointer_cast<IndexAccess>(expr)) {
        DEBUG_LOG("Resolving chained index access: " + indexAcc->toString());
        auto baseExpr = indexAcc->express(scope);
        if (!baseExpr) {
            console.error("Failed to evaluate chained index access base expression");
            return nullptr;
        }
        return baseExpr;
    }
    
    // Handle member access as container (e.g., obj.array[index])
    if (auto memberAcc = std::dynamic_pointer_cast<MemberAccess>(expr)) {
        DEBUG_LOG("Resolving member access as container: " + memberAcc->toString());
        auto baseExpr = memberAcc->express(scope);
        if (!baseExpr) {
            console.error("Failed to evaluate member access container expression");
            return nullptr;
        }
        return baseExpr;
    }
    
    // Handle arrow access as container (e.g., ptr->array[index])
    if (auto arrowAcc = std::dynamic_pointer_cast<ArrowAccess>(expr)) {
        DEBUG_LOG("Resolving arrow access as container: " + arrowAcc->toString());
        auto baseExpr = arrowAcc->express(scope);
        if (!baseExpr) {
            console.error("Failed to evaluate arrow access container expression");
            return nullptr;
        }
        return baseExpr;
    }
    
    // General case - evaluate the container expression
    auto containerExpr = expr->express(scope);
    if (!containerExpr) {
        console.error("Failed to evaluate container expression for index access");
        return nullptr;
    }
    
    DEBUG_LOG("Resolved container expression: " + containerExpr->getType()->toString());
    return containerExpr;
}

std::shared_ptr<Omniscript::Expression> IndexAccess::resolveIndexExpression(SymbolTableType scope) {
    // Handle complex index expressions (e.g., arr[obj.index], arr[func()])
    if (auto memberAcc = std::dynamic_pointer_cast<MemberAccess>(index)) {
        DEBUG_LOG("Resolving member access as index: " + memberAcc->toString());
        auto indexExpr = memberAcc->express(scope);
        if (!indexExpr) {
            console.error("Failed to evaluate member access index expression");
            return nullptr;
        }
        return indexExpr;
    }
    
    if (auto arrowAcc = std::dynamic_pointer_cast<ArrowAccess>(index)) {
        DEBUG_LOG("Resolving arrow access as index: " + arrowAcc->toString());
        auto indexExpr = arrowAcc->express(scope);
        if (!indexExpr) {
            console.error("Failed to evaluate arrow access index expression");
            return nullptr;
        }
        return indexExpr;
    }
    
    // General case - evaluate the index expression
    auto indexExprValue = index->express(scope);
    if (!indexExprValue) {
        console.error("Failed to evaluate index expression");
        return nullptr;
    }
    
    // Validate that index is numeric type
    auto indexType = indexExprValue->getType();
    // Todo:: Add an is numeric type flag
    if (!indexType->isInteger() && !indexType->isFloat()) {
        console.error("Index must be a numeric type, got: " + indexType->toString());
        return nullptr;
    }
    
    DEBUG_LOG("Resolved index expression: " + indexType->toString());
    return indexExprValue;
}

std::shared_ptr<Omniscript::Type> IndexAccess::validateAndGetElementType(
    std::shared_ptr<Omniscript::Type> containerType) {
    
    if (containerType->isArray()) {
        auto elementType = containerType->elementType;
        if (!elementType) {
            console.error("Array type missing element type information");
            return nullptr;
        }
        DEBUG_LOG("Array element type: " + elementType->toString());
        return elementType;
    }
    
    if (containerType->isPointer()) {
        auto pointeeType = containerType->getBasePointeeType();
        if (!pointeeType) {
            console.error("Pointer type missing pointee type information");
            return nullptr;
        }
        DEBUG_LOG("Pointer pointee type: " + pointeeType->toString());
        return pointeeType;
    }
    
    console.error("Index access requires an array or pointer type, got: " + 
                 containerType->toString());
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> IndexAccess::processAssignment(SymbolTableType scope) {
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
        return nullptr;
    }
    
    // Validate assignment type compatibility
    if (type && assignmentExpr->getType()) {
        if (!Omniscript::Type::isSameOrCastableTo(assignmentExpr->getType(), type)) {
            console.error("Cannot assign " + assignmentExpr->getType()->toString() + 
                         " to " + type->toString());
            return nullptr;
        }
    }
    
    DEBUG_LOG("Processed assignment: " + assignmentExpr->toString());
    return assignmentExpr;
}
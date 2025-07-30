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

std::shared_ptr<Expression> IndexAccess::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    
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
    auto result = std::make_shared<IndexAccessExpression>(
        containerExpr, indexExprValue, type, assignmentExpr
    );
    
    result->type = type;
    result->setSpan(this->getSpan());
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
std::shared_ptr<Expression> IndexAccess::resolveContainerExpression(SymbolTableType scope) {
    // Handle chained index access (e.g., arr[0][1][2])
    if (auto indexAcc = std::dynamic_pointer_cast<IndexAccess>(expr)) {
        DEBUG_LOG("Resolving chained index access: " + indexAcc->toString());
        auto baseExpr = indexAcc->express(scope);
        if (!baseExpr) {
            std::string suggestion = "To resolve chained index access:\n"
                                   "1. Check each index in the chain is valid\n"
                                   "2. Verify intermediate arrays are initialized\n"
                                   "3. Add debug output for each access step";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate chained index access base expression",
                suggestion,
                indexAcc->getSpan()
            );
            return nullptr;
        }
        return baseExpr;
    }
    
    // Handle member access as container (e.g., obj.array[index])
    if (auto memberAcc = std::dynamic_pointer_cast<MemberAccess>(expr)) {
        DEBUG_LOG("Resolving member access as container: " + memberAcc->toString());
        auto baseExpr = memberAcc->express(scope);
        if (!baseExpr) {
            std::string suggestion = "To resolve member access:\n"
                                   "1. Verify the base object exists\n"
                                   "2. Check member accessibility\n"
                                   "3. Ensure proper initialization";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate member access container expression",
                suggestion,
                memberAcc->getSpan()
            );
            return nullptr;
        }
        return baseExpr;
    }
    
    // Handle arrow access as container (e.g., ptr->array[index])
    if (auto arrowAcc = std::dynamic_pointer_cast<ArrowAccess>(expr)) {
        DEBUG_LOG("Resolving arrow access as container: " + arrowAcc->toString());
        auto baseExpr = arrowAcc->express(scope);
        if (!baseExpr) {
            std::string suggestion = "To resolve arrow access:\n"
                                   "1. Verify the pointer is initialized\n"
                                   "2. Check pointer type compatibility\n"
                                   "3. Add debug output before access";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate arrow access container expression",
                suggestion,
                arrowAcc->getSpan()
            );
            return nullptr;
        }
        return baseExpr;
    }
    
    // General case - evaluate the container expression
    auto containerExpr = expr->express(scope);
    if (!containerExpr) {
        std::string suggestion = "To resolve container expression:\n"
                               "1. Check variable is declared\n"
                               "2. Verify proper initialization\n"
                               "3. Add debug output before access";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to evaluate container expression for index access",
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }
    
    DEBUG_LOG("Resolved container expression: " + containerExpr->getType()->toString());
    return containerExpr;
}

std::shared_ptr<Expression> IndexAccess::resolveIndexExpression(SymbolTableType scope) {
    // Handle complex index expressions (e.g., arr[obj.index], arr[func()])
    if (auto memberAcc = std::dynamic_pointer_cast<MemberAccess>(index)) {
        DEBUG_LOG("Resolving member access as index: " + memberAcc->toString());
        auto indexExpr = memberAcc->express(scope);
        if (!indexExpr) {
            std::string suggestion = "To resolve member access index:\n"
                                   "1. Verify the base object exists\n"
                                   "2. Check member accessibility\n"
                                   "3. Ensure proper initialization";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate member access index expression",
                suggestion,
                memberAcc->getSpan()
            );
            return nullptr;
        }
        return indexExpr;
    }
    
    if (auto arrowAcc = std::dynamic_pointer_cast<ArrowAccess>(index)) {
        DEBUG_LOG("Resolving arrow access as index: " + arrowAcc->toString());
        auto indexExpr = arrowAcc->express(scope);
        if (!indexExpr) {
            std::string suggestion = "To resolve arrow access index:\n"
                                   "1. Verify the pointer is initialized\n"
                                   "2. Check pointer type compatibility\n"
                                   "3. Add debug output before access";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate arrow access index expression",
                suggestion,
                arrowAcc->getSpan()
            );
            return nullptr;
        }
        return indexExpr;
    }
    
    // General case - evaluate the index expression
    auto indexExprValue = index->express(scope);
    if (!indexExprValue) {
        std::string suggestion = "To resolve index expression:\n"
                               "1. Check index expression is valid\n"
                               "2. Verify proper initialization\n"
                               "3. Add debug output for index value";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to evaluate index expression",
            suggestion,
            index->getSpan()
        );
        return nullptr;
    }
    
    // Validate that index is numeric type
    auto indexType = indexExprValue->getType();
    if (!indexType->isInteger() && !indexType->isFloat()) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use a numeric type for indexing\n"
            "2. Available conversions:\n"
            " - Explicit cast: `(%s)value`\n"
            " - Ensure index is an integer or float\n"
            "3. Verify index expression type",
            indexType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Index must be a numeric type, got: '%s'",
                             indexType->toString().c_str()),
            suggestion,
            index->getSpan()
        );
        return nullptr;
    }
    
    DEBUG_LOG("Resolved index expression: " + indexType->toString());
    return indexExprValue;
}

std::shared_ptr<Type> IndexAccess::validateAndGetElementType(
    std::shared_ptr<Type> containerType) {
    
    if (containerType->isArray()) {
        auto elementType = containerType->elementType;
        if (!elementType) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure array type is properly defined\n"
                                   "2. Check array declaration for element type\n"
                                   "3. Verify type imports";
            console.reportError(
                Console::TYPE_ERROR,
                "Array type missing element type information",
                suggestion,
                getSpan()
            );
            return nullptr;
        }
        DEBUG_LOG("Array element type: " + elementType->toString());
        return elementType;
    }
    
    if (containerType->isPointer()) {
        auto pointeeType = containerType->getBasePointeeType();
        if (!pointeeType) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure pointer type is properly defined\n"
                                   "2. Check pointer declaration for pointee type\n"
                                   "3. Verify type imports";
            console.reportError(
                Console::TYPE_ERROR,
                "Pointer type missing pointee type information",
                suggestion,
                getSpan()
            );
            return nullptr;
        }
        DEBUG_LOG("Pointer pointee type: " + pointeeType->toString());
        return pointeeType;
    }
    
    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Declare as array or pointer: `%s`\n"
        "2. Check container type declaration\n"
        "3. Verify type compatibility",
        containerType->toString().c_str()
    );
    console.reportError(
        Console::TYPE_ERROR,
        Console::formatString("Index access requires an array or pointer type, got: '%s'",
                         containerType->toString().c_str()),
        suggestion,
        getSpan()
    );
    return nullptr;
}

std::shared_ptr<Expression> IndexAccess::processAssignment(SymbolTableType scope) {
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
            "Failed to evaluate assignment expression",
            suggestion,
            assignmentValue->getSpan()
        );
        return nullptr;
    }
    
    // Validate assignment type compatibility
    if (type && assignmentExpr->getType()) {
        if (!Type::isSameOrCastableTo(assignmentExpr->getType(), type)) {
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
    }
    
    DEBUG_LOG("Processed assignment: " + assignmentExpr->toString());
    return assignmentExpr;
}

} // namespace Omniscript 

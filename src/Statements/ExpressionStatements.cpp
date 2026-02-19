#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Types/BaseType.h>
#include <omniscript/Types/Types.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Expressions/VariableAccessExpression.h>
#include <omniscript/Expressions/Expressions.h>

namespace Omniscript {
    
std::shared_ptr<Expression> ASTTernaryExpression::express(SymbolTableType scope) {
    setSpan(getSpan());
    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(truthy)) {
        stmt->setType(type);
    }

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(falsey)) {
        stmt->setType(type);
    }

    extendContextOf(condition);
    extendContextOf(truthy);
    extendContextOf(falsey);

    // Evaluate condition, then branches
    std::shared_ptr<Expression> condValue = condition->express(scope);
    if (!condValue) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check condition expression validity\n"
            "2. Verify condition evaluates to boolean\n"
            "3. Add debug output for condition\n"
            "4. Condition expression: %s",
            condition->toString().c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            "Ternary expression has invalid condition",
            suggestion,
            condition->getSpan()
        );
    }

    std::shared_ptr<Expression> trueValue = truthy->express(scope);
    if (!trueValue) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check truthy branch expression validity\n"
            "2. Verify type compatibility\n"
            "3. Add debug output for truthy value\n"
            "4. Truthy expression: %s",
            truthy->toString().c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            "Ternary expression has invalid true branch",
            suggestion,
            truthy->getSpan()
        );
    }

    std::shared_ptr<Expression> falseValue = falsey->express(scope);
    if (!falseValue) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check falsey branch expression validity\n"
            "2. Verify type compatibility\n"
            "3. Add debug output for falsey value\n"
            "4. Falsey expression: %s",
            falsey->toString().c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            "Ternary expression has invalid false branch",
            suggestion,
            falsey->getSpan()
        );
    }

    if (Type::isSameOrCastableTo(trueValue->getType(), falseValue->getType())) {
        type = falseValue->getType();
    } else if (Type::isSameOrCastableTo(falseValue->getType(), trueValue->getType())) {
        type = trueValue->getType();
    }

    if (type) {
        if (!Type::isSameOrCastableTo(trueValue->getType(), type)) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Check type requirements for truthy branch\n"
                "2. Available conversions:\n"
                " - Explicit cast: `(%s)value`\n"
                " - Conversion method: `value.to_%s()`\n"
                "3. Verify source type implements required traits\n"
                "4. Expected type: %s\n"
                "5. Actual type: %s",
                type->toString().c_str(),
                type->toString().c_str(),
                type->toString().c_str(),
                trueValue->getType()->toString().c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                Console::formatString("Type mismatch in ternary expression: cannot use truthy value of type '%s' as '%s'",
                                   trueValue->getType()->toString().c_str(),
                                   type->toString().c_str()),
                suggestion,
                truthy->getSpan()
            );
        } else if (!Type::isSameOrCastableTo(falseValue->getType(), type)) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Check type requirements for falsey branch\n"
                "2. Available conversions:\n"
                " - Explicit cast: `(%s)value`\n"
                " - Conversion method: `value.to_%s()`\n"
                "3. Verify source type implements required traits\n"
                "4. Expected type: %s\n"
                "5. Actual type: %s",
                type->toString().c_str(),
                type->toString().c_str(),
                type->toString().c_str(),
                falseValue->getType()->toString().c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                Console::formatString("Type mismatch in ternary expression: cannot use falsey value of type '%s' as '%s'",
                                   falseValue->getType()->toString().c_str(),
                                   type->toString().c_str()),
                suggestion,
                falsey->getSpan()
            );
        }
    }

    auto ternaryExpr = std::make_shared<TernaryExpression>(
        condValue, trueValue, falseValue, type
    );
    ternaryExpr->setSpan(this->getSpan());
    return ternaryExpr;
}

std::shared_ptr<Expression> ASTBinaryExpression::express(SymbolTableType scope) {
    setSpan(getSpan());
    DEBUG_LOG("Evaluating binary expression: " + toString());

    DEBUG_LOG("Left expression: " + (left ? left->toString() : "null"));
    DEBUG_LOG("The operation is '" + op.getValue() + "'.");
    DEBUG_LOG("Right expression: " + (right ? right->toString() : "null"));
    if (type) {
        DEBUG_LOG("The binary expression's type is '" + type->toString() + "'.");
    }

    extendContextOf(left);
    extendContextOf(right);

    std::shared_ptr<Type> leftType = nullptr;
    std::shared_ptr<Type> rightType = nullptr;

    auto leftTyped = std::dynamic_pointer_cast<TypedStatement>(left);
    auto rightTyped = std::dynamic_pointer_cast<TypedStatement>(right);

    if (auto leftLiteral = std::dynamic_pointer_cast<Literal>(left)) {
        if (type) {
            leftType = type;
        } else {
            leftType = leftLiteral->getRootType();
        }
    } else if (leftTyped) {
        leftType = leftTyped->getType();
    } else {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check left operand declaration\n"
            "2. Add explicit type annotation if needed\n"
            "3. Verify expression validity\n"
            "4. Left operand: %s",
            left->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Left operand '%s' has no type or is not a typed statement",
                               left->toString().c_str()),
            suggestion,
            left->getSpan()
        );
    }

    if (auto rightLiteral = std::dynamic_pointer_cast<Literal>(right)) {
        if (type) {
            rightType = type;
        } else {
            rightType = rightLiteral->getRootType();
        }
    } else if (rightTyped) {
        rightType = rightTyped->getType();
    } else {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check right operand declaration\n"
            "2. Add explicit type annotation if needed\n"
            "3. Verify expression validity\n"
            "4. Right operand: %s",
            right->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Right operand '%s' has no type or is not a typed statement",
                               right->toString().c_str()),
            suggestion,
            right->getSpan()
        );
        return left->express(scope);
    }

    std::shared_ptr<Expression> leftResult;
    std::shared_ptr<Expression> rightResult;
    if (!leftType || leftType->isInvalid()) {
        auto tempScope = scope->createChildScope("temp");
        auto leftClone = left->clone();
        extendContextOf(leftClone);
        leftResult = leftClone->express(tempScope);
        leftType = leftResult->getType();
        DEBUG_LOG("The evaluated left operand type is of type '" + leftType->toString() + "'.");
    }
    
    if (!rightType || rightType->isInvalid()) {
        auto tempScope = scope->createChildScope("temp");
        auto rightClone = right->clone();
        extendContextOf(rightClone);
        rightResult = rightClone->express(tempScope);
        rightType = rightResult->getType();
        DEBUG_LOG("The evaluated right operand type is of type '" + rightType->toString() + "'.");
    }

    if (!leftType || leftType->isInvalid()) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check left operand type declaration\n"
            "2. Verify expression validity\n"
            "3. Add debug output for left value\n"
            "4. Left operand: %s",
            left->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Left operand '%s' has an invalid type",
                               left->toString().c_str()),
            suggestion,
            left->getSpan()
        );
    }
    
    if (!rightType || rightType->isInvalid()) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check right operand type declaration\n"
            "2. Verify expression validity\n"
            "3. Add debug output for right value\n"
            "4. Right operand: %s",
            right->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Right operand '%s' has an invalid type",
                               right->toString().c_str()),
            suggestion,
            right->getSpan()
        );
    }

    if (type) {
        if (!op.isComparisonOperator()) {
            if (Type::isSameOrCastableTo(leftType, type)) {
                if (auto leftLiteral = std::dynamic_pointer_cast<Literal>(left)) {
                    leftType = type;
                    if (leftTyped) leftTyped->setType(type);
                }
            } else {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Check left operand type requirements\n"
                    "2. Available conversions:\n"
                    " - Explicit cast: `(%s)value`\n"
                    " - Conversion method: `value.to_%s()`\n"
                    "3. Verify source type implements required traits\n"
                    "4. Left operand type: %s\n"
                    "5. Expected type: %s",
                    type->toString().c_str(),
                    type->toString().c_str(),
                    leftType->toString().c_str(),
                    type->toString().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Left operand '%s' of type '%s' is not compatible with binary expression type '%s'",
                                       left->toString().c_str(),
                                       leftType->toString().c_str(),
                                       type->toString().c_str()),
                    suggestion,
                    left->getSpan()
                );
            }
    
            if (Type::isSameOrCastableTo(rightType, type)) {
                if (auto rightLiteral = std::dynamic_pointer_cast<Literal>(right)) {
                    rightType = type;
                    if (rightTyped) rightTyped->setType(type);
                }
            } else {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Check right operand type requirements\n"
                    "2. Available conversions:\n"
                    " - Explicit cast: `(%s)value`\n"
                    " - Conversion method: `value.to_%s()`\n"
                    "3. Verify source type implements required traits\n"
                    "4. Right operand type: %s\n"
                    "5. Expected type: %s",
                    type->toString().c_str(),
                    type->toString().c_str(),
                    rightType->toString().c_str(),
                    type->toString().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Right operand '%s' of type '%s' is not compatible with binary expression type '%s'",
                                       right->toString().c_str(),
                                       rightType->toString().c_str(),
                                       type->toString().c_str()),
                    suggestion,
                    right->getSpan()
                );
            }
        }
    }

    if (!type) {
        if (op.isComparisonOperator()) {
            type = resolveType({ "bool" });
            if (Type::isSameOrCastableTo(rightType, leftType)) {
                if (leftType->isPointer()) {
                    if (rightTyped) rightTyped->setType(leftType->getPointeeType());
                } else {
                    if (rightTyped) rightTyped->setType(leftType);
                }
            } else if (Type::isSameOrCastableTo(leftType, rightType)) {
                if (leftType->isPointer()) {
                    if (leftTyped) leftTyped->setType(rightType->getPointeeType());
                } else {
                    if (leftTyped) leftTyped->setType(rightType);
                }
            } else {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Check comparison operand types\n"
                    "2. Add explicit casts if needed\n"
                    "3. Verify type compatibility\n"
                    "4. Left type: %s\n"
                    "5. Right type: %s",
                    leftType->toString().c_str(),
                    rightType->toString().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Incompatible comparison types: '%s' vs '%s'",
                                       leftType->toString().c_str(),
                                       rightType->toString().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        } else if (op.isArithmeticOperator() || op.isBitwiseOperator()) {
            if (Type::isSameOrCastableTo(leftType, rightType)) {
                type = rightType;
                if (!Type::isSame(leftType, rightType)) {
                    left = std::make_shared<ASTCast>(left, rightType);
                } else {
                    if (leftTyped) leftTyped->setType(rightType);
                }
            } else if (Type::isSameOrCastableTo(rightType, leftType)) {
                type = leftType;
                if (!Type::isSame(rightType, leftType)) {
                    right = std::make_shared<ASTCast>(right, leftType);
                } else {
                    if (rightTyped) rightTyped->setType(leftType);
                }
            } else {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Check arithmetic/bitwise operand types\n"
                    "2. Add explicit casts if needed\n"
                    "3. Verify type compatibility\n"
                    "4. Left type: %s\n"
                    "5. Right type: %s",
                    leftType->toString().c_str(),
                    rightType->toString().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Incompatible arithmetic/bitwise types: '%s' vs '%s'",
                                       leftType->toString().c_str(),
                                       rightType->toString().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        } else if (op.isLogicalOperator()) {
            type = resolveType({ "bool" });
        } else if (op.isAssignmentOperator()) {
            type = leftType;
        } else {
            DEBUG_LOG("Unhandled binary operator type: " + getTokenTypeName(op.getType()));
            return nullptr;
        }

        if (!type || type->isInvalid()) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Check operand types\n"
                "2. Verify operator is supported for these types\n"
                "3. Left type: %s\n"
                "4. Right type: %s\n"
                "5. Operator: %s",
                leftType ? leftType->toString().c_str() : "unknown",
                rightType ? rightType->toString().c_str() : "unknown",
                op.getValue().c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                "Failed to infer binary expression type",
                suggestion,
                getSpan()
            );
            return nullptr;
        }

        DEBUG_LOG("Inferred binary expression type as: " + type->toString());
    }

    std::shared_ptr<Expression> leftValue = left->express(scope);
    std::shared_ptr<Expression> rightValue = right->express(scope);
    
    if (!leftValue) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check left operand expression\n"
            "2. Verify variable is initialized\n"
            "3. Add debug output for left value\n"
            "4. Left expression: %s",
            left->toString().c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            "Left operand evaluated to null",
            suggestion,
            left->getSpan()
        );
    }
    
    if (!rightValue) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check right operand expression\n"
            "2. Verify variable is initialized\n"
            "3. Add debug output for right value\n"
            "4. Right expression: %s",
            right->toString().c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            "Right operand evaluated to null",
            suggestion,
            right->getSpan()
        );
    }
    
    if (rightType->isNull() && leftType->isNullable()) {
        if (auto varAccess = std::dynamic_pointer_cast<VariableAccessExpression>(leftValue)) {
            varAccess->extractValue = false;
        }
    }

    if (leftType->isNull() && rightType->isNullable()) {
        if (auto varAccess = std::dynamic_pointer_cast<VariableAccessExpression>(rightValue)) {
            varAccess->extractValue = false;
        }
    }

    DEBUG_LOG("The left value is: " + leftValue->toString());
    DEBUG_LOG("The right value is: " + rightValue->toString());

    auto binaryExpr = std::make_shared<BinaryExpression>(leftValue, op, rightValue, type);
    binaryExpr->setSpan(this->getSpan());
    return binaryExpr;
}

bool ASTBinaryExpression::hasSideEffects() {
    return !isCompileTimeEvaluatable();
}

bool ASTBinaryExpression::isCompileTimeEvaluatable() {
    if (left->isCompileTimeEvaluatable() && right->isCompileTimeEvaluatable()) {
        return true;
    }
    return false;
}

std::shared_ptr<Expression> ASTUnaryExpression::express(SymbolTableType scope) {
    setSpan(getSpan());
    if (operand) {
        DEBUG_LOG("Creating a unary expression " + operand->toString());
    } else {
        std::string suggestion = "To resolve this:\n"
                               "1. Check unary operator usage\n"
                               "2. Verify operand is provided\n"
                               "3. Operator: " + op.getValue();
        console.reportError(
            Console::SYNTAX_ERROR,
            "Unary operator has no operand",
            suggestion,
            getSpan()
        );
    }

    extendContextOf(operand);

    std::shared_ptr<Expression> operandValue;

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(operand)) {
        if (type) {
            stmt->setType(type);
            operandValue = operand->express(scope);
        } else {
            operandValue = operand->express(scope);
            setType(stmt->getType());
        }
    }

    if (!operandValue) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Check operand expression validity\n"
            "2. Verify variable is initialized\n"
            "3. Add debug output for operand\n"
            "4. Operand: %s",
            operand ? operand->toString().c_str() : "null"
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to evaluate unary operand",
            suggestion,
            operand ? operand->getSpan() : getSpan()
        );
    }

    bool isPrefix = position == Position::Prefix;

    auto unaryExpr = std::make_shared<UnaryExpression>(op, operandValue, type, isPrefix);
    unaryExpr->setSpan(this->getSpan());
    return unaryExpr;
}

} // namespace Omniscript

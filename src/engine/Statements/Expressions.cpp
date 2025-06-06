#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>

// ============================== Binary, Unary and Ternary Expressions ============================== //

std::shared_ptr<Omniscript::Expression> TernaryExpression::express(SymbolTableType scope) {
    // Assign types
    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(truthy)) {
        stmt->setType(type);
    }

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(falsey)) {
        stmt->setType(type);
    }

    // if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(condition)) {
    //     std::vector<std::string> typeStr = {"bool"};
    //     stmt->setType(Omniscript::resolveType(typeStr));
    // }

    extendContextOf(condition);
    extendContextOf(truthy);
    extendContextOf(falsey);

    // Evaluate condition, then branches
    std::shared_ptr<Omniscript::Expression> condValue = condition->express(scope);
    if (!condValue) return nullptr;

    std::shared_ptr<Omniscript::Expression> trueValue = truthy->express(scope);
    if (!trueValue) return nullptr;

    std::shared_ptr<Omniscript::Expression> falseValue = falsey->express(scope);
    if (!falseValue) return nullptr;

    return std::make_shared<Omniscript::TernaryExpression>(
        condValue, trueValue, falseValue, type
    );
}

std::shared_ptr<Omniscript::Expression> BinaryExpression::express(SymbolTableType scope) {
    DEBUG_LOG();

    DEBUG_LOG("Left expression: " + (left ? left->toString() : "null"));
    DEBUG_LOG("Right expression: " + (right ? right->toString() : "null"));
    if (type) {
        DEBUG_LOG("The binary expression's type is '" + type->toString() + "'.");
    }

    extendContextOf(left);
    extendContextOf(right);

    std::shared_ptr<Omniscript::Type> leftType = nullptr;
    std::shared_ptr<Omniscript::Type> rightType = nullptr;

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
        console.error("The left operand '" + left->toString() + "' has no type or is not a typed statement");
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
        console.error("The left operand '" + right->toString() + "' has no type or is not a typed statement");
    }

    if (!leftType || leftType->isInvalid()) {
        auto tempScope = scope->createChildScope("temp");
        auto leftClone = left->clone();
        extendContextOf(leftClone);
        leftType = leftClone->express(tempScope)->getType();
        DEBUG_LOG("The evaluated left operand type is of type '" + leftType->toString() + "'.");
    }
    
    if (!rightType || rightType->isInvalid()) {
        auto tempScope = scope->createChildScope("temp");
        auto rightClone = right->clone();
        extendContextOf(rightClone);
        rightType = rightClone->express(tempScope)->getType();
        DEBUG_LOG("The evaluated right operand type is of type '" + rightType->toString() + "'.");
    }

    if (!leftType || leftType->isInvalid()) {
        console.error("The left operand '" + left->toString() + "' has an invalid type.");
    }
    
    if (!rightType || rightType->isInvalid()) {
        console.error("The right operand '" + right->toString() + "' has an invalid type.");
    }

    if (type) {
        if (Omniscript::isSameOrCastableTo(leftType, type)) {
            if (auto leftLiteral = std::dynamic_pointer_cast<Literal>(left)) {
                leftType = type;
                if (leftTyped) leftTyped->setType(type);
            }
        } else {
            console.error("The left operand '" + left->toString() + "' of type '" + leftType->toString() + "' is not compatible with the binary expression's type '" + type->toString() + "'.");
        }

        if (Omniscript::isSameOrCastableTo(rightType, type)) {
            if (auto rightLiteral = std::dynamic_pointer_cast<Literal>(right)) {
                rightType = type;
                if (rightTyped) rightTyped->setType(type);
            }
        } else {
            console.error("The right operand '" + right->toString() + "' of type '" + rightType->toString() + "' is not compatible with the binary expression's type '" + type->toString() + "'.");
        }
    }

    // Infer a common type if not already set
    if (!type) {
        if (op.isComparisonOperator()) {
            type = Omniscript::resolveType({ "bool" });
            if (Omniscript::isSameOrCastableTo(leftType, rightType)) {
                type = rightType;
                if (auto leftLiteral = std::dynamic_pointer_cast<Literal>(left)) {
                    if (leftTyped) leftTyped->setType(rightType);
                }
            } else if (Omniscript::isSameOrCastableTo(rightType, leftType)) {
                type = leftType;
                if (auto rightLiteral = std::dynamic_pointer_cast<Literal>(right)) {
                    if (rightTyped) rightTyped->setType(leftType);
                }
            } else {
                console.error("Incompatible comparison types: " + leftType->toString() + " vs " + rightType->toString());
                return nullptr;
            }
        } else if (op.isArithmeticOperator() || op.isBitwiseOperator()) {
            if (Omniscript::isSameOrCastableTo(leftType, rightType)) {
                type = rightType;
                if (auto leftLiteral = std::dynamic_pointer_cast<Literal>(left)) {
                    if (leftTyped) leftTyped->setType(rightType);
                }
            } else if (Omniscript::isSameOrCastableTo(rightType, leftType)) {
                type = leftType;
                if (auto rightLiteral = std::dynamic_pointer_cast<Literal>(right)) {
                    if (rightTyped) rightTyped->setType(leftType);
                }
            } else {
                console.error("Incompatible arithmetic/bitwise types: " + leftType->toString() + " vs " + rightType->toString());
                return nullptr;
            }
        } else if (op.isLogicalOperator()) {
            type = Omniscript::resolveType({ "bool" });
        } else if (op.isAssignmentOperator()) {
            type = leftType;
        } else {
            DEBUG_LOG("Unhandled binary operator type: " + getTokenTypeName(op.getType()));
            return nullptr;
        }

        if (!type || type->isInvalid()) {
            console.error("Failed to infer binary expression type.");
            return nullptr;
        }

        DEBUG_LOG("Inferred binary expression type as: " + type->toString());
    }

    
    // if (rightTyped) rightTyped->setType(type);

    std::shared_ptr<Omniscript::Expression> leftValue = left->express(scope);
    std::shared_ptr<Omniscript::Expression> rightValue = right->express(scope);

    if (!leftValue) console.error("The left value is null");
    if (!rightValue) console.error("The right value is null");

    DEBUG_LOG("The left value is: " + leftValue->toString());
    DEBUG_LOG("The right value is: " + rightValue->toString());

    return std::make_shared<Omniscript::BinaryExpression>(leftValue, op.getType(), rightValue, type);
}

bool BinaryExpression::hasSideEffects() {
    return !isCompileTimeEvaluatable();
}

bool BinaryExpression::isCompileTimeEvaluatable() {
    if (left->isCompileTimeEvaluatable() && right->isCompileTimeEvaluatable()) {
        return true;
    }
    return false;
}

std::shared_ptr<Omniscript::Expression> UnaryExpression::express(SymbolTableType scope) {
    if (operand) {
        DEBUG_LOG("Creating a unary expression " + operand->toString());
    } else {
        console.error("There is no operand");
    }

    extendContextOf(operand);
    // Set the expected type on the operand
    std::shared_ptr<Omniscript::Expression> operandValue;

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(operand)) {
        if (type) {
            stmt->setType(type);
            operandValue = operand->express(scope);
        } else {
            operandValue = operand->express(scope);
            setType(stmt->getType());
        }
    }

    // Evaluate the operand
    if (!operandValue) {
        console.error("No operand value");
    }

    bool isPrefix = position == Position::Prefix;

    return std::make_shared<Omniscript::UnaryExpression>(op, operandValue, type, isPrefix);
}

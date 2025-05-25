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

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(condition)) {
        std::vector<std::string> typeStr = {"bool"};
        stmt->setType(Omniscript::resolveType(typeStr));
    }

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

    extendContextOf(left);
    extendContextOf(right);

    // Always evaluate expressions first
    std::shared_ptr<Omniscript::Expression> leftValue = left->express(scope);
    std::shared_ptr<Omniscript::Expression> rightValue = right->express(scope);

    if (!leftValue || !rightValue) return nullptr;

    auto leftTyped = std::dynamic_pointer_cast<TypedStatement>(left);
    auto rightTyped = std::dynamic_pointer_cast<TypedStatement>(right);

    if (!leftTyped || !rightTyped) {
        DEBUG_LOG("One or both expressions are not typed after evaluation.");
        return nullptr;
    }

    auto leftType = leftTyped->getType();
    auto rightType = rightTyped->getType();

    if (!leftType) {
        DEBUG_LOG("left is null");
    }
    
    if (!rightType) {
        DEBUG_LOG("right is null");
    }

    if (!leftType || !rightType) {
        DEBUG_LOG("One or both types are still null after evaluation.");
        return nullptr;
    }

    // Infer a common type if type hasn't been set
    if (!type) {
        if (op.isComparisonOperator()) {
            type = Omniscript::resolveType({ "bool" });
        } else if (op.isArithmeticOperator() || op.isBitwiseOperator()) {
            // Infer based on compatible numeric types
            if (Omniscript::isSameOrCastableTo(leftType, rightType)) {
                type = rightType;
            } else if (Omniscript::isSameOrCastableTo(rightType, leftType)) {
                type = leftType;
            } else {
                DEBUG_LOG("Incompatible arithmetic/bitwise types: " + leftType->description() + " vs " + rightType->description());
                return nullptr;
            }
        } else if (op.isLogicalOperator()) {
            type = Omniscript::resolveType({ "bool" });
        } else if (op.isAssignmentOperator()) {
            type = leftType; // Typically the type of the left-hand side
        } else {
            DEBUG_LOG("Unhandled binary operator type: " + getTokenTypeName(op.getType( )));
            return nullptr;
        }

        DEBUG_LOG("Inferred binary expression type as: " + type->description());
    }

    // Set the inferred type on both typed statements
    leftTyped->setType(type);
    rightTyped->setType(type);

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
    DEBUG_LOG("Creating a unary expression");

    extendContextOf(operand);
    // Set the expected type on the operand
    std::shared_ptr<Omniscript::Expression> operandValue;

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(operand)) {
        if (type) {
            stmt->setType(type);
            operand->express(scope);
        } else {
            operandValue = operand->express(scope);
            setType(stmt->getType());
        }
    }

    // Evaluate the operand
    if (!operandValue) return nullptr;

    bool isPrefix = position == Position::Prefix;

    return std::make_shared<Omniscript::UnaryExpression>(op, operandValue, type, isPrefix);
}

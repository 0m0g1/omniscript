#pragma once
#include <omniscript/Statement.h>

namespace Omniscript {

class UnaryExpression : public TypedStatement, public Expression {
public:
    enum class Position { Prefix, Postfix };

    UnaryExpression(Token op, std::shared_ptr<Statement> operand, Position pos = Position::Prefix)
        : op(op), operand(operand), position(pos) {

    }

    // Accessors
    Token getOperator() const { return op; }
    std::shared_ptr<Statement> getOperand() const { return operand; }
    Position getPosition() const { return position; }

    // Code generation method
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        std::string opStr = op.getValue();
        std::string operandStr = operand ? operand->toString() : "<null>";

        if (position == Position::Prefix) {
            return "(" + opStr + operandStr + ")";
        } else {
            return "(" + operandStr + opStr + ")";
        }
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in unary expression '" + toString() + "'.\n" + msg;
    };
    std::shared_ptr<Statement> clone() const override {
        std::shared_ptr<Statement> clonedOperand = operand ? operand->clone() : nullptr;
        return std::make_shared<UnaryExpression>(op, clonedOperand, position);
    }

private:
    Token op;
    std::shared_ptr<Statement> operand;
    Position position;  // For ++/-- to distinguish prefix/postfix
};

// Binary expression statement
class BinaryExpression : public TypedStatement, public Expression {
public:
    BinaryExpression(std::shared_ptr<Statement> left = std::shared_ptr<Statement>{}, Token op = Token(), std::shared_ptr<Statement> right = std::shared_ptr<Statement>{})
        : left(left), op(op), right(right) {}

    
    // Method to evaluate the binary expression
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        std::string leftStr = left ? left->toString() : "null";
        std::string rightStr = right ? right->toString() : "null";
        std::string opStr = getTokenTypeName(op.getType());
        return "(" + leftStr + " " + opStr + " " + rightStr + ")";
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in binary expression '" + toString() + "'.\n" + msg;
    };
    std::shared_ptr<Statement> clone() const override {
        // Clone left and right operands
        std::shared_ptr<Statement> clonedLeft = left ? left->clone() : nullptr;
        std::shared_ptr<Statement> clonedRight = right ? right->clone() : nullptr;

        // Return a new BinaryExpression with the cloned operands
        return std::make_shared<BinaryExpression>(clonedLeft, op, clonedRight);
    }

    bool hasSideEffects() override;
    bool isCompileTimeEvaluatable() override;

private:
    std::shared_ptr<Statement> left;
    Token op;
    std::shared_ptr<Statement> right;
};

class TernaryExpression : 
public Expression,
public TypedStatement {
    public:
        TernaryExpression(std::shared_ptr<Statement> condition, std::shared_ptr<Statement> truthy, std::shared_ptr<Statement> falsey) :
        condition(condition), truthy(truthy), falsey(falsey) {}

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        std::string condStr   = condition ? condition->toString() : "<null>";
        std::string truthyStr = truthy ? truthy->toString() : "<null>";
        std::string falseyStr = falsey ? falsey->toString() : "<null>";

        return "(" + condStr + " ? " + truthyStr + " : " + falseyStr + ")";
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in binary expression '" + toString() + "'.\n" + msg;
    };
    std::shared_ptr<Statement> clone() const override {
        std::shared_ptr<Statement> clonedCondition = condition ? condition->clone() : nullptr;
        std::shared_ptr<Statement> clonedTruthy = truthy ? truthy->clone() : nullptr;
        std::shared_ptr<Statement> clonedFalsey = falsey ? falsey->clone() : nullptr;
        return std::make_shared<TernaryExpression>(clonedCondition, clonedTruthy, clonedFalsey);
    }
    private:
        std::shared_ptr<Statement> condition;
        std::shared_ptr<Statement> truthy;
        std::shared_ptr<Statement> falsey;
};

}// namespace Omniscript

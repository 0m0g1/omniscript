#pragma once
#include <omniscript/engine/Statement.h>

class UnaryExpression : public TypedStatement, public Expression {
public:
    enum class Position { Prefix, Postfix };

    UnaryExpression(TokenTypes op, std::shared_ptr<Statement> operand, Position pos = Position::Prefix)
        : op(op), operand(operand), position(pos) {
        // Validate that this is a valid unary operator
        if (getOperatorString(op) == "?") {
            console.error("Invalid unary operator");
        }
    }

    // Get operator as a string (only for unary operators)
    static std::string getOperatorString(TokenTypes op) {
        switch (op) {
            case TokenTypes::Plus: return "+";
            case TokenTypes::Minus: return "-";
            case TokenTypes::LogicalNot: return "!";
            case TokenTypes::Tilde: return "~";
            case TokenTypes::Increment: return "++";
            case TokenTypes::Decrement: return "--";
            case TokenTypes::Multiply: return "*";
            case TokenTypes::BitwiseAnd: return "&";
            default: return "?";
        }
    }

    // Accessors
    TokenTypes getOperator() const { return op; }
    std::shared_ptr<Statement> getOperand() const { return operand; }
    Position getPosition() const { return position; }

    // Code generation method
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        std::string opStr = getOperatorString(op);
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
    TokenTypes op;
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
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
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
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
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

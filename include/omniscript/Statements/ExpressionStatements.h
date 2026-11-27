#pragma once
#include <omniscript/Statements/Statement.h>

namespace Omniscript {

// True when `t` is a user struct/class VALUE type (not a pointer) that defines `can <op>`, i.e. a method
// named '<Type>.operator<suffix>' (suffix: "+", "==", "[]", "[]=", "!", "unary-", "++", ...).
bool hasOperatorMethod(SymbolTableType scope, const std::shared_ptr<Type>& t, const std::string& suffix);

class ASTUnaryExpression : public TypedStatement, public ASTExpression {
public:
    enum class Position { Prefix, Postfix };

    ASTUnaryExpression(Token op, std::shared_ptr<Statement> operand, Position pos = Position::Prefix)
        : op(op), operand(operand), position(pos) {

    }

    // Accessors
    Token getOperator() const { return op; }
    std::shared_ptr<Statement> getOperand() const { return operand; }
    Position getPosition() const { return position; }

    // Operator overloading: `!a`, `~a`, `-a`, `+a`, `a++`, `++a`, `a--`, `--a` call a.operator<op>() when a's type
    // defines it. `handled` is set when the operator applies; the result may still be null if the call failed.
    std::shared_ptr<Expression> tryOperator(SymbolTableType scope, bool& handled);

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
        auto copy = std::make_shared<ASTUnaryExpression>(op, clonedOperand, position);
        copy->setSpan(getSpan());
        return copy;
    }

private:
    Token op;
    std::shared_ptr<Statement> operand;
    Position position;  // For ++/-- to distinguish prefix/postfix
};

// Binary expression statement
class ASTBinaryExpression : public TypedStatement, public ASTExpression {
    friend class GenericInstantiator;
public:
    ASTBinaryExpression(std::shared_ptr<Statement> left = std::shared_ptr<Statement>{}, Token op = Token(), std::shared_ptr<Statement> right = std::shared_ptr<Statement>{})
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

        // Return a new ASTBinaryExpression with the cloned operands
        auto copy = std::make_shared<ASTBinaryExpression>(clonedLeft, op, clonedRight);
        copy->fromCompound = fromCompound;
        copy->setSpan(getSpan());
        return copy;
    }

    bool hasSideEffects() override;
    bool isCompileTimeEvaluatable() override;

    // Set by the parser for `a += b` (and -=, *=, ...), which it lowers to `a = a <op> b`.
    // If a's type defines `can <op>=`, AssignVariable::express calls tryCompoundOperator() and skips the assignment.
    bool fromCompound = false;
    // Returns the call to a.operator<op>=(b), or nullptr when the left type has no such method.
    std::shared_ptr<Expression> tryCompoundOperator(SymbolTableType scope);

private:
    std::shared_ptr<Statement> left;
    Token op;
    std::shared_ptr<Statement> right;
};

// `a.x += b` / `a[i] += b` (the target is a member or element, not a plain variable).
// Parsed as the ordinary setter `a.x = a.x + b` (`fallback`); if the target's type defines `can +=`, the
// operator is called directly instead and the setter is dropped.
class CompoundAccessAssignment : public TypedStatement, public ASTExpression {
public:
    CompoundAccessAssignment(std::shared_ptr<ASTBinaryExpression> compound, std::shared_ptr<Statement> fallback)
        : compound(compound), fallback(fallback) {}

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "CompoundAccessAssignment: " + (fallback ? fallback->toString() : std::string("null")); }
    std::string formatError(const std::string& msg) const override {
        return "Error in compound assignment '" + toString() + "'.\n" + msg;
    }
    std::shared_ptr<Statement> clone() const override {
        auto c = std::dynamic_pointer_cast<ASTBinaryExpression>(compound ? compound->clone() : nullptr);
        auto copy = std::make_shared<CompoundAccessAssignment>(c, fallback ? fallback->clone() : nullptr);
        copy->setSpan(getSpan());
        return copy;
    }

private:
    std::shared_ptr<ASTBinaryExpression> compound;
    std::shared_ptr<Statement> fallback;
};

class ASTTernaryExpression : 
public ASTExpression,
public TypedStatement {
    friend class GenericInstantiator;
    public:
        ASTTernaryExpression(std::shared_ptr<Statement> condition, std::shared_ptr<Statement> truthy, std::shared_ptr<Statement> falsey) :
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
        auto copy = std::make_shared<ASTTernaryExpression>(clonedCondition, clonedTruthy, clonedFalsey);
        copy->setSpan(getSpan());
        return copy;
    }
    private:
        std::shared_ptr<Statement> condition;
        std::shared_ptr<Statement> truthy;
        std::shared_ptr<Statement> falsey;
};

}// namespace Omniscript
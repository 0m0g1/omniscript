#pragma once

#include <omniscript/engine/IRGenerator.h>
#include <omniscript/runtime/object.h>
#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/runtime/symboltable.h>

class Statement { // Base class for all statements
    public:
        // enum Type { // implement a statement type for each statement for speed
        //     Value,
        //     Assignment,
        //     ConstantAssignment,
        //     Return,

        // }
        // virtual std::unique_ptr<Statement> clone() const = 0; // clone method
        // virtual void execute(SymbolTable &scope) = 0; //Function to execute a statement
        virtual ~Statement() = default;
        // llvm::Value* codegen() { return nullptr; }

        inline void setPosition(int line, int column, const std::string& file) {
            pos.line = line;
            pos.col = column;
            pos.fileName = file;
        }

        inline void setPosition(Omniscript::filePosition position) {
            pos.line = position.line;
            pos.col = position.col;
            pos.fileName = position.fileName;
        }

        inline Omniscript::filePosition getPosition() {
            return pos;
        }

    protected:
        Omniscript::filePosition pos;

};

template <typename Derived>
class StatementCRTP : public Statement {
    public:
        llvm::Value* generateIR(IRGenerator& generator) {
            // return static_cast<Derived*>(this)->codegen(scope);
            return nullptr;
        }
};

class Literal : public StatementCRTP<Literal> {
public:
    virtual ~Literal() = default;
};

// ============================== Numeric Literals ============================== //

// Base class for numeric literals
template <typename T, typename V>
class NumericLiteral : public Literal {
public:
    using ValueType = V;
    ValueType value;

    explicit NumericLiteral(ValueType val) : value(val) {}

    llvm::Value* codegen(IRGenerator& generator) {
        return static_cast<T*>(this)->codegenImpl(generator);
    }
};

class Int8Bit : public NumericLiteral<Int8Bit, int8_t> {
public:
    using NumericLiteral::NumericLiteral;
    llvm::Value* codegenImpl(IRGenerator& generator);
};

class Int16Bit : public NumericLiteral<Int16Bit, int16_t> {
public:
    using NumericLiteral::NumericLiteral;
    llvm::Value* codegenImpl(IRGenerator& generator);
};

class Int32Bit : public NumericLiteral<Int32Bit, int32_t> {
public:
    using NumericLiteral::NumericLiteral;
    llvm::Value* codegenImpl(IRGenerator& generator);
};

class Int64Bit : public NumericLiteral<Int64Bit, int64_t> {
public:
    using NumericLiteral::NumericLiteral;
    llvm::Value* codegenImpl(IRGenerator& generator);
};

class Float32Bit : public NumericLiteral<Float32Bit, float> {
public:
    using NumericLiteral::NumericLiteral;
    llvm::Value* codegenImpl(IRGenerator& generator);
};

class Float64Bit : public NumericLiteral<Float64Bit, double> {
public:
    using NumericLiteral::NumericLiteral;
    llvm::Value* codegenImpl(IRGenerator& generator);
};

// Arbitrary-precision integer (BigInt)
class BigInt : public NumericLiteral<BigInt, std::string> {
public:
    using NumericLiteral::NumericLiteral;
    llvm::Value* codegenImpl(IRGenerator& generator);
};

// ============================== Other Literals ============================== //

class StringLiteral : public Literal {
public:
    std::string value;

    explicit StringLiteral(std::string val) : value(std::move(val)) {}
    llvm::Value* codegen(IRGenerator& generator);
};

class BoolLiteral : public Literal {
public:
    bool value;

    explicit BoolLiteral(bool val) : value(std::move(val)) {}
    llvm::Value* codegen(IRGenerator& generator);
};
        
    
// ============================== Assignments ============================== //
class Assignment : public StatementCRTP<Assignment> {
public:
    Assignment(const std::string &variable, std::shared_ptr<Statement> value) :
        variable(variable), value(value) {}

    llvm::Value* codegen(IRGenerator& generator);
    

private:
    std::string variable;
    std::shared_ptr<Statement> value;
};

class ConstantAssignment : public StatementCRTP<ConstantAssignment> {
public:
    ConstantAssignment(const std::string &variable, std::shared_ptr<Statement> value) :
        variable(variable), value(value) {}

    llvm::Value* codegen(IRGenerator& generator);

    

private:
    std::string variable;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Statement> tempValue;
};

class GenericAssignment : public StatementCRTP<GenericAssignment> {
public:
    GenericAssignment(const std::string &variable, std::shared_ptr<Statement> value) :
        variable(variable), value(value) {}

    llvm::Value* codegen(IRGenerator& generator);

    

private:
    std::string variable;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Statement> tempValue;
};


class Variable : public StatementCRTP<Variable> {
public:
    Variable(std::string variableName, std::shared_ptr<Statement> value = std::shared_ptr<Statement>{})
        : variable(variable), value(value) {}

        
    llvm::Value* codegen(IRGenerator& generator);
        
    std::shared_ptr<Statement> value;
    std::shared_ptr<Statement> variable;
private:
    std::shared_ptr<Statement> tempValue;
};


class ReturnStatement : public StatementCRTP<ReturnStatement> {
public:
    ReturnStatement(std::shared_ptr<Statement> value)
        : returnValue(value) {}
    llvm::Value* codegen(IRGenerator& generator);
    std::shared_ptr<Statement> returnValue; // Store the return value

    // std::vector<std::optional<std::shared_ptr<Statement>>> variableReturnValues;
    std::shared_ptr<Statement> variableReturnValue;
    
};


class FunctionCallStatement : public StatementCRTP<FunctionCallStatement> {
public:
    FunctionCallStatement(
            std::shared_ptr<Statement> func,
            std::vector<std::shared_ptr<Statement>> args,
            std::string baseName = "",
            std::string specializedName = "",
            std::vector<std::string> types = {}
            ) : 
            
            func(func), args(std::move(args)), types(types), baseName(baseName), specializedName(specializedName) {}

        llvm::Value* codegen(IRGenerator& generator);
    
    private:
        std::shared_ptr<Statement> func;
        std::vector<std::shared_ptr<Statement>> args;
        std::vector<std::string> types;
        std::string baseName;
        std::string specializedName;
};

class BlockStatement : public StatementCRTP<BlockStatement> {
public:
    BlockStatement(std::vector<std::shared_ptr<Statement>> statements = {})
    : statements(std::move(statements)) {}

    llvm::Value* codegen(IRGenerator& generator);

private:
    std::vector<std::shared_ptr<Statement>> statements;
};

class IfStatement : public StatementCRTP<IfStatement> {
public:
    IfStatement(std::shared_ptr<Statement> condition,
                std::vector<std::shared_ptr<Statement>> body = {},
                std::vector<std::shared_ptr<IfStatement>> branches = {}, 
                std::vector<std::shared_ptr<Statement>> falseBranch = {}) 
        : condition(condition), body(body), branches(branches), falseBranch(falseBranch) {}
    
    llvm::Value* codegen(IRGenerator& generator);
    
    std::shared_ptr<Statement> condition;
    std::vector<std::shared_ptr<Statement>> body;
    std::vector<std::shared_ptr<IfStatement>> branches;
    std::vector<std::shared_ptr<Statement>> falseBranch;


    bool conditionIsMet(SymbolTable &scope);

    
};


// Binary expression statement
class BinaryExpression : public StatementCRTP<BinaryExpression> {
public:
    BinaryExpression(std::shared_ptr<Statement> left = std::shared_ptr<Statement>{}, TokenTypes op = TokenTypes::Null, std::shared_ptr<Statement> right = std::shared_ptr<Statement>{})
        : left(left), op(op), right(right) {}

     // Helper function to get operator as a string
    static std::string getOperatorString(TokenTypes op) {
        switch (op) {
            case TokenTypes::Plus: return "+";
            case TokenTypes::Minus: return "-";
            case TokenTypes::Multiply: return "*";
            case TokenTypes::Divide: return "/";
            case TokenTypes::Modulo: return "%";
            case TokenTypes::Equals: return "==";
            case TokenTypes::NotEquals: return "!=";
            case TokenTypes::LessThan: return "<";
            case TokenTypes::GreaterThan: return ">";
            case TokenTypes::LessEqual: return "<=";
            case TokenTypes::GreaterEqual: return ">=";
            default: return "?";
        }
    }

    
    // Method to evaluate the binary expression
    llvm::Value* codegen(IRGenerator& generator);
    std::shared_ptr<Statement> evaluate(SymbolTable &scope);
    

private:
    std::shared_ptr<Statement> left;
    TokenTypes op;
    std::shared_ptr<Statement> right;

};

class UnaryExpression : public StatementCRTP<UnaryExpression> {
public:
    UnaryExpression(std::shared_ptr<Statement> left = std::shared_ptr<Statement>{}, TokenTypes op = TokenTypes::Null, std::shared_ptr<Statement> right = std::shared_ptr<Statement>{})
        : left(left), op(op), right(right) {}

     // Helper function to get operator as a string
    static std::string getOperatorString(TokenTypes op) {
        switch (op) {
            case TokenTypes::Plus: return "+";
            case TokenTypes::Minus: return "-";
            case TokenTypes::Multiply: return "*";
            case TokenTypes::Divide: return "/";
            case TokenTypes::Modulo: return "%";
            case TokenTypes::Equals: return "==";
            case TokenTypes::NotEquals: return "!=";
            case TokenTypes::LessThan: return "<";
            case TokenTypes::GreaterThan: return ">";
            case TokenTypes::LessEqual: return "<=";
            case TokenTypes::GreaterEqual: return ">=";
            default: return "?";
        }
    }

    
    // Method to evaluate the binary expression
    llvm::Value* codegen(IRGenerator& generator);
    std::shared_ptr<Statement> evaluate(SymbolTable &scope);
    


private:
    std::shared_ptr<Statement> left;
    TokenTypes op;
    std::shared_ptr<Statement> right;

};


class TenaryExpression : public StatementCRTP<TenaryExpression> {
    public:
        TenaryExpression(std::shared_ptr<Statement> condition, std::shared_ptr<Statement> truthy, std::shared_ptr<Statement> falsey) :
        condition(condition), truthy(truthy), falsey(falsey) {}

    llvm::Value* codegen(IRGenerator& generator);

    private:
        std::shared_ptr<Statement> condition;
        std::shared_ptr<Statement> truthy;
        std::shared_ptr<Statement> falsey;
};

// A while statement
class WhileStatement : public StatementCRTP<WhileStatement> {
public:
    WhileStatement(std::shared_ptr<Statement> condition, std::vector<std::shared_ptr<Statement>> body = {})
        : condition(condition), body(body) {}

    llvm::Value* codegen(IRGenerator& generator);
    std::shared_ptr<Statement> condition;
    std::vector<std::shared_ptr<Statement>> body;

private:
    // Helper function to evaluate the condition as a boolean
    bool evaluateCondition(SymbolTable &scope) {
        // auto result = Expression::evaluate(condition, scope);
        
        return false; // If the condition cannot be evaluated to a valid boolean, stop the loop
    }
};


// A class to call methods on objects
class CallMethod : public StatementCRTP<CallMethod> {
public:
    CallMethod(std::shared_ptr<Statement> object, const std::string& methodName, std::vector<std::shared_ptr<Statement>> args)
        : object(object), methodName(methodName), arguments(std::move(args)) {}
    
    
    llvm::Value* codegen(IRGenerator& generator);
    std::shared_ptr<Statement> evaluate(SymbolTable& scope);
private:
    std::shared_ptr<Statement> object; // The base object on which the method is called.
    std::string methodName; // The method name.
    std::vector<std::shared_ptr<Statement>> arguments; // The arguments to the method.

};

class GetProperty : public StatementCRTP<GetProperty> {
private:
    std::shared_ptr<Statement> object; // The base object on which the method is called.
    std::string propertyName; // The method name.
    std::vector<std::shared_ptr<Statement>> arguments; // The arguments to the method.

public:
    GetProperty(std::shared_ptr<Statement> object, const std::string& propertyName)
        : object(object), propertyName(propertyName) {}
    
    
    llvm::Value* codegen(IRGenerator& generator);
    std::shared_ptr<Statement> evaluate(SymbolTable& scope);
};


class ForLoop : public StatementCRTP<ForLoop> {
    std::shared_ptr<Statement> initialization;
    std::shared_ptr<Statement> condition;
    std::shared_ptr<Statement> increment;
    std::vector<std::shared_ptr<Statement>> body;

public:
    ForLoop(std::shared_ptr<Statement> init, std::shared_ptr<Statement> cond, 
            std::shared_ptr<Statement> incr, std::vector<std::shared_ptr<Statement>> body)
        :   initialization(std::move(init)), condition(std::move(cond)), 
            increment(std::move(incr)), body(std::move(body)) {}
    llvm::Value* codegen(IRGenerator& generator);
    std::shared_ptr<Statement> evaluate(SymbolTable& scope);
};

class BreakStatement : public StatementCRTP<BreakStatement> {
    public:
        llvm::Value* codegen(IRGenerator& generator);
};

class ContinueStatement : public StatementCRTP<ContinueStatement> {
public:
    llvm::Value* codegen(IRGenerator& generator);
};

class ObjectConstructorStatement : public StatementCRTP<ObjectConstructorStatement> {
private:
    std::shared_ptr<Object> obj;
    std::vector<std::shared_ptr<Statement>> constructorArgs;

public:
    ObjectConstructorStatement(std::shared_ptr<Object> obj,
                               std::vector<std::shared_ptr<Statement>> args = {})
        : obj(obj), constructorArgs(std::move(args)) {}

    llvm::Value* codegen(IRGenerator& generator);
    
};

class ObjectDestructorStatement : public StatementCRTP<ObjectDestructorStatement> {
private:
    std::string variableName; // The variable holding the object reference

public:
    explicit ObjectDestructorStatement(const std::string& variableName)
        : variableName(variableName) {}
        llvm::Value* codegen(IRGenerator& generator);
    
};
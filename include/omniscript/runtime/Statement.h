#pragma once

// #include <llvm/IR/Value.h>
// #include <cmath>
// #include <vector>
// #include <string>
// #include <iostream>
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
        virtual void execute(SymbolTable &scope) = 0; //Function to execute a statement
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

class Expression {
    public:
    // Helper function to extract values from statements
    static std::optional<SymbolTable::ValueType> evaluate(const SymbolTable::ValueType& statement, SymbolTable &scope);
};

class Value: public Statement {
    public:
        Value(SymbolTable::ValueType value = {})
            : value(value) {}

        SymbolTable::ValueType value; // Store a value

        void execute(SymbolTable &scope) override {}

        SymbolTable::ValueType evaluate(SymbolTable &scope);



};

class Assignment : public Statement {
public:
    Assignment(const std::string &variable, std::optional<SymbolTable::ValueType> value = SymbolTable::ValueType{}) :
        variable(variable), value(value) {}

    const std::string &getVariable() const { return variable; }
    void setValue(SymbolTable::ValueType newValue) {
        value = newValue;
    }
    SymbolTable::ValueType getValue() {
        auto currentValue = value;
        return currentValue.value(); 
    }
    std::shared_ptr<Assignment> clone() const {
        return std::make_shared<Assignment>(*this); // Copy constructor
    }
    void execute(SymbolTable &scope) override;

private:
    std::string variable;
    std::optional<SymbolTable::ValueType> value;
};

class ConstantAssignment : public Statement {
public:
    ConstantAssignment(const std::string &variable, std::optional<SymbolTable::ValueType> value) :
        variable(variable), value(value) {}

    const std::string &getVariable() const { return variable; }
    void setValue(SymbolTable::ValueType newValue) {
        value = newValue;
    }
    SymbolTable::ValueType getValue() {
        auto tempValue = value;
        return tempValue.value(); 
    }
    std::shared_ptr<ConstantAssignment> clone() const {
        return std::make_shared<ConstantAssignment>(*this); // Copy constructor
    }

    void execute(SymbolTable &scope) override;

private:
    std::string variable;
    std::optional<SymbolTable::ValueType> value;
    std::optional<SymbolTable::ValueType> tempValue;
};

class GenericAssignment : public Statement {
public:
    GenericAssignment(const std::string &variable, SymbolTable::ValueType value) :
        variable(variable), value(value) {}

    const std::string &getVariable() const { return variable; }
    void setValue(SymbolTable::ValueType newValue) {
        value = newValue;
    }
    SymbolTable::ValueType getValue() {
        auto tempValue = value;
        return tempValue.value(); 
    }
    std::shared_ptr<GenericAssignment> clone() const {
        return std::make_shared<GenericAssignment>(*this); // Copy constructor
    }

    void execute(SymbolTable &scope) override;

private:
    std::string variable;
    std::optional<SymbolTable::ValueType> value;
    std::optional<SymbolTable::ValueType> tempValue;
};


class Variable : public Statement {
public:
    Variable(SymbolTable::ValueType variable, SymbolTable::ValueType value = SymbolTable::ValueType{})
        : variable(variable), value(value) {}

    SymbolTable::ValueType value;
    
    SymbolTable::ValueType getValue() {
        auto currentValue = value;
        value = tempValue;
        // console.debug("The variable statement for '" + valueToString(variable) + "' was reset");
        return currentValue; 
    }

    void execute(SymbolTable &scope) override {};
    SymbolTable::ValueType evaluate(SymbolTable &scope);


    SymbolTable::ValueType variable;
private:
    SymbolTable::ValueType tempValue;
};


class ReturnStatement : public Statement {
public:
    ReturnStatement(std::optional<SymbolTable::ValueType> value = std::nullopt)
        : returnValue(value) {}

    std::optional<SymbolTable::ValueType> returnValue; // Store the return value

    // std::vector<std::optional<SymbolTable::ValueType>> variableReturnValues;
    std::optional<SymbolTable::ValueType> variableReturnValue = std::nullopt;

    void execute(SymbolTable &scope) override {};
    SymbolTable::ValueType evaluate(SymbolTable &scope);
};


class FunctionCallStatement : public Statement {
public:
    FunctionCallStatement(
            SymbolTable::ValueType func,
            std::vector<std::shared_ptr<Statement>> args,
            std::string baseName = "",
            std::string specializedName = "",
            std::vector<std::string> types = {}
            ) : 
            
            func(func), args(std::move(args)), types(types), baseName(baseName), specializedName(specializedName) {}

    // New method to get the return value

    void execute(SymbolTable &scope) override {}

    SymbolTable::ValueType evaluate(SymbolTable &scope);
    
    
    private:
    SymbolTable::ValueType func;
    std::vector<std::shared_ptr<Statement>> args;
    std::vector<std::string> types;
    std::string baseName;
    std::string specializedName;
};

class BlockStatement : public Statement {
public:
    BlockStatement(std::vector<std::shared_ptr<Statement>> statements = {})
    : statements(std::move(statements)) {}

    void execute(SymbolTable &scope) override;
    // SymbolTable::ValueType evaluate(SymbolTable &scope) override;

private:
    std::vector<std::shared_ptr<Statement>> statements;
};

class IfStatement : public Statement {
public:
    IfStatement(std::shared_ptr<Statement> condition,
                std::vector<std::shared_ptr<Statement>> body = {},
                std::vector<std::shared_ptr<IfStatement>> branches = {}, 
                std::vector<std::shared_ptr<Statement>> falseBranch = {}) 
        : condition(condition), body(body), branches(branches), falseBranch(falseBranch) {}

    std::shared_ptr<Statement> condition;
    std::vector<std::shared_ptr<Statement>> body;
    std::vector<std::shared_ptr<IfStatement>> branches;
    std::vector<std::shared_ptr<Statement>> falseBranch;


    bool conditionIsMet(SymbolTable &scope);

    void execute(SymbolTable &scope) override {
    }

    SymbolTable::ValueType evaluate(SymbolTable &scope);
};


// Binary expression statement
class BinaryExpression : public Statement {
public:
    BinaryExpression(SymbolTable::ValueType left = SymbolTable::ValueType{}, TokenTypes op = TokenTypes::Null, SymbolTable::ValueType right = SymbolTable::ValueType{})
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
    std::optional<SymbolTable::ValueType> evaluate(SymbolTable &scope);
    
    void execute(SymbolTable &scope) override {
    }

private:
    SymbolTable::ValueType left;
    TokenTypes op;
    SymbolTable::ValueType right;

};

class UnaryExpression : public Statement {
public:
    UnaryExpression(SymbolTable::ValueType left = SymbolTable::ValueType{}, TokenTypes op = TokenTypes::Null, SymbolTable::ValueType right = SymbolTable::ValueType{})
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
    std::optional<SymbolTable::ValueType> evaluate(SymbolTable &scope);
    
    void execute(SymbolTable &scope) override {
    }

private:
    SymbolTable::ValueType left;
    TokenTypes op;
    SymbolTable::ValueType right;

};


class TenaryExpression : public Statement {
    public:
        TenaryExpression(std::shared_ptr<Statement> condition, std::shared_ptr<Statement> truthy, std::shared_ptr<Statement> falsey) :
        condition(condition), truthy(truthy), falsey(falsey) {}

        void execute(SymbolTable &scope) override {};

        SymbolTable::ValueType evaluate(SymbolTable &scope);

    private:
        std::shared_ptr<Statement> condition;
        std::shared_ptr<Statement> truthy;
        std::shared_ptr<Statement> falsey;
};

// A while statement
class WhileStatement : public Statement {
public:
    WhileStatement(std::shared_ptr<Statement> condition, std::vector<std::shared_ptr<Statement>> body = {})
        : condition(condition), body(body) {}

    std::shared_ptr<Statement> condition;
    std::vector<std::shared_ptr<Statement>> body;

    void execute(SymbolTable &scope) override {
    }

    SymbolTable::ValueType evaluate(SymbolTable &scope);

private:
    // Helper function to evaluate the condition as a boolean
    bool evaluateCondition(SymbolTable &scope) {
        auto result = Expression::evaluate(condition, scope);
        
        return false; // If the condition cannot be evaluated to a valid boolean, stop the loop
    }
};


// A class to call methods on objects
class CallMethod : public Statement {
private:
    SymbolTable::ValueType object; // The base object on which the method is called.
    std::string methodName; // The method name.
    std::vector<std::shared_ptr<Statement>> arguments; // The arguments to the method.

public:
    CallMethod(SymbolTable::ValueType object, const std::string& methodName, std::vector<std::shared_ptr<Statement>> args)
        : object(object), methodName(methodName), arguments(std::move(args)) {}
    
    void execute(SymbolTable &scope) override {}

    SymbolTable::ValueType evaluate(SymbolTable& scope);
};

class GetProperty : public Statement {
private:
    SymbolTable::ValueType object; // The base object on which the method is called.
    std::string propertyName; // The method name.
    std::vector<std::shared_ptr<Statement>> arguments; // The arguments to the method.

public:
    GetProperty(SymbolTable::ValueType object, const std::string& propertyName)
        : object(object), propertyName(propertyName) {}
    
    void execute(SymbolTable &scope) override {}

    SymbolTable::ValueType evaluate(SymbolTable& scope);
};


class ForLoop : public Statement {
    std::shared_ptr<Statement> initialization;
    std::shared_ptr<Statement> condition;
    std::shared_ptr<Statement> increment;
    std::vector<std::shared_ptr<Statement>> body;

public:
    ForLoop(std::shared_ptr<Statement> init, std::shared_ptr<Statement> cond, 
            std::shared_ptr<Statement> incr, std::vector<std::shared_ptr<Statement>> body)
        :   initialization(std::move(init)), condition(std::move(cond)), 
            increment(std::move(incr)), body(std::move(body)) {}

    void execute(SymbolTable &scope) override {
    }

    SymbolTable::ValueType evaluate(SymbolTable& scope);
};

class BreakStatement : public Statement {
public:
    void execute(SymbolTable& scope) override {
        // The break statement just stops the loop, no value is returned
    }
};

class ContinueStatement : public Statement {
public:
    void execute(SymbolTable& scope) override {
        // The continue statement just skips to the next loop iteration
    }
};

class ObjectConstructorStatement : public Statement {
private:
    SymbolTable::ValueType obj;
    std::vector<std::shared_ptr<Statement>> constructorArgs;

public:
    ObjectConstructorStatement(SymbolTable::ValueType obj,
                               std::vector<std::shared_ptr<Statement>> args = {})
        : obj(obj), constructorArgs(std::move(args)) {}

    void execute(SymbolTable &scope) override {};
    SymbolTable::ValueType evaluate(SymbolTable &scope);
};

class ObjectDestructorStatement : public Statement {
private:
    std::string variableName; // The variable holding the object reference

public:
    explicit ObjectDestructorStatement(const std::string& variableName)
        : variableName(variableName) {}

    void execute(SymbolTable &scope) override;
};
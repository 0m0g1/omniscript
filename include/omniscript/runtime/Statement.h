#pragma once

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <omniscript/runtime/object.h>
#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/runtime/symboltable.h>

class IRGenerator;

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
        ~Statement() = default;

        virtual llvm::Value* codegen(IRGenerator& generator) {return nullptr; }

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

class NamedStatement: public Statement {
public:
    ~NamedStatement() = default;
    virtual std::string getName() = 0;
};

class ImportModule : public Statement {
public:
    std::string moduleName;
    std::string alias;
    std::string path;
    std::vector<std::string> importedMembers;

    ImportModule(const std::string& modName, const std::string& aliasName = "", 
                    const std::vector<std::string>& members = {}, const std::string& path = "")
        : moduleName(modName), alias(aliasName), importedMembers(members), path(path) {}

    llvm::Value* codegen(IRGenerator& irGen) override;
};
    
class CreateModule : public NamedStatement {
public:
    std::string moduleName;
    std::vector<std::shared_ptr<Statement>> statements;

    CreateModule(std::string name, std::vector<std::shared_ptr<Statement>> stmts)
    : moduleName(std::move(name)), statements(std::move(stmts)) {}
    
    std::string getName() override { return moduleName; }
    std::vector<std::shared_ptr<Statement>> getStatements() { return statements; }
    llvm::Value* codegen(IRGenerator& irGen) override;
};

class PublicMember : public NamedStatement {
public:
    std::string memberName;
    std::shared_ptr<Statement> value;

    PublicMember(std::string name, std::shared_ptr<Statement> value)
        : memberName(std::move(name)), value(std::move(value)) {}

    std::string getName() override { return memberName; }
    llvm::Value* codegen(IRGenerator& irGen) override;
};

class Literal : public Statement {
public:
    virtual ~Literal() = default;
};

// ============================== Numeric Literals ============================== //
class NumericLiteral : public Literal {
public:
    virtual ~NumericLiteral() = default;
};

class Int8Bit : public NumericLiteral {
public:
    explicit Int8Bit(int8_t val) : value(val) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    int8_t value;
};

class Int16Bit : public NumericLiteral {
public:
    explicit Int16Bit(int16_t val) : value(val) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    int16_t value;
};

class Int32Bit : public NumericLiteral {
public:
    explicit Int32Bit(int32_t val) : value(val) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    int32_t value;
};

class Int64Bit : public NumericLiteral {
public:
    explicit Int64Bit(int64_t val) : value(val) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    int64_t value;
};

class Float32Bit : public NumericLiteral {
public:
    explicit Float32Bit(float val) : value(val) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    float value;
};

class Float64Bit : public NumericLiteral {
public:
    explicit Float64Bit(double val) : value(val) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    double value;
};
    

// Arbitrary-precision integer (BigInt)
class BigInt : public NumericLiteral {
public:
    BigInt(const std::string& value) : value(value) {}
    llvm::Value* codegen(IRGenerator& generator);
    std::string value;
};

// ============================== Other Literals ============================== //

class StringLiteral : public Literal {
public:
    std::string value;

    explicit StringLiteral(std::string val) : value(std::move(val)) {}
    llvm::Value* codegen(IRGenerator& generator) override;
};

class BoolLiteral : public Literal {
public:
    bool value;

    explicit BoolLiteral(bool val) : value(std::move(val)) {}
    llvm::Value* codegen(IRGenerator& generator) override;
};
        
// ============================== Assignments ============================== //
// Assignments
class createVariable : public NamedStatement {
public:
    createVariable(const std::string &variable, llvm::Type* type, std::shared_ptr<Statement> value);
    std::string getName() override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;

private:
    std::string variable;
    llvm::Type* type;
    std::shared_ptr<Statement> value;
};

class createConstant : public NamedStatement {
public:
    createConstant(const std::string &variable, llvm::Type* type, std::shared_ptr<Statement> value);
    std::string getName() override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;

private:
    std::string variable;
    llvm::Type* type;
    std::shared_ptr<Statement> value;
};

class createDynamicVariable : public NamedStatement {
public:
    createDynamicVariable(const std::string &variable, std::shared_ptr<Statement> value);
    std::string getName() override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;

private:
    std::string variable;
    std::shared_ptr<Statement> value;
};


// Variable Retrieval
class GetVariable : public NamedStatement {
public:
    GetVariable(const std::string &variable);
    std::string getName() override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;

private:
    std::string variable;
};

class GetDynamicVariable : public NamedStatement {
public:
    GetDynamicVariable(const std::string &variable);
    std::string getName() override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;

private:
    std::string variable;
};
    

class GenericAssignment : public NamedStatement {
public:
    GenericAssignment(const std::string &variable, std::shared_ptr<Statement> value) :
        variable(variable), value(value) {}
    std::string getName() override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;

private:
    std::string variable;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Statement> tempValue;
};

class ReturnStatement : public Statement {
public:
    ReturnStatement(std::shared_ptr<Statement> value)
        : returnValue(value) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::shared_ptr<Statement> returnValue; // Store the return value

    // std::vector<std::optional<std::shared_ptr<Statement>>> variableReturnValues;
    std::shared_ptr<Statement> variableReturnValue;
    
};


class FunctionCallStatement : public Statement {
public:
    FunctionCallStatement(
            std::shared_ptr<Statement> func,
            std::vector<std::shared_ptr<Statement>> args,
            std::string baseName = "",
            std::string specializedName = "",
            std::vector<std::string> types = {}
            ) : 
            
            func(func), args(std::move(args)), types(types), baseName(baseName), specializedName(specializedName) {}

        llvm::Value* codegen(IRGenerator& generator) override;
    
    private:
        std::shared_ptr<Statement> func;
        std::vector<std::shared_ptr<Statement>> args;
        std::vector<std::string> types;
        std::string baseName;
        std::string specializedName;
};

class BlockStatement : public Statement {
public:
    BlockStatement(std::vector<std::shared_ptr<Statement>> statements = {})
    : statements(std::move(statements)) {}

    llvm::Value* codegen(IRGenerator& generator) override;

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
    
    llvm::Value* codegen(IRGenerator& generator) override;
    
    std::shared_ptr<Statement> condition;
    std::vector<std::shared_ptr<Statement>> body;
    std::vector<std::shared_ptr<IfStatement>> branches;
    std::vector<std::shared_ptr<Statement>> falseBranch;


    bool conditionIsMet(SymbolTable &scope);

    
};


// Binary expression statement
class BinaryExpression : public Statement {
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
    llvm::Value* codegen(IRGenerator& generator) override;
    
    

private:
    std::shared_ptr<Statement> left;
    TokenTypes op;
    std::shared_ptr<Statement> right;

};

class UnaryExpression : public Statement {
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
    llvm::Value* codegen(IRGenerator& generator) override;
    


private:
    std::shared_ptr<Statement> left;
    TokenTypes op;
    std::shared_ptr<Statement> right;

};


class TenaryExpression : public Statement {
    public:
        TenaryExpression(std::shared_ptr<Statement> condition, std::shared_ptr<Statement> truthy, std::shared_ptr<Statement> falsey) :
        condition(condition), truthy(truthy), falsey(falsey) {}

    llvm::Value* codegen(IRGenerator& generator) override;

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

    llvm::Value* codegen(IRGenerator& generator) override;
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
class CallMethod : public Statement {
public:
    CallMethod(std::shared_ptr<Statement> object, const std::string& methodName, std::vector<std::shared_ptr<Statement>> args)
        : object(object), methodName(methodName), arguments(std::move(args)) {}
    
    
    llvm::Value* codegen(IRGenerator& generator) override;
    
private:
    std::shared_ptr<Statement> object; // The base object on which the method is called.
    std::string methodName; // The method name.
    std::vector<std::shared_ptr<Statement>> arguments; // The arguments to the method.

};

class GetProperty : public Statement {
private:
    std::shared_ptr<Statement> object; // The base object on which the method is called.
    std::string propertyName; // The method name.
    std::vector<std::shared_ptr<Statement>> arguments; // The arguments to the method.

public:
    GetProperty(std::shared_ptr<Statement> object, const std::string& propertyName)
        : object(object), propertyName(propertyName) {}
    
    
    llvm::Value* codegen(IRGenerator& generator) override;
    
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
    llvm::Value* codegen(IRGenerator& generator) override;
    
};

class BreakStatement : public Statement {
    public:
        llvm::Value* codegen(IRGenerator& generator) override;
};

class ContinueStatement : public Statement {
public:
    llvm::Value* codegen(IRGenerator& generator) override;
};

class ObjectConstructorStatement : public Statement {
private:
    std::shared_ptr<Object> obj;
    std::vector<std::shared_ptr<Statement>> constructorArgs;

public:
    ObjectConstructorStatement(std::shared_ptr<Object> obj,
                               std::vector<std::shared_ptr<Statement>> args = {})
        : obj(obj), constructorArgs(std::move(args)) {}

    llvm::Value* codegen(IRGenerator& generator) override;
    
};

class ObjectDestructorStatement : public Statement {
private:
    std::string variableName; // The variable holding the object reference

public:
    explicit ObjectDestructorStatement(const std::string& variableName)
        : variableName(variableName) {}
        llvm::Value* codegen(IRGenerator& generator) override;
    
};